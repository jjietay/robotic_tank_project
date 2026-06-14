#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#include "config.hpp"
#include "motor.hpp"
#include "encoder.hpp"

static constexpr float DUTY_START = 0.00f;
static constexpr float DUTY_END   = 0.50f;
static constexpr float DUTY_STEP  = 0.01f;
static constexpr uint32_t STEP_MS = 700;

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

    printf("Minimum-PWM test: ramping duty %.2f -> %.2f, step %.2f\n",
           DUTY_START, DUTY_END, DUTY_STEP);
    printf("First duty with non-zero vel is your MOTOR_MIN_DUTY.\n\n");

    for (float duty = DUTY_START; duty <= DUTY_END + 1e-6f; duty += DUTY_STEP)
    {
        motor.move(duty, duty);

        uint64_t t_end = time_us_64() + (uint64_t)STEP_MS * 1000ULL;
        float vel_l = 0.0f, vel_r = 0.0f;
        while (time_us_64() < t_end) {
            vel_l = enc_l.get_vel();
            vel_r = enc_r.get_vel();
            sleep_ms(10);
        }

        printf("duty %.2f  vel(L:%+.3f R:%+.3f) m/s\n", duty, vel_l, vel_r);
    }

    motor.stop();
    printf("\nSweep complete. Motors stopped.\n");

    while (true) {
        tight_loop_contents();
    }
}
