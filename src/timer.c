#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "display.h"
#include "button.h"
#include "spi.h"
#include "uart.h"

volatile uint8_t pb_debounced_state = 0xFF;
static uint8_t count0 = 0;
static uint8_t count1 = 0;

void timer_init(void)
{
    TCB1.CTRLB = TCB_CNTMODE_INT_gc; // Configure TCB1 in periodic interrupt mode
    TCB1.CCMP = 16667;                // Set interval for 5 ms (16667 clocks @ 3.333 MHz)
    TCB1.INTCTRL = TCB_CAPT_bm;      // CAPT interrupt enable
    TCB1.CTRLA = TCB_ENABLE_bm;  
}

ISR(TCB1_INT_vect)
{
    // Handle button debouncing
    uint8_t pb_sample = PORTA.IN;
    uint8_t pb_changed = pb_sample ^ pb_debounced_state;
    count1 = (count1^count0) & pb_changed;
    count0 = ~count0 & pb_changed;
    pb_debounced_state ^= (count1 & count0) | (pb_changed & pb_debounced_state);

    static uint8_t display_side = 0;
    if(display_side)
    {
        spi_write(right_byte);
    }
    else 
    {
        spi_write(left_byte);
    }
    display_side ^= 1;

    TCB1.INTFLAGS = TCB_CAPT_bm;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         