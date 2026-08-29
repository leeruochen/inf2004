/*
 * LAB 3 - HC-SR04P ultrasonic ranger, the naive way.
 * INF2004 LAB 3
 *
 * Adapted from https://github.com/KleistRobotics/Pico-Ultrasonic
 *
 * Wiring:  TRIG -> GP16,  ECHO -> GP17,  VCC -> 3V3,  GND -> GND
 *          (HC-SR04P or HC-SR04+ only. The plain HC-SR04 is a 5 V part
 *           and will damage the Pico's input.)
 *
 * How it is supposed to work, from the README:
 *   1. hold TRIG high for at least 10 us, then bring it low
 *   2. the module emits eight 40 kHz pulses and raises ECHO
 *   3. ECHO stays high for the round-trip time of the sound
 *   4. distance = (time * speed of sound) / 2
 *
 * THREE defects are planted. One stops it compiling; two let it compile
 * and give the wrong answer. The block-waiting is NOT one of them - that
 * is a design flaw, and you are asked to argue about it separately.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const uint TRIG_PIN = 16;
const uint ECHO_PIN = 17;

void setup_ultrasonic_pins(uint trig_pin, uint echo_pin)
{
    gpio_init(trig_pin);
    gpio_init(echo_pin);
    gpio_set_dir(trig_pin, GPIO_OUT);
    gpio_set_dir(echo_pin, GPIO_IN);
}

/*
 * Fire one ping and return the width of the ECHO pulse in microseconds.
 */
uint64_t get_pulse_us(uint trig_pin, uint echo_pin)
{
    absolute_time_t start_time, end_time

    gpio_put(trig_pin, 1);
    sleep_us(10);

    /* Block until ECHO goes high. See the README: this is the design flaw. */
    while (gpio_get(echo_pin) == 0)
        tight_loop_contents();
    start_time = get_absolute_time();

    /* Block until ECHO goes low again. */
    while (gpio_get(echo_pin) == 1)
        tight_loop_contents();
    end_time = get_absolute_time();

    return (uint64_t)absolute_time_diff_us(start_time, end_time);
}

/*
 * Convert that pulse width to centimetres.
 *
 * Sound travels at roughly 343 m/s, which is 29.1 microseconds per
 * centimetre.
 */
uint64_t get_distance_cm(uint trig_pin, uint echo_pin)
{
    uint64_t pulse_us = get_pulse_us(trig_pin, echo_pin);

    return pulse_us / 29;
}

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);

    setup_ultrasonic_pins(TRIG_PIN, ECHO_PIN);

    printf("LAB 3 - ultrasonic ranger\n");

    while (true) {
        uint64_t cm = get_distance_cm(TRIG_PIN, ECHO_PIN);
        printf("distance: %llu cm\n", (unsigned long long)cm);
        sleep_ms(500);
    }
}
