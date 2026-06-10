// ---------------------------------------------------------------------------
//                               main.cpp
// ---------------------------------------------------------------------------
//  Responsibilities:
//      config.hpp  — contains all pins, geometry, thresholds
//      electronics — common base class
//      ultrasonic  — HC-SR04 distance (metres)
//      motor       — Cytron MDD10A driver
//      encoder     — quadrature hall encoder (tick counter + get_vel)
//      main.cpp    — micro-ROS and the 100 Hz control loop
//
//   Control mode: Closed loop — feedforward + PID velocity control
//    Feedforward: duty = vel / V_MAX_MPS  (handles ~80% of the work)
//    PID corrects the residual error between setpoint and encoder velocity.
// ---------------------------------------------------------------------------

#include "pico/stdlib.h"
#include "pico/time.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "pico_uart_transports.h"
}

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <geometry_msgs/msg/twist.h>
#include <sensor_msgs/msg/range.h>
#include <rosidl_runtime_c/string_functions.h>

#include "config.hpp"
#include "ultrasonic.hpp"
#include "motor.hpp"
#include "encoder.hpp"
#include "pid.hpp"

#define RCCHECK(fn) { rcl_ret_t rc = (fn); \
    if (rc != RCL_RET_OK) { printf("RCL error %ld at %s:%d\n", rc, __FILE__, __LINE__); } }


// ---------------------------------------------------------------------------
//                             micro-ROS handles
// ---------------------------------------------------------------------------
rcl_subscription_t cmd_vel_sub;
rcl_publisher_t usrm_front_pub, usrm_back_pub, usrm_left_pub, usrm_right_pub;
rcl_publisher_t enc_left_pub, enc_right_pub;

geometry_msgs__msg__Twist cmd_vel_msg;
sensor_msgs__msg__Range usrm_front_msg, usrm_back_msg, usrm_left_msg, usrm_right_msg;
std_msgs__msg__Int32 enc_left_msg, enc_right_msg;

rclc_executor_t executor;
rclc_support_t  support;
rcl_allocator_t allocator;
rcl_node_t node;

// ---------------------------------------------------------------------------
//                         Shared state — cmd_vel
// ---------------------------------------------------------------------------
float target_vel_l_mps = 0.0f;
float target_vel_r_mps = 0.0f;
uint64_t last_cmd_vel_time_us = 0;

// ---------------------------------------------------------------------------
//                           cmd_vel callback
// ---------------------------------------------------------------------------
void cmd_vel_callback(const void* msg_in)
{
    const auto* msg = (const geometry_msgs__msg__Twist*)msg_in;

    float v_mps = (float)msg->linear.x;     // in m/s
    float w_rps = (float)msg->angular.z;    // in rad/s

    target_vel_l_mps     = v_mps - w_rps * (WHEEL_BASE_M * 0.5f);
    target_vel_r_mps     = v_mps + w_rps * (WHEEL_BASE_M * 0.5f);
    last_cmd_vel_time_us = time_us_64();
}

// ---------------------------------------------------------------------------
//                               Helpers
// ---------------------------------------------------------------------------
static void init_range_msg(sensor_msgs__msg__Range* msg, uint8_t rad_type,
                        float fov, float min_r, float max_r)
{
    msg->radiation_type = rad_type;
    msg->field_of_view  = fov;
    msg->min_range = min_r;
    msg->max_range = max_r;
    msg->range  = 0.0f;
}

static void set_msg_stamp(std_msgs__msg__Header* header)
{
    uint64_t now_us = time_us_64();
    header->stamp.sec = now_us / 1000000ULL;
    header->stamp.nanosec = (now_us % 1000000ULL) * 1000ULL;
}

