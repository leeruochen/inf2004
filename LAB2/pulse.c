/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * GPIO - OUTPUT: a bespoke pulse train on a real pin.
 *
 * Wiring: GP15 -> 330R -> LED anode, LED cathode -> GND.
 *
 * This drives an actual GPIO pin, not the on-board LED. On the Pico W the
 * on-board LED hangs off the CYW43 WiFi chip, so cyw43_arch_gpio_put() is an
 * SPI transaction rather than a register write: it costs microseconds, its
 * timing is not GPIO timing, and you cannot get a scope probe onto it.
 * Put a probe on GP15 and you can watch this waveform directly.
 */

#include <stdio.h>
#include "pico/stdlib.h"

const uint PULSE_PIN = 15;

int main() {
    stdio_init_all();

    gpio_init(PULSE_PIN);
    gpio_set_dir(PULSE_PIN, GPIO_OUT);

    while (true) {
        // Drive the pin high for 1 second
        gpio_put(PULSE_PIN, 1);
        sleep_ms(1000);  // 1 second pulse

        // Drive the pin low for 2 seconds
        gpio_put(PULSE_PIN, 0);
        sleep_ms(2000);  // 2 seconds gap
    }
}
