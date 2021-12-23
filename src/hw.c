#include "hw.h"

#include <avrtos/avrtos.h>

#include <avr/io.h>

// @see "init" function from arduino in "wiring.c"
void hw_ll_init(void)
{
    // on the ATmega168, timer 0 is also used for fast hardware pwm
    // (using phase-correct PWM would mean that timer 0 overflowed half as often
    // resulting in different millis() behavior on the ATmega8 and ATmega168)

    // millis used by can library
    SET_BIT(TCCR0A, WGM01);
    SET_BIT(TCCR0A, WGM00);

    // set timer 0 prescale factor to 64
    // this combination is for the standard 168/328/1280/2560
    SET_BIT(TCCR0B, CS01);
    SET_BIT(TCCR0B, CS00);

    // enable timer 0 overflow interrupt
    SET_BIT(TIMSK0, TOIE0);

    // timers 1 and 2 are used for phase-correct hardware pwm
    // this is better for motors as it ensures an even waveform
    // note, however, that fast pwm mode can achieve a frequency of up
    // 8 MHz (with a 16 MHz clock) at 50% duty cycle
#if ARDUINO_ENABLE_FAST_PWM
    TCCR1B = 0;
    SET_BIT(TCCR1B, CS11);
    SET_BIT(TCCR1B, CS10);

    SET_BIT(TCCR1A, WGM10);
#endif

    // set timer 2 prescale factor to 64
    // sbi(TCCR2B, CS22);
    // sbi(TCCR2A, WGM20);

    // set a2d prescaler so we are inside the desired 50-200 KHz range.
#if ARDUINO_ENABLE_ANALOG
    SET_BIT(ADCSRA, ADPS2);
    SET_BIT(ADCSRA, ADPS1);
    SET_BIT(ADCSRA, ADPS0);
    SET_BIT(ADCSRA, ADEN);  // enable disable analog
#endif

    // the bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
	// UCSR0B = 0;

    // enable interrupts before io init
    // sei();
}