#ifndef _BOARD_SHUTTER_H_
#define _BOARD_SHUTTER_H_

#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>

#include "board.h"

#define CONFIG_SHUTTERS_COUNT 4u

#define SHUTTER1 0u
#define SHUTTER2 1u
#define SHUTTER3 2u
#define SHUTTER4 3u

#define SHUTTER_OC_POS 0u
#define SHUTTER_OC_NEG 1u

struct shutters_system_oc {
	struct pin power_oc;
	struct pin shutters[CONFIG_SHUTTERS_COUNT][2u];
};

#define SHUTTER_INIT(_pos_dev, _pos_pin, _neg_dev, _neg_pin) { \
		[SHUTTER_OC_POS] = { .dev = _pos_dev, .pin = _pos_pin }, \
		[SHUTTER_OC_NEG] = { .dev = _neg_dev, .pin = _neg_pin } \
	}


int shutters_system_init(void);

/**
 * @brief Set shutter position
 * 
 * @param s 
 * @param openness 
 * @return int 
 */
int shutter_set_openness(uint8_t s, uint8_t openness);

#endif /* _BOARD_SHUTTER_H_ */