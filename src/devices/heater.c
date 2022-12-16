#if CONFIG_HEATERS_COUNT

#include "heater.h"
#include "config.h"

#include "bsp/bsp.h"

#include <avr/pgmspace.h>

#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_WRN

#if !CONFIG_KERNEL_DELAY_OBJECT_U32
#error "Heaters controller needs CONFIG_KERNEL_DELAY_OBJECT_U32 to be set"
#endif

#if !CONFIG_KERNEL_EVENTS
#error "Heaters controller needs CONFIG_KERNEL_EVENTS to be set"
#endif

#if CONFIG_EXTIO_ENABLED && !CONFIG_WORKQUEUE_HEATERS_EXECUTION
#error "CONFIG_EXTIO_ENABLED requires CONFIG_WORKQUEUE_HEATERS_EXECUTION"
#endif

#if CONFIG_WORKQUEUE_HEATERS_EXECUTION && !CONFIG_SYSTEM_WORKQUEUE_ENABLE
#error "Heaters controller needs CONFIG_SYSTEM_WORKQUEUE_ENABLE to be set"
#endif

#define HEATER_CONFORT_MIN_1_ACTIVE_DURATION_MS 	(3*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_2_ACTIVE_DURATION_MS 	(7*MSEC_PER_SEC)
#define HEATER_CONFORT_MIN_PERIOD_MS			(300*MSEC_PER_SEC)
// #define HEATER_CONFORT_MIN_PERIOD_MS			(20*MSEC_PER_SEC) // For testing only

#define HEATER_CONFORT_MIN_1_INACTIVE_DURATION_MS \
	(HEATER_CONFORT_MIN_PERIOD_MS - HEATER_CONFORT_MIN_1_ACTIVE_DURATION_MS)
#define HEATER_CONFORT_MIN_2_INACTIVE_DURATION_MS \
	(HEATER_CONFORT_MIN_PERIOD_MS - HEATER_CONFORT_MIN_2_ACTIVE_DURATION_MS)

struct heater
{
	/* Event used to schedule the next state change
	 *
	 * Note: Keep event as first member of the structure
	 * for optomization purposes
	 */
	struct k_event event;

#if CONFIG_WORKQUEUE_HEATERS_EXECUTION
	/* Work used to schedule the next state change */
	struct k_work work;
#endif

	heater_mode_t mode;
};

/* Heaters state */
static struct heater hs[CONFIG_HEATERS_COUNT];

#define HEATER_INDEX(_hp) ((_hp) - hs)

#define COMPLEMENT(_x) ((_x) ? 0u : 1u)

static uint8_t pin_descr_get(uint8_t heater, uint8_t pin)
{
	return pgm_read_byte(&heaters_io[heater][pin]);
}

static inline void heater_activate_oc(pin_descr_t descr)
{
	bsp_descr_gpio_output_write(descr, GPIO_LOW);
}

static inline void heater_deactivate_oc(pin_descr_t descr)
{
	bsp_descr_gpio_output_write(descr, GPIO_HIGH);
}

static inline void heater_set_active(pin_descr_t descr, uint8_t active)
{
	bsp_descr_gpio_output_write(descr, COMPLEMENT(active));
}

static inline uint8_t heater_oc_is_active(pin_descr_t descr)
{
	return COMPLEMENT(bsp_descr_gpio_input_read(descr));
}

