#include <avr/io.h>
#include <avr/interrupt.h>
#include "button.h"

extern volatile uint8_t pb_debounced_state;

void buttons_init(void)
{
    // Enable pull-up resistors for push buttons.
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}

volatile uint8_t pb_previous_state = 0xFF; // Assume all buttons are unpressed.
uint8_t detect_button_pressed(uint8_t pin_bm)
{
    uint8_t pb_current_state = pb_debounced_state;
    uint8_t pb_change = pb_current_state ^ pb_previous_state;
    uint8_t pb_falling_edge = pb_change & ~pb_current_state;
    return pb_falling_edge & pin_bm;
}

// Button debouncing is now handled in timer.c ISR