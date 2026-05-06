// ---------------------------------------------------------------------------
//                       main.cpp  —  micro-ROS robot core loop
// ---------------------------------------------------------------------------
//  Responsibility split:
//    • config.hpp     — pins / geometry / gains
//    • electronics.*  — common base class
//    • ultrasonic.*   — HC-SR04 distance (METRES)
//    • motor.*        — Cytron MDD10A duty-cycle driver
//    • encoder.*      — quadrature wheel encoder (m/s, sign-corrected)
//    • pid.*          — discrete PID with anti-windup
//    • main.cpp       — micro-ROS plumbing and the 100 Hz control loop
//
//  Units convention:  ALL velocities are m/s, ALL distances are metres.
//  No [-1, 1] velocity normalisation anywhere.  The only [-1, 1] saturation
//  in the codebase is on PWM duty cycle, which is a hardware physical limit.
// ---------------------------------------------------------------------------

#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>

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
//                              micro-ROS handles
// ---------------------------------------------------------------------------
rcl_subscription_t cmd_vel_sub;
rcl_publisher_t    usrm_front_pub, usrm_back_pub, usrm_left_pub, usrm_right_pub;
rcl_publisher_t    enc_left_pub,   enc_right_pub;

geometry_msgs__msg__Twist cmd_vel_msg;
sensor_msgs__msg__Range   usrm_front_msg, usrm_back_msg, usrm_left_msg, usrm_right_msg;
std_msgs__msg__Int32      enc_left_msg, enc_right_msg;

rclc_executor_t executor;
rclc_support_t  support;
rcl_allocator_t allocator;
rcl_node_t      node;

// ---------------------------------------------------------------------------
//                              Shared state
// ---------------------------------------------------------------------------
//  Written by cmd_vel_callback (which runs synchronously inside
//  rclc_executor_spin_some on this same core), read by the main loop.
//  `volatile` is defensive — there is no real cross-core access here.
volatile float    target_vel_l_mps     = 0.0f;
volatile float    target_vel_r_mps     = 0.0f;
volatile uint64_t last_cmd_vel_time_us = 0;

// ---------------------------------------------------------------------------
//                              cmd_vel callback
// ---------------------------------------------------------------------------
//  ROS REP-103 convention:
//      msg->linear.x   forward speed         [m/s]
//      msg->angular.z  yaw rate (CCW = +)    [rad/s]
//
//  Differential-drive kinematics:
//      v_left  = v - ω · (L/2)
//      v_right = v + ω · (L/2)
//
//  No clamping / normalisation is applied — values pass through as raw m/s.
//  PID will saturate at the actuator boundary if the request is unreachable.
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
//                              Helpers
// ---------------------------------------------------------------------------
static void init_range_msg(sensor_msgs__msg__Range* msg, uint8_t rad_type,
                        float fov, float min_r, float max_r)
{
    msg->radiation_type = rad_type;
    msg->field_of_view  = fov;
    msg->min_range      = min_r;
    msg->max_range      = max_r;
    msg->range          = 0.0f;
}

static void set_msg_stamp(std_msgs__msg__Header* header)
{
    uint64_t now_us = time_us_64();
    header->stamp.sec     = now_us / 1000000ULL;
    header->stamp.nanosec = (now_us % 1000000ULL) * 1000ULL;
}

