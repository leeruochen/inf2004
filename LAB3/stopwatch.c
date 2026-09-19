#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define BUTTON_PIN 21
#define DEBOUNCE_DELAY_US 50000 // 50 milliseconds

volatile bool is_running = false;
volatile uint32_t elapsed_seconds = 0;
volatile bool print_update = false;
volatile uint32_t last_irq_time_us = 0;

// 1. Hardware Timer Interrupt (Fires every 1000ms)
bool timer_callback(struct repeating_timer *t) {
    if (is_running) {
        elapsed_seconds++;
        print_update = true; // Tell the main loop to print the new time
    }
    return true; // Return true to keep the timer repeating
}

// 2. GPIO Hardware Interrupt (Fires on button press and release)
void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time_us = time_us_32();
    
    // Debounce: Ignore bouncing edges that happen within 50ms of each other
    if ((current_time_us - last_irq_time_us) < DEBOUNCE_DELAY_US) {
        return; 
    }
    last_irq_time_us = current_time_us;

    if (events & GPIO_IRQ_EDGE_FALL) {
        // Maker Pi Pico buttons pull low when pressed (Falling Edge)
        is_running = true;
        elapsed_seconds = 0;
        print_update = true;
    } 
    else if (events & GPIO_IRQ_EDGE_RISE) {
        // Button released (Rising Edge)
        is_running = false;
        elapsed_seconds = 0;
        print_update = true;
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Give time for USB serial monitor to connect

    printf("Stopwatch ready. Press and hold GP21 to start.\n");

    // Initialize GP21 as input with an internal pull-up resistor
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Arm the GPIO interrupt for BOTH pressing and releasing the button
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, 
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, 
                                       true, 
                                       &button_callback);

    // Arm the Hardware Timer Interrupt to fire every 1000ms (1 second)
    struct repeating_timer timer;
    add_repeating_timer_ms(1000, timer_callback, NULL, &timer);

    // 3. The Main Loop (Handles all slow tasks like printf)
    while (true) {
        if (print_update) {
            print_update = false; // Reset the flag immediately
            
            if (is_running) {
                printf("Elapsed time: %u seconds\n", elapsed_seconds);
            } else {
                printf("Timer stopped. Reset to: %u seconds\n\n", elapsed_seconds);
            }
        }
        tight_loop_contents(); // Tell the CPU it can sleep to save power
    }
}

// How the Mechanics Work
// Debouncing: When you push a physical button, the metal contacts bounce against each other, creating dozens of microscopic high/low spikes. Without lines 27-31, the Pico is so fast it would trigger the interrupt dozens of times instantly. The code records last_irq_time_us and forces the processor to ignore any new signals for the next 50,000 microseconds, smoothing the physical signal into a single software event.

// Timer Callback: add_repeating_timer_ms commands a dedicated hardware timer on the RP2040 chip to automatically trigger timer_callback every 1,000 milliseconds.

// Deferring Printf: By setting print_update = true inside the ISRs, the interrupts exit in less than a microsecond. The CPU returns to the while(true) loop, sees the flag, and executes the slow printf operation safely outside the interrupt context.