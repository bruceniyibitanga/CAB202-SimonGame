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

// ----------------------  INITIALISATION  -------------------------------
void timer_init(void)
{
    // Initialize TCB0 for 1ms general timing if available
    // Used for the Simon game timing requirements
    // TCB0: 1ms interrupt for millisecond timing
    // At 3.333MHz: 1ms = 3,333 cycles

    // TCB0.CCMP = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = 3333 - 1;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_ENABLE_bm;

    // TCB1: 5ms interrupt for button debouncing and display multiplexing
    // At 3.333MHz: 5ms = 16,665 cycles (using 16667 for slight over-sampling)
    
    TCB1.CTRLB = TCB_CNTMODE_INT_gc;  // Configure TCB1 in periodic interrupt mode
    // Set interval for 5ms (-1 because counter starts at 0)
    TCB1.CCMP = 16667 - 1;            
    TCB1.INTCTRL = TCB_CAPT_bm;       // Enable CAPT interrupt
    TCB1.CTRLA = TCB_ENABLE_bm;       // Enable timer
}
// ----------------------  DELAY RESET  --------------------------------
void prepare_delay(void)
{
    elapsed_time_in_milliseconds = 0; // Reset the elapsed time counter
}

// ----------------------  1ms TIMER INTERRUPT  ------------------------

ISR(TCB0_INT_vect)
{
    // Increment the elapsed time counter
    elapsed_time_in_milliseconds++;
    // Update the frequency of the buzzer to the current_freq only if a tone is playing
    extern volatile uint8_t is_playing;
    if (is_playing && current_freq > 0) {
        TCA0.SINGLE.PERBUF = (F_CPU / current_freq); // Update buzzer frequency
    }
    // Clear interrupt flags
    TCB0.INTFLAGS = TCB_CAPT_bm; 
}

// ----------------------  PUSH BUTTON HANDLING  ----------------------

// TCB1 ISR - Handles button debouncing and display multiplexing every 5ms
ISR(TCB1_INT_vect)
{    // Button debouncing logic
    uint8_t pb_sample = PORTA.IN;
    uint8_t pb_changed = pb_sample ^ pb_debounced_state;
    
    // Two-step debouncing algorithm
    count1 = (count1 ^ count0) & pb_changed;
    count0 = ~count0 & pb_changed;
    pb_debounced_state ^= (count1 & count0) | (pb_changed & pb_debounced_state);
    
    // Update display
    swap_display_digit();
    
    // Clear interrupt flag
    TCB1.INTFLAGS = TCB_CAPT_bm;
}