#if CONFIG_SHUTTERS_COUNT > 0u

#include "shutter.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/pgmspace.h>
#define LOG_LEVEL LOG_LEVEL_DBG

#if !CONFIG_KERNEL_EVENTS
#error "Heaters controller needs CONFIG_KERNEL_EVENTS to be set"
#endif

#if CONFIG_EXTIO_ENABLED && !CONFIG_WORKQUEUE_SHUTTERS_EXECUTION
#error "CONFIG_EXTIO_ENABLED requires CONFIG_WORKQUEUE_HEATERS_EXECUTION"
#endif

#if CONFIG_WORKQUEUE_SHUTTERS_EXECUTION && !CONFIG_SYSTEM_WORKQUEUE_ENABLE
#error "Shutter controller needs CONFIG_SYSTEM_WORKQUEUE_ENABLE to be set"
#endif

#define SHUTTER_OPENNING_DURATION_MS 10000u
#define POWER_ON_STARTUP_DELAY_MS    100u

#define ENSURE_ADDITIONAL_DURATION_MS 1000u

#define SHUTTER_MINIMAL_OPENNESS_DIFF_PERCENT 10u
#define SHUTTER_MINIMAL_ALLOWED_DURATION_MS   100u

#if SHUTTER_OPENNING_DURATION_MS * SHUTTER_MINIMAL_OPENNESS_DIFF_PERCENT / 100 <         \
    SHUTTER_MINIMAL_ALLOWED_DURATION_MS
#error "Minimal allowed duration is less than minimal allowed openness diff"
#endif

enum {
    SHUTTER_STATE_STOPPED,
    SHUTTER_STATE_OPENNING,
    SHUTTER_STATE_CLOSING,
};

struct shutter {
    /* Event used to schedule the next state change
     *
     * Note: Keep event as first member of the structure
     * for optomization purposes
     */
    struct k_event event;

#if CONFIG_WORKQUEUE_SHUTTERS_EXECUTION
    /* Work used to schedule the next state change */
    struct k_work work;
#endif

    /* Openness in percents,
     * 0 - closed, 100 - opened
     */
    uint8_t openness;

    /* Duration of the next state to be applied */
    uint16_t duration;

    /* State to be applied (see enum above) */
    uint8_t state;
};

#define FLAG_POWERED (1u << 0u)

#define FLAG_SHUTTER(_s) (1u << ((_s) + 1u))
#define FLAG_SHUTTER_1   FLAG_SHUTTER(0u)
#define FLAG_SHUTTER_2   FLAG_SHUTTER(1u)
#define FLAG_SHUTTER_3   FLAG_SHUTTER(2u)
#define FLAG_SHUTTER_4   FLAG_SHUTTER(3u)

#define MASK_SHUTTERS (FLAG_SHUTTER_1 | FLAG_SHUTTER_2 | FLAG_SHUTTER_3 | FLAG_SHUTTER_4)

static uint8_t flags = 0u;

static struct shutter shutters[CONFIG_SHUTTERS_COUNT];

#define SHUTTER_INDEX(_sp) ((_sp)-shutters)

#define COMPLEMENT(_x) ((_x) ? 0u : 1u)

static pin_descr_t pin_descr_get(uint8_t shutter, uint8_t pin)
{
    return pgm_read_byte(&shutters_io.shutters[shutter][pin]);
}

static void set_power(uint8_t active)
{
    bsp_descr_gpio_output_write(shutters_io.power_oc, COMPLEMENT(active));
}

static inline void set_active(pin_descr_t pin, uint8_t active)
{
    bsp_descr_gpio_output_write(pin, COMPLEMENT(active));
}

static inline void power_on(void)
{
    set_power(1u);
}

static inline void power_off(void)
{
    flags &= ~FLAG_POWERED;
    set_power(0u);
}

