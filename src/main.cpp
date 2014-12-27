#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifdef _USE_EEPROM_
#include <avr/eeprom.h>
uint8_t EEMEM _saved_data;
#endif

#define SR595_SER_DATA                      PB0	// 14 pin
#define SR595_RCLK_LATCH                    PB1	// 12 pin
#define SR595_SRCLK_CLOCK                   PB2	// 11 pin
#define BUTTON_PIN                          PB3

#define BUTTON_MASK_SINGLE_PRESS            0
#define BUTTON_MASK_SINGLE_DELAYED          1
#define BUTTON_MASK_DOUBLE_CLICK            2
#define LED_FLASHING_MASK                   3
#ifdef _USE_EEPROM_
#define STATE_STORE_MASK                    4
#endif
#define STATE_UPDATED_MASK                  7

#define BUTTON_IS_PRESSED(X)                X & (1 << BUTTON_MASK_SINGLE_PRESS)
#define BUTTON_IS_DELAYED(X)                X & (1 << BUTTON_MASK_SINGLE_DELAYED)
#define LED_IS_FLASHING(X)                  X & (1 << LED_FLASHING_MASK)
#ifdef _USE_EEPROM_
#define STORE_REQUESTED(X)                  X & (1 << STATE_STORE_MASK)
#endif
#define STATE_CHANGED(X)                    X & (1 << STATE_UPDATED_MASK)

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

volatile uint16_t _wdt_cnt = 0;

void wdt_disable() {
    cli();
    MCUSR &= ~(1 << WDRF);  // reset watchdog first
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0x00;
    _wdt_cnt = 0;
    sei();
}

void wdt_enable() {
    cli();
    MCUSR &= ~(1 << WDRF);  // reset watchdog first
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = (1 << WDTIE) | (1 << WDP2) | (1 << WDP0);
    _wdt_cnt = 0;
    sei();
}

static uint8_t _digits[16] = {
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
#ifdef _USE_EEPROM_
volatile uint8_t _data;
#else
volatile uint8_t _data = 0;
#endif

volatile uint8_t _app_state = 0;
uint8_t dot = 0;
volatile uint8_t _flash = 0;

ISR(WDT_vect) {
    _flash = _flash ? 0 : 1;
    _wdt_cnt++;
    if (_wdt_cnt > 10) {
        wdt_disable();
        _flash = 0;
#ifdef _USE_EEPROM_
        MAKE_HIGH(_app_state, STATE_STORE_MASK);
#endif
    }
    MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
}

ISR(PCINT0_vect) {
    cli();
    if (!(PINB & (1 << BUTTON_PIN))) {
        MAKE_HIGH(_app_state, BUTTON_MASK_SINGLE_PRESS);
        if (_wdt_cnt == 0) {
            wdt_enable();    
        }
        _wdt_cnt = 0;
    }
    sei();
}

int main() {
    wdt_enable();
    DDRB = 0xFF & ~(1 << BUTTON_PIN);
    PORTB = 0x00 | (1 << BUTTON_PIN);
    MAKE_LOW(ADCSRA, ADEN);                 // turn off ADC
    MAKE_HIGH(PCMSK, PCINT3);               // enable PCINT3
    sei();
    MAKE_HIGH(GIMSK, PCIE);                 // enable global pc interrupts
#ifdef _USE_EEPROM_
    _data = eeprom_read_byte(&_saved_data);
    if (_data == 0xFF) { _data = 0x00; }
#endif
    shift(_digits[_data], 0);

    while (true) {
#ifdef _USE_EEPROM_
        if (STORE_REQUESTED(_app_state)) {
            eeprom_write_byte(&_saved_data, _data);
            MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
        } else 
#endif
        if (BUTTON_IS_DELAYED(_app_state)) {
            MAKE_HIGH(_app_state, LED_FLASHING_MASK);
            MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
        } else if (BUTTON_IS_PRESSED(_app_state)) {
            _data++;
            if (_data >= sizeof(_digits) / sizeof(uint8_t)) {
                _data = 0;
            }
            MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
        }
        if (STATE_CHANGED(_app_state)) {
            shift(_digits[_data] + dot, _flash);
            MAKE_LOW(_app_state, STATE_UPDATED_MASK);
            _app_state = 0;
        }
    }
    return 0;
}
