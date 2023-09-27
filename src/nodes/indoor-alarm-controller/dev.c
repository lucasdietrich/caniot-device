#include "bsp/bsp.h"
#include "class/class.h"
#include "devices/mcp3008.h"

#include <stdio.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <dev.h>

#define LOG_LEVEL LOG_LEVEL_DBG

// TODO add lock mecanism (atomic/semaphore) to make sure we sent valid/recent ADC data
// over telemetry
/** Tell whether the ADC should read new ADC values or not */
// static atomic_t adc_flags = K_ATOMIC_INIT(1u);

struct adc_channel {
	uint16_t value;
};

static uint16_t channels[8u];

static void adc_task(void *arg);
Z_THREAD_DEFINE(adc, adc_task, 256u, K_COOPERATIVE, NULL, 'A', 0);

#define VREF 4.886f

void app_init(void)
{
	mcp3008_init();

	k_thread_start(&adc);
}

void adc_task(void *arg)
{
	(void)arg;

	for (;;) {
		mcp3008_read_all(channels);

		trigger_telemetrys(BIT(CANIOT_ENDPOINT_1) | BIT(CANIOT_ENDPOINT_2));
		k_sleep(K_SECONDS(1u));
	}
}

int app_telemetry_handler(struct caniot_device *dev,
			  caniot_endpoint_t ep,
			  const char *buf,
			  uint8_t *len)
{
	uint16_t *p_chans;
	switch (ep) {
	case CANIOT_ENDPOINT_1:
		p_chans = &channels[0u];
		break;
	case CANIOT_ENDPOINT_2:
		p_chans = &channels[4u];
		break;
	default:
		return -CANIOT_ENIMPL;
	}

	*len = 8u;
	for (uint8_t i = 0; i < 4u; i++) {
		sys_write_le16(&buf[i << 1u], p_chans[i]);
	}

	return 0;
}

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep,
			char *buf,
			uint8_t len)
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
	.flags =
		{
			.error_response	     = 1u,
			.telemetry_delay_rdm = 1u,
			.telemetry_endpoint  = CANIOT_ENDPOINT_BOARD_CONTROL,
			.telemetry_periodic_enabled  = 1u,
		},
	.timezone = CANIOT_TIMEZONE_DEFAULT,
	.location =
		{
			.region	 = CANIOT_LOCATION_REGION_DEFAULT,
			.country = CANIOT_LOCATION_COUNTRY_DEFAULT,
		},
	.cls1_gpio =
		{
			.self_managed	     = BIT(PB0_IDX),
			.telemetry_on_change = 0x0000u,
			.directions	     = 0x0000u,
			.outputs_default     = 0x0000u,
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