#ifndef _CUSTOMPCB_GPIO_H_
#define _CUSTOMPCB_GPIO_H_

#include <stdint.h>

#include <avrtos/drivers/gpio.h>

struct pin
{
	GPIO_Device *dev;
	uint8_t pin;
};

#define PIN_INIT(_dev, _pin) { .dev = _dev, .pin = _pin }

#endif /* _CUSTOMPCB_GPIO_H_ */