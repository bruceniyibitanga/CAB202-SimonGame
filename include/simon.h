#ifndef SIMON_H
#define SIMON_H

#include <stdint.h>
#include "display_macros.h"

// Timing configuration
#define PLAYBACK_DELAY playback_delay // between 250ms and 2000ms, set by potentiometer

// Simon game states matching state diagram
typedef enum {
    SIMON_GENERATE,    // Generate sequence
    SIMON_PLAY_ON,     // Display pattern
    SIMON_PLAY_OFF,    // Gap between patterns
    AWAITING_INPUT,    // Wait for button press
    HANDLE_INPUT,      // Process button press
    EVALUATE_INPUT,    // Check if input matches sequence
    SUCCESS,           // Show success pattern
    FAIL,              // Show failure pattern
    DISP_SCORE,        // Display final score
    DISP_BLANK,       // Display blank after score before new game
    ENTER_NAME        // New state for name entry
} simon_state_t;

// Function prototypes
void simon_init(void);
void simon_task(void);
void display_two_digit_number(uint8_t num);  // Add declaration
void uart_print_high_scores(void);  // Print high scores table via UART
void update_lfsr_state(uint32_t new_seed);  // Function to update LFSR state
extern uint32_t game_seed;
extern uint32_t pending_seed;
extern uint8_t has_pending_seed;

// State handler prototypes for simon.c state machine
void state_generate(void);
void state_play_on(void);
void state_play_off(void);
void state_awaiting_input(void);
void state_handle_input(void);
void state_evaluate_input(void);
void state_success(void);
void state_fail(void);
void state_disp_score(void);
void state_disp_blank(void);
void state_enter_name(void); // Prototype for state_enter_name

#endif // SIMON_H
