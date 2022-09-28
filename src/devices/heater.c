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

extern const struct pin heaters[CONFIG_HEATERS_COUNT][2u] PROGMEM;

heater_mode_t heaters_mode[CONFIG_HEATERS_COUNT];

struct k_event heaters_event[CONFIG_HEATERS_COUNT];

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
	const uint8_t hid = ev - heaters_event;

	const struct pin *pos = pin_get(hid, HEATER_OC_POS);
	const struct pin *neg = pin_get(hid, HEATER_OC_NEG);

	/* Get current state */
	bool next_active = !heater_oc_is_active(pos);
	const heater_mode_t mode = heaters_mode[hid];

	/* Get next period */
	uint32_t next_timeout_ms = 0u;
	switch (mode) {
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

int heater_init(uint8_t hid)
{
	if (hid >= CONFIG_HEATERS_COUNT) {
		return -EINVAL;
	}

	const struct pin *pos = pin_get(hid, HEATER_OC_POS);
	const struct pin *neg = pin_get(hid, HEATER_OC_NEG);

	bsp_pgm_pin_init(pos, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);
	bsp_pgm_pin_init(neg, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

	/* Set initial state (Off) */
	heater_set_mode(hid, HEATER_MODE_OFF);

	k_event_init(&heaters_event[hid], heater_ev_cb);

	return 0;
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
		/* Set mode immediately, in case the event is called */
		heaters_mode[hid] = mode;
		k_event_schedule(&heaters_event[hid], K_NO_WAIT);
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

	heaters_mode[hid] = mode;

	return 0;
}