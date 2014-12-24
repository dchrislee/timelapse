#includ			e <avr/io.h>
#include <util/delay.h>

#define	SR595_SER_DATA		PB0	// 14 pin
#define SR595_RCLK_LATCH	PB1	// 12 pin
#define SR595_SRCLK_CLOCK	PB2	// 11 pin

#define _HA	0	// B segment
#define _HB	1	// C segment
#define _HC	2	// D segment
#define _HD	3	// E segment
#define _HE	4	// F segment
#define	_HF	5	// G segment
#define _HG	6	// H segment
#define	_HH	7	// dot

/*
1 = B | C
2 = A | B | G | E | D
3 = A | B | G | C | D
4 = F | G | B | C
5 = A | F | G | C | D
6 = A | F | G | C | D | E
7 = A | B | C
8 = A | F | G | B | E | D | C
9 = A | F | G | B | C | D
0 = A | B | D | E | F
point = H
*/

#define SETUP_DIGIT(X) 0xFF & ~(X)
#define SETUP_DGT(X) 0xFF - X
static uint8_t _digits[10] = {
    // 0xC0
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HC) | (1 << _HD) | (1 << _HE) | (1 << _HF)),			// 0
    // 0xF9
    SETUP_DIGIT((1 << _HB) | (1 << _HC)),									// 1
    //
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HG) | (1 << _HE) | (1 << _HD)),				// 2
    // 
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HG) | (1 << _HC) | (1 << _HD)),				// 3
    //
    SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HB) | (1 << _HC)),						// 4
    //
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD)),				// 5
    // 
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE)),			// 6
    // 
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HC)),								// 7
    // 
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE) | (1 << _HB)),	// 8
    //
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HB)),			// 9
}; 

void shift(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
	if (0 == (data & _BV(7 - i))) {
	    PORTB &= ~(1 << SR595_SER_DATA);
	} else {
	    PORTB |= (1 << SR595_SER_DATA);
	}
	PORTB &= ~(1 << SR595_SRCLK_CLOCK);
	PORTB |= (1 << SR595_SRCLK_CLOCK); 
    }
    PORTB &= ~(1 << SR595_RCLK_LATCH);
    PORTB |= (1 << SR595_RCLK_LATCH); 
}

int main() {
    DDRB = 0xFF;
    PORTB = 0x00;
    uint8_t dot = 0;
    while (1) {
	for(uint8_t i = 0; i < sizeof(_digits)/sizeof(uint8_t);i++) {
	    shift(_digits[i] + dot);
	    _delay_ms(1000);
	}
	if (dot > 0) {
	    dot = 0;
	} else {
	    dot = (1 << _HH);
	}
    }
    return 0;
}
