#ifndef BUZZER_H
#define BUZZER_H

// Initialise the buzzer hardware
void buzzer_init(void);
// Play a tone (frequency in Hz, duration in ms)
void buzzer_play(uint16_t frequency, uint16_t duration);
// Stop the buzzer
void buzzer_stop(void);

// TESTING:

void buzzer_set_frequency(uint16_t freq_hz);

#endif
