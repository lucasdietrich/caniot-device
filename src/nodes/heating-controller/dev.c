#include <stdio.h>
#include <avr/pgmspace.h>

#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

#include <avrtos/kernel.h>

#include "custompcb/board.h"
#include "custompcb/heater.h"

#include <dev.h>

#include <logging.h>
#define LOG_LEVEL LOG_LEVEL_DBG

/* Heater1 optocoupler gpios and pins */
#define HEATER1_OC_POS_GPIO GPIOD
#define HEATER1_OC_POS_PIN  PIN7
#define HEATER1_OC_NEG_GPIO GPIOD
#define HEATER1_OC_NEG_PIN  PIN3

/* Heater2 optocoupler gpios and pins */
#define HEATER2_OC_POS_GPIO GPIOD
#define HEATER2_OC_POS_PIN  PIN0
#define HEATER2_OC_NEG_GPIO GPIOD
#define HEATER2_OC_NEG_PIN  PIN1

/* Heater3 optocoupler gpios and pins */
#define HEATER3_OC_POS_GPIO GPIOD
#define HEATER3_OC_POS_PIN  PIN4
#define HEATER3_OC_NEG_GPIO GPIOD
#define HEATER3_OC_NEG_PIN  PIN5

#define HEATER1_INIT_MODE HEATER_MODE_OFF
#define HEATER2_INIT_MODE HEATER_MODE_OFF
#define HEATER3_INIT_MODE HEATER_MODE_OFF

const struct heater_oc heaters[CONFIG_HEATERS_COUNT][2u] PROGMEM = {
	[HEATER1] = {
		[HEATER_OC_POS] = HEATER_OC_INIT(HEATER1_OC_POS_GPIO, HEATER1_OC_POS_PIN),
		[HEATER_OC_NEG] = HEATER_OC_INIT(HEATER1_OC_NEG_GPIO, HEATER1_OC_NEG_PIN),
	},
#if CONFIG_HEATERS_COUNT >= 2u
	[HEATER2] = {
		[HEATER_OC_POS] = HEATER_OC_INIT(HEATER2_OC_POS_GPIO, HEATER2_OC_POS_PIN),
		[HEATER_OC_NEG] = HEATER_OC_INIT(HEATER2_OC_NEG_GPIO, HEATER2_OC_NEG_PIN),
	},
#endif
#if CONFIG_HEATERS_COUNT >= 3u
	[HEATER3] = {
		[HEATER_OC_POS] = HEATER_OC_INIT(HEATER3_OC_POS_GPIO, HEATER3_OC_POS_PIN),
		[HEATER_OC_NEG] = HEATER_OC_INIT(HEATER3_OC_NEG_GPIO, HEATER3_OC_NEG_PIN),
	},
#endif
};

heater_mode_t heaters_mode[CONFIG_HEATERS_COUNT] = {
	HEATER1_INIT_MODE,
#if CONFIG_HEATERS_COUNT >= 2u
	HEATER2_INIT_MODE,
#endif
#if CONFIG_HEATERS_COUNT >= 3u
	HEATER3_INIT_MODE,
#endif
};

void app_init(void)
{
	heater_init(HEATER1);

#if CONFIG_HEATERS_COUNT >= 2u
	heater_init(HEATER2);
#endif
#if CONFIG_HEATERS_COUNT >= 3u
	heater_init(HEATER3);
#endif
}

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep, char *buf,
			uint8_t len)
{
	if (ep == CANIOT_ENDPOINT_APP) {
		struct caniot_heating_control *const cmds =
			(struct caniot_heating_control *)buf;

		if (cmds->heater1_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER1, cmds->heater1_cmd - 1u);
		}
#if CONFIG_HEATERS_COUNT >= 2u
		if (cmds->heater2_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER2, cmds->heater2_cmd - 1u);
		}
#endif
#if CONFIG_HEATERS_COUNT >= 3u
		if (cmds->heater3_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER3, cmds->heater3_cmd - 1u);
		}
#endif
	}

	return 0;
}

int app_telemetry_handler(struct caniot_device *dev, caniot_endpoint_t ep, char *buf, uint8_t *len)
{
	return 0;
}

const struct caniot_config default_config PROGMEM = {
	.telemetry = {
		.period = 10U,
		.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT,
		.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT,
	},
	.flags = {
		.error_response = 1u,
		.telemetry_delay_rdm = 1u,
		.telemetry_endpoint = CANIOT_ENDPOINT_BOARD_CONTROL
	},
	.timezone = CANIOT_TIMEZONE_DEFAULT,
	.location = {
		.region = CANIOT_LOCATION_REGION_DEFAULT,
		.country = CANIOT_LOCATION_COUNTRY_DEFAULT,
	},
	.custompcb = {
		.gpio = {
			.pulse_duration = {
				.rl1 = 100u,
				.rl2 = 200u,
				.oc1 = 300u,
				.oc2 = 400u,
			},
			.mask = {
				.outputs_default = {
					.relays = 0U,
				},
				.telemetry_on_change = {
					.mask = 0xFFFFFFFFLU
				}
			}
		}
	}
};