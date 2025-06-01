#ifndef DISPLAY_MACROS_H
#define DISPLAY_MACROS_H

#define DIGIT_0 0b0001000
#define DIGIT_1 0b1101011
#define DIGIT_2 0b1000100
#define DIGIT_3 0b1000001
#define DIGIT_4 0b0100011
#define DIGIT_5 0b0010001
#define DIGIT_6 0b0010000
#define DIGIT_7 0b1001011
#define DIGIT_8 0b0000000
#define DIGIT_9 (DISP_SEG_A | DISP_SEG_B | DISP_SEG_C | DISP_SEG_D | DISP_SEG_F | DISP_SEG_G)

#define DISP_SEG_F 0b00111111
#define DISP_SEG_A 0b01011111
#define DISP_SEG_B 0b01101111
#define DISP_SEG_G 0b01110111
#define DISP_SEG_C 0b01111011
#define DISP_SEG_D 0b01111101
#define DISP_SEG_E 0b01111110

#define DISP_BAR_LEFT (DISP_SEG_E & DISP_SEG_F)
#define DISP_BAR_RIGHT (DISP_SEG_B & DISP_SEG_C)

#define DISP_OFF 0b1111111   // All segments OFF (active-low)
#define DISP_LHS 0b10000000  // Left-hand side digit selector (bit 7)

#define DISP_DASH DISP_SEG_G

// Additional display macros
#define DISP_SUCCESS 0x00 // All segments ON for success
#define DISP_FAIL DISP_SEG_G
#define DISP_UNDERSCORE DISP_SEG_D

#endif
