/*
 * BUG HUNT #6 - Only the disassembly tells the truth
 * INF2004 LAB 6
 *
 * A telemetry packet layer: CRC-8 integrity, an LFSR for test-pattern
 * generation, and a parser for incoming frames.
 *
 * SPECIFICATION
 * -------------
 *   crc8(data, len)
 *       CRC-8/ATM, polynomial 0x07, init 0x00, no reflection, no final XOR.
 *       Reference vector: crc8("123456789", 9) == 0xF4
 *
 *   lfsr_next()
 *       16-bit Fibonacci LFSR, taps at bits 16,14,13,11 (mask 0xB400).
 *       Seeded with 0xACE1. Its period must be 65535 - it must visit every
 *       non-zero 16-bit value exactly once before repeating, and must never
 *       reach zero.
 *
 *   parse_frame(frame, len, out)
 *       Frame layout:  [0xA5][len][payload...][crc8 over payload]
 *       Payload:       [0..3] timestamp, big-endian
 *                      [4..5] value, big-endian
 *
 *   format_reading(v)
 *       Returns a printable string for a reading.
 *
 * TWELVE defects are planted. Then, when you are done, there is pid.c.
 *
 * There are no hints in this file and none in the brief.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef PICO_ON_DEVICE
#include "pico/stdlib.h"
#endif

#define FRAME_SOF   0xA5u
#define POLY        0x07u
#define LFSR_SEED   0xACE1u
// bug 3: LFSR_TAPS is defined as 0xB000, but the correct mask for taps at bits 16,14,13,11 0x0400 + 0x2000 + 0x4000 + 0x8000 = 0xB400 in the SPECIFICATION at the top of this file
// fix: change the definition of LFSR_TAPS to 0xB400
#define LFSR_TAPS   0xB400u

/* ------------------------------------------------------------------ */
static uint8_t  crc_table[256];
static uint16_t lfsr_state = LFSR_SEED;
static bool     crc_ready  = false;

// bug 1: crc_table is declared with 255 elemnts, but crc_init loops i from 0 to 255
// fix: change the declaration of crc_table to have 256 elements
void crc_init(void)
{
    for (int i = 0; i < 256; i++) {
        uint8_t c = (uint8_t)i;
        for (int b = 0; b < 8; b++)
            c = (c & 0x80u) ? (uint8_t)((c << 1) ^ POLY) : (uint8_t)(c << 1);
        crc_table[i] = c;
    }
    crc_ready = true;
}

uint8_t crc8(const uint8_t *data, uint8_t len)
{
    // bug 12: crc_ready is defined and set to true in crc_init, but crc8 does not check if its ready
    // compiler flags crc_ready as unused, any caller that calls crc8 without calling crc_init gets wrong results
    // fix: add a check for crc_ready and call crc_init if it is false
    if (!crc_ready)
        crc_init();

    // bug 2: crc needs to be initialized before use
    // fix: initialize crc to 0 before the loop
    // uint8_t crc;
    uint8_t crc = 0x00u;

    for (uint8_t i = 0; i < len; i++)
        crc = crc_table[crc ^ data[i]];

    return crc;
}

/* ------------------------------------------------------------------ */
uint16_t lfsr_next(void)
{
    uint16_t lsb = lfsr_state & 1u;
    lfsr_state >>= 1;
    if (lsb)
        lfsr_state ^= LFSR_TAPS;
    return lfsr_state;
}

uint32_t lfsr_period(void)
{
    uint16_t start = lfsr_state;
    uint32_t n = 0;

    do {
        lfsr_next();
        n++;
    } while (lfsr_state != start && n < 70000u);
    // bug 4: the loop stops as soon as n reaches 200, making it impossible to reach the full period of 65535
    // fix: change the loop condition to n < 70000u, originally it was n < 200u

    return n;
}

/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t timestamp;
    uint16_t value;
} reading_t;

