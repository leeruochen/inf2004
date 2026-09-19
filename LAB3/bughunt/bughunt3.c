/*
 * BUG HUNT #3 - Works in Debug, hangs in Release
 * INF2004 LAB 3
 *
 * An IR wheel-encoder driver. It should:
 *   - count one pulse per slot of the encoder wheel
 *   - measure the width of each slot in microseconds
 *   - debounce the optical noise on each edge
 *      what does debounce mean? electrical signals are messy, signals may fluctuate up and down before settling
 *      this is called noise, if a microcontroller is too fast, it will read those fluctuations as separate slots
 *      debouncing is a software trick to ignore this noise, designed to register the first change then intentionally ignore any further changes for a short time (DEBOUNCE_US which is 50000u which is 50 milliseconds)
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
#define DEBOUNCE_US   500u // bug 4: changed from 50000u to 500u, this is more appropriate for a wheel encoder, 0.5 milliseconds is a good debounce time for a wheel encoder
#define SLOTS_PER_REV 20

/* ---- state shared between the ISR and main ---- */
// task A/bug 1: we have to add volatile keyword to these variables
// this shows us the blind spot that C compilers have when dealing with interrupts
// when built in release mode, the compiler analyzes the code to make it run as fast as possible
// looking at the while(pulse_count < TARGET_SLOTS) loop, the compiler concludes this loop to be "if pulse_count is less than 20, we enter this loop and nothing will change, it will be less than 20 forever"
// to save time, it optimizes the code and removes the comparison step, replacing the loop into an infinite loop and stops looking at the pulse_count var
// the compiler does not know the physical wheel encoder is triggering the isr function and incrementing pulse_count
// volatile fixes this by giving the compiler a strict command that this value can change any time and need to fetch it from memory every time it is used.
static volatile uint32_t pulse_count      = 0;   /* slots counted so far        */
static volatile uint64_t last_edge_us     = 0;   /* wall clock of the last edge */
static volatile uint32_t last_debounce_us = 0;   /* 32-bit timer, for debounce  */
static volatile uint32_t slot_start_us    = 0;
static volatile uint32_t slot_width_us    = 0;

typedef enum {
    ENC_IDLE,
    ENC_SLOT,
} enc_state_t;

// same as Task A/bug 1, we have to add volatile keyword to this variable
static volatile enc_state_t state = ENC_IDLE; // initial state is idle, waiting for the first edge to be detected

/* ------------------------------------------------------------------
 * Interrupt service routine - runs on every configured edge on GP2.
 * ------------------------------------------------------------------ */
void encoder_isr(uint gpio, uint32_t events)
{
    uint32_t now = time_us_32(); // get the current time in microseconds since boot

    // bug 3: initially if (now > last_debounce_us + DEBOUNCE_US)
    // we have to calculate time differences using subtraction to avoid overflow issues.
    // last_debounce_us is a 32-bit unsigned int, the maximum limit is 2^32 - 1 = 4294967295, which is about 71 minutes
    // if last_debounce_us is close to the max and we add DEBOUNCE_US, it might overflow and make last_debounce_us small.
    // this is debouncing, which is a trick to ignore noise after the first change is detected
    // DEBOUNCE_US is our threshold, if the time difference is greater than this, we will consider it a valid edge and process it.
    // how it works: initially, now is 0, last_debounce_us is 0, DEBOUNCE_US is 50000, the if statement only runs if now becomes greater than 50000. this means the interrupt will only be processed if the threshold has been at least 50000u.
    // bug 4: DEBOUNCE_US being 50000u is also the bug, it is too long for a wheel encoder, it is more appropriate for a button press, we will change it to 500u which is 0.5 milliseconds, this is more appropriate for a wheel encoder.
    if (now - last_debounce_us > DEBOUNCE_US) {

        last_debounce_us = now;
        last_edge_us     = time_us_64();

        switch (state) { // initial is idle
            case ENC_IDLE:
                slot_start_us = now;
                state = ENC_SLOT;
                // bug 5: no break statement, causing code to fall to next case
                break;

            case ENC_SLOT:
                slot_width_us = now - slot_start_us;
                pulse_count++;
                state = ENC_IDLE;
                break;
        }

        // bug 6: printf is commented out. initally was not commented out making the ISR slow.
        // ISRs should run fast, printf is slow and can cause issues if called from ISR.
        // printf statements would unintentionally increase the debounce time.
        // printf("  edge: count=%lu width=%lu us\n",
        //        (unsigned long)pulse_count, (unsigned long)slot_width_us);
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
    stdio_init_all(); // initialize all standard IO
    sleep_ms(3000);

    gpio_init(ENCODER_PIN); // initialize the GPIO pin for the encoder ENCODER_PIN = 2
    gpio_set_dir(ENCODER_PIN, GPIO_IN); // set the direction of the GPIO pin to input
    gpio_pull_up(ENCODER_PIN); // enable the internal pull-up resistor on the GPIO pin

    // this line sets the interrupt to true, meaning the wheel encoders sensor will trigger the interrupt if it detects something, this time a wheel slot passing by the sensor.
    // bug 7: intially the interrupt was set to fire on GPIO_IRQ_EDGE_FALL only, meaning it would only trigger on the falling edge of the signal, but we want to count both edges (rising and falling) to get a more accurate count of the slots.
    // add GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE means the interrupt will trigger on both falling and rising edges of the signal.
    gpio_set_irq_enabled_with_callback(ENCODER_PIN,
                                       GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE,
                                       true,
                                       &encoder_isr); // enable the interrupt on the falling edge of the GPIO pin and set the callback function to encoder_isr

    printf("BUG HUNT #3 - turn the wheel, waiting for %d slots\n", TARGET_SLOTS); // wait for 20 slots to be counted

    while (pulse_count < TARGET_SLOTS) // wait until the pulse_count reaches TARGET_SLOTS
        tight_loop_contents(); // from pico/stdlib.h, this is a hint to the compiler that we are in a tight loop and it can optimize the code accordingly

    // bug 2: add new interrupt call to disable the interrupt after slots have counted
    // if this line is not set, the interrupt will keep triggering as the physical wheel is still spinning, and the program will continue incrementing pulse_count.
    // bug 7: similar to above.
    gpio_set_irq_enabled_with_callback(ENCODER_PIN, GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE, false, &encoder_isr);

    uint64_t finished_at = last_edge_us;

    // once 20 slots have been counted, print the results
    printf("\ndone.\n");
    printf("  slots counted : %lu\n",     (unsigned long)pulse_count);
    printf("  last width    : %lu us\n",  (unsigned long)slot_width_us);
    printf("  speed         : %lu rpm\n", (unsigned long)rpm_from_width(slot_width_us));
    printf("  last edge at  : %llu us since boot\n", (unsigned long long)finished_at);

    while (true)
        tight_loop_contents();
}
