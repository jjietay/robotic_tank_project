#include "pid.hpp"
#include "pico/time.h"

PID::PID(float _kp, float _ki, float _kd, float out_min, float out_max, float i_max)
    : kp(_kp), ki(_ki), kd(_kd), integral_max(i_max),
    pwm_out_min(out_min), pwm_out_max(out_max), last_time(time_us_64())
{}

float PID::calculate(float setpoint, float measured)
{

    uint64_t now = time_us_64();
    float dt = (float)(now - last_time) * 1e-6f;

    if (dt <= 0.0f || dt > 0.5f) dt = 0.01f;

    float error = setpoint - measured;

    float p_term = kp * error;

    integral += error * dt;
    if (integral >  integral_max) integral =  integral_max;
    if (integral < -integral_max) integral = -integral_max;
    float i_term = ki * integral;

    float d_term = 0.0f;
    if (!first_run) {d_term = kd * (error - previous_error) / dt;}
    first_run = false;

    float pwm_to_set = p_term + i_term + d_term;

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
