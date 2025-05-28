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
    
    // Additional delay flag
    uint8_t waiting_extra_delay = 0;

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
                waiting_extra_delay = 0;
            }
            else if (!pb_released)
            {
                // Check for button release
                if (pb_rising_edge & PIN4_bm && pb_current == 1)
                {
                    pb_released = 1;
                    // Only start extra delay if we've already played for minimum time
                    if (elapsed_time_in_milliseconds >= playback_delay)
                    {
                        prepare_delay(); // Reset timer for the extra delay
                        waiting_extra_delay = 1;
                    }
                }
                else if (pb_rising_edge & PIN5_bm && pb_current == 2)
                {
                    pb_released = 1;
                    if (elapsed_time_in_milliseconds >= playback_delay)
                    {
                        prepare_delay();
                        waiting_extra_delay = 1;
                    }
                }
                else if (pb_rising_edge & PIN6_bm && pb_current == 3)
                {
                    pb_released = 1;
                    if (elapsed_time_in_milliseconds >= playback_delay)
                    {
                        prepare_delay();
                        waiting_extra_delay = 1;
                    }
                }
                else if (pb_rising_edge & PIN7_bm && pb_current == 4)
                {
                    pb_released = 1;
                    if (elapsed_time_in_milliseconds >= playback_delay)
                    {
                        prepare_delay();
                        waiting_extra_delay = 1;
                    }
                }
            }
            else // Button is released
            {
                if (waiting_extra_delay)
                {
                    // In extra delay period after long press
                    if (elapsed_time_in_milliseconds >= (playback_delay / 2))
                    {
                        stop_tone();
                        TONE_STATE = PAUSED;
                        waiting_extra_delay = 0;
                    }
                }
                else if (elapsed_time_in_milliseconds >= playback_delay)
                {
                    // Normal delay period finished
                    stop_tone();
                    TONE_STATE = PAUSED;
                }
                // If neither waiting_extra_delay nor playback_delay reached,
                // continue playing the tone
            }
            break;
        default:
            TONE_STATE = PAUSED;
            stop_tone();
            waiting_extra_delay = 0;
            break;
        }
    }
}