#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

// Button states and edge detection
extern volatile uint8_t pb_debounced_state;
extern volatile uint8_t pb_state_curr;
extern volatile uint8_t pb_state_prev;
extern volatile uint8_t pb_falling_edge;
extern volatile uint8_t pb_rising_edge;
extern volatile uint8_t pb_released;
extern volatile uint8_t pb_current;

// Initialize button handling
void buttons_init(void);

// Update button states (call this regularly)
void update_button_states(void);

// Check if specific button was pressed (using falling edge)
bool button_pressed(uint8_t button_mask);

// Check if specific button was released (using rising edge)
bool button_released(uint8_t button_mask);

#endif // BUTTON_H