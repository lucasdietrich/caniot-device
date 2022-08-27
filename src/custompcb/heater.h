#ifndef _BOARD_HEATER_H_
#define _BOARD_HEATER_H_

#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>

#define HEATERS_COUNT_MAX 3u

#define HEATER1 0u
#define HEATER2 1u
#define HEATER3 1u

#define HEATER_OC_POS 0u
#define HEATER_OC_NEG 1u

#if !CONFIG_KERNEL_DELAY_OBJECT_U32
#error "Heaters controller needs CONFIG_KERNEL_DELAY_OBJECT_U32 to be set"
#endif

#define HEATER_CONFORT_MIN_1_HIGH_DURATION_MS 	(3*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_2_HIGH_DURATION_MS 	(7*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_PERIOD_MS		(300*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_1_LOW_DURATION_MS 	\
	(HEATER_CONFORT_MIN_PERIOD_MS - HEATER_CONFORT_MIN_1_HIGH_DURATION_MS)
#define HEATER_CONFORT_MIN_2_LOW_DURATION_MS 	\
	(HEATER_CONFORT_MIN_PERIOD_MS - HEATER_CONFORT_MIN_2_HIGH_DURATION_MS)

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