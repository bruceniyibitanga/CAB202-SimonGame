#include "stdint.h"

void timer_init(void);
extern volatile uint16_t elapsed_time_in_milliseconds;
extern volatile uint16_t playback_delay;
extern volatile uint16_t new_playback_delay;
extern volatile uint8_t pb_debounced_state;

void prepare_delay(void);
