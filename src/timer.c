#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "display.h"

void timer_init(void)
{
    TCB1.CTRLB = TCB_CNTMODE_INT_gc; // Configure TCB1 in periodic interrupt mode
    TCB1.CCMP = 16667;               // Set interval for 5 ms (16667 clocks @ 3.333 MHz)
    TCB1.INTCTRL = TCB_CAPT_bm;      // CAPT interrupt enable
    TCB1.CTRLA = TCB_ENABLE_bm;      // Enable
}

ISR(TCB1_INT_vect)
{
    /** CODE: Write your code for Ex 10.3 within this ISR. */
    static uint8_t display_side = 0; // 0 = left, 1 = right
    if (display_side)
    {
        // spi_write(right_byte);
    } else {
        // spi_write(left_byte);
    }
    display_side ^= 1; // Toggle between left and right | display_side XOR 1 = toggle
    TCB1.INTFLAGS = TCB_CAPT_bm;
}