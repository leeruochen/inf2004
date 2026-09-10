/* Corrected wheel-encoder reference. See README.md for the eight fixes. */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#define ENCODER_PIN 2
#define TARGET_SLOTS 20
#define SLOTS_PER_REV 20
/* Example for slow manual rotation; shorten for narrower pulses at speed. */
#define DEBOUNCE_US 1000u

static volatile uint32_t pulse_count;
static volatile uint64_t last_edge_us;
static volatile uint32_t slot_width_us, slot_period_us;
static uint32_t last_debounce_us, slot_start_us, previous_start_us;
static bool in_slot, have_previous_start;

static void encoder_isr(uint gpio, uint32_t events)
{
    if (gpio != ENCODER_PIN || pulse_count >= TARGET_SLOTS) return;
    uint32_t now = time_us_32();
    if ((uint32_t)(now - last_debounce_us) <= DEBOUNCE_US) return;
    /* Both bits mean edges accumulated before service. Restart rather than
     * claiming a pulse width that we could not actually measure. */
    if ((events & GPIO_IRQ_EDGE_FALL) && (events & GPIO_IRQ_EDGE_RISE)) {
        in_slot = false;
        have_previous_start = false;
        return;
    }
    last_debounce_us = now;
    last_edge_us = time_us_64();
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (have_previous_start) slot_period_us = now - previous_start_us;
        previous_start_us = now;
        have_previous_start = true;
        slot_start_us = now;
        in_slot = true;
    } else if ((events & GPIO_IRQ_EDGE_RISE) && in_slot) {
        slot_width_us = now - slot_start_us;
        pulse_count++;
        in_slot = false;
    }
}

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);
    gpio_init(ENCODER_PIN);
    gpio_set_dir(ENCODER_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_PIN);
    gpio_set_irq_enabled_with_callback(ENCODER_PIN,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, encoder_isr);
    printf("Turn the wheel: waiting for %d complete low pulses\n", TARGET_SLOTS);
    while (pulse_count < TARGET_SLOTS) tight_loop_contents();

    uint32_t saved = save_and_disable_interrupts();
    uint32_t count = pulse_count, width = slot_width_us, period = slot_period_us;
    uint64_t finished = last_edge_us;
    restore_interrupts(saved);

    unsigned long rpm = period ? (unsigned long)(60000000ull /
        ((uint64_t)period * SLOTS_PER_REV)) : 0;
    printf("slots=%lu width=%lu us period=%lu us speed=%lu rpm last edge=%llu us\n",
        (unsigned long)count, (unsigned long)width, (unsigned long)period,
        rpm, (unsigned long long)finished);
    while (true) tight_loop_contents();
}
