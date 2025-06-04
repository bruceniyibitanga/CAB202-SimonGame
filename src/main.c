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
        // extern volatile uint32_t new_seed;
        // extern volatile uint8_t update_seed;
        // if (update_seed) {
        //     game_seed = new_seed;
        //     update_lfsr_state(new_seed); // Update the current LFSR state
        //     update_seed = 0;
        // }
        
        simon_task();
    }

    return 0;
}