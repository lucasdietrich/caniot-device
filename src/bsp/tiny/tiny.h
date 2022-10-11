#ifndef _BSP_TINY_H_
#define _BSP_TINY_H_

#include "bsp/bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_IO_COUNT 		(19u)

#define BSP_PC0 BSP_GPIO_DESCR_PC0
#define BSP_PC1 BSP_GPIO_DESCR_PC1
#define BSP_PC2 BSP_GPIO_DESCR_PC2
#define BSP_PC3 BSP_GPIO_DESCR_PC3

#define BSP_PD0 BSP_GPIO_DESCR_PD0
#define BSP_PD1 BSP_GPIO_DESCR_PD1
#define BSP_PD2 BSP_GPIO_DESCR_PD2
#define BSP_PD3 BSP_GPIO_DESCR_PD3

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


#define CONFIG_EXTIO_DEVICES_COUNT	(1u)

struct extio_device
{
	uint8_t addr;
	uint8_t state;
};

extern struct extio_device extio_devices[CONFIG_EXTIO_DEVICES_COUNT];

#define EXTIO_DEVICE(_port) (&extio_devices[_port])

void bsp_tiny_init(struct extio_device *dev);

void bsp_extio_set_pin_direction(struct extio_device *dev, uint8_t pin, uint8_t direction);

void bsp_extio_write_state(struct extio_device *dev);

uint8_t bsp_extio_read_state(struct extio_device *dev);

void bsp_extio_write_pin_state(struct extio_device *dev, uint8_t pin, uint8_t state);

void bsp_extio_toggle_pin(struct extio_device *dev, uint8_t pin);

uint8_t bsp_extio_read_pin_state(struct extio_device *dev, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_TINY_H_ */