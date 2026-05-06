// Takes in current and desired values of velocities, calculates error and passes it to PID. 
// PID outputs 0-1 which is PWM value.

#include "pid.hpp"
#include "pico/time.h"

PID::PID(float _kp, float _ki, float _kd, float out_min, float out_max, float i_max) 
    : kp(_kp), ki(_ki), kd(_kd), integral_max(i_max), output_min(out_min), output_max(out_max), last_time(time_us_64())
{}

float PID::calculate(float setpoint, float measured)
{
    uint64_t now = time_us_64();
    float    dt  = (float)(now - last_time) * 1e-6f;    // in seconds, last_time was init time_us_64 in constructor

    // in the event of jitters (lag), we don't want dt to accumulate
    if (dt <= 0.0f || dt > 0.5f) dt = 0.01f;

    float error = setpoint - measured;  // error

    // ---- P ----
    float p_term = kp * error;      // kp is just "how much" PWM to tweak per unit error to tweak such that we can reduce the error

    // ---- I (with hard clamp; back-calc happens after output saturation) ----
    integral += error * dt;
    if (integral >  integral_max) integral =  integral_max;     // clamp to max value
    if (integral < -integral_max) integral = -integral_max;     // clamp to min value
    float i_term = ki * integral;                               // get I term

    // ---- D (skip on the very first call) ----
    float d_term = 0.0f;
    if (!first_run) {      // for first run, previous_error would be 0. So we need 1st run
        d_term = kd * (error - previous_error) / dt;
    }
    first_run = false;

    float u = p_term + i_term + d_term; // pwm to set

    // ---- Output saturation + anti-windup back-calculation ----
    if (u > output_max) {   // limit to output_max, also windup for I part
        if (ki > 1e-6f) integral -= (u - output_max) / ki;
        u = output_max;
    } else if (u < output_min) {    // limit to min output
        if (ki > 1e-6f) integral += (output_min - u) / ki;
        u = output_min;
    }

    previous_error = error;
    last_time      = now;
    return u;   // returns pwm value
}

void PID::reset()
{
    integral       = 0.0f;
    previous_error = 0.0f;
    first_run      = true;
    last_time      = time_us_64();
}


/*
We know that we want the output, u, of the PID controller to just give us the PWM value we should set. 
*/