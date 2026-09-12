/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * GPIO - INPUT: a button on GP20 gating an LED on GP15.
 *
 * Wiring: button on GP20 (pull-up, active low, on the Maker board)
 *         GP15 -> 330R -> LED anode, LED cathode -> GND
 *
 * The LED blinks while GP20 reads high (button NOT pressed). Pressing the
 * button pulls GP20 low and the blinking stops.
 */
#include <stdio.h>
#include "pico/stdlib.h"

const uint BTN_PIN = 20; // 20 because we want to use GP20 button
const uint LED_PIN = 15; // 15 because we want to use GP15 LED

int main() {
    stdio_init_all(); // initialize all basic IO functions, including UART and USB

    /* gpio_init() is not optional just because the code appears to work
     * without it. See the note in the README: without it you are relying on
     * the RP2040's reset defaults rather than stating what you want. */
    gpio_init(BTN_PIN); // this will work even with this line as RP2040 resets input-enable to 1. basically, dont rely on this reset defaults. initialize every pin you use every time as reset defaults are different for different boards, initialize GPIO20
    gpio_set_dir(BTN_PIN, GPIO_IN); // set GP20 as input
    gpio_set_pulls(BTN_PIN, true, false); // true activates pull-up, false disables pull-down. this is needed because the button is active low, so we need to pull it up to 3.3V when not pressed

    gpio_init(LED_PIN); // initialize GPIO15
    gpio_set_dir(LED_PIN, GPIO_OUT); // set GP15 as output

    while (true) {
        if(gpio_get(BTN_PIN)) // in this infinite loop, this reads the state of GP20. if it is high = 1 = true (button not pressed), it will blink the LED on GP15
        {
            // blink the LED on GP15
            gpio_put(LED_PIN, 1);
            sleep_ms(250);
            gpio_put(LED_PIN, 0);
            sleep_ms(250);
        }
    }
}