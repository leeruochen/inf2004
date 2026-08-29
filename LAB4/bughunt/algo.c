#include "algo.h"

uint16_t adc_to_mv(uint16_t raw)
{
    uint16_t scaled = raw * VREF_MV;
    return scaled / ADC_MAX;
}

uint16_t duty_from_adc(uint16_t raw, uint16_t wrap)
{
    return (raw / ADC_MAX) * wrap;
}

static uint32_t y = 0;

void iir_reset(void)
{
    y = 0;
}

uint32_t iir_step(unsigned ch, uint32_t x)
{
    (void)ch;
    y += (x - y) >> ALPHA_SHIFT;
    return y;
}

uint16_t pwm_wrap_for_hz(uint32_t hz, float clk_div)
{
    return (uint16_t)(ALGO_SYS_CLK_HZ / clk_div / hz);
}
