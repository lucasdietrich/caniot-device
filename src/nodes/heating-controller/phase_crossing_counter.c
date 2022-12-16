#include <avrtos/kernel.h>
#include <avrtos/drivers/exti.h>

#include <bsp/bsp.h>

#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_DEBUG

/* On board PC1 */
// #define PHASE_ZERO_CROSSING_COUNTER_PORT 	PORTC
// #define PHASE_ZERO_CROSSING_COUNTER_PIN  	PINC1
// #define PHASE_ZERO_CROSSING_COUNTER_DESCR  	BSP_PC1

/* For dev PB0 */
#define PHASE_ZERO_CROSSING_COUNTER_PORT 		GPIOB
#define PHASE_ZERO_CROSSING_COUNTER_PIN  		PINB0
#define PHASE_ZERO_CROSSING_COUNTER_DESCR  		BSP_PB0
#define PHASE_ZERO_CROSSING_COUNTER_PCINT  		PCINT0
#define PHASE_ZERO_CROSSING_COUNTER_PCINT_GROUP  	PCINT_0_7

#define FREQ_TOLERANCE	5u
#define FREQ_EXPECTED	50u

volatile uint8_t counter = 0u;
volatile uint8_t last_counter = 0u;
volatile uint8_t freq = 0u;

ISR(PCINT_0_7_vect)
{
	counter++;
}

ISR(TIMER1_COMPA_vect)
{
	const uint8_t delta = counter - last_counter;
	last_counter = counter;
	freq = delta >> 1u;
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
		.mode = TIMER_MODE_CTC,
		.prescaler = TIMER_PRESCALER_1024,
		.counter = TIMER_CALC_COUNTER_VALUE(1000000LU, 1024LU),
		.timsk = BIT(OCIEnA)
	};
	ll_timer16_init(TIMER1_DEVICE, 1U, &cfg);
}

uint8_t pcc_get_current_frequency(void)
{
	return freq;
}