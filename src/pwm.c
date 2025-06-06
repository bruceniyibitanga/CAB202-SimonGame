#include <avr/io.h>
void pwm_init(void)
{
    // Setup PWM on timer TCA0 for the buzzer
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;

    // Start with PWM off (no sound)
    TCA0.SINGLE.PER = 1;
    TCA0.SINGLE.CMP0 = 0;

    // Turn on the timer with divide-by-2 prescaler
    // This gives us better frequency range for the buzzer
    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV2_gc;
}void pwm_set_frequency(uint32_t freq_hz)
{
    // Figure out what period gives us the frequency we want
    // We're using a divide-by-2 prescaler so the clock is half speed
    uint32_t period = (F_CPU >> 1) / freq_hz;
    TCA0.SINGLE.PER = period;
    TCA0.SINGLE.CMP0 = period >> 1;
}