/*
 * BUG HUNT #2 - the link itself.
 * INF2004 LAB 2
 *
 * Flash this SAME program onto BOTH Picos. Wire them as in the lab's paired
 * UART task:
 *
 *      Pico A GP8 (TX1) ----> Pico B GP9 (RX1)
 *      Pico A GP9 (RX1) <---- Pico B GP8 (TX1)
 *      Pico A GND       <---> Pico B GND
 *
 * Press the button on GP20 to transmit one reading. Both boards print every
 * frame they send and every frame they receive, as hex.
 *
 * Two of the seven planted defects live in THIS file and cannot be reproduced
 * on your laptop. Fix frame.c first.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "frame.h"

#define LINK_UART   uart1
#define LINK_TX_PIN 8
#define LINK_RX_PIN 9
#define LINK_BAUD   9600
#define BUTTON_PIN  20

static void link_init(void)
{
    uart_init(LINK_UART, LINK_BAUD);
    gpio_set_function(LINK_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LINK_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(LINK_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(LINK_UART, true);

    /* keeps the serial monitor tidy */
    // BUG #5: this is a text formatting convenience that looks for line feed byte such as \n or 0x0A
    // if found, automatically tranmists a carriage return \r or 0x0D before it.
    // an example of what happens on pico
    // tx: AA 09 12 34 01 00 FD 0A 0B 0C 0D 72
    // rx: AA 09 12 34 01 00 FD 0D 0A 0B 0C 0D
    // we can see that 0D comes before 0A, which meant that the carriage return was added, corrupting the frame.
    // originally, this translate_crlf was set to true, we set to false to ensure UART transmits raw binary without any interception
    uart_set_translate_crlf(LINK_UART, false);
}

static void send_reading(const reading_t *r)
{
    uint8_t frame[FRAME_MAX];
    uint8_t n = frame_encode(r, frame);

    hexdump("tx", frame, n);

    for (uint8_t i = 0; i < n; i++)
        uart_putc(LINK_UART, frame[i]);
}

static void rx_poll(void)
{
    static uint8_t buf[FRAME_MAX];
    static uint8_t n = 0;

    while (uart_is_readable(LINK_UART)) {
        buf[n++] = (uint8_t)uart_getc(LINK_UART);
        // BUG #5: buf accepts every incoming byte into buf blindly and increments til it reaches 12.
        // if the first byte is not a start of frame, we need to reset the buffer and start over.
        // this forces the receiver to reject garbage bytes and stay hunting for a real SOF byte which is 0xAA. 
        // this is a security measure to prevent an attacker from sending garbage bytes to the receiver and cause a buffer overflow.
        if (n == 1 && buf[0] != FRAME_SOF){
            n = 0; // reset buffer if first byte is not start of frame
        }

        if (n == 2 + FRAME_PAYLOAD + 1) {
            reading_t r;

            hexdump("rx", buf, n);

            if (frame_decode(buf, n, &r))
                printf("           id=0x%04X status=%u temp=%d.%d C t=%u ms\n",
                       r.sensor_id, r.status,
                       r.temp_c_x10 / 10,
                       (r.temp_c_x10 < 0 ? -r.temp_c_x10 : r.temp_c_x10) % 10,
                       r.timestamp_ms);
            else
                printf("           BAD FRAME (checksum or header rejected)\n");

            n = 0;
        }
    }
}

// ideal answer
// static void rx_poll(void)
// {
//     static uint8_t buf[FRAME_MAX];
//     static uint8_t n = 0;
//     static uint32_t last_byte_us;

//     /* Discard an incomplete frame after an inter-byte gap. At 9600 baud,
//      * a complete 12-byte frame takes about 12.5 ms. */
//     if (n && (uint32_t)(time_us_32() - last_byte_us) > 100000u) n = 0;

//     while (uart_is_readable(LINK_UART)) {
//         uint8_t byte = (uint8_t)uart_getc(LINK_UART);
//         last_byte_us = time_us_32();
//         if (n == 0 && byte != FRAME_SOF) continue;
//         if (n == 1 && byte != FRAME_PAYLOAD) {
//             n = byte == FRAME_SOF ? 1 : 0;
//             continue;
//         }
//         buf[n++] = byte;

//         if (n == 2 + FRAME_PAYLOAD + 1) {
//             reading_t r;

//             hexdump("rx", buf, n);

//             if (frame_decode(buf, n, &r))
//                 printf("           id=0x%04X status=%u temp=%s%d.%d C t=%lu ms\n",
//                        r.sensor_id, r.status,
//                        r.temp_c_x10 < 0 ? "-" : "",
//                        (r.temp_c_x10 < 0 ? -r.temp_c_x10 : r.temp_c_x10) / 10,
//                        (r.temp_c_x10 < 0 ? -r.temp_c_x10 : r.temp_c_x10) % 10,
//                        (unsigned long)r.timestamp_ms);
//             else {
//                 printf("           BAD FRAME (checksum or header rejected)\n");
//                 /* Retain a possible next start already inside the failed
//                  * frame, rather than discarding it with the bad prefix. */
//                 uint8_t start = 1;
//                 while (start < n && buf[start] != FRAME_SOF) start++;
//                 uint8_t remaining = n - start;
//                 for (uint8_t i = 0; i < remaining; i++) buf[i] = buf[start + i];
//                 n = remaining;
//                 if (n >= 2 && buf[1] != FRAME_PAYLOAD) n = 0;
//                 continue;
//             }

//             n = 0;
//         }
//     }
// }

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);

    link_init();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    printf("BUG HUNT #2 - press GP20 to send a reading\n");
    printf("sizeof(reading_t) = %u bytes\n\n", (unsigned)sizeof(reading_t));

    bool was_down = false;
    uint16_t seq  = 0;

    while (true) {
        bool is_down = !gpio_get(BUTTON_PIN);

        if (is_down && !was_down) {
            reading_t r = {
                .sensor_id    = 0x1234,
                .status       = 0x01,
                .temp_c_x10   = 253,
                .timestamp_ms = 0x0A0B0C0D + seq++,
            };
            send_reading(&r);
        }
        was_down = is_down;

        rx_poll();
        sleep_ms(10);
    }
}

// reflection
// Plain char is unsigned on the Pico's compiler (arm-none-eabi-gcc) and signed on your laptop's x86 GCC. If you ever store a received byte in a char and compare it to 0xAA, that comparison is true on the Pico and false on your laptop, from identical source.
// Nothing in this hunt depends on it. But when the host harness and the hardware disagree and you cannot see why, this is the first thing to check — and it is why "it passed on my laptop" is never the end of the argument.
// Use uint8_t for bytes. Always.