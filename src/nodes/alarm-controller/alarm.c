#include <caniot.h>
#include <device.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

ISR(PCINT2_vect)
{
	usart_transmit('!');
}

struct caniot_config config = {
	.telemetry = {
		.period = 15,
		// .delay = CANIOT_TELEMETRY_DELAY_DEFAULT,
		.delay_min = 50,
		.delay_max = 500,
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
	ll_inputs_enable_pcint(BIT(IN0) | BIT(IN1) | BIT(IN2) | BIT(IN3));
}

void device_process(void)
{
	printf_P(PSTR("device_process()\n"));	
}