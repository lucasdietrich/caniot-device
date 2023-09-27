#include "bsp/bsp.h"
#include "class/class.h"
#include "devices/heater.h"
#include "devices/shutter.h"
#include "pcc.h"

#include <stdio.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/pgmspace.h>
#include <bsp/bsp.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <dev.h>

#define LOG_LEVEL LOG_LEVEL_INF

#define PHASE_CROSSING_COUNTER_ENABLED 1u

#define HEATER1_OC_POS_DESCR BSP_EIO7 /* 1H */
#define HEATER1_OC_NEG_DESCR BSP_EIO6 /* 1L */
#define HEATER2_OC_POS_DESCR BSP_EIO5 /* 2H */
#define HEATER2_OC_NEG_DESCR BSP_EIO4 /* 2L */
#define HEATER3_OC_POS_DESCR BSP_EIO3 /* 3H */
#define HEATER3_OC_NEG_DESCR BSP_EIO2 /* 3L */
#define HEATER4_OC_POS_DESCR BSP_EIO1 /* 4H */
#define HEATER4_OC_NEG_DESCR BSP_EIO0 /* 4L */

#if PHASE_CROSSING_COUNTER_ENABLED
static bool power_current_status = 0u;
#endif

const uint8_t heaters_io[CONFIG_HEATERS_COUNT][2u] PROGMEM = {
	[HEATER1] =
		{
			[HEATER_OC_POS] = HEATER1_OC_POS_DESCR,
			[HEATER_OC_NEG] = HEATER1_OC_NEG_DESCR,
		},
#if CONFIG_HEATERS_COUNT >= 2u
	[HEATER2] =
		{
			[HEATER_OC_POS] = HEATER2_OC_POS_DESCR,
			[HEATER_OC_NEG] = HEATER2_OC_NEG_DESCR,
		},
#endif
#if CONFIG_HEATERS_COUNT >= 3u
	[HEATER3] =
		{
			[HEATER_OC_POS] = HEATER3_OC_POS_DESCR,
			[HEATER_OC_NEG] = HEATER3_OC_NEG_DESCR,
		},
#endif
#if CONFIG_HEATERS_COUNT >= 4u
	[HEATER4] =
		{
			[HEATER_OC_POS] = HEATER4_OC_POS_DESCR,
			[HEATER_OC_NEG] = HEATER4_OC_NEG_DESCR,
		},
#endif
};

void app_init(void)
{
	heaters_init();

#if PHASE_CROSSING_COUNTER_ENABLED
	pcc_init();

	/* Set initial propable state to off */
	power_current_status = 0u;
#endif

	/* Send initial state */
	trigger_telemetry(CANIOT_ENDPOINT_APP);
}

void app_process(void)
{
#if PHASE_CROSSING_COUNTER_ENABLED
	bool new_status = pcc_get_power_status();

	if (new_status != power_current_status) {
		power_current_status = new_status;

		trigger_telemetry(CANIOT_ENDPOINT_APP);
	}

	LOG_DBG("freq: %d status: %u", pcc_get_get_frequency(), new_status);
#endif
}

int app_command_handler(struct caniot_device *dev,
			caniot_endpoint_t ep,
			char *buf,
			uint8_t len)
{
	if (ep == CANIOT_ENDPOINT_APP) {
		struct caniot_heating_control *const cmds =
			(struct caniot_heating_control *)buf;

		if (cmds->heater1_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER1, cmds->heater1_cmd - 1u);
		}
#if CONFIG_HEATERS_COUNT >= 2u
		if (cmds->heater2_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER2, cmds->heater2_cmd - 1u);
		}
#endif
#if CONFIG_HEATERS_COUNT >= 3u
		if (cmds->heater3_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER3, cmds->heater3_cmd - 1u);
		}
#endif
#if CONFIG_HEATERS_COUNT >= 4u
		if (cmds->heater4_cmd != CANIOT_HEATER_NONE) {
			heater_set_mode(HEATER4, cmds->heater4_cmd - 1u);
		}
#endif
	}

	return 0;
}

int app_telemetry_handler(struct caniot_device *dev,
			  caniot_endpoint_t ep,
			  char *buf,
			  uint8_t *len)
{
	if (ep == CANIOT_ENDPOINT_APP) {
		struct caniot_heating_control *const res =
			(struct caniot_heating_control *)buf;

		res->heater1_cmd = heater_get_mode(HEATER1) + 1u;
#if CONFIG_HEATERS_COUNT >= 2u
		res->heater2_cmd = heater_get_mode(HEATER2) + 1u;
#endif
#if CONFIG_HEATERS_COUNT >= 3u
		res->heater3_cmd = heater_get_mode(HEATER3) + 1u;
#endif
#if CONFIG_HEATERS_COUNT >= 4u
		res->heater4_cmd = heater_get_mode(HEATER4) + 1u;
#endif

#if PHASE_CROSSING_COUNTER_ENABLED
		res->power_status = power_current_status;
#endif
		*len = 8u;
	}
	return 0;
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
			.self_managed = BIT(PC1_IDX) | BIT(EIO0_IDX) | BIT(EIO1_IDX) |
					BIT(EIO2_IDX) | BIT(EIO3_IDX) | BIT(EIO4_IDX) |
					BIT(EIO5_IDX) | BIT(EIO6_IDX) | BIT(EIO7_IDX), /* PCC + heaters control */
			.telemetry_on_change = 0x0FF00u, /* Extio only */
			.directions	     = 0x0FF00u, /* Extio as output */
			.outputs_default = 0x0AA00u, /* Positive phase (Heaters off) */
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