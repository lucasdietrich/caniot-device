#if defined(CONFIG_BOARD_TINY_REVA)

#include "dev.h"
#include "devices/pcf8574.h"
#include "tiny.h"

#include <stdio.h>

#include <avrtos/avrtos.h>
#include <avrtos/drivers/exti.h>
#include <avrtos/drivers/gpio.h>
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

const pin_descr_t bsp_pins[] PROGMEM = {
	BSP_PC0,  BSP_PC1,  BSP_PC2,  BSP_PC3,	BSP_PD4,  BSP_PD5,  BSP_PD6,
	BSP_PD7,  BSP_EIO0, BSP_EIO1, BSP_EIO2, BSP_EIO3, BSP_EIO4, BSP_EIO5,
	BSP_EIO6, BSP_EIO7, BSP_PB0,  BSP_PE0,	BSP_PE1,
};

static struct pcf8574_state pcf_state;

struct extio_device extio_devices[CONFIG_EXTIO_DEVICES_COUNT] = {{
	.addr	= PCF8574_ADDR,
	.state	= 0u, /* All outputs low */
	.device = {.p_pcf = &pcf_state},
}};

#if CONFIG_PCF8574_ENABLED && CONFIG_PCF8574_INT_ENABLED
ISR(BSP_PCF_INT_vect)
{
#if DEBUG_INT
	serial_transmit('#');
#endif

	pcf8574_invalidate_buffer(&pcf_state);

	struct k_thread *ready = trigger_process();

	/* Immediately yield to schedule main thread */
	k_yield_from_isr_cond(ready);
}
#endif

void bsp_tiny_init(struct extio_device *dev)
{
	/* Configure all pins as inputs with no pullup */
	bsp_descr_gpio_pin_init(BSP_PC0, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PC1, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PC2, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PC3, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);

	bsp_descr_gpio_pin_init(BSP_PD4, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PD5, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PD6, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PD7, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);

	bsp_descr_gpio_pin_init(BSP_PB0, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);

#if BSP_PORTE_SUPPORT
	bsp_descr_gpio_pin_init(BSP_PE0, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
	bsp_descr_gpio_pin_init(BSP_PE1, GPIO_INPUT, GPIO_INPUT_NO_PULLUP);
#endif

#if CONFIG_PCF8574_ENABLED
	pcf8574_init(dev->device.p_pcf, dev->addr);

	/* Only initialize external IO once PCF8574 is initialized.
	 * Set all pins as inputs without pullup.
	 */
	bsp_extio_write(dev, 0x00, 0x00);

#if CONFIG_PCF8574_INT_ENABLED
	/* configure PCF interrupt on falling  (active low) */
	bsp_descr_gpio_pin_init(BSP_PCF_INT_DESCR, GPIO_INPUT, GPIO_INPUT_PULLUP);

	exti_clear_flag(BSP_PCF_INT);
	exti_configure(BSP_PCF_INT, ISC_FALLING);
	exti_enable(BSP_PCF_INT);
#endif
#endif
}

#if CONFIG_PCF8574_ENABLED

void bsp_extio_set_pin_direction(struct extio_device *dev, uint8_t pin, uint8_t direction)
{
	if (direction == GPIO_INPUT) {
		dev->state |= (1u << pin);
	}

	pcf8574_set(dev->device.p_pcf, dev->state);
}

static void bsp_extio_write_state(struct extio_device *dev)
{
	pcf8574_set(dev->device.p_pcf, dev->state);
}

void bsp_extio_write(struct extio_device *dev, uint8_t mask, uint8_t value)
{
	dev->state = (dev->state & ~mask) | (value & mask);

	bsp_extio_write_state(dev);
}

uint8_t bsp_extio_read_state(struct extio_device *dev)
{
	return pcf8574_get(dev->device.p_pcf);
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
	const uint8_t r =
		(pcf8574_get(dev->device.p_pcf) & (1u << pin)) ? GPIO_HIGH : GPIO_LOW;

	LOG_DBG("extio: read pin %u state %u", pin, r);

	return r;
}

#endif /* CONFIG_PCF8574_ENABLED */
#endif /* CONFIG_BOARD_TINY_REVA */