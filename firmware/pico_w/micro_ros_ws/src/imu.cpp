// ---------------------------------------------------------------------------
//                    imu.cpp  —  BNO085 IMU wrapper (Pico SDK + SH2)
// ---------------------------------------------------------------------------

#include "imu.hpp"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ---------------------------------------------------------------------------
//  Static member definitions
// ---------------------------------------------------------------------------
sh2_Hal_t   IMU::_hal      = {};
i2c_inst_t* IMU::_s_i2c   = nullptr;        
IMU*        IMU::_instance = nullptr;

// ---------------------------------------------------------------------------
//  HAL: open
//
//  Called once by sh2_open() to initialise the transport link.
//  I2C is already configured by init(), so we only need to wait for the
//  BNO085 to finish its internal boot sequence (~300 ms).
// ---------------------------------------------------------------------------
int IMU::hal_open(sh2_Hal_t*)   // just a simple function to ensure things work
                                // init() already setup the hardware via gpio_set_function()
{
    sleep_ms(300);
    return SH2_OK;
}

// ---------------------------------------------------------------------------
//  HAL: close
//  Nothing to tear down for I2C; leave the peripheral running.
// ---------------------------------------------------------------------------
void IMU::hal_close(sh2_Hal_t*) {}

// ---------------------------------------------------------------------------
//  HAL: read
//  SHTP is Sensor Hub Transport Protocol, is a standard protocol used by 
// sensor hubs like BN0080 to transmit complex, multi-axis sensor data
// to a host microcontroller

//  BNO085 SHTP-over-I2C Packet Structure:
//    Byte 0 : cargo length LSB
//    Byte 1 : cargo length MSB   (bit 15 = continuation flag — mask with 0x7F)
//    Byte 2 : channel number
//    Byte 3 : sequence number
//    Bytes 4..N : payload
//

//  Sensor Hub Transport Protocol (SHTP) splits traffic across specific channels:
//  Command (0), Executable (1), Control (2), Reports (3)

//  Strategy:
//    Read the whole packet (up to `len` bytes) in a single I2C transaction.
//    The BNO085 clock-stretches if it is not ready, so the blocking call
//    is safe.  Returning 0 tells the SH2 library there is no new data yet.
//
//  Note: `len` is the size of the SH2 library's internal receive buffer
//  (typically 512 bytes), so we never ask for more than the device is
//  likely to send.
// ---------------------------------------------------------------------------
int IMU::hal_read(sh2_Hal_t*, uint8_t* buf, unsigned len, uint32_t* t_us)
{
    if (!_s_i2c || len < 4) return 0;

    int rc = i2c_read_blocking(_s_i2c, BNO085_I2C_ADDR, buf, len, false);
    if (rc < 4) return 0;   // negative = NACK or timeout; < 4 = incomplete header

    // Parse cargo length and mask off the continuation bit in byte 1
    uint16_t cargo = ((uint16_t)(buf[1] & 0x7F) << 8) | buf[0];
    if (cargo == 0) return 0;  // BNO085 has nothing to send right now

    *t_us = time_us_32();

    // Return the smaller of the declared cargo length and what we actually read.
    // This protects against a corrupt/oversized length field.
    return (int)((cargo <= (uint16_t)rc) ? cargo : (uint16_t)rc);
}

// ---------------------------------------------------------------------------
//  HAL: write
//
//  The SH2 library calls this to send SHTP packets to the BNO085
//  (e.g. to set sensor report rates, request product IDs, etc.).
// ---------------------------------------------------------------------------
int IMU::hal_write(sh2_Hal_t*, uint8_t* buf, unsigned len)
{
    if (!_s_i2c) return 0;
    int rc = i2c_write_blocking(_s_i2c, BNO085_I2C_ADDR, buf, len, false);
    return (rc == (int)len) ? (int)len : 0;
}

// ---------------------------------------------------------------------------
//  HAL: getTimeUs
//  Returns a 32-bit microsecond timestamp.  The SH2 library uses this for
//  internal timing and report-interval scheduling.
// ---------------------------------------------------------------------------
uint32_t IMU::hal_get_time(sh2_Hal_t*)
{
    return time_us_32();
}

// ---------------------------------------------------------------------------
//  Static sensor-event callback
//  The SH2 library calls this (from inside sh2_service()) for every decoded
//  sensor packet.  We simply forward to the owning instance.
// ---------------------------------------------------------------------------
void IMU::on_sensor_event(void* /*cookie*/, sh2_SensorEvent_t* evt)
{
    if (_instance) _instance->handle_event(evt);
}

