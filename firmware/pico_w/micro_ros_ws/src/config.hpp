#pragma once
#include "pico/stdlib.h"

constexpr uint ENC_L_A = 20, ENC_L_B = 21;
constexpr uint ENC_R_A = 18, ENC_R_B = 19;

constexpr uint L_DIR = 2, L_PWM = 9;
constexpr uint R_DIR = 0, R_PWM = 8;

constexpr uint USRM_FRONT_TRIG = 10, USRM_FRONT_ECHO = 11;
constexpr uint USRM_BACK_TRIG = 14, USRM_BACK_ECHO = 15;
constexpr uint USRM_RIGHT_TRIG = 12, USRM_RIGHT_ECHO = 13;
constexpr uint USRM_LEFT_TRIG = 6, USRM_LEFT_ECHO = 7;

constexpr uint PWM_TOP = 6249;

constexpr float MOTOR_MIN_DUTY = 0.15f;

constexpr float LEFT_MOTOR_TRIM = 1.00f;
constexpr float RIGHT_MOTOR_TRIM = 1.00f;

constexpr float WHEEL_DIAMETER_M = 0.0423f;
constexpr float WHEEL_BASE_M = 0.135f;
constexpr float GEAR_REDUCTION = 131.0f;
constexpr int   ENCODER_PPR = 11;
constexpr float ENC_EMA_ALPHA = 0.7f;
constexpr float V_MAX_MPS = 0.175f;

constexpr float V_MIN_MPS = MOTOR_MIN_DUTY * V_MAX_MPS;

constexpr float FRONT_STOP_DIST_M = 0.15f;
constexpr float BACK_STOP_DIST_M  = 0.15f;
constexpr float DIRECTION_DEADZONE_MPS = 0.02f;

constexpr float KP_L = 0.4f;
constexpr float KI_L = 0.2f;
constexpr float KD_L = 0.02f;

constexpr float KP_R = 0.4f;
constexpr float KI_R = 0.2f;
constexpr float KD_R = 0.02f;
constexpr float PID_OUT_MAX = 0.15f;

constexpr uint USRM_DIVIDER = 3;

constexpr uint64_t CMD_VEL_TIMEOUT_US = 500000ULL;

constexpr uint IMU_SDA = 4;
constexpr uint IMU_SCL = 5;
constexpr uint IMU_DIVIDER = 2;
