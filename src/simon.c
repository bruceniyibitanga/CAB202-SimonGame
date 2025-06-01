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

// Leaderboard
#define MAX_NAME_LEN 20

typedef struct {
    char name[MAX_NAME_LEN + 1]; // +1 for null terminator
    uint8_t score;
} leaderboard_entry_t;

static leaderboard_entry_t leaderboard[5];
static uint8_t leaderboard_count = 0;

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

void sort_leaderboard() {
    for (uint8_t i = 0; i < leaderboard_count; i++) {
        for (uint8_t j = i + 1; j < leaderboard_count; j++) {
            if (leaderboard[i].score < leaderboard[j].score) {
                leaderboard_entry_t temp = leaderboard[i];
                leaderboard[i] = leaderboard[j];
                leaderboard[j] = temp;
            }
        }
    }
}

// Returns true if the score is in the top 5
bool is_player_in_top_5(uint8_t score) {
    if (leaderboard_count < 5) return true;
    return score > leaderboard[leaderboard_count - 1].score;
}

// Adds a player to the leaderboard if eligible
void add_player_to_leaderboard(const char* name, uint8_t score) {
    if (!is_player_in_top_5(score)) return;
    if (leaderboard_count < 5) {
        leaderboard[leaderboard_count].score = score;
        strncpy(leaderboard[leaderboard_count].name, name, MAX_NAME_LEN);
        leaderboard[leaderboard_count].name[MAX_NAME_LEN] = '\0';
        leaderboard_count++;
    } else {
        leaderboard[leaderboard_count - 1].score = score;
        strncpy(leaderboard[leaderboard_count - 1].name, name, MAX_NAME_LEN);
        leaderboard[leaderboard_count - 1].name[MAX_NAME_LEN] = '\0';
    }
    sort_leaderboard();
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
    sequence_length = 1;  // Start with 1 step
    sequence_index = 0;
    lfsr_pos = 0;
    lfsr_state = INITIAL_SEED;
    // Update playback delay from potentiometer at start
    playback_delay = get_potentiometer_delay();
    // Generate the initial 1-step sequence
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
    // Sequence generation/extension is now handled in:
    // - simon_init (for the very first step)
    // - state_success (via add_new_sequence_step to extend the sequence)
    // - state_disp_score (for starting a new 1-step sequence after failure, offset from INITIAL_SEED by lfsr_pos)

    // Update playback delay from potentiometer only when starting to play the sequence
    if (sequence_index == 0) {
        playback_delay = get_potentiometer_delay();
    }
    if (sequence_index < sequence_length) {
        display_step_pattern(sequence[sequence_index]);
        prepare_delay(); // Start timing as soon as tone/display ON
        state = SIMON_PLAY_ON;
    } else {
        sequence_index = 0;
        state = AWAITING_INPUT;
        pb_current = 0;
        pb_released = 1;
        waiting_extra_delay = 0;
    }
}

void state_play_on(void) {
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY / 2)) {
        stop_tone();
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay(); // Start timing as soon as tone/display OFF
        state = SIMON_PLAY_OFF;
    }
}

void state_play_off(void) {
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY / 2)) {
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
    }    // Wait for physical button press
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

    // If button is still held
    if (!pb_released) {
        if (pb_rising_edge & button_mask) {
            pb_released = 1;
            if (elapsed_time_in_milliseconds < (PLAYBACK_DELAY / 2)) {
                waiting_extra_delay = 1;
                prepare_delay(); // Start waiting for the rest of 50% delay
            } else {
                stop_tone();
                update_display(DISP_OFF, DISP_OFF);
                // Evaluate input and transition
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
    } else {
        // Button has been released
        if (waiting_extra_delay) {
            if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY / 2)) {
                stop_tone();
                update_display(DISP_OFF, DISP_OFF);
                waiting_extra_delay = 0;
                // Evaluate input and transition
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
            // Evaluate input and transition
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
}

void state_evaluate_input(void) {
    // Input evaluation is handled within HANDLE_INPUT state
    // This case exists to match the state diagram but is not used
    state = AWAITING_INPUT;
}

void state_success(void) {
    
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
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY << 1)) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        // On fail, set lfsr_pos to the current score (sequence_length - 1 before it's reset)
        // This lfsr_pos will be used to offset the LFSR from INITIAL_SEED for the next game.
        // Note: sequence_length here is the length of the failed sequence.
        // Score is typically sequence_length - 1.
        // If sequence_length was 1 (first step failed), score is 0.
        if (sequence_length > 0) { // Avoid underflow if sequence_length was 0 (shouldn't happen)
             lfsr_pos = sequence_length - 1;
        } else {
             lfsr_pos = 0;
        }
        state = DISP_SCORE;
    }
}

void state_disp_score(void) {
    // Calculate score: number of completed rounds
    // sequence_length represents the round that just failed
    // Score = completed rounds = sequence_length - 1
    // But if they failed on the very first step of the first round, score is 0
    uint8_t score_to_display;
    if (sequence_length <= 1) {
        score_to_display = 0; // Failed on first round
    } else {
        score_to_display = sequence_length - 1; // Number of completed rounds
    }
    display_two_digit_number(score_to_display);    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY << 2)) { // Display score for 4x PLAYBACK_DELAY
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();

        // Reset game parameters for a new game
        sequence_length = 1; // New game starts with 1 step
        sequence_index = 0;
        
        // Update playback delay from potentiometer for new game
        playback_delay = get_potentiometer_delay();

        // Reset LFSR to INITIAL_SEED and then advance it by lfsr_pos
        // lfsr_pos now holds the score of the game that just ended in failure.
        reset_lfsr(); // lfsr_state = INITIAL_SEED;
        for (uint8_t i = 0; i < lfsr_pos; i++) {
            (void)get_next_step(); // Advance LFSR, discard result
        }

        // Generate new 1-step sequence from the current (offset) LFSR state
        for (uint8_t i = 0; i < sequence_length; i++) {
            sequence[i] = get_next_step();
        }
        state = SIMON_GENERATE;  // Start a new game
    }
}
