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

#include <logging.h>
#if defined(CONFIG_BOARD_LOG_LEVEL)
#	define LOG_LEVEL CONFIG_BOARD_LOG_LEVEL
#else
#	define LOG_LEVEL LOG_LEVEL_NONE
#endif

// @see "init" function from arduino in "wiring.c"
static void hw_ll_init(void)
{
	// on the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)

	// millis used by can library
	SET_BIT(TCCR0A, WGM01);
	SET_BIT(TCCR0A, WGM00);

	// set timer 0 prescale factor to 64
	// this combination is for the standard 168/328/1280/2560
	SET_BIT(TCCR0B, CS01);
	SET_BIT(TCCR0B, CS00);

	// enable timer 0 overflow interrupt
	SET_BIT(TIMSK0, TOIE0);

	// timers 1 and 2 are used for phase-correct hardware pwm
	// this is better for motors as it ensures an even waveform
	// note, however, that fast pwm mode can achieve a frequency of up
	// 8 MHz (with a 16 MHz clock) at 50% duty cycle
#if ARDUINO_ENABLE_FAST_PWM
	TCCR1B = 0;
	SET_BIT(TCCR1B, CS11);
	SET_BIT(TCCR1B, CS10);

	SET_BIT(TCCR1A, WGM10);
#endif

	// set timer 2 prescale factor to 64
	// sbi(TCCR2B, CS22);
	// sbi(TCCR2A, WGM20);

	// set a2d prescaler so we are inside the desired 50-200 KHz range.
#if ARDUINO_ENABLE_ANALOG
	SET_BIT(ADCSRA, ADPS2);
	SET_BIT(ADCSRA, ADPS1);
	SET_BIT(ADCSRA, ADPS0);
	SET_BIT(ADCSRA, ADEN);  // enable disable analog
#endif

	// the bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
	// UCSR0B = 0;

	// enable interrupts before io init
	// sei();
}

void bsp_init(void)
{
	/* General low-level initialisation */
	hw_ll_init();

	/* UART initialisation */
	const struct usart_config usart_config = {
		.baudrate = USART_BAUD_500000,
		.receiver = 0u,
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

	/* configure CAN interrupt on falling on INT0 */
	bsp_gpio_init(BSP_CAN_INT_PIN, GPIO_INPUT, GPIO_INPUT_PULLUP);
	exti_clear_flag(BSP_CAN_INT);
	exti_configure(BSP_CAN_INT, ISC_FALLING);
	exti_enable(BSP_CAN_INT);

	/* Enable interrupts */
	irq_enable();

	/* Board specific initialisation */
#if defined(CONFIG_BOARD_V1)
	bsp_v1_init();
#elif defined(CONFIG_BOARD_TINY)
	bsp_tiny_init();
#endif
}

NOINLINE int bsp_gpio_from_descr(pin_descr_t descr, struct pin *gpio)
{
	__ASSERT_TRUE(gpio);

	if ((BSP_GPIO_STATUS_GET(descr) == BSP_GPIO_ACTIVE) &&
	    (BSP_GPIO_DRIVER_GET(descr) == BSP_GPIO_DRIVER_GPIO)) {
		gpio->dev = BSP_GPIO_DEVICE_GET(descr);
		gpio->pin = BSP_GPIO_PIN_GET(descr);

		LOG_DBG("dev=%x, pin=%u\n", gpio->dev, gpio->pin);

		return 0;
	}

	return -ENOTSUP;
}

void bsp_gpio_init(uint8_t descr, uint8_t direction, uint8_t state)
{
	struct pin gpio;
	if (bsp_gpio_from_descr(descr, &gpio) == 0) {
		gpio_set_pin_direction(gpio.dev, gpio.pin, direction);
		gpio_write_pin_state(gpio.dev, gpio.pin, state);
	}
}

void bsp_gpio_output_write(uint8_t descr, uint8_t state)
{
	struct pin gpio;
	if (bsp_gpio_from_descr(descr, &gpio) == 0) {
		gpio_write_pin_state(gpio.dev, gpio.pin, state);
	}
}

void bsp_gpio_toggle(uint8_t descr)
{
	struct pin gpio;

	if (bsp_gpio_from_descr(descr, &gpio) == 0) {
		gpio_toggle_pin(gpio.dev, gpio.pin);
	}
}

uint8_t bsp_gpio_input_read(uint8_t descr)
{
	struct pin gpio;
	if (bsp_gpio_from_descr(descr, &gpio) == 0) {
		return gpio_read_pin_state(gpio.dev, gpio.pin);
	}

	return 0;
}