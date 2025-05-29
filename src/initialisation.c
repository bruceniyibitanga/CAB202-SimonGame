#include <avr/interrupt.h>
#include "initialisation.h"
#include "timer.h"
#include "display.h"
#include "buzzer.h"
#include "uart.h"
#include "pwm.h"
#include "adc.h"
#include "spi.h"

void system_init(void) {
}

void peripherals_init(void) {
    uart_init(); // Move this to the top!

    // BUZZER (PIN0), USART0 TXD (PIN2)
    PORTB.DIRSET = PIN0_bm | PIN2_bm;

    // Initialise display
    display_init();

    // Initialise SPI for display
    spi_init();

    // Initialise ADC
    adc_init();

    // Initialise PWM
    pwm_init();

    // Initialise timer counters
    timer_init();
}
