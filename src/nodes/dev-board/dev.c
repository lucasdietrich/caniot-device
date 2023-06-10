#include "bsp/bsp.h"

#include <stdio.h>

#include <avrtos/avrtos.h>
#include <avrtos/drivers/timer.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

ISR(TIMER1_COMPA_vect)
{
	serial_transmit('Q');
}

void app_init(void)
{
	struct timer_config cfg = {.mode      = TIMER_MODE_CTC,
				   .prescaler = TIMER_PRESCALER_1024,
				   .counter = TIMER_CALC_COUNTER_VALUE(1000000LU, 1024LU),
				   .timsk   = BIT(OCIEnA)};
	ll_timer16_init(TIMER1_DEVICE, 1U, &cfg);
}

#include <dev.h>

static uint64_t counter = 0;

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep,
			char *buf,
			uint8_t len)
{
	uint64_t add = 0U;
	memcpy(&add, buf, len);

	counter += add;

	return 0;
}

int app_telemetry_handler(struct caniot_device *dev,
			  caniot_endpoint_t ep,
			  char *buf,
			  uint8_t *len)
{
	*((uint64_t *)buf) = counter;

	*len = 8U;

	return 0;
}

const struct caniot_device_config default_config PROGMEM = {
	.telemetry =
		{
			.period	   = 10000u,
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
#if defined(CONFIG_BOARD_V1)
	.cls0_gpio =
		{
			.pulse_durations     = {0u, 0u, 0u, 0u},
			.telemetry_on_change = 0u,
			.outputs_default     = 0u,
		}
#elif defined(CONFIG_BOARD_TINY)
	.cls1_gpio =
		{
			.pulse_durations     = {100u,  200u,  300u,  400u,  500u,
						600u,  700u,  800u,  900u,  1000u,
						1100u, 1200u, 1300u, 1400u, 1500u,
						1600u, 1700u, 1800u, 1900u, 2000u},
			.outputs_default     = 0x0007fffflu, /* state high */
			.directions	     = 0x0007fffflu, /* outputs*/
			.telemetry_on_change = 0x0,	     /* No Change */
		}
#endif
};