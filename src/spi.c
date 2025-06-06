
#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"
#include "uart.h"

void spi_init(void){
    // Use alternate SPI pins instead of the default ones
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;
    
    // Configure SPI pins as outputs
    PORTC.DIRSET = PIN0_bm | PIN2_bm; // PC0=SCK, PC2=MOSI
    PORTA.DIRSET = PIN1_bm;           // PA1=Display latch pin
    
    // Start with the latch pin high
    PORTA.OUTSET = PIN1_bm;

    // Setup SPI in master mode
    SPI0.CTRLA = SPI_MASTER_bm;       
    
    // We don't need slave select since we handle our own latch
    SPI0.CTRLB = SPI_SSD_bm;          
    
    // Enable SPI interrupts so we can pulse the latch
    SPI0.INTCTRL = SPI_IE_bm;         
    
    // Turn on SPI
    SPI0.CTRLA |= SPI_ENABLE_bm;
}

void spi_write(uint8_t b){
    SPI0.DATA = b;
}

ISR(SPI0_INT_vect) {
    // Pulse the latch to update the display - this happens after SPI finishes
    PORTA.OUTCLR = PIN1_bm;
    PORTA.OUTSET = PIN1_bm; 
    // Clear the interrupt flag
    SPI0.INTFLAGS = SPI_IF_bm;
}