// ---------------------------------------------------------------------------
//  Instance event handler
//  Unpacks the sh2_SensorValue_t union and stores the values we care about.
//
//  SH2_ROTATION_VECTOR   : absolute orientation fused with magnetometer
//  SH2_GYROSCOPE_CALIBRATED : bias-corrected angular velocity in rad/s
//  SH2_LINEAR_ACCELERATION  : acceleration with gravity subtracted, in m/s²
// ---------------------------------------------------------------------------
void IMU::handle_event(sh2_SensorEvent_t* evt)
{
    sh2_SensorValue_t v;
    if (sh2_decodeSensorEvent(&v, evt) != SH2_OK) return;

    switch (v.sensorId)
    {
        case SH2_ROTATION_VECTOR:
            _qi = v.un.rotationVector.i;
            _qj = v.un.rotationVector.j;
            _qk = v.un.rotationVector.k;
            _qr = v.un.rotationVector.real;
            break;

        case SH2_GYROSCOPE_CALIBRATED:
            _gx = v.un.gyroscope.x;
            _gy = v.un.gyroscope.y;
            _gz = v.un.gyroscope.z;
            break;

        case SH2_LINEAR_ACCELERATION:
            _ax = v.un.linearAcceleration.x;
            _ay = v.un.linearAcceleration.y;
            _az = v.un.linearAcceleration.z;
            break;

        default:
            break;  // ignore other report types
    }
}

// ---------------------------------------------------------------------------
//  Constructor
// ---------------------------------------------------------------------------
IMU::IMU(const char* name, const char* status,
        i2c_inst_t* i2c, uint sda_pin, uint scl_pin, uint32_t i2c_freq)
    : _name(name), _status(status),
    _i2c(i2c), _sda(sda_pin), _scl(scl_pin), _freq(i2c_freq),
    _ready(false),
    _qi(0.0f), _qj(0.0f), _qk(0.0f), _qr(1.0f),   // identity quaternion
    _gx(0.0f), _gy(0.0f), _gz(0.0f),
    _ax(0.0f), _ay(0.0f), _az(0.0f)
{
    _instance = this;
    _s_i2c    = i2c;
}

// ---------------------------------------------------------------------------
//  init
//
//  1. Brings up the I2C peripheral on the requested pins.
//  2. Wires up the HAL struct and calls sh2_open().
//  3. Enables three sensor reports at IMU_REPORT_US interval.
//
//  Returns true on full success.  Any failure prints a diagnostic and
//  leaves _ready = false so update() becomes a no-op.
// ---------------------------------------------------------------------------
bool IMU::init()
{
    // ---- I2C peripheral init -----------------------------------------------
    i2c_init(_i2c, _freq);
    gpio_set_function(_sda, GPIO_FUNC_I2C);
    gpio_set_function(_scl, GPIO_FUNC_I2C);
    gpio_pull_up(_sda);
    gpio_pull_up(_scl);

    // ---- Wire up SH2 HAL ---------------------------------------------------
    _hal.open      = hal_open;
    _hal.close     = hal_close;
    _hal.read      = hal_read;
    _hal.write     = hal_write;
    _hal.getTimeUs = hal_get_time;

    // ---- Open SH2 transport (internally calls hal_open + handshakes) -------
    int rc = sh2_open(&_hal, on_sensor_event, nullptr);
    if (rc != SH2_OK) {
        printf("[%s] sh2_open failed (err=%d) — check wiring and I2C address\n",
            _name, rc);
        return false;
    }

    // ---- Enable sensor reports ---------------------------------------------
    sh2_SensorConfig_t cfg = {};
    cfg.reportInterval_us  = IMU_REPORT_US;

    if (sh2_setSensorConfig(SH2_ROTATION_VECTOR, &cfg) != SH2_OK) {
        printf("[%s] Failed to enable SH2_ROTATION_VECTOR\n", _name);
        return false;
    }
    if (sh2_setSensorConfig(SH2_GYROSCOPE_CALIBRATED, &cfg) != SH2_OK) {
        printf("[%s] Failed to enable SH2_GYROSCOPE_CALIBRATED\n", _name);
        return false;
    }
    if (sh2_setSensorConfig(SH2_LINEAR_ACCELERATION, &cfg) != SH2_OK) {
        printf("[%s] Failed to enable SH2_LINEAR_ACCELERATION\n", _name);
        return false;
    }

    _ready = true;
    printf("[%s] BNO085 ready — 3 reports @ %u µs\n", _name, IMU_REPORT_US);
    return true;
}

// ---------------------------------------------------------------------------
//  update
//
//  Pumps the SH2 service loop once.  The library internally calls hal_read(),
//  decodes any complete SHTP packets, and fires on_sensor_event() for each
//  new sensor report.  When no data is available, hal_read() returns 0 and
//  the call completes in microseconds.
//
//  Call this every main loop tick (100 Hz is fine — the overhead when idle
//  is negligible).  Publish the getter values at whatever rate you prefer.
// ---------------------------------------------------------------------------
bool IMU::update()
{
    if (!_ready) return false;
    sh2_service();
    return true;
}

// ---------------------------------------------------------------------------
//  ShowStatus
// ---------------------------------------------------------------------------
void IMU::ShowStatus()
{
    printf("[%s] Status: %s | %s | SDA: GP%u | SCL: GP%u | Addr: 0x%02X | Rate: %u Hz\n",
            _name, _status,
            (_i2c == i2c0) ? "I2C0" : "I2C1",
            _sda, _scl,
            BNO085_I2C_ADDR,
            1000000u / IMU_REPORT_US);
}