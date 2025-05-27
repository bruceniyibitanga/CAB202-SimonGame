#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"

void spi_init(void){

    // // Configures pins PA1, PC0, and PC2, to drive the 7-segment display
    // display_init();
    // // Drives DISP EN net HIGH
    // display_on();
    
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc; // Set SPI0 to use alternative pins PORTC.DIRSET= (PIN0_bm |PIN2_bm); // Set SCK PC0 and MOSI PC2 as output
    PORTA.DIRSET = PIN1_bm; // Set DISP LATCH pin as OUTPUT
  
    SPI0.CTRLA = SPI_MASTER_bm; // Master, /4 prescaler, MSB first
    SPI0.CTRLB = SPI_SSD_bm; // Mode 0, client select disabled, unbuffered mode
    SPI0.INTCTRL = SPI_IE_bm; // Enable SPI interrupt
    SPI0.CTRLA |= SPI_ENABLE_bm; // Enable SPI0
}

void spi_write(uint8_t b){
    SPI0.DATA = b;
}

ISR(SPI0_INT_vect)
{
    PORTA.OUTCLR = PIN1_bm;
    PORTA.OUTSET = PIN1_bm; // Then set high = rising edge
    SPI0.INTFLAGS = SPI_IE_bm | SPI_IF_bm; // Clear interrupt flag
} 