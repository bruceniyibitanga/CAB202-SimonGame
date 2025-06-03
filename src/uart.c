
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

// ----------------------  MAIN UART LOGIC  ----------------------

// Base frequencies for student number 32
#define BASE_FREQ_EHIGH 324 // S1
#define BASE_FREQ_CSHARP 272 // S2
#define BASE_FREQ_A 432 // S3
#define BASE_FREQ_ELOW 162 // S4

// Current frequencies
volatile uint16_t current_freq_a = BASE_FREQ_A;
volatile uint16_t current_freq_csharp = BASE_FREQ_CSHARP;
volatile uint16_t current_freq_ehigh = BASE_FREQ_EHIGH;
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
        uint16_t period = F_CPU / new_freq;
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

ISR(USART0_RXC_vect)
{   static Serial_State SERIAL_STATE = AWAITING_COMMAND;
    static uint8_t chars_received = 0;
    static uint32_t seed_value = 0;

    char rx_data = USART0.RXDATAL;

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
            // uart_print_high_scores();
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