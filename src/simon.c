#include <stdint.h>
#include <stdbool.h>
#include "simon.h"
#include "display.h"
#include "buzzer.h"
#include "display_macros.h"
#include "button.h"
#include "button_macros.h"
#include "timer.h"
#include "adc.h"
#include "uart.h"

// Define display patterns for the bars
#define DISP_BAR_LEFT (DISP_SEG_E & DISP_SEG_F)   // Left segments
#define DISP_BAR_RIGHT (DISP_SEG_B & DISP_SEG_C)  // Right segments

// Game states and variables
static simon_state_t state = SIMON_GENERATE;
static uint8_t sequence_length = 4;  // Start with length 4 as per requirements
static uint8_t sequence_index = 0;
static uint8_t lfsr_pos = 0; // Track current position in LFSR sequence

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

void add_new_sequence_step() {
    sequence[sequence_length] = get_next_step();
    sequence_length++;
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



// Add a waiting_extra_delay flag
static bool waiting_extra_delay = 0;

void simon_init(void) {
    state = SIMON_GENERATE;
    sequence_length = 4;  // Start with 4 steps
    sequence_index = 0;
    lfsr_pos = 0;
    lfsr_state = INITIAL_SEED;
    // Generate the initial 4-step sequence
    for (uint8_t i = 0; i < sequence_length; i++) {
        sequence[i] = get_next_step();
    }
    prepare_delay();
}

void simon_task(void) {
    switch (state) {
        case SIMON_GENERATE: state_generate(); break;
        case SIMON_PLAY_ON: state_play_on(); break;
        case SIMON_PLAY_OFF: state_play_off(); break;
        case AWAITING_INPUT: state_awaiting_input(); break;
        case HANDLE_INPUT: state_handle_input(); break;
        case EVALUATE_INPUT: state_evaluate_input(); break;
        case SUCCESS: state_success(); break;
        case FAIL: state_fail(); break;
        case DISP_SCORE: state_disp_score(); break;
    }
}

// =========================
// State handler functions
// =========================

void state_generate(void) {
    // Only add a new step when we've successfully completed the previous sequence
    if (sequence_index == 0 && sequence_length == 4 - 1 && state != DISP_SCORE) {
        sequence[sequence_length] = get_next_step();
        sequence_length++;
    }
    // Play current step in sequence
    if (sequence_index < sequence_length) {
        display_step_pattern(sequence[sequence_index]);
        prepare_delay();
        state = SIMON_PLAY_ON;
    } else {
        // All steps played, wait for user input
        sequence_index = 0;
        state = AWAITING_INPUT;
        pb_current = 0;
        pb_released = 1;
        waiting_extra_delay = 0;
    }
}

void state_play_on(void) {
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
        update_display(DISP_OFF, DISP_OFF);
        stop_tone();
        prepare_delay();
        state = SIMON_PLAY_OFF;
    }
}

void state_play_off(void) {
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY/2) {
        sequence_index++;
        state = SIMON_GENERATE;
    }
}

void state_awaiting_input(void) {
    // UART input: simulate instant press and release
    if (uart_button_flag) {
        pb_current = uart_button_flag;
        display_step_pattern(pb_current - 1);
        pb_released = 1; // Simulate instant release
        prepare_delay();
        uart_button_flag = 0;
        state = HANDLE_INPUT;
    }
    // Wait for physical button press
    else if (pb_falling_edge & PIN4_bm) {
        pb_current = 1;
        display_step_pattern(0);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    }
    else if (pb_falling_edge & PIN5_bm) {
        pb_current = 2;
        display_step_pattern(1);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    }
    else if (pb_falling_edge & PIN6_bm) {
        pb_current = 3;
        display_step_pattern(2);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    }
    else if (pb_falling_edge & PIN7_bm) {
        pb_current = 4;
        display_step_pattern(3);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    }
}

void state_handle_input(void) {
    uint8_t button_mask = 0;
    switch (pb_current) {
        case 1: button_mask = PIN4_bm; break;
        case 2: button_mask = PIN5_bm; break;
        case 3: button_mask = PIN6_bm; break;
        case 4: button_mask = PIN7_bm; break;
        default: break;
    }
    // If pb_released is already 1 (from UART), skip waiting for physical release
    if (pb_released) {
        if (waiting_extra_delay) {
            if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY >> 1)) {
                stop_tone();
                update_display(DISP_OFF, DISP_OFF);
                waiting_extra_delay = 0;
                if ((pb_current - 1) == sequence[sequence_index]) {
                    if (sequence_index < sequence_length - 1) {
                        sequence_index++;
                        state = AWAITING_INPUT;
                    } else {
                        update_display(DISP_SUCCESS, DISP_SUCCESS);
                        prepare_delay();
                        sequence_index = 0;
                        state = SUCCESS;
                    }
                } else {
                    update_display(DISP_FAIL, DISP_FAIL);
                    prepare_delay();
                    state = FAIL;
                }
            }
        } else if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
            stop_tone();
            update_display(DISP_OFF, DISP_OFF);
            if ((pb_current - 1) == sequence[sequence_index]) {
                if (sequence_index < sequence_length - 1) {
                    sequence_index++;
                    state = AWAITING_INPUT;
                } else {
                    update_display(DISP_SUCCESS, DISP_SUCCESS);
                    prepare_delay();
                    sequence_index = 0;
                    state = SUCCESS;
                }
            } else {
                update_display(DISP_FAIL, DISP_FAIL);
                prepare_delay();
                state = FAIL;
            }
        }
    }
    // Otherwise, wait for physical button release
    else if (!pb_released && (pb_rising_edge & button_mask)) {
        pb_released = 1;
        if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
            prepare_delay();
            waiting_extra_delay = 1;
        }
    }
}

void state_evaluate_input(void) {
    // Input evaluation is handled within HANDLE_INPUT state
    // This case exists to match the state diagram but is not used
    state = AWAITING_INPUT;
}

void state_success(void) {
    // Success state - display success pattern for 1 second then continue
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY * 2) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        add_new_sequence_step(); // Add a new step after a successful round
        lfsr_pos++; // Move LFSR position forward
        sequence_index = 0;
        state = SIMON_GENERATE;
    }
}

void state_fail(void) {
    // Failure state - display failure pattern for 1 second then show score
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY << 1)) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        // On fail, set lfsr_pos to the failed step (sequence_length - 1)
        if (sequence_length > 4) {
            lfsr_pos = sequence_length - 1;
        } else {
            lfsr_pos = 0;
        }
        state = DISP_SCORE;
    }
}

void state_disp_score(void) {
    // Display score (sequence_length - 1) for 3 seconds
    display_two_digit_number(sequence_length - 1);
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY << 3)) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        // Reset game parameters
        sequence_length = 4;
        sequence_index = 0;
        // Reset LFSR to initial seed for new game
        reset_lfsr();
        // Advance LFSR to lfsr_pos
        for (uint8_t i = 0; i < lfsr_pos; i++) {
            get_next_step();
        }
        // Generate new 4-step sequence from current LFSR position
        for (uint8_t i = 0; i < sequence_length; i++) {
            sequence[i] = get_next_step();
        }
        state = SIMON_GENERATE;  // Start a new game
    }
}
