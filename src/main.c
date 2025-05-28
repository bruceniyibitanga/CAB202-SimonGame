#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "buzzer.h"
#include "initialisation.h"
#include "timer.h"
#include "uart.h"
#include "button.h"

void state_machine(void);

// Define states for the Simon game
typedef enum {
    SIMON_GENERATE,
    SIMON_PLAY_ON,
    SIMON_PLAY_OFF,
    AWAITING_INPUT,
    HANDLE_INPUT,
    EVALUATE_INPUT,
    SUCCESS,
    FAIL,
    DISP_SCORE
} SimonState;

typedef enum{
    PAUSED,
    PLAYING
} ToneState;


int main(void)
{
    cli();
    // Call your initialisation functions here
    system_init();
    buttons_init();
    peripherals_init();
    // simon_init();
    sei();

    state_machine();

    // The program should not reach this point
    while (1)
        ;
}

void state_machine(void)
{
    ToneState TONE_STATE = PAUSED;

    // Pushbutton states
    uint8_t pb_state_prev = 0xFF;
    uint8_t pb_state_curr = 0xFF;

    uint8_t pb_current = 0;

    // Pushbutton flags
    uint8_t pb_falling_edge, pb_rising_edge, pb_released = 0;

    while (1)
    {
        // Save state from previous iteration
        pb_state_prev = pb_state_curr;
        // Read current state
        pb_state_curr = pb_debounced_state;

        // Find edges
        pb_falling_edge = (pb_state_prev ^ pb_state_curr) & pb_state_prev;
        pb_rising_edge = (pb_state_prev ^ pb_state_curr) & pb_state_curr;


        // State machine
        switch (TONE_STATE)
        {
        case PAUSED:
            // Wait for press
            if (pb_falling_edge & (PIN4_bm | PIN5_bm | PIN6_bm | PIN7_bm))
            {
                if (pb_falling_edge & PIN4_bm)
                    pb_current = 1;
                else if (pb_falling_edge & PIN5_bm)
                    pb_current = 2;
                else if (pb_falling_edge & PIN6_bm)
                    pb_current = 3;
                else if (pb_falling_edge & PIN7_bm)
                    pb_current = 4;

                play_tone(pb_current - 1);

                // Update flags
                pb_released = 0;
                prepare_delay();

                // State transition
                TONE_STATE = PLAYING;
            }
            else if (uart_play == 1)
            {
                // Play whatever was previously selected
                play_selected_tone();
                prepare_delay();

                // Update flags
                pb_released = 1;
                uart_play = 0;

                // State transition
                TONE_STATE= PLAYING;
            }
            break;
        case PLAYING:
            if (uart_stop == 1)
            {
                stop_tone();
                TONE_STATE = PAUSED;
                uart_stop = 0;
            }
            else if (!pb_released)
            {
                // Wait for release
                if (pb_rising_edge & PIN4_bm && pb_current == 1)
                    pb_released = 1;
                else if (pb_rising_edge & PIN5_bm && pb_current == 2)
                    pb_released = 1;
                else if (pb_rising_edge & PIN6_bm && pb_current == 3)
                    pb_released = 1;
                else if (pb_rising_edge & PIN7_bm && pb_current == 4)
                    pb_released = 1;
            }
            else
            {
                // Stop if elapsed time is greater than playback time
                if (elapsed_time_in_milliseconds >= playback_delay)
                {
                    stop_tone();
                    TONE_STATE = PAUSED;
                }
            }
            break;
        default:
            TONE_STATE = PAUSED;
            stop_tone();
            break;
        }
    }
}