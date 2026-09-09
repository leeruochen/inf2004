/*
 * BUG HUNT #1 - Bits that lie about themselves
 * INF2004 LAB 1
 *
 * A small library of bit-manipulation helpers, of the kind that appears in
 * every GPIO driver ever written. Six defects have been planted.
 *
 * Build on your laptop:   gcc -Wall -Wextra -o bughunt1 bughunt1.c && ./bughunt1
 * Build for the Pico:     see CMakeLists.txt in this folder
 *
 * DO NOT change main() or the expected values. Fix the functions.
 */

#include <stdio.h>
#include <stdbool.h>
// bug 1, missing include for uint8_t and uint32_t types, added #include <stdint.h>
#include <stdint.h>

#ifdef PICO_ON_DEVICE
#include "pico/stdlib.h"
#endif

/* A GPIO mask with GP2 and GP5 set: 0b0010_0100 */
#define LED_MASK 0x00000024u

/* ------------------------------------------------------------------
 * 1. Count how many bits are set in a value.
 *    count_bits(0xFF) must be 8.  count_bits(-1) must be 32.
 *    pico is a 32-bit machine, so int is 32 bits. changing 1 to -1 inverts every bit and add 1, therefore all bits are set.
 * ------------------------------------------------------------------ */
uint8_t count_bits(int value)
{
    // bug 3, missing cast to uint32_t to avoid issues with negative values
    // passing in -1 will cause infinite loop as v >>= 1 will add 1 to the leftmost bit, this means v will never become 0
    // >>= 1 works by shifting bits of v to the right by 1 position, and filling the leftmost bit with the sign bit (0 for positive, 1 for negative). This means that if v is negative, the leftmost bit will always be 1, and v will never become 0.
    // therefore we cast v to uint32_t to avoid issues with negative values, as the leftmost bit will always be 0 for unsigned integers.
    uint32_t v = (uint32_t)value; // Cast to unsigned to avoid issues with negative values

    uint8_t count = 0; // bug 2, missing ;

    while (v) {
        count += v & 1;
        v >>= 1;
    }
    return count;
}

/* ------------------------------------------------------------------
 * 2. True when the value has an even number of bits set.
 *    Used for parity checking on a serial link.
 * ------------------------------------------------------------------ */
bool even_parity(uint32_t value)
{
    uint8_t bits = count_bits(value);

    if (bits % 2 == 0) {
        return true;
    }
    else {
        return false;
    }
}

/* ------------------------------------------------------------------
 * 3. True when the given pin's bit is CLEAR in the mask.
 *    pin_is_clear(0x24, 2) is false - bit 2 is set. 
 *    0x24 = 0010 0100
 *    pin_is_clear(0x24, 3) is true  - bit 3 is clear.
 * ------------------------------------------------------------------ */
bool pin_is_clear(uint32_t mask, unsigned pin)
{
    // originally return (mask & 1u << pin == 0);
    // bug 4, missing parentheses around the bitwise AND operation, causing the comparison 
    // what does 1u << pin do here? what is 1u? 
    // 1u tells c to treat 1 as an unsigned int, when doing bit manipulation, signed ints can cause issues

    // pin_is_clear(0x24, 2)
    // an example, 0x24 = 0010 0100
    // since pin = 2 is set, we want to find if bit 2 is set, in this case, it is set.
    // 1u << 2 equates to 100, so
    // 0010 0100 & 0000 0100, this would give us 0000 0100, which would return false as it is not == 0

    // what if 3 is put in?
    // 1u << 3 equates to 1000
    // 0010 0100 & 0000 1000, gives us 0000 0000, return true

    // why the original was broken, because of operator precedence, the original code was evaluated as:
    // mask & (1u << pin == 0)
    // << comes first, then ==, then &.
    return (mask & (1u << pin)) == 0;
}

/* ------------------------------------------------------------------
 * 4. Reverse the bit order of a 32-bit word.
 *    Bit 0 becomes bit 31, bit 1 becomes bit 30, and so on.
 * ------------------------------------------------------------------ */
