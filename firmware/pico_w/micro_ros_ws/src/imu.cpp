#include "imu.hpp"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

sh2_Hal_t IMU::_hal = {};
i2c_inst_t* IMU::_s_i2c = nullptr;
IMU* IMU::_instance = nullptr;

int IMU::hal_open(sh2_Hal_t*)

{
    sleep_ms(300);
    return SH2_OK;
}

void IMU::hal_close(sh2_Hal_t*) {}

int IMU::hal_read(sh2_Hal_t*, uint8_t* buf, unsigned len, uint32_t* t_us)
{
    if (!_s_i2c || len < 4) return 0;

    int rc = i2c_read_timeout_us(_s_i2c, BNO085_I2C_ADDR, buf, len, false, 10000);
    if (rc < 4) return 0;

    uint16_t cargo = ((uint16_t)(buf[1] & 0x7F) << 8) | buf[0];
    if (cargo == 0) return 0;

    *t_us = time_us_32();

    return (int)((cargo <= (uint16_t)rc) ? cargo : (uint16_t)rc);
}

int IMU::hal_write(sh2_Hal_t*, uint8_t* buf, unsigned len)
{
    if (!_s_i2c) return 0;
    int rc = i2c_write_timeout_us(_s_i2c, BNO085_I2C_ADDR, buf, len, false, 10000);
    return (rc == (int)len) ? (int)len : 0;
}

uint32_t IMU::hal_get_time(sh2_Hal_t*)
{
    return time_us_32();
}

void IMU::on_async_event(void* , sh2_AsyncEvent_t* evt)
{
    if (!_instance) return;

    if (evt->eventId == SH2_RESET) {
        printf("[%s] BNO085 reset detected, re-enabling reports\n", _instance->_name);

        sh2_SensorConfig_t cfg = {};
        cfg.reportInterval_us = IMU_REPORT_US;
        if (sh2_setSensorConfig(SH2_ROTATION_VECTOR, &cfg) != SH2_OK) {
            printf("[%s] Failed to re-enable SH2_ROTATION_VECTOR\n", _instance->_name);
        }
        if (sh2_setSensorConfig(SH2_GYROSCOPE_CALIBRATED, &cfg) != SH2_OK) {
            printf("[%s] Failed to re-enable SH2_GYROSCOPE_CALIBRATED\n", _instance->_name);
        }
        if (sh2_setSensorConfig(SH2_LINEAR_ACCELERATION, &cfg) != SH2_OK) {
            printf("[%s] Failed to re-enable SH2_LINEAR_ACCELERATION\n", _instance->_name);
        }
    }
}

void IMU::on_sensor_event(void* , sh2_SensorEvent_t* evt)
{
    if (_instance) _instance->handle_event(evt);
}

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
            break;
    }
}

IMU::IMU(const char* name, const char* status,
        i2c_inst_t* i2c, uint sda_pin, uint scl_pin, uint32_t i2c_freq)
    : _name(name), _status(status),
    _i2c(i2c), _sda(sda_pin), _scl(scl_pin), _freq(i2c_freq),
    _ready(false),
    _qi(0.0f), _qj(0.0f), _qk(0.0f), _qr(1.0f),
    _gx(0.0f), _gy(0.0f), _gz(0.0f),
    _ax(0.0f), _ay(0.0f), _az(0.0f)
{
    _instance = this;
    _s_i2c    = i2c;
}

bool IMU::init()
{

    i2c_init(_i2c, _freq);
    gpio_set_function(_sda, GPIO_FUNC_I2C);
    gpio_set_function(_scl, GPIO_FUNC_I2C);
    gpio_pull_up(_sda);
    gpio_pull_up(_scl);

    _hal.open = hal_open;
    _hal.close = hal_close;
    _hal.read = hal_read;
    _hal.write = hal_write;
    _hal.getTimeUs = hal_get_time;

    int rc = sh2_open(&_hal, on_async_event, nullptr);
    if (rc != SH2_OK) {
        printf("[%s] sh2_open failed (err=%d), check wiring and I2C address\n",
            _name, rc);
        return false;
    }

    rc = sh2_setSensorCallback(on_sensor_event, nullptr);
    if (rc != SH2_OK) {
        printf("[%s] sh2_setSensorCallback failed (err=%d)\n", _name, rc);
        return false;
    }

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
    printf("[%s] BNO085 ready, 3 reports @ %u µs\n", _name, IMU_REPORT_US);
    return true;
}

bool IMU::update()
{
    if (!_ready) return false;
    sh2_service();
    return true;
}

void IMU::ShowStatus()
{
    printf("[%s] Status: %s | %s | SDA: GP%u | SCL: GP%u | Addr: 0x%02X | Rate: %u Hz\n",
            _name, _status,
            (_i2c == i2c0) ? "I2C0" : "I2C1",
            _sda, _scl,
            BNO085_I2C_ADDR,
            1000000u / IMU_REPORT_US);
}
