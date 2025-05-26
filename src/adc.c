#include <avr/io.h>
#include "stdint.h"
#include "stdio.h"
#include "adc.h"
#include "timer_delay.h"

void adc_init()
{
    // Enable ADC
    ADC0.CTRLA = ADC_ENABLE_bm;
    // Configure prescaler
    ADC0.CTRLB = ADC_PRESC_DIV2_gc;
    // Need 4 CLK PER cyles @ 3.3MHz for 1us, select VDD as ref
    ADC0.CTRLC = (10 << ADC_TIMEBASE_gp) | ADC_REFSEL_VDD_gc;
    // Configure the sample duration of 64
    ADC0.CTRLE = 64;
    // Free running, left adjust result
    ADC0.CTRLF = (ADC_FREERUN_bm | ADC_LEFTADJ_bm);
    // Select the potentiometer (R1)
    ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;
    // Select 8-bit resolution, single-ended
    ADC0.COMMAND = ADC_MODE_SINGLE_8BIT_gc | ADC_START_IMMEDIATE_gc;
}

/*
All results for the Anologue digital converter are stored in the Results register of ADC port.Therefore, we only have to access the to the value in register and then perform some calculations it to determine the ultimate delay.
void adc_read(void) 
*/
uint8_t adc_read()
{
    delay_ms_timer(1);  // instead of _delay_us(50)
    return ADC0.RESULT0;
}


float get_playback_delay()
{
    uint8_t pot_value = adc_read();  // Get latest ADC result (0–255)

    uint8_t reversed_adc = 255 - pot_value;

    // Linearly interpolate between 0.25 and 2.0 seconds
    return 0.25 + ((float)reversed_adc / 255.0f) * (2.0f - 0.25f);
}
