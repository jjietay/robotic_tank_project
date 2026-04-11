from machine import Pin, PWM # type: ignore
import time # type: ignore

DRIVE_DUTY  = 0.85
TURN_DUTY   = 0.80
INNER_DUTY  = 0.55
OUTER_DUTY  = 0.90

class Sensor:
    def __init__(self, name, status):
        self.name = name
        self.status = status

    def ShowStatus(self):
        print(f"{self.name} initialized.")

    def CheckStatus(self):
        print(f"{self.name} disconnected.")


class Ultrasonic(Sensor):
    def __init__(self, name, status, trigger_pin, echo_pin, current=15e-3, voltage=5, sound_vel=340):
        super().__init__(name, status)
        self.trigger_pin = Pin(trigger_pin, Pin.OUT)
        self.echo_pin = Pin(echo_pin, Pin.IN)
        self.sound_vel = sound_vel          # 340m/s
    
    def distance(self):
        self.trigger_pin.low()              # send 0 to ultrasonic sensor
        time.sleep_us(2)                    # sleep for 2 microseconds
        self.trigger_pin.high()             # send 1 to ultrasonic sensor
        time.sleep_us(10)                   # 10us for input (into sensor) to register
        self.trigger_pin.low()

        timeout = time.ticks_us()
        while not self.echo_pin.value():    # if ECHO is 0 (False) ---> not ECHO is 1 (True)
            if time.ticks_diff(time.ticks_us(), timeout) > 38000:
                return None                     # sensor not responding
            
        time1 = time.ticks_us()             # Start Stopwatch
        while self.echo_pin.value():
            pass
        time2 = time.ticks_us()             # Stop Stopwatch
        diff = time.ticks_diff(time2, time1)
        if diff > 38000:                    # If out of range
            return "out of range"
        else:
            dist = (diff * 1e-6 * self.sound_vel / 2) * 100
            return dist     # in cm


class Motor:
    def __init__(self, l_in1, l_in2, l_en, r_in1, r_in2, r_en, freq=1907):
        self.l_in1 = Pin(l_in1, Pin.OUT)
        self.l_in2 = Pin(l_in2, Pin.OUT)
        self.l_pwm = PWM(Pin(l_en));  self.l_pwm.freq(freq)

        self.r_in1 = Pin(r_in1, Pin.OUT)
        self.r_in2 = Pin(r_in2, Pin.OUT)
        self.r_pwm = PWM(Pin(r_en));  self.r_pwm.freq(freq)

    def _set(self, in1, in2, pwm, speed):       # private class
        speed = max(-1.0, min(1.0, speed))
        if speed > 0:   in1.value(0); in2.value(1)
        elif speed < 0: in1.value(1); in2.value(0)
        else:           in1.value(1); in2.value(1)  # brake
        pwm.duty_u16(int(abs(speed) * 65535))

    def move(self, left, right):
        self._set(self.l_in1, self.l_in2, self.l_pwm, left)
        self._set(self.r_in1, self.r_in2, self.r_pwm, right)

    def forward(self):          self.move( DRIVE_DUTY,  DRIVE_DUTY)
    def backward(self):         self.move(-DRIVE_DUTY, -DRIVE_DUTY)
    def tank_turn_left(self):   self.move( TURN_DUTY, -TURN_DUTY)
    def tank_turn_right(self):  self.move(-TURN_DUTY,  TURN_DUTY)
    def forward_left(self):     self.move( INNER_DUTY,  OUTER_DUTY)
    def forward_right(self):    self.move( OUTER_DUTY,  INNER_DUTY)
    def backward_left(self):    self.move(-INNER_DUTY, -OUTER_DUTY)
    def backward_right(self):   self.move(-OUTER_DUTY, -INNER_DUTY)
    def stop(self):             self.move(0, 0)


class PID:
    def __init__(self, kp, ki, kd, setpoint=0.0):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.setpoint = setpoint



# START OF CODE
USRM_TL = Ultrasonic("TL", "ON", 6, 7)
USRM_TR = Ultrasonic("TR", "ON", 10, 11)
USRM_BL = Ultrasonic("BL", "ON", 12, 13)
USRM_BR = Ultrasonic("BR", "ON", 14, 15)
MOTOR = Motor(l_in1=0, l_in2=1, l_en=8, r_in1=2, r_in2=3, r_en=9)


while True:
    data = input().strip()          # receives "0.85 -0.85"
    try:
        left, right = map(float, data.split())
        MOTOR.move(left, right)
        print("OK")
    except:
        print("ERROR")