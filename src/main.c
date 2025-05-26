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
    while (1)
    {
        float delay = get_playback_delay(); // This returns seconds (0.25 to 2.0)
        uint16_t delay_ms = (uint16_t)(delay * 1000.0f); // Convert to milliseconds
        uint16_t half_delay_ms = delay_ms / 2; // Half the delay for 50% duty cycle timing

        // Turn on buzzer (50% duty cycle)
        pwm_set_frequency(270);
        pwm_set_duty(50);
        delay_ms_timer(half_delay_ms); // Active for half the period

        // Turn off buzzer
        pwm_set_duty(0);
        delay_ms_timer(half_delay_ms); // Silent for half the period

        break; // Remove this to continue the loop
    }
}
