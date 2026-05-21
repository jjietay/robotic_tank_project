#include "motor.hpp"
#include "config.hpp"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <cmath>

// Initialization
void Motor::setup_pwm(uint pin)
{
    // Initialization of pins
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, PWM_TOP);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), 0);
    pwm_set_enabled(slice, true);
}

// Constructor
Motor::Motor(std::string name_, std::string status_,
            uint l_dir, uint l_pwm, uint r_dir, uint r_pwm,
            bool invert_left, bool invert_right,
            float trim_left, float trim_right)
    : Electronics(std::move(name_), std::move(status_)),
    l_dir_pin(l_dir), l_pwm_pin(l_pwm),
    r_dir_pin(r_dir), r_pwm_pin(r_pwm),
    l_invert(invert_left), r_invert(invert_right),
    l_trim(trim_left),    r_trim(trim_right)
{
    gpio_init(l_dir_pin); gpio_set_dir(l_dir_pin, GPIO_OUT);
    gpio_init(r_dir_pin); gpio_set_dir(r_dir_pin, GPIO_OUT);
    setup_pwm(l_pwm_pin);
    setup_pwm(r_pwm_pin);
}

// Set 1 motor
void Motor::set_one_side(uint dir_pin, uint pwm_pin, float duty)
{
    // Keep max and min duty cycle within [-1, 1]
    if (duty >  1.0f) duty =  1.0f;
    if (duty < -1.0f) duty = -1.0f;

    // If duty is +ve, means forward, we pull dir_pin to high
    gpio_put(dir_pin, duty >= 0.0f ? 1 : 0);
    
    // Set raw PWM to particular pwm_pin
    pwm_set_gpio_level(pwm_pin, (uint16_t)(std::fabs(duty) * (float)PWM_TOP));
}

// Move both motors
void Motor::move(float duty_left, float duty_right)
{
    // this changes level of duty cycle with based on trim factor
    duty_left  *= l_trim;
    duty_right *= r_trim;

    // Deadband compensation helps to ensure min duty of
    // commanded duty_left and duty_right is not less than 0, and not less than min_duty
    if (duty_left  > 0.0f && duty_left  < MOTOR_MIN_DUTY)  duty_left  =  MOTOR_MIN_DUTY;
    if (duty_left  < 0.0f && duty_left  > -MOTOR_MIN_DUTY) duty_left  = -MOTOR_MIN_DUTY;
    if (duty_right > 0.0f && duty_right < MOTOR_MIN_DUTY)  duty_right =  MOTOR_MIN_DUTY;
    if (duty_right < 0.0f && duty_right > -MOTOR_MIN_DUTY) duty_right = -MOTOR_MIN_DUTY;

    // Flip polarity (if invert is True)
    if (l_invert) duty_left  = -duty_left;
    if (r_invert) duty_right = -duty_right;

    // Send the actual raw PWM to pins to drive motor
    set_one_side(l_dir_pin, l_pwm_pin, duty_left);
    set_one_side(r_dir_pin, r_pwm_pin, duty_right);
}

// Stopping function
void Motor::stop()
{
    // Stop both motors
    set_one_side(l_dir_pin, l_pwm_pin, 0.0f);
    set_one_side(r_dir_pin, r_pwm_pin, 0.0f);
}
