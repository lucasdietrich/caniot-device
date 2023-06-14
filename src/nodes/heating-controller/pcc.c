/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* PCC: Phase Crossing Counter */

#include <avrtos/avrtos.h>
#include <avrtos/drivers/exti.h>
#include <avrtos/logging.h>

#include <bsp/bsp.h>
#define LOG_LEVEL LOG_LEVEL_DEBUG

/* For dev PB0 */
// #define PHASE_ZERO_CROSSING_COUNTER_PORT	GPIOB
// #define PHASE_ZERO_CROSSING_COUNTER_PIN		PINB0
// #define PHASE_ZERO_CROSSING_COUNTER_DESCR	BSP_PB0
// #define PHASE_ZERO_CROSSING_COUNTER_PCINT	PCINT0
// #define PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP PCINT_0_7
// #define PHASE_ZERO_CROSSING_COUNTER_PCINT_VECT PCINT_0_7_vect

/* On board PC1 */
#define PHASE_ZERO_CROSSING_COUNTER_PORT	GPIOC
#define PHASE_ZERO_CROSSING_COUNTER_PIN		PINC1
#define PHASE_ZERO_CROSSING_COUNTER_DESCR	BSP_PC1
#define PHASE_ZERO_CROSSING_COUNTER_PCINT	PCINT9
#define PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP PCINT_8_15
#define PHASE_ZERO_CROSSING_COUNTER_PCINT_VECT	PCINT_8_15_vect

#define FREQ_TOLERANCE 5u
#define FREQ_EXPECTED  50u

/* We assume maxium expected ISR(PHASE_ZERO_CROSSING_COUNTER_PCINT_VECT) frequency
 * is 255Hz, so we can use 8 bits for the counters
 */
volatile uint8_t counter      = 0u;
volatile uint8_t last_counter = 0u;
volatile uint8_t freq	      = 0u;

ISR(PHASE_ZERO_CROSSING_COUNTER_PCINT_VECT)
{
#if DEBUG_INT
	serial_transmit(':');
#endif
	counter++;
}

ISR(TIMER1_COMPA_vect)
{
#if DEBUG_INT
	serial_transmit('_');
#endif
	const uint8_t delta = counter - last_counter;
	last_counter	    = counter;

	/* 2 edges per pulse: rising/falling
	 * 2 pulses per period: (1 pulse per zero crossing)
	 * so: freq = int_count / 4
	 */
	freq = delta >> 2u;
}

void pcc_init(void)
{
	gpio_pin_init(PHASE_ZERO_CROSSING_COUNTER_PORT,
		      PHASE_ZERO_CROSSING_COUNTER_PIN,
		      GPIO_INPUT,
		      GPIO_INPUT_NO_PULLUP);

	pci_configure(PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP,
		      1 << PHASE_ZERO_CROSSING_COUNTER_PCINT);
	pci_clear_flag(PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP);
	pci_pin_enable_group_line(PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP,
				  PHASE_ZERO_CROSSING_COUNTER_PCINT);
	pci_enable(PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP);

	struct timer_config cfg = {
		.mode	   = TIMER_MODE_CTC,
		.prescaler = TIMER_PRESCALER_1024,
		.counter   = TIMER_CALC_COUNTER_VALUE(1000000lu, 1024lu), // 1s
		.timsk	   = BIT(OCIEnA),
	};
	ll_timer16_init(TIMER1_DEVICE, 1u, &cfg);
}

uint8_t pcc_get_get_frequency(void)
{
	return freq;
}

bool pcc_get_power_status(void)
{
	return (freq > FREQ_EXPECTED - FREQ_TOLERANCE) &&
	       (freq < FREQ_EXPECTED + FREQ_TOLERANCE);
}