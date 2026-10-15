#include "algo.h"

uint16_t adc_to_mv(uint16_t raw)
{
    // OLD uint16_t scaled = raw * VREF_MV; // #define VREF_MV 3300u
    // BUG 1: vref_mv is 3300, using uint16_t has upper limit of 65535
    // for any ADC reading greater than 19, eg. 20 * 3300 = 66000 overflows
    return (uint16_t)(((uint32_t)raw * VREF_MV) / ADC_MAX);
    // promote raw to uint32 first to avoid overflow then divide and set to uint16_t
}

uint16_t duty_from_adc(uint16_t raw, uint16_t wrap)
{
    // BUG 2: similar to bug 1, raw * wrap can overflow uint16_t for any raw > 19 and wrap > 3
    // return (raw * wrap) / ADC_MAX;
    return (uint16_t)(((uint32_t)raw * wrap) / ADC_MAX);
}

// bug 3: iir_step() requires to remember the preiovus y value for each channel even after the function finishes
// static variable is used to store the previous y value for each channel
// if different channels are used, the previous y gets overwritten to the latest channel's y
// in algo.h, there is a definition for NUM_CHANNELS which is 2u
// we likley need to use it to fix y
// OLD static uint32_t y = 0;
static uint32_t y[NUM_CHANNELS] = {0};
// 

void iir_reset(void)
{
    for (unsigned ch = 0; ch < NUM_CHANNELS; ch++) {
        y[ch] = 0;
    }
}

uint32_t iir_step(unsigned ch, uint32_t x)
{
    // bug 4: using void type cast is literally to suppress warnings, probably a method to hide bugs
    // (void)ch;
    // y += (x - y) >> ALPHA_SHIFT;
    // return y;
    if (ch >= NUM_CHANNELS) {
        return 0; // invalid channel, return 0
    }

    int32_t diff = (int32_t)x - (int32_t)y[ch];
    y[ch] += (diff >> ALPHA_SHIFT);
    return (uint32_t)y[ch];
    // initially it uses uint32 for x, but if x < y, the diff would be negative.
    // we use int32 so that 2's complement is used to represent negs
    // then we also apply the previous bug 3 fix to use y[ch] instead of y so each channel has its own y
    // the result is stored in y[ch] as a uint32_t and returned as a uint32_t

}

uint16_t pwm_wrap_for_hz(uint32_t hz, float clk_div)
{
    // bug 5: this is a reverse formulae to calculate wrap value from clk_div and hz wanted for the PWM period.
    // they want 20hz so this formula gives 125,000,000 / 250 / 20 = 25000.
    // we know the formula is f_sys/clk_div * (warp + 1).
    // so we need to return the answer - 1 for correct wrap value.
    // return (uint16_t)(ALGO_SYS_CLK_HZ / clk_div / hz);
    uint32_t total_ticks = (uint32_t)((float)ALGO_SYS_CLK_HZ / (clk_div * (float)hz));
    return (uint16_t)(total_ticks - 1);
}
