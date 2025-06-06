#include <avr/io.h>
#include <avr/interrupt.h>
#include "button.h"
#include "uart.h"

// Button state variables
volatile uint8_t pb_debounced_state;
volatile uint8_t pb_state_curr;
volatile uint8_t pb_state_prev;
volatile uint8_t pb_falling_edge;
volatile uint8_t pb_rising_edge;
volatile uint8_t pb_released = 1;
volatile uint8_t pb_current = 0;

// Define and initialize the global latch for button presses
volatile uint8_t g_latched_button_flags = 0;

void buttons_init(void)
{
    // Set PORTA PIN4-7 as inputs with pull-ups
    PORTA.DIRCLR = PIN4_bm | PIN5_bm | PIN6_bm | PIN7_bm;
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm;

    // Initialize states
    pb_debounced_state = PORTA.IN;
    pb_state_curr = pb_debounced_state;
    pb_state_prev = pb_state_curr;
}

void update_button_states(void)
{
    // Save previous state
    pb_state_prev = pb_state_curr;
    // Read current state
    pb_state_curr = pb_debounced_state;

    // Calculate edges
    pb_falling_edge = (pb_state_prev ^ pb_state_curr) & pb_state_prev;
    pb_rising_edge = (pb_state_prev ^ pb_state_curr) & pb_state_curr;

    // Update the global latch with any new falling edges
    if (pb_falling_edge) {
        g_latched_button_flags |= pb_falling_edge;
    }
}

bool button_pressed(uint8_t button_mask)
{
    return (pb_falling_edge & button_mask) != 0;
}

bool button_released(uint8_t button_mask)
{
    return (pb_rising_edge & button_mask) != 0;
}