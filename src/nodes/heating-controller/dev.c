#include <stdio.h>
#include <avr/pgmspace.h>

#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

#include <avrtos/kernel.h>

#include "bsp/bsp.h"
#include "devices/heater.h"
#include "devices/shutter.h"

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
#define HEATER3_OC_POS_GPIO GPIOC
#define HEATER3_OC_POS_PIN  PIN4
#define HEATER3_OC_NEG_GPIO GPIOC
#define HEATER3_OC_NEG_PIN  PIN5

/* Shutters power supply gpio device and pin */
#define SHUTTERS_POWER_GPIO GPIOD
#define SHUTTERS_POWER_PIN  PIN2

/* Shutter1 optocoupler gpios and pins */
#define SHUTTER1_OC_POS_GPIO GPIOD
#define SHUTTER1_OC_POS_PIN  PIN4
#define SHUTTER1_OC_NEG_GPIO GPIOD
#define SHUTTER1_OC_NEG_PIN  PIN5

/* Shutter2 optocoupler gpios and pins */
#define SHUTTER2_OC_POS_GPIO GPIOB
#define SHUTTER2_OC_POS_PIN  PIN0
#define SHUTTER2_OC_NEG_GPIO GPIOB
#define SHUTTER2_OC_NEG_PIN  PIN1

/* Shutter3 optocoupler gpios and pins */
#define SHUTTER3_OC_POS_GPIO GPIOC
#define SHUTTER3_OC_POS_PIN  PIN0
#define SHUTTER3_OC_NEG_GPIO GPIOC
#define SHUTTER3_OC_NEG_PIN  PIN1

/* Shutter4 optocoupler gpios and pins */
#define SHUTTER4_OC_POS_GPIO GPIOC
#define SHUTTER4_OC_POS_PIN  PIN2
#define SHUTTER4_OC_NEG_GPIO GPIOC
#define SHUTTER4_OC_NEG_PIN  PIN3

const struct pin heaters[CONFIG_HEATERS_COUNT][2u] PROGMEM = {
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

const struct shutters_system_oc ss PROGMEM = {
	.power_oc = PIN_INIT(GPIOD, PIN6),
	.shutters = {
		[SHUTTER1] = SHUTTER_INIT(SHUTTER1_OC_POS_GPIO, SHUTTER1_OC_POS_PIN,
					  SHUTTER1_OC_NEG_GPIO, SHUTTER1_OC_NEG_PIN),
		[SHUTTER2] = SHUTTER_INIT(SHUTTER2_OC_POS_GPIO, SHUTTER2_OC_POS_PIN,
					  SHUTTER2_OC_NEG_GPIO, SHUTTER2_OC_NEG_PIN),
		[SHUTTER3] = SHUTTER_INIT(SHUTTER3_OC_POS_GPIO, SHUTTER3_OC_POS_PIN,
					  SHUTTER3_OC_NEG_GPIO, SHUTTER3_OC_NEG_PIN),
		[SHUTTER4] = SHUTTER_INIT(SHUTTER4_OC_POS_GPIO, SHUTTER4_OC_POS_PIN,
					  SHUTTER4_OC_NEG_GPIO, SHUTTER4_OC_NEG_PIN),
	},
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

	shutters_system_init();
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

		for (uint8_t i = 0u; i < CONFIG_SHUTTERS_COUNT; i++) {
			if (cmds->shutters_openness[i] != CANIOT_SHUTTER_CMD_NONE) {
				shutter_set_openness(i, cmds->shutters_openness[i]);
			}
		}
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