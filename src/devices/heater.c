#if CONFIG_HEATERS_COUNT

#include "heater.h"
#include "config.h"

#include "bsp/bsp.h"

#include <avr/pgmspace.h>

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

extern const struct pin heaters[CONFIG_HEATERS_COUNT][2u] PROGMEM;

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

static const struct pin *pin_get(uint8_t heater, uint8_t pin)
{
	return &heaters[heater][pin];
}

static inline void heater_activate_oc(const struct pin *farp_pin)
{
	bsp_pgm_pin_output_write(farp_pin, GPIO_LOW);
}

static inline void heater_deactivate_oc(const struct pin *farp_pin)
{
	bsp_pgm_pin_output_write(farp_pin, GPIO_HIGH);
}

static inline void heater_set_active(const struct pin *farp_pin, uint8_t active)
{
	bsp_pgm_pin_output_write(farp_pin, active);
}

static inline uint8_t heater_oc_is_active(const struct pin *farp_pin)
{
	return 1u - bsp_pgm_pin_input_read(farp_pin);
}

void heater_ev_cb(struct k_event *ev)
{
	struct heater *const heater = CONTAINER_OF(ev, struct heater, event);
	const uint8_t heater_index = HEATER_INDEX(heater);

	const struct pin *pos = pin_get(heater_index, HEATER_OC_POS);
	const struct pin *neg = pin_get(heater_index, HEATER_OC_NEG);

	/* Get current state */
	bool next_active = !heater_oc_is_active(pos);

	/* Get next period */
	uint32_t next_timeout_ms = 0u;
	switch (heater->mode) {
	case HEATER_MODE_CONFORT_MIN_1:
		next_timeout_ms = next_active ?
			HEATER_CONFORT_MIN_1_HIGH_DURATION_MS :
			HEATER_CONFORT_MIN_1_LOW_DURATION_MS;
		break;
	case HEATER_MODE_CONFORT_MIN_2:
		next_timeout_ms = next_active ?
			HEATER_CONFORT_MIN_2_HIGH_DURATION_MS :
			HEATER_CONFORT_MIN_2_LOW_DURATION_MS;
		break;
	default:
		/* We changed mode so we don't apply any change and
		 * don't reschedule the event */
		return;
	}

	/* Apply new state */
	const uint8_t active = next_active ? GPIO_HIGH : GPIO_LOW;
	heater_set_active(pos, active);
	heater_set_active(neg, active);

	/* Reschedule the event */
	k_event_schedule(ev, K_MSEC(next_timeout_ms));
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
		const struct pin *pos = pin_get(h, HEATER_OC_POS);
		const struct pin *neg = pin_get(h, HEATER_OC_NEG);

		bsp_pgm_pin_init(pos, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);
		bsp_pgm_pin_init(neg, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

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
	if (hid >= CONFIG_HEATERS_COUNT) {
		return -EINVAL;
	}

	const struct pin *pos = pin_get(hid, HEATER_OC_POS);
	const struct pin *neg = pin_get(hid, HEATER_OC_NEG);

	switch (mode) {
	case HEATER_MODE_CONFORT:
		heater_deactivate_oc(pos);
		heater_deactivate_oc(neg);
		break;
	case HEATER_MODE_CONFORT_MIN_1:
	case HEATER_MODE_CONFORT_MIN_2:
		/* Set mode immediately, in case the event is immediately called */
		hs[hid].mode = mode;
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
	if (hid >= CONFIG_HEATERS_COUNT) {
		return HEATER_MODE_OFF;
	}

	return hs[hid].mode;
}

#endif