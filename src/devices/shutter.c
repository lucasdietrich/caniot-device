#include <avrtos/kernel.h>

#include <avr/pgmspace.h>

#include "shutter.h"

#if !CONFIG_KERNEL_EVENTS
#error "Heaters controller needs CONFIG_KERNEL_EVENTS to be set"
#endif

#define SHUTTER_OPENNING_DURATION_MS 10000u
#define POWER_ON_STARTUP_DELAY_MS 100u

#define ENSURE_ADDITIONAL_DURATION_MS 1000u

#define SHUTTER_MINIMAL_OPENNESS_DIFF_PERCENT 10u
#define SHUTTER_MINIMAL_ALLOWED_DURATION_MS 100u

#if SHUTTER_OPENNING_DURATION_MS * SHUTTER_MINIMAL_OPENNESS_DIFF_PERCENT / 100 \
	< SHUTTER_MINIMAL_ALLOWED_DURATION_MS
#error "Minimal allowed duration is less than minimal allowed openness diff"
#endif


extern const struct shutters_system_oc ss PROGMEM;

enum {
	SHUTTER_STATE_STOPPED,
	SHUTTER_STATE_OPENNING,
	SHUTTER_STATE_CLOSING,
};

struct shutter
{
	/* Event used to schedule the next state change */
	struct k_event event;

	/* Openness in percents,
	 * 0 - closed, 100 - opened
	 */
	uint8_t openness;

	/* Duration of the next state to be applied */
	uint16_t duration;

	/* State to be applied (see enum above) */
	uint8_t state;
};

#define FLAG_SHUTTER(_s) (1u << ((_s) + 1u))

#define FLAG_POWERED 		(1u << 0u)
#define FLAG_SHUTTER_1 FLAG_SHUTTER(0u)
#define FLAG_SHUTTER_2 FLAG_SHUTTER(1u)
#define FLAG_SHUTTER_3 FLAG_SHUTTER(2u)
#define FLAG_SHUTTER_4 FLAG_SHUTTER(3u)

#define MASK_SHUTTERS (FLAG_SHUTTER_1 | FLAG_SHUTTER_2 | FLAG_SHUTTER_3 | FLAG_SHUTTER_4)

static uint8_t flags = 0u;

static struct shutter shutters[CONFIG_SHUTTERS_COUNT];


#define SHUTTER_INDEX(_sp) ((_sp) - shutters)

static const struct pin *pin_get(uint8_t shutter, uint8_t pin)
{
	return &ss.shutters[shutter][pin];
}

static void power(uint8_t state)
{
	bsp_pgm_pin_output_write(&ss.power_oc, state);
}

static inline void power_on(void)
{
	power(GPIO_HIGH);
}

static inline void power_off(void)
{
	flags &= ~FLAG_POWERED;
	power(GPIO_LOW);
}

static void run_direction(uint8_t s, uint8_t dir)
{
	const struct pin *pos = pin_get(s, SHUTTER_OC_POS);
	const struct pin *neg = pin_get(s, SHUTTER_OC_POS);

	uint8_t pos_state = GPIO_LOW;
	uint8_t neg_state = GPIO_LOW;

	if (dir == SHUTTER_STATE_OPENNING) {
		pos_state = GPIO_HIGH;
	} else if (dir == SHUTTER_STATE_CLOSING) {
		neg_state = GPIO_HIGH;
	}

	bsp_pgm_pin_output_write(pos, pos_state);
	bsp_pgm_pin_output_write(neg, neg_state);
}

static void event_cb(struct k_event *ev)
{
	struct shutter *shutter = CONTAINER_OF(ev, struct shutter, event);
	const uint8_t shutter_index = SHUTTER_INDEX(shutter);

	/* Power has necessarily been turned on at least
	 * POWER_ON_STARTUP_DELAY_MS ago. So we can safely
	 * set the flag here.
	 */
	flags |= FLAG_POWERED;

	/* Apply the state */
	run_direction(shutter_index, shutter->state);

	if (shutter->state == SHUTTER_STATE_STOPPED) {
		/* Mark the shutter as stopped */
		flags &= ~FLAG_SHUTTER(shutter_index);
	} else {
		/* Schedule the stop */
		shutter->state = SHUTTER_STATE_STOPPED;
		k_event_schedule(ev, K_MSEC(shutter->duration));
	}

	/* Depower if no more shutters are running */
	if (!(flags & MASK_SHUTTERS)) {
		power_off();
	}
}

/* __attribute__((noinline)) */ int shutters_system_init(void)
{
	bsp_pgm_pin_init(&ss.power_oc, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

	for (uint8_t i = 0u; i < CONFIG_SHUTTERS_COUNT; i++) {
		bsp_pgm_pin_init(&ss.shutters[i][SHUTTER_OC_POS],
				 GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);
		bsp_pgm_pin_init(&ss.shutters[i][SHUTTER_OC_NEG],
				 GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

		/* Assume closed */
		shutters[i].openness = 0u;
		k_event_init(&shutters[i].event, event_cb);
	}

	return 0;
}

int shutter_set_openness(uint8_t s, uint8_t openness)
{
	if (s >= CONFIG_SHUTTERS_COUNT)
		return -EINVAL;

	if (openness > 100u)
		return -EINVAL;

	if (flags & FLAG_SHUTTER(s))
		return -EBUSY;

	struct shutter *const shutter = &shutters[s];

	uint8_t state;
	uint16_t duration = SHUTTER_OPENNING_DURATION_MS +
			    ENSURE_ADDITIONAL_DURATION_MS;

	if (openness == 100u) {
		/* Make sure the shutter is fully open */
		state = SHUTTER_STATE_OPENNING;
	} else if (openness == 0u) {
		/* Make sure the shutter is fully closed */
		state = SHUTTER_STATE_CLOSING;
	} else {
		/* Compute the duration */
		int8_t rel = openness - shutter->openness;

		/* If no change, do nothing */
		if (rel == 0)
			return 0;

		if (rel > 0) {
			state = SHUTTER_STATE_OPENNING;
		} else {
			state = SHUTTER_STATE_CLOSING;

			/* Compute absolute value  for duration calculation*/
			rel = -rel;
		}

		/* Prevent too short durations */
		if (rel < SHUTTER_MINIMAL_OPENNESS_DIFF_PERCENT)
			return -ENOTSUP;

		duration = (SHUTTER_OPENNING_DURATION_MS / 100) * rel;
	}

	/* Directly set final openness */
	shutter->openness = openness;

	/* Mark the shutter as running */
	flags |= FLAG_SHUTTER(s);

	/* Check wether we need to power on */
	if (flags & FLAG_POWERED) {
		/* If powered, start immediately and schedule shutter stop */
		run_direction(s, state);

		shutter->state = SHUTTER_STATE_STOPPED;

		k_event_schedule(&shutter->event,
				 K_MSEC(duration));
	} else {
		/* If not powered, power on and schedule shutter action */
		power_on();

		/* We don't set the flag "powered" immediately, we wait for
		 * duration POWER_ON_STARTUP_DELAY_MS.
		 */

		shutter->state = state;
		shutter->duration = duration;

		k_event_schedule(&shutter->event,
				 K_MSEC(POWER_ON_STARTUP_DELAY_MS));
	}

	return 0;
}