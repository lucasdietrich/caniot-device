#ifndef _BOARD_HEATER_H_
#define _BOARD_HEATER_H_

#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>

#include "devices/gpio.h"

#define HEATERS_COUNT_MAX 3u

#define HEATER1 0u
#define HEATER2 1u
#define HEATER3 1u

#define HEATER_OC_POS 0u
#define HEATER_OC_NEG 1u

#define HEATER_CONFORT_MIN_1_HIGH_DURATION_MS 	(3*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_2_HIGH_DURATION_MS 	(7*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_PERIOD_MS		(300*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_1_LOW_DURATION_MS 	\
	(HEATER_CONFORT_MIN_PERIOD_MS - HEATER_CONFORT_MIN_1_HIGH_DURATION_MS)
#define HEATER_CONFORT_MIN_2_LOW_DURATION_MS 	\
	(HEATER_CONFORT_MIN_PERIOD_MS - HEATER_CONFORT_MIN_2_HIGH_DURATION_MS)

#define HEATER_OC_INIT(_dev, _pin) PIN_INIT_SOC(_dev, _pin)

typedef enum {
	HEATER_MODE_CONFORT = 0u, /* Confort mode */
	HEATER_MODE_CONFORT_MIN_1, /* Confort mode minus 1 °C */
	HEATER_MODE_CONFORT_MIN_2, /* Confort mode minus 2 °C */
	HEATER_MODE_ENERGY_SAVING, /* Energy saving mode */
	HEATER_MODE_FROST_FREE, /* Frost free mode */
	HEATER_MODE_OFF /* Off mode */
} heater_mode_t;

int heaters_init(void);

int heater_set_mode(uint8_t hid, heater_mode_t mode);

heater_mode_t heater_get_mode(uint8_t hid);

#endif /* _BOARD_HEATER_H_ */