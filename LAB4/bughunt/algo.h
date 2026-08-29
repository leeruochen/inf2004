/*
 * BUG HUNT #4 - the signal chain, as pure arithmetic.
 * INF2004 LAB 4
 *
 * SPECIFICATION - this is correct. The implementation is not.
 *
 *   adc_to_mv(raw)
 *       Convert a 12-bit ADC reading (0..4095) to millivolts against a
 *       3.3 V reference. Integer truncation is acceptable; losing the
 *       top of the range is not.
 *           adc_to_mv(0)    ==    0
 *           adc_to_mv(2048) == 1650
 *           adc_to_mv(4095) == 3300
 *
 *   duty_from_adc(raw, wrap)
 *       Map a 12-bit ADC reading onto a PWM level in 0..wrap, linearly.
 *           duty_from_adc(0,    999) ==   0
 *           duty_from_adc(2048, 999) == 499
 *           duty_from_adc(4095, 999) == 999
 *
 *   iir_step(ch, x)
 *       First-order exponential filter, one independent state per channel:
 *           y[ch] += (x - y[ch]) / 16
 *       It must converge on x from BOTH directions, and must never
 *       overshoot or wrap.
 *
 *   pwm_wrap_for_hz(hz, clk_div)
 *       The wrap value that gives a PWM period of exactly 1/hz seconds,
 *       given the system clock of 125 MHz and the supplied clock divider.
 *       Remember the counter runs 0..wrap INCLUSIVE.
 */
#ifndef ALGO_H
#define ALGO_H

#include <stdint.h>

#define ADC_MAX      4095u
#define VREF_MV      3300u
#define ALPHA_SHIFT  4u
#define NUM_CHANNELS 2u
#define ALGO_SYS_CLK_HZ   125000000u

uint16_t adc_to_mv(uint16_t raw);
uint16_t duty_from_adc(uint16_t raw, uint16_t wrap);
uint32_t iir_step(unsigned ch, uint32_t x);
uint16_t pwm_wrap_for_hz(uint32_t hz, float clk_div);

void iir_reset(void);

#endif