uint32_t parse_ts(const uint8_t *frame)
{
    // bug 5: 3 errors at once, frame is a uint8 array. &frame[2] is guaranteed to be at an offset of +2 bytes
    // meaning it is halfword (2-byte) aligned, not word-aligned(4-byte).
    // ARM in rp2040 does not support unaligned access.
    // error 2: wrong endianness, the timestamp in frame[2..5] is big-endian
    // rp2040 is little-endian so a raw uint32 reads 0x44332211 instead of 0x11223344
    // error 3: the *frame taken in is a const, casting frame to uint32_t * discards const and violates C's strict aliasing rules.
    // return *(uint32_t *)&frame[2];
    //fix: assemble 32-bit int byte by byte in big endian
    return ((uint32_t)frame[2] << 24) |
           ((uint32_t)frame[3] << 16) |
           ((uint32_t)frame[4] << 8)  |
           ((uint32_t)frame[5]);
}

bool parse_frame(const uint8_t *frame, uint8_t frame_len, reading_t *out)
{
    if (frame_len < 3)         return false;
    if (frame[0] != FRAME_SOF) return false;

    // bug 6: parse frame checks frame_len < 3, reads frame[1] then immediately uses frame[1] as len to read frame[2 + len] without checking that frame_len >= 3 + len
    // fix: check that frame_len >= 3 + len before reading frame[2 + len]
    uint8_t len = frame[1];
    if (len < 6 || (uint16_t)len + 3u > frame_len)
        return false;
    // fix: this check ensures that the frame has enough bytes to contain the payload and the CRC byte.

    if (crc8(&frame[2], len) != frame[2 + len])
        return false;

    out->timestamp = parse_ts(frame);
    out->value     = (uint16_t)((frame[6] << 8) | frame[7]);

    return true;
}

/* ------------------------------------------------------------------ */
char *format_reading(const reading_t *r)
{
    // bug 7: buf is a local array allocated on the stack frame of format_reading.
    // returning buf returns a dangling pointer to stack memory that becomes invalid after the function returns.
    // fix: declare buffer as static so lifetime extends across program execution
    static char buf[32];
    snprintf(buf, sizeof buf, "t=%lu v=%u",
             (unsigned long)r->timestamp, r->value);
    return buf;
}

char *label_for(const char *name)
{
    // bug 8: strlen returns the number of chars excluding null terminator whereas strcpy copies the char plus the null terminator.
    // every time label_for is called, malloc allocates a buffer that is one byte too small
    // fix: add 1 to the length passed to malloc to account for the null terminator 
    char *out = malloc(strlen(name) + 1);
    strcpy(out, name);
    return out;
}

/* ------------------------------------------------------------------
 * A calibration delay the sensor datasheet requires: at least 500 us
 * between powering the part and reading it.
 * ------------------------------------------------------------------ */
void calibration_delay(void)
{
    // bug 9: loop increments a non-volatile var, i, from 0 to 6000, with no side effects.
    // with -o0, compiler optimizes and emits the loop instructions
    // with -o2 and -o3, compiler deletes the loop
    // fix: make the loop counter volatile
    for (volatile uint32_t i = 0; i < 6000; i++)
        ;
}

/* ------------------------------------------------------------------
 * Wait for the acquisition ISR to signal that a conversion finished.
 * ------------------------------------------------------------------ */
// bug 10: conversion_done is modified by ISR(acquisition_isr) and polled in a spin-loop by main thread(wait_for_conversion) but it is not declared volatile, so the compiler may optimize away the loop in wait_for_conversion
// fix: declare conversion_done as volatile so the compiler knows it can change at any time
static volatile bool conversion_done = false;

void acquisition_isr(void)
{
    conversion_done = true;
}

void wait_for_conversion(void)
{
    while (!conversion_done)
        ;
    conversion_done = false;
}

/* ------------------------------------------------------------------
 * Sum every byte of a buffer. Used for a quick integrity check.
 * ------------------------------------------------------------------ */
uint32_t checksum_all(const uint8_t *data, uint16_t len)
{
    uint32_t sum = 0;
    // bug 11: parameter len is a uint16, but i is a uint8
    // because i is uint8, if the loop reaches 255, it wraps around to 0, causeing a infinite loop
    // fix: change the type of i to uint16_t
    for (uint16_t i = 0; i < len; i++)
        sum += data[i];
    return sum;
}
