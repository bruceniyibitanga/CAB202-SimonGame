#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/io.h>
#include <stdio.h>
#include "timer.h"
#include "buzzer.h"

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

// ----------------------  MAIN UART LOGIC  ----------------------

// Base frequencies for student number 32
#define BASE_FREQ_A 128
#define BASE_FREQ_CSHARP 108
#define BASE_FREQ_EHIGH 152
#define BASE_FREQ_ELOW 51

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

// Buffer for transmitting UART strings
static char tx_buffer[32];

static void uart_transmit_string(const char* str) {
    // Wait for any previous transmission to complete
    while (!(USART0.STATUS & USART_DREIF_bm));
    
    // Send each character
    while (*str) {
        USART0.TXDATAL = *str++;
        while (!(USART0.STATUS & USART_DREIF_bm));
    }
}



// Frequency management functions
static void increase_frequencies(void)
{
    // Check if any frequency would exceed 20kHz
    if (current_freq_ehigh > 10000 || current_freq_csharp > 10000 ||
        current_freq_a > 10000 || current_freq_elow > 10000)
    {
        return;
    }

    // Double all frequencies
    current_freq_ehigh *= 2;
    current_freq_csharp *= 2;
    current_freq_a *= 2;
    current_freq_elow *= 2;
}

static void decrease_frequencies(void)
{
    // Check if any frequency would go below 20Hz
    if (current_freq_ehigh < 40 || current_freq_csharp < 40 ||
        current_freq_a < 40 || current_freq_elow < 40)
    {
        return;
    }

    // Halve all frequencies
    current_freq_ehigh /= 2;
    current_freq_csharp /= 2;
    current_freq_a /= 2;
    current_freq_elow /= 2;
}

static void reset_frequencies(void)
{
    current_freq_ehigh = BASE_FREQ_EHIGH;
    current_freq_csharp = BASE_FREQ_CSHARP;
    current_freq_a = BASE_FREQ_A;
    current_freq_elow = BASE_FREQ_ELOW;
}

static uint8_t hexchar_to_int(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return 10 + c - 'a';
    else
        return 16; // Invalid
}

ISR(USART0_RXC_vect)
{   static Serial_State SERIAL_STATE = AWAITING_COMMAND;
    static uint8_t chars_received = 0;
    static uint16_t payload = 0;
    static uint8_t payload_valid = 1;
    static uint32_t seed_value = 0;

    char rx_data = USART0.RXDATAL;

    switch (SERIAL_STATE)
    {
    case AWAITING_COMMAND:
        // Gameplay inputs - each key maps to the corresponding tone (0-3)
        if (rx_data == '1' || rx_data == 'q') {
            play_tone(0);
            prepare_delay();
        }
        else if (rx_data == '2' || rx_data == 'w') {
            play_tone(1);
            prepare_delay();
        }
        else if (rx_data == '3' || rx_data == 'e') {
            play_tone(2);
            prepare_delay();
        }
        else if (rx_data == '4' || rx_data == 'r') {
            play_tone(3);
            prepare_delay();
        }
        // Frequency control
        else if (rx_data == ',' || rx_data == 'k')
            decrease_frequencies();
        else if (rx_data == '.' || rx_data == 'l')
            increase_frequencies();
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
            payload_valid = 1;
            chars_received = 0;
            payload = 0;
            SERIAL_STATE = AWAITING_PAYLOAD;
        }
        break;

    case AWAITING_PAYLOAD:
        {
            uint8_t parsed_result = hexchar_to_int(rx_data);
            if (parsed_result != 16)
                payload = (payload << 4) | parsed_result;
            else
                payload_valid = 0;

            if (++chars_received == 4)
            {
                if (payload_valid)
                    playback_delay = payload;
                SERIAL_STATE = AWAITING_COMMAND;
            }
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
