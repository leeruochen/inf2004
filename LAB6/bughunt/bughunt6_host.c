/*
 * BUG HUNT #6 - laptop harness.
 * INF2004 LAB 6
 *
 *   gcc -Wall -Wextra -O0 -o bughunt6_host bughunt6_host.c bughunt6.c
 *   ./bughunt6_host
 *
 * Then do it again at -O2. Then at -O3. The results are not the same.
 *
 * This harness deliberately does NOT tell you which defect a failure
 * corresponds to. Some defects it cannot see at all, because your laptop is
 * not a Cortex-M0+.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct { uint32_t timestamp; uint16_t value; } reading_t;

void      crc_init(void);
uint8_t   crc8(const uint8_t *data, uint8_t len);
uint16_t  lfsr_next(void);
uint32_t  lfsr_period(void);
bool      parse_frame(const uint8_t *frame, uint8_t frame_len, reading_t *out);
char     *format_reading(const reading_t *r);
char     *label_for(const char *name);
void      calibration_delay(void);
uint32_t  checksum_all(const uint8_t *data, uint16_t len);

static int failures = 0;
static void check(const char *what, long got, long expect)
{
    int ok = (got == expect);
    if (!ok) failures++;
    printf("  %-34s got %-12ld expect %-12ld %s\n",
           what, got, expect, ok ? "ok" : "FAIL");
}

int main(void)
{
    printf("BUG HUNT #6 - telemetry layer\n\n");

    crc_init();

    printf("crc8 - reference vector for CRC-8/ATM\n");
    check("crc8(\"123456789\", 9)", crc8((const uint8_t *)"123456789", 9), 0xF4);
    check("crc8(\"123456789\", 9) again", crc8((const uint8_t *)"123456789", 9), 0xF4);

    printf("\nlfsr - must visit every non-zero 16-bit value\n");
    check("lfsr_period()", lfsr_period(), 65535);

    printf("\nparse_frame\n");
    {
        /* [A5][len=6][ts BE = 0x11223344][value BE = 0x0708][crc] */
        uint8_t payload[6] = { 0x11, 0x22, 0x33, 0x44, 0x07, 0x08 };
        uint8_t frame[9];
        reading_t r = { 0, 0 };

        frame[0] = 0xA5;
        frame[1] = 6;
        memcpy(&frame[2], payload, 6);
        frame[8] = crc8(payload, 6);

        if (!parse_frame(frame, sizeof frame, &r)) {
            printf("  parse_frame rejected a valid frame\n");
            failures++;
        } else {
            check("timestamp", (long)r.timestamp, 0x11223344);
            check("value",     (long)r.value,     0x0708);
        }

        /* a hostile frame: the length byte lies */
        uint8_t evil[4] = { 0xA5, 200, 0x00, 0x00 };
        printf("  feeding a frame whose length byte says 200...\n");
        if (parse_frame(evil, sizeof evil, &r)) {
            printf("  accepted a frame that could not possibly be valid\n");
            failures++;
        }
    }

    printf("\nformat_reading\n");
    {
        reading_t r = { 42, 7 };
        char *s = format_reading(&r);
        printf("  returned: \"%s\"  (expect \"t=42 v=7\")\n", s ? s : "(null)");
        if (!s || strcmp(s, "t=42 v=7") != 0) failures++;
    }

    printf("\nlabel_for\n");
    {
        char *s = label_for("temperature");
        printf("  returned: \"%s\"  (expect \"temperature\")\n", s ? s : "(null)");
        if (!s || strcmp(s, "temperature") != 0) failures++;
    }

    printf("\ncalibration_delay - datasheet requires at least 500 us\n");
    {
        struct timespec a, b;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < 1000; i++) calibration_delay();
        clock_gettime(CLOCK_MONOTONIC, &b);
        double us = ((b.tv_sec - a.tv_sec) * 1e6
                   + (b.tv_nsec - a.tv_nsec) / 1e3) / 1000.0;
        printf("  measured %.2f us per call\n", us);
        printf("  (this number is meaningless on your laptop - measure it on the Pico,\n"
               "   at -O0 and at -O3, and explain the difference)\n");
    }

    printf("\nchecksum_all\n");
    {
        static uint8_t big[300];
        for (int i = 0; i < 300; i++) big[i] = 1;
        printf("  summing 300 bytes of value 1, expecting 300...\n");
        fflush(stdout);
        check("checksum_all(big, 300)", (long)checksum_all(big, 300), 300);
    }

    printf("\n%s  (%d failure%s)\n",
           failures ? "HUNT INCOMPLETE" : "TELEMETRY LAYER MATCHES THE SPECIFICATION",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