void heater_ev_cb(struct k_event *ev)
{
	struct heater *const heater = CONTAINER_OF(ev, struct heater, event);
	const uint8_t heater_index = HEATER_INDEX(heater);

	const pin_descr_t pos = pin_descr_get(heater_index, HEATER_OC_POS);
	const pin_descr_t neg = pin_descr_get(heater_index, HEATER_OC_NEG);

	/* Get state of negative phase as reference  */
	const bool to_activate = COMPLEMENT(heater_oc_is_active(neg));

	LOG_DBG("Heater idx=%u [%p] mode=%u to_activate=%u", 
		heater_index, heater, heater->mode, to_activate);

	bool z_reschedule = true;

	/* Get next period */
	uint32_t next_timeout_ms = 0u;
	switch (heater->mode) {
	case HEATER_MODE_CONFORT_MIN_1:
		next_timeout_ms = to_activate ?
			HEATER_CONFORT_MIN_1_ACTIVE_DURATION_MS :
			HEATER_CONFORT_MIN_1_INACTIVE_DURATION_MS;
		break;
	case HEATER_MODE_CONFORT_MIN_2:
		next_timeout_ms = to_activate ?
			HEATER_CONFORT_MIN_2_ACTIVE_DURATION_MS :
			HEATER_CONFORT_MIN_2_INACTIVE_DURATION_MS;
		break;
	default:
		/* We changed mode so we don't apply any change and
		 * don't reschedule the event */
		z_reschedule = false;
		break;
	}

	/* Schedule next phase */
	if (z_reschedule) {
		/* Apply new state */
		const uint8_t active = to_activate ? GPIO_HIGH : GPIO_LOW;
		heater_set_active(pos, active);
		heater_set_active(neg, active);

		/* Reschedule the event */
		k_event_schedule(ev, K_MSEC(next_timeout_ms));
	}
}

static void event_cb(struct k_event *ev)
{
#if CONFIG_WORKQUEUE_HEATERS_EXECUTION
	struct heater *const heater = CONTAINER_OF(ev, struct heater, event);
	k_system_workqueue_submit(&heater->work);
#else
	heater_ev_cb(ev);
#endif /* CONFIG_WORKQUEUE_HEATERS_EXECUTION */
}

#if CONFIG_WORKQUEUE_HEATERS_EXECUTION
static void work_cb(struct k_work *work)
{
	struct heater *const heater = CONTAINER_OF(work, struct heater, work);
	heater_ev_cb(&heater->event);
}
#endif /* CONFIG_WORKQUEUE_HEATERS_EXECUTION */

int heaters_init(void)
{
	int ret = 0;

	for (uint8_t h = 0u; h < CONFIG_HEATERS_COUNT; h++) {
		const pin_descr_t pos = pin_descr_get(h, HEATER_OC_POS);
		const pin_descr_t neg = pin_descr_get(h, HEATER_OC_NEG);

		bsp_descr_gpio_pin_init(pos, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);
		bsp_descr_gpio_pin_init(neg, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

		/* Set initial state (Off) */
		heater_set_mode(h, HEATER_MODE_OFF);

		k_event_init(&hs[h].event, event_cb);

#if CONFIG_WORKQUEUE_HEATERS_EXECUTION
		k_work_init(&hs[h].work, work_cb);
#endif
	}

	return ret;
}

int heater_set_mode(uint8_t hid, heater_mode_t mode)
{
#if CONFIG_CHECKS
	if (hid >= CONFIG_HEATERS_COUNT) {
		return -EINVAL;
	}
#endif

	const pin_descr_t pos = pin_descr_get(hid, HEATER_OC_POS);
	const pin_descr_t neg = pin_descr_get(hid, HEATER_OC_NEG);

	switch (mode) {
	case HEATER_MODE_CONFORT:
		heater_deactivate_oc(pos);
		heater_deactivate_oc(neg);
		break;
	case HEATER_MODE_CONFORT_MIN_1:
	case HEATER_MODE_CONFORT_MIN_2:
		/* Set mode immediately before event handler gets called */
		hs[hid].mode = mode;

		/* If event is already scheduled, it won't be sceduled again, 
		 * heater state will be properly applied on next iteration.
		 * 300 seconds later at maximum. (TODO does this needs to be improved ?)
		 */
		k_event_schedule(&hs[hid].event, K_NO_WAIT);
		break;
	case HEATER_MODE_ENERGY_SAVING:
		heater_activate_oc(pos);
		heater_activate_oc(neg);
		break;
	case HEATER_MODE_FROST_FREE:
		heater_deactivate_oc(pos);
		heater_activate_oc(neg);
		break;
	case HEATER_MODE_OFF:
		heater_activate_oc(pos);
		heater_deactivate_oc(neg);
		break;
	default:
		return -EINVAL;
	}

	hs[hid].mode = mode;

	return 0;
}

heater_mode_t heater_get_mode(uint8_t hid)
{
#if CONFIG_CHECKS
	if (hid >= CONFIG_HEATERS_COUNT) {
		return HEATER_MODE_OFF;
	}
#endif

	return hs[hid].mode;
}

#endif