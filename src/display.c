#include <avr/io.h>
#include <avr/interrupt.h>
#include "stdio.h"
#include "stdint.h"
#include "display.h"
#include "display_macros.h"
#include "spi.h"

volatile uint8_t left_byte = DISP_OFF | DISP_LHS;
volatile uint8_t right_byte = DISP_OFF;
static uint8_t display_side = 0; // 0 = left, 1 = right

void display_init()
{
    // Configure the DISP
    PORTB.DIRSET = PIN1_bm;
    PORTB.OUTCLR = PIN5_bm; // TURN ON LED DP

}

void update_display(const uint8_t left, const uint8_t right)
{
    left_byte = left | DISP_LHS;
    right_byte = right;    
}

// Function to toggle display side
void display_toggle(void)
{
    PORTB.OUTCLR = PIN1_bm;
}

void display_show(uint8_t value) {
    // TODO: Implement display pattern lookup and update
    update_display(value, value);
}

void display_clear(void) {
    update_display(DISP_OFF, DISP_OFF);
}

void display_show_pattern(uint8_t pattern) {
    update_display(pattern, pattern);
}