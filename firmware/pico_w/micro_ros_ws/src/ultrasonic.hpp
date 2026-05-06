// ---------------------------------------------------------------------------
//                       ultrasonic.hpp  —  HC-SR04 driver
// ---------------------------------------------------------------------------
//  Blocking, single-fire trigger / echo measurement.  update() returns
//  distance in METRES.  One sensor is fired per main-loop tick (staggered
//  in main.cpp) so the worst-case loop blocking stays bounded.
// ---------------------------------------------------------------------------

#pragma once
#include "electronics.hpp"
#include "pico/stdlib.h"

class Ultrasonic : public Electronics
{
private:
    uint  trigger_pin;
    uint  echo_pin;
    float sound_vel;             // m/s  (≈346 at 25°C)
    float last_distance_m = -1.0f;

public:
    Ultrasonic(std::string name, std::string status,
               uint trig, uint echo, float vel = 346.0f);

    // Fires trigger, blocks for echo, returns distance in METRES.
    //   -1.0  → echo never went HIGH (no object / wiring fault)
    //   -2.0  → echo stuck HIGH past timeout
    float update();
    float get_distance() const { return last_distance_m; }
};
