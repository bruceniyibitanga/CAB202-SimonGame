#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "buzzer.h"
#include "initialisation.h"
#include "timer.h"
#include "uart.h"
#include "button.h"
#include "display.h"
#include "display_macros.h"
#include "simon.h"

int main(void) {
    cli();
    system_init();
    buttons_init();
    peripherals_init();
    display_init();
    simon_init();
    sei(); 

    while (1) {
        update_button_states();
        
        // Handle UART reset command
        extern volatile uint8_t uart_reset;
        if (uart_reset) {
            simon_init();  // Reset the game
            uart_reset = 0;  // Clear the flag
        }
        
        simon_task();
    }

    return 0;
}