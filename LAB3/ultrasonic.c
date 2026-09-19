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

const uint TRIG_PIN = 1;
const uint ECHO_PIN = 0;

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
    absolute_time_t start_time, end_time; // fix 1: missing semicolon

    gpio_put(trig_pin, 1);
    sleep_us(10);
    gpio_put(trig_pin, 0); // fix 2: TRIG high must be held for 10 us then brought back to low, this was missing

    /* Block until ECHO goes high. See the README: this is the design flaw. */
    while (gpio_get(echo_pin) == 0)
        tight_loop_contents();
    start_time = get_absolute_time();

    /* Block until ECHO goes low again. */
    while (gpio_get(echo_pin) == 1)
        tight_loop_contents();
    end_time = get_absolute_time();

    return (uint64_t)absolute_time_diff_us(start_time, end_time); // this returns in microseconds
}

// 1. How long does the processor spend inside those loops for an object 2 m away?
// An object 2 meters away means the sound must travel 200 cm out and 200 cm back, for a total distance of 400 cm.
// This means the sound has to travel 400 cm, which is 400 * 29.1 = 11,640 microseconds (or about 11.6 milliseconds).
// For those 11.6 milliseconds, the CPU is completely paralyzed inside the while loop, executing tight_loop_contents() over and over again.   

// 2. What happens if the echo never arrives at all?
// If the sensor is unplugged or the object is too far away to reflect sound, the ECHO pin will either never go high, or go high and never drop low.
// Because the while loops lack a timeout mechanism, the program hangs forever. The processor gets permanently stuck on lines 31 or 35. It will never return to the main loop, meaning your device has effectively crashed and requires a hard reset.   

// 3. Can your stopwatch keep counting? What would you change?Can it? No. Because the CPU is entirely occupied by spinning in those while loops waiting for the echo, it has no processing time left to update a stopwatch display, check other sensors, or run a motor.
// What to change: You must completely remove the while loops and replace them with hardware interrupts (just like you did for the wheel encoder). You would configure an interrupt to trigger on the rising edge of the ECHO pin (to start a timer) and another interrupt on the falling edge (to calculate the elapsed time). This allows the CPU to freely run your stopwatch code in the main() loop while the hardware handles the sensor timing invisibly in the background.


/*
 * Convert that pulse width to centimetres.
 *
 * Sound travels at roughly 343 m/s, which is 29.1 microseconds per
 * centimetre.
 */
uint64_t get_distance_cm(uint trig_pin, uint echo_pin)
{
    uint64_t pulse_us = get_pulse_us(trig_pin, echo_pin); // this returns in microseconds

    return pulse_us / 29 / 2; // initially pulse_us / 29, the formulae states that distance = (time * speed of sound) / 2.
    // pulse_us is in microseconds, speed of sound is 343 m/s = 34300 cm/s. 1000000 microseconds per second, so 34300/1000000 = 0.0343 cm per microsecond. 
    // time * (343/10000) = time / (10000/343) = time / 29.1, where they approx to time / 29. we add /2 according to formula
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


// What This Component Is Meant For
// The HC-SR04P is an ultrasonic distance ranger. It acts exactly like sonar on a submarine or echolocation used by bats.   
//The Physics: One cylinder acts as a speaker that fires a short, 40 kHz burst of high-frequency sound (well above human hearing) when the TRIG pin is pulsed. 
//The second cylinder acts as a microphone. When the sound wave bounces off an object and hits the microphone, it drives the ECHO pin high.   
//The Software: By measuring the exact microseconds between the "shout" and the "echo," the software calculates distance.   
//Real-World Applications: These sensors are ubiquitous in robotics and automation. They are the exact technology used in car reversing sensors (the system that beeps faster as you back into a wall), robot vacuum cleaners for obstacle avoidance, and industrial fluid tanks to measure water levels without touching the liquid.