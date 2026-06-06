// ---------------------------------------------------------------------------
//                       debug/main.cpp  —  Minimum-PWM test
// ---------------------------------------------------------------------------
//  Standalone (no micro-ROS).  Power the Pico W, wait ~5 s for the USB serial
//  to enumerate, then it runs automatically.
//
//  Purpose: find the minimum duty cycle that overcomes motor stiction and
//  actually starts the wheels turning.  It ramps the duty up slowly from 0,
//  holding each step, and prints the commanded duty alongside the measured
//  encoder velocity for each side.  Watch the serial output: the first duty
//  at which vel goes non-zero is your MOTOR_MIN_DUTY.
//
//  NOTE: Motor::move() only forces MOTOR_MIN_DUTY when the raw command is
//  ABOVE it, so duties below that threshold pass through unchanged — exactly
//  what we want for this sweep.
//
//  Put the robot on blocks / lift the wheels so it doesn't drive away.
// ---------------------------------------------------------------------------

#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#include "config.hpp"
#include "motor.hpp"
#include "encoder.hpp"

// ---- Sweep parameters -----------------------------------------------------
static constexpr float DUTY_START = 0.00f;  // first duty tested
static constexpr float DUTY_END   = 0.50f;  // last duty tested
static constexpr float DUTY_STEP  = 0.01f;  // increment per step
static constexpr uint32_t STEP_MS = 700;    // hold time per step (let EMA settle)

int main()
{
    stdio_init_all();
    sleep_ms(5000);   // give USB serial time to connect before we start

    Motor motor("Motors", "ON",
                L_DIR, L_PWM, R_DIR, R_PWM,
                /*invert_left*/  true,
                /*invert_right*/ true,
                LEFT_MOTOR_TRIM,
                RIGHT_MOTOR_TRIM);

    Encoder enc_l("L_ENC", "ON",
                  ENC_L_A, ENC_L_B,
                  GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                  /*invert*/ false);
    Encoder enc_r("R_ENC", "ON",
                  ENC_R_A, ENC_R_B,
                  GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                  /*invert*/ true);

    printf("Minimum-PWM test: ramping duty %.2f -> %.2f, step %.2f\n",
           DUTY_START, DUTY_END, DUTY_STEP);
    printf("First duty with non-zero vel is your MOTOR_MIN_DUTY.\n\n");

    for (float duty = DUTY_START; duty <= DUTY_END + 1e-6f; duty += DUTY_STEP)
    {
        motor.move(duty, duty);

        // Hold this step, sampling velocity at ~100 Hz so the encoder EMA
        // settles before we read it.
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
