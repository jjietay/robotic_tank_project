#include "hardware/pwm.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define LED_PIN PICO_DEFAULT_LED_PIN

const uint TOP = 65535;

//////////////// SINGLE L298N /////////////////////
// LEFT MOTOR
const uint IN_L1 = 0; // GPIO0
const uint IN_L2 = 1; // GPIO1
const uint EN_L = 8;  // GPIO8 PWM

// RIGHT MOTOR
const uint IN_R1 = 2; // GPIO2
const uint IN_R2 = 3; // GPIO3
const uint EN_R = 9;  // GPIO9 PWM

//////////////// SPEED SETTINGS ///////////////////
const float DRIVE_DUTY = 0.85f;
const float TURN_DUTY  = 0.80f;
const float INNER_DUTY = 0.55f;
const float OUTER_DUTY = 0.90f;

//////////////// HELPER FUNCTIONS //////////////////

uint duty(float fraction) {
  if (fraction < 0.0f)
    fraction = 0.0f;
  if (fraction > 1.0f)
    fraction = 1.0f;
  return (uint)(TOP * fraction);
}

void setup_pwm(uint pin) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(pin);
  pwm_set_wrap(slice_num, TOP);
  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(pin), 0);
  pwm_set_enabled(slice_num, true);
}

void setup_gpio() {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0);

  gpio_init(IN_L1);
  gpio_set_dir(IN_L1, GPIO_OUT);
  gpio_init(IN_L2);
  gpio_set_dir(IN_L2, GPIO_OUT);
  gpio_init(IN_R1);
  gpio_set_dir(IN_R1, GPIO_OUT);
  gpio_init(IN_R2);
  gpio_set_dir(IN_R2, GPIO_OUT);
}

void set_left_speed(uint speed) { pwm_set_gpio_level(EN_L, speed); }

void set_right_speed(uint speed) { pwm_set_gpio_level(EN_R, speed); }

void set_speed(uint left_speed, uint right_speed) {
  set_left_speed(left_speed);
  set_right_speed(right_speed);
}

void left_forward() {
  gpio_put(IN_L1, 0);
  gpio_put(IN_L2, 1);
}

void left_backward() {
  gpio_put(IN_L1, 1);
  gpio_put(IN_L2, 0);
}

void right_forward() {
  gpio_put(IN_R1, 0);
  gpio_put(IN_R2, 1);
}

void right_backward() {
  gpio_put(IN_R1, 1);
  gpio_put(IN_R2, 0);
}

void stop() {
  set_speed(0, 0);
  gpio_put(IN_L1, 0);
  gpio_put(IN_L2, 0);
  gpio_put(IN_R1, 0);
  gpio_put(IN_R2, 0);
}

void forward() {
  set_speed(duty(DRIVE_DUTY), duty(DRIVE_DUTY));
  left_forward();
  right_forward();
}

void backward() {
  set_speed(duty(DRIVE_DUTY), duty(DRIVE_DUTY));
  left_backward();
  right_backward();
}

void tank_turn_left() {
  set_speed(duty(TURN_DUTY), duty(TURN_DUTY));
  left_backward();
  right_forward();
}

void tank_turn_right() {
  set_speed(duty(TURN_DUTY), duty(TURN_DUTY));
  left_forward();
  right_backward();
}

void forward_left() {
  set_speed(duty(INNER_DUTY), duty(OUTER_DUTY));
  left_forward();
  right_forward();
}

void forward_right() {
  set_speed(duty(OUTER_DUTY), duty(INNER_DUTY));
  left_forward();
  right_forward();
}

void backward_left() {
  set_speed(duty(INNER_DUTY), duty(OUTER_DUTY));
  left_backward();
  right_backward();
}

void backward_right() {
  set_speed(duty(OUTER_DUTY), duty(INNER_DUTY));
  left_backward();
  right_backward();
}

//////////////// MAIN FUNCTION /////////////////////

int main() {
  stdio_init_all();
  setup_gpio();

  setup_pwm(EN_L);
  setup_pwm(EN_R);

  while (!stdio_usb_connected()) {
    sleep_ms(100);
  }

  for (int i = 0; i < 3; i++) {
    gpio_put(LED_PIN, 1);
    sleep_ms(100);
    gpio_put(LED_PIN, 0);
    sleep_ms(400);
  }

  stop();

  while (true) {
    int c = getchar();
    if (c == PICO_ERROR_TIMEOUT || c == EOF)
      continue;

    gpio_put(LED_PIN, 1);

    switch ((char)c) {
    case 'W':
      forward();
      printf("Forward\n");
      break;
    case 'S':
      backward();
      printf("Backward\n");
      break;
    case 'A':
      tank_turn_right();
      printf("Right\n");
      break;
    case 'D':
      tank_turn_left();
      printf("Left\n");
      break;
    case 'U':
      forward_left();
      printf("Forward Left\n");
      break;
    case 'I':
      forward_right();
      printf("Forward Right\n");
      break;
    case 'O':
      backward_left();
      printf("Backward Left\n");
      break;
    case 'P':
      backward_right();
      printf("Backward Right\n");
      break;
    case 'X':
      stop();
      printf("Stop\n");
      break;
    default:
      break;
    }

    gpio_put(LED_PIN, 0);
  }
}
