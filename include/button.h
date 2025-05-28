#include "stdint.h"

// Initial state of the push button is off (logic high).
void buttons_init(void);
// uint8_t get_button_pressed();
extern volatile uint8_t pb_previous_state;