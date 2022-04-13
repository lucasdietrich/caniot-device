#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

#include <dev.h>

static uint64_t counter = 0;

int app_command_handler(struct caniot_device *dev,
			uint8_t ep, char *buf,
			uint8_t len)
{
	uint64_t add = 0U;
	memcpy(&add, buf, len);
	
	counter += add;
	
	return 0;
}

int app_telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	*((uint64_t*) buf) = counter;

	*len = 8U;

	return 0;
}

struct caniot_config config = {
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
				.rl1 = 500U,
				.rl2 = 1500U,
				.oc1 = 5000U,
				.oc2 = 30000U,
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