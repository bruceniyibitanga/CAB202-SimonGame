#include <avr/io.h>

#include <avr/interrupt.h>
#include "stdio.h"
#include "stdint.h"
#include "display.h"
#include "display_macros.h"
#include "spi.h"

volatile uint8_t left_byte = DISP_OFF | DISP_LHS;
volatile uint8_t right_byte = DISP_OFF;

void display_init()
{
    PORTB.DIRSET = PIN1_bm;     // PB1 is DISP_EN
    PORTB.OUTCLR = PIN1_bm;     // Active LOW to enable display
}

void update_display(const uint8_t left, const uint8_t right)
{
    left_byte = left | DISP_LHS;   // Left side with LHS bit set
    right_byte = right;            // Right side (LHS bit not set)
}

// Function to display specific digits
void display_digits(uint8_t left_digit, uint8_t right_digit)
{
    const uint8_t digit_patterns[] = {
        DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4,
        DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9
    };
    
    uint8_t left_pattern = (left_digit < 10) ? digit_patterns[left_digit] : DISP_OFF;
    uint8_t right_pattern = (right_digit < 10) ? digit_patterns[right_digit] : DISP_OFF;
    
    update_display(left_pattern, right_pattern);
}

void display_clear(void) {
    update_display(DISP_OFF, DISP_OFF);
}