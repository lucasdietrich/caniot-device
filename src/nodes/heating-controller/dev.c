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


void app_init(void)
{
	
}

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep, char *buf,
			uint8_t len)
{
	return 0;
}

int app_telemetry_handler(struct caniot_device *dev, caniot_endpoint_t ep, char *buf, uint8_t *len)
{
	return 0;
}

const struct caniot_config default_config PROGMEM = {
	.telemetry = {
		.period = CANIOT_TELEMETRY_PERIOD_DEFAULT,
		.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT,
		.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT,
	},
	.flags = {
		.error_response = 1u,
		.telemetry_delay_rdm = 1u,
		.telemetry_endpoint = CANIOT_ENDPOINT_APP,
	},
	.timezone = CANIOT_TIMEZONE_DEFAULT,
	.location = {
		.region = CANIOT_LOCATION_REGION_DEFAULT,
		.country = CANIOT_LOCATION_COUNTRY_DEFAULT,
	},
	.cls1_gpio = {
		.telemetry_on_change = 0x0FF00u, /* Extio only */
		.directions = 0x0FF00u, /* Extio as output */
		.outputs_default = 0x0AA00u, /* Positive phase (Heaters off) */
		.pulse_durations = {
			[0] = 0x0u,
			[1] = 0x0u,
			[2] = 0x0u,
			[3] = 0x0u,
			[4] = 0x0u,
			[5] = 0x0u,
			[6] = 0x0u,
			[7] = 0x0u,
			[8] = 0x0u,
			[9] = 0x0u,
			[10] = 0x0u,
			[11] = 0x0u,
			[12] = 0x0u,
			[13] = 0x0u,
			[14] = 0x0u,
			[15] = 0x0u,
			[16] = 0x0u,
			[17] = 0x0u,
			[18] = 0x0u,
		},
	},
};