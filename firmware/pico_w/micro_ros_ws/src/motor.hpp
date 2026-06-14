#pragma once
#include "electronics.hpp"
#include "pico/stdlib.h"

class Motor : public Electronics
{
private:
    uint  l_dir_pin, l_pwm_pin;
    uint  r_dir_pin, r_pwm_pin;
    bool  l_invert, r_invert;
    float l_trim,   r_trim;

    void setup_pwm(uint pin);
    void set_one_side(uint dir_pin, uint pwm_pin, float duty);

public:
    Motor(std::string name, std::string status,
        uint l_dir, uint l_pwm, uint r_dir, uint r_pwm,
        bool  invert_left  = true,
        bool  invert_right = false,
        float trim_left    = 1.0f,
        float trim_right   = 1.0f);

    void move(float duty_left, float duty_right);
    void stop();
};
