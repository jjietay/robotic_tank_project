#pragma once
#include "pico/stdlib.h"

class PID
{
private:
    float kp, ki, kd;
    float integral = 0.0f;
    float integral_max;
    float pwm_out_min;
    float pwm_out_max;
    float previous_error = 0.0f;
    uint64_t last_time;
    bool first_run = true;

public:
    PID(float kp, float ki, float kd,
        float out_min = -1.0f,
        float out_max = 1.0f,
        float i_max = 1.0f);

    float calculate(float setpoint, float measured);
    void reset();
};
