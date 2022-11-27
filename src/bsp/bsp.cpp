#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/drivers/exti.h>

#include <caniot/datatype.h>

#include <mcp_can.h>
#include <Wire.h>

#include "config.h"

#include "bsp.h"
#include "bsp/tiny/tiny.h"

#include "devices/tcn75.h"
#include "devices/ow_ds_drv.h"

#include <logging.h>
#if defined(CONFIG_BOARD_LOG_LEVEL)
#	define LOG_LEVEL CONFIG_BOARD_LOG_LEVEL
#else
#	define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define ARDUINO_ENABLE_MILLIS

// @see "init" function from arduino in "wiring.c"
static inline void hw_ll_init(void)
{
	irq_enable();
	
#if defined(ARDUINO_ENABLE_MILLIS)
	// On the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)
	TCCR0A |= _BV(WGM01) | _BV(WGM00);

  	// Set timer 0 prescale factor to 64

  	// This combination is for the standard 168/328/640/1280/1281/2560/2561
	TCCR0B |= _BV(CS01) | _BV(CS00);

	// Enable timer 0 overflow interrupt
  	TIMSK0 |= _BV(TOIE0);
#endif

#if defined(ARDUINO_ENABLE_FAST_PWM)
	// Timers 1 and 2 are used for phase-correct hardware pwm
	// this is better for motors as it ensures an even waveform
	// note, however, that fast pwm mode can achieve a frequency of up
	// 8 MHz (with a 16 MHz clock) at 50% duty cycle
	TCCR1B = _BV(CS11); // Set timer 1 prescale factor to 64
	TCCR1B |= _BV(CS10);
	TCCR1A |= _BV(WGM10); // Put timer 1 in 8-bit phase correct pwm mode
#endif

#if defined(ARDUINO_CONFIGURE_TIMERS)
	// Set timer 2 prescale factor to 64
	TCCR2B |= _BV(CS22);

	// Configure timer 2 for phase correct pwm (8-bit)
	TCCR2A |= _BV(WGM20);

	TCCR3B |= _BV(CS31) | _BV(CS30); // Set timer 3 prescale factor to 64
	TCCR3A |= _BV(WGM30);            // Put timer 3 in 8-bit phase correct pwm mode

	TCCR4B |= _BV(CS41) | _BV(CS40); // Set timer 4 prescale factor to 64
	TCCR4A |= _BV(WGM40);            // Put timer 4 in 8-bit phase correct pwm mode
#endif

	// set a2d prescaler so we are inside the desired 50-200 KHz range.
#if defined(ARDUINO_ENABLE_ANALOG)
	ADCSRA = _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADEN);
#endif

	// The bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
	UCSR0B = 0;
}

void bsp_early_init(void)
{
	/* General low-level initialisation */
	hw_ll_init();
}

void bsp_init(void)
{
	/* UART initialisation */
	const struct usart_config usart_config = {
		.baudrate = USART_BAUD_500000,
		.receiver = CONFIG_USART_SHELL ? 1u : 0u,
		.transmitter = 1u,
		.mode = USART_MODE_ASYNCHRONOUS,
		.parity = USART_PARITY_NONE,
		.stopbits = USART_STOP_BITS_1,
		.databits = USART_DATA_BITS_8,
		.speed_mode = USART_SPEED_MODE_NORMAL
	};
	usart_ll_drv_init(BSP_USART, &usart_config);

	/* i2c init */
	Wire.begin();

#if CONFIG_TCN75
	tcn75_init();
#endif

	/* configure CAN interrupt on falling on INT0 */
	bsp_descr_gpio_init(BSP_CAN_INT_DESCR, GPIO_INPUT, GPIO_INPUT_PULLUP);
	
	exti_clear_flag(BSP_CAN_INT);
	exti_configure(BSP_CAN_INT, ISC_FALLING);
	exti_enable(BSP_CAN_INT);

#if CONFIG_OW_DS_ENABLED
	/* initialize OW */
	ow_ds_drv_init(CONFIG_OW_DS_ARDUINO_PIN);
#endif /* CONFIG_OW_DS_ENABLED */

	/* Board specific initialisation */
#if defined(CONFIG_BOARD_V1)
	bsp_v1_init();
#elif defined(CONFIG_BOARD_TINY)
	bsp_tiny_init(EXTIO_DEVICE(0u));
#endif
}

static void get_pin_from_pgm(struct pin *pin, const struct pin *farp_pin)
{
	pin->dev = (void *)pgm_read_word(&farp_pin->dev);
	pin->pin = (uint8_t)pgm_read_byte(&farp_pin->dev + 1u);
}

void bsp_pgm_pin_init(const struct pin *farp_pin, uint8_t direction, uint8_t state)
{
	struct pin pin;
	get_pin_from_pgm(&pin, farp_pin);
	bsp_pin_init(&pin, direction, state);
}

void bsp_pgm_pin_output_write(const struct pin *farp_pin, uint8_t state)
{
	struct pin pin;
	get_pin_from_pgm(&pin, farp_pin);
	bsp_pin_output_write(&pin, state);
}

void bsp_pgm_pin_toggle(const struct pin *farp_pin)
{
	struct pin pin;
	get_pin_from_pgm(&pin, farp_pin);
	bsp_pin_toggle(&pin);
}

uint8_t bsp_pgm_pin_input_read(const struct pin *farp_pin)
{
	struct pin pin;
	get_pin_from_pgm(&pin, farp_pin);
	return bsp_pin_input_read(&pin);
}

