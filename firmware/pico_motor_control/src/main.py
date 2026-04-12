from machine import Pin, PWM # type: ignore
import time # type: ignore
import machine # type: ignore

DRIVE_DUTY  = 0.85
TURN_DUTY   = 0.80
INNER_DUTY  = 0.55
OUTER_DUTY  = 0.90

class Electronics:
    def __init__(self, name, status):
        self.name = name
        self.status = status

    def ShowStatus(self):
        print(f"{self.name} initialized.")

    def CheckStatus(self):
        print(f"{self.name} disconnected.")

class Encoder(Electronics):
    # NOTE: AB Dual-Phase Incremental Magnetic Hall Encoder
    
    _PPR = 11 # Pulses per revolution
    
    def __init__(self,name, status, _pin_a, _pin_b, _reduction_ratio=1):   # TO CHANGE REDUCTION_RATIO
        super().__init__(name, status)
        # Pull up holds the pins at HIGH when no signal is present
        self._pin_a = Pin(_pin_a, PIN.IN, Pin.PULL_UP) # type:ignore
        self._pin_b = Pin(_pin_b, PIN.IN, Pin.PULL_UP) # type:ignore
        self._reduction_ratio = _reduction_ratio
        self._cpr_motor = 4 * self._PPR                # counts per revolution (4 edges of A and B), raw motor shaft resolution
        self._cpr_output = self._cpr_motor * _reduction_ratio  # counts per rev for output after gearbox (reduction_ratio)
        # this is how many counts equal one full wheel turn

        self._counts = 0
        self._last_counts = 0
        self._last_time = time.ticks_us()

        # Attach IRQs to both edges of both channels A and B --> IRQ (Interrupt Request) will make CPU suspend its current task to handle to request
        self._pin_a.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=self._callback_a)
        self._pin_b.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=self.callback_b)

    # ----------------------------------------- #
    # IRQ Callbacks (called at interrupt level)
    # ----------------------------------------- #

    def _callback_a(self):
        a = self._pin_a.value()
        b = self._pin_b.value()
        self.counts += 1 if (a==b) else -1

    def _callback_b(self):
        a = self._pin_a.value()
        b =self._pin_b.value()
        self._counts += 1 if (a!=b) else -1


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
        


# START OF CODE
USRM_TL = Ultrasonic("TL", "ON", 6, 7)
USRM_TR = Ultrasonic("TR", "ON", 10, 11)
USRM_BL = Ultrasonic("BL", "ON", 12, 13)
USRM_BR = Ultrasonic("BR", "ON", 14, 15)
MOTOR = Motor("LEFT_RIGHT_MOTORS","ON", l_in1=0, l_in2=1, l_en=8, r_in1=2, r_in2=3, r_en=9)
USRM_TL.ShowStatus()
USRM_TR.ShowStatus()
USRM_BL.ShowStatus()
USRM_BR.ShowStatus()
MOTOR.ShowStatus()

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
    
    
        
    




