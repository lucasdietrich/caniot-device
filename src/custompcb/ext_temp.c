/**
 * @file measurement.c
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Measurement loop
 * @version 0.1
 * @date 2022-01-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <avrtos/kernel.h>

// #include <OneWire.h>

#include <custompcb/board.h>
#include <custompcb/owds.h>

#include <datatype.h>

#include "ext_temp.h"

/* Update measurement after 5 seconds if possible (in seconds) */
#define OW_EXT_TMP_MEASURE_MIN_PERIOD_SEC 5U

/* Time after which measurement is outdated (in seconds) */
#define OW_EXT_TMP_MEASURE_OUTDATED_SEC 60LU

/* Describe an invalid/outdated measurement */
#define OW_EXT_INVALID_VALUE CANIOT_DT_T16_INVALID

int16_t ow_ext_tmp = 0;

enum ow_ext_state {
	OW_EXT_STATE_INIT, /* being initialized */
	OW_EXT_STATE_PENDING, /* measuring */
	OW_EXT_STATE_READY, /* measurement ready */
	OW_EXT_STATE_ERROR, /* any error */
};

static enum ow_ext_state state = OW_EXT_STATE_INIT;

static void measurement_handler(struct k_work *w);

struct ow_ext_context {
	int16_t temperature;
	struct k_sem sem;
	struct k_work work;
};

static struct ow_ext_context ctx = {
	OW_EXT_INVALID_VALUE,
	K_SEM_INIT(ctx.sem, 0, 1),
	K_WORK_INIT(measurement_handler),
};

static void measurement_handler(struct k_work *w)
{
	struct ow_ext_context *ctx = CONTAINER_OF(w, struct ow_ext_context, work);

	int16_t raw;

	if (ow_ds_read(&raw) == true) {
		ctx->temperature = ow_ds_raw_to_T16(raw);
#if DEBUG
		printf_P(PSTR("DS18B20: Temperature : %.2f °C\n"),
			      ow_ds_raw2float(raw));
#endif /* DEBUG */
	} else {
		ctx->temperature = OW_EXT_INVALID_VALUE;

		printf_P(PSTR("OW DS read failed\n"));
	}

	k_sem_give(&ctx->sem);
}

bool ow_ext_get(int16_t *temp)
{
	static uint32_t last_meas = 0U;
	uint32_t now = k_uptime_get();

#if DEBUG
	printf_P(PSTR("OW DS: state = %d last_meas = %lu ow_ext_tmp = %d\n"),
		 state, last_meas, ow_ext_tmp);
#endif 

	switch (state) {
	case OW_EXT_STATE_INIT:
		if (ll_ow_ds_init() == true) {
			state = OW_EXT_STATE_PENDING;
			k_system_workqueue_submit(&ctx.work);	/* first measurement */
		} else {
			state = OW_EXT_STATE_ERROR;
		}
		break;
	case OW_EXT_STATE_PENDING:
		if (k_sem_take(&ctx.sem, K_NO_WAIT) == 0) {
			if (ctx.temperature != OW_EXT_INVALID_VALUE) {
				last_meas = now;
				ow_ext_tmp = ctx.temperature;
				state = OW_EXT_STATE_READY;
			} else {
				k_system_workqueue_submit(&ctx.work);
			}
		}
		break;
	case OW_EXT_STATE_READY:
		if (now - last_meas > OW_EXT_TMP_MEASURE_MIN_PERIOD_SEC) {
			state = OW_EXT_STATE_PENDING;
			k_system_workqueue_submit(&ctx.work);
		}
		break;
	case OW_EXT_STATE_ERROR:
		break;
	default:
		break;
	}

	if (ow_ext_tmp != OW_EXT_INVALID_VALUE) {
		if (now - last_meas < OW_EXT_TMP_MEASURE_OUTDATED_SEC) {
			if (temp != NULL) {
				*temp = ow_ext_tmp;
			}
			return true;
		}

		ow_ext_tmp = OW_EXT_INVALID_VALUE;
	}

	return false;
}