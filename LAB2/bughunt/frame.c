#include <stdio.h>
#include <string.h>
#include "frame.h"
#include <stddef.h> // for offsetof to check struct padding

// 5 bugs

uint8_t frame_encode(const reading_t *r, uint8_t *out)
{
    uint8_t len = FRAME_PAYLOAD;  // BUG #1: len should be FRAME_PAYLOAD, not sizeof(reading_t)
    /* if we use sizeof(reading_t), len will be 12 bytes. this is due to c compiler silently padding reading_t.
        uint16 requires the byte to start at a multiple of 2, so 0, 2, 4
        uint8 requires multiple of 1, so 0 1 2 3
        uint32 requires multiple of 4
        so the first uint16 takes 0 1, then uint8 takes 2, then the next uint16 takes 4 5, since 3 is not a multiple of 2, so currently we have 0 1 2 3 4 5
        uint32 pads a further 6 7 then takes 8 9 10 11, this gives us a total of 12 bytes.
        according to frame.h specification, the payload should be 9 bytes, so we need to use FRAME_PAYLOAD instead of sizeof(reading_t)
     */
    uint8_t sum = 0;
    uint8_t n   = 0;

    // const uint8_t *p = (const uint8_t *)r; // BUG #2: r is passed in as &t->r, r is a pointer to reading_t
    /* casting uint8_t to r tells the compiler to start at the beginning of the reading_t struct.
        as said above, the compiler pads the struct to 12 bytes with 3 bytes of 00s. 
        check bughunt2_host.c for the memory offsets of each field in reading_t.
        this is an example of the padding problem caused for vector A: 
        encoded: AA 0C 34 12 01 00 FD 00 00 00 0D 0C 0B 68 
        expected: AA 09 12 34 01 00 FD 0A 0B 0C 0D 72
        1. AA is the start of frame byte
        2. 0C is the length of payload, which is 12 bytes, but it should be 9 bytes
        3. 34 12 is the sensor_id, which is flipped because of big-endian, but the expected is 12 34
        4. 01 is the status
        5. there is a padding byte of 00 between 01 and FD
        6. FD 00 is the temp_c_x10, which is flipped because of big-endian, but the expected is 00 FD
        7. there are 2 padding bytes of 00 00 between FD 00 and 0D 0C 0B
        8. 0D 0C 0B is the timestamp_ms, expected is 0A 0B 0C 0D
        9. 68 is the checksum, expected is 72
        so the encoded bytes do not match the specification because of the padding problem.
    */
    // temp_c_x10 is int16_t, meaning it is signed, so we need to cast it to uint16_t, similar to bughunt 1
    uint16_t temp = (uint16_t)r->temp_c_x10;
    uint8_t p[FRAME_PAYLOAD] = {
        // previously, it was flipped as computers are little endian. we want big endian.
        // {} forms an array of bytes and p is an array of bytes
        // , acts as an element separator, so first value goes into p[0], second value goes into p[1], etc.
        // when this happens, the little endianness is still not accounted, so if sensor_id is 0x1234, when we do bitwise shifting, it is still 0x1234.
        // since we want 0x12 to go in first, we shift >> by 8 bits so it becomes 0x0012.
        // then we pass it in separated by commas.
        // for 0x34, we dont have to shift as when we pass in 0x1234, it takes in 0x34 as the array is uint8_t, so it only takes in the last 8 bits, which is 0x34.
        // the rest of the logic is the same for status, temp, timestamp
        r->sensor_id >> 8, 
        r->sensor_id, 

        r->status,

        temp >> 8, 
        temp,

        r->timestamp_ms >> 24, 
        r->timestamp_ms >> 16,
        r->timestamp_ms >> 8, 
        r->timestamp_ms
    };

    // according to this hint: do not copy structs onto a wire, write each field out explicitly, one at a time.
    out[n++] = FRAME_SOF;
    out[n++] = len;

    // BUG #3: len instead of len - 1, previously len was using sizeof so -1 was expected, but now len is FRAME_PAYLOAD, so we need to use len instead of len - 1
    for (uint8_t i = 0; i < len; i++) {
        out[n++] = p[i];
        sum += p[i];
    }

    // brute force way to write each field out explicitly
    // out[n++] = r->sensor_id >> 8;
    // sum += r->sensor_id >> 8;
    // out[n++] = r->sensor_id & 0xFF;
    // sum += r->sensor_id & 0xFF;

    // out[n++] = r->status;
    // sum += r->status;

    // out[n++] = r->temp_c_x10 >> 8;
    // sum += r->temp_c_x10 >> 8;
    // out[n++] = r->temp_c_x10 & 0xFF;
    // sum += r->temp_c_x10 & 0xFF;

    // out[n++] = r->timestamp_ms >> 24;
    // sum += r->timestamp_ms >> 24;
    // out[n++] = (r->timestamp_ms >> 16) & 0xFF;
    // sum += (r->timestamp_ms >> 16) & 0xFF;
    // out[n++] = (r->timestamp_ms >> 8) & 0xFF;
    // sum += (r->timestamp_ms >> 8) & 0xFF;
    // out[n++] = r->timestamp_ms & 0xFF;
    // sum += r->timestamp_ms & 0xFF;

    out[n++] = sum;
    return n;
}

bool frame_decode(const uint8_t *in, uint8_t in_len, reading_t *r)
{
    uint8_t payload[FRAME_MAX];
    uint8_t sum = 0;
    uint8_t len;

    if (in_len < 3)         return false;
    if (in[0] != FRAME_SOF) return false;

    len = in[1];
    // BUG #5: len compared to FRAME_PAYLOAD, this is for security reasons 
    // an attacker amy send a frame with a length of 255, which is larger than the payload size of 9, and cause a buffer overflow.
    // so we need to check if len is equal to FRAME_PAYLOAD, and if not, return false.
    if (len != FRAME_PAYLOAD || in_len != len + 3u) return false;

    for (uint8_t i = 0; i < len; i++)
        payload[i] = in[2 + i];

    for (uint8_t i = 0; i < len; i++)
        sum += payload[i];

    if (sum != in[2 + len])
        return false;

    r->sensor_id    =  ((uint16_t)payload[0] << 8) | (uint16_t)payload[1];
    r->status       =   payload[2];

    // typecasted to uint16_t to avoid sign extension when shifting and better fit as temp_c is int16_t
    r->temp_c_x10   =  ((uint16_t)payload[3] << 8) | ((uint16_t)payload[4]);

    // BUG #4: performing bitwise operations on data types smaller than int which is uint8_t in this case
    // leads to undefined behavior if we shift the byte into the 31st bit(signed bit).
    // this error happens here with payload[5] << 24, this shifts the byte into 31st bit.
    // so we typecast payload[5] to uint32_t to avoid this error.
    // to make this even more bulletproof for all architectures, we typecast all payloads to uint32_t before performing bitwise operations.
    // however, this is not a problem for payload[6] << 16, payload[7] << 8, and payload[8] because they are not shifting into the signed bit.
    // but if we move this code to a 16-bit architecture, then payload[6] << 16 will shift into the signed bit, so we typecast all payloads to uint32_t to avoid this error.
    r->timestamp_ms =  ((uint32_t)payload[5] << 24) | ((uint32_t)payload[6] << 16)
                    |  ((uint32_t)payload[7] <<  8) |  ((uint32_t)payload[8]);

    return true;
}

void hexdump(const char *label, const uint8_t *buf, uint8_t len)
{
    printf("%-10s [%2u] ", label, len);
    for (uint8_t i = 0; i < len; i++)
        printf("%02X ", buf[i]);
    printf("\n");
}
