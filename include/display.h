#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void display_init(void);
void update_display(const uint8_t left, const uint8_t right);
void display_write(uint8_t data);
void swap_display_digit(void);

extern volatile uint8_t left_byte;
extern volatile uint8_t right_byte;

#endif