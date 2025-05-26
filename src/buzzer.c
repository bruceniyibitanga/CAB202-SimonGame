#include <stdint.h>
#include <avr/io.h>
#include "buzzer.h"

void buzzer_init(void) {
    // Configure buzzer pin
    PORTB.DIRSET = PIN0_bm;
}

void buzzer_play(uint16_t frequency, uint16_t duration) {
    // Generate tone using timer or PWM (hardware-specific)
    // Example: set timer for frequency, enable output
    // Delay for duration (blocking or non-blocking)
}

void buzzer_stop(void) {
    // Stop the buzzer (disable timer/PWM)
}
