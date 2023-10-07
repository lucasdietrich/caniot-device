/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BSP_H_
#define _BSP_H_

#include "config.h"
#include "devices/pcf8574.h"

#include <stdint.h>

#include <avrtos/drivers/exti.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/drivers/timer.h>
#include <avrtos/drivers/usart.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

/*____________________________________________________________________________*/

/**
 * @brief Represent a pin on the board associated with its driver.
 */
struct pin {
    /* Driver address if GPIO;,
     * I2C address if PCF8574 */
    void *dev;

    /* first 3 bits contain de actual pin,
     * the 4th bit tells whether the driver is the default GPIO or
     * the GPIO driver */
    uint8_t pin;
};

#define PIN_INIT(_dev, _pin, _soc)                                                       \
    {                                                                                    \
        .dev = _dev, .pin = (_pin) | ((_soc) << 3u)                                      \
    }

#define PIN_INIT_SOC(_dev, _pin) PIN_INIT(_dev, _pin, 1u)
#define PIN_INIT_EXT(_dev, _pin) PIN_INIT(_dev, _pin, 0u)

#define BSP_GPIO_PIN_GET(_pin) ((_pin)&0x07u)

#define BSP_GPIO_PIN_TYPE_MASK      (1u << 3u)
#define BSP_GPIO_PIN_TYPE_GET(_pin) ((_pin)&BSP_GPIO_PIN_TYPE_MASK)
#define BSP_GPIO_PIN_TYPE_GPIO      (0u << 3u)
#define BSP_GPIO_PIN_TYPE_EXTIO     (1u << 3u)

/*____________________________________________________________________________*/

/* Macros */

/* 0 : 2 = PIN
 * 3 : 6 = PORT
 * 6 = DRIVER
 * 7 = RESERVED for non-digital purpose
 */

#define BSP_GPIO_PIN_MASK (0x07u)

#define BSP_GPIO_PORT_BIT  (3u)
#define BSP_GPIO_PORT_MASK (0x7u << 3u)
#define BSP_GPIO_PORTA     (0u << 3u)
#define BSP_GPIO_PORTB     (1u << 3u)
#define BSP_GPIO_PORTC     (2u << 3u)
#define BSP_GPIO_PORTD     (3u << 3u)
#define BSP_GPIO_PORTE     (4u << 3u)
#define BSP_GPIO_PORTF     (5u << 3u)
#define BSP_GPIO_PORTG     (6u << 3u)
#define BSP_GPIO_PORTH     (7u << 3u)

#define BSP_GPIO_EXTI0 (0u << 3u)
#define BSP_GPIO_EXTI1 (1u << 3u)
#define BSP_GPIO_EXTI2 (2u << 3u)
#define BSP_GPIO_EXTI3 (3u << 3u)

#define BSP_DESCR_DRIVER_MASK  (1u << 6u)
#define BSP_DESCR_DRIVER_GPIO  (0u << 6u)
#define BSP_DESCR_DRIVER_EXTIO (1u << 6u)

#define BSP_DESCR_STATUS_MASK (1u << 7u)
#define BSP_DESCR_ACTIVE      (1u << 7u)
#define BSP_DESCR_RESERVED    (0u << 7u)

#define BSP_DESCR_GPIO_PIN_GET(_descr) ((uint8_t)((_descr)&BSP_GPIO_PIN_MASK))
#define BSP_DESCR_GPIO_PORT_GET_INDEX(_descr)                                            \
    (((_descr)&BSP_GPIO_PORT_MASK) >> BSP_GPIO_PORT_BIT)
#define BSP_DESCR_PORT_DEVICE_GET(_descr)                                                \
    GPIO_DEVICE(BSP_DESCR_GPIO_PORT_GET_INDEX(_descr))
#define BSP_DESCR_DEVICE_GET(_descr) BSP_DESCR_PORT_DEVICE_GET(_descr)
#define BSP_DESCR_DRIVER_GET(_descr) ((_descr)&BSP_DESCR_DRIVER_MASK)
#define BSP_DESCR_STATUS_GET(_descr) ((_descr)&BSP_DESCR_STATUS_MASK)

/* Port E is only available with ATmega328PB and not ATmega328P */
#define BSP_PORTE_SUPPORT defined(PORTE)

