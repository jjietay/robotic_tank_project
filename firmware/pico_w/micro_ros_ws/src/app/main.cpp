#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include <stdio.h>
#include <string>
#include <cmath>
#include <algorithm>

// micro-ROS headers
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>

// Constants
constexpr float DRIVE_DUTY = 0.85f;
constexpr float TURN_DUTY  = 0.80f;
constexpr uint  PWM_TOP    = 65535;

constexpr uint ENC_L_A = 16, ENC_L_B = 17;
constexpr uint ENC_R_A = 18, ENC_R_B = 19;
constexpr uint L_DIR = 0,  L_PWM = 8;
constexpr uint R_DIR = 2,  R_PWM = 9;
constexpr uint USRM_T_TRIG = 6,  USRM_T_ECHO = 7;
constexpr uint USRM_B_TRIG = 10, USRM_B_ECHO = 11;
constexpr uint USRM_R_TRIG = 12, USRM_R_ECHO = 13;
constexpr uint USRM_L_TRIG = 14, USRM_L_ECHO = 15;


// ---------------------------------------------------------
//                      Base Class
// ---------------------------------------------------------
class Electronics
{
protected:
    std::string name;
    std::string status;
public:
    Electronics(std::string device_name, std::string device_status)
        : name(device_name), status(device_status) {}

    void ShowStatus()  { printf("%s initialised.\n",  name.c_str()); }
    void CheckStatus() { printf("%s disconnected.\n", name.c_str()); }
};


// ---------------------------------------------------------
//                       Ultrasonic
// ---------------------------------------------------------
class Ultrasonic : public Electronics
{
private:
    uint  trigger_pin, echo_pin;
    float sound_vel;

public:
    Ultrasonic(std::string name, std::string status,
    uint trig, uint echo, float vel = 340.0f)
        : Electronics(name, status),
        trigger_pin(trig), echo_pin(echo), sound_vel(vel)
    {
        gpio_init(trigger_pin); gpio_set_dir(trigger_pin, GPIO_OUT);
        gpio_init(echo_pin);    gpio_set_dir(echo_pin,    GPIO_IN);
    }

    float distance()
    {
        gpio_put(trigger_pin, 0); sleep_us(2);
        gpio_put(trigger_pin, 1); sleep_us(10);
        gpio_put(trigger_pin, 0);

        uint64_t timeout = time_us_64();
        while (!gpio_get(echo_pin))
        {
            if ((time_us_64() - timeout) > 38000) return -1.0f;
        }

        uint64_t time1 = time_us_64();
        while (gpio_get(echo_pin))
        {           
            if ((time_us_64() - timeout) > 38000) return -2.0f;
        }
        uint64_t time2 = time_us_64();
        uint64_t diff  = time2 - time1;
        if (diff > 38000) return -2.0f;

        return (float)(diff * 1e-6f * sound_vel / 2.0f) * 100.0f;
    }
};


// ---------------------------------------------------------
//                 Motor (Cytron MDD10A) 
// ---------------------------------------------------------
class Motor : public Electronics
{
private:
    uint l_dir_pin, l_pwm_pin;
    uint r_dir_pin, r_pwm_pin;

    void setup_pwm(uint pin)
    {          
        gpio_set_function(pin, GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(pin);
        pwm_set_wrap(slice, PWM_TOP);
        pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), 0);
        pwm_set_enabled(slice, true);
    }

    void _set(uint dir_pin, uint pwm_pin, float speed)
    {
        speed = std::max(-1.0f, std::min(1.0f, speed)); 
        gpio_put(dir_pin, speed > 0 ? 1 : 0);
        pwm_set_gpio_level(pwm_pin, (uint16_t)(fabsf(speed) * PWM_TOP));
    }

public:
    Motor(std::string name, std::string status, uint l_dir, uint l_pwm, uint r_dir, uint r_pwm)
        : Electronics(name, status),
        l_dir_pin(l_dir), l_pwm_pin(l_pwm),    
        r_dir_pin(r_dir), r_pwm_pin(r_pwm)    
    {
        gpio_init(l_dir_pin); gpio_set_dir(l_dir_pin, GPIO_OUT);
        gpio_init(r_dir_pin); gpio_set_dir(r_dir_pin, GPIO_OUT);
        setup_pwm(l_pwm_pin);
        setup_pwm(r_pwm_pin);
    }

    void move(float left, float right)
    {
        _set(l_dir_pin, l_pwm_pin, left);
        _set(r_dir_pin, r_pwm_pin, right);
    }

    void forward()         { move( DRIVE_DUTY,  DRIVE_DUTY); }
    void backward()        { move(-DRIVE_DUTY, -DRIVE_DUTY); }
    void tank_turn_left()  { move( TURN_DUTY,  -TURN_DUTY);  }
    void tank_turn_right() { move(-TURN_DUTY,   TURN_DUTY);  }
    void stop()            { move(0.0f, 0.0f);               }
};


