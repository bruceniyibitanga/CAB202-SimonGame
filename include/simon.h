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
    SUCCESS,          // Show success pattern
    FAIL,            // Show failure pattern
    DISP_SCORE       // Display final score
} simon_state_t;

// Display patterns for success/fail using correct segment macros
#define DISP_SUCCESS (DISP_SEG_A & DISP_SEG_B & DISP_SEG_F & DISP_SEG_G)  // Letter P
#define DISP_FAIL (DISP_SEG_A & DISP_SEG_E & DISP_SEG_F & DISP_SEG_G)     // Letter F

// Function prototypes
void simon_init(void);
void simon_task(void);
void display_two_digit_number(uint8_t num);  // Add declaration

#endif // SIMON_H
