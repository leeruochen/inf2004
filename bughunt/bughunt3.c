/*
 * BUG HUNT #3 - Works in Debug, hangs in Release
 * INF2004 LAB 3
 *
 * An IR wheel-encoder driver. It should:
 *   - count one pulse per slot of the encoder wheel
 *   - measure the width of each slot in microseconds
 *   - debounce the optical noise on each edge
 *   - stop and report once TARGET_SLOTS slots have gone past
 *
 * Wiring: HC020K OUT -> GP2,  VCC -> 3V3,  GND -> GND
 *
 * Eight defects are planted. This file COMPILES (with warnings - read them).
 * Building it is not the exercise; making it behave is.
 *
 * You must build this at BOTH -O0 (Debug) and -O3 (Release) before you
 * change anything.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define ENCODER_PIN   2
#define TARGET_SLOTS  20
#define DEBOUNCE_US   50000u
#define SLOTS_PER_REV 20

/* ---- state shared between the ISR and main ---- */
static uint32_t pulse_count      = 0;   /* slots counted so far        */
static uint64_t last_edge_us     = 0;   /* wall clock of the last edge */
static uint32_t last_debounce_us = 0;   /* 32-bit timer, for debounce  */
static uint32_t slot_start_us    = 0;
static uint32_t slot_width_us    = 0;

typedef enum {
    ENC_IDLE,
    ENC_SLOT,
} enc_state_t;

static enc_state_t state = ENC_IDLE;

/* ------------------------------------------------------------------
 * Interrupt service routine - runs on every configured edge on GP2.
 * ------------------------------------------------------------------ */
void encoder_isr(uint gpio, uint32_t events)
{
    uint32_t now = time_us_32();

    if (now > last_debounce_us + DEBOUNCE_US) {

        last_debounce_us = now;
        last_edge_us     = time_us_64();

        switch (state) {
            case ENC_IDLE:
                slot_start_us = now;
                state = ENC_SLOT;

            case ENC_SLOT:
                slot_width_us = now - slot_start_us;
                pulse_count++;
                state = ENC_IDLE;
                break;
        }

        printf("  edge: count=%lu width=%lu us\n",
               (unsigned long)pulse_count, (unsigned long)slot_width_us);
    }
}

/* ------------------------------------------------------------------
 * Speed in revolutions per minute, from the most recent slot width.
 * ------------------------------------------------------------------ */
static uint32_t rpm_from_width(uint32_t width_us)
{
    if (width_us == 0) return 0;
    return 60000000u / (width_us * SLOTS_PER_REV);
}

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);

    gpio_init(ENCODER_PIN);
    gpio_set_dir(ENCODER_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_PIN);

    gpio_set_irq_enabled_with_callback(ENCODER_PIN,
                                       GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &encoder_isr);

    printf("BUG HUNT #3 - turn the wheel, waiting for %d slots\n", TARGET_SLOTS);

    while (pulse_count < TARGET_SLOTS)
        tight_loop_contents();

    uint64_t finished_at = last_edge_us;

    printf("\ndone.\n");
    printf("  slots counted : %lu\n",     (unsigned long)pulse_count);
    printf("  last width    : %lu us\n",  (unsigned long)slot_width_us);
    printf("  speed         : %lu rpm\n", (unsigned long)rpm_from_width(slot_width_us));
    printf("  last edge at  : %u us since boot\n", finished_at);

    while (true)
        tight_loop_contents();
}
