
#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"
#include "uart.h"

void spi_init(void){
    // Route SPI to alternate pins (PC0=SCK, PC2=MOSI)
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;
    
    // Set up SPI pins as outputs
    PORTC.DIRSET = PIN0_bm | PIN2_bm; // PC0=SCK, PC2=MOSI
    PORTA.DIRSET = PIN1_bm;           // PA1=DISP LATCH
    
    // Initialize DISP LATCH high
    PORTA.OUTSET = PIN1_bm;

    // Configure SPI:
    // - Master mode
    // - MSB first (default)
    // - Mode 0 (default: clock idle low, sample on leading edge)
    SPI0.CTRLA = SPI_MASTER_bm;       
    
    // Disable Slave Select since we're using our own latch
    SPI0.CTRLB = SPI_SSD_bm;          
    
    // Enable SPI interrupt
    SPI0.INTCTRL = SPI_IE_bm;         
    
    // Enable SPI
    SPI0.CTRLA |= SPI_ENABLE_bm;      
}

void spi_write(uint8_t b){
    SPI0.DATA = b;
}

ISR(SPI0_INT_vect) {
    //rising edge on DISP_LATCH
    PORTA.OUTCLR = PIN1_bm;
    PORTA.OUTSET = PIN1_bm;  

    SPI0.INTFLAGS = SPI_IF_bm;
}