/*
 * LAB 6 EXERCISE - the slow versions are given. Write the fast ones.
 * INF2004 LAB 6
 *
 * The specification and the rules are at the top of optimise.h.
 *
 * DO NOT modify any function whose name ends in _slow, and do not modify
 * calib_low() or calib_high().
 */

#include "optimise.h"

/* ==================================================================
 * 1. ADC counts -> millivolts
 * ================================================================== */

/* GIVEN - do not modify. */
void mv_convert_slow(const uint16_t *raw, uint16_t *out, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        double mv = (double)raw[i] * 3300.0 / 4095.0;
        out[i] = (uint16_t)mv;
    }
}

void mv_convert_fast(const uint16_t *raw, uint16_t *out, size_t n)
{
    /* TODO. Same answers, no floating point. */
    mv_convert_slow(raw, out, n);
}

/* ==================================================================
 * 2. Count samples inside a calibrated band
 * ================================================================== */

/*
 * GIVEN - do not modify, and do not try to make these cheaper. They
 * stand in for a vendor library call whose source you do not have.
 * `noinline` is there to stop the compiler doing your job for you.
 */
#if defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

NOINLINE uint16_t calib_low(uint16_t cal)
{
    uint32_t acc = cal;
    for (int i = 0; i < 64; i++)
        acc = (acc * 1103515245u + 12345u) >> 1;
    return (uint16_t)(cal / 4 + (acc & 0x3u));
}

NOINLINE uint16_t calib_high(uint16_t cal)
{
    uint32_t acc = cal;
    for (int i = 0; i < 64; i++)
        acc = (acc * 1103515245u + 12345u) >> 1;
    return (uint16_t)(cal - cal / 4 + (acc & 0x3u));
}

/* GIVEN - do not modify. */
uint32_t count_in_band_slow(const uint16_t *raw, size_t n, uint16_t cal)
{
    uint32_t count = 0;

    for (size_t i = 0; i < n; i++) {
        if (raw[i] >= calib_low(cal) && raw[i] <= calib_high(cal))
            count++;
    }
    return count;
}

uint32_t count_in_band_fast(const uint16_t *raw, size_t n, uint16_t cal)
{
    /* TODO. Same answer, far fewer calls. */
    return count_in_band_slow(raw, n, cal);
}

/* ==================================================================
 * 3. CRC-8/ATM
 * ================================================================== */

#define CRC8_POLY 0x07u

/* GIVEN - do not modify. Eight iterations per byte. */
uint8_t crc8_slow(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00u;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ CRC8_POLY)
                                : (uint8_t)(crc << 1);
    }
    return crc;
}

uint8_t crc8_fast(const uint8_t *data, size_t len)
{
    /* TODO. One iteration per byte. Say what it costs. */
    return crc8_slow(data, len);
}

/* ==================================================================
 * 4. Normalise to permille
 * ================================================================== */

/* GIVEN - do not modify. */
void normalise_slow(const uint16_t *raw, uint16_t *out, size_t n)
{
    uint32_t total = 0;

    for (size_t i = 0; i < n; i++)
        total += raw[i];

    if (total == 0) {
        for (size_t i = 0; i < n; i++)
            out[i] = 0;
        return;
    }

    for (size_t i = 0; i < n; i++)
        out[i] = (uint16_t)(((uint32_t)raw[i] * 1000u) / total);
}

void normalise_fast(const uint16_t *raw, uint16_t *out, size_t n)
{
    /* TODO. Same answers - the harness checks every element. */
    normalise_slow(raw, out, n);
}
