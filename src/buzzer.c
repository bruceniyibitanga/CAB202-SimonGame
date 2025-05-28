#include "buzzer.h"

#include <stdint.h>

#include <avr/io.h>

// -----------------------------  BUZZER  -----------------------------

#define MAX_OCTAVE 3
#define MIN_OCTAVE -3

volatile uint8_t is_playing = 0;
static int8_t selected_tone = 0;
static int8_t octave = 0;

void increase_octave(void)
{
    if (octave < MAX_OCTAVE)
    {
        octave++;
        if (is_playing)
            play_tone(selected_tone);
    }
}

void decrease_octave(void)
{
    if (octave > MIN_OCTAVE)
    {
        octave--;
        if (is_playing)
            play_tone(selected_tone);
    }
}

void update_tone(uint8_t new_tone)
{
    if (new_tone > 3) return; // Validate tone number
    
    // Update the tone if already active
    if (is_playing)
        play_tone(new_tone);
    else
        // otherwise, select a new tone for the next time a tone is played
        selected_tone = new_tone;
}

void play_selected_tone(void)
{
    play_tone(selected_tone);
}

void play_tone(uint8_t tone)
{
    if (tone > 3) return; // Validate tone number

    // Base periods for each note at octave 0
    // F_CPU/freq gives us the period in clock cycles
    static const uint16_t base_periods[4] = {
        F_CPU/324,  // E(high)  = 1029
        F_CPU/372,  // C#       = 896
        F_CPU/432,  // A        = 772
        F_CPU/216   // E(low)   = 1543
    };

    // Calculate actual period based on octave
    uint16_t period = base_periods[tone] >> octave;
    
    // Set PWM period and duty cycle (50%)
    TCA0.SINGLE.PERBUF = period;
    TCA0.SINGLE.CMP0BUF = period >> 1;

    selected_tone = tone;
    is_playing = 1;
}

void stop_tone(void)
{
    TCA0.SINGLE.CMP0BUF = 0;
    is_playing = 0;
}