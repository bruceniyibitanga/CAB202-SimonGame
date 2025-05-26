#include <stdint.h>

// Initialise UART hardware
void uart_init(void);
// Send a character
void uart_send(char c);
// Receive a character (blocking)
char uart_receive(void);
// Check if data is available
int uart_available(void);


void uart_puts(const char *str);

void uart_putnum(uint16_t num);

void uart_putfloat(float val);
