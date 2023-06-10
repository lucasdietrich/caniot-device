#if defined(CONFIG_BOARD_TINY)

#include "devices/pcf8574.h"
#include "tiny.h"

#include <stdio.h>

#include <avrtos/drivers/exti.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/avrtos.h>
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

struct extio_device extio_devices[CONFIG_EXTIO_DEVICES_COUNT] = {{
	.addr = PCF8574_ADDR, .state = 0u, /* All outputs low */
}};

void bsp_tiny_init(struct extio_device *dev)
{
	/* TODO change to GPIO_LOW */
	const uint8_t state = GPIO_HIGH;

	bsp_descr_gpio_pin_init(BSP_PC0, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PC1, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PC2, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PC3, GPIO_OUTPUT, state);

	bsp_descr_gpio_pin_init(BSP_PD4, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PD5, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PD6, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PD7, GPIO_OUTPUT, state);

	bsp_descr_gpio_pin_init(BSP_PB0, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PE0, GPIO_OUTPUT, state);
	bsp_descr_gpio_pin_init(BSP_PE1, GPIO_OUTPUT, state);

#if CONFIG_PCF8574_ENABLED
	pcf8574_init(dev->addr);

	/* Only initialize external IO once PCF8574 is initialized */

	/* Comment all this */
	// -bsp_descr_gpio_pin_init(BSP_EIO0, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO1, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO2, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO3, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO4, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO5, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO6, GPIO_OUTPUT, state);
	// -bsp_descr_gpio_pin_init(BSP_EIO7, GPIO_OUTPUT, state);

	/* Use this instead */
	bsp_extio_write(dev, 0xFFu, 0x00u);
#endif
}

#if CONFIG_PCF8574_ENABLED

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

void bsp_extio_write(struct extio_device *dev, uint8_t mask, uint8_t value)
{
	dev->state = (dev->state & ~mask) | (value & mask);

	bsp_extio_write_state(dev);
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

	LOG_DBG("extio: write pin %u state %u", pin, state);

	bsp_extio_write_state(dev);
}

void bsp_extio_toggle_pin(struct extio_device *dev, uint8_t pin)
{
	dev->state ^= (1u << pin);

	bsp_extio_write_state(dev);
}

uint8_t bsp_extio_read_pin_state(struct extio_device *dev, uint8_t pin)
{
	const uint8_t r = (pcf8574_get(dev->addr) & (1u << pin)) ? GPIO_HIGH : GPIO_LOW;

	LOG_DBG("extio: read pin %u state %u", pin, r);

	return r;
}

#endif /* CONFIG_PCF8574_ENABLED */

#endif /* CONFIG_BOARD_TINY */