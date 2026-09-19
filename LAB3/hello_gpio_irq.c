/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

volatile uint32_t notch_count = 0;
volatile uint32_t pulse_start_time = 0;
volatile uint32_t pulse_width_us = 0;

#define GPIO_WATCH_PIN 2

static char event_str[128];

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
    // Record the current time in microseconds
    uint32_t current_time = time_us_32();

    if (events & GPIO_IRQ_EDGE_RISE) {
        // A notch started: increment distance counter and mark the start time
        notch_count++;
        pulse_start_time = current_time;
    } 
    else if (events & GPIO_IRQ_EDGE_FALL) {
        // A notch ended: calculate how long the pulse lasted
        pulse_width_us = current_time - pulse_start_time;
    }
}
int main() {
    stdio_init_all();

    printf("Hello GPIO IRQ\n");
    gpio_init(GPIO_WATCH_PIN);
    gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback); // Enable both rising and falling edge interrupts on the GPIO pin

    // Wait forever
    while (1){
        printf("Distance: %u notches | Speed (Pulse Width): %u us\n", notch_count, pulse_width_us);
        sleep_ms(500); // Update the monitor twice a second
    };
}


static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}
