#ifndef _BSP_H_
#define _BSP_H_

#include <avr/io.h>

#include <avrtos/drivers/gpio.h>
#include <avrtos/drivers/exti.h>
#include <avrtos/drivers/usart.h>
#include <avrtos/drivers/timer.h>

#include <stdint.h>

/*____________________________________________________________________________*/

struct pin
{
	GPIO_Device *dev;
	uint8_t pin;
};

#define PIN_INIT(_dev, _pin) { .dev = _dev, .pin = _pin }

/*____________________________________________________________________________*/

/* Macros */

/* 0 : 2 = PIN
 * 3 : 6 = PORT
 * 6 = DRIVER 
 * 7 = RESERVED for non-digital purpose
 */

#define BSP_GPIO_PIN_MASK (0x07u)

#define BSP_GPIO_PORT_BIT (3u)
#define BSP_GPIO_PORT_MASK (0x7u << 3u)
#define BSP_GPIO_PORTA (0u << 3u)
#define BSP_GPIO_PORTB (1u << 3u)
#define BSP_GPIO_PORTC (2u << 3u)
#define BSP_GPIO_PORTD (3u << 3u)
#define BSP_GPIO_PORTE (4u << 3u)
#define BSP_GPIO_PORTF (5u << 3u)
#define BSP_GPIO_PORTG (6u << 3u)
#define BSP_GPIO_PORTH (7u << 3u)

#define BSP_GPIO_EXTI0 (0u << 3u)
#define BSP_GPIO_EXTI1 (1u << 3u)
#define BSP_GPIO_EXTI2 (2u << 3u)
#define BSP_GPIO_EXTI3 (3u << 3u)

#define BSP_GPIO_DRIVER_GPIO_MASK (1u << 6u)
#define BSP_GPIO_DRIVER_GPIO (0u << 6u)
#define BSP_GPIO_DRIVER_EXTIO (1u << 6u)

#define BSP_GPIO_STATUS_MASK (1u << 7u)
#define BSP_GPIO_ACTIVE (1u << 7u)
#define BSP_GPIO_RESERVED (0u << 7u)

#define BSP_GPIO_PIN_GET(_descr) ((_descr) & BSP_GPIO_PIN_MASK)
#define BSP_GPIO_PORT_GET_INDEX(_descr) (((_descr) & BSP_GPIO_PORT_MASK) >> BSP_GPIO_PORT_BIT)
#define BSP_GPIO_PORT_DEVICE_GET(_descr) GPIO_DEVICE(BSP_GPIO_PORT_GET_INDEX(_descr))
#define BSP_GPIO_DEVICE_GET(_descr) BSP_GPIO_PORT_DEVICE_GET(_descr)
#define BSP_GPIO_DRIVER_GET(_descr) ((_descr) & BSP_GPIO_DRIVER_GPIO_MASK)
#define BSP_GPIO_STATUS_GET(_descr) ((_descr) & BSP_GPIO_STATUS_MASK)

/*____________________________________________________________________________*/

/* BSP Digital IOs */
#define BSP_GPIO_PB0 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN0 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB1 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN1 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB2 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN2 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB3 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN3 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB4 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN4 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB5 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN5 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB6 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN6 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PB7 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTB | PIN7 | BSP_GPIO_ACTIVE)

#define BSP_GPIO_PC0 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN0 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC1 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN1 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC2 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN2 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC3 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN3 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC4 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN4 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC5 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN5 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC6 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN6 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PC7 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTC | PIN7 | BSP_GPIO_ACTIVE)

#define BSP_GPIO_PD0 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN0 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD1 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN1 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD2 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN2 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD3 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN3 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD4 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN4 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD5 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN5 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD6 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN6 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PD7 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTD | PIN7 | BSP_GPIO_ACTIVE)

#define BSP_GPIO_PE0 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTE | PIN0 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_PE1 (BSP_GPIO_DRIVER_GPIO | BSP_GPIO_PORTE | PIN1 | BSP_GPIO_ACTIVE)

/* Extended IOs (PCF8574) */
#define BSP_GPIO_EIO0 (BSP_GPIO_DRIVER_EXTIO | PIN0 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO1 (BSP_GPIO_DRIVER_EXTIO | PIN1 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO2 (BSP_GPIO_DRIVER_EXTIO | PIN2 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO3 (BSP_GPIO_DRIVER_EXTIO | PIN3 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO4 (BSP_GPIO_DRIVER_EXTIO | PIN4 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO5 (BSP_GPIO_DRIVER_EXTIO | PIN5 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO6 (BSP_GPIO_DRIVER_EXTIO | PIN6 | BSP_GPIO_ACTIVE)
#define BSP_GPIO_EIO7 (BSP_GPIO_DRIVER_EXTIO | PIN7 | BSP_GPIO_ACTIVE)

