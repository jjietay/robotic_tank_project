from machine import Pin, PWM 
import time 
import machine 

DRIVE_DUTY  = 0.85
TURN_DUTY   = 0.80

class Electronics:
    def __init__(self, name, status):
        self.name = name
        self.status = status

    def ShowStatus(self):
        print(f"{self.name} initialized.")

    def CheckStatus(self):
        print(f"{self.name} disconnected.")


# NOTE: Quadrature Incremental Magnetic Encoder
class Encoder(Electronics):
    _PPR = 11   # Pulses per revolution - the number of possible highs and lows per rev PER PIN !!

    def __init__(self, name, status, _pin_a, _pin_b, _reduction_ratio = 1, _diameter=4.7):
        super().__init__(name, status)
        # Pull up holds the pins at HIGH when no signal is present, this is to prevent floating inputs
        self._pin_a = Pin(_pin_a, Pin.IN, Pin.PULL_UP)
        self._pin_b = Pin(_pin_b, Pin.IN, Pin.PULL_UP)

        self._reduction_ratio = _reduction_ratio
        self._diameter = _diameter # 4.7cm
        self._circumference = 2 * 3.141592653589793 * (_diameter/2)
        self._cpr_motor = 4 * self._PPR     # Counts per revolution OF SHAFT measures the highs and lows on both A and B (QUADRATURE)
        self._cpr_output = self._cpr_motor * self._reduction_ratio # Counts per revolution OF WHEEL measured with the highs and lows of both A and B

        # State variables
        self.count = 0
        self._last_count = 0
        self._last_time = time.ticks_us()

        self._pin_a.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=self._callback_a)     # Whenever pin rising or falling, stop all tasks first and run _callback_a function 
        self._pin_b.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=self._callback_b)      # Whenever pin rising or falling, stop all tasks first and run _callback_b function 

    def _callback_a(self, _pin_a):         
        a = _pin_a.value()
        b = self._pin_b.value()
        if a == b:
            self.count -= 1         
        else:
            self.count += 1        

    def _callback_b(self, _pin_b):         
        a = self._pin_a.value()
        b = _pin_b.value()
        if a == b:
            self.count += 1         
        else:
            self.count -= 1        

    def get_count(self):
        return self.count
    
    def get_vel(self):
        _now_time = time.ticks_us()
        time_diff = time.ticks_diff(_now_time, self._last_time)
        if time_diff <= 0:
            return 0.0
        
        time_diff = time_diff * 1e-6                                # convert from micro seconds to seconds    
        vel = self._get_distance_delta() / time_diff
        self._last_time = _now_time                                 # reset
        self._last_count = self.count
        return vel                                                  # in cm/s
    
    "Helper Method (Private Method) to calculate distance change since last velocity check."
    def _get_distance_delta(self):
        count_diff = self.count - self._last_count
        distance_diff = (count_diff / self._cpr_output) * self._circumference
        return distance_diff


class Ultrasonic(Electronics):
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
            if time.ticks_diff(time.ticks_us(), timeout) > 38000:
                return "out of range"
        time2 = time.ticks_us()             # Stop Stopwatch
        diff = time.ticks_diff(time2, time1)
        if diff > 38000:                    # If out of range
            return "out of range"
        else:
            dist = (diff * 1e-6 * self.sound_vel / 2) * 100
            return dist     # in cm
        

class Motor(Electronics):     # NOTE: TAKES IN (0.2, 0.4) from RPI4
    def __init__(self, name, status, l_in1, l_in2, l_en, r_in1, r_in2, r_en, freq=1907):
        super().__init__(name, status)
        self.l_in1 = Pin(l_in1, Pin.OUT)
        self.l_in2 = Pin(l_in2, Pin.OUT)
        self.l_pwm = PWM(Pin(l_en));  self.l_pwm.freq(freq)

        self.r_in1 = Pin(r_in1, Pin.OUT)
        self.r_in2 = Pin(r_in2, Pin.OUT)
        self.r_pwm = PWM(Pin(r_en));  self.r_pwm.freq(freq)

    def _set(self, in1, in2, pwm, speed):               # private class (speed between -1 to 1)
        speed = max(-1.0, min(1.0, speed))              # used to clamp speed within -1 to 1
        if speed > 0:   in1.value(0); in2.value(1)      # THIS IS FOR DIRECTION (W)
        elif speed < 0: in1.value(1); in2.value(0)      # THIS IS FOR DIRECTION (S)
        else:           in1.value(1); in2.value(1)      # THIS IS FOR DIRECTION (STOP)
        pwm.duty_u16(int(abs(speed) * 65535))           # 0 - 65535 (DUTY CYCLE - CHANGE SPEED)

    def move(self, left, right):
        self._set(self.l_in1, self.l_in2, self.l_pwm, left)
        self._set(self.r_in1, self.r_in2, self.r_pwm, right)

    # THIS IS FOR FIXED MOVEMENTS
    def forward(self):          self.move( DRIVE_DUTY,  DRIVE_DUTY)
    def backward(self):         self.move(-DRIVE_DUTY, -DRIVE_DUTY)
    def tank_turn_left(self):   self.move( TURN_DUTY, -TURN_DUTY)
    def tank_turn_right(self):  self.move(-TURN_DUTY,  TURN_DUTY)
    def stop(self):             self.move(0, 0)


class PID:
    def __init__(self, kp, ki, kd, setpoint=0.0):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.setpoint = setpoint

        self.integral = 0.0
        self.previous_error = 0.0
        self.last_time = time.ticks_us()

    def calculate(self, setpoint, current_value):
        current_time = time.ticks_us()
        dt = time.ticks_diff(current_time, self.last_time) / 1000000 # Convert ms to seconds

        if dt <= 0.0:
            dt = 0.01
        
        error = setpoint - current_value
        self.integral += error * dt
        self.integral = max(-10.0, min(10.0, self.integral))
        
        p_term = self.kp * error
        i_term = self.ki * self.integral
        d_term = self.kd * ((error - self.previous_error) / dt) 
        
        self.previous_error = error
        self.last_time = current_time

        output = p_term + i_term + d_term
        return output
        


# INITIALISING COMPONENTS
USRM_TL = Ultrasonic("TL", "ON", 6, 7)
USRM_TR = Ultrasonic("TR", "ON", 10, 11)
USRM_BL = Ultrasonic("BL", "ON", 12, 13)
USRM_BR = Ultrasonic("BR", "ON", 14, 15)
MOTOR = Motor("LEFT_RIGHT_MOTORS","ON", l_in1=0, l_in2=1, l_en=8, r_in1=2, r_in2=3, r_en=9)
ENCODER = Encoder("ENC", "ON", 999, 999)
PID = PID(999, 999, 999)
USRM_TL.ShowStatus()
USRM_TR.ShowStatus()
USRM_BL.ShowStatus()
USRM_BR.ShowStatus()
MOTOR.ShowStatus()
ENCODER.ShowStatus()

# MAIN LOOP
while True:
    data = input().strip()         # receives "(0.85, 0.75)" ---> "(left_motor_pwm, right_motor_pwm)"
    try:
        left, right = map(float, data.strip("()").split(","))
        MOTOR.move(left, right)
        print(f"Changed pwm for motors to {left} and {right} respectively")
    except:
        print("ERROR: Change of speed.")

    dist = []
    dist.append(USRM_TL.distance())
    dist.append(USRM_TR.distance())
    dist.append(USRM_BL.distance())
    dist.append(USRM_BR.distance())

    for element in dist:
        if isinstance(element, (int, float)):
            if element < 3:
                MOTOR.stop()
                break
    
    
        
    




