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

// ---------------------------------------------------------------------------
//                              Robot Geometry  (METRES)
// ---------------------------------------------------------------------------
//  Diameter is stored in METRES so encoder distance / velocity come out in
//  metres / metres-per-second naturally — no unit conversion in the loop.
constexpr float WHEEL_DIAMETER_M = 0.05465f;     // 5.465 cm
constexpr float WHEEL_BASE_M     = 0.356f;        // distance between L & R wheels
constexpr float GEAR_REDUCTION   = 169.0f;
constexpr int   ENCODER_PPR      = 11;           // raw pulses per motor rev (pre-gearbox)
constexpr float V_MAX_MPS = 0.14f;   // max wheel surface speed at full PWM

// ---------------------------------------------------------------------------
//                              Safety / E-stop  (METRES, METRES-PER-SECOND)
// ---------------------------------------------------------------------------
constexpr float FRONT_STOP_DIST_M       = 0.15f;
constexpr float BACK_STOP_DIST_M        = 0.15f;
constexpr float DIRECTION_DEADZONE_MPS  = 0.02f; // ignore tiny commands when
                                                 // deciding "forward / backward"

// ---------------------------------------------------------------------------
//                              PID Gains
// ---------------------------------------------------------------------------
//  Output is duty cycle in [-1, 1].  Error is in m/s.  Units of Kp are
//  (duty per m/s); Ki is (duty per (m/s × s)); Kd is (duty per (m/s / s)).
//  Feed-forward (duty = setpoint / V_MAX_MPS) supplies the bulk of the
//  command, so PID only needs to trim — gains are deliberately modest.
constexpr float PID_KP = 2.0f;
constexpr float PID_KI = 4.0f;
constexpr float PID_KD = 0.0f;

// EWMA smoothing on measured wheel velocity. α∈(0,1]: higher = more
// responsive, lower = more filtered. 0.3 trades a small lag for much
// less tick-quantisation noise into the PID at 100 Hz.
constexpr float VEL_FILTER_ALPHA = 0.3f;

// ---------------------------------------------------------------------------
//                              Watchdog
// ---------------------------------------------------------------------------
constexpr uint64_t CMD_VEL_TIMEOUT_US = 500000ULL;  // 500 ms — stop if no cmd_vel