/* Get EXTI line for given GPIO */
#define BSP_GPIO_EXTI_GROUP(_descr) GPIO_EXTI_GROUP(BSP_GPIO_DEVICE_GET(_descr))
#define BSP_GPIO_EXTI_LINE(_descr) (BIT(BSP_GPIO_PIN_GET(_descr)))

#define BSP_INT0_PIN 		BSP_GPIO_PD2
#define BSP_INT1_PIN 		BSP_GPIO_PD3

/* Uart */
#define BSP_USART_RX 		BSP_GPIO_PD0
#define BSP_USART_TX 		BSP_GPIO_PD1
#define BSP_USART_DEVICE 	USART0_DEVICE
#define BSP_USART 		BSP_USART_DEVICE

/* Can interrupt */
#define BSP_CAN_INT_PIN	  	BSP_INT0_PIN
#define BSP_CAN_INT 		INT0
#define BSP_CAN_INT_vect	INT0_vect
#define BSP_CAN_SS_ARDUINO_PIN  10

/*____________________________________________________________________________*/

/* Board version
 * 1: v1 (custompcb) : TCN75, integrated Relay, OpenCollector
 * 2: tiny : TCN75, extended GPIO
 */

#if defined(CONFIG_BOARD_V1)
#include "v1/v1.h"
#elif defined(CONFIG_BOARD_TINY)
#include "tiny/tiny.h"
#else
#error "Invalid board version"
#endif

/*____________________________________________________________________________*/

/* TODO remove */
typedef uint8_t output_t;
typedef uint8_t input_t;

typedef uint8_t pin_descr_t;

/*____________________________________________________________________________*/

#ifdef __cplusplus
extern "C" {
#endif

void bsp_init(void);

void bsp_gpio_init(uint8_t descr, uint8_t direction, uint8_t state);
void bsp_gpio_output_write(uint8_t descr, uint8_t state);
void bsp_gpio_toggle(uint8_t descr);
uint8_t bsp_gpio_input_read(uint8_t descr);

static inline void inline_bsp_gpio_init(uint8_t descr, uint8_t direction, uint8_t state)
{
	if (BSP_GPIO_DRIVER_GET(descr) == BSP_GPIO_DRIVER_GPIO) {
		GPIO_Device *const port = BSP_GPIO_DEVICE_GET(descr);
		const uint8_t pin = BSP_GPIO_PIN_GET(descr);

		gpio_set_pin_direction(port, pin, direction);
		gpio_write_pin_state(port, pin, state);
	} else {
		/* TODO */
	}
}

static inline void inline_bsp_gpio_output_set(uint8_t descr, uint8_t state)
{
	if (BSP_GPIO_DRIVER_GET(descr) == BSP_GPIO_DRIVER_GPIO) {
		GPIO_Device *const port = BSP_GPIO_DEVICE_GET(descr);
		const uint8_t pin = BSP_GPIO_PIN_GET(descr);

		gpio_write_pin_state(port, pin, state);
	} else {
		/* TODO */
	}
}

static inline void inline_bsp_gpio_toggle(uint8_t descr)
{
	if (BSP_GPIO_DRIVER_GET(descr) == BSP_GPIO_DRIVER_GPIO) {
		GPIO_Device *const port = BSP_GPIO_DEVICE_GET(descr);
		const uint8_t pin = BSP_GPIO_PIN_GET(descr);

		gpio_toggle_pin(port, pin);
	} else {
		/* TODO */
	}
}

static inline uint8_t inline_bsp_gpio_input_get(uint8_t descr)
{
	if (BSP_GPIO_DRIVER_GET(descr) == BSP_GPIO_DRIVER_GPIO) {
		GPIO_Device *const port = BSP_GPIO_DEVICE_GET(descr);
		const uint8_t pin = BSP_GPIO_PIN_GET(descr);

		return gpio_read_pin_state(port, pin);
	} else {
		/* TODO */
	}
}

#ifdef __cplusplus
}
#endif

/*____________________________________________________________________________*/

/* Tiny */

#endif /* _BSP_H_ */