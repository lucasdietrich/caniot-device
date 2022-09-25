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

#define PCINT0_vect_ENABLED \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN1) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN2) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN3) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN4) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_OC1) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_OC2) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_RL1) == GPIOB_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_RL2) == GPIOB_INDEX)

#define PCINT1_vect_ENABLED \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN1) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN2) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN3) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN4) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_OC1) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_OC2) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_RL1) == GPIOC_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_RL2) == GPIOC_INDEX)

#define PCINT2_vect_ENABLED \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN1) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN2) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN3) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_IN4) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_OC1) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_OC2) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_RL1) == GPIOD_INDEX) || \
	(BSP_GPIO_PORT_GET_INDEX(BSP_RL2) == GPIOD_INDEX)

extern "C" void trigger_telemetry(void);

#if PCINT0_vect_ENABLED
ISR(PCINT0_vect)
{
	usart_transmit('*');
	trigger_telemetry();
}
#endif

#if PCINT1_vect_ENABLED
ISR(PCINT1_vect)
{
	usart_transmit('!');
	trigger_telemetry();
}
#endif

#if PCINT2_vect_ENABLED
ISR(PCINT2_vect)
{
	usart_transmit('%');
	trigger_telemetry();
}
#endif

NOINLINE void bsp_v1_init(void)
{
	/* outputs init */
	bsp_gpio_init(BSP_OC1, GPIO_OUTPUT, GPIO_LOW);
	bsp_gpio_init(BSP_OC2, GPIO_OUTPUT, GPIO_LOW);
	bsp_gpio_init(BSP_RL1, GPIO_OUTPUT, GPIO_LOW);
	bsp_gpio_init(BSP_RL2, GPIO_OUTPUT, GPIO_LOW);

	/* inputs init */
	bsp_gpio_init(BSP_IN1, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_gpio_init(BSP_IN2, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_gpio_init(BSP_IN3, GPIO_INPUT, GPIO_INPUT_PULLUP);
	bsp_gpio_init(BSP_IN4, GPIO_INPUT, GPIO_INPUT_PULLUP);

	/* Enable interrupts */
	pci_configure(PCINT_0_7, 0u);
	pci_configure(PCINT_8_15, 0u);
	pci_configure(PCINT_16_23, 0u);
	
#if PCINT0_vect_ENABLED
	pci_clear_flag(PCINT_0_7);
	pci_enable(PCINT_0_7);
#endif
#if PCINT1_vect_ENABLED
	pci_clear_flag(PCINT_8_15);
	pci_enable(PCINT_8_15);
#endif
#if PCINT2_vect_ENABLED
	pci_clear_flag(PCINT_16_23);
	pci_enable(PCINT_16_23);
#endif
}

#endif /* CONFIG_BOARD_V1 */