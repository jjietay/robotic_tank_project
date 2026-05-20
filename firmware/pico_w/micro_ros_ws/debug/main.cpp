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

#include "config.hpp"
#include "motor.hpp"

// ---- globals ----
Motor* g_motor = nullptr;

void cmd_vel_callback(const void* msg_in)
{
    const auto* msg = (const geometry_msgs__msg__Twist*)msg_in;

    float v = (float)msg->linear.x;
    float w = (float)msg->angular.z;

    float vel_l = v - w * (WHEEL_BASE_M * 0.5f);
    float vel_r = v + w * (WHEEL_BASE_M * 0.5f);

    float duty_l = vel_l / V_MAX_MPS;
    float duty_r = vel_r / V_MAX_MPS;

    if (duty_l >  1.0f) duty_l =  1.0f;
    if (duty_l < -1.0f) duty_l = -1.0f;
    if (duty_r >  1.0f) duty_r =  1.0f;
    if (duty_r < -1.0f) duty_r = -1.0f;

    g_motor->move(duty_l, duty_r);
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
    g_motor = &motor;

    // ---- micro-ROS ----
    rmw_uros_set_custom_transport(
        true, NULL,
        pico_serial_transport_open,  pico_serial_transport_close,
        pico_serial_transport_write, pico_serial_transport_read);

    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t  support;
    rclc_support_init(&support, 0, NULL, &allocator);

    rcl_node_t node;
    rclc_node_init_default(&node, "pico_debug", "", &support);

    rcl_subscription_t sub;
    geometry_msgs__msg__Twist msg;
    rclc_subscription_init_default(
        &sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel");

    rclc_executor_t executor;
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(&executor, &sub, &msg, &cmd_vel_callback, ON_NEW_DATA);

    printf("debug: listening on /cmd_vel\n");

    while (true)
    {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    }
}
