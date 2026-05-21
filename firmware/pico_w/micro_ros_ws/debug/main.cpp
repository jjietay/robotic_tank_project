#include "pico/stdlib.h"
#include "pico/time.h"
#include <math.h>
#include <stdio.h>

extern "C" {
#include "pico_uart_transports.h"
}

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/int32.h>

#include "config.hpp"
#include "motor.hpp"
#include "encoder.hpp"
#include "pid.hpp"

// ---------------------------------------------------------------------------
//  PID + feedforward tuning — kept separate so they're easy to adjust.
//
//  Feedforward handles ~80% of the work; PID only corrects the residual.
//  Keeping gains low prevents oscillation at slow speeds.
//
//  If sluggish:   raise KP first, then KI slightly.
//  If oscillates: lower KP, then KD.
// ---------------------------------------------------------------------------
static constexpr float KP          = 0.8f;   // proportional
static constexpr float KI          = 0.5f;   // integral
static constexpr float KD          = 0.02f;  // derivative
static constexpr float PID_OUT_MAX = 0.25f;  // PID can only nudge ±25% duty

// ---------------------------------------------------------------------------
//  Shared cmd_vel state (written by ROS callback, read by control loop)
// ---------------------------------------------------------------------------
static volatile float    g_vel_l        = 0.0f;
static volatile float    g_vel_r        = 0.0f;
static volatile uint64_t g_last_cmd_us  = 0;      // timestamp of last cmd_vel

static constexpr uint64_t CMD_TIMEOUT_US = 500000ULL;  // 500 ms watchdog

void cmd_vel_callback(const void* msg_in)
{
    const auto* msg = (const geometry_msgs__msg__Twist*)msg_in;
    float v = (float)msg->linear.x;
    float w = (float)msg->angular.z;
    g_vel_l       = v - w * (WHEEL_BASE_M * 0.5f);
    g_vel_r       = v + w * (WHEEL_BASE_M * 0.5f);
    g_last_cmd_us = time_us_64();
}

static inline float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    Motor motor("Motors", "ON",
                L_DIR, L_PWM, R_DIR, R_PWM,
                /*invert_left*/  true,
                /*invert_right*/ true,
                LEFT_MOTOR_TRIM,
                RIGHT_MOTOR_TRIM);

    Encoder enc_l("L_ENC", "ON",
                  ENC_L_A, ENC_L_B,
                  GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                  /*invert*/ false);
    Encoder enc_r("R_ENC", "ON",
                  ENC_R_A, ENC_R_B,
                  GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                  /*invert*/ true);

    PID pid_l(KP, KI, KD, -PID_OUT_MAX, PID_OUT_MAX, 0.3f);
    PID pid_r(KP, KI, KD, -PID_OUT_MAX, PID_OUT_MAX, 0.3f);

    // ---- micro-ROS ----
    rmw_uros_set_custom_transport(
        true, NULL,
        pico_serial_transport_open,  pico_serial_transport_close,
        pico_serial_transport_write, pico_serial_transport_read);

    rcl_allocator_t    allocator = rcl_get_default_allocator();
    rclc_support_t     support;
    rclc_support_init(&support, 0, NULL, &allocator);

    rcl_node_t node;
    rclc_node_init_default(&node, "pico_debug", "", &support);

    // ---- subscription ----
    rcl_subscription_t sub;
    geometry_msgs__msg__Twist twist_msg;
    rclc_subscription_init_default(
        &sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel");

    // ---- encoder publishers (same topics as production) ----
    rcl_publisher_t       enc_left_pub,  enc_right_pub;
    std_msgs__msg__Int32  enc_left_msg,  enc_right_msg;
    rclc_publisher_init_default(&enc_left_pub,  &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/sensors/encoders/left_ticks");
    rclc_publisher_init_default(&enc_right_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/sensors/encoders/right_ticks");

    rclc_executor_t executor;
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(&executor, &sub, &twist_msg, &cmd_vel_callback, ON_NEW_DATA);

    printf("pico_debug: PID+feedforward, listening on /cmd_vel\n");
    printf("Publishing encoder ticks on /sensors/encoders/{left,right}_ticks\n");
    printf("500 ms watchdog — motors stop if no cmd_vel received.\n\n");

    constexpr uint64_t LOOP_US     = 10000ULL;  // 100 Hz control
    constexpr uint32_t PRINT_EVERY = 20;         // print at 5 Hz
    uint32_t tick = 0;

    // Seed watchdog so we don't trip immediately on startup
    g_last_cmd_us = time_us_64();

    while (true)
    {
        uint64_t t0 = time_us_64();

        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(2));

        // ---- Watchdog: zero setpoints if cmd_vel has gone silent ----
        float sp_l = g_vel_l;
        float sp_r = g_vel_r;
        if ((t0 - g_last_cmd_us) > CMD_TIMEOUT_US) {
            sp_l = 0.0f;
            sp_r = 0.0f;
        }

        float meas_l = enc_l.get_vel();
        float meas_r = enc_r.get_vel();

        float duty_l, duty_r;

        if (sp_l == 0.0f && sp_r == 0.0f) {
            // Explicit stop — bypass PID, reset integrators
            pid_l.reset();
            pid_r.reset();
            duty_l = 0.0f;
            duty_r = 0.0f;
        } else {
            // Feedforward + PID correction
            float ff_l = clampf(sp_l / V_MAX_MPS, -1.0f, 1.0f);
            float ff_r = clampf(sp_r / V_MAX_MPS, -1.0f, 1.0f);
            float pid_out_l = pid_l.calculate(sp_l, meas_l);
            float pid_out_r = pid_r.calculate(sp_r, meas_r);
            duty_l = clampf(ff_l + pid_out_l, -1.0f, 1.0f);
            duty_r = clampf(ff_r + pid_out_r, -1.0f, 1.0f);
        }

        motor.move(duty_l, duty_r);

        // ---- Publish encoder tick counts ----
        enc_left_msg.data  = enc_l.get_count();
        enc_right_msg.data = enc_r.get_count();
        rcl_publish(&enc_left_pub,  &enc_left_msg,  NULL);
        rcl_publish(&enc_right_pub, &enc_right_msg, NULL);

        // ---- Serial print at 5 Hz ----
        if (++tick >= PRINT_EVERY) {
            tick = 0;
            printf("sp(L:%+.2f R:%+.2f) meas(L:%+.3f R:%+.3f) duty(L:%+.2f R:%+.2f)\n",
                   sp_l, sp_r, meas_l, meas_r, duty_l, duty_r);
        }

        while ((time_us_64() - t0) < LOOP_US) tight_loop_contents();
    }
}
