#include <stdint.h>
#include <stdbool.h>
#include <avr/interrupt.h>
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
#define NAME_ENTRY_TIMEOUT 5000 // 5 seconds in ms

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
uint32_t game_seed = INITIAL_SEED;
// Number of steps in the current round
static uint8_t round_length = 1;
// For displaying score after fail
static uint8_t score_to_display = 0;

// Name entry buffer and state
static char name_entry_buffer[MAX_NAME_LEN + 1];
static uint8_t name_entry_len = 0;
static uint32_t name_entry_start_time = 0;
static uint32_t name_entry_last_input_time = 0;
static bool name_entry_active = false;

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
    }    return lfsr_state & 0b11;
}

void update_lfsr_state(uint32_t new_seed) {
    lfsr_state = new_seed;
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

// Print high scores table via UART
void uart_print_high_scores(void) {
    // uart_send('\n'); // Ensure leaderboard starts on a new line
    for (uint8_t i = 0; i < leaderboard_count; i++) {
        // Print name followed by space
        uart_send_str(leaderboard[i].name);
        uart_send(' ');
        // Print score followed by newline
        uart_putnum(leaderboard[i].score);
        uart_send('\n');
    }
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
    // Always use the current game_seed value for LFSR state
    // This preserves UART-provided seeds across resets
    lfsr_state = game_seed;
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
        case ENTER_NAME: state_enter_name(); break;
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
    if(has_pending_uart_seed){
        game_seed = new_uart_seed;
        update_lfsr_state(new_uart_seed);
        has_pending_uart_seed = 0;
    }

    // Always update delay at the start of every round
    playback_delay = get_potentiometer_delay();
    prepare_delay();
    simon_step = get_next_step();
    display_step_pattern(simon_step);
    state = SIMON_PLAY_ON;
}

void state_play_on(void) {
    uint16_t half_delay = playback_delay >> 1;  // Cache the half delay value
    if (elapsed_time_in_milliseconds >= half_delay) {
        stop_tone();
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        state = SIMON_PLAY_OFF;
    }
}

void state_play_off(void) {
    uint16_t half_delay = playback_delay >> 1;  // Cache the half delay value
    if (elapsed_time_in_milliseconds >= half_delay) {
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
            prepare_delay();
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
        uint8_t button = uart_button_flag;
        uart_button_flag = 0;  // Clear flag immediately
        pb_current = button;
        display_step_pattern(pb_current - 1);
        prepare_delay();
        pb_released = 1;
        waiting_extra_delay = 1;
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
    uint16_t half_delay = playback_delay >> 1;  // Cache the half delay value
    bool min_time_reached = elapsed_time_in_milliseconds >= half_delay;
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
        // Send SUCCESS message via UART during SUCCESS pattern display
        uart_send_str("SUCCESS\n");
        uart_putnum(round_length);
        uart_send('\n');
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= playback_delay) {
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
        // Send GAME OVER message via UART during FAIL pattern display
        uart_send_str("GAME OVER\n");
        uart_putnum(round_length);
        uart_send('\n');
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= playback_delay) {
        update_display(DISP_OFF, DISP_OFF);
        lfsr_state = game_seed;
        // Advance LFSR multiple times to ensure a different sequence
        // If sequnce 1,2,3,4,1,4 and playe fails at round 3, the next sequence should be 4 and then 1,4...n
        for(uint8_t i = 0; i < round_length; i++) {
            get_next_step();
        }
        game_seed = lfsr_state;
        score_to_display = round_length;
        round_length = 1;
        first_entry = 1;
        prepare_delay();      
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
    if (elapsed_time_in_milliseconds >= playback_delay) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        first_entry = 1;
        // Always go to DISP_BLANK first (spec requirement)
        state = DISP_BLANK;
    }
}

void state_evaluate_input(void) {
    prepare_delay();  // Reset timer when transitioning to AWAITING_INPUT
    state = AWAITING_INPUT;
}

void state_disp_blank(void) {
    static uint8_t first_entry = 1;
    if (first_entry) {
        update_display(DISP_OFF, DISP_OFF);
        prepare_delay();
        first_entry = 0;
    }
    if (elapsed_time_in_milliseconds >= playback_delay) {
        prepare_delay();
        first_entry = 1;
        
        // Check if we should prompt for name entry (after score display and blank period)
        if (is_player_in_top_5(score_to_display)) {
            state = ENTER_NAME;
        } else {
            // For new game, just reset round length but keep LFSR advancing
            round_length = 1;
            score_to_display = 0; // Reset score display flag
            state = SIMON_GENERATE;
        }
    }
}

// Name entry state handler
void state_enter_name(void) {
    extern volatile uint32_t uart_input_timer;
    if (!name_entry_active) {
        name_entry_len = 0;
        name_entry_buffer[0] = '\0';
        name_entry_start_time = uart_input_timer;
        name_entry_last_input_time = name_entry_start_time;
        name_entry_active = true;        uart_enable_name_entry(); // Enable name entry mode
        uart_send_str("Enter name: ");
    }
    
    // Process one character at a time to avoid blocking the state machine
    if (uart_rx_available()) {
        char c = uart_receive();          
        if (c == '\n' || c == '\r') {
            name_entry_buffer[name_entry_len] = '\0';
            add_player_to_leaderboard(name_entry_buffer, score_to_display);
            uart_print_high_scores(); // Print updated high scores table
            uart_disable_name_entry(); // Disable name entry mode
            name_entry_active = false;
            // prepare_delay(); // Reset timer for next state
            state = SIMON_GENERATE;
            return;
        }
        else if (name_entry_len < MAX_NAME_LEN) {
            name_entry_buffer[name_entry_len++] = c;
            name_entry_buffer[name_entry_len] = '\0';
            name_entry_last_input_time = uart_input_timer;
        }
    }
    
    uint32_t now = uart_input_timer;
    
    // Timeout: no input at all
    if (name_entry_len == 0 && (now - name_entry_start_time >= NAME_ENTRY_TIMEOUT)) {
        name_entry_buffer[0] = '\0';
        add_player_to_leaderboard(name_entry_buffer, score_to_display);
        uart_print_high_scores(); // Print updated high scores table
        uart_disable_name_entry(); // Disable name entry mode
        name_entry_active = false;
        // prepare_delay(); // Reset timer for next state        
        state = SIMON_GENERATE;
        return;
    }
    
    // Timeout: no input for 5s after last char
    if (name_entry_len > 0 && (now - name_entry_last_input_time >= NAME_ENTRY_TIMEOUT)) {
        name_entry_buffer[name_entry_len] = '\0';
        add_player_to_leaderboard(name_entry_buffer, score_to_display);
        uart_print_high_scores(); // Print updated high scores table
        uart_disable_name_entry(); // Disable name entry mode
        name_entry_active = false;
        // prepare_delay(); // Reset timer for next state
        state = SIMON_GENERATE;
        return;
    }
}