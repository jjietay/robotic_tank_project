#pragma once
#include "electronics.hpp"
#include "pico/stdlib.h"

class Encoder : public Electronics
{
public:
    static constexpr int MAX_ENCODERS = 2;
    float vel_filtered = 0.0f;

private:
    uint pin_a, pin_b;
    float reduction_ratio;
    float diameter_m;
    float circumference_m;
    float counts_per_output_rev;
    int ppr;
    bool invert;

    volatile int count = 0;
    int last_count = 0;
    uint64_t last_time = 0;
    bool first_vel_call = true;
    bool ema_seeded = false;
    static Encoder* instances[MAX_ENCODERS];
    static volatile int instance_count;

    static void irq_handler(uint gpio, uint32_t events);
    void handle_irq(uint gpio, uint32_t events);

public:
    Encoder(std::string name, std::string status,
            uint _pin_a, uint _pin_b,
            float _reduction_ratio,
            float _diameter_m,
            int _ppr = 11,
            bool _invert = false);

    int get_count();
    float get_vel();
};