// ---------------------------------------------------------
//                         Encoder 
// ---------------------------------------------------------
// 2 main things:
//  (1) IRQ/callback immediately when either pin A or B goes from 0 to 1 or vice versa
//  (2) get_vel purely gets the velocity
class Encoder : public Electronics
{
private:
    static constexpr int PPR = 11;
    static constexpr float PI = 3.141592653589793f;

    uint  pin_a, pin_b;
    float reduction_ratio, diameter, circumference, cpr_output;

    volatile int count = 0;         // because its modified inside an interrupt and read in main
    int last_count = 0;
    uint64_t last_time = 0;

    static Encoder* instances[2];   // instances[2] is a static array of pointers to Encoder
    // (look from backwards) An array of size 2, of the pointer "instances", points to the "Encoder" class
    static int instance_count;

    // Main guy who changes count values (number of times it passes A and B edges)
    static void irq_handler(uint gpio, uint32_t events)
    {
        for (int i = 0; i < instance_count; i++) {
            if (instances[i]) instances[i]->handle_irq(gpio, events);  
            // (instances[i]) is True if it has a value, else false
            //the pointer "instances[i]"" is calling the method "handle_irq"
            // that is the class method of the "Encoder" class (pointed at)
        }
    }

    void handle_irq(uint gpio, uint32_t /*events*/)
    {
        bool a = gpio_get(pin_a);
        bool b = gpio_get(pin_b);
        if (gpio == pin_a)
        {
            if (a == b) count--; else count++;
        }
        else
        {
            if (a == b) count++; else count--;
        }
    }

public:
    Encoder(std::string name, std::string status,
            uint _pin_a, uint _pin_b,
            float _reduction_ratio = 1.0f, float _diameter = 4.7f)
        : Electronics(name, status),
        pin_a(_pin_a), pin_b(_pin_b),
        reduction_ratio(_reduction_ratio), diameter(_diameter)
    {
        circumference = 2.0f * PI * (_diameter / 2.0f);
        cpr_output    = (4 * PPR) * _reduction_ratio;   // counts per rev after reduction ratio
        last_time     = time_us_64();

        // Pull up holds the pins at HIGH when no signal is present, this is to prevent floating inputs
        gpio_init(pin_a); gpio_set_dir(pin_a, GPIO_IN); gpio_pull_up(pin_a);
        gpio_init(pin_b); gpio_set_dir(pin_b, GPIO_IN); gpio_pull_up(pin_b);

        instances[instance_count++] = this;     // "this" points to the current object being constructed (initialized in main loop)
        gpio_set_irq_enabled_with_callback(pin_a,
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &Encoder::irq_handler); 
        gpio_set_irq_enabled(pin_b,
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true); // only start callback once, pin_b will follow
    }

    int get_count()
    {
    return count;
    }  

    float get_vel()
    {
        uint64_t now       = time_us_64();
        int64_t  time_diff = (int64_t)(now - last_time);
        if (time_diff <= 0) return 0.0f;   

        float dt  = time_diff * 1e-6f;    
        float vel = get_distance_delta() / dt;
        last_time  = now;
        last_count = count;
        return vel;
    }

private:
    float get_distance_delta()
    {
        int   count_diff    = count - last_count;
        float distance_diff = (count_diff / cpr_output) * circumference;
        return distance_diff;
    }
};

Encoder* Encoder::instances[2]  = {nullptr, nullptr}; // set instances[0] and instances[1] = nullptr
int      Encoder::instance_count = 0;


// ---------------------------------------------------------
//                           PID
// ---------------------------------------------------------
class PID
{
private:
    float    kp, ki, kd;
    float    integral = 0.0f;
    float    previous_error = 0.0f;
    uint64_t last_time;

public:
    PID(float kp, float ki, float kd)
        : kp(kp), ki(ki), kd(kd), last_time(time_us_64()) {} 

