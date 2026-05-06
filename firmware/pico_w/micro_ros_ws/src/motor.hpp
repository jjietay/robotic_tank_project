// ---------------------------------------------------------------------------
//                       motor.hpp  —  Cytron MDD10A driver
// ---------------------------------------------------------------------------
//  Accepts a duty cycle in [-1, 1].  -1 = full reverse, 0 = stop, +1 = full
//  forward.  This is the only place the [-1, 1] saturation lives — it is a
//  HARDWARE limit of the PWM peripheral, not a normalisation of velocity.
//
//  Each side has an `invert` flag so the wiring polarity is encapsulated
//  here; callers always pass "logical forward = positive" duty values.
// ---------------------------------------------------------------------------

#pragma once
#include "electronics.hpp"
#include "pico/stdlib.h"

class Motor : public Electronics
{
private:
    uint l_dir_pin, l_pwm_pin;
    uint r_dir_pin, r_pwm_pin;
    bool l_invert, r_invert;

    void setup_pwm(uint pin);
    void set_one_side(uint dir_pin, uint pwm_pin, float duty);

public:
    Motor(std::string name, std::string status,
          uint l_dir, uint l_pwm, uint r_dir, uint r_pwm,
          bool invert_left  = true,    // left side wired reversed by default
          bool invert_right = false);

    // duty_left, duty_right ∈ [-1, 1]  (saturated to hardware limit)
    void move(float duty_left, float duty_right);
    void stop();
};
