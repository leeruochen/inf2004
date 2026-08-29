/*
 * LAB 6 EXERCISE - measurement on the target.
 * INF2004 LAB 6
 *
 * This file is complete. Do not change it - it is the instrument, and an
 * instrument you have adjusted to give the reading you wanted is not an
 * instrument.
 *
 * THESE are the numbers that go in your report. The laptop harness only
 * tells you whether your answers are right.
 *
 * Build it three times - at -O0, -O2 and -O3 - and report all three.
 * See CMakeLists.txt.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "optimise.h"

#define N_BUF  1024
#define REPS    200

static uint16_t raw[N_BUF];
static uint16_t out[N_BUF];
static uint8_t  msg[512];

/* Volatile so the optimiser cannot decide the benchmark has no effect and
 * delete it. This is the same mechanism as dead-code elimination in the
 * lab README, and it is why the original timing example measured nothing. */
static volatile uint32_t sink;

static void build_data(void)
{
    for (size_t i = 0; i < N_BUF;      i++) raw[i] = (uint16_t)((i * 2654435761u) % 4096u);
    for (size_t i = 0; i < sizeof msg; i++) msg[i] = (uint8_t)(i * 31 + 5);
}

static void row(const char *name, int64_t slow_us, int64_t fast_us)
{
    printf("  %-16s %8lld us  %8lld us  ", name,
           (long long)slow_us, (long long)fast_us);
    if (fast_us > 0 && slow_us > 0)
        printf("%.2fx\n", (double)slow_us / (double)fast_us);
    else
        printf("   -\n");
}

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);

    build_data();

    printf("LAB 6 EXERCISE - optimisation, measured on the RP2040\n");
    printf("%d repetitions, buffer of %d samples\n\n", REPS, N_BUF);
    printf("  %-16s %11s %11s %s\n", "", "slow", "fast", "speedup");

    absolute_time_t t0;
    int64_t a, b;

    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) { mv_convert_slow(raw, out, N_BUF); sink += out[0]; }
    a = absolute_time_diff_us(t0, get_absolute_time());
    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) { mv_convert_fast(raw, out, N_BUF); sink += out[0]; }
    b = absolute_time_diff_us(t0, get_absolute_time());
    row("mv_convert", a, b);

    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) sink += count_in_band_slow(raw, N_BUF, 2048);
    a = absolute_time_diff_us(t0, get_absolute_time());
    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) sink += count_in_band_fast(raw, N_BUF, 2048);
    b = absolute_time_diff_us(t0, get_absolute_time());
    row("count_in_band", a, b);

    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) sink += crc8_slow(msg, sizeof msg);
    a = absolute_time_diff_us(t0, get_absolute_time());
    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) sink += crc8_fast(msg, sizeof msg);
    b = absolute_time_diff_us(t0, get_absolute_time());
    row("crc8", a, b);

    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) { normalise_slow(raw, out, N_BUF); sink += out[0]; }
    a = absolute_time_diff_us(t0, get_absolute_time());
    t0 = get_absolute_time();
    for (int r = 0; r < REPS; r++) { normalise_fast(raw, out, N_BUF); sink += out[0]; }
    b = absolute_time_diff_us(t0, get_absolute_time());
    row("normalise", a, b);

    printf("\nsink = %u  (printed so none of the above can be optimised away)\n",
           (unsigned)sink);
    printf("\nRun this at -O0, -O2 and -O3 and report all three.\n");

    while (true)
        tight_loop_contents();
}
