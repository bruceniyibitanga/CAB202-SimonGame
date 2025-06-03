#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include "buzzer.h"
#include "simon.h"

// ----------------------  INITIALISATION  ----------------------

void uart_init(void) {

    USART0.BAUD = 1389;                           // 9600 baud @ 3.333 MHz
    USART0.CTRLA = USART_RXCIE_bm;
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm; // Enable Tx/Rx

}

// ----------------------  UART SEND FUNCTIONS  ----------------------

void uart_send(char c) {
    // Send character over UART (polling)
    while (!(USART0.STATUS & USART_DREIF_bm)); // Wait for TXDATA empty
    USART0.TXDATAL = c;
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

// Helper for compatibility with simon.c
void uart_send_str(const char* str) {
    uart_puts(str);
}

// ----------------------  MAIN UART LOGIC  ----------------------

// Base frequencies for student number 32
#define BASE_FREQ_EHIGH 324 // S1
#define BASE_FREQ_CSHARP 272 // S2
#define BASE_FREQ_A 432 // S3
#define BASE_FREQ_ELOW 162 // S4

// Current frequencies
volatile uint16_t current_freq_ehigh = BASE_FREQ_EHIGH;
volatile uint16_t current_freq_csharp = BASE_FREQ_CSHARP;
volatile uint16_t current_freq_a = BASE_FREQ_A;
volatile uint16_t current_freq_elow = BASE_FREQ_ELOW;

// State tracking
typedef enum
{
    AWAITING_COMMAND,
    AWAITING_PAYLOAD,
    AWAITING_SEED
} Serial_State;

volatile uint8_t uart_play = 0;
volatile uint8_t uart_stop = 0;
volatile uint8_t uart_reset = 0;
volatile uint32_t new_seed = 0;
volatile uint8_t update_seed = 0;

// Name entry buffer for characters not processed by game commands
#define NAME_ENTRY_BUFFER_SIZE 32
volatile char name_entry_char_buffer[NAME_ENTRY_BUFFER_SIZE];
volatile uint8_t name_entry_buffer_head = 0;
volatile uint8_t name_entry_buffer_tail = 0;
volatile uint8_t name_entry_mode = 0; // 1 when in name entry mode

// Forward declarations for state preservation functions
void save_uart_state(void);
void restore_uart_state(void);

// UART input helpers for simon.c
int uart_rx_available(void) {
    // Check if data is available in name entry buffer
    return name_entry_buffer_head != name_entry_buffer_tail;
}

char uart_receive(void) {
    // Non-blocking receive from name entry buffer
    if (name_entry_buffer_head != name_entry_buffer_tail) {
        char c = name_entry_char_buffer[name_entry_buffer_tail];
        name_entry_buffer_tail = (name_entry_buffer_tail + 1) % NAME_ENTRY_BUFFER_SIZE;
        return c;
    }
    return 0; // No data available
}

// Enable name entry mode - UART input goes to buffer instead of game commands
void uart_enable_name_entry(void) {
    // Save current UART state if we're in the middle of seed entry
    save_uart_state();
    
    name_entry_mode = 1;
    name_entry_buffer_head = 0;
    name_entry_buffer_tail = 0;
}

// Disable name entry mode - UART input processes game commands normally
void uart_disable_name_entry(void) {
    name_entry_mode = 0;
    
    // Restore UART state if we were in the middle of seed entry
    restore_uart_state();
}

// Frequency management functions
static void increase_frequencies(void){
    // Check if any frequency would exceed 20kHz
    if (current_freq_ehigh > 10000 || current_freq_csharp > 10000 ||
        current_freq_a > 10000 || current_freq_elow > 10000)
    {
        return;
    }

    // Double all frequencies
    current_freq_ehigh = current_freq_ehigh << 1;
    current_freq_csharp = current_freq_csharp << 1;
    current_freq_a = current_freq_a << 1;
    current_freq_elow = current_freq_elow << 1;
}

static void decrease_frequencies(void){
    // Check if any frequency would go below 20Hz
    if (current_freq_ehigh < 40 || current_freq_csharp < 40 ||
        current_freq_a < 40 || current_freq_elow < 40)
    {
        return;
    }

    // Halve all frequencies
    current_freq_ehigh = current_freq_ehigh >> 1;
    current_freq_csharp = current_freq_csharp >> 1;
    current_freq_a = current_freq_a >> 1;
    current_freq_elow = current_freq_elow >> 1;
}

static void update_buzzer_frequencies(void){
    // Update the buzzer frequency based on the current button being played
    extern uint8_t current_button_playing;
    extern volatile uint16_t current_freq;
    
    // Only update if a tone is currently playing
    if (current_button_playing == 0) return;
    
    uint16_t new_freq = 0;
    switch(current_button_playing) {
        case 1:
            new_freq = current_freq_ehigh;
            break;
        case 2:
            new_freq = current_freq_csharp;
            break;
        case 3:
            new_freq = current_freq_a;
            break;
        case 4:
            new_freq = current_freq_elow;
            break;
        default:
            return; // No valid button
    }
      // Update the frequency if it's different
    if (new_freq != current_freq && new_freq > 0) {
        current_freq = new_freq;
        uint32_t period = F_CPU / new_freq;
        TCA0.SINGLE.PERBUF = period;
        TCA0.SINGLE.CMP0BUF = period >> 1;  // 50% duty cycle
    }
}

static void reset_frequencies(void){
    current_freq_ehigh = BASE_FREQ_EHIGH;
    current_freq_csharp = BASE_FREQ_CSHARP;
    current_freq_a = BASE_FREQ_A;
    current_freq_elow = BASE_FREQ_ELOW;
}

static uint8_t hexchar_to_int(char c){
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return 10 + c - 'a';
    else
        return 16; // Invalid
}

// UART button flag for simon_task
volatile uint8_t uart_button_flag = 0;

// UART state variables (moved outside ISR for state preservation)
static Serial_State SERIAL_STATE = AWAITING_COMMAND;
static uint8_t chars_received = 0;
static uint32_t seed_value = 0;

// State preservation variables for when name entry interrupts seed entry
static Serial_State saved_serial_state = AWAITING_COMMAND;
static uint8_t saved_chars_received = 0;
static uint32_t saved_seed_value = 0;

ISR(USART0_RXC_vect)
{
    char rx_data = USART0.RXDATAL;

    // If in name entry mode, buffer the character instead of processing commands
    if (name_entry_mode) {
        uint8_t next_head = (name_entry_buffer_head + 1) % NAME_ENTRY_BUFFER_SIZE;
        if (next_head != name_entry_buffer_tail) { // Buffer not full
            name_entry_char_buffer[name_entry_buffer_head] = rx_data;
            name_entry_buffer_head = next_head;
        }
        return; // Don't process as game command
    }

    switch (SERIAL_STATE)
    {
    case AWAITING_COMMAND:
        // Gameplay inputs - each key maps to the corresponding tone (0-3)
        if (rx_data == '1' || rx_data == 'q') {
            uart_button_flag = 1;
        }
        else if (rx_data == '2' || rx_data == 'w') {
            uart_button_flag = 2;
        }
        else if (rx_data == '3' || rx_data == 'e') {
            uart_button_flag = 3;
        }
        else if (rx_data == '4' || rx_data == 'r') {
            uart_button_flag = 4;
        }        // Frequency control
        else if (rx_data == ',' || rx_data == 'k') {
            increase_frequencies();
            update_buzzer_frequencies();
            // If a tone is currently playing from buzzer.c, update it too
            extern void update_current_tone_frequency(void);
            update_current_tone_frequency();
        }
        else if (rx_data == '.' || rx_data == 'l') {
            decrease_frequencies();
            update_buzzer_frequencies();
            // If a tone is currently playing from buzzer.c, update it too
            extern void update_current_tone_frequency(void);
            update_current_tone_frequency();
        }
        // Reset and seed
        else if (rx_data == '0' || rx_data == 'p')
        {
            reset_frequencies();
            uart_reset = 1;
        }
        else if (rx_data == '9' || rx_data == 'o')
        {
            chars_received = 0;
            seed_value = 0;
            SERIAL_STATE = AWAITING_SEED;
        }
        // Handle delay command
        else if (rx_data == 'd')
        {
            chars_received = 0;
            SERIAL_STATE = AWAITING_PAYLOAD;
        }
        // Add UART command to print high scores (e.g. 'h')
        else if (rx_data == 'h') {
            uart_print_high_scores();
        }
        break;

    case AWAITING_PAYLOAD:
        {
            
        }
        break;

    case AWAITING_SEED:
        {
            uint8_t parsed_result = hexchar_to_int(rx_data);
            if (parsed_result != 16) {
                seed_value = (seed_value << 4) | parsed_result;
                chars_received++;
                
                if (chars_received == 8) {
                    new_seed = seed_value;
                    update_seed = 1;
                    SERIAL_STATE = AWAITING_COMMAND;
                }
            } else {
                // Invalid hex digit, cancel seed update
                SERIAL_STATE = AWAITING_COMMAND;
            }
        }
        break;

    default:
        SERIAL_STATE = AWAITING_COMMAND;
        break;
    }
}

// State preservation functions for SEED entry during name entry interruption
void save_uart_state(void) {
    saved_serial_state = SERIAL_STATE;
    saved_chars_received = chars_received;
    saved_seed_value = seed_value;
}

void restore_uart_state(void) {
    SERIAL_STATE = saved_serial_state;
    chars_received = saved_chars_received;
    seed_value = saved_seed_value;
}