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
#include <string.h>

// Define display patterns for the bars
#define DISP_BAR_LEFT (DISP_SEG_E & DISP_SEG_F)   // Left segments
#define DISP_BAR_RIGHT (DISP_SEG_B & DISP_SEG_C)  // Right segments

// Game states and variables
static simon_state_t state = SIMON_GENERATE;

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
// Replace round_seed with game_seed for persistent sequence
static uint32_t game_seed = INITIAL_SEED;
// Number of steps in the current round
static uint8_t round_length = 1;
// For displaying score after fail
static uint8_t score_to_display = 0;

// Function to display a two-digit number
void display_two_digit_number(uint8_t num) {
    uint8_t tens = num / 10;
    uint8_t ones = num % 10;
    // For single-digit numbers (0-9), show leading space (DISP_OFF)
    if (num < 10) {
        update_display(DISP_OFF,
            ones == 0 ? DIGIT_0 :
            ones == 1 ? DIGIT_1 :
            ones == 2 ? DIGIT_2 :
            ones == 3 ? DIGIT_3 :
            ones == 4 ? DIGIT_4 :
            ones == 5 ? DIGIT_5 :
            ones == 6 ? DIGIT_6 :
            ones == 7 ? DIGIT_7 :
            ones == 8 ? DIGIT_8 : DIGIT_9);
    } else {
        // For two-digit numbers (10-99) or overflow (>99), show both digits
        update_display(
            tens == 0 ? DIGIT_0 :
            tens == 1 ? DIGIT_1 :
            tens == 2 ? DIGIT_2 :
            tens == 3 ? DIGIT_3 :
            tens == 4 ? DIGIT_4 :
            tens == 5 ? DIGIT_5 :
            tens == 6 ? DIGIT_6 :
            tens == 7 ? DIGIT_7 :
            tens == 8 ? DIGIT_8 : DIGIT_9,
            ones == 0 ? DIGIT_0 :
            ones == 1 ? DIGIT_1 :
            ones == 2 ? DIGIT_2 :
            ones == 3 ? DIGIT_3 :
            ones == 4 ? DIGIT_4 :
            ones == 5 ? DIGIT_5 :
            ones == 6 ? DIGIT_6 :
            ones == 7 ? DIGIT_7 :
            ones == 8 ? DIGIT_8 : DIGIT_9);
    }
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

// Remove unused variables and functions
// Removed: sequence_length, sequence_index, lfsr_pos, sequence[], add_new_sequence_step(), reset_lfsr()

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
    round_length = 1;
    lfsr_state = INITIAL_SEED;
    game_seed = INITIAL_SEED;
    prepare_delay();
}

void simon_task(void) {
    if (state == AWAITING_INPUT && (uart_button_flag || pb_falling_edge)) {
        state_awaiting_input();
        return;
    }
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
        case DISP_BLANK: state_disp_blank(); break;
    }
}

// =========================
// State handler functions
// =========================

static uint8_t simon_step = 0; // Current step being played
static uint8_t simon_play_index = 0; // Index for Simon's playback
static uint8_t user_input_index = 0; // Index for user input

void state_generate(void) {
    // Always start from game_seed for cumulative sequence
    simon_play_index = 0;
    lfsr_state = game_seed;
    prepare_delay();
    simon_step = get_next_step();
    display_step_pattern(simon_step);
    state = SIMON_PLAY_ON;
}

void state_play_on(void) {
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY >> 1)) {
        stop_tone();
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        state = SIMON_PLAY_OFF;
    }
}

void state_play_off(void) {
    if (elapsed_time_in_milliseconds >= (PLAYBACK_DELAY >> 1)) {
        simon_play_index++;
        if (simon_play_index < round_length) {
            lfsr_state = game_seed;
            for (uint8_t i = 0; i <= simon_play_index; i++) {
                simon_step = get_next_step();
            }
            prepare_delay();
            display_step_pattern(simon_step);
            state = SIMON_PLAY_ON;
        } else {
            user_input_index = 0;
            lfsr_state = game_seed;
            state = AWAITING_INPUT;
            pb_current = 0;
            pb_released = 1;
            waiting_extra_delay = 0;
        }
    }
}

void state_awaiting_input(void) {
    // UART input: simulate instant press and release
    if (uart_button_flag) {
        pb_current = uart_button_flag;
        display_step_pattern(pb_current - 1);
        prepare_delay();
        pb_released = 1;
        waiting_extra_delay = 1;
        uart_button_flag = 0;
        state = HANDLE_INPUT;
    } else if (pb_falling_edge & PIN4_bm) {
        pb_current = 1;
        display_step_pattern(0);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    } else if (pb_falling_edge & PIN5_bm) {
        pb_current = 2;
        display_step_pattern(1);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    } else if (pb_falling_edge & PIN6_bm) {
        pb_current = 3;
        display_step_pattern(2);
        pb_released = 0;
        prepare_delay();
        state = HANDLE_INPUT;
    } else if (pb_falling_edge & PIN7_bm) {
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
    bool button_released = (pb_rising_edge & button_mask) || pb_released;
    bool min_time_reached = elapsed_time_in_milliseconds >= (PLAYBACK_DELAY >> 1);
    bool should_stop = min_time_reached && (button_released || pb_current == 0);
    if (pb_released && waiting_extra_delay) {
        should_stop = min_time_reached;
    }
    if (should_stop) {
        stop_tone();
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        waiting_extra_delay = 0;
        pb_released = 1;
        // Check user input against generated step
        lfsr_state = game_seed;
        for (uint8_t i = 0; i <= user_input_index; i++) {
            simon_step = get_next_step();
        }
        if ((pb_current - 1) == simon_step) {
            user_input_index++;
            if (user_input_index < round_length) {
                state = AWAITING_INPUT;
            } else {
                update_display(DISP_SUCCESS, DISP_SUCCESS);
                prepare_delay();
                state = SUCCESS;
            }
        } else {
            update_display(DISP_FAIL, DISP_FAIL);
            prepare_delay();
            state = FAIL;
        }
    }
    if (!pb_released && (pb_rising_edge & button_mask)) {
        pb_released = 1;
    }
}

void state_success(void) {
    static uint8_t first_entry = 1;
    if (first_entry) {
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        // On success, increase round length (do not change game_seed)
        round_length++;
        first_entry = 1;
        state = SIMON_GENERATE;
    }
}

void state_fail(void) {
    static uint8_t first_entry = 1;
    if (first_entry) {
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        // On fail, advance LFSR once, set round length to 1, and save new game_seed
        lfsr_state = game_seed;
        get_next_step(); // Advance LFSR to next step
        game_seed = lfsr_state;
        // Show the score for the last successfully completed round
        score_to_display = round_length;
        round_length = 1;
        first_entry = 1;
        state = DISP_SCORE;
    }
}

void state_disp_score(void) {
    static uint8_t first_entry = 1;
    if (first_entry) {
        display_two_digit_number(score_to_display);
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        first_entry = 1;
        state = DISP_BLANK;
    }
}

void state_disp_blank(void) {
    static uint8_t first_entry = 1;
    if (first_entry) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= PLAYBACK_DELAY) {
        prepare_delay();
        // Reset for new game
        round_length = 1;
        lfsr_state = INITIAL_SEED;
        game_seed = INITIAL_SEED;
        first_entry = 1;
        state = SIMON_GENERATE;
    }
}

void state_evaluate_input(void) {
    // No-op: evaluation is handled in state_handle_input after stateless LFSR refactor.
    // This function exists to resolve linker errors and maintain state machine compatibility.
    // If needed, add logic here for future extensions.
    state = AWAITING_INPUT;
}
