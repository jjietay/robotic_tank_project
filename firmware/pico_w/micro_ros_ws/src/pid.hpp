// ---------------------------------------------------------------------------
//                   pid.hpp  —  Discrete PID controller
// ---------------------------------------------------------------------------
//  Standard parallel-form PID with:
//    • output saturation (out_min, out_max)
//    • integral clamp (i_max)
//    • back-calculation anti-windup (integral is rewound by the amount the
//      output had to be saturated, scaled by 1/Ki)
//    • derivative skipped on the first iteration to avoid a spurious kick
//
//  Units convention used in this project:
//      setpoint, measured  → m/s
//      output              → motor duty cycle ∈ [-1, 1]
//      → Kp has units of  duty / (m/s)
//      → Ki has units of  duty / (m/s · s)
//      → Kd has units of  duty / (m/s / s)
// ---------------------------------------------------------------------------

#pragma once
#include "pico/stdlib.h"

class PID
{
private:
    float kp, ki, kd;
    float integral = 0.0f;
    float integral_max; // for clamping
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
