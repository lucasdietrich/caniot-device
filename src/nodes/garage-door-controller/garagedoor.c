#include <caniot.h>
#include <device.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

struct caniot_config config = {
	.telemetry = {
		.period = 60,
		// .delay = CANIOT_TELEMETRY_DELAY_DEFAULT,
		.delay_min = 100,
		.delay_max = 2000,
	},
	.flags = {
		.error_response = 1u,
		.telemetry_delay_rdm = 1u,
		.telemetry_endpoint = CANIOT_TELEMETRY_ENDPOINT_DEFAULT
	}
};

static int telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	ARG_UNUSED(dev);

	if (ep == 0) {
		*((uint32_t *)buf) = k_uptime_get_ms32();
		*len = 4;
	} else {
		*len = 0;
	}

	return 0;
}

const struct caniot_api api = {
	.update_time = NULL,
	.config = {
		.on_read = NULL,
		.written = NULL,
	},
	.custom_attr = {
		.read = NULL,
		.write = NULL,
	},
	.command_handler = NULL,
	.telemetry = telemetry_handler
};

void device_init(void)
{
	custompcb_hw_init();

	printf_P(PSTR("garage_door_controller_init()\n"));
}