// Used for clamping duty cycle after feedforward + PID summation for
// saturating a value to a closed interval
static inline float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// ---------------------------------------------------------------------------
//                                   main
// ---------------------------------------------------------------------------
int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // ---- Hardware (core 0 owned) ----
    Ultrasonic USRM_FRONT("Front", "ON", USRM_FRONT_TRIG, USRM_FRONT_ECHO);
    Ultrasonic USRM_BACK ("Back",  "ON", USRM_BACK_TRIG,  USRM_BACK_ECHO );
    Ultrasonic USRM_RIGHT("Right", "ON", USRM_RIGHT_TRIG, USRM_RIGHT_ECHO);
    Ultrasonic USRM_LEFT ("Left",  "ON", USRM_LEFT_TRIG,  USRM_LEFT_ECHO );

    Motor      MOTOR("MOTORS", "ON", L_DIR, L_PWM, R_DIR, R_PWM,
                    /*invert_left*/  false,
                    /*invert_right*/ false,
                    /*trim_left*/    LEFT_MOTOR_TRIM,
                    /*trim_right*/   RIGHT_MOTOR_TRIM);

    Encoder    LEFT_ENCODER ("L_ENC", "ON",
                            ENC_L_A, ENC_L_B,
                            GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                            /*invert*/ true);
    Encoder    RIGHT_ENCODER("R_ENC", "ON",
                            ENC_R_A, ENC_R_B,
                            GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                            /*invert*/ false);

    PID pid_l(KP_L, KI_L, KD_L, -PID_OUT_MAX, PID_OUT_MAX, 0.3f);
    PID pid_r(KP_R, KI_R, KD_R, -PID_OUT_MAX, PID_OUT_MAX, 0.3f);

    USRM_FRONT.ShowStatus(); USRM_BACK.ShowStatus();
    USRM_RIGHT.ShowStatus(); USRM_LEFT.ShowStatus();
    MOTOR.ShowStatus();
    LEFT_ENCODER.ShowStatus(); RIGHT_ENCODER.ShowStatus();

    // ---- micro-ROS init ----
    rmw_uros_set_custom_transport(
        true, NULL,
        pico_serial_transport_open,  pico_serial_transport_close,
        pico_serial_transport_write, pico_serial_transport_read);

    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "pico", "", &support);

    rclc_subscription_init_default(
        &cmd_vel_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel");

    rclc_publisher_init_default(&usrm_front_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_front");
    rclc_publisher_init_default(&usrm_back_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_back");
    rclc_publisher_init_default(&usrm_left_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_left");
    rclc_publisher_init_default(&usrm_right_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_right");
    rclc_publisher_init_default(&enc_left_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/sensors/encoders/left_ticks");
    rclc_publisher_init_default(&enc_right_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/sensors/encoders/right_ticks");

    init_range_msg(&usrm_front_msg, 0, 0.26f, 0.02f, 4.00f);
    init_range_msg(&usrm_back_msg,  0, 0.26f, 0.02f, 4.00f);
    init_range_msg(&usrm_left_msg,  0, 0.26f, 0.02f, 4.00f);
    init_range_msg(&usrm_right_msg, 0, 0.26f, 0.02f, 4.00f);

    rosidl_runtime_c__String__assign(&usrm_front_msg.header.frame_id, "ultrasonic_front_link");
    rosidl_runtime_c__String__assign(&usrm_back_msg.header.frame_id,  "ultrasonic_back_link");
    rosidl_runtime_c__String__assign(&usrm_left_msg.header.frame_id,  "ultrasonic_left_link");
    rosidl_runtime_c__String__assign(&usrm_right_msg.header.frame_id, "ultrasonic_right_link");

    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(
        &executor, &cmd_vel_sub, &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);

    last_cmd_vel_time_us = time_us_64();
    printf("micro-ROS ready, listening on /cmd_vel\n");

    // -----------------------------------------------------------------------
    //                              Main loop @ 100 Hz   (CORE 0)
    // -----------------------------------------------------------------------
    uint8_t  usrm_index = 0;
    uint8_t  usrm_div_counter = 0;

    constexpr uint64_t LOOP_PERIOD_US = 10000ULL;  // 100 Hz = 10 ms

    while (true)
    {
        // ===== 0. Mark the start of this control tick =====
        uint64_t loop_start = time_us_64();

        // ===== 1. Pump the ROS executor =====
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(2));

        // ===== 2a. Fire one ultrasonic per Nth tick (round-robin) =====
        //  Spaced out so a blocking HC-SR04 echo doesn't corrupt PID timing
        //  every loop.
        if (usrm_div_counter == 0) {
            switch (usrm_index) {
                case 0:
                    usrm_front_msg.range = USRM_FRONT.update();
                    set_msg_stamp(&usrm_front_msg.header);
                    RCCHECK(rcl_publish(&usrm_front_pub, &usrm_front_msg, NULL));
                    break;
                case 1:
                    usrm_back_msg.range = USRM_BACK.update();
                    set_msg_stamp(&usrm_back_msg.header);
                    RCCHECK(rcl_publish(&usrm_back_pub, &usrm_back_msg, NULL));
                    break;
                case 2:
                    usrm_right_msg.range = USRM_RIGHT.update();
                    set_msg_stamp(&usrm_right_msg.header);
                    RCCHECK(rcl_publish(&usrm_right_pub, &usrm_right_msg, NULL));
                    break;
                case 3:
                    usrm_left_msg.range = USRM_LEFT.update();
                    set_msg_stamp(&usrm_left_msg.header);
                    RCCHECK(rcl_publish(&usrm_left_pub, &usrm_left_msg, NULL));
                    break;
            }
            usrm_index = (usrm_index + 1) % 4;
        }
        usrm_div_counter = (usrm_div_counter + 1) % USRM_DIVIDER;

        // ===== 3. Snapshot setpoint + cmd_vel watchdog =====
        //  IMPORTANT: compare against time_us_64() (current time), NOT
        //  loop_start.  rclc_executor_spin_some() above can deliver a fresh
        //  cmd_vel and stamp last_cmd_vel_time_us with a value > loop_start.
        //  With uint64_t arithmetic, loop_start - last_cmd_vel_time_us would
        //  then underflow to ~2^64 and spuriously trigger the watchdog,
        //  zeroing every fresh command.  Using "now" instead also correctly
        //  charges any time spent inside spin_some against the watchdog.
        float vel_l = target_vel_l_mps;
        float vel_r = target_vel_r_mps;
        if ((time_us_64() - last_cmd_vel_time_us) > CMD_VEL_TIMEOUT_US) {
            vel_l = 0.0f;
            vel_r = 0.0f;
        }

        // ===== 4. Directional safety E-stop =====
        float v_avg         = 0.5f * (vel_l + vel_r);
        bool  want_forward  = (v_avg >  DIRECTION_DEADZONE_MPS);
        bool  want_backward = (v_avg < -DIRECTION_DEADZONE_MPS);
        float d_front       = usrm_front_msg.range;
        float d_back        = usrm_back_msg.range;
        if (want_forward  && d_front > 0.0f && d_front < FRONT_STOP_DIST_M) {
            vel_l = 0.0f; vel_r = 0.0f;
        }
        if (want_backward && d_back  > 0.0f && d_back  < BACK_STOP_DIST_M) {
            vel_l = 0.0f; vel_r = 0.0f;
        }

        // ===== 5. Feedforward + PID velocity control =====
        float meas_l = LEFT_ENCODER.get_vel();
        float meas_r = RIGHT_ENCODER.get_vel();

        float duty_l, duty_r;
        if (vel_l == 0.0f && vel_r == 0.0f) {
            pid_l.reset();
            pid_r.reset();
            duty_l = 0.0f;
            duty_r = 0.0f;
        } else {
            float ff_l = clampf(vel_l / V_MAX_MPS, -1.0f, 1.0f);
            float ff_r = clampf(vel_r / V_MAX_MPS, -1.0f, 1.0f);
            // duty_l = clampf(ff_l + pid_l.calculate(vel_l, meas_l), -1.0f, 1.0f); FOR PID LOOP
            // duty_r = clampf(ff_r + pid_r.calculate(vel_r, meas_r), -1.0f, 1.0f); FOR PID LOOP
            
            duty_l = ff_l   // just pass feedforward directly to duty
            duty_r = ff_r
            
            // duty_l = 0.3f;
            // duty_r = 0.3f;
        }

        // ===== 6. Drive motors =====
        MOTOR.move(duty_l, duty_r);

        // ===== 7. Publish encoder ticks =====
        enc_left_msg .data = LEFT_ENCODER .get_count();
        enc_right_msg.data = RIGHT_ENCODER.get_count();
        RCCHECK(rcl_publish(&enc_left_pub,  &enc_left_msg,  NULL));
        RCCHECK(rcl_publish(&enc_right_pub, &enc_right_msg, NULL));

        // ===== 8. Pace at 100 Hz =====
        //  Busy-wait until 10 ms has elapsed since loop_start.  If the work
        //  above ever overruns 10 ms (e.g. an exceptionally slow ultrasonic),
        //  we skip the wait and run straight into the next tick — there is
        //  no time compensation, by design.  Watch for this with a logic
        //  analyser on GP_x if you suspect overruns.
        while ((time_us_64() - loop_start) < LOOP_PERIOD_US) {
            tight_loop_contents();
        }
    }
    return 0;
}