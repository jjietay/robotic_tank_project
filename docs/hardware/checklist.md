# Wiring Checklist

## Ultrasonic Sensors (HC-SR04P)

### Power
- [ ] All 4× sensor VCC → Pico W **Pin 36 (3V3OUT)** !!! NOT Pin 40 (VBUS)
- [ ] All 4× sensor GND → any Pico W GND pin

### USRM_TL
- [ ] TRIG → Pico W **Pin 9** (GP6)
- [ ] ECHO → Pico W **Pin 10** (GP7)

### USRM_TR
- [ ] TRIG → Pico W **Pin 14** (GP10)
- [ ] ECHO → Pico W **Pin 15** (GP11)

### USRM_BL
- [ ] TRIG → Pico W **Pin 16** (GP12)
- [ ] ECHO → Pico W **Pin 17** (GP13)

### USRM_BR
- [ ] TRIG → Pico W **Pin 19** (GP14)
- [ ] ECHO → Pico W **Pin 20** (GP15)

---

## Motor Driver (MDD10A)

### Signal Wires (Pico W → MDD10A)
- [ ] DIR1 ← Pico W **Pin 1** (GP0)
- [ ] PWM1 ← Pico W **Pin 11** (GP8)
- [ ] DIR2 ← Pico W **Pin 4** (GP2)
- [ ] PWM2 ← Pico W **Pin 12** (GP9)
- [ ] GND ← any Pico W GND pin (shared ground)

### Power Wires (18650 → MDD10A)
- [ ] B+ ← 18650 pack positive (red wire)
- [ ] B− ← 18650 pack negative (black wire)

### Motor Outputs
- [ ] M1A / M1B → LEFT_MOTOR
- [ ] M2A / M2B → RIGHT_MOTOR

### PWM Slice
- [ ] GP8 (PWM slice 4A) and GP9 (PWM slice 4B) share the same PWM slice

---

## Encoders

### Left Encoder
- [ ] ENC_A → Pico W **Pin 21** (GP16)
- [ ] ENC_B → Pico W **Pin 22** (GP17)
- [ ] VCC → 3.3V (check encoder spec)
- [ ] GND → common GND

### Right Encoder
- [ ] ENC_A → Pico W **Pin 24** (GP18)
- [ ] ENC_B → Pico W **Pin 25** (GP19)
- [ ] VCC → 3.3V (check encoder spec)
- [ ] GND → common GND

---

## Power System

- [ ] Pico W powered via USB-C from RPi4
- [ ] RPi4 powered by power bank
- [ ] 18650 pack (~11.1V 3S) powers MDD10A only
- [ ] Common GND shared between Pico W and MDD10A

---

## Before First Power-On

- [ ] Sensor VCC rail confirmed = 3.3V (measure with multimeter)
- [ ] No exposed wire shorts on breadboard
- [ ] Breadboard power rail mid-break bridged (if applicable)
- [ ] PID values (currently 999, 999, 999) are placeholders — do NOT run PID control until tuned