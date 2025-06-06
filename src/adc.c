#include <avr/io.h>
#include "stdint.h"
#include "stdio.h"
#include "adc.h"
#include "uart.h"
#include "uart.h"

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
    // Manual conversion mode, left adjust result
    ADC0.CTRLF = ADC_LEFTADJ_bm;
    // Select the potentiometer (R1)
    ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;
    // Configure for 8-bit resolution, single-ended (don't start conversion yet)
    ADC0.COMMAND = ADC_MODE_SINGLE_8BIT_gc;
}

/*
All results for the Anologue digital converter are stored in the Results register of ADC port.Therefore, we only have to access the to the value in register and then perform some calculations it to determine the ultimate delay.
void adc_read(void) 
*/
/*
All results for the Anologue digital converter are stored in the Results register of ADC port.Therefore, we only have to access the to the value in register and then perform some calculations it to determine the ultimate delay.
*/
uint8_t adc_read()
{
    ADC0.COMMAND |= ADC_START_IMMEDIATE_gc;
    
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {
    }
    
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    
    return ADC0.RESULT0;
}

uint16_t get_potentiometer_delay(void)
{
    uint8_t pot_value = adc_read();
    // Map potentiometer value (0-255) to range 250ms - 2000ms
    uint16_t value = 250 + ((uint32_t)pot_value * (2000 - 250)) / 255;
    return value; // Return the calculated delay in milliseconds

}