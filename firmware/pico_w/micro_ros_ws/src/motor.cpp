#include "motor.hpp"
#include "config.hpp"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <cmath>

void Motor::setup_pwm(uint pin)
{
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, PWM_TOP);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), 0);
    pwm_set_enabled(slice, true);
}

void Motor::set_one_side(uint dir_pin, uint pwm_pin, float duty)
{
    // Hardware saturation only — a duty cycle physically cannot exceed 100 %.
    if (duty >  1.0f) duty =  1.0f; // max duty is 1.0
    if (duty < -1.0f) duty = -1.0f; // min duty is -1.0

    gpio_put(dir_pin, duty >= 0.0f ? 1 : 0);
    pwm_set_gpio_level(pwm_pin, (uint16_t)(std::fabs(duty) * (float)PWM_TOP));
}

Motor::Motor(std::string name_, std::string status_,
            uint l_dir, uint l_pwm, uint r_dir, uint r_pwm,
            bool invert_left, bool invert_right)
    : Electronics(std::move(name_), std::move(status_)),
    l_dir_pin(l_dir), l_pwm_pin(l_pwm),
    r_dir_pin(r_dir), r_pwm_pin(r_pwm),
    l_invert(invert_left), r_invert(invert_right)
{
    gpio_init(l_dir_pin); gpio_set_dir(l_dir_pin, GPIO_OUT);
    gpio_init(r_dir_pin); gpio_set_dir(r_dir_pin, GPIO_OUT);
    setup_pwm(l_pwm_pin);
    setup_pwm(r_pwm_pin);
}

void Motor::move(float duty_left, float duty_right)
{
    if (l_invert) duty_left  = -duty_left;
    if (r_invert) duty_right = -duty_right;
    set_one_side(l_dir_pin, l_pwm_pin, duty_left);
    set_one_side(r_dir_pin, r_pwm_pin, duty_right);
}

void Motor::stop()
{
    set_one_side(l_dir_pin, l_pwm_pin, 0.0f);
    set_one_side(r_dir_pin, r_pwm_pin, 0.0f);
}
