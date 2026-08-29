/*
 * LAB 6 EXERCISE - laptop harness: correctness first, then timing.
 * INF2004 LAB 6
 *
 *   gcc -O2 -Wall -Wextra -o optimise_host optimise_host.c optimise.c
 *   ./optimise_host
 *
 * CORRECTNESS is checked before speed, and it is checked element by
 * element. A _fast() version that is quick and wrong scores nothing.
 *
 * The timings printed here are from YOUR LAPTOP, which has a
 * floating-point unit and a hardware divider. The Pico has neither.
 * Use these numbers to check that you have not made something slower;
 * use the PICO numbers in your report.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "optimise.h"

#define N_RAMP       256
#define N_HIGH        64
#define N_SCATTERED 1024
#define N_SMALL      512
#define N_MAX       1024

static uint16_t ramp[N_RAMP], high[N_HIGH], scattered[N_SCATTERED], small[N_SMALL];
static uint16_t zeros[N_SMALL];
static uint8_t  msg[512];

static int failures = 0;

static void build_vectors(void)
{
    for (size_t i = 0; i < N_RAMP;      i++) ramp[i]      = (uint16_t)(i * 13 + 7);
    for (size_t i = 0; i < N_HIGH;      i++) high[i]      = (uint16_t)(4095 - i);
    for (size_t i = 0; i < N_SCATTERED; i++) scattered[i] = (uint16_t)((i * 2654435761u) % 4096u);
    for (size_t i = 0; i < N_SMALL;     i++) small[i]     = (uint16_t)(i % 7);
    memset(zeros, 0, sizeof zeros);
    for (size_t i = 0; i < sizeof msg; i++) msg[i] = (uint8_t)(i * 31 + 5);
}

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

static void report(const char *what, int bad)
{
    if (bad) failures++;
    printf("  %-42s %s", what, bad ? "FAIL" : "ok");
    if (bad) printf("  (%d element%s differ)", bad, bad == 1 ? "" : "s");
    printf("\n");
}

/* ------------------------------------------------------------------ */
static int check_buffer_fn(void (*slow)(const uint16_t *, uint16_t *, size_t),
                           void (*fast)(const uint16_t *, uint16_t *, size_t),
                           const uint16_t *in, size_t n)
{
    static uint16_t a[N_MAX], b[N_MAX];
    int bad = 0;
    slow(in, a, n);
    fast(in, b, n);
    for (size_t i = 0; i < n; i++)
        if (a[i] != b[i]) bad++;
    return bad;
}

static void check_correctness(void)
{
    printf("correctness - mv_convert\n");
    report("ramp",      check_buffer_fn(mv_convert_slow, mv_convert_fast, ramp,      N_RAMP));
    report("high",      check_buffer_fn(mv_convert_slow, mv_convert_fast, high,      N_HIGH));
    report("scattered", check_buffer_fn(mv_convert_slow, mv_convert_fast, scattered, N_SCATTERED));
    report("small",     check_buffer_fn(mv_convert_slow, mv_convert_fast, small,     N_SMALL));

    printf("correctness - count_in_band\n");
    {
        const uint16_t cals[] = { 100, 1000, 2048, 4000, 4095 };
        for (size_t k = 0; k < sizeof cals / sizeof cals[0]; k++) {
            char label[48];
            uint32_t a = count_in_band_slow(scattered, N_SCATTERED, cals[k]);
            uint32_t b = count_in_band_fast(scattered, N_SCATTERED, cals[k]);
            snprintf(label, sizeof label, "cal = %u  (slow %u, fast %u)", cals[k], a, b);
            report(label, a != b);
        }
    }

    printf("correctness - crc8\n");
    {
        const uint8_t ref[] = "123456789";
        int bad = (crc8_fast(ref, 9) != 0xF4u);
        report("golden vector crc8(\"123456789\") == 0xF4", bad);
        report("agrees with crc8_slow over 512 bytes",
               crc8_slow(msg, sizeof msg) != crc8_fast(msg, sizeof msg));
        for (size_t len = 0; len <= 32; len++)
            if (crc8_slow(msg, len) != crc8_fast(msg, len)) { report("short lengths 0..32", 1); break; }
    }

    printf("correctness - normalise\n");
    report("ramp",      check_buffer_fn(normalise_slow, normalise_fast, ramp,      N_RAMP));
    report("high",      check_buffer_fn(normalise_slow, normalise_fast, high,      N_HIGH));
    report("scattered", check_buffer_fn(normalise_slow, normalise_fast, scattered, N_SCATTERED));
    report("small",     check_buffer_fn(normalise_slow, normalise_fast, small,     N_SMALL));
    report("all zeros (watch the divide)",
                        check_buffer_fn(normalise_slow, normalise_fast, zeros,     N_SMALL));
}

