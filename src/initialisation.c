#include <avr/interrupt.h>
#include "initialisation.h"
#include "timer.h"
#include "timer_delay.h"
#include "display.h"
#include "buzzer.h"
#include "uart.h"
#include "pwm.h"
#include "adc.h"
#include "spi.h"

void system_init(void) {
}

void peripherals_init(void) {

    // Initialise display
    display_init();

    // Initialise SPI for display
    spi_init();

    // Initialise buzzer
    buzzer_init();

    // Initialise ADC
    adc_init();

    // Initialise PWM
    pwm_init();

    // Initialise UART
    uart_init();
    
    // Initialise timer counters
    timer_init();
    delay_timer_init();

}
