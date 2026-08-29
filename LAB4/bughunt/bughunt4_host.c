/*
 * BUG HUNT #4 - laptop harness for the signal chain.
 * INF2004 LAB 4
 *
 *   gcc -Wall -Wextra -o bughunt4_host bughunt4_host.c algo.c && ./bughunt4_host
 *
 * Five of the eight planted defects are in algo.c and are visible here, with no
 * hardware, in under a second. The other three need the Pico.
 *
 * Note the STEP RESPONSE section. Feeding an algorithm a synthetic input whose
 * correct output you already know is the fastest debugging tool in this module.
 */

#include <stdio.h>
#include "algo.h"

static int failures = 0;

static void check(const char *what, long got, long expect)
{
    int ok = (got == expect);
    if (!ok) failures++;
    printf("  %-30s got %-12ld expect %-12ld %s\n",
           what, got, expect, ok ? "ok" : "FAIL");
}

int main(void)
{
    printf("BUG HUNT #4 - signal chain\n\n");

    printf("adc_to_mv  (0..4095 -> 0..3300 mV)\n");
    check("adc_to_mv(0)",    adc_to_mv(0),       0);
    check("adc_to_mv(1024)", adc_to_mv(1024),  825);
    check("adc_to_mv(2048)", adc_to_mv(2048), 1650);
    check("adc_to_mv(4095)", adc_to_mv(4095), 3300);

    printf("\nduty_from_adc  (0..4095 -> 0..999)\n");
    check("duty_from_adc(0,    999)", duty_from_adc(0,    999),   0);
    check("duty_from_adc(1024, 999)", duty_from_adc(1024, 999), 249);
    check("duty_from_adc(2048, 999)", duty_from_adc(2048, 999), 499);
    check("duty_from_adc(4095, 999)", duty_from_adc(4095, 999), 999);

    printf("\npwm_wrap_for_hz  (counter runs 0..wrap inclusive)\n");
    check("pwm_wrap_for_hz(20 Hz, 250.0)", pwm_wrap_for_hz(20, 250.0f), 24999);
    check("pwm_wrap_for_hz(50 Hz, 100.0)", pwm_wrap_for_hz(50, 100.0f), 24999);

    /* ----------------------------------------------------------------
     * STEP RESPONSE
     * A first-order filter with alpha = 1/16 fed a step must climb
     * smoothly toward the target and settle on it - from EITHER side.
     * ---------------------------------------------------------------- */
    printf("\nstep response - rising 0 -> 3000\n  ");
    iir_reset();
    for (int i = 0; i < 60; i++) {
        uint32_t v = iir_step(0, 3000);
        if (i % 6 == 0) printf("%6u ", v);
    }
    printf("\n  (should climb smoothly and settle near 3000)\n");

    printf("\nstep response - falling 3000 -> 500\n  ");
    for (int i = 0; i < 60; i++) {
        uint32_t v = iir_step(0, 500);
        if (i % 6 == 0) printf("%6u ", v);
    }
    printf("\n  (should fall smoothly and settle near 500)\n");

    uint32_t settled = iir_step(0, 500);
    if (settled > 600) {
        printf("  FAIL: filter did not converge downward (ended at %u)\n", settled);
        failures++;
    }

    /* ----------------------------------------------------------------
     * CHANNEL INDEPENDENCE
     * Two channels are filtered alternately. They must not affect
     * each other.
     * ---------------------------------------------------------------- */
    printf("\nchannel independence\n");
    iir_reset();
    uint32_t a = 0, b = 0;
    for (int i = 0; i < 200; i++) {
        a = iir_step(0, 1000);
        b = iir_step(1, 2000);
    }
    printf("  channel 0 fed a constant 1000, channel 1 fed a constant 2000\n");
    check("channel 0 settles at", a, 1000);
    check("channel 1 settles at", b, 2000);

    printf("\n%s  (%d failure%s)\n",
           failures ? "HUNT INCOMPLETE" : "SIGNAL CHAIN MATCHES THE SPECIFICATION",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