void bsp_pin_init(struct pin *pin, uint8_t direction, uint8_t state)
{
	if (BSP_GPIO_PIN_TYPE_GET(pin->pin) == BSP_GPIO_PIN_TYPE_GPIO) {
		gpio_set_pin_direction(
			(GPIO_Device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			direction);
		gpio_write_pin_state(
			(GPIO_Device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			state);
#if CONFIG_EXTIO_ENABLED
	} else {
		__ASSERT_THREAD_CONTEXT();

		bsp_extio_set_pin_direction(
			(struct extio_device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			direction);
		bsp_extio_write_pin_state(
			(struct extio_device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			state);
#endif
	}
}

void bsp_pin_output_write(struct pin *pin, uint8_t state)
{
	if (BSP_GPIO_PIN_TYPE_GET(pin->pin) == BSP_GPIO_PIN_TYPE_GPIO) {
		gpio_write_pin_state(
			(GPIO_Device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			state);
#if CONFIG_EXTIO_ENABLED
	} else {
		__ASSERT_THREAD_CONTEXT();

		bsp_extio_write_pin_state(
			(struct extio_device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			state);
#endif
	}
}

void bsp_pin_toggle(struct pin *pin)
{
	if (BSP_GPIO_PIN_TYPE_GET(pin->pin) == BSP_GPIO_PIN_TYPE_GPIO) {
		gpio_toggle_pin(
			(GPIO_Device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin));
#if CONFIG_EXTIO_ENABLED
	} else {
		__ASSERT_THREAD_CONTEXT();

		bsp_extio_toggle_pin(
			(struct extio_device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin));
#endif
	}
}

uint8_t bsp_pin_input_read(struct pin *pin)
{
	if (BSP_GPIO_PIN_TYPE_GET(pin->pin) == BSP_GPIO_PIN_TYPE_GPIO) {
		return gpio_read_pin_state(
			(GPIO_Device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin));
#if CONFIG_EXTIO_ENABLED
	} else {
		__ASSERT_THREAD_CONTEXT();

		return bsp_extio_read_pin_state(
			(struct extio_device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin));
#endif
	}

	return 0;
}

void bsp_pin_set_direction(struct pin *pin, uint8_t direction)
{
	if (BSP_GPIO_PIN_TYPE_GET(pin->pin) == BSP_GPIO_PIN_TYPE_GPIO) {
		gpio_set_pin_direction(
			(GPIO_Device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			direction);
#if CONFIG_EXTIO_ENABLED
	} else {
		__ASSERT_THREAD_CONTEXT();

		bsp_extio_set_pin_direction(
			(struct extio_device *)pin->dev,
			BSP_GPIO_PIN_GET(pin->pin),
			direction);
#endif
	}
}

static int get_pin_from_descr(uint8_t descr, struct pin *pin)
{
	if (BSP_DESCR_STATUS_GET(descr) == BSP_DESCR_ACTIVE) {
		if (BSP_DESCR_DRIVER_GET(descr) == BSP_DESCR_DRIVER_GPIO) {
			pin->dev = BSP_DESCR_DEVICE_GET(descr);
			pin->pin = BSP_DESCR_GPIO_PIN_GET(descr) | BSP_GPIO_PIN_TYPE_GPIO;
#if CONFIG_EXTIO_ENABLED
		} else if (BSP_DESCR_DRIVER_GET(descr) == BSP_DESCR_DRIVER_EXTIO) {
			pin->dev = EXTIO_DEVICE(BSP_DESCR_GPIO_PORT_GET_INDEX(descr));
			pin->pin = BSP_DESCR_GPIO_PIN_GET(descr) | BSP_GPIO_PIN_TYPE_EXTIO;
#endif
		}
	} else {
		return -ENOTSUP;
	}
	
	LOG_DBG("descr=0x%02x dev=%p pin=%u ext=%u",
		descr, pin->dev, BSP_GPIO_PIN_GET(pin->pin),
		(BSP_GPIO_PIN_TYPE_GET(pin->pin) == BSP_GPIO_PIN_TYPE_EXTIO) ? 1u : 0u);

	return 0;
}

int bsp_descr_gpio_init(uint8_t descr, uint8_t direction, uint8_t state)
{
	int ret;
	struct pin pin;

	ret = get_pin_from_descr(descr, &pin);

	if (ret == 0) {
		bsp_pin_init(&pin, direction, state);
	}

	return ret;
}

int bsp_descr_gpio_output_write(uint8_t descr, uint8_t state)
{
	int ret;
	struct pin pin;

	ret = get_pin_from_descr(descr, &pin);
	if (ret == 0) {
		bsp_pin_output_write(&pin, state);
	}

	return 0;
}

int bsp_descr_gpio_toggle(uint8_t descr)
{
	int ret;
	struct pin pin;

	ret = get_pin_from_descr(descr, &pin);
	if (ret == 0) {
		bsp_pin_toggle(&pin);
	}

	return 0;
}

uint8_t bsp_descr_gpio_input_read(uint8_t descr)
{
	struct pin pin;
	
	if (get_pin_from_descr(descr, &pin) == 0) {
		return bsp_pin_input_read(&pin);
	}

	return 0u;
}

void bsp_descr_gpio_set_direction(uint8_t descr, uint8_t direction)
{
	struct pin pin;
	
	if (get_pin_from_descr(descr, &pin) == 0) {
		return bsp_pin_set_direction(&pin, direction);
	}
}