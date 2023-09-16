#include "bsp/bsp.h"
#include "class/class.h"
#include "devices/shutter.h"

#include <stdio.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/pgmspace.h>
#include <bsp/bsp.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <dev.h>

#define LOG_LEVEL LOG_LEVEL_DBG

#define SHUTTER_POWER_DESCR BSP_PB0

#define SHUTTER1_POS_DESCR BSP_EIO7 /* 1H */
#define SHUTTER1_NEG_DESCR BSP_EIO6 /* 1L */
#define SHUTTER2_POS_DESCR BSP_EIO5 /* 2H */
#define SHUTTER2_NEG_DESCR BSP_EIO4 /* 2L */
#define SHUTTER3_POS_DESCR BSP_EIO3 /* 3H */
#define SHUTTER3_NEG_DESCR BSP_EIO2 /* 3L */
#define SHUTTER4_POS_DESCR BSP_EIO1 /* 4H */
#define SHUTTER4_NEG_DESCR BSP_EIO0 /* 4L */

const struct shutters_system_oc shutters_io PROGMEM = {
	.power_oc = SHUTTER_POWER_DESCR,
	.shutters =
		{
			[SHUTTER1] =
				{
					[SHUTTER_OC_POS] = SHUTTER1_POS_DESCR,
					[SHUTTER_OC_NEG] = SHUTTER1_NEG_DESCR,
				},
			[SHUTTER2] =
				{
					[SHUTTER_OC_POS] = SHUTTER2_POS_DESCR,
					[SHUTTER_OC_NEG] = SHUTTER2_NEG_DESCR,
				},
			[SHUTTER3] =
				{
					[SHUTTER_OC_POS] = SHUTTER3_POS_DESCR,
					[SHUTTER_OC_NEG] = SHUTTER3_NEG_DESCR,
				},
			[SHUTTER4] =
				{
					[SHUTTER_OC_POS] = SHUTTER4_POS_DESCR,
					[SHUTTER_OC_NEG] = SHUTTER4_NEG_DESCR,
				},
		},
};

void app_init(void)
{
	shutters_system_init();
}

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep,
			char *buf,
			uint8_t len)
{
	if (ep == CANIOT_ENDPOINT_APP) {
		struct caniot_shutters_control *const cmds =
			(struct caniot_shutters_control *)buf;

		for (uint8_t i = 0u; i < CONFIG_SHUTTERS_COUNT; i++) {
			if (cmds->shutters_openness[i] != CANIOT_SHUTTER_CMD_NONE) {
				shutter_set_openness(i, cmds->shutters_openness[i]);
			}
		}
	}

	return 0;
}

int app_telemetry_handler(struct caniot_device *dev,
			  caniot_endpoint_t ep,
			  char *buf,
			  uint8_t *len)
{
	if (ep == CANIOT_ENDPOINT_APP) {
		struct caniot_shutters_control *const res =
			(struct caniot_shutters_control *)buf;

		for (uint8_t i = 0u; i < CONFIG_SHUTTERS_COUNT; i++) {
			res->shutters_openness[i] = shutter_get_openness(i);
		}

		*len = 4u;
	}
	return 0;
}

const struct caniot_device_config default_config PROGMEM = {
	.telemetry =
		{
			.period	   = CANIOT_TELEMETRY_PERIOD_DEFAULT_MS,
			.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT_MS,
			.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT_MS,
		},
	.flags =
		{
			.error_response	     = 1u,
			.telemetry_delay_rdm = 1u,
			.telemetry_endpoint  = CANIOT_ENDPOINT_APP,
		},
	.timezone = CANIOT_TIMEZONE_DEFAULT,
	.location =
		{
			.region	 = CANIOT_LOCATION_REGION_DEFAULT,
			.country = CANIOT_LOCATION_COUNTRY_DEFAULT,
		},
	.cls1_gpio =
		{
			.telemetry_on_change = 0x0FF00u, /* Extio only */
			.directions	     = 0x0FF00u, /* Extio as output */
			.outputs_default     = 0x00000u, /* All off */
			.pulse_durations =
				{
					[0] = 0x0u,  [1] = 0x0u,  [2] = 0x0u,
					[3] = 0x0u,  [4] = 0x0u,  [5] = 0x0u,
					[6] = 0x0u,  [7] = 0x0u,  [8] = 0x0u,
					[9] = 0x0u,  [10] = 0x0u, [11] = 0x0u,
					[12] = 0x0u, [13] = 0x0u, [14] = 0x0u,
					[15] = 0x0u, [16] = 0x0u, [17] = 0x0u,
					[18] = 0x0u,
				},
		},
};