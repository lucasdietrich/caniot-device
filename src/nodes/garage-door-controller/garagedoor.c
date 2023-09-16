#include "bsp/bsp.h"
#include "class/class.h"

#include <stdio.h>

#include <avrtos/avrtos.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <dev.h>

#if CONFIG_GPIO_PULSE_SUPPORT != 1
#error "CONFIG_GPIO_PULSE_SUPPORT must be enabled"
#endif

#define RELAY_PULSE_DURATION_MS 500U

#define LEFT_DOOR_COMMAND  RL1_IDX
#define RIGHT_DOOR_COMMAND RL2_IDX

#define LEFT_DOOR_STATUS  IN3_IDX
#define RIGHT_DOOR_STATUS IN4_IDX
#define GATE_STATUS	  IN2_IDX

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

int app_telemetry_handler(struct caniot_device *dev,
			  caniot_endpoint_t ep,
			  char *buf,
			  uint8_t *len)
{
	return -CANIOT_ENIMPL;
}

const struct caniot_device_config default_config PROGMEM = {
	.telemetry =
		{
			.period	   = CANIOT_TELEMETRY_PERIOD_DEFAULT_MS,
			.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT_MS,
			.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT_MS,
		},
	.flags	  = {.error_response	  = 1u,
		     .telemetry_delay_rdm = 1u,
		     .telemetry_endpoint  = CANIOT_ENDPOINT_BOARD_CONTROL},
	.timezone = CANIOT_TIMEZONE_DEFAULT,
	.location =
		{
			.region	 = CANIOT_LOCATION_REGION_DEFAULT,
			.country = CANIOT_LOCATION_COUNTRY_DEFAULT,
		},
	.cls0_gpio = {
		.pulse_durations =
			{
				[RL1_IDX] = RELAY_PULSE_DURATION_MS,
				[RL2_IDX] = RELAY_PULSE_DURATION_MS,
			},
		.outputs_default     = 0u,
		.telemetry_on_change = BIT(RL1_IDX) | BIT(RL2_IDX) | BIT(IN2_IDX) |
				       BIT(IN3_IDX) | BIT(IN4_IDX),
	}};