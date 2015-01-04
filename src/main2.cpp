#include <avr/io.h>

#define OCR0_VALUE(X)                       ((F_CPU / X / 2 / 8) - 1)

void generate_pulses() {
    TCCR0B = (1 << CS00);       // Prescaling: using = 0
    TCCR0A = (1 << WGM01) | (1  << COM0A0);      // set CTC mode, OCR0A to have a top value
    OCR0A = OCR0_VALUE(32768);
    TIMSK0 |= (1 << OCIE0A);
}

int main() {
   DDRB |= (1 << PB0);

   CLKPR = 1<<CLKPCE; //oznam cpu, ze ideme nastavovat CLKPR
   CLKPR = 0x6; //clkio = clkcpu/hodn

   generate_pulses();

   while (1) {
   }
}