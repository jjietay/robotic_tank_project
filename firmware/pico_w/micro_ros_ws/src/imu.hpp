// ---------------------------------------------------------------------------
//                    imu.hpp  —  BNO085 IMU wrapper (Pico SDK + SH2)
// ---------------------------------------------------------------------------
//  Wraps the Hillcrest SH2 library to expose:
//    - Orientation  : unit quaternion (rotation vector, sensor-fused)
//    - Angular vel  : calibrated gyroscope in rad/s
//    - Linear accel : gravity-removed acceleration in m/s²
//
//  Follows the same class pattern as Ultrasonic, Motor, and Encoder.
//
//  Usage in main.cpp:
//    IMU imu("IMU", "ON", i2c0, IMU_SDA, IMU_SCL);
//    imu.init();       // call once after hardware init, before main loop
//    ...
//    imu.update();     // call every main loop tick — pumps SH2 service
//
//  Required SH2 source files (copy from Adafruit_BNO08x/src/sh2/):
//    sh2.c / sh2.h
//    sh2_SensorValue.c / sh2_SensorValue.h
//    sh2_err.h
//    sh2_hal.h
//    shtp.c / shtp.h
//    sh2_util.c / sh2_util.h
//  Place them in firmware/pico_w/micro_ros_ws/src/sh2/ and add to CMakeLists.
// ---------------------------------------------------------------------------

#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"

extern "C" {
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
}

// ---------------------------------------------------------------------------
//  BNO085 I2C address — 0x4A when PS0 and PS1 are floating or pulled low.
//  Change to 0x4B if you tie PS1 high.
// ---------------------------------------------------------------------------
constexpr uint8_t BNO085_I2C_ADDR = 0x4A;

// ---------------------------------------------------------------------------
//  Report interval for all enabled sensor reports.
//  20 000 µs = 50 Hz.  Decrease to 10 000 for 100 Hz if needed.
// ---------------------------------------------------------------------------
constexpr uint32_t IMU_REPORT_US = 20000;

// ---------------------------------------------------------------------------
//  Class IMU
// ---------------------------------------------------------------------------
class IMU {
public:
    // -------------------------------------------------------------------------
    //  Constructor
    //    name      : label printed by ShowStatus()
    //    status    : "ON" or "OFF" — same convention as other classes
    //    i2c       : i2c0 or i2c1
    //    sda_pin   : GPIO pin for SDA (must be valid I2C pin for chosen bus)
    //    scl_pin   : GPIO pin for SCL (must be valid I2C pin for chosen bus)
    //    i2c_freq  : I2C clock in Hz, default 400 kHz (fast mode)
    // -------------------------------------------------------------------------
    IMU(const char* name, const char* status,
        i2c_inst_t* i2c, uint sda_pin, uint scl_pin,
        uint32_t i2c_freq = 400000);

    // -------------------------------------------------------------------------
    //  init  — configure I2C peripheral, open SH2 link, enable sensor reports.
    //  Returns true on success.  Call once, before the main loop.
    // -------------------------------------------------------------------------
    bool init();

    // -------------------------------------------------------------------------
    //  update  — pump the SH2 service loop.  Call once per main loop tick.
    //  When the BNO085 has new data, the internal callback updates the stored
    //  values; when it does not, the call returns quickly.
    //  Returns false if init() was not called or failed.
    // -------------------------------------------------------------------------
    bool update();

    // ---- Rotation vector (unit quaternion, magnetometer-fused) --------------
    float getQuatI() const{return _qi;}
    float getQuatJ() const{return _qj;}
    float getQuatK() const{return _qk;}
    float getQuatReal() const {return _qr;}

    // ---- Calibrated angular velocity (rad/s) ---------------------------------
    float getGyroX() const{return _gx;}
    float getGyroY() const{return _gy;}
    float getGyroZ() const{return _gz;}

    // ---- Linear acceleration with gravity removed (m/s²) --------------------
    float getLinAccelX() const{return _ax;}
    float getLinAccelY() const{return _ay;}
    float getLinAccelZ() const{return _az;}

    void ShowStatus();

private:
    // -------------------------------------------------------------------------
    //  SH2 HAL function pointers must be plain C functions (no 'this').
    //  _s_i2c and _instance are static so the HAL callbacks can reach them.
    //  This limits the design to one IMU instance — fine for this project.
    // -------------------------------------------------------------------------
    static sh2_Hal_t   _hal;
    static i2c_inst_t* _s_i2c;
    static IMU*        _instance;

    // SH2 HAL callbacks
    static int hal_open(sh2_Hal_t* self);
    static void hal_close(sh2_Hal_t* self);
    static int hal_read(sh2_Hal_t* self, uint8_t* buf, unsigned len, uint32_t* t_us);
    static int hal_write(sh2_Hal_t* self, uint8_t* buf, unsigned len);
    static uint32_t hal_get_time(sh2_Hal_t* self);

    // SH2 async-event callback (resets, shutdowns) — required by sh2_open()
    static void on_async_event(void* cookie, sh2_AsyncEvent_t* evt);

    // SH2 sensor-event callback — registered via sh2_setSensorCallback()
    static void on_sensor_event(void* cookie, sh2_SensorEvent_t* evt);
    void handle_event (sh2_SensorEvent_t* evt);

    // Instance fields
    const char* _name;
    const char* _status;
    i2c_inst_t* _i2c;
    uint _sda;
    uint _scl;
    uint32_t _freq;
    bool _ready;

    // Latest sensor values
    float _qi, _qj, _qk, _qr;   // quaternion (real = _qr)
    float _gx, _gy, _gz;        // gyroscope (rad/s)
    float _ax, _ay, _az;        // lin. accel (m/s²)
};