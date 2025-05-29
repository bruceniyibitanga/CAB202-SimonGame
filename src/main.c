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
    uart_puts("Starting Simon Game Before Loop...\n");

    // Main game loop
    while (1) {
        update_button_states();  // Update button states before simon task
        simon_task();
    }

    return 0;
}