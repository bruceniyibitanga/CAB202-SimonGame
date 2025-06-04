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
    // Initialize all peripherals
    system_init();
    buttons_init();
    peripherals_init();
    display_init();
    simon_init();
    sei();

    // Main game loop
    while (1) {
        // Handle UART reset command
        extern volatile uint8_t uart_reset;
        if (uart_reset) {
            simon_init();  // Reset the game
            uart_reset = 0;  // Clear the flag
        }
          // Handle UART seed update
        extern volatile uint32_t new_seed;
        extern volatile uint8_t update_seed;
        if (update_seed) {
            // Store the new seed but don't apply it until the next game
            extern uint32_t pending_seed;
            extern uint8_t has_pending_seed;
            pending_seed = new_seed;
            has_pending_seed = 1;
            update_seed = 0;
        }
        
        simon_task();
    }

    return 0;
}