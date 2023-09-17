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

	uint16_t adc_values[8u];
	float percent[8u];

	for (;;) {
		mcp3008_read_all(adc_values);

		/*
		for (uint8_t i = 0u; i < 8u; i++) {

			const float voltage =
				(adc_values[i] * VREF) / (1 << MCP3008_ADC_RESOLUTION);

			LOG_INF("ADC %u  value: %u,\tvoltage: %.3f V (%.2f "
				"%%)",
				i,
				adc_values[i],
				voltage,
				(100.0f * adc_values[i] / MCP3008_ADC_MAX_VALUE));
		}
		*/

		for (uint8_t i = 0u; i < 8u; i++)
			percent[i] = 100.0f * adc_values[i] / MCP3008_ADC_MAX_VALUE;

		LOG_DBG("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f",
			percent[0],
			percent[1],
			percent[2],
			percent[3],
			percent[4],
			percent[5],
			percent[6],
			percent[7]);

		k_sleep(K_SECONDS(1u));
	}
}

int app_telemetry_handler(struct caniot_device *dev,
			  caniot_endpoint_t ep,
			  const char *buf,
			  uint8_t *len)
{
	return -CANIOT_ENIMPL;
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