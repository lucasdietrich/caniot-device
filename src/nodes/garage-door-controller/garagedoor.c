#include <stdio.h>
#include <avr/pgmspace.h>

#include <avrtos/kernel.h>

#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>
#include <caniot/classes/class0.h>

#include <dev.h>

#include "bsp/bsp.h"

void app_init(void)
{
	
}

#define RELAY_PULSE_DURATION_MS     	500U

#define LEFT_DOOR_COMMAND 		RL1_IDX
#define RIGHT_DOOR_COMMAND 		RL2_IDX

#define LEFT_DOOR_STATUS 		IN3_IDX
#define RIGHT_DOOR_STATUS 		IN4_IDX
#define GATE_STATUS			IN2_IDX

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep,
			const char *buf,
			uint8_t len)
{
	return -CANIOT_ENIMPL;
}

void app_process(void)
{

}


int app_telemetry_handler(struct caniot_device *dev, caniot_endpoint_t ep, char *buf, uint8_t *len)
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
	.cls0_gpio = {
		.pulse_durations = {
			[RL1_IDX] = RELAY_PULSE_DURATION_MS,
			[RL2_IDX] = RELAY_PULSE_DURATION_MS,
		},
		.outputs_default = 0u,
		.telemetry_on_change = 0x7C, /* RL1, RL2, IN2, IN3, IN4 */
	}
};