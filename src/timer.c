#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "display.h"
#include "button.h"
#include "spi.h"
#include "uart.h"
#include "adc.h"
#include "buzzer.h"

volatile uint8_t pb_debounced_state = 0xFF;
static uint8_t count0 = 0;
static uint8_t count1 = 0;

// Timing variables for general use
volatile uint16_t elapsed_time_in_milliseconds = 0;
volatile uint16_t playback_delay = 250; // Default playback delay in milliseconds
// Timer for UART stuff like name entry timeouts
volatile uint32_t uart_input_timer = 0;

// ----------------------  INITIALISATION  -------------------------------
void timer_init(void)
{
    // Set up TCB0 for 1ms timing - used for simon game delays
    // At 3.333MHz: 1ms = 3,333 cycles

    TCB0.CTRLB = TCB_CNTMODE_INT_gc;  // Periodic interrupt mode
    TCB0.CCMP = 3333 - 1;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_ENABLE_bm;

    
    TCB1.CTRLB = TCB_CNTMODE_INT_gc;  // Periodic interrupt mode
    TCB1.CCMP = 16667 - 1;            // 5ms interval
    TCB1.INTCTRL = TCB_CAPT_bm;       // Enable interrupt
    TCB1.CTRLA = TCB_ENABLE_bm;       // Start the timer
}
// ----------------------  DELAY RESET  --------------------------------
void prepare_delay(void)
{
    elapsed_time_in_milliseconds = 0; // Reset the elapsed time counter
}

// ----------------------  1ms TIMER INTERRUPT  ------------------------

ISR(TCB0_INT_vect)
{
    // Count up milliseconds for simon game timing
    elapsed_time_in_milliseconds++;
    // Also count for UART name entry timeouts
    uart_input_timer++;
    
    // Clear the interrupt flag
    TCB0.INTFLAGS = TCB_CAPT_bm; 
}

// ----------------------  PUSH BUTTON HANDLING  ----------------------

// TCB1 ISR - Does button debouncing and switches display digits every 5ms
ISR(TCB1_INT_vect)
{    
    uint8_t pb_sample = PORTA.IN;
    uint8_t pb_changed = pb_sample ^ pb_debounced_state;
    
    count1 = (count1 ^ count0) & pb_changed;
    count0 = ~count0 & pb_changed;
    pb_debounced_state ^= (count1 & count0);    
    
    // Switch between left and right display digits
    swap_display_digit();
    
    // Clear interrupt flag
    TCB1.INTFLAGS = TCB_CAPT_bm;
}