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
#include "ext_temp.h"

#include <datatype.h>

#include "config.h"

#define K_MODULE_LL    0x22
#define K_MODULE K_MODULE_LL

#define PORTC_OUTPUT_MASK ((1 << DDC0) | (1 << DDC1) | (1 << DDC2) | (1 << DDC3))

void ll_outputs_init(void)
{
	DDRC |= (1 << DDC0) | (1 << DDC1) | (1 << DDC2) | (1 << DDC3);

	/* initial state low */
	PORTC &= ~((1 << PORTC0) | (1 << PORTC1) | (1 << PORTC2) | (1 << PORTC3));
}

static void ll_portc_set_mask(uint8_t state, uint8_t mask)
{
	if (mask != 0) {
		const uint8_t prev = PORTC & ~mask;

		PORTC = prev | (state & mask);
	}
}

/* interesting, read page 78 figure 14-4 */
static inline uint8_t ll_portc_read()
{
	/* read page 77 : "14.2.4 Reading the Pin Value" */
	return PINC;
}

void ll_outputs_set(uint8_t state)
{
	ll_portc_set_mask(state << PORTC0, PORTC_OUTPUT_MASK);
}

void ll_outputs_reset(uint8_t state)
{
	ll_portc_set_mask(state << PORTC0, 0U);
}

void ll_outputs_set_mask(uint8_t state, uint8_t mask)
{
	ll_portc_set_mask(state << PORTC0, mask & PORTC_OUTPUT_MASK);
}

uint8_t ll_outputs_read(void)
{
	return (ll_portc_read() & PORTC_OUTPUT_MASK) >> PORTC0;
}

void ll_outputs_toggle_mask(uint8_t mask)
{
	ll_outputs_set_mask(~ll_portc_read(), mask);
}

static inline void ll_inputs_init(bool pullup)
{
	/* inputs PB0, PD4, PD5, PD6 */
	DDRD &= ~((1 << DDD4)
		  | (1 << DDD5) | (1 << DDD6));
	DDRB &= ~(1 << DDB0);

	/* COnfigure pullup (Datasheet page 76) :
	 * 
	 * "If PORTxn is written logic one when the pin is configured as an input pin, 
	 * the pull-up resistor is activated. To switch the pull-up resistor off, 
	 * PORTxn has to be written logic zero or the pin has to be configured as 
	 * an output pin. The port pins are tri-stated when reset condition 
	 * becomes active, even if no clocks are running."
	 */
	if (pullup) {
		PORTD |= (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6);
		PORTB |= (1 << PORTB0);
	} else {
		PORTD &= ~((1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6));
		PORTB &= ~(1 << PORTB0);
	}
}

extern "C" void trigger_telemetry(void);

#if CONFIG_INPUTS_INT_MASK & BIT(IN1)
ISR(PCINT0_vect)
{
	trigger_telemetry();
}
#endif 

#if CONFIG_INPUTS_INT_MASK & (BIT(IN2) | BIT(IN3) | BIT(IN4))
ISR(PCINT2_vect)
{
	trigger_telemetry();
}
#endif

void ll_inputs_enable_pcint(uint8_t mask)
{
	if (mask != 0) {
		/* enable Pin Change interrupt for inputs
		* - See ATmega328P datasheet :
		* 	-page 88 - table 14-9
		* 	-page 74 - 13.2.6
		* 	-page 73 - 13.2.4
		*/
		if (mask & BIT(IN1)) {
			PCMSK0 |= BIT(PCINT0);
			SET_BIT(PCICR, BIT(PCIE0));
		}
		if (mask & (BIT(IN2) | BIT(IN3) | BIT(IN4))) {
			mask <<= (PCINT20 - 1);
			PCMSK2 |= mask;
			SET_BIT(PCICR, BIT(PCIE2));
		}
	}
}

/* optocoupler */
uint8_t ll_inputs_read(void)
{
	const uint8_t portd = PIND & ((1 << PIND4) | (1 << PIND5) | (1 << PIND6));
	const uint8_t portb = PINB & (1 << PINB0);

	return ((portd >> PIND4) << 1) | (portb >> PINB0);
}

struct board_dio ll_read(void)
{
	struct board_dio s;

	s.outputs = ll_outputs_read();
	s.inputs = ll_inputs_read();

	return s;
}

static inline void ll_i2c_init(void)
{
	Wire.begin();
}

void print_T16(int16_t temp)
{
	printf_P(PSTR("Int Temp (TCN75) : %.1f °C\n"), tcn75_int16tofloat(temp));
}

static void ll_int0_init(bool pullup)
{
	/* INT0 is PortD bit 2 = PD2 */
	DDRD &= ~(1 << DDD2);

	/* COnfigure pullup (Datasheet page 76) :
	 * 
	 * "If PORTxn is written logic one when the pin is configured as an input pin, 
	 * the pull-up resistor is activated. To switch the pull-up resistor off, 
	 * PORTxn has to be written logic zero or the pin has to be configured as 
	 * an output pin. The port pins are tri-stated when reset condition 
	 * becomes active, even if no clocks are running."
	 */
	if (pullup) {
		PORTD |= (1 << PORTD2);
	} else {
		PORTD &= ~(1 << PORTD2);
	}
}

static void ll_int1_init(bool pullup)
{
	/* INT0 is PortD bit 3 = PD3 */
	DDRD &= ~(1 << DDD3);

	/* Configure pullup (Datasheet page 76) :
	 * 
	 * "If PORTxn is written logic one when the pin is configured as an input pin, 
	 * the pull-up resistor is activated. To switch the pull-up resistor off, 
	 * PORTxn has to be written logic zero or the pin has to be configured as 
	 * an output pin. The port pins are tri-stated when reset condition 
	 * becomes active, even if no clocks are running."
	 */
	if (pullup) {
		PORTD |= (1 << PORTD3);
	} else {
		PORTD &= ~(1 << PORTD3);
	}
}

#if CONFIG_APP_OW_EXTTEMP
static bool ow_status = false;
#endif 

void custompcb_hw_init(void)
{
	ll_outputs_init();
	ll_inputs_init(true);
	ll_int0_init(true);
	ll_int1_init(true); /* INT1 is unused */
	ll_i2c_init();

	tcn75_init();

	/* enable interrupts on input changes */
	ll_inputs_enable_pcint(CONFIG_INPUTS_INT_MASK);

#if CONFIG_APP_OW_EXTTEMP
	/* initialize OW */
	bool ow_status = false;
	ow_status = ow_ext_wait_init(K_MSEC(CONFIG_APP_OW_EXTTEMP_INIT_DELAY_MS));
	printf_P(PSTR("<drv> Ext temp sensor "));
	printf_P(ow_status ? PSTR("FOUND\n") : PSTR("NOT found\n"));
#endif
}

void custompcb_hw_process(void)
{
#if CONFIG_APP_OW_EXTTEMP
	/* update ow_status */
	if (ow_status != ow_ext_get(NULL)) {
		ow_status = !ow_status;

		/* if OW thermocouple become available during runtime
		 * telemetry should be triggered */
		if (ow_status == true) {
			trigger_telemetry();
		}
	}
#endif 
}

/* print board_dio struct */
void custompcb_print_io(struct board_dio io)
{
	printf_P(PSTR("outputs: %hhx, inputs: %hhx"),
		 io.outputs, io.inputs);
}