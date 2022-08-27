#include "heater.h"

#include <avr/pgmspace.h>

extern const struct heater_oc heaters[CONFIG_HEATERS_COUNT][2u] PROGMEM;
extern heater_mode_t heaters_mode[CONFIG_HEATERS_COUNT];

struct k_event heaters_event[CONFIG_HEATERS_COUNT];

static inline GPIO_Device *oc_dev(uint8_t hid, uint8_t ocid)
{
	return (GPIO_Device *)pgm_read_word(&heaters[hid][ocid].dev);
}

static inline uint8_t oc_pin(uint8_t hid, uint8_t ocid)
{
	return pgm_read_byte(&heaters[hid][ocid].pin);
}

static inline void heater_activate_oc(GPIO_Device *oc, uint8_t pin)
{
	gpio_set_pin_output_state(oc, pin, GPIO_LOW);
}

static inline void heater_deactivate_oc(GPIO_Device *oc, uint8_t pin)
{
	gpio_set_pin_output_state(oc, pin, GPIO_HIGH);
}

static inline void heater_set_active(GPIO_Device *oc, uint8_t pin, uint8_t active)
{
	gpio_set_pin_output_state(oc, pin, ~active);
}

static inline uint8_t heater_oc_is_active(GPIO_Device *oc, uint8_t pin)
{
	return 1u - gpio_get_pin_state(oc, pin);
}

void heater_ev_cb(struct k_event *ev)
{
	const uint8_t hid = ev - heaters_event;

	GPIO_Device *const pos = oc_dev(hid, HEATER_OC_POS);
	uint8_t pos_pin = oc_pin(hid, HEATER_OC_POS);

	GPIO_Device *const neg = oc_dev(hid, HEATER_OC_NEG);
	uint8_t neg_pin = oc_pin(hid, HEATER_OC_NEG);

	/* Get current state */
	bool next_active = !heater_oc_is_active(pos, pos_pin);
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
		/* We changed mode so we don't reschedule the event */
		return;
	}

	/* Apply new state */
	const uint8_t active = next_active ? GPIO_HIGH : GPIO_LOW;
	heater_set_active(pos, pos_pin, active);
	heater_set_active(neg, neg_pin, active);

	/* Reschedule the event */
	k_event_schedule(ev, K_MSEC(next_timeout_ms));
}

int heater_init(uint8_t hid)
{
	if (hid >= CONFIG_HEATERS_COUNT) {
		return -EINVAL;
	}

	GPIO_Device *const pos = oc_dev(hid, HEATER_OC_POS);
	uint8_t pos_pin = oc_pin(hid, HEATER_OC_POS);

	GPIO_Device *const neg = oc_dev(hid, HEATER_OC_NEG);
	uint8_t neg_pin = oc_pin(hid, HEATER_OC_NEG);

	gpio_pin_init(pos, pos_pin, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);
	gpio_pin_init(neg, neg_pin, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

	heater_set_mode(hid, heaters_mode[hid]);

	k_event_init(&heaters_event[hid], heater_ev_cb);

	return 0;
}

int heater_set_mode(uint8_t hid, heater_mode_t mode)
{
	if (hid >= CONFIG_HEATERS_COUNT) {
		return -EINVAL;
	}

	GPIO_Device *const pos = oc_dev(hid, HEATER_OC_POS);
	uint8_t pos_pin = oc_pin(hid, HEATER_OC_POS);

	GPIO_Device *const neg = oc_dev(hid, HEATER_OC_NEG);
	uint8_t neg_pin = oc_pin(hid, HEATER_OC_NEG);

	switch (mode) {
	case HEATER_MODE_CONFORT:
		heater_deactivate_oc(pos, pos_pin);
		heater_deactivate_oc(neg, neg_pin);
		break;
	case HEATER_MODE_CONFORT_MIN_1:
	case HEATER_MODE_CONFORT_MIN_2:
		/* Set mode immediately, in case the event is called */
		heaters_mode[hid] = mode;
		k_event_schedule(&heaters_event[hid], K_NO_WAIT);
		break;
	case HEATER_MODE_ENERGY_SAVING:
		heater_activate_oc(pos, pos_pin);
		heater_activate_oc(neg, neg_pin);
		break;
	case HEATER_MODE_FROST_FREE:
		heater_deactivate_oc(pos, pos_pin);
		heater_activate_oc(neg, neg_pin);
		break;
	case HEATER_MODE_OFF:
		heater_activate_oc(pos, pos_pin);
		heater_deactivate_oc(neg, neg_pin);
		break;
	default:
		return -EINVAL;
	}

	heaters_mode[hid] = mode;

	return 0;
}