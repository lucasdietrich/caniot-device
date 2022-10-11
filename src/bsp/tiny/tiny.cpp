#if defined(CONFIG_BOARD_TINY)

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

#include "tiny.h"

#include "devices/pcf8574.h"

struct extio_device extio_devices[CONFIG_EXTIO_DEVICES_COUNT] = {
	{
		.addr = PCF8574_ADDR,
		.state = 0u, /* All outputs low */
	}
};

void bsp_tiny_init(struct extio_device *dev)
{
	/* TODO change to GPIO_LOW */
	const uint8_t state = GPIO_HIGH;

	bsp_descr_gpio_init(BSP_PC0, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PC1, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PC2, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PC3, GPIO_OUTPUT, state);

	bsp_descr_gpio_init(BSP_PD0, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PD1, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PD2, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PD3, GPIO_OUTPUT, state);

	bsp_descr_gpio_init(BSP_PB0, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PE0, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_PE1, GPIO_OUTPUT, state);

#if CONFIG_PCF8574
	pcf8574_init(dev->addr);
#endif

	/* Only initialize external IO once PCF8574 is initialized */
	bsp_descr_gpio_init(BSP_EIO0, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO1, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO2, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO3, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO4, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO5, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO6, GPIO_OUTPUT, state);
	bsp_descr_gpio_init(BSP_EIO7, GPIO_OUTPUT, state);
}

void bsp_extio_set_pin_direction(struct extio_device *dev, uint8_t pin, uint8_t direction)
{
	if (direction == GPIO_INPUT) {
		dev->state |= (1u << pin);
	}

	pcf8574_set(dev->addr, dev->state);
}

void bsp_extio_write_state(struct extio_device *dev)
{
	pcf8574_set(dev->addr, dev->state);
}

uint8_t bsp_extio_read_state(struct extio_device *dev)
{
	return pcf8574_get(dev->addr);
}

void bsp_extio_write_pin_state(struct extio_device *dev, uint8_t pin, uint8_t state)
{
	if (state == GPIO_HIGH) {
		dev->state |= (1u << pin);
	} else {
		dev->state &= ~(1u << pin);
	}

	bsp_extio_write_state(dev);
}

void bsp_extio_toggle_pin(struct extio_device *dev, uint8_t pin)
{
	dev->state ^= (1u << pin);

	bsp_extio_write_state(dev);
}

uint8_t bsp_extio_read_pin_state(struct extio_device *dev, uint8_t pin)
{
	return (pcf8574_get(dev->addr) & (1u << pin)) ? GPIO_HIGH : GPIO_LOW;
}

#endif /* CONFIG_BOARD_TINY */