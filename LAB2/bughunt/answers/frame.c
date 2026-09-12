#include <stdio.h>
#include <string.h>
#include "frame.h"

uint8_t frame_encode(const reading_t *r, uint8_t *out)
{
    uint8_t len = FRAME_PAYLOAD;
    uint8_t sum = 0;
    uint8_t n   = 0;

    uint16_t temp = (uint16_t)r->temp_c_x10;
    uint8_t p[FRAME_PAYLOAD] = {
        r->sensor_id >> 8, r->sensor_id, r->status,
        temp >> 8, temp,
        r->timestamp_ms >> 24, r->timestamp_ms >> 16,
        r->timestamp_ms >> 8, r->timestamp_ms
    };

    out[n++] = FRAME_SOF;
    out[n++] = len;

    for (uint8_t i = 0; i < len; i++) {
        out[n++] = p[i];
        sum += p[i];
    }

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
    if (len != FRAME_PAYLOAD || in_len != len + 3u) return false;

    for (uint8_t i = 0; i < len; i++)
        payload[i] = in[2 + i];

    for (uint8_t i = 0; i < len; i++)
        sum += payload[i];

    if (sum != in[2 + len])
        return false;

    r->sensor_id    =  (payload[0] << 8) | payload[1];
    r->status       =   payload[2];
    uint16_t temp = ((uint16_t)payload[3] << 8) | payload[4];
    r->temp_c_x10 = (int16_t)(temp <= INT16_MAX ? (int32_t)temp : (int32_t)temp - 65536);
    r->timestamp_ms =  ((uint32_t)payload[5] << 24) | (payload[6] << 16)
                    |  (payload[7] <<  8) |  payload[8];

    return true;
}

void hexdump(const char *label, const uint8_t *buf, uint8_t len)
{
    printf("%-10s [%2u] ", label, len);
    for (uint8_t i = 0; i < len; i++)
        printf("%02X ", buf[i]);
    printf("\n");
}
