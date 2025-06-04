#include <stdint.h>
// Control flags
extern volatile uint8_t uart_play;
extern volatile uint8_t uart_stop;
extern volatile uint8_t uart_reset;
extern volatile uint32_t new_seed;
extern volatile uint8_t update_seed;
extern volatile uint8_t uart_button_flag;

// Game reporting
void report_score(uint16_t score, uint8_t is_success);

void uart_puts(const char *str);

void uart_send(char c);

void uart_init(void);

void uart_putnum(uint16_t num);

extern void update_buzzer_frequencies(uint8_t button_pressed);

extern volatile uint8_t uart_button_flag;

void uart_send_str(const char* str);

int uart_rx_available(void);

char uart_receive(void);

void uart_enable_name_entry(void);

void uart_disable_name_entry(void);

void uart_print_high_scores(void);