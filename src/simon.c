#include "simon.h"
#include "display.h"
#include "buzzer.h"
#include "display_macros.h"
#include "button.h"
#include "button_macros.h"
#include "timer.h"
#include <stdint.h>
#include <stdbool.h>

// Define display patterns for the bars
#define DISP_BAR_LEFT (DISP_SEG_E & DISP_SEG_F)   // Left segments
#define DISP_BAR_RIGHT (DISP_SEG_B & DISP_SEG_C)  // Right segments

// Game states and variables
static simon_state_t state = SIMON_GENERATE;
static uint8_t sequence_length = 4;  // Start with length 4 as per requirements
static uint8_t sequence_index = 0;
static uint8_t current_step = 0;

// Store sequence steps
static uint8_t sequence[32];  // Array to store the sequence

// LFSR configuration
#define LFSR_MASK 0xE2025CAB
#define INITIAL_SEED 0x12236632  // Student number

// LFSR state
static uint32_t lfsr_state = INITIAL_SEED;

// Function to display a two-digit number
void display_two_digit_number(uint8_t num) {
    uint8_t tens = num / 10;
    uint8_t ones = num % 10;
    
    uint8_t tens_pattern = DIGIT_0;  // Default to 0
    uint8_t ones_pattern = DIGIT_0;  // Default to 0
    
    // Convert digits to patterns
    switch(tens) {
        case 1: tens_pattern = DIGIT_1; break;
        case 2: tens_pattern = DIGIT_2; break;
        case 3: tens_pattern = DIGIT_3; break;
        case 4: tens_pattern = DIGIT_4; break;
        case 5: tens_pattern = DIGIT_5; break;
        case 6: tens_pattern = DIGIT_6; break;
        case 7: tens_pattern = DIGIT_7; break;
        case 8: tens_pattern = DIGIT_8; break;
        case 9: tens_pattern = DIGIT_9; break;
    }
    
    switch(ones) {
        case 1: ones_pattern = DIGIT_1; break;
        case 2: ones_pattern = DIGIT_2; break;
        case 3: ones_pattern = DIGIT_3; break;
        case 4: ones_pattern = DIGIT_4; break;
        case 5: ones_pattern = DIGIT_5; break;
        case 6: ones_pattern = DIGIT_6; break;
        case 7: ones_pattern = DIGIT_7; break;
        case 8: ones_pattern = DIGIT_8; break;
        case 9: ones_pattern = DIGIT_9; break;
    }
    
    update_display(tens_pattern, ones_pattern);
}

// LFSR state management functions
// Get next step from LFSR (0-3)
static uint8_t get_next_step(void) {
    uint8_t bit = lfsr_state & 1;
    lfsr_state = lfsr_state >> 1;
    if (bit) {
        lfsr_state = lfsr_state ^ LFSR_MASK;
    }
    return lfsr_state & 0b11;
}

// Reset LFSR to initial state
static void reset_lfsr(void) {
    lfsr_state = INITIAL_SEED;
}

// Display pattern and play tone for a step
static void display_step_pattern(uint8_t step) {
    switch(step) {
        case 0:  // E(high) - segments EF on left digit
            update_display(DISP_BAR_LEFT, DISP_OFF);
            play_tone(0);
            break;
        case 1:  // C# - segments BC on left digit
            update_display(DISP_BAR_RIGHT, DISP_OFF);
            play_tone(1);
            break;
        case 2:  // A - segments EF on right digit
            update_display(DISP_OFF, DISP_BAR_LEFT);
            play_tone(2);
            break;
        case 3:  // E(low) - segments BC on right digit
            update_display(DISP_OFF, DISP_BAR_RIGHT);
            play_tone(3);
            break;
    }
}

// Convert button press to step number (0-3)
static uint8_t get_button_step(void) {
    if (button_pressed(PIN4_bm)) return 0;  // S1 (left)
    if (button_pressed(PIN5_bm)) return 1;  // S2 (right)
    if (button_pressed(PIN6_bm)) return 2;  // S3 (up)
    if (button_pressed(PIN7_bm)) return 3;  // S4 (down)
    return 0xFF;  // No button pressed
}