    float calculate(float setpoint, float current_value)
    {
        uint64_t current_time = time_us_64();
        float dt = (float)(current_time - last_time) * 1e-6f;
        if (dt <= 0.0f) dt = 0.01f;

        float error = setpoint - current_value;
        integral += error * dt;
        integral = std::max(-10.0f, std::min(10.0f, integral));

        float p_term = kp * error;
        float i_term = ki * integral;
        float d_term = kd * ((error - previous_error) / dt);

        previous_error = error;
        last_time = current_time;
        return p_term + i_term + d_term;
    }
};


// ---------------------------------------------------------
//                   Micro-ROS Globals 
// ---------------------------------------------------------
rcl_subscription_t cmd_vel_sub;         // Subscription Handle
geometry_msgs__msg__Twist cmd_vel_msg;  // Message Buffer
rclc_executor_t executor;               // Event loop manager
rclc_support_t support;                 // Support context
rcl_allocator_t allocator;              // Allows for custom heap management
rcl_node_t node;                        // Pico_node

// These are written by the ROS callback, read by the main loop
float target_vel_l = 0.0f;
float target_vel_r = 0.0f;

// Make it global for access to E stop
Ultrasonic* g_usrm[4];

// Called automatically by micro-ROS when RPi4 publishes to /cmd_vel
// linear.x  = forward/backward speed  (-1.0 to 1.0)
// angular.z = turning speed           (-1.0 to 1.0)
void cmd_vel_callback(const void* msg_in) {
    const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msg_in;

    float linear  = (float)msg->linear.x;
    float angular = (float)msg->angular.z;

    // Mix linear + angular → left/right wheel speeds (same as differential drive)
    target_vel_l = linear - angular;
    target_vel_r = linear + angular;

    // Safety: clamp
    target_vel_l = std::max(-1.0f, std::min(1.0f, target_vel_l));
    target_vel_r = std::max(-1.0f, std::min(1.0f, target_vel_r));

    // Emergency stop if any sensor < 3cm
    for (int i = 0; i < 4; i++) {
        float d = g_usrm[i]->distance();
        if (d > 0.0f && d < 3.0f) {
            target_vel_l = 0.0f;
            target_vel_r = 0.0f;
            break;
        }
    }
}


// ---------------------------------------------------------
//                          MAIN 
// ---------------------------------------------------------
int main() {
    stdio_init_all();

    Ultrasonic USRM_T("TL", "ON", USRM_T_TRIG, USRM_T_ECHO);
    Ultrasonic USRM_B("TR", "ON", USRM_B_TRIG, USRM_B_ECHO);
    Ultrasonic USRM_R("BL", "ON", USRM_R_TRIG, USRM_R_ECHO);
    Ultrasonic USRM_L("BR", "ON", USRM_L_TRIG, USRM_L_ECHO);
    Motor      MOTOR("MOTORS", "ON", L_DIR, L_PWM, R_DIR, R_PWM);
    Encoder    LEFT_ENCODER ("L_ENC", "ON", ENC_L_A, ENC_L_B);
    Encoder    RIGHT_ENCODER("R_ENC", "ON", ENC_R_A, ENC_R_B);
    PID        LEFT_PID (0.8f, 0.0f, 0.0f);
    PID        RIGHT_PID(0.8f, 0.0f, 0.0f);

    // Give sensor pointers to the callback
    g_usrm[0] = &USRM_T; g_usrm[1] = &USRM_B;
    g_usrm[2] = &USRM_R; g_usrm[3] = &USRM_L;

    USRM_T.ShowStatus(); USRM_B.ShowStatus();
    USRM_R.ShowStatus(); USRM_L.ShowStatus();
    MOTOR.ShowStatus();
    LEFT_ENCODER.ShowStatus(); RIGHT_ENCODER.ShowStatus();

    // micro-ROS init
    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "pico_node", "", &support);

    // Subscribe to /cmd_vel
    rclc_subscription_init_default(
        &cmd_vel_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel");

    // Executor ---> 1 handle = 1 subscriber
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(
        &executor, &cmd_vel_sub, &cmd_vel_msg,
        &cmd_vel_callback, ON_NEW_DATA);

    printf("micro-ROS ready, listening on /cmd_vel\n");

    // Main loop
    while (true) {
        // Spin micro-ROS — fires cmd_vel_callback if new message arrived
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));

        // PID velocity control
        float current_vel_l = LEFT_ENCODER.get_vel();
        float current_vel_r = RIGHT_ENCODER.get_vel();
        float vel_l = LEFT_PID .calculate(target_vel_l, current_vel_l);
        float vel_r = RIGHT_PID.calculate(target_vel_r, current_vel_r);

        MOTOR.move(vel_l, vel_r);
        sleep_ms(10);
    }

    return 0;
}