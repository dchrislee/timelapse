#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#ifdef __AVR_ATtiny45__
#include <avr/power.h>
#endif

#ifdef _USE_EEPROM_
#include <avr/eeprom.h>
uint8_t EEMEM _saved_data;
#endif

#define SR595_SER_DATA                      PB0	// 14 pin
#define SR595_RCLK_LATCH                    PB1	// 12 pin
#define SR595_SRCLK_CLOCK                   PB2	// 11 pin
#define BUTTON_PIN                          PB3
#define LED_PIN                             PB4


#define BUTTON_MASK_SINGLE_PRESS            0
#ifdef _USE_EEPROM_
#define STATE_STORE_MASK                    1
#endif //_USE_EEPROM_
#define WATCHDOG_MASK                       2
#define SHOOTING_MASK                       3
#define STATE_UPDATED_MASK                  7

#define BUTTON_IS_PRESSED(X)                X & (1 << BUTTON_MASK_SINGLE_PRESS)
#ifdef _USE_EEPROM_
#define STORE_REQUESTED(X)                  X & (1 << STATE_STORE_MASK)
#endif //_USE_EEPROM_
#define STATE_CHANGED(X)                    X & (1 << STATE_UPDATED_MASK)
#define WATCHDOG_PING(X)                    X & (1 << WATCHDOG_MASK)
#define IS_SHOOTING(X)                      X & (1 << SHOOTING_MASK)

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

#ifdef _USE_EEPROM_
volatile uint8_t _data;
#else
volatile uint8_t _data = 0xFF;
#endif //_USE_EEPROM_

volatile uint8_t _app_state = 0;
uint8_t dot = 0;
volatile uint8_t _flash = 0;

#define HPERIOD                 10
#define NPULSES                 40

#define SHUT_INSTANT            7330
#define SHUT_DELAYED            5360

uint8_t _pulses = 0;

ISR(TIM0_COMPA_vect) {
    MAKE_HIGH(_app_state, SHOOTING_MASK);
}

void send_pulses();

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

uint16_t _wdt_cnt = 0;

void wdt_disable() {
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0x00;
    _wdt_cnt = 0;
    sei();
}

void wdt_enable() {
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = (1 << WDIE) | (1 << WDP2);   // 0.25sec
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

ISR(WDT_vect) {
    _flash = _flash ? 0 : 1;
    MAKE_HIGH(_app_state, WATCHDOG_MASK);        
}

ISR(PCINT0_vect) {
    cli();
    if (!(PINB & (1 << BUTTON_PIN))) {
        MAKE_HIGH(_app_state, BUTTON_MASK_SINGLE_PRESS);
    }
    sei();
}

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

#ifdef _USE_EEPROM_
    _data = eeprom_read_byte(&_saved_data);
    if (_data == 0xFF) { _data = 0x00; }
#endif //_USE_EEPROM_
    shift(_digits[_data], 0);

    while (true) {
        if (IS_SHOOTING(_app_state)) {
            MAKE_LOW(_app_state, SHOOTING_MASK);
            shoot_camera();
        } else {
            if (WATCHDOG_PING(_app_state)) {
                _wdt_cnt++;
                if (_wdt_cnt > 10) {
                    wdt_disable();
                    _flash = 0;
#ifdef _USE_EEPROM_
                    MAKE_HIGH(_app_state, STATE_STORE_MASK);
#endif //_USE_EEPROM_
                    shoot_camera();
                }
                MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
            }
#ifdef _USE_EEPROM_
            if (STORE_REQUESTED(_app_state)) {
                eeprom_write_byte(&_saved_data, _data);
                MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
            } 
#endif //_USE_EEPROM_

            if (BUTTON_IS_PRESSED(_app_state)) {
                _data++;
                if (_data >= sizeof(_digits) / sizeof(uint8_t)) {
                    _data = 0;
                }
                if (_wdt_cnt == 0) {
                    wdt_enable();    
                }
                _wdt_cnt = 0;
                MAKE_HIGH(_app_state, STATE_UPDATED_MASK);
            }
            if (STATE_CHANGED(_app_state)) {
                shift(_digits[_data] + dot, _flash);
                MAKE_LOW(_app_state, STATE_UPDATED_MASK);
                _app_state = 0;
            }
        }
    }
    return 0;
}
