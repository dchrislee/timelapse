#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LED_PIN					PB4
#define BUTTON_PIN				PB3
#define HPERIOD					10
#define NPULSES					40

#define SHUT_INSTANT			7330
#define SHUT_DELAYED			5360

void send_pulses();
void shoot_camera();

volatile uint8_t _shoot = 0;

ISR(PCINT0_vect) {
	cli();
	if (!(PINB & (1 << BUTTON_PIN))) {
		_shoot = 1;
	}
	sei();
}

void shoot_camera() {
	send_pulses();
	_delay_us(SHUT_INSTANT);
	send_pulses();
}

void send_pulses() {
	uint8_t i = 0;
	for(i = 0; i < NPULSES; i++) {
		PORTB ^= (1 << LED_PIN);
		_delay_us(HPERIOD);
	}
	PORTB &= ~(1 << LED_PIN);
}

/*
void _delay_using_timer(uint16_t us) {
	uint8_t _bits_prescaler = 0x00;
	if (us > 255) {
		OCR0A = us / 256;					// actual only if F_CPU = 1MHz
		_bits_prescaler = (1 << CS02);
	} else {
		OCR0A = us;							// actual only if F_CPU = 1MHz
		_bits_prescaler = (1 << CS00);
	}
	TCCR0B = _bits_prescaler;
	TCCR0A = (1 << WGM01);					// CTC mode
	while (!(TIFR & (1 << OCF0A))) {};
	TIFR |= (1 << OCF0A);
	TCCR0B &= ~(_bits_prescaler);			// stop timer
}
*/
int main() {
	DDRB = 0xFF & ~(1 << BUTTON_PIN);
	PORTB = 0x00 | (1 << BUTTON_PIN);
	PCMSK |= (1 << PCINT3);               // enable PCINT3
	sei();
	GIMSK |= (1 << PCIE);                 // enable global pc interrupts
	while(true) {
		if (_shoot) {
			GIMSK &= ~(1 << PCIE);
			shoot_camera();
			_shoot = 0;
			GIMSK = (1 << PCIE);
		}
	}
	return 0;
}
