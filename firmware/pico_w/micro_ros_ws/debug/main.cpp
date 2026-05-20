#include "pico/stdlib.h"
#include <stdio.h>
#include "config.hpp"
#include "motor.hpp"

// FRICTION TEST FOR MINIMUM DUTY CYCLE

int main() {
    stdio_init_all();
    
    // Give yourself 5 seconds to get ready after plugging in
    sleep_ms(5000); 

    // Initialize Motor with your existing pinout
    // (Ensure invert_left/right match your wiring)
    Motor motor("Test", "ON", L_DIR, L_PWM, R_DIR, R_PWM, true, true);

    // Start at 50% duty cycle (adjust start_duty if 50% is too slow/fast)
    float duty = 0.50f; 
    float step = 0.01f;

    printf("--- Friction Floor Test Starting ---\n");
    printf("Ramping down from %.2f to 0.00\n", duty);

    while (duty >= 0.0f) {
        // Print current duty cycle so you can see it in Serial Monitor
        printf("Current Duty: %.2f\n", duty);
        
        motor.move(duty, duty);
        
        // Wait 500ms at each step so you can observe the motors
        sleep_ms(500); 
        
        duty -= step;
    }

    // Stop at the end
    motor.stop();
    printf("--- Test Complete. Motors Stopped. ---\n");

    while (true) {
        sleep_ms(1000);
    }
}