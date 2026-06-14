#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"

extern "C" {
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
}

constexpr uint8_t BNO085_I2C_ADDR = 0x4A;

constexpr uint32_t IMU_REPORT_US = 20000;

class IMU {
public:

    IMU(const char* name, const char* status,
        i2c_inst_t* i2c, uint sda_pin, uint scl_pin,
        uint32_t i2c_freq = 400000);

    bool init();

    bool update();

    float getQuatI() const{return _qi;}
    float getQuatJ() const{return _qj;}
    float getQuatK() const{return _qk;}
    float getQuatReal() const {return _qr;}

    float getGyroX() const{return _gx;}
    float getGyroY() const{return _gy;}
    float getGyroZ() const{return _gz;}

    float getLinAccelX() const{return _ax;}
    float getLinAccelY() const{return _ay;}
    float getLinAccelZ() const{return _az;}

    void ShowStatus();

private:

    static sh2_Hal_t   _hal;
    static i2c_inst_t* _s_i2c;
    static IMU*        _instance;

    static int hal_open(sh2_Hal_t* self);
    static void hal_close(sh2_Hal_t* self);
    static int hal_read(sh2_Hal_t* self, uint8_t* buf, unsigned len, uint32_t* t_us);
    static int hal_write(sh2_Hal_t* self, uint8_t* buf, unsigned len);
    static uint32_t hal_get_time(sh2_Hal_t* self);

    static void on_async_event(void* cookie, sh2_AsyncEvent_t* evt);

    static void on_sensor_event(void* cookie, sh2_SensorEvent_t* evt);
    void handle_event (sh2_SensorEvent_t* evt);

    const char* _name;
    const char* _status;
    i2c_inst_t* _i2c;
    uint _sda;
    uint _scl;
    uint32_t _freq;
    bool _ready;

    float _qi, _qj, _qk, _qr;
    float _gx, _gy, _gz;
    float _ax, _ay, _az;
};
