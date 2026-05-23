// Takes in current and desired values of velocities, calculates error and passes it to PID. 
// PID outputs 0-1 which is PWM value.

#include "pid.hpp"
#include "pico/time.h"

PID::PID(float _kp, float _ki, float _kd, float out_min, float out_max, float i_max) 
    : kp(_kp), ki(_ki), kd(_kd), integral_max(i_max),
    pwm_out_min(out_min), pwm_out_max(out_max), last_time(time_us_64())
{}

float PID::calculate(float setpoint, float measured)
{
    // Calculation of dt (loop supposed to run at 100Hz, meaning dt should be 10ms)
    uint64_t now = time_us_64();
    float dt = (float)(now - last_time) * 1e-6f;

    // Prevents dt from accumulating in the event of jitters/lag
    if (dt <= 0.0f || dt > 0.5f) dt = 0.01f;

    // Error calculation
    float error = setpoint - measured;

    // P Term
    // max_p_term = max_error * kp = 0.60 * 0.8 = 0.48
    float p_term = kp * error;

    // I Term with hard clamp
    integral += error * dt;
    if (integral >  integral_max) integral =  integral_max;     // clamp to max value
    if (integral < -integral_max) integral = -integral_max;     // clamp to min value
    float i_term = ki * integral;                               // get I term

    // D Term (we skip for the first call)
    float d_term = 0.0f;
    if (!first_run) {d_term = kd * (error - previous_error) / dt;}
    first_run = false;

    // PWM value from adding all PID terms
    float pwm_to_set = p_term + i_term + d_term;

    // Anti-integral windup
    if (pwm_to_set > pwm_out_max)
    {
        if (ki > 1e-6f) integral -= (pwm_to_set - pwm_out_max) / ki;
        pwm_to_set = pwm_out_max;
    }
    else if (pwm_to_set < pwm_out_min)
    {
        if (ki > 1e-6f) integral += (pwm_out_min - pwm_to_set) / ki;
        pwm_to_set = pwm_out_min;
    }

    previous_error = error;
    last_time = now;
    return pwm_to_set;
}

void PID::reset()
{
    integral = 0.0f;
    previous_error = 0.0f;
    first_run = true;
    last_time = time_us_64();
}
