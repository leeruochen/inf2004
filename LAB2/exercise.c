#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID   uart1
#define BAUD_RATE 115200

#define UART_TX_PIN 8   // GP8
#define UART_RX_PIN 9   // GP9 (looped back to GP8)

#define BUTTON_GP22 22  // Maker Pi Pico onboard button, active LOW

int main() {
    stdio_init_all();

    // --- UART1 setup (TX = GP8, RX = GP9) ---
    uart_init(UART_ID, BAUD_RATE); // initialize UART1 with the specified baud rate
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // set GP8 to UART function
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // set GP9 to UART function

    // --- Button setup ---
    // Maker Pi Pico buttons pull the pin to GND when pressed,
    // so we enable the internal pull-up and treat LOW as "pressed".
    gpio_init(BUTTON_GP22);
    gpio_set_dir(BUTTON_GP22, GPIO_IN); // set GP22 as input
    gpio_pull_up(BUTTON_GP22); // enable pull-up resistor on GP22

    char letter = 'A';
    absolute_time_t next_send_time = make_timeout_time_ms(1000); // absolute_time_t is a struct that represents an absolute time in microseconds since boot. make_timeout_time_ms() creates an absolute_time_t that is the current time plus the specified number of milliseconds.

    while (true) {
        // --- Transmit side: fires once every 1 second ---
        if (time_reached(next_send_time)) {
            bool pressed = !gpio_get(BUTTON_GP22); // gpio_get returns false if pressed, hence we negate it to get true if pressed.

            if (pressed) { // transmit uppercase letters A-Z when the button is pressed
                uart_putc(UART_ID, letter);
                letter++;
                if (letter > 'Z') { // reset to 'A' after 'Z'
                    letter = 'A';
                }
            } else { // transmit '1' when the button is not pressed
                uart_putc(UART_ID, '1');
            }

            next_send_time = make_timeout_time_ms(1000); // schedule the next send time to be 1 second from now
        }

        // --- Receive side: checked every loop, non-blocking ---
        if (uart_is_readable(UART_ID)) { // check if there is data available to read from UART1
            char received = uart_getc(UART_ID); // read a character from UART1

            if (received >= 'A' && received <= 'Z') { // if the received character is an uppercase letter, convert it to lowercase and print it
                char lower = received + ('a' - 'A'); // eg. A + (97 - 65) = 65 + 32 = 97 = a
                printf("%c\n", lower);
            } else if (received == '1') { // if the received character is '1', print "2"
                printf("2\n");
            }
        }

        // testing the bug which is chars dropping when loop is fast.
        // sleep_ms(1000); // baseline: 1 char/sec: no character losses
        // sleep_ms(100); // 10 chars/sec: some laptops may be clean and have no char losses, others may drop some characters. this timing is dependent on USB host stack.
        // // no sleep, to run as fast as possible: 

    }

    return 0;
}
