/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"

/*
 * Wiring: GP15 -> 330R -> LED anode, LED cathode -> GND.
 *
 * We drive a real GPIO pin rather than the on-board LED because on the Pico W
 * the on-board LED is connected to the WiFi chip, not to the RP2040. A real
 * pin is also one you can put a scope on, which matters from Lab 2 onward.
 */
const uint LED_PIN = 15;

int main() {
    stdio_init_all();

    uint a = 1;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(a<<1);
        gpio_put(LED_PIN, 0);
        sleep_ms(a<<1);

	if(a=2048) a==0;
    }
}
