#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

#include <dev.h>

/*
- right : IN4, RL2),
- left  : IN3, RL1),
- gate  : IN2,
*/

#define RELAY_PULSE_DURATION_MS     500U

int app_command_handler(struct caniot_device *dev,
			uint8_t ep, char *buf,
			uint8_t len)
{
	if (AS_CRTHPT(buf)->r1)
		command_output(RL1, CANIOT_XPS_SET_ON);

	if (AS_CRTHPT(buf)->r2)
		command_output(RL2, CANIOT_XPS_SET_ON);

	return 0;
}


int app_telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	return -CANIOT_ENIMPL;
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
				.rl1 = RELAY_PULSE_DURATION_MS,
				.rl2 = RELAY_PULSE_DURATION_MS,
				.oc1 = 0U,
				.oc2 = 0U,
			},
			.mask = {
				.outputs_default = {
					.relays = 0U,
				},
				.telemetry_on_change = {
					.rl1 = 1U,
					.rl2 = 1U,
					.in2 = 1U,
					.in3 = 1U,
					.in4 = 1U,
				}
			}
		}
	}
};