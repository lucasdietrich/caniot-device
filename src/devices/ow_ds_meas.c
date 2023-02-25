#include "ow_ds_drv.h"
#include "ow_ds_meas.h"

#include <avrtos/kernel.h>
#include <avrtos/logging.h>

#include <bsp/bsp.h>
#if defined(CONFIG_OW_LOG_LEVEL)
#define LOG_LEVEL CONFIG_OW_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

struct meas_context {
	/* array of sensors ids */
	ow_ds_sensor_t *sensors;

	/* size of the array */
	uint8_t expected : 3;

	/* current ds temperature being measured */
	uint8_t cur : 3;

	/* Flag to indicate that a new discovery is required */
	uint8_t do_discovery : 1;

	/* Flag to indicate that next event should be meas_running */
	uint8_t meas_running : 1;

	/* time to next discovery */
	uint16_t remaining_to_discovery;

	/* period between two measurements
	 * (between two different sensors
	 */
	uint32_t period_ms;

	struct k_work _work;
	struct k_event _ev;
	struct k_sem _sched_sem;
};

static struct meas_context ctx;

static ow_ds_sensor_t *get_sensor_by_id(ow_ds_id_t *id)
{
	ow_ds_sensor_t *sens;

	for (sens = ctx.sensors; sens < ctx.sensors + ctx.expected; sens++) {
		if ((sens->registered == 1U) &&
		    (memcmp(&sens->id, id, sizeof(ow_ds_id_t)) == 0)) {
			return sens;
		}
	}

	return NULL;
}

static ow_ds_sensor_t *get_free_slot(void)
{
	ow_ds_sensor_t *sens;

	for (sens = ctx.sensors; sens < ctx.sensors + ctx.expected; sens++) {
		if (sens->registered == 0U) {
			return sens;
		}
	}

	return NULL;
}

static void event_handler(struct k_event *ev)
{
	k_system_workqueue_submit(&ctx._work);
}

static uint8_t get_active_sensors_count(void)
{
	uint8_t count = 0U;
	ow_ds_sensor_t *sensor;

	for (sensor = ctx.sensors; sensor < ctx.sensors + ctx.expected; sensor++) {
		if (sensor->active == 1U) {
			count++;
		}
	}

	return count;
}

static int8_t measure_sensor(ow_ds_sensor_t *sens)
{
	int8_t ret;
	if (sens->active == 0U) {
		ret = -OW_DS_DRV_SENS_INACTIVE;
	} else if (ow_ds_drv_read(&sens->id, &sens->temp) == OW_DS_DRV_SUCCESS) {

		/* if the sensor has been discovered, try to read it's temperature */
		LOG_INF("ow sens: %p temp: %d.%02u °C",
			(void *)sens,
			sens->temp / 100,
			sens->temp % 100u);

		sens->valid  = 1U;
		sens->errors = 0U;

		ret = 0;

	} else {
		LOG_WRN("ow sens: %p failed", (void *)sens);

		sens->valid = 0U;
		sens->errors++;

		ret = -OW_DS_DRV_SENS_MEAS_FAILED;
	}

	return ret;
}

static bool ds_discovered_cb(ow_ds_id_t *id, void *user_data)
{
	/* try to find the corresponding sensor in the array */
	ow_ds_sensor_t *sensor = get_sensor_by_id(id);

	/* if not already registered/found, try to register it */
	if (sensor == NULL) {
		sensor = get_free_slot();

		if (sensor != NULL) {
			memcpy(&sensor->id, id, sizeof(ow_ds_id_t));

			sensor->registered = 1U;

			/* Invalidate sensor measurement only on first discovery */
			sensor->valid	   = 0U; 
		}
	}

	if (sensor != NULL) {
		/* reset sensor state */
		sensor->errors	    = 0U;
		sensor->active	    = 1U;
		sensor->in_progress = 0U;

		/* Don't invalidate sensor measurement when it is discovered
		 * again */
	}

	LOG_DBG("ds_discovered_cb(id=%p), sensor=%p", id, sensor);

	return sensor != NULL;
}

static int8_t discover()
{
	int8_t ret = ow_ds_drv_discover_iter(ctx.expected, ds_discovered_cb, NULL);

	LOG_DBG("discovered %d OW sensors", ret);

	/* at least one sensor should be discovered */
	ctx.do_discovery = ret <= 0u;

	ctx.remaining_to_discovery = OW_DS_DISCOVERIES_PERIODICITY * ctx.expected;

	return ret;
}

