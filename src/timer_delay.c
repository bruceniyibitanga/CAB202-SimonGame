#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "timer_delay.h"

volatile bool tcb0_done = false;

void delay_timer_init(void) {
    // Configure TCB0 for periodic interrupt mode
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    
    // Use CLK_PER with DIV2 prescaler (3.333MHz / 2 = 1.6665 MHz)
    // This gives us better resolution for timing
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc; // Use DIV2 prescaler
    
    // Enable CAPT interrupt
    TCB0.INTCTRL = TCB_CAPT_bm;
}

void delay_ms_timer(uint16_t ms) {
    if (ms == 0) return;
    
    // Clock frequency after DIV2 prescaler: F_CPU / 2
    // At 3.333 MHz: 3333000 / 2 = 1666500 Hz
    const uint32_t timer_freq = F_CPU / 2;
    
    // Calculate ticks needed for the delay
    // For milliseconds: ticks = (timer_freq * ms) / 1000
    uint32_t total_ticks = ((uint32_t)timer_freq * ms) / 1000UL;
    
    // TCB0 is 16-bit, so we need to handle delays longer than ~39ms in chunks
    while (total_ticks > 0) {
        uint16_t ticks_this_cycle;
        
        if (total_ticks > 65535) {
            ticks_this_cycle = 65535;
        } else {
            ticks_this_cycle = (uint16_t)total_ticks;
        }
        
        // Set up the timer for this chunk
        tcb0_done = false;
        TCB0.CCMP = ticks_this_cycle;
        TCB0.CNT = 0;                    // Reset counter
        TCB0.CTRLA |= TCB_ENABLE_bm;     // Start timer
        
        // Wait for completion
        while (!tcb0_done) {
            // Busy wait - could add sleep mode here if needed
        }
        
        total_ticks -= ticks_this_cycle;
    }
}

ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;     // Clear interrupt flag
    TCB0.CTRLA &= ~TCB_ENABLE_bm;    // Stop timer
    tcb0_done = true;
}