#ifndef _BSP_TINY_H_
#define _BSP_TINY_H_

#include "bsp/bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_IO_COUNT (19u)

#define BSP_PC0 BSP_GPIO_DESCR_PC0
#define BSP_PC1 BSP_GPIO_DESCR_PC1
#define BSP_PC2 BSP_GPIO_DESCR_PC2
#define BSP_PC3 BSP_GPIO_DESCR_PC3

#define BSP_PD4 BSP_GPIO_DESCR_PD4
#define BSP_PD5 BSP_GPIO_DESCR_PD5
#define BSP_PD6 BSP_GPIO_DESCR_PD6
#define BSP_PD7 BSP_GPIO_DESCR_PD7

#define BSP_EIO0 BSP_GPIO_DESCR_EIO0
#define BSP_EIO1 BSP_GPIO_DESCR_EIO1
#define BSP_EIO2 BSP_GPIO_DESCR_EIO2
#define BSP_EIO3 BSP_GPIO_DESCR_EIO3
#define BSP_EIO4 BSP_GPIO_DESCR_EIO4
#define BSP_EIO5 BSP_GPIO_DESCR_EIO5
#define BSP_EIO6 BSP_GPIO_DESCR_EIO6
#define BSP_EIO7 BSP_GPIO_DESCR_EIO7

#define BSP_PB0 BSP_GPIO_DESCR_PB0
#define BSP_PE0 BSP_GPIO_DESCR_PE0
#define BSP_PE1 BSP_GPIO_DESCR_PE1

#define CONFIG_EXTIO_DEVICES_COUNT (1u)

/* PCF interrupt */
#define BSP_PCF_INT_DESCR BSP_INT1_DESCR
#define BSP_PCF_INT	  INT1
#define BSP_PCF_INT_vect  INT1_vect

struct pcf8574_state;

struct extio_device {
	uint8_t addr;
	uint8_t state;

	union {
		/* Currently only PCF8574 is supported */
		struct pcf8574_state *p_pcf;
	} device;
};

extern struct extio_device extio_devices[CONFIG_EXTIO_DEVICES_COUNT];

#define EXTIO_DEVICE(_port) (&extio_devices[_port])

/**
 * @brief Initialize tiny board BSP
 *
 * @param dev
 */
void bsp_tiny_init(struct extio_device *dev);

void bsp_extio_set_pin_direction(struct extio_device *dev,
				 uint8_t pin,
				 uint8_t direction);

void bsp_extio_write(struct extio_device *dev, uint8_t mask, uint8_t value);

uint8_t bsp_extio_read_state(struct extio_device *dev);

void bsp_extio_write_pin_state(struct extio_device *dev, uint8_t pin, uint8_t state);

void bsp_extio_toggle_pin(struct extio_device *dev, uint8_t pin);

uint8_t bsp_extio_read_pin_state(struct extio_device *dev, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_TINY_H_ */