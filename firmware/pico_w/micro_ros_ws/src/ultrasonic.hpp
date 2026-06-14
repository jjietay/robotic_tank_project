#pragma once
#include "electronics.hpp"
#include "pico/stdlib.h"

class Ultrasonic : public Electronics
{
private:
    uint  trigger_pin;
    uint  echo_pin;
    float sound_vel;
    float last_distance_m = -1.0f;

public:
    Ultrasonic(std::string name, std::string status,
            uint trig, uint echo, float vel = 346.0f);

    float update();
};
