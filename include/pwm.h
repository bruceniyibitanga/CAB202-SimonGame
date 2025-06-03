#include "stdint.h"

void pwm_init(void);
void pwm_set_frequency(uint32_t freq_hz);
void pwm_set_duty(uint32_t duty_percentage);