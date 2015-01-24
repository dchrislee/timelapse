#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#ifdef __AVR_ATtiny45__
#include <avr/power.h>
#endif
#include <avr/sleep.h>

void send_pulses();
void shoot_camera();
void wdt_disable();
void wdt_enable();
void shift(uint8_t data);

#define SR595_SER_DATA                      PB0	// 14 pin
#define SR595_RCLK_LATCH                    PB1	// 12 pin
#define SR595_SRCLK_CLOCK                   PB2	// 11 pin
#define BUTTON_PIN                          PB3
#define LED_PIN                             PB4

#define _HA                                 0 // B segment
#define _HB                                 1 // C segment
#define _HC                                 2 // D segment
#define _HD                                 3 // E segment
#define _HE                                 4 // F segment
#define _HF                                 5 // G segment
#define _HG                                 6 // H segment
#define _HH                                 7 // dot

#ifdef _COMMON_CATODE_
#define SETUP_DIGIT(X)                      (X)         // common catode 7segment indicator
#else
#define SETUP_DIGIT(X)                      0xFF & ~(X) // common anode 7segment indicator
#endif
#define MAKE_LOW(X, Y)                      X &= ~(1 << Y)
#define MAKE_HIGH(X, Y)                     X |= (1 << Y)
#define TOGGLE_BIT(X, Y)                    X ^= (1 << Y)

#define OCR_VALUE(X, P)                     ((F_CPU / P) / X) - 1

uint8_t _data = 0;
uint16_t _app_state = 0;
uint8_t _flash_cnt = 0;
uint8_t _flashed = 0;
uint8_t _display_timeout = 0;

#define SET_MODE(_MODE)                     MAKE_HIGH(_app_state, _MODE)
#define IS_MODE(_MODE)                      _app_state & (1 << _MODE)
#define CLEAR_MODE(_MODE)                   MAKE_LOW(_app_state, _MODE)

#define BUTTON_MODE                         1
#define TURN_ON_SR_LED                      2
#define TURN_OFF_SR_LED                     3
#define DISPLAY_VALUE                       4
#define COUNT_TO_DISPLAY_OFF                5
#define FLASH_VALUE                         6
#define FLASHED_VALUE                       7
#define TURN_OFF_FLASH                      8
#define SHOOT_CAMERA                        9

#define HPERIOD                             10
#define NPULSES                             40

#define SHUT_INSTANT                        7330
#define SHUT_DELAYED                        5360

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

void wdt_disable() {
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0x00;
    sei();
}

/*
#define WDP3    5       16
#define WDP2    2       4
#define WDP1    1       2
#define WDP0    0       1
*/

void wdt_enable() {
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = (1 << WDIE) | (1 << WDP2);   // 0.25sec
    sei();
}

static uint8_t _digits[18] = {
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HC) | (1 << _HD) | (1 << _HE) | (1 << _HF)),               // 0
    SETUP_DIGIT((1 << _HB) | (1 << _HC)),                                                                   // 1
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HG) | (1 << _HE) | (1 << _HD)),                            // 2
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HG) | (1 << _HC) | (1 << _HD)),                            // 3
    SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HB) | (1 << _HC)),                                         // 4
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD)),                            // 5
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE)),               // 6
    SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HC)),                                                      // 7
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE) | (1 << _HB)),  // 8
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HB)),               // 9
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HE) | (1 << _HB)),               // A
    SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE)),                            // b
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HD) | (1 << _HE)),                                         // C
    SETUP_DIGIT((1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HB) | (1 << _HE)),                            // d
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HD) | (1 << _HE)),                            // E
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HE)),                                         // F
    SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HD) | (1 << _HE) | (1 << _HC)),                            // G
    SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HE) | (1 << _HB)),                            // H
}; 

