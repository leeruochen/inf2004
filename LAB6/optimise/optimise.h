/*
 * LAB 6 EXERCISE - four things that are slower than they need to be.
 * INF2004 LAB 6
 *
 * ==================================================================
 * THE RULES
 * ==================================================================
 *
 * Four functions below come in pairs. The _slow() version is written for
 * you and WORKS - it is not buggy, it is merely expensive. You write the
 * _fast() version.
 *
 *   1. The _fast() version must return EXACTLY the same answer as the
 *      _slow() one for every input the harness tries. An optimisation
 *      that changes the answer is not an optimisation, it is a defect.
 *
 *   2. You may not modify the _slow() versions, the harness, or the test
 *      data. Making the benchmark easier is not making the code faster.
 *
 *   3. For each one you must report THREE things:
 *        - microseconds before and after, measured on the Pico
 *        - WHY it is faster, in one sentence naming the actual mechanism
 *        - for at least one of the four, a disassembly extract proving it
 *
 * A number without a mechanism is a measurement, not an explanation. A
 * mechanism without a number is a belief.
 *
 * ==================================================================
 * WHY THIS PROCESSOR MAKES THESE CHOICES MATTER
 * ==================================================================
 *
 * The RP2040 has two Cortex-M0+ cores. Two facts about that core drive
 * three of the four exercises below, and neither is true of the laptop
 * you have been testing on all semester:
 *
 *   - There is NO floating-point unit. Every float operation is a call
 *     into a software library - tens to hundreds of cycles for something
 *     that costs one cycle on your laptop.
 *
 *   - There is NO hardware divide instruction. ARMv6-M has no SDIV or
 *     UDIV. Every `/` and `%` on a value the compiler cannot fold is a
 *     call to __aeabi_uidiv. (The RP2040 has a hardware divider in its
 *     SIO block, and the SDK will use it - but a division is still far
 *     more expensive than a multiply.)
 *
 * This is why "it was fast enough on my machine" is not evidence, and it
 * is why you are asked to measure on the target.
 *
 * ==================================================================
 * Build and check correctness on your laptop first:
 *   gcc -O2 -Wall -Wextra -o optimise_host optimise_host.c optimise.c
 *   ./optimise_host
 *
 * Then measure on the Pico: see CMakeLists.txt in this folder.
 * ==================================================================
 */
#ifndef OPTIMISE_H
#define OPTIMISE_H

#include <stdint.h>
#include <stddef.h>

#define ADC_MAX_COUNT  4095u
#define VREF_MV        3300u

/* ------------------------------------------------------------------
 * 1. ADC counts -> millivolts, over a whole buffer.
 *
 *    out[i] = raw[i] * 3300 / 4095, truncated toward zero.
 *
 *    The slow version does it in floating point. Read the note about
 *    the missing FPU above, then find another way to compute the same
 *    values. Watch for overflow: what is the largest intermediate your
 *    arithmetic produces, and does it fit?
 * ------------------------------------------------------------------ */
void mv_convert_slow(const uint16_t *raw, uint16_t *out, size_t n);
void mv_convert_fast(const uint16_t *raw, uint16_t *out, size_t n);

/* ------------------------------------------------------------------
 * 2. Count how many samples fall inside a calibrated band.
 *
 *    The band edges come from calib_low()/calib_high(), which depend
 *    only on `cal` - a value that does not change inside the loop.
 *    Those two functions are deliberately expensive and deliberately
 *    NOT inlinable, exactly like the vendor library function you will
 *    one day find being called 4096 times per frame.
 *
 *    The answer must not change.
 * ------------------------------------------------------------------ */
uint32_t count_in_band_slow(const uint16_t *raw, size_t n, uint16_t cal);
uint32_t count_in_band_fast(const uint16_t *raw, size_t n, uint16_t cal);

/* ------------------------------------------------------------------
 * 3. CRC-8/ATM over a buffer. Polynomial 0x07, init 0x00, no reflection,
 *    no final XOR.  crc8("123456789", 9) == 0xF4
 *
 *    The slow version shifts one bit at a time: eight iterations per
 *    byte. There is a well-known way to do this one byte at a time.
 *    You have already seen it - it is in bughunt6.c.
 *
 *    This one has a price attached. State what it costs as well as what
 *    it saves, and say when you would refuse to pay it.
 * ------------------------------------------------------------------ */
uint8_t crc8_slow(const uint8_t *data, size_t len);
uint8_t crc8_fast(const uint8_t *data, size_t len);

/* ------------------------------------------------------------------
 * 4. Normalise a buffer to permille (0..1000) of its total.
 *
 *    out[i] = raw[i] * 1000 / total, where total is the sum of raw[].
 *
 *    `total` is computed once and then divided by n times. Re-read the
 *    note about the missing divide instruction, and think about what
 *    you can compute once instead of n times.
 *
 *    Getting the same answer as the slow version is the hard part of
 *    this one, not the speed. Fixed-point reciprocals lose precision;
 *    the harness will catch you if yours does.
 * ------------------------------------------------------------------ */
void normalise_slow(const uint16_t *raw, uint16_t *out, size_t n);
void normalise_fast(const uint16_t *raw, uint16_t *out, size_t n);

#endif
