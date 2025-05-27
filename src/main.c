#include <avr/interrupt.h>
#include "stdio.h"
#include "stdint.h"

#include "initialisation.h"
#include "timer_delay.h"
#include "simon.h"
#include "button.h"
#include "pwm.h"
#include "adc.h"
#include "uart.h"
#include "display.h"
#include "timer.h"
#include "button_macros.h"

void state_machine(void);

int main(void)
{
    cli();
    // Call your initialisation functions here
    system_init();
    buttons_init();
    peripherals_init();
    // simon_init();
    sei();

    state_machine();

    // The program should not reach this point
    while (1)
        ;
}

void state_machine(void)
{

    // Base frequency for each button
   /*  const uint16_t frequencies[] = {262, 330, 392, 440}; // C4, E4, G4, A4

    const uint8_t button_pins[] = {
        (1 << 4),  // S1 - PA4
        (1 << 5),  // S2 - PA5
        (1 << 6),  // S3 - PA6
        (1 << 7)   // S4 - PA7
    }; */

    while (1)
    {
        /* for(uint8_t i = 0; i < 4; i++) {
            if(detect_button_pressed(button_pins[i])) {
                
                char msg[50];
                sprintf(msg, "Button S%d pressed\r\n", i+1);
                uart_puts(msg);
                
                // Play the corresponding tone
                pwm_set_frequency(frequencies[i]);
                pwm_set_duty(50);  // 50% duty cycle

                // Get playback delay (if you have this function)
                // Otherwise use a fixed delay
                float delay = 0.5f; // 500ms default, replace with get_playback_delay() if available
                uint16_t half_delay_ms = (uint16_t)((delay * 1000.0f) / 2.0f);
                
                // Tone active for half the delay period
                delay_ms_timer(half_delay_ms);
                
                // Stop tone for the remaining half
                pwm_set_duty(0);
                // delay_ms_timer(half_delay_ms);

                uart_puts("Tone finished\r\n");
            }
        }
            // After checking all buttons, update pb_previous_state
            pb_previous_state = pb_debounced_state;
            // Small delay to prevent overwhelming the system
            delay_ms_timer(10);*/ 

            while(uart_receive() != 'a');
        
            uart_puts("Past the while loop! Should turn on display. This indicates that SPI logic is working!");
            
            update_display(0,0);

            uart_puts("Passed update function.");
    } 
}
