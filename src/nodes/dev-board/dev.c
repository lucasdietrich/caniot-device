#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "bsp/bsp.h"

#include <avrtos/drivers/timer.h>

ISR(TIMER1_COMPA_vect)
{
	usart_transmit('Q');
}

void app_init(void)
{
	struct timer_config cfg = {
		.mode = TIMER_MODE_CTC,
		.prescaler = TIMER_PRESCALER_1024,
		.counter = TIMER_CALC_COUNTER_VALUE(1000000LU, 1024LU),
		.timsk = BIT(OCIEnA)
	};
	ll_timer16_drv_init(TIMER1_DEVICE, 1U, &cfg);
}

#include <dev.h>

static uint64_t counter = 0;

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep, char *buf,
			uint8_t len)
{
	uint64_t add = 0U;
	memcpy(&add, buf, len);
	
	counter += add;
	
	return 0;
}

int app_telemetry_handler(struct caniot_device *dev, caniot_endpoint_t ep, char *buf, uint8_t *len)
{
	*((uint64_t*) buf) = counter;

	*len = 8U;

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
		.pulse_durations = { 0u, 0u, 0u, 0u},
		.telemetry_on_change = 0u,
		.outputs_default = 0u,
	}
};