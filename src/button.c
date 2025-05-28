#include <avr/io.h>
#include <avr/interrupt.h>
#include "button.h"
#include "uart.h"

void buttons_init(void)
{
    // Enable pull-up resistors for push buttons.
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm;
}

// extern volatile uint8_t pb_debounced_state;
// volatile uint8_t pb_previous_state = 0xFF; // Assume all buttons are unpressed.
// uint8_t get_button_pressed()
// {
    
//     uint8_t pb_current_state = pb_debounced_state;
//     uint8_t pb_change = pb_current_state ^ pb_previous_state;
//     uint8_t pb_falling_edge = pb_change & ~pb_current_state;
//     pb_previous_state = pb_current_state;
//     return pb_falling_edge;
// }


// Button debouncing is now handled in timer.c ISR