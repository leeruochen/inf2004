/*
 * BUG HUNT #2 - laptop harness.
 * INF2004 LAB 2
 *
 *   gcc -Wall -Wextra -o bughunt2_host bughunt2_host.c frame.c && ./bughunt2_host
 *
 * This tests the codec against the golden frames from the specification at the
 * top of frame.h. It finds MOST of the planted defects without any hardware.
 * It cannot find all of them - two only appear on a real UART link.
 */

#include <stdio.h>
#include <string.h>
#include "frame.h"

static int failures = 0;

static void fail(const char *what) { printf("      FAIL: %s\n", what); failures++; }

typedef struct {
    const char *name;
    reading_t   r;
    uint8_t     golden[12];
} vector_t;

/* Hand-computed from the specification in frame.h. These are correct. */
static const vector_t vectors[] = {
    { "A: normal",
      { 0x1234, 0x01,  253, 0x0A0B0C0D },
      { 0xAA, 0x09, 0x12, 0x34, 0x01, 0x00, 0xFD, 0x0A, 0x0B, 0x0C, 0x0D, 0x72 } },

    { "B: high timestamp, negative temp",
      { 0x0007, 0x00,  -55, 0x80112233 },
      { 0xAA, 0x09, 0x00, 0x07, 0x00, 0xFF, 0xC9, 0x80, 0x11, 0x22, 0x33, 0xB5 } },

    { "C: extremes",
      { 0xFFFF, 0xFF, -32768, 0x00000000 },
      { 0xAA, 0x09, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D } },
};

static void compare_readings(const reading_t *got, const reading_t *want)
{
    if (got->sensor_id    != want->sensor_id)
        printf("      sensor_id     got 0x%04X  want 0x%04X\n", got->sensor_id, want->sensor_id), failures++;
    if (got->status       != want->status)
        printf("      status        got 0x%02X    want 0x%02X\n", got->status, want->status), failures++;
    if (got->temp_c_x10   != want->temp_c_x10)
        printf("      temp_c_x10    got %-8d want %d\n", got->temp_c_x10, want->temp_c_x10), failures++;
    if (got->timestamp_ms != want->timestamp_ms)
        printf("      timestamp_ms  got 0x%08X want 0x%08X\n", got->timestamp_ms, want->timestamp_ms), failures++;
}

int main(void)
{
    printf("BUG HUNT #2 - frame codec\n");
    printf("sizeof(reading_t) = %zu bytes\n\n", sizeof(reading_t));

    for (unsigned v = 0; v < sizeof vectors / sizeof vectors[0]; v++) {
        const vector_t *t = &vectors[v];
        uint8_t buf[64];
        reading_t out;

        printf("--- vector %s ---\n", t->name);

        /* 1. ENCODE: do we produce the bytes the specification demands? */
        memset(buf, 0, sizeof buf);
        uint8_t n = frame_encode(&t->r, buf);

        hexdump("expected", t->golden, 12);
        hexdump("encoded",  buf, n);

        if (n != 12)
            fail("encoder produced the wrong number of bytes");
        else if (memcmp(buf, t->golden, 12) != 0)
            fail("encoded bytes do not match the specification");

        /* 2. DECODE: given known-good bytes, do we recover the reading? */
        memset(&out, 0, sizeof out);
        if (!frame_decode(t->golden, 12, &out))
            fail("decoder rejected a valid frame");
        else
            compare_readings(&out, &t->r);

        /* 3. ROUND TRIP: encoder into decoder. */
        memset(&out, 0, sizeof out);
        if (frame_decode(buf, n, &out))
            compare_readings(&out, &t->r);
        else
            printf("      (round trip rejected - expected, until encode is fixed)\n");

        printf("\n");
    }

    printf("%s  (%d failure%s)\n",
           failures ? "HUNT INCOMPLETE" : "CODEC MATCHES THE SPECIFICATION",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
