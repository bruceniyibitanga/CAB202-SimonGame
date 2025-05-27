#include "stdint.h"

// Initial state of the push button is off (logic high).
void buttons_init(void);
uint8_t detect_button_pressed(uint8_t pin_bm);
extern volatile uint8_t pb_previous_state;