void shift(uint8_t data, uint8_t flash) {
    if (flash) {
        data = 0xFF;
    }
    for (uint8_t i = 0; i < 8; i++) {
        if (0 == (data & _BV(7 - i))) {
            MAKE_LOW(PORTB, SR595_SER_DATA);
        } else {
            MAKE_HIGH(PORTB, SR595_SER_DATA);
        }
        MAKE_LOW(PORTB, SR595_SRCLK_CLOCK);
        MAKE_HIGH(PORTB, SR595_SRCLK_CLOCK);
    }
    MAKE_LOW(PORTB, SR595_RCLK_LATCH);
    MAKE_HIGH(PORTB, SR595_RCLK_LATCH);
}

ISR(PCINT0_vect) {
    cli();
    if (!(PINB & (1 << BUTTON_PIN))) {
        SET_MODE(BUTTON_MODE);
    }
    sei();
}

ISR(WDT_vect) {}

int main() {
    wdt_enable();
    DDRB = 0xFF & ~(1 << BUTTON_PIN);
    PORTB = 0x00 | (1 << BUTTON_PIN);
    MAKE_LOW(ADCSRA, ADEN);                 // turn off ADC
    MAKE_LOW(ACSR, ACD);                    // turn off Analog Comparator
#ifdef __AVR_ATtiny45__
    power_adc_disable();
    power_timer1_disable();
    power_usi_disable();
#endif
    MAKE_HIGH(PCMSK, PCINT3);               // enable PCINT3
    sei();
    MAKE_HIGH(GIMSK, PCIE);                 // enable global pc interrupts

    SET_MODE(DISPLAY_VALUE);

    while (true) {
        if (IS_MODE(BUTTON_MODE)) {
            if (++_data >= (sizeof(_digits) / sizeof(uint8_t))) {
                _data = 0;
            }
            _flash_cnt = 0;
            SET_MODE(TURN_ON_SR_LED);
            CLEAR_MODE(BUTTON_MODE);
            CLEAR_MODE(COUNT_TO_DISPLAY_OFF);
        }
        if (IS_MODE(TURN_ON_SR_LED)) {
            // turn on Shift Register and LED
            SET_MODE(DISPLAY_VALUE);
            CLEAR_MODE(TURN_ON_SR_LED);
            MAKE_LOW(PORTB, LED_PIN);
        }
        if (IS_MODE(TURN_OFF_SR_LED)) {
            // turn off Shift Register and LED
            CLEAR_MODE(TURN_OFF_SR_LED);
            MAKE_HIGH(PORTB, LED_PIN);
        }
        if (IS_MODE(COUNT_TO_DISPLAY_OFF)) {
            _display_timeout++;
            if (_display_timeout >= 40) {   // 5 seconds
                _display_timeout = 0;
                SET_MODE(TURN_OFF_SR_LED);
                CLEAR_MODE(DISPLAY_VALUE);
                CLEAR_MODE(COUNT_TO_DISPLAY_OFF);
            }
        }
        if (IS_MODE(DISPLAY_VALUE)) {
            shift(_digits[_data], IS_MODE(FLASHED_VALUE));
            CLEAR_MODE(DISPLAY_VALUE);
            if (IS_MODE(TURN_OFF_FLASH)) {
                CLEAR_MODE(TURN_OFF_FLASH);
                CLEAR_MODE(FLASH_VALUE);
                SET_MODE(COUNT_TO_DISPLAY_OFF);
            } else {
                SET_MODE(FLASH_VALUE);
            }
        }
        if (IS_MODE(FLASH_VALUE)) {
            if ((_flash_cnt % 2) == 0) {
                TOGGLE_BIT(_app_state, FLASHED_VALUE);
            }
            if (++_flash_cnt >= 30) {
                _flash_cnt = 0;
                SET_MODE(TURN_OFF_FLASH);
                CLEAR_MODE(FLASH_VALUE);
                CLEAR_MODE(FLASHED_VALUE);
            }
            SET_MODE(DISPLAY_VALUE);
        }
/*
        if (IS_MODE(SHOOT_CAMERA)) {
            shoot_camera();
            CLEAR_MODE(SHOOT_CAMERA);
        }
*/
        set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode
        sleep_mode(); //Go to sleep
      }
    return 0;
}
