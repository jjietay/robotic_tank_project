#include "ultrasonic.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"

Ultrasonic::Ultrasonic(std::string name_, std::string status_,
                    uint trig, uint echo, float vel)
    : Electronics(std::move(name_), std::move(status_)),
    trigger_pin(trig), echo_pin(echo), sound_vel(vel)
{
    gpio_init(trigger_pin); gpio_set_dir(trigger_pin, GPIO_OUT);
    gpio_init(echo_pin);    gpio_set_dir(echo_pin,    GPIO_IN);
    gpio_put(trigger_pin, 0);
}

float Ultrasonic::update()
{

    gpio_put(trigger_pin, 0); busy_wait_us(5);
    gpio_put(trigger_pin, 1); busy_wait_us(15);
    gpio_put(trigger_pin, 0);

    uint64_t t0 = time_us_64();
    while (!gpio_get(echo_pin)) {
        if (time_us_64() - t0 > 38000ULL) {
            last_distance_m = -1.0f;
            return last_distance_m;
        }
    }

    uint64_t rise = time_us_64();
    while (gpio_get(echo_pin)) {
        if (time_us_64() - rise > 38000ULL) {
            last_distance_m = -2.0f;
            return last_distance_m;
        }
    }
    uint64_t fall = time_us_64();

    float dt_s = (float)(fall - rise) * 1e-6f;
    last_distance_m = (dt_s * sound_vel) * 0.5f;
    return last_distance_m;
}
