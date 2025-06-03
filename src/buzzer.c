#include "buzzer.h"

#include <stdint.h>

#include <avr/io.h>

// -----------------------------  BUZZER  -----------------------------

#define MAX_OCTAVE 3
#define MIN_OCTAVE -3

volatile uint8_t is_playing = 0;
static uint8_t selected_tone = 0;
// The current frequency is defined globally to be used across modules
volatile uint16_t current_freq = 0; 
static int8_t octave = 0;

// Define the current_button_playing variable
uint8_t current_button_playing = 0;

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

    extern volatile uint16_t current_freq_ehigh;
    extern volatile uint16_t current_freq_csharp;
    extern volatile uint16_t current_freq_a;
    extern volatile uint16_t current_freq_elow;
    uint16_t freq = 0;
    switch (tone) {
        case 0: freq = current_freq_ehigh; break;
        case 1: freq = current_freq_csharp; break;
        case 2: freq = current_freq_a; break;
        case 3: freq = current_freq_elow; break;
        default: return;
    }
    if (freq < 40) freq = 40;
    if (freq > 20000) freq = 20000;
    uint16_t period = F_CPU / freq;
    TCA0.SINGLE.PERBUF = period;
    TCA0.SINGLE.CMP0BUF = period >> 1;
    selected_tone = tone;
    current_freq = freq;
    is_playing = 1;
    current_button_playing = tone + 1; // Track which button is playing (1-4)
}

// Function to update the currently playing tone when frequencies change
void update_current_tone_frequency(void)
{
    if (is_playing) {
        // Re-play the current tone with updated frequency
        play_tone(selected_tone);
    }
}

void stop_tone(void)
{
    TCA0.SINGLE.CMP0BUF = 0;
    is_playing = 0;
    current_button_playing = 0; // No button playing
}