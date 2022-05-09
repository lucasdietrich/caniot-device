#include "pulse.h"

#include <stdbool.h>

#include <avrtos/mutex.h>
#include <avrtos/dstruct/tqueue.h>

#include <avr/io.h>

#include "dev.h"

#define K_MODULE K_MODULE_APPLICATION

#if CONFIG_GPIO_PULSE_SUPPORT

struct pulse_event
{
	struct titem tie;

	uint8_t reset_state: 1;
	uint8_t scheduled: 1;
};

static K_MUTEX_DEFINE(mutex);
static struct pulse_event events[4U];
static DEFINE_TQUEUE(ev_queue);

#define PULSE_CONTEXT_LOCK() k_mutex_lock(&mutex, K_FOREVER)
#define PULSE_CONTEXT_UNLOCK() k_mutex_unlock(&mutex)

#define EVENT_FROM_TIE(tie_p) CONTAINER_OF(tie_p, struct pulse_event, tie)

#define EVENT_TO_PIN(ev) (ev - events)


static struct pulse_event *get_event(output_t pin)
{
	if (GPIO_VALID_OUTPUT_PIN(pin)) {
		return &events[pin];
	}

	return NULL;
}

static inline void output_set_state(output_t pin, bool state)
{
	ll_outputs_set_mask(state ? BIT(pin) : 0U, BIT(pin));
}

/**
 * @brief Assume event is not null
 * 
 * @param ev 
 */
static void cancel_event(struct pulse_event *ev)
{
	__ASSERT_NOTNULL(ev);

	if (ev->scheduled == 1U) {
		tqueue_remove(&ev_queue, &ev->tie);
		ev->scheduled = 0U;
	}
}

void pulse_init(void)
{
	PULSE_CONTEXT_LOCK();
	
	for (struct pulse_event *ev = events;
	     ev < events + ARRAY_SIZE(events); ev++)
	{
		ev->scheduled = 0U;
	}

	PULSE_CONTEXT_UNLOCK();
}

void pulse_trigger(output_t pin, bool state, uint32_t duration_ms)
{
	if (duration_ms == 0) {
		return;
	}

	PULSE_CONTEXT_LOCK();

	struct pulse_event *ev = get_event(pin);
	if (ev != NULL) {
		cancel_event(ev);
		output_set_state(pin, state);
		tqueue_schedule(&ev_queue, &ev->tie, duration_ms);
		ev->scheduled = 1U;
		ev->reset_state = !state;
	}

	PULSE_CONTEXT_UNLOCK();
}

void pulse_cancel(output_t pin)
{
	PULSE_CONTEXT_LOCK();

	struct pulse_event *ev = get_event(pin);
	if (ev != NULL) {
		cancel_event(ev);
		output_set_state(pin, ev->reset_state);
	}

	PULSE_CONTEXT_UNLOCK();
}

bool pulse_is_active(output_t pin)
{
	struct pulse_event *ev = get_event(pin);
	if (ev != NULL) {
		return ev->scheduled == 1U;
	}

	return false;
}

void pulse_process(uint32_t time_passed_ms)
{
	struct titem *tie = NULL;
	bool trigger = false;

	PULSE_CONTEXT_LOCK();

	tqueue_shift(&ev_queue, time_passed_ms);

	while ((tie = tqueue_pop(&ev_queue)) != NULL) {
		struct pulse_event *ev = EVENT_FROM_TIE(tie);

		ev->scheduled = 0U;
		output_set_state(EVENT_TO_PIN(ev),
				 (bool)ev->reset_state);
		
		trigger = true;
	}
	
	PULSE_CONTEXT_UNLOCK();
	
	/* if at least one event has been dequeue, request telemetry */
	if (trigger == true) {
		trigger_telemetry();
	}
}

uint32_t pulse_remaining(void)
{
	uint32_t remaining = -1;

	PULSE_CONTEXT_LOCK();

	if (ev_queue != NULL) {
		remaining = (*ev_queue).timeout;
	}

	PULSE_CONTEXT_UNLOCK();

	return remaining;
}

#endif