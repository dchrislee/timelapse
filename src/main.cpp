#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef __AVR_ATtiny45__
#include <avr/power.h>
#endif
#include <avr/sleep.h>

#ifndef _USE_UTIL_DELAY
#include <util/dela.h>
#endif

#ifdef _USE_EEPROM_
#include <avr/eeprom.h>
uint8_t EEMEM _saved_data;
#endif

void send_pulses();
void shoot_camera();
void wdt_disable();
void wdt_enable();
void shift(uint8_t data);

#define WDT_DEFAULT                         (1 << WDP2)   // 0.25 sec

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

#ifdef _USE_EEPROM_
uint8_t _data;
#else
uint8_t _data = 0xFF;
#endif //_USE_EEPROM_

uint16_t _app_state = 0;
uint8_t _flash_cnt = 0;
uint8_t _flashed = 0;
uint8_t _display_timeout = 0;
uint16_t _program_cnt = 0;

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
#define RUN_PROGRAM                         10
#define SHOOT_SINGLE_CAMERA                 11

#define CONVERT_MS_TO_CYCLES(MS)            (MS * ((double)F_CPU / (double)1000000UL))

#define NPULSES                             40
#define HPERIOD                             CONVERT_MS_TO_CYCLES(10)
#define SHUT_INSTANT                        CONVERT_MS_TO_CYCLES(7330)
#define SHUT_DELAYED                        CONVERT_MS_TO_CYCLES(5360)


static inline void _power_sleep() __attribute__((always_inline));
#ifndef _USE_UTIL_DELAY
static inline void _delay_loop_2(uint16_t __count) __attribute__((always_inline));
#define delay_us(cycles) _delay_loop_2(cycles / 2)
#endif

void shoot_camera() {
    send_pulses();
    delay_us(SHUT_INSTANT);
    send_pulses();
}

void send_pulses() {
    uint8_t i = 0;
    for(i = 0; i < NPULSES; i++) {
        PORTB ^= (1 << LED_PIN);
        delay_us(HPERIOD);
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

void wdt_enable(uint8_t mode) {
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = (1 << WDIE) | mode;
    sei();
}

struct interval_duration {
    uint8_t     digit;
    uint8_t     interval;
    uint8_t     wdt;
};

static interval_duration durations[] = {
    {   // 0
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HC) | (1 << _HD) | (1 << _HE) | (1 << _HF)),
        .interval   = 0,
        .wdt        = 0
    },
    {   // 1
        .digit      = SETUP_DIGIT((1 << _HB) | (1 << _HC)),
        .interval   = 1,
        .wdt        = (1 << WDP2) | (1 << WDP1)
    },
    {   // 2
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HG) | (1 << _HE) | (1 << _HD)),
        .interval   = 1,
        .wdt        = (1 << WDP0) | (1 << WDP1) | (1 << WDP2)
    },
    {   // 3
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HG) | (1 << _HC) | (1 << _HD)),
        .interval   = 3,
        .wdt        = (1 << WDP2) | (1 << WDP1)
    },
    {   // 4
        .digit      = SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HB) | (1 << _HC)),
        .interval   = 1,
        .wdt        = (1 << WDP3)
    },
    {   // 5
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD)),
        .interval   = 5,
        .wdt        = (1 << WDP2) | (1 << WDP1)
    },
    {   // 6
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE)),
        .interval   = 3,
        .wdt        = (1 << WDP0) | (1 << WDP1) | (1 << WDP2)
    },
    {   // 7
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HB) | (1 << _HC)),
        .interval   = 7,
        .wdt        = (1 << WDP2) | (1 << WDP1)
    },
    {   // 8
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE) | (1 << _HB)),
        .interval   = 1,
        .wdt        = (1 << WDP0)
    },
    {   // 9
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HB)),
        .interval   = 9,
        .wdt        = (1 << WDP2) | (1 << WDP1)
    },
    {   // a 1
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HE) | (1 << _HB)),
        .interval   = 15,
        .wdt        = (1 << WDP3)
    },
    {   // b 2
        .digit      = SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HE)),
        .interval   = 15,
        .wdt        = (1 << WDP0)
    },
    {   // c 3
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HD) | (1 << _HE)),
        .interval   = 45,
        .wdt        = (1 << WDP3)
    },
    {   // d 4
        .digit      = SETUP_DIGIT((1 << _HG) | (1 << _HC) | (1 << _HD) | (1 << _HB) | (1 << _HE)),
        .interval   = 30,
        .wdt        = (1 << WDP0)
    },
    {   // e 5
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HD) | (1 << _HE)),
        .interval   = 75,
        .wdt        = (1 << WDP3)
    },
    {   // f 6
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HG) | (1 << _HE)),
        .interval   = 45,
        .wdt        = (1 << WDP0)
    },
    {   // g 7
        .digit      = SETUP_DIGIT((1 << _HA) | (1 << _HF) | (1 << _HD) | (1 << _HE) | (1 << _HC)),
        .interval   = 105,
        .wdt        = (1 << WDP3)
    },
    {   // h 8
        .digit      = SETUP_DIGIT((1 << _HF) | (1 << _HG) | (1 << _HC) | (1 << _HE) | (1 << _HB)),
        .interval   = 60,
        .wdt        = (1 << WDP0)
    },
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
    wdt_enable(WDT_DEFAULT);
    DDRB = 0xFF & ~(1 << BUTTON_PIN);
    PORTB = 0x00 | (1 << BUTTON_PIN);
    MAKE_LOW(ADCSRA, ADEN);                 // turn off ADC
    MAKE_LOW(ACSR, ACD);                    // turn off Analog Comparator
