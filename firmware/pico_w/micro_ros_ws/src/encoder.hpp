// ---------------------------------------------------------------------------
//                       encoder.hpp  —  Quadrature wheel encoder
// ---------------------------------------------------------------------------
//  IRQ-driven 4× quadrature decoder.  Diameter is in METRES → get_vel()
//  returns m/s natively, no unit conversion needed downstream.
//
//  An `invert` flag flips both count and velocity so wiring polarity is
//  hidden from callers.  Up to MAX_ENCODERS instances are supported.
// ---------------------------------------------------------------------------

#pragma once
#include "electronics.hpp"
#include "pico/stdlib.h"

class Encoder : public Electronics
{
public:
    static constexpr int MAX_ENCODERS = 2;     // bump if you add more wheels
    float vel_filtered = 0.0f;
    
private:
    uint     pin_a, pin_b;
    float    reduction_ratio;
    float    diameter_m;
    float    circumference_m;
    float    counts_per_output_rev;            // 4 × PPR × gearbox
    int      ppr;
    bool     invert;

    volatile int count = 0;                    // written from ISR
    int          last_count    = 0;
    uint64_t     last_time     = 0;
    bool         first_vel_call = true;        // gate the first get_vel() call
                                               // so it can baseline last_count
                                               // against any ticks accumulated
                                               // between construction and first
                                               // use (see encoder.cpp).

    static Encoder*     instances[MAX_ENCODERS];
    static volatile int instance_count;

    static void irq_handler(uint gpio, uint32_t events);
    void        handle_irq(uint gpio, uint32_t events);

public:
    Encoder(std::string name, std::string status,
            uint _pin_a, uint _pin_b,
            float _reduction_ratio,
            float _diameter_m,            // METRES
            int   _ppr    = 11,
            bool  _invert = false);

    int   get_count();      // ticks       (sign-corrected)
    float get_vel();        // metres/sec  (sign-corrected)
};