/*____________________________________________________________________________*/

/* BSP Digital IOs */
#define BSP_GPIO_DESCR_PB0                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN0 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB1                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN1 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB2                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN2 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB3                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN3 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB4                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN4 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB5                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN5 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB6                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN6 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PB7                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTB | PIN7 | BSP_DESCR_ACTIVE)

#define BSP_GPIO_DESCR_PC0                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN0 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC1                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN1 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC2                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN2 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC3                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN3 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC4                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN4 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC5                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN5 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC6                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN6 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PC7                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTC | PIN7 | BSP_DESCR_ACTIVE)

#define BSP_GPIO_DESCR_PD0                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN0 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD1                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN1 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD2                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN2 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD3                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN3 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD4                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN4 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD5                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN5 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD6                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN6 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PD7                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTD | PIN7 | BSP_DESCR_ACTIVE)

#if BSP_PORTE_SUPPORT
#define BSP_GPIO_DESCR_PE0                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTE | PIN0 | BSP_DESCR_ACTIVE)
#define BSP_GPIO_DESCR_PE1                                                               \
    (BSP_DESCR_DRIVER_GPIO | BSP_GPIO_PORTE | PIN1 | BSP_DESCR_ACTIVE)
#else
#define BSP_GPIO_DESCR_PE0 BSP_DESCR_RESERVED
#define BSP_GPIO_DESCR_PE1 BSP_DESCR_RESERVED
#endif

/* Extended IOs (PCF8574) */
#if defined(CONFIG_BOARD_V1)
#define BSP_GPIO_EXTIO_ACTIVE BSP_DESCR_RESERVED
#else
#define BSP_GPIO_EXTIO_ACTIVE BSP_DESCR_ACTIVE
#endif

#define BSP_GPIO_DESCR_EIO0 (BSP_DESCR_DRIVER_EXTIO | PIN0 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO1 (BSP_DESCR_DRIVER_EXTIO | PIN1 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO2 (BSP_DESCR_DRIVER_EXTIO | PIN2 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO3 (BSP_DESCR_DRIVER_EXTIO | PIN3 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO4 (BSP_DESCR_DRIVER_EXTIO | PIN4 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO5 (BSP_DESCR_DRIVER_EXTIO | PIN5 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO6 (BSP_DESCR_DRIVER_EXTIO | PIN6 | BSP_GPIO_EXTIO_ACTIVE)
#define BSP_GPIO_DESCR_EIO7 (BSP_DESCR_DRIVER_EXTIO | PIN7 | BSP_GPIO_EXTIO_ACTIVE)

/* Get EXTI line for given GPIO */
#define BSP_GPIO_PCINT_DESCR_GROUP(_descr) GPIO_PCINT_GROUP(BSP_DESCR_DEVICE_GET(_descr))
#define BSP_GPIO_PCINT_DESCR_LINE(_descr)  (BSP_DESCR_GPIO_PIN_GET(_descr))

#define BSP_INT0_DESCR BSP_GPIO_DESCR_PD2
#define BSP_INT1_DESCR BSP_GPIO_DESCR_PD3

/* Uart */
#define BSP_USART_RX_DESCR BSP_GPIO_DESCR_PD0
#define BSP_USART_TX_DESCR BSP_GPIO_DESCR_PD1
#define BSP_USART_DEVICE   USART0_DEVICE
#define BSP_USART          BSP_USART_DEVICE
#define BSP_USART_RX_vect  USART0_RX_vect

/* Can interrupt */
#define BSP_CAN_INT_DESCR      BSP_INT0_DESCR
#define BSP_CAN_INT            INT0
#define BSP_CAN_INT_vect       INT0_vect
#define BSP_CAN_SS_ARDUINO_PIN 10

/*____________________________________________________________________________*/

/* Board version
 * 1: v1 (custompcb) : TCN75, integrated Relay, OpenCollector
 * 2: tiny : TCN75, extended GPIO
 */

#if defined(CONFIG_BOARD_V1)
#include "v1/v1.h"
#elif defined(CONFIG_BOARD_TINY_REVA)
#include "tiny/tiny.h"
#else
#error "Invalid board version"
#endif