#ifdef __AVR_ATtiny45__
    power_adc_disable();
    power_timer0_disable();
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

    SET_MODE(DISPLAY_VALUE);

    while (true) {
        if (IS_MODE(BUTTON_MODE)) {
            if (++_data >= (sizeof(durations) / sizeof(struct interval_duration))) {
                _data = 0;
            }
            _flash_cnt = 0;
            SET_MODE(TURN_ON_SR_LED);
            CLEAR_MODE(BUTTON_MODE);
            CLEAR_MODE(COUNT_TO_DISPLAY_OFF);
            CLEAR_MODE(RUN_PROGRAM);
            _program_cnt = 0;
        }
        if (IS_MODE(TURN_ON_SR_LED)) {
            // turn on Shift Register and LED
            SET_MODE(DISPLAY_VALUE);
            CLEAR_MODE(TURN_ON_SR_LED);
            wdt_enable(WDT_DEFAULT);
        }
        if (IS_MODE(TURN_OFF_SR_LED)) {
            // turn off Shift Register and LED
            shift(0xFF, 0);
            CLEAR_MODE(TURN_OFF_SR_LED);
            if (_data) {
                SET_MODE(RUN_PROGRAM);
            } else {
                wdt_disable();
            }
#ifdef _USE_EEPROM_
            eeprom_write_byte(&_saved_data, _data);
#endif //_USE_EEPROM_
        }
        if (IS_MODE(COUNT_TO_DISPLAY_OFF)) {
            _display_timeout++;
            if (_display_timeout >= 20) {   // 5 seconds
                _display_timeout = 0;
                SET_MODE(TURN_OFF_SR_LED);
                CLEAR_MODE(DISPLAY_VALUE);
                CLEAR_MODE(COUNT_TO_DISPLAY_OFF);
            }
        }
        if (IS_MODE(DISPLAY_VALUE)) {
            shift(durations[_data].digit, IS_MODE(FLASHED_VALUE));
            CLEAR_MODE(DISPLAY_VALUE);
            if (IS_MODE(TURN_OFF_FLASH)) {
                CLEAR_MODE(TURN_OFF_FLASH);
                CLEAR_MODE(FLASH_VALUE);
                SET_MODE(COUNT_TO_DISPLAY_OFF);
                _display_timeout = 0;
            } else {
                SET_MODE(FLASH_VALUE);
            }
        }
        if (IS_MODE(FLASH_VALUE)) {
            TOGGLE_BIT(_app_state, FLASHED_VALUE);
            if (++_flash_cnt >= 20) {
                _flash_cnt = 0;
                SET_MODE(TURN_OFF_FLASH);
                CLEAR_MODE(FLASH_VALUE);
                CLEAR_MODE(FLASHED_VALUE);
            }
            SET_MODE(DISPLAY_VALUE);
        }
        if (IS_MODE(SHOOT_CAMERA)) {
            shoot_camera();
            CLEAR_MODE(SHOOT_CAMERA);
            _program_cnt = 0;
        }
        if (IS_MODE(SHOOT_SINGLE_CAMERA)) {
            shoot_camera();
            CLEAR_MODE(SHOOT_SINGLE_CAMERA);
        }
        _power_sleep();
        if (IS_MODE(RUN_PROGRAM)) {
            _program_cnt++;
            if (_program_cnt >= durations[_data].interval) {
                _program_cnt = 0;
                SET_MODE(SHOOT_CAMERA);
            }
        }
    }
    return 0;
}

#ifndef _USE_UTIL_DELAY
void _delay_loop_2(uint16_t __count) {
    __asm__ volatile (
        "1: sbiw %0,1" "\n\t"
        "brne 1b"
            : "=w" (__count)
            : "0" (__count)
    );
}
#endif

void _power_sleep() {
    if (IS_MODE(RUN_PROGRAM)) {
        wdt_enable(durations[_data].wdt);
    }
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
    sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
    wdt_enable(WDT_DEFAULT);
}
