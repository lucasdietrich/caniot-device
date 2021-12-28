#include "board.h"

#include <stdio.h>
#include <avrtos/misc/uart.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <mcp_can.h>
#include <Wire.h>

#include "tcn75.h"

ISR(PCINT2_vect)
{
	usart_transmit('=');
}

static inline void ll_relays_init(void)
{
    	/* relays PC2, PC3 */
	DDRC |= (1 << DDC2) | (1 << DDC3);
	PORTC &= ~((1 << PORTC2) | (1 << PORTC3));
}

static inline void ll_inputs_init(void)
{
    	/* inputs PD3, PD4, PD5, PD6 */
	DDRD &= ~((1 << DDD3) | (1 << DDD4)
		  | (1 << DDD5) | (1 << DDD6));

    	/* disable pull-up */
	PORTD &= ~((1 << PORTD3) | (1 << PORTD4)
		   | (1 << PORTD5) | (1 << PORTD6));

    	/* enable Pin Change interrupt for inputs */
	PCMSK2 |= (1 << PCINT19) | (1 << PCINT20) |
		(1 << PCINT21) | (1 << PCINT22);
	PCICR |= 1 << PCIE2;
}

static inline void ll_i2c_init(void)
{
	Wire.begin();
}

static inline void dev_tcn75_init(void)
{
	Wire.beginTransmission(TCN75_ADDR);
	Wire.write((uint8_t)TCN75_TEMPERATURE_REGISTER);
	Wire.endTransmission();
}

void custompcb_hw_init(void)
{
	ll_relays_init();
	ll_inputs_init();
	ll_i2c_init();
	// dev_tcn75_init();

	printf_P(PSTR("custompcb_hw_init()\n"));
}