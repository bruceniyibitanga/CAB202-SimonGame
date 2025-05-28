#include <avr/io.h>
#include <avr/interrupt.h>
#include "display.h"
#include "display_macros.h"

volatile uint8_t left_byte = DISP_OFF | DISP_LHS;
volatile uint8_t right_byte = DISP_OFF;

void display_init(void) {
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;  // SPI pins on PC0-3

    // SPI SCK and MOSI
    PORTC.DIRSET = PIN0_bm | PIN2_bm;   // SCK (PC0) and MOSI (PC2) output

    // DISP_LATCH
    PORTA.OUTSET = PIN1_bm;        // DISP_LATCH initial high
    PORTA.DIRSET = PIN1_bm;        // set DISP_LATCH pin as output

    SPI0.CTRLA = SPI_MASTER_bm;    // Master, /4 prescaler, MSB first
    SPI0.CTRLB = SPI_SSD_bm;       // Mode 0, client select disable, unbuffered
    SPI0.INTCTRL = SPI_IE_bm;      // Interrupt enable
    SPI0.CTRLA |= SPI_ENABLE_bm;   // Enable
    
    // Initialize with all segments off
    update_display(DISP_OFF, DISP_OFF);
}

void update_display(const uint8_t left, const uint8_t right) {
    left_byte = left | DISP_LHS;   // Left side with LHS bit set
    right_byte = right;            // Right side (LHS bit not set)
}

void display_write(uint8_t data) {
    SPI0.DATA = data;
}

void swap_display_digit(void) {
    static int digit = 0;
    if (digit) {
        display_write(left_byte);    //left
    } else {
        display_write(right_byte);   //right
    }
    digit = !digit;
}