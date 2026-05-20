// ---------------------------------------------------------------------------
//                          config.hpp  —  Robot constants
// ---------------------------------------------------------------------------
//  Single source of truth for pin assignments, robot geometry, controller
//  gains and safety thresholds.  ALL physical quantities are SI (metres,
//  seconds, m/s, rad/s).
// ---------------------------------------------------------------------------

#pragma once
#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
//                              Pin Assignments
// ---------------------------------------------------------------------------
constexpr uint ENC_L_A = 16, ENC_L_B = 17;
constexpr uint ENC_R_A = 18, ENC_R_B = 19;

constexpr uint L_DIR   = 0,  L_PWM   = 8;
constexpr uint R_DIR   = 2,  R_PWM   = 9;

constexpr uint USRM_FRONT_TRIG = 10, USRM_FRONT_ECHO = 11;
constexpr uint USRM_BACK_TRIG  = 14, USRM_BACK_ECHO  = 15;
constexpr uint USRM_RIGHT_TRIG = 12, USRM_RIGHT_ECHO = 13;
constexpr uint USRM_LEFT_TRIG  =  6, USRM_LEFT_ECHO  =  7;

// ---------------------------------------------------------------------------
//                              Hardware Limits
// ---------------------------------------------------------------------------
constexpr uint  PWM_TOP = 6249;

// Minimum PWM duty cycle that overcomes motor stiction (measured empirically).
// Any non-zero duty below this value is automatically lifted to this floor so
// the motors never stall in the deadband and cause the PID to lurch.
constexpr float MOTOR_MIN_DUTY = 0.28f;

// Per-motor output trim factors ∈ (0, 1].  Left motor runs stronger, so we
// scale it down to drive straight.  Adjust LEFT_MOTOR_TRIM until the tank
// tracks a straight line at cruising speed.  Start here and fine-tune by
// 0.01 increments: if still drifting right, lower it; if overcorrected, raise it.
constexpr float LEFT_MOTOR_TRIM  = 0.90f;
constexpr float RIGHT_MOTOR_TRIM = 1.00f;

// ---------------------------------------------------------------------------
//                              Robot Geometry  (METRES)
// ---------------------------------------------------------------------------
constexpr float WHEEL_DIAMETER_M = 0.0423f;     // 42.3 mm
constexpr float WHEEL_BASE_M     = 0.1488f;     // distance between L & R wheels
constexpr float GEAR_REDUCTION   = 30.0f;
constexpr int   ENCODER_PPR      = 13;           // raw pulses per motor rev (pre-gearbox)
constexpr float V_MAX_MPS        = 0.60f;        // max wheel surface speed at full PWM

// ---------------------------------------------------------------------------
//                              Safety / E-stop  (METRES, METRES-PER-SECOND)
// ---------------------------------------------------------------------------
constexpr float FRONT_STOP_DIST_M       = 0.15f;
constexpr float BACK_STOP_DIST_M        = 0.15f;
constexpr float DIRECTION_DEADZONE_MPS  = 0.02f;

// ---------------------------------------------------------------------------
//                              PID Gains
// ---------------------------------------------------------------------------
//  Output is duty cycle in [-1, 1].  Error is in m/s.
//
//  Tuning context (assumes V_MAX_MPS = 0.20):
//    - Feedforward provides ~80% of the duty.  PID only corrects residual.
//    - Kp small enough that worst-case error (0.20) gives ~0.30 P-term, well
//      below saturation, so PID stays in linear regime.
//    - Ki provides slow integral correction without runaway.
//    - Kd damps overshoot from the feedforward step.
//
//  If still unstable: halve Kp and Ki together.
//  If sluggish: raise Kp first, then Ki.
constexpr float PID_KP = 1.5f;    // was 6.0 — reduced to keep PID in linear regime
constexpr float PID_KI = 2.0f;    // was 10.0 — slower integral, less wind-up
constexpr float PID_KD = 0.05f;   // was 0.0 — small damping term added

// ---------------------------------------------------------------------------
//                              Motion Smoothing
// ---------------------------------------------------------------------------
//  Slew-rate limit on the velocity setpoint, applied in the firmware so that
//  ANY publisher (teleop, brain node, joystick…) gets smooth motion without
//  having to ramp on its own side.
//
//  At 0.5 m/s² and 100 Hz loop, max change per tick = 0.005 m/s.
//  A 0 -> V_MAX (0.20 m/s) ramp takes 40 ticks = 400 ms.  Tweak to taste.
constexpr float MAX_ACCEL_MPS2 = 0.5f;

// ---------------------------------------------------------------------------
//                              Ultrasonic Scheduling
// ---------------------------------------------------------------------------
//  HC-SR04 reads block for up to ~30 ms while waiting for the echo timeout.
//  Firing one every loop iteration corrupts the 100 Hz PID timing.  Instead
//  we fire one only every USRM_DIVIDER iterations.  With DIVIDER = 3 and a
//  4-sensor round-robin, each individual sensor updates at ~8 Hz — fine for
//  collision avoidance at our top speed of 0.20 m/s (= 25 mm of travel
//  between readings worst case).
constexpr uint  USRM_DIVIDER = 3;

// ---------------------------------------------------------------------------
//                              Watchdog
// ---------------------------------------------------------------------------
constexpr uint64_t CMD_VEL_TIMEOUT_US = 500000ULL;  // 500 ms — stop if no cmd_vel

// ---------------------------------------------------------------------------
//                              IMU (BNO085 — I2C0)
// ---------------------------------------------------------------------------
constexpr uint IMU_SDA     = 4;   // GP4 — free on current pinout
constexpr uint IMU_SCL     = 5;   // GP5 — free on current pinout
constexpr uint IMU_DIVIDER = 2;   // publish at 50 Hz from the 100 Hz loop