static void meas_handler(struct k_work *w)
{
	int8_t ret;

	const uint8_t diff = ctx.expected - get_active_sensors_count();
	if (ctx.remaining_to_discovery < diff) {
		ctx.do_discovery = 1U;
	} else {
		ctx.remaining_to_discovery -= diff;
	}

	/* discover devices if requested */
	if (ctx.do_discovery == 1U) {
		discover();
	}

	ow_ds_sensor_t *sens = &ctx.sensors[ctx.cur];

	if (sens->active) {
		if (!sens->in_progress) {
			ret = ow_ds_drv_read_start(&sens->id);
			if (ret == OW_DS_DRV_SUCCESS) {
				sens->in_progress = 1U;
				k_event_schedule(&ctx._ev,
						 K_MSEC(OW_DS_MEAS_DURATION_MS));
				return;
			}
		} else {
			ret = ow_ds_drv_read_handle_result(&sens->id, &sens->temp);
			sens->in_progress = 0U;
		}

		if (ret == OW_DS_DRV_SUCCESS) {
			/* if the sensor has been discovered, try to read it's temperature
			 */
			LOG_INF("ow sens: %p temp: %d.%02u °C",
				(void *)sens,
				sens->temp / 100,
				sens->temp % 100u);

			sens->valid  = 1U;
			sens->errors = 0U;
		} else {
			LOG_WRN("ow sens: %p failed", (void *)sens);

			sens->valid = 0U;
			sens->errors++;

			if ((ret == -OW_DS_DRV_SENS_MEAS_FAILED) &&
			    (sens->errors > OW_DS_MAX_CONSECUTIVE_ERRORS)) {

				/* if too many consecutive errors, deactivate the sensor
				 * and trigger a new discovery
				 */
				sens->active	 = 0U;
				sens->errors	 = 0U;
				ctx.do_discovery = 1U;
			}
		}
	}

	/* fetch next sensor */
	ctx.cur = (ctx.cur + 1) % ctx.expected;

	/* check if we continue the periodic measurements or not */
	if (ctx.meas_running == 1U) {
		k_event_schedule(&ctx._ev, K_MSEC(ctx.period_ms));
	} else {
		k_sem_give(&ctx._sched_sem);
	}
}

int8_t ds_init(ow_ds_sensor_t *array, uint8_t count)
{
	int8_t ret = -EINVAL;

	if (array != NULL) {
		ctx.sensors  = array;
		ctx.expected = count;
		// ctx.discovered = 0U;

		ctx.cur		 = 0U;
		ctx.do_discovery = 1U;
		ctx.meas_running = 0U;

		k_work_init(&ctx._work, meas_handler);
		k_event_init(&ctx._ev, event_handler);
		k_sem_init(&ctx._sched_sem, 1U, 1U);

		ret = 0;
	}

	return ret;
}

int8_t ds_meas_start(uint16_t period_ms)
{
	int8_t ret = -OW_DS_DRV_PERIODIC_MEAS_STARTED;

	if (k_sem_take(&ctx._sched_sem, K_NO_WAIT) == 0) {
		ctx.meas_running = 1U;
		ctx.period_ms	 = period_ms;

		if (k_system_workqueue_submit(&ctx._work) != 0) {
			ret = -OW_DS_DRV_SENS_OS_ERROR;
		}
	}

	return ret;
}

int8_t ds_meas_stop(void)
{
	int8_t ret = -OW_DS_DRV_PERIODIC_MEAS_NOT_STARTED;

	if (ctx.meas_running == 1U) {
		ctx.meas_running = 0U;
		ret		 = OW_DS_DRV_SUCCESS;
	}

	return ret;
}

bool ds_meas_running(void)
{
	return ctx.meas_running == 1U;
}

int8_t ds_discover(void)
{
	int8_t ret = -OW_DS_DRV_PERIODIC_MEAS_STARTED;

	if (ds_meas_running() == false) {
		ret = discover();
	}

	return ret;
}

int8_t ds_measure_all(void)
{
	int8_t ret = -OW_DS_DRV_PERIODIC_MEAS_STARTED;

	if (ds_meas_running() == false) {
		uint8_t ret = OW_DS_DRV_SUCCESS;

		/* iterate over all sensors and read their temperature */
		for (uint8_t i = 0; i < ctx.expected; i++) {
			ow_ds_sensor_t *sens = &ctx.sensors[i];

			if (measure_sensor(sens) == OW_DS_DRV_SUCCESS) {
				ret++;
			}
		}
	}

	return ret;
}