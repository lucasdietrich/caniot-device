#ifndef _BOARD_HEATER_H_
#define _BOARD_HEATER_H_

#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>

#define HEATERS_COUNT_MAX 2u

#define HEATER1 0u
#define HEATER2 1u

#define HEATER_OC_POS 0u
#define HEATER_OC_NEG 1u

struct heater_oc {
	GPIO_Device *dev;
	uint8_t pin;
};

#define HEATER_OC_INIT(_dev, _pin) { .dev = _dev, .pin = _pin }

typedef enum {
	HEATER_MODE_CONFORT = 0u,
	HEATER_MODE_CONFORT_MIN_1,
	HEATER_MODE_CONFORT_MIN_2,
	HEATER_MODE_ENERGY_SAVING,
	HEATER_MODE_FROST_FREE,
	HEATER_MODE_OFF
} heater_mode_t;

int heater_init(uint8_t hid);

int heater_set_mode(uint8_t hid, heater_mode_t mode);

#endif /* _BOARD_HEATER_H_ */