// ---------------------------------------------------------------------------
//                                   main
// ---------------------------------------------------------------------------
int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // ---- Hardware ----
    Ultrasonic USRM_FRONT("Front", "ON", USRM_FRONT_TRIG, USRM_FRONT_ECHO);
    Ultrasonic USRM_BACK ("Back",  "ON", USRM_BACK_TRIG,  USRM_BACK_ECHO );
    Ultrasonic USRM_RIGHT("Right", "ON", USRM_RIGHT_TRIG, USRM_RIGHT_ECHO);
    Ultrasonic USRM_LEFT ("Left",  "ON", USRM_LEFT_TRIG,  USRM_LEFT_ECHO );

    Motor      MOTOR("MOTORS", "ON", L_DIR, L_PWM, R_DIR, R_PWM,
                    /*invert_left*/  true,
                    /*invert_right*/ false);

    Encoder    LEFT_ENCODER ("L_ENC", "ON",
                            ENC_L_A, ENC_L_B,
                            GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                            /*invert*/ true);   // matches MOTOR's left inversion
    Encoder    RIGHT_ENCODER("R_ENC", "ON",
                            ENC_R_A, ENC_R_B,
                            GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                            /*invert*/ false);

    // PID per wheel — m/s setpoint vs m/s measured -> duty cycle [-1, 1]
    PID LEFT_PID (PID_KP, PID_KI, PID_KD, -1.0f, 1.0f, 1.0f);
    PID RIGHT_PID(PID_KP, PID_KI, PID_KD, -1.0f, 1.0f, 1.0f);

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
    //                              Main loop @ 100 Hz
    // -----------------------------------------------------------------------
    uint64_t loop_last  = 0;
    uint8_t  usrm_index = 0;

    while (true)
    {
        // 0. Pace at 100 Hz
        uint64_t now = time_us_64();
        if (now - loop_last < 10000ULL) {
            tight_loop_contents();
            continue;
        }
        loop_last = now;

        // 1. Pump the ROS executor
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(2));

        // 2. Fire one ultrasonic per tick (round-robin, avoids crosstalk)
        switch (usrm_index) {
            case 0:
                usrm_front_msg.range = USRM_FRONT.update();        // METRES
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

        // 3. Snapshot setpoint (m/s).  Apply cmd_vel watchdog timeout.
        float setpoint_l = target_vel_l_mps;
        float setpoint_r = target_vel_r_mps;
        if (now - last_cmd_vel_time_us > CMD_VEL_TIMEOUT_US) {
            setpoint_l = 0.0f;
            setpoint_r = 0.0f;
        }

        // 4. Directional safety E-stop  (METRES vs METRES, no unit hacks)
        float v_avg        = 0.5f * (setpoint_l + setpoint_r);
        bool  want_forward  = (v_avg >  DIRECTION_DEADZONE_MPS);
        bool  want_backward = (v_avg < -DIRECTION_DEADZONE_MPS);
        float d_front       = usrm_front_msg.range;
        float d_back        = usrm_back_msg.range;
        if (want_forward  && d_front > 0.0f && d_front < FRONT_STOP_DIST_M) {
            setpoint_l = 0.0f; setpoint_r = 0.0f;
        }
        if (want_backward && d_back  > 0.0f && d_back  < BACK_STOP_DIST_M) {
            setpoint_l = 0.0f; setpoint_r = 0.0f;
        }

        // 5. Closed-loop velocity control
        //    measured (m/s) ←-- encoder        setpoint (m/s) ←-- cmd_vel
        //                       \             /
        //                        +--- PID ---+ ---> duty cycle ∈ [-1, 1]
        float meas_l = LEFT_ENCODER .get_vel();
        float meas_r = RIGHT_ENCODER.get_vel();

        float duty_l, duty_r;
        if (setpoint_l == 0.0f && setpoint_r == 0.0f) {
            // Commanded stop — drop integrator so we don't creep
            LEFT_PID .reset();
            RIGHT_PID.reset();
            duty_l = 0.0f;
            duty_r = 0.0f;
        } else {
            duty_l = LEFT_PID .calculate(setpoint_l, meas_l);
            duty_r = RIGHT_PID.calculate(setpoint_r, meas_r);
        }

        // 6. Drive motors (Motor handles wiring polarity internally)
        MOTOR.move(duty_l, duty_r);

        // 7. Publish encoder ticks (sign-corrected by Encoder class itself)
        enc_left_msg .data = LEFT_ENCODER .get_count();
        enc_right_msg.data = RIGHT_ENCODER.get_count();
        RCCHECK(rcl_publish(&enc_left_pub,  &enc_left_msg,  NULL));
        RCCHECK(rcl_publish(&enc_right_pub, &enc_right_msg, NULL));
    }
    return 0;
}