// Get currently pressed button (for long press handling)
static uint8_t get_current_button(void) {
    uint8_t button_state = PORTA.IN;
    if (!(button_state & PIN4_bm)) return 0;
    if (!(button_state & PIN5_bm)) return 1;
    if (!(button_state & PIN6_bm)) return 2;
    if (!(button_state & PIN7_bm)) return 3;
    return 0xFF;
}

// Check if specific button was released
static bool was_button_released(uint8_t step) {
    switch(step) {
        case 0: return button_released(PIN4_bm);
        case 1: return button_released(PIN5_bm);
        case 2: return button_released(PIN6_bm);
        case 3: return button_released(PIN7_bm);
        default: return false;
    }
}

void simon_init(void) {
    state = SIMON_GENERATE;
    sequence_length = 4;  // Start with 4 steps
    sequence_index = 0;
    reset_lfsr();
    prepare_delay();
}

void simon_task(void) {
    // Update button states at the start of each task iteration
    update_button_states();

    switch (state) {
        case SIMON_GENERATE:
            if (sequence_index <= sequence_length - 1) {
                sequence[sequence_index] = get_next_step();
                display_step_pattern(sequence[sequence_index]);
                prepare_delay();
                state = SIMON_PLAY_ON;
            } else {
                sequence_index = 0;
                state = AWAITING_INPUT;
            }
            break;

        case SIMON_PLAY_ON:
            if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
                update_display(DISP_OFF, DISP_OFF);
                stop_tone();
                prepare_delay();
                state = SIMON_PLAY_OFF;
            }
            break;

        case SIMON_PLAY_OFF:
            if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY/2) {
                sequence_index++;
                state = SIMON_GENERATE;
            }
            break;

        case AWAITING_INPUT:
            {
                uint8_t pressed_step = get_button_step();
                if (pressed_step != 0xFF) {  // Valid button press
                    display_step_pattern(pressed_step);
                    pb_current = pressed_step + 1;  // Save current button
                    pb_released = 0;
                    prepare_delay();
                    state = HANDLE_INPUT;
                }
            }
            break;

        case HANDLE_INPUT:
            if (was_button_released(pb_current - 1) && elapsed_time_in_milliseconds >= PLAYBACK_DELAY/2) {
                update_display(DISP_OFF, DISP_OFF);
                stop_tone();
                pb_released = 1;

                // Check if the pressed button matched the sequence
                if (pb_current - 1 == sequence[sequence_index]) {  // Correct input
                    if (sequence_index < sequence_length - 1) {
                        sequence_index++;
                        state = AWAITING_INPUT;
                    } else {  // Completed sequence successfully
                        update_display(DISP_SUCCESS, DISP_SUCCESS);
                        prepare_delay();
                        sequence_index = 0;
                        sequence_length++;
                        state = SUCCESS;
                    }
                } else {  // Wrong input
                    update_display(DISP_FAIL, DISP_FAIL);
                    prepare_delay();
                    state = FAIL;
                }
            }
            // Handle long press
            else if (!pb_released && elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
                uint8_t current = get_current_button();
                if (current == pb_current - 1) {  // Button still held
                    // Keep playing tone and showing pattern
                    display_step_pattern(current);
                }
            }
            break;

        case SUCCESS:
            if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
                update_display(DISP_OFF, DISP_OFF);
                reset_lfsr();  // Reset LFSR for next sequence
                state = SIMON_GENERATE;
            }
            break;

        case FAIL:
            if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
                state = DISP_SCORE;
                prepare_delay();
            }
            break;

        case DISP_SCORE:
            if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
                // Show final score
                display_two_digit_number(sequence_length - 1);
                if (elapsed_time_in_milliseconds >= 2 * PLAYBACK_DELAY) {
                    sequence_length = 4;  // Reset to initial length
                    sequence_index = 0;
                    reset_lfsr();
                    state = SIMON_GENERATE;
                }
            }
            break;
    }
}
