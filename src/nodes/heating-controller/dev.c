#include <stdio.h>
#include <avr/pgmspace.h>

#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

#include <avrtos/kernel.h>

#include <bsp/bsp.h>

#include "bsp/bsp.h"
#include "devices/heater.h"
#include "devices/shutter.h"

#include <dev.h>

#include <logging.h>
#define LOG_LEVEL LOG_LEVEL_DBG

#define HEATER1_OC_POS_DESCR BSP_EIO7 /* 1H */
#define HEATER1_OC_NEG_DESCR BSP_EIO6 /* 1L */
#define HEATER2_OC_POS_DESCR BSP_EIO5 /* 2H */
#define HEATER2_OC_NEG_DESCR BSP_EIO4 /* 2L */
#define HEATER3_OC_POS_DESCR BSP_EIO3 /* 3H */
#define HEATER3_OC_NEG_DESCR BSP_EIO2 /* 3L */
#define HEATER4_OC_POS_DESCR BSP_EIO1 /* 4H */
#define HEATER4_OC_NEG_DESCR BSP_EIO0 /* 4L */ 

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

const uint8_t heaters_io[CONFIG_HEATERS_COUNT][2u] PROGMEM = {
	[HEATER1] = {
		[HEATER_OC_POS] = HEATER1_OC_POS_DESCR,
		[HEATER_OC_NEG] = HEATER1_OC_NEG_DESCR,
	},
#if CONFIG_HEATERS_COUNT >= 2u
	[HEATER2] = {
		[HEATER_OC_POS] = HEATER2_OC_POS_DESCR,
		[HEATER_OC_NEG] = HEATER2_OC_NEG_DESCR,
	},
#endif
#if CONFIG_HEATERS_COUNT >= 3u
	[HEATER3] = {
		[HEATER_OC_POS] = HEATER3_OC_POS_DESCR,
		[HEATER_OC_NEG] = HEATER3_OC_NEG_DESCR,
	},
#endif
#if CONFIG_HEATERS_COUNT >= 4u
	[HEATER4] = {
		[HEATER_OC_POS] = HEATER4_OC_POS_DESCR,
		[HEATER_OC_NEG] = HEATER4_OC_NEG_DESCR,
	},
#endif
};

#if CONFIG_SHUTTERS_COUNT > 0u
const struct shutters_system_oc shutters_io PROGMEM = {
	.power_oc = PIN_INIT_SOC(GPIOD, PIN6),
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
#endif

void app_init(void)
{
	heaters_init();

#if CONFIG_SHUTTERS_COUNT > 0u
	shutters_system_init();
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
#if CONFIG_HEATERS_COUNT >= 4u
		if (cmds->heater4_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER4, cmds->heater4_cmd - 1u);
		}
#endif

#if CONFIG_SHUTTERS_COUNT > 0u
		for (uint8_t i = 0u; i < CONFIG_SHUTTERS_COUNT; i++) {
			if (cmds->shutters_openness[i] != CANIOT_SHUTTER_CMD_NONE) {
				shutter_set_openness(i, cmds->shutters_openness[i]);
			}
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
	.cls0_gpio = {
		.outputs_default = 0u,
		.telemetry_on_change = 0u
	}
};