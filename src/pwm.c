#include <avr/io.h>

 void pwm_init(void)
 {
   //  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | 
   //                      TCA_SINGLE_CMP0EN_bm | // Enabling buzzer
   //                      TCA_SINGLE_CMP1_bm;     // Enable display;

   //  // Route WO0/1/2 to PORTB (where PB0 = WO0, PB1 = WO1)
   //  PORTMUX.TCAROUTEA = PORTMUX_TCA00_DEFAULT_gc;
   //  // Enable TCA0
   //  TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;


    // Enable output override on PB0.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;

    // PWM initially OFF
    TCA0.SINGLE.PER = 1;
    TCA0.SINGLE.CMP0 = 0;

    // Enable TCA0
    // Use DIV2 clock to reduce frequency
    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV2_gc;

 } void pwm_set_frequency(uint32_t freq_hz)
 {
    // Calculating the period (TOP value)
    // Account for DIV2 prescaler: effective clock = F_CPU/2
    uint32_t period = (F_CPU >> 1) / freq_hz;
    TCA0.SINGLE.PER = period;
    TCA0.SINGLE.CMP0 = period >> 1;
 }

 // TODO: I haven't needed this function yet, remove when refactoring.
 void pwm_set_duty(uint32_t duty_percentage)
 {
    if(duty_percentage > 100) duty_percentage = 100;
    TCA0.SINGLE.CMP0 = (TCA0.SINGLE.PER * duty_percentage/100);
 }