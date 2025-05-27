#include "stdio.h"
#include "stdint.h"

#ifndef DISPLAY_H
#define DISPLAY_H

// Initialise the display hardware
void display_init(void);
// Show a digit (0-9) or character on the display
void display_show(uint8_t value);
// Show a custom pattern (bitmask)
void display_show_pattern(uint8_t pattern);
// Clear the display
void display_clear(void);

// Update the display buffer
void update_display(const uint8_t left, const uint8_t right);

// Toggle between displaying left and right digit (used by timer ISR)
void display_toggle(void);

extern volatile uint8_t left_byte;

extern volatile uint8_t right_byte;

#endif
