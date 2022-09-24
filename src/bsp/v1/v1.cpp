#if defined(CONFIG_BOARD_V1)

#include "v1.h"

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <avrtos/misc/uart.h>
#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/drivers/exti.h>

#include <caniot/datatype.h>

#include <mcp_can.h>
#include <Wire.h>

#include "config.h"

#include "logging.h"
#if defined(CONFIG_BOARD_LOG_LEVEL)
#	define LOG_LEVEL CONFIG_BOARD_LOG_LEVEL
#else
#	define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define K_MODULE_LL    0x22
#define K_MODULE K_MODULE_LL

#define PORTC_OUTPUT_MASK ((1 << DDC0) | (1 << DDC1) | (1 << DDC2) | (1 << DDC3))
#define PORTD_INPUT_MASK  ((1 << DDD4) | (1 << DDD5) | (1 << DDD6))
#define PORTB_INPUT_MASK  (1u << DDB0)

static void ll_portc_set_mask(uint8_t state, uint8_t mask)
{
	if (mask != 0) {
		const uint8_t prev = GPIOC->PORT & ~mask;
		GPIOC->PORT = prev | (state & mask);
	}
}

/* interesting, read page 78 figure 14-4 */
static inline uint8_t ll_portc_read()
{
	/* read page 77 : "14.2.4 Reading the Pin Value" */
	return GPIOC->PIN;
}

void ll_outputs_set(uint8_t state)
{
	ll_portc_set_mask(state, PORTC_OUTPUT_MASK);
}

void ll_outputs_reset(uint8_t state)
{
	ll_portc_set_mask(state, 0U);
}

void ll_outputs_set_mask(uint8_t state, uint8_t mask)
{
	ll_portc_set_mask(state, mask & PORTC_OUTPUT_MASK);
}

uint8_t ll_outputs_read(void)
{
	return (ll_portc_read() & PORTC_OUTPUT_MASK);
}

void ll_outputs_toggle_mask(uint8_t mask)
{
	GPIOD->PIN |= mask & PORTC_OUTPUT_MASK;
}

extern "C" void trigger_telemetry(void);

#if CONFIG_INPUTS_INT_MASK & BIT(IN1_PIN)
ISR(PCINT0_vect)
{
	trigger_telemetry();
}
#endif 

#if CONFIG_INPUTS_INT_MASK & (BIT(IN2_PIN) | BIT(IN3_PIN) | BIT(IN4_PIN))
ISR(PCINT2_vect)
{
	trigger_telemetry();
}
#endif

/* optocoupler */
uint8_t ll_inputs_read(void)
{
	const uint8_t portd = GPIOD->PIN & PORTD_INPUT_MASK;
	const uint8_t portb = GPIOB->PIN & PORTB_INPUT_MASK;

	return ((portd >> PIND4) << 1u) | (portb >> PINB0);
}

struct board_dio ll_read(void)
{
	struct board_dio s;

	s.outputs = ll_outputs_read();
	s.inputs = ll_inputs_read();

	return s;
}

NOINLINE void bsp_v1_init(void)
{
	/* outputs init */
	bsp_gpio_init(OC1, GPIO_OUTPUT, GPIO_LOW);
	bsp_gpio_init(OC2, GPIO_OUTPUT, GPIO_LOW);
	bsp_gpio_init(RL1, GPIO_OUTPUT, GPIO_LOW);
	bsp_gpio_init(RL2, GPIO_OUTPUT, GPIO_LOW);

	/* inputs init */
	bsp_gpio_init(IN1, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_gpio_init(IN2, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_gpio_init(IN3, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_gpio_init(IN4, GPIO_INPUT, GPIO_INPUT_PULLUP);

	/* Enable interrupts */
	pci_configure(PCINT_0_7, 0u);
	pci_configure(PCINT_8_15, 0u);
	pci_configure(PCINT_16_23, 0u);

	pci_pin_enable_group_line(BSP_GPIO_EXTI_GROUP(IN1), 
				  BSP_GPIO_EXTI_LINE(IN1));

	pci_pin_enable_group_line(BSP_GPIO_EXTI_GROUP(IN2), 
				  BSP_GPIO_EXTI_LINE(IN2));

	pci_pin_enable_group_line(BSP_GPIO_EXTI_GROUP(IN3), 
				  BSP_GPIO_EXTI_LINE(IN3));

	pci_pin_enable_group_line(BSP_GPIO_EXTI_GROUP(IN4), 
				  BSP_GPIO_EXTI_LINE(IN4));

	pci_clear_flag(PCINT_0_7);
	pci_clear_flag(PCINT_8_15);
	pci_clear_flag(PCINT_16_23);
	
	pci_enable(PCINT_0_7);
	pci_enable(PCINT_8_15);
	pci_enable(PCINT_16_23);
}

#endif /* CONFIG_BOARD_V1 */