/* ------------------------------------------------------------------ */
static uint16_t sink16;
static uint32_t sink32;

static void time_one(const char *name, double t_slow, double t_fast)
{
    printf("  %-16s slow %9.1f us   fast %9.1f us   ", name, t_slow, t_fast);
    if (t_fast <= 0.0 || t_slow <= 0.0) { printf("-\n"); return; }
    double ratio = t_slow / t_fast;
    if (ratio >= 1.05)      printf("%.2fx faster\n", ratio);
    else if (ratio <= 0.95) printf("%.2fx SLOWER\n", 1.0 / ratio);
    else                    printf("no change\n");
}

#define REPS 2000

static void measure(void)
{
    static uint16_t out[N_MAX];
    double t0;

    printf("\ntiming on this laptop (%d repetitions each)\n", REPS);

    t0 = now_us();
    for (int r = 0; r < REPS; r++) { mv_convert_slow(scattered, out, N_SCATTERED); sink16 += out[0]; }
    double a1 = now_us() - t0;
    t0 = now_us();
    for (int r = 0; r < REPS; r++) { mv_convert_fast(scattered, out, N_SCATTERED); sink16 += out[0]; }
    time_one("mv_convert", a1, now_us() - t0);

    t0 = now_us();
    for (int r = 0; r < REPS; r++) sink32 += count_in_band_slow(scattered, N_SCATTERED, 2048);
    double a2 = now_us() - t0;
    t0 = now_us();
    for (int r = 0; r < REPS; r++) sink32 += count_in_band_fast(scattered, N_SCATTERED, 2048);
    time_one("count_in_band", a2, now_us() - t0);

    t0 = now_us();
    for (int r = 0; r < REPS; r++) sink32 += crc8_slow(msg, sizeof msg);
    double a3 = now_us() - t0;
    t0 = now_us();
    for (int r = 0; r < REPS; r++) sink32 += crc8_fast(msg, sizeof msg);
    time_one("crc8", a3, now_us() - t0);

    t0 = now_us();
    for (int r = 0; r < REPS; r++) { normalise_slow(scattered, out, N_SCATTERED); sink16 += out[0]; }
    double a4 = now_us() - t0;
    t0 = now_us();
    for (int r = 0; r < REPS; r++) { normalise_fast(scattered, out, N_SCATTERED); sink16 += out[0]; }
    time_one("normalise", a4, now_us() - t0);

    /* sink is printed so the optimiser cannot delete the work above.
       That is not a formality - see the note on dead-code elimination
       in the lab README. */
    printf("\n(checksum %u/%u - printed so the compiler cannot delete the benchmark)\n",
           (unsigned)sink16, (unsigned)sink32);
}

int main(void)
{
    build_vectors();

    printf("LAB 6 EXERCISE - optimisation\n\n");
    check_correctness();

    if (failures) {
        printf("\n%d correctness failure%s. Fix those before you look at any timing.\n",
               failures, failures == 1 ? "" : "s");
        return 1;
    }

    printf("\nall correct.\n");
    measure();
    printf("\nNow measure on the Pico. These laptop numbers are not your answer.\n");
    return 0;
}
