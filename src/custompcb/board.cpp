#include "board.h"

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <avrtos/misc/uart.h>
#include <avrtos/kernel.h>

#include <mcp_can.h>
#include <Wire.h>

#include "tcn75.h"

#define K_MODULE_LL    0x22
#define K_MODULE K_MODULE_LL

ISR(PCINT2_vect)
{
	// printf_P(PSTR("D: %hhx"), ll_inputs_read());
}

static inline void ll_relays_init(void)
{
	/* relays PC2, PC3 */
	DDRC |= (1 << DDC2) | (1 << DDC3);

	/* initial state low */
	PORTC &= ~((1 << PORTC2) | (1 << PORTC3));
}

void ll_relays_set(uint8_t state)
{
	const uint8_t prev = PORTC & ~((1 << PORTC2) | (1 << PORTC3));
	PORTC = (state & ((1 << PORTC2) | (1 << PORTC3))) | prev;
}

/* interesting, read page 78 figure 14-4 */
uint8_t ll_relays_read(void)
{
	return (PORTC & ((1 << PORTC2) | (1 << PORTC3)));
}

static inline void ll_oc_init(void)
{
	/* output PC0, PC1 */
	DDRC |= (1 << DDC0) | (1 << DDC1);

	/* initial state low */
	PORTC &= ~((1 << PORTC0) | (1 << PORTC1));
}

static inline void ll_inputs_init(void)
{
	/* inputs PD3, PD4, PD5, PD6 */
	DDRD &= ~((1 << DDD3) | (1 << DDD4)
		  | (1 << DDD5) | (1 << DDD6));

	/* disable pull-up */
	PORTD &= ~((1 << PORTD3) | (1 << PORTD4)
		   | (1 << PORTD5) | (1 << PORTD6));

	/* enable Pin Change interrupt for inputs
	 * - See ATmega328P datasheet :
	 * 	-page 88 - table 14-9
	 * 	-page 74 - 13.2.6
	 * 	-page 73 - 13.2.4
	 */
	PCMSK2 |= (1 << PCINT19) | (1 << PCINT20) |
		(1 << PCINT21) | (1 << PCINT22);
	PCICR |= 1 << PCIE2;
}

/* optocoupler */
uint8_t ll_inputs_read(void)
{
	return (PIND & ((1 << PIND3) | (1 << PIND4)
			| (1 << PIND5) | (1 << PIND6))) >> PIND3;
}

void ll_oc_set(uint8_t state)
{
	const uint8_t prev = PORTC & ~((1 << PORTC0) | (1 << PORTC1));
	PORTC = (state & ((1 << PORTC0) | (1 << PORTC1))) | prev;
}

uint8_t ll_oc_read(void)
{
	return (PINC & ((1 << PINC0) | (1 << PINC1))) >> PINC0;
}

static inline void ll_i2c_init(void)
{
	Wire.begin();
}

static inline void dev_tcn75_init(void)
{
	__ASSERT_INTERRUPT();

	Wire.beginTransmission(TCN75_ADDR);
	Wire.write((uint8_t)TCN75_TEMPERATURE_REGISTER);
	Wire.endTransmission();
}

int16_t dev_tcn75_read(void)
{
	uint8_t data[2];
	int16_t temperature;

	Wire.requestFrom(TCN75_ADDR, 2);

	data[0] = Wire.read();
	data[1] = Wire.read();

	temperature = tcn75_temp2int16(data[0], data[1]);

	return temperature;
}

void custompcb_hw_init(void)
{
	ll_relays_init();
	ll_inputs_init();
	ll_oc_init();
	ll_i2c_init();
	dev_tcn75_init();
}