/*____________________________________________________________________________*/

typedef uint8_t pin_descr_t;

/*____________________________________________________________________________*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization basic hardware before C++ constructors are called.
 * Note: https://www.nongnu.org/avr-libc/user-manual/mem_sections.html
 */
__attribute__((naked, used, section(".init5"))) void bsp_early_init(void);

/**
 * @brief Pins supported by the board.
 */
extern const pin_descr_t bsp_pins[] PROGMEM;

/**
 * @brief Initialization of the reset of the hardware, in main when RTOS is running.
 */
void bsp_init(void);

/* Functions for pin structure stored in flash */

/**
 * @brief Initialize a pin from a pin structure stored in flash.
 *
 * @param farp_pin
 * @param direction 1 for output, 0 for input
 * @param state 1 for high, 0 for low
 */
void bsp_pgm_pin_init(const struct pin *farp_pin, uint8_t direction, uint8_t state);

/**
 * @brief Set a pin from a pin structure stored in flash.
 *
 * @param farp_pin
 * @param state 1 for high, 0 for low
 */
void bsp_pgm_pin_output_write(const struct pin *farp_pin, uint8_t state);

/**
 * @brief Toggle a pin from a pin structure stored in flash.
 *
 * @param farp_pin
 */
void bsp_pgm_pin_toggle(const struct pin *farp_pin);

/**
 * @brief Read input pin from pin structure stored in flash
 *
 * @param pin
 * @return uint8_t 1 if high, 0 if low
 */
uint8_t bsp_pgm_pin_input_read(const struct pin *farp_pin);

/* Functions for pin structure stored in ram */

/**
 * @brief Initialize a pin from a pin structure stored in ram.
 *
 * @param pin
 * @param direction 1 for output, 0 for input
 * @param state 1 for high, 0 for low
 */
void bsp_pin_init(struct pin *pin, uint8_t direction, uint8_t state);

/**
 * @brief Set a pin from a pin structure stored in ram.
 *
 * @param pin
 * @param state 1 for high, 0 for low
 */
void bsp_pin_output_write(struct pin *pin, uint8_t state);

/**
 * @brief Toggle a pin from a pin structure stored in ram.
 *
 * @param pin
 */
void bsp_pin_toggle(struct pin *pin);

/**
 * @brief Read input pin
 *
 * @param pin
 * @return uint8_t 1 if high, 0 if low
 */
uint8_t bsp_pin_input_read(struct pin *pin);

/**
 * @brief Set pin direction for a pin structure stored in ram.
 *
 * @param pin
 * @param direction 1 for output, 0 for input
 */
void bsp_pin_set_direction(struct pin *pin, uint8_t direction);

/* Function for pin descriptor */

/**
 * @brief Initialize a pin from a pin descriptor.
 *
 * @param descr
 * @param direction 1 for output, 0 for input
 * @param state 1 for high, 0 for low
 * @return int
 */
int bsp_descr_gpio_pin_init(pin_descr_t descr, uint8_t direction, uint8_t state);

/**
 * @brief Write a pin from a pin descriptor.
 *
 * @param descr
 * @param state 1 for high, 0 for low
 * @return int
 */
int bsp_descr_gpio_output_write(pin_descr_t descr, uint8_t state);

/**
 * @brief Toggle a pin from a pin descriptor.
 *
 * @param descr
 * @return int
 */
int bsp_descr_gpio_toggle(pin_descr_t descr);

/**
 * @brief Read input pin from descriptor
 *
 * @param pin
 * @return uint8_t 1 if high, 0 if low
 */
uint8_t bsp_descr_gpio_input_read(pin_descr_t descr);

/**
 * @brief Set pin direction for a pin descriptor.
 *
 * @param descr
 * @param direction 1 for output, 0 for input
 */
void bsp_descr_gpio_set_direction(pin_descr_t descr, uint8_t direction);

/**
 * @brief Enable or disable PCI for a pin descriptor.
 *
 * @param descr
 * @param state
 */
void bsp_pin_pci_set_enabled(uint8_t descr, uint8_t state);

#ifdef __cplusplus
}
#endif

/*____________________________________________________________________________*/

/* Tiny */

#endif /* _BSP_H_ */