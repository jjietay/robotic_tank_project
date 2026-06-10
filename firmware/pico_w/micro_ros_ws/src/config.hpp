// ---------------------------------------------------------------------------
//                          config.hpp  —  Robot constants
// ---------------------------------------------------------------------------
//  Contains pin assignments, robot geometry, controller gains and 
//  safety thresholds.  ALL physical quantities are in
//  SI (metres, seconds, m/s, rad/s).
// ---------------------------------------------------------------------------

#pragma once
#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
//                              Pin Assignments
// ---------------------------------------------------------------------------
constexpr uint ENC_L_A = 18, ENC_L_B = 19;
constexpr uint ENC_R_A = 20, ENC_R_B = 21;

constexpr uint L_DIR = 0, L_PWM = 8;
constexpr uint R_DIR = 2, R_PWM = 9;

constexpr uint USRM_FRONT_TRIG = 10, USRM_FRONT_ECHO = 11;
constexpr uint USRM_BACK_TRIG = 14, USRM_BACK_ECHO = 15;
constexpr uint USRM_RIGHT_TRIG = 12, USRM_RIGHT_ECHO = 13;
constexpr uint USRM_LEFT_TRIG = 6, USRM_LEFT_ECHO = 7;

// ---------------------------------------------------------------------------
//                             Hardware Limits
// ---------------------------------------------------------------------------
constexpr uint PWM_TOP = 6249;

// Minimum PWM duty cycle that overcomes motor stiction
constexpr float MOTOR_MIN_DUTY = 0.15f;

// Trim factor for balancing motor strength
constexpr float LEFT_MOTOR_TRIM = 1.00f;
constexpr float RIGHT_MOTOR_TRIM = 1.00f;

// ---------------------------------------------------------------------------
//                       Robot Geometry (in metres)
// ---------------------------------------------------------------------------
constexpr float WHEEL_DIAMETER_M = 0.0423f;
constexpr float WHEEL_BASE_M = 0.135f; // distance between L & R wheels
constexpr float GEAR_REDUCTION = 131.0f; // JBG31-520-12V-76RPM gearbox
constexpr int   ENCODER_PPR = 11;       // raw pulses per motor rev
constexpr float ENC_EMA_ALPHA = 0.7f;
constexpr float V_MAX_MPS = 0.175f;      // max wheel surface speed at full PWM

// Minimum controllable speed
constexpr float V_MIN_MPS = MOTOR_MIN_DUTY * V_MAX_MPS;

// ---------------------------------------------------------------------------
//                Safety / E-stop  (METRES, METRES-PER-SECOND)
// ---------------------------------------------------------------------------
constexpr float FRONT_STOP_DIST_M = 0.15f;
constexpr float BACK_STOP_DIST_M  = 0.15f;
constexpr float DIRECTION_DEADZONE_MPS = 0.02f;

// ---------------------------------------------------------------------------
//                               PID Gains
// ---------------------------------------------------------------------------
//  Output is duty cycle in [-1, 1], error is in m/s,
// Feedforward provides ~80% of the duty while PID only corrects residual
// Left motor PID (weaker motor, needs more aggressive correction)
constexpr float KP_L = 0.4f;
constexpr float KI_L = 0.2f;
constexpr float KD_L = 0.02f;

// Right motor PID (stronger motor, needs gentler correction)
constexpr float KP_R = 0.4f;
constexpr float KI_R = 0.2f;
constexpr float KD_R = 0.02f;
constexpr float PID_OUT_MAX = 0.15f;

// ---------------------------------------------------------------------------
//                         Ultrasonic Scheduling
// ---------------------------------------------------------------------------
//  HC-SR04 reads block for up to ~30 ms while waiting for the echo timeout.
//  Firing one every loop iteration corrupts the 100 Hz PID timing.  Instead
//  we fire one only every USRM_DIVIDER iterations.  With DIVIDER = 3 and a
//  4-sensor round-robin, each individual sensor updates at ~8 Hz — fine for
//  collision avoidance at our top speed
constexpr uint USRM_DIVIDER = 3;

// ---------------------------------------------------------------------------
//                Watchdog (500 ms — stop if no cmd_vel)
// ---------------------------------------------------------------------------
constexpr uint64_t CMD_VEL_TIMEOUT_US = 500000ULL;

// ---------------------------------------------------------------------------
//                        IMU (BNO085 — I2C0)
// ---------------------------------------------------------------------------
constexpr uint IMU_SDA = 4;
constexpr uint IMU_SCL = 5;
constexpr uint IMU_DIVIDER = 2;   // publish at 50 Hz from the 100 Hz loop