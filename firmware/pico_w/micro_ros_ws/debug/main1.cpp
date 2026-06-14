#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#include "config.hpp"
#include "motor.hpp"
#include "encoder.hpp"

static constexpr float    FULL_DUTY     = 1.0f;
static constexpr uint32_t SPINUP_MS     = 2000;
static constexpr uint32_t MEASURE_MS    = 3000;
static constexpr uint32_t SAMPLE_EVERY  = 100;

int main()
{
    stdio_init_all();
    sleep_ms(5000);

    Motor motor("Motors", "ON",
                L_DIR, L_PWM, R_DIR, R_PWM,
                  true,
                 true,
                LEFT_MOTOR_TRIM,
                RIGHT_MOTOR_TRIM);

    Encoder enc_l("L_ENC", "ON",
                  ENC_L_A, ENC_L_B,
                  GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                   false);
    Encoder enc_r("R_ENC", "ON",
                  ENC_R_A, ENC_R_B,
                  GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                   true);

    printf("Maximum-velocity test: driving full duty for spin-up...\n");
    printf("Largest steady-state vel is your V_MAX_MPS.\n\n");

    motor.move(FULL_DUTY, FULL_DUTY);

    uint64_t t_end = time_us_64() + (uint64_t)SPINUP_MS * 1000ULL;
    while (time_us_64() < t_end) {
        enc_l.get_vel();
        enc_r.get_vel();
        sleep_ms(10);
    }

    float max_l = 0.0f, max_r = 0.0f;
    uint32_t elapsed = 0;
    while (elapsed < MEASURE_MS) {
        float vel_l = 0.0f, vel_r = 0.0f;
        uint64_t t_sample = time_us_64() + (uint64_t)SAMPLE_EVERY * 1000ULL;
        while (time_us_64() < t_sample) {
            vel_l = enc_l.get_vel();
            vel_r = enc_r.get_vel();
            sleep_ms(10);
        }
        if (vel_l > max_l) max_l = vel_l;
        if (vel_r > max_r) max_r = vel_r;
        elapsed += SAMPLE_EVERY;
        printf("vel(L:%+.3f R:%+.3f) m/s   peak(L:%.3f R:%.3f)\n",
               vel_l, vel_r, max_l, max_r);
    }

    motor.stop();
    printf("\nDone. Peak vel  L:%.3f  R:%.3f m/s\n", max_l, max_r);
    printf("Use the larger of the two as V_MAX_MPS.\n");

    while (true) {
        tight_loop_contents();
    }
}
