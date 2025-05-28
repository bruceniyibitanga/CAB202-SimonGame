#include "stdint.h"

#ifndef SIMON_H
#define SIMON_H

// Simon game states
typedef enum {
    SIMON_IDLE,
    SIMON_PLAYBACK,
    SIMON_USER_INPUT,
    SIMON_SUCCESS,
    SIMON_FAILURE
} simon_state_t;

// Maximum sequence length
#define SIMON_MAX_SEQUENCE 32

// Function prototypes
void simon_init(void);
void simon_task(void);
uint8_t LFSR();
#endif // SIMON_H