static void run_direction(uint8_t s, uint8_t dir)
{
    const pin_descr_t pos = pin_descr_get(s, SHUTTER_OC_POS);
    const pin_descr_t neg = pin_descr_get(s, SHUTTER_OC_NEG);

    uint8_t pos_active = 0u;
    uint8_t neg_active = 0u;

    if (dir == SHUTTER_STATE_OPENNING) {
        pos_active = 1u;
    } else if (dir == SHUTTER_STATE_CLOSING) {
        neg_active = 1u;
    }

    LOG_DBG("Shutter %u: run %u/100 [ pos %u -> %x ] [ neg %u -> %x ]",
            s,
            dir,
            pos_active,
            pos,
            neg_active,
            neg);

    set_active(pos, pos_active);
    set_active(neg, neg_active);
}

static void shutter_event_handler(struct k_event *ev)
{
    struct shutter *shutter     = CONTAINER_OF(ev, struct shutter, event);
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

static void event_cb(struct k_event *ev)
{
#if CONFIG_WORKQUEUE_SHUTTERS_EXECUTION
    struct shutter *const shutter = CONTAINER_OF(ev, struct shutter, event);
    k_system_workqueue_submit(&shutter->work);
#else
    shutter_event_handler(ev);
#endif /* CONFIG_WORKQUEUE_SHUTTERS_EXECUTION */
}

#if CONFIG_WORKQUEUE_SHUTTERS_EXECUTION
static void work_cb(struct k_work *work)
{
    struct shutter *const shutter = CONTAINER_OF(work, struct shutter, work);
    shutter_event_handler(&shutter->event);
}
#endif /* CONFIG_WORKQUEUE_SHUTTERS_EXECUTION */

/* __attribute__((noinline)) */ int shutters_system_init(void)
{
    bsp_descr_gpio_pin_init(shutters_io.power_oc, GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

    for (uint8_t i = 0u; i < CONFIG_SHUTTERS_COUNT; i++) {
        bsp_descr_gpio_pin_init(
            shutters_io.shutters[i][SHUTTER_OC_POS], GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);
        bsp_descr_gpio_pin_init(
            shutters_io.shutters[i][SHUTTER_OC_NEG], GPIO_OUTPUT, GPIO_OUTPUT_DRIVEN_LOW);

        /* Assume closed */
        shutters[i].openness = 0u;
        k_event_init(&shutters[i].event, event_cb);

#if CONFIG_WORKQUEUE_SHUTTERS_EXECUTION
        k_work_init(&shutters[i].work, work_cb);
#endif /* CONFIG_WORKQUEUE_SHUTTERS_EXECUTION */
    }

    return 0;
}

int shutter_set_openness(uint8_t s, uint8_t openness)
{
#if CONFIG_CHECKS
    if (s >= CONFIG_SHUTTERS_COUNT) return -EINVAL;
    if (openness > 100u) return -EINVAL;
    if (flags & FLAG_SHUTTER(s)) return -EBUSY;
#endif

    struct shutter *const shutter = &shutters[s];

    uint8_t state;
    uint16_t duration = SHUTTER_OPENNING_DURATION_MS + ENSURE_ADDITIONAL_DURATION_MS;

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
        if (rel == 0) return 0;

        if (rel > 0) {
            state = SHUTTER_STATE_OPENNING;
        } else {
            state = SHUTTER_STATE_CLOSING;

            /* Compute absolute value  for duration calculation*/
            rel = -rel;
        }

        /* Prevent too short durations */
        if (rel < SHUTTER_MINIMAL_OPENNESS_DIFF_PERCENT) return -ENOTSUP;

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

        k_event_schedule(&shutter->event, K_MSEC(duration));
    } else {
        /* If not powered, power on and schedule shutter action */
        power_on();

        /* We don't set the flag "powered" immediately, we wait for
         * duration POWER_ON_STARTUP_DELAY_MS.
         */

        shutter->state    = state;
        shutter->duration = duration;

        k_event_schedule(&shutter->event, K_MSEC(POWER_ON_STARTUP_DELAY_MS));
    }

    return 0;
}

int shutter_get_openness(uint8_t s)
{
#if CONFIG_CHECKS
    if (s >= CONFIG_SHUTTERS_COUNT) return -EINVAL;
#endif

    return shutters[s].openness;
}

#endif /* CONFIG_SHUTTERS */