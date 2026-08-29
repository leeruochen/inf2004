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

const uint BTN_PIN = 20;
const uint LED_PIN = 15;

int main() {
    stdio_init_all();

    /* gpio_init() is not optional just because the code appears to work
     * without it. See the note in the README: without it you are relying on
     * the RP2040's reset defaults rather than stating what you want. */
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_set_pulls(BTN_PIN, true, false);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        if(gpio_get(BTN_PIN))
        {
            gpio_put(LED_PIN, 1);
            sleep_ms(250);
            gpio_put(LED_PIN, 0);
            sleep_ms(250);
        }
    }
}
