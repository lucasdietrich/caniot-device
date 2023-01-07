#if defined(CONFIG_BOARD_V1)

#include "config.h"
#include "v1.h"

#include <stdio.h>

#include <avrtos/drivers/exti.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/kernel.h>
#include <avrtos/logging.h>
#include <avrtos/misc/serial.h>

#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <caniot/datatype.h>
#include <mcp_can.h>
#if defined(CONFIG_BOARD_LOG_LEVEL)
#define LOG_LEVEL CONFIG_BOARD_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define PCINT0_ISR_ENABLED                                                               \
	(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN1) == GPIOB_INDEX) ||                       \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN2) == GPIOB_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN3) == GPIOB_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN4) == GPIOB_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_OC1) == GPIOB_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_OC2) == GPIOB_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_RL1) == GPIOB_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_RL2) == GPIOB_INDEX)

#define PCINT1_ISR_ENABLED                                                               \
	(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN1) == GPIOC_INDEX) ||                       \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN2) == GPIOC_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN3) == GPIOC_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN4) == GPIOC_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_OC1) == GPIOC_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_OC2) == GPIOC_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_RL1) == GPIOC_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_RL2) == GPIOC_INDEX)

#define PCINT2_ISR_ENABLED                                                               \
	(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN1) == GPIOD_INDEX) ||                       \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN2) == GPIOD_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN3) == GPIOD_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_IN4) == GPIOD_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_OC1) == GPIOD_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_OC2) == GPIOD_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_RL1) == GPIOD_INDEX) ||               \
		(BSP_DESCR_GPIO_PORT_GET_INDEX(BSP_RL2) == GPIOD_INDEX)

extern "C" void trigger_telemetry(void);

#if PCINT0_ISR_ENABLED
ISR(PCINT0_vect)
{
#if DEBUG_INT
	serial_transmit('*');
#endif
	trigger_telemetry();

	/* TODO add k_yield_from_isr() */
}
#endif

#if PCINT1_ISR_ENABLED
ISR(PCINT1_vect)
{
#if DEBUG_INT
	serial_transmit('!');
#endif
	trigger_telemetry();

	/* TODO add k_yield_from_isr() */
}
#endif

#if PCINT2_ISR_ENABLED
ISR(PCINT2_vect)
{
#if DEBUG_INT
	serial_transmit('%');
#endif
	trigger_telemetry();

	/* TODO add k_yield_from_isr() */
}
#endif

NOINLINE void bsp_v1_init(void)
{
	/* outputs init */
	bsp_descr_gpio_pin_init(BSP_OC1, GPIO_OUTPUT, GPIO_LOW);
	bsp_descr_gpio_pin_init(BSP_OC2, GPIO_OUTPUT, GPIO_LOW);
	bsp_descr_gpio_pin_init(BSP_RL1, GPIO_OUTPUT, GPIO_LOW);
	bsp_descr_gpio_pin_init(BSP_RL2, GPIO_OUTPUT, GPIO_LOW);

	/* inputs init */
	bsp_descr_gpio_pin_init(BSP_IN1, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_descr_gpio_pin_init(BSP_IN2, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_descr_gpio_pin_init(BSP_IN3, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_descr_gpio_pin_init(BSP_IN4, GPIO_INPUT, GPIO_INPUT_PULLUP);

	/* Enable interrupts */
	pci_configure(PCINT_0_7, 0u);
	pci_configure(PCINT_8_15, 0u);
	pci_configure(PCINT_16_23, 0u);

#if PCINT0_ISR_ENABLED
	pci_clear_flag(PCINT_0_7);
	pci_enable(PCINT_0_7);
#endif
#if PCINT1_ISR_ENABLED
	pci_clear_flag(PCINT_8_15);
	pci_enable(PCINT_8_15);
#endif
#if PCINT2_ISR_ENABLED
	pci_clear_flag(PCINT_16_23);
	pci_enable(PCINT_16_23);
#endif
}

#endif /* CONFIG_BOARD_V1 */