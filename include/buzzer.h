#include <stdint.h>

void buzzer_init(void);
void increase_octave(void);
void decrease_octave(void);

void update_tone(uint8_t new_tone);
void play_selected_tone(void);
void play_tone(uint8_t tone);
void stop_tone(void);

volatile uint8_t is_playing;
