/*
 * LAB 2 CHALLENGE - the command parser on real hardware.
 * INF2004 LAB 2
 *
 * This file is complete. You do not need to change it. The only thing
 * standing between it and a working device is cmdparse.c.
 *
 * Wiring - the same peer-to-peer link as Bug Hunt #2:
 *     this Pico GP8 (TX) -> partner GP9 (RX)
 *     this Pico GP9 (RX) <- partner GP8 (TX)
 *     GND <-> GND
 *
 * Four LEDs on GP2..GP5, each through a 330R resistor to GND.
 *
 * printf goes over USB, NOT over the link. Keep it that way: if your
 * replies and your debug output share a wire you will spend the session
 * debugging the debugger. That is exactly the fault the "Deliberate
 * Intermittent Fault" exercise in this lab is about.
 *
 * Type commands into your partner's serial terminal, or use a USB-serial
 * adapter, and watch the replies come back:
 *
 *     PING            -> OK
 *     SET LED 2 ON    -> OK
 *     GET TEMP        -> OK 25.0
 *     SET LED 9 ON    -> ERR ARG_RANGE
 *     BLINK           -> ERR UNKNOWN_VERB
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "cmdparse.h"

#define LINK_UART   uart1
#define LINK_TX_PIN 8
#define LINK_RX_PIN 9
#define LINK_BAUD   115200

#define LED_BASE    2       /* LEDs on GP2, GP3, GP4, GP5 */

static void link_init(void)
{
    uart_init(LINK_UART, LINK_BAUD);
    gpio_set_function(LINK_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LINK_RX_PIN, GPIO_FUNC_UART);
}

static void leds_init(void)
{
    for (uint i = 0; i < 4; i++) {
        gpio_init(LED_BASE + i);
        gpio_set_dir(LED_BASE + i, GPIO_OUT);
        gpio_put(LED_BASE + i, 0);
    }
}

static void reply(const char *s)
{
    while (*s)
        uart_putc_raw(LINK_UART, *s++);
    uart_putc_raw(LINK_UART, '\r');
    uart_putc_raw(LINK_UART, '\n');
}

/* Stand-in until Lab 4, where you will read the real temperature sensor. */
static float read_temperature_c(void)
{
    return 25.0f;
}

static void act_on(const command_t *c)
{
    char out[32];

    switch (c->verb) {
        case CMD_PING:
            reply("OK");
            break;

        case CMD_GET_TEMP:
            snprintf(out, sizeof out, "OK %.1f", (double)read_temperature_c());
            reply(out);
            break;

        case CMD_SET_LED:
            gpio_put(LED_BASE + c->led_index, c->led_on ? 1 : 0);
            reply("OK");
            break;
    }
}

int main(void)
{
    cmd_reader_t reader;

    stdio_init_all();
    sleep_ms(3000);

    link_init();
    leds_init();
    cmd_reader_reset(&reader);

    printf("LAB 2 CHALLENGE - command parser ready on UART1 @ %d baud\n", LINK_BAUD);
    printf("try: PING / GET TEMP / SET LED 2 ON\n");

    while (true) {
        if (!uart_is_readable(LINK_UART))
            continue;

        char c = (char)uart_getc(LINK_UART);
        int  r = cmd_reader_feed(&reader, c);

        if (r == 0)
            continue;

        if (r < 0) {
            printf("line too long, discarded\n");
            reply(cmd_status_str(CMD_ERR_TOO_LONG));
            continue;
        }

        command_t    cmd;
        char         line[CMD_MAX_LINE + 1];
        cmd_status_t st;

        /* cmd_parse modifies the line in place, so hand it a copy and keep
         * the original for the log. On a real device you would not bother. */
        memcpy(line, reader.buf, sizeof line);
        st = cmd_parse(line, &cmd);

        printf("rx \"%s\" -> %s\n", reader.buf, cmd_status_str(st));

        if (st == CMD_OK) act_on(&cmd);
        else              reply(cmd_status_str(st));
    }
}