uint32_t reverse_bits(uint32_t v)
{
    uint32_t r = 0;

    // bug 5, changed i <= 32 to i < 32, because 32 is out of bounds for a 32-bit integer, the highest bit is bit 31.
    // this shows undefined behavior may silently pass on laptops where i <= 32 may pass (if we ran this code on vscode), but on the pico it will fail as it is out of bounds.
    for (int i = 0; i < 32; i++)
        // (v >> i) & 1, v >> i shifts the bit at position i to LSB, & 1 will perform bitwise AND operation so that only LSB is left.
        // << (31 - i) shifts the LSB to the reversed position.
        // r |=, performs an OR operation against r and the shifted bit, if the shifted bit is 1, it will set the corresponding bit in r to 1, if it is 0, it will leave the corresponding bit in r unchanged.
        r |= ((v >> i) & 1) << (31 - i);

    return r;
}

/* ------------------------------------------------------------------
 * 5. Swap the two nibbles inside every byte of a word.
 *    0x12345678 becomes 0x21436587.
 *    (This one may or may not be broken. Check it.)
 * ------------------------------------------------------------------ */
uint32_t swap_nibbles(uint32_t v)
{
    return ((v & 0x0F0F0F0Fu) << 4) | ((v & 0xF0F0F0F0u) >> 4);
}

/* ------------------------------------------------------------------
 * Test harness - do not modify.
 * ------------------------------------------------------------------ */
static int failures = 0;

static void check_u32(const char *what, uint32_t got, uint32_t expect)
{
    bool ok = (got == expect);
    if (!ok) failures++;
    printf("  %-34s got 0x%08lX  expect 0x%08lX  %s\n",
           what, (unsigned long)got, (unsigned long)expect, ok ? "ok" : "FAIL");
}

static void check_int(const char *what, int got, int expect)
{
    bool ok = (got == expect);
    if (!ok) failures++;
    printf("  %-34s got %-10d expect %-10d %s\n",
           what, got, expect, ok ? "ok" : "FAIL");
}

int main(void)
{
#ifdef PICO_ON_DEVICE
    stdio_init_all();
    sleep_ms(3000);
#endif

    printf("BUG HUNT #1 - bit manipulation\n\n");

    printf("count_bits\n");
    check_int("count_bits(0x000000FF)", count_bits(0x000000FF), 8);
    check_int("count_bits(0x00000024)", count_bits(0x00000024), 2);
    check_int("count_bits(-1)",         count_bits(-1),         32);

    printf("even_parity\n");
    check_int("even_parity(0x000000FF)", even_parity(0x000000FFu), 1);
    check_int("even_parity(0x00000007)", even_parity(0x00000007u), 0);

    printf("pin_is_clear (mask 0x24)\n"); // 0x24 = 0010 0100
    check_int("pin_is_clear(LED_MASK, 2)", pin_is_clear(LED_MASK, 2), 0);
    check_int("pin_is_clear(LED_MASK, 3)", pin_is_clear(LED_MASK, 3), 1);
    check_int("pin_is_clear(LED_MASK, 5)", pin_is_clear(LED_MASK, 5), 0);

    printf("reverse_bits\n");
    check_u32("reverse_bits(0x00000001)", reverse_bits(0x00000001u), 0x80000000u);
    check_u32("reverse_bits(0x12345678)", reverse_bits(0x12345678u), 0x1E6A2C48u);
    check_u32("reverse_bits(0xFFFFFFFF)", reverse_bits(0xFFFFFFFFu), 0xFFFFFFFFu);

    printf("swap_nibbles\n");
    check_u32("swap_nibbles(0x12345678)", swap_nibbles(0x12345678u), 0x21436587u);

    printf("\n%s  (%d failure%s)\n",
           failures ? "HUNT INCOMPLETE" : "ALL TESTS PASS",
           failures, failures == 1 ? "" : "s");

#ifdef PICO_ON_DEVICE
    while (true) tight_loop_contents();
#endif
    return failures ? 1 : 0;
}
