#include <stdio.h>
#include "pico/stdlib.h"

// --- LEDs: bit0..bit3 map to GP2..GP5 ---
#define NUM_LEDS 4
const uint LED_PINS[NUM_LEDS] = {2, 3, 4, 5};

#define BUTTON_A 20   // shift left (rotate)
#define BUTTON_B 21   // toggle rightmost LED which is GP2

#define LED_MASK 0x0F // keep the pattern within 4 bits

static void update_leds(uint8_t pattern) { // Update the physical LEDs to match the given 4-bit pattern, eg. pattern = 0x01 means only the rightmost LED is ON, pattern = 0x0F means all 4 LEDs are ON.
    for (int i = 0; i < NUM_LEDS; i++) { // iterate through each LED pin
        // Extract bit i using shift + mask, no other bitwise op needed
        bool on = (pattern >> i) & 0x1; // Extract the i-th bit, shifts the pattern right by i and masks with 1 to get the least significant bit
        gpio_put(LED_PINS[i], on); // if on is true, turn the LED on.
    }
}

static void print_pattern(uint8_t pattern) {
    printf("LED pattern: ");
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
        printf("%d", (pattern >> i) & 0x1);
    }
    printf("\n");
}

// Returns true once per physical press (falling edge, active-low button),
// with a simple release-required debounce so a held button only fires once.
static bool button_pressed_edge(uint gpio, bool *was_down) {
    bool down_now = !gpio_get(gpio); // active low with pull-up
    bool fired = false;

    if (down_now && !(*was_down)) {
        sleep_ms(20); // debounce settle
        if (!gpio_get(gpio)) {
            fired = true;
        }
    }
    *was_down = down_now;
    return fired;
}

int main() {
    stdio_init_all();

    for (int i = 0; i < NUM_LEDS; i++) { // Initialize each LED pin as output
        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
    }

    gpio_init(BUTTON_A); // Initialize button A pin as input with pull-up
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B); // Initialize button B pin as input with pull-up
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    uint8_t leds = 0x01; // start at 0001, rightmost LED ON
    update_leds(leds);
    print_pattern(leds);

    bool a_was_down = false;
    bool b_was_down = false;

    while (true) {
        if (button_pressed_edge(BUTTON_A, &a_was_down)) {
            // Shift left, then wrap back to 0001 if the bit fell off the top
            leds = (leds << 1) & LED_MASK;
            if (leds == 0) {
                leds = 0x01;
            }
            update_leds(leds);
            print_pattern(leds);
        }

        if (button_pressed_edge(BUTTON_B, &b_was_down)) {
            // Toggle only the rightmost LED, leave the other 3 bits untouched
            leds ^= 0x01;
            update_leds(leds);
            print_pattern(leds);
        }

        sleep_ms(5); // light polling delay, keeps the loop responsive
    }

    return 0;
}