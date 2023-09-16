#ifndef _BOARD_HEATER_H_
#define _BOARD_HEATER_H_

#include <avrtos/avrtos.h>
#include <avrtos/drivers/gpio.h>

#define HEATERS_COUNT_MAX 4u

#define HEATER1 0u
#define HEATER2 1u
#define HEATER3 2u
#define HEATER4 3u

#define HEATER_OC_POS 0u
#define HEATER_OC_NEG 1u

typedef enum {
	/**
	 * @brief Comfort mode: Pilot wire, no signal.
	 */
	HEATER_MODE_COMFORT = 0u,

	/**
	 * @brief Comfort mode minus 1 °C.
	 * Pilot wire, complete 3 seconds high per period of 300 seconds.
	 * No signal rest of period.
	 */
	HEATER_MODE_COMFORT_MIN_1, /* Confort mode minus 1 °C */

	/**
	 * @brief Comfort mode minus 2 °C.
	 * Pilot wire, complete 7 seconds high per period of 300 seconds.
	 * No signal rest of period.
	 */
	HEATER_MODE_COMFORT_MIN_2, /* Confort mode minus 2 °C */

	/**
	 * @brief  Energy saving mode.
	 * Pilot wire, complete signal.
	 */
	HEATER_MODE_ENERGY_SAVING, /* Energy saving mode */

	/**
	 * @brief  Frost free mode
	 * Pilot wire, negative phase.
	 */
	HEATER_MODE_FROST_FREE, /* Frost free mode */

	/**
	 * @brief  Off
	 * Pilot wire, positive phase.
	 */
	HEATER_MODE_OFF
} heater_mode_t;

/**
 * @brief Heater control structure to be defined by the application.
 */
extern const uint8_t heaters_io[CONFIG_HEATERS_COUNT][2u] PROGMEM;

/**
 * @brief Initialize all defined heaters
 *
 * @return int
 */
int heaters_init(void);

/**
 * @brief Set heater mode
 *
 * @param hid
 * @param mode
 * @return int
 */
int heater_set_mode(uint8_t hid, heater_mode_t mode);

/**
 * @brief Get heater mode
 *
 * @param hid
 * @return heater_mode_t
 */
heater_mode_t heater_get_mode(uint8_t hid);

#endif /* _BOARD_HEATER_H_ */