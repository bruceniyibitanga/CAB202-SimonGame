#include <avr/io.h>
#include "stdio.h"
#include "stdint.h"
#include "display.h"
#include "display_macros.h"

volatile uint8_t left_byte = DISP_OFF | DISP_LHS;
volatile uint8_t right_byte = DISP_OFF;

void display_init()
{
    // Configure the display to be an output
    PORTB.DIRSET = PIN1_bm;
    
    // Initially have the display turned off.
    update_display(0,0);
}


void update_display(const uint8_t left, const uint8_t right)
{
    left_byte = left | DISP_LHS;
    right_byte = right;    
}