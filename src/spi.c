#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"
#include "uart.h"

void spi_init(void){
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;
    PORTC.DIRSET = PIN0_bm | PIN2_bm; // SCK and MOSI
    PORTA.DIRSET = PIN1_bm;           // DISP LATCH pin

    SPI0.CTRLA = SPI_MASTER_bm;       // Master mode
    SPI0.CTRLB = SPI_SSD_bm;          // Disable SS
    SPI0.INTCTRL = SPI_IE_bm;         // Enable interrupt
    SPI0.CTRLA |= SPI_ENABLE_bm;      // Enable SPI
}




void spi_write(uint8_t b){
    SPI0.DATA = b;
}

ISR(SPI0_INT_vect) {
    PORTA.OUTCLR = PIN1_bm;
    PORTA.OUTSET = PIN1_bm;
    SPI0.INTFLAGS = SPI_IF_bm; // Correct: only clear IF
}

