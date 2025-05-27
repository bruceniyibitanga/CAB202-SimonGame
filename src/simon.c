#include "simon.h"
#include "display.h"
#include "buzzer.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static simon_state_t state = SIMON_IDLE;
static uint8_t sequence[SIMON_MAX_SEQUENCE];
static uint8_t sequence_length = 1;
static uint8_t user_index = 0;
static uint8_t score = 0;

// Helper: Generate a random step (0-3)
static uint8_t random_step(void) {
    // Simple way to generate pseudo-random numbers
    static uint8_t lfsr = 0x01;  // Linear feedback shift register
    
    // Implement LFSR with taps at bits 7,5,4,3
    uint8_t bit = ((lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 3)) & 1;
    lfsr = (lfsr << 1) | bit;
    
    // Return value 0-3
    return lfsr & 0x03;
}

// Helper: Play back the sequence
static void playback_sequence(void) {
    for (uint8_t i = 0; i < sequence_length; i++) {
        display_show(sequence[i]);
        buzzer_play(440 + 100 * sequence[i], 200); // Example: different tone per step
        // Delay (replace with timer or busy-wait as needed)
        for (volatile uint32_t d = 0; d < 20000; d++);
        display_clear();
        for (volatile uint32_t d = 0; d < 10000; d++);
    }
}

// Helper: Get user input (stub, replace with button read)
static int8_t get_user_input(void) {
    // TODO: Implement button reading
    
    return -1;
}

void simon_init(void) {
    state = SIMON_IDLE;
    sequence_length = 1;
    user_index = 0;
    score = 0;
    for (uint8_t i = 0; i < SIMON_MAX_SEQUENCE; i++) sequence[i] = random_step();
}

void simon_task(void) {
    switch (state) {
        case SIMON_IDLE:
            display_show(0); // Show 0 or attract mode
            // TODO: Wait for start button
            // If start pressed:
            //   state = SIMON_PLAYBACK;
            break;
        case SIMON_PLAYBACK:
            playback_sequence();
            user_index = 0;
            state = SIMON_USER_INPUT;
            break;
        case SIMON_USER_INPUT: {
            int8_t input = get_user_input();
            if (input >= 0) {
                display_show(input);
                buzzer_play(440 + 100 * input, 100);
                if (input == sequence[user_index]) {
                    user_index++;
                    if (user_index >= sequence_length) {
                        score++;
                        sequence_length++;
                        state = SIMON_SUCCESS;
                    }
                } else {
                    state = SIMON_FAILURE;
                }
            }
            break;
        }
        case SIMON_SUCCESS:
            // Success feedback
            display_show(score);
            for (volatile uint32_t d = 0; d < 30000; d++);
            state = SIMON_PLAYBACK;
            break;
        case SIMON_FAILURE:
            // Failure feedback
            display_show_pattern(0xFF); // All segments on
            buzzer_play(200, 500);
            for (volatile uint32_t d = 0; d < 30000; d++);
            simon_init();
            break;
    }
}
