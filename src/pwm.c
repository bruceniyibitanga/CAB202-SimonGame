#include <avr/io.h>

 void pwm_init(void)
 {
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | 
                        TCA_SINGLE_CMP0EN_bm | // Enabling buzzer
                        TCA_SINGLE_CMP1_bm;     // Enable display;

    // Route WO0/1/2 to PORTB (where PB0 = WO0, PB1 = WO1)
    PORTMUX.TCAROUTEA = PORTMUX_TCA00_DEFAULT_gc;
    // Enable TCA0
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
 }

 void pwm_set_frequency(uint16_t freq_hz)
 {
    // Calculating the peiod (TOP value)
    uint32_t period = F_CPU/freq_hz;
    TCA0.SINGLE.PER = period;
    
    // Default 50% duty cyle.
    // Have the compare value as 50% the TOP value
    TCA0.SINGLE.CMP0 = period/2;
 }

 void pwm_set_duty(uint16_t duty_percentage)
 {
    if(duty_percentage > 100) duty_percentage = 100;
    TCA0.SINGLE.CMP0 = (TCA0.SINGLE.PER * duty_percentage/100);
 }