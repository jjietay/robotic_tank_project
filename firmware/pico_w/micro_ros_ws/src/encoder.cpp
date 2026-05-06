#include "encoder.hpp"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"

static constexpr float ENC_PI = 3.141592653589793f;

// Static-member definitions
Encoder*     Encoder::instances[Encoder::MAX_ENCODERS] = {nullptr, nullptr};
volatile int Encoder::instance_count                   = 0;

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
    circumference_m       = ENC_PI * diameter_m;                          // π × d (METRES)
    counts_per_output_rev = 4.0f * (float)ppr * reduction_ratio;          // 4× quadrature
    last_time             = time_us_64();

    // Pull-ups prevent floating inputs when no signal is present
    gpio_init(pin_a); gpio_set_dir(pin_a, GPIO_IN); gpio_pull_up(pin_a);
    gpio_init(pin_b); gpio_set_dir(pin_b, GPIO_IN); gpio_pull_up(pin_b);

    // The Pico SDK has ONE shared GPIO IRQ callback per core. Only the first
    // encoder registers it; subsequent encoders just enable their pins.
    if (instance_count < MAX_ENCODERS) {
        instances[instance_count] = this;

        if (instance_count == 0) {
            gpio_set_irq_enabled_with_callback(
                pin_a, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true,
                &Encoder::irq_handler);
        } else {
            gpio_set_irq_enabled(
                pin_a, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
        }
        gpio_set_irq_enabled(
            pin_b, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

        instance_count++;
    }
}

void Encoder::irq_handler(uint gpio, uint32_t events)
{
    // Dispatch the IRQ to whichever encoder owns this pin
    for (int i = 0; i < instance_count; i++) {
        if (instances[i]) instances[i]->handle_irq(gpio, events);
    }
}

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

int Encoder::get_count()
{
    uint32_t saved = save_and_disable_interrupts();
    int c = count;
    restore_interrupts(saved);
    return invert ? -c : c;
}

float Encoder::get_vel()
{
    uint64_t now          = time_us_64();
    int64_t  time_diff_us = (int64_t)(now - last_time);

    // Floor dt at 1 ms — any tighter and tick-quantisation noise dominates.
    if (time_diff_us < 1000) return 0.0f;

    uint32_t saved         = save_and_disable_interrupts();
    int      current_count = count;
    restore_interrupts(saved);

    float dt_s        = (float)time_diff_us * 1e-6f;
    int   count_diff  = current_count - last_count;
    float distance_m  = ((float)count_diff / counts_per_output_rev) * circumference_m;

    last_time  = now;
    last_count = current_count;

    float vel_mps = distance_m / dt_s;
    return invert ? -vel_mps : vel_mps;
}
