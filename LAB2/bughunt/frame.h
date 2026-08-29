/*
 * BUG HUNT #2 - wire format for a sensor reading.
 * INF2004 LAB 2
 *
 * Frame layout on the wire (9 bytes of payload):
 *
 *   +------+------+------------------------+----------+
 *   | 0xAA | LEN  | LEN bytes of payload   | CHECKSUM |
 *   +------+------+------------------------+----------+
 *
 *   LEN       number of payload bytes that follow (must be 9)
 *   payload   the reading_t fields, BIG-ENDIAN, tightly packed:
 *
 *               offset 0..1   sensor_id     (uint16, big-endian)
 *               offset 2      status        (uint8)
 *               offset 3..4   temp_c_x10    (int16, big-endian)
 *               offset 5..8   timestamp_ms  (uint32, big-endian)
 *
 *   CHECKSUM  8-bit sum of every payload byte, truncated to 8 bits.
 *             It does NOT cover the 0xAA or the LEN byte.
 *
 * This layout is the specification. It is correct. The code is not.
 */
#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdbool.h>

#define FRAME_SOF      0xAAu
#define FRAME_PAYLOAD  9u      /* bytes of payload, per the spec above */
#define FRAME_MAX      16u     /* biggest frame we will ever buffer    */

typedef struct {
    uint16_t sensor_id;
    uint8_t  status;
    int16_t  temp_c_x10;
    uint32_t timestamp_ms;
} reading_t;

/* Build a frame into out[]. Returns the total number of bytes written. */
uint8_t frame_encode(const reading_t *r, uint8_t *out);

/* Parse a frame from in[]. Returns true only if the frame was valid. */
bool frame_decode(const uint8_t *in, uint8_t in_len, reading_t *r);

/* Print a byte buffer as hex - your most important instrument this week. */
void hexdump(const char *label, const uint8_t *buf, uint8_t len);

#endif
