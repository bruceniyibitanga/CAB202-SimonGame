#include "uart.h"
#include <stdint.h>
#include <avr/interrupt.h>

void uart_init(void) {

    // Configure UART registers (baud rate, enable TX/RX)
    PORTB.DIRSET = PIN2_bm;
    USART0.BAUD = 1389;                           // 9600 baud @ 3.333 MHz
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm; // Enable Tx/Rx
}

void uart_send(char c) {
    // Send character over UART (polling)
    while (!(USART0.STATUS & USART_DREIF_bm)); // Wait for TXDATA empty
    USART0.TXDATAL = c;
}

char uart_receive(void) {
    // Receive character over UART (blocking)
    while (!(USART0.STATUS & USART_RXCIF_bm)); // Wait for data
    return USART0.RXDATAL;
}

int uart_available(void) {
    return (USART0.STATUS & USART_RXCIF_bm);
}


// Helper functions to help debugging
void uart_puts(const char *str) {
    while (*str) {
        uart_send(*str++);
    }
}

void uart_putnum(uint16_t num) {
    char buf[6];
    itoa(num, buf, 10);  // Convert to decimal string
    uart_puts(buf);
}

void uart_putfloat(float val) {
    int whole = (int)val;
    int frac = (int)((val - whole) * 100);  // 2 decimal places
    uart_putnum(whole);
    uart_send('.');
    if (frac < 10) uart_send('0');  // Pad small fractions like 0.04
    uart_putnum(frac);
}
