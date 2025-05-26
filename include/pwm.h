#include "stdint.h"

void pwm_init(void);
void pwm_set_frequency(uint16_t freq_hz);
void pwm_set_duty(uint16_t duty_percentage);