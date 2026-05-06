#include "pid.hpp"
#include "pico/time.h"

PID::PID(float _kp, float _ki, float _kd, float out_min, float out_max, float i_max) 
    : kp(_kp), ki(_ki), kd(_kd), integral_max(i_max), output_min(out_min), output_max(out_max), last_time(time_us_64())
{}

float PID::calculate(float setpoint, float measured)
{
    uint64_t now = time_us_64();
    float    dt  = (float)(now - last_time) * 1e-6f;

    // Sane fallback for the first call and for jittered ticks
    if (dt <= 0.0f || dt > 0.5f) dt = 0.01f;

    float error = setpoint - measured;

    // ---- P ----
    float p_term = kp * error;

    // ---- I (with hard clamp; back-calc happens after output saturation) ----
    integral += error * dt;
    if (integral >  integral_max) integral =  integral_max;
    if (integral < -integral_max) integral = -integral_max;
    float i_term = ki * integral;

    // ---- D (skip on the very first call) ----
    float d_term = 0.0f;
    if (!first_run) {
        d_term = kd * (error - previous_error) / dt;
    }
    first_run = false;

    float u = p_term + i_term + d_term;

    // ---- Output saturation + anti-windup back-calculation ----
    if (u > output_max) {
        if (ki > 1e-6f) integral -= (u - output_max) / ki;
        u = output_max;
    } else if (u < output_min) {
        if (ki > 1e-6f) integral += (output_min - u) / ki;
        u = output_min;
    }

    previous_error = error;
    last_time      = now;
    return u;
}

void PID::reset()
{
    integral       = 0.0f;
    previous_error = 0.0f;
    first_run      = true;
    last_time      = time_us_64();
}
