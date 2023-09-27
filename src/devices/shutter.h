#ifndef _BOARD_SHUTTER_H_
#define _BOARD_SHUTTER_H_

#include "bsp/bsp.h"

#include <avrtos/avrtos.h>
#include <avrtos/drivers/gpio.h>

#define SHUTTERS_COUNT_MAX 4u

#define SHUTTER1 0u
#define SHUTTER2 1u
#define SHUTTER3 2u
#define SHUTTER4 3u

#define SHUTTER_OC_POS 0u
#define SHUTTER_OC_NEG 1u

struct shutters_system_oc {
    pin_descr_t power_oc;
    pin_descr_t shutters[CONFIG_SHUTTERS_COUNT][2u];
};

/**
 * @brief Shutter control structure to be defined by the application.
 */
extern const struct shutters_system_oc shutters_io PROGMEM;

#define SHUTTER_INIT(_pos_dev, _pos_pin, _neg_dev, _neg_pin)                             \
    {                                                                                    \
        [SHUTTER_OC_POS] = PIN_INIT_SOC(_pos_dev, _pos_pin), [SHUTTER_OC_NEG] =          \
                                                                 PIN_INIT_SOC(_neg_dev,  \
                                                                              _neg_pin)  \
    }

/**
 * @brief Initialize the shutters system.
 */
int shutters_system_init(void);

/**
 * @brief Set shutter position
 *
 * @param s Shutter index to set the position.
 * @param openness Shutter openness in percent.
 * @return int
 */
int shutter_set_openness(uint8_t s, uint8_t openness);

/**
 * @brief Get shutter position
 *
 * @return int Shutter openness in percent.
 */
int shutter_get_openness(uint8_t s);

#endif /* _BOARD_SHUTTER_H_ */