#ifndef _BSP_TINY_H_
#define _BSP_TINY_H_

#include "bsp/bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

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