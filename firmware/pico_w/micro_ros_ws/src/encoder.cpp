#include "encoder.hpp"
#include "config.hpp"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"

// Pi constant
static constexpr float ENC_PI = 3.141592653589793f;

// Static-member definitions
Encoder* Encoder::instances[Encoder::MAX_ENCODERS] = {nullptr, nullptr};
volatile int Encoder::instance_count = 0;

// Constructor
Encoder::Encoder(std::string name_, std::string status_,
                uint _pin_a, uint _pin_b,
                float _reduction_ratio,
                float _diameter_m,
                int _ppr, bool _invert)
    : Electronics(std::move(name_), std::move(status_)),
    pin_a(_pin_a), pin_b(_pin_b),
    reduction_ratio(_reduction_ratio),
    diameter_m(_diameter_m),
    ppr(_ppr),
    invert(_invert)
{
    // Circumference of Wheel
    circumference_m = ENC_PI * diameter_m;
    
    // A quadrature hall encoder has both A and B (rising and falling edges)
    counts_per_output_rev = 4.0f * (float)ppr * reduction_ratio;
    last_time = time_us_64();

    // Pull-ups forces pins to high to prevent floating inputs when no signal is present (safeguard against electrical noise)
    gpio_init(pin_a); gpio_set_dir(pin_a, GPIO_IN); gpio_pull_up(pin_a);
    gpio_init(pin_b); gpio_set_dir(pin_b, GPIO_IN); gpio_pull_up(pin_b);

    // We only register IRQ with callback for 1st Encoder since Pico SDK has one shared GPIO IRQ callback per core
    // for subsequent encoders, we just enable irq for their pins
    if (instance_count < MAX_ENCODERS)
    {
        instances[instance_count] = this;

        if (instance_count == 0)
        {gpio_set_irq_enabled_with_callback(pin_a, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &Encoder::irq_handler);}
        else {gpio_set_irq_enabled(pin_a, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);}
        gpio_set_irq_enabled(pin_b, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

        instance_count++;
    }
}

// High-level IRQ Calling
void Encoder::irq_handler(uint gpio, uint32_t events)
{
    // Dispatch the IRQ to whichever encoder owns this pin
    for (int i = 0; i < instance_count; i++) {
        if (instances[i]) instances[i]->handle_irq(gpio, events);
    }
}

// Low-level IRQ Handling
void Encoder::handle_irq(uint gpio, uint32_t /*events*/)
{
    if (gpio != pin_a && gpio != pin_b) return;
    bool a = gpio_get(pin_a);
    bool b = gpio_get(pin_b);
    if (gpio == pin_a) {
        if (a == b) count--; else count++;
    } else {
        if (a == b) count++; else count--;
    }
}

// Stop Interrupts first --> save the total counts --> resume interrupts after
int Encoder::get_count()
{
    uint32_t saved = save_and_disable_interrupts();
    int c = count;
    restore_interrupts(saved);
    return invert ? -c : c;
}

// get velocity of a motor
float Encoder::get_vel()
{
    // calculate change in time
    uint64_t now = time_us_64();
    int64_t time_diff_us = (int64_t)(now - last_time);

    // Floor dt at 1ms (prevents poor signal to quantization noise)
    if (time_diff_us < 1000) return 0.0f;

    uint32_t saved = save_and_disable_interrupts();
    int current_count = count;
    restore_interrupts(saved);

    // Initial Artifact Suppression
    if (first_vel_call) {
        last_count     = current_count;
        last_time      = now;
        first_vel_call = false;
        return 0.0f;
    }

    // get dt is seconds --> cal count difference --> new displacement/distance in metres
    float dt_s = (float)time_diff_us * 1e-6f;
    int count_diff = current_count - last_count;
    float distance_m = ((float)count_diff / counts_per_output_rev) * circumference_m;
    float vel_mps = distance_m / dt_s;
    
    // Exponential Moving Average (EMA) filter that smooths the velocity
    // For first get_vel call, we set vel_filtered as vel_mps
    if (!ema_seeded && vel_mps != 0.0f) {
        vel_filtered = vel_mps;
        ema_seeded = true;
    } else {
        vel_filtered = (1.0f - ENC_EMA_ALPHA) * vel_filtered + ENC_EMA_ALPHA * vel_mps;
    }

    last_time  = now;
    last_count = current_count;

    return invert ? -vel_filtered : vel_filtered;
}