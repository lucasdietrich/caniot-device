/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_GPIO_PULSE_SUPPORT

#include "bsp/bsp.h"
#include "devices/gpio_pulse.h"

#include <stdbool.h>

#include <avrtos/avrtos.h>
#include <avrtos/dstruct/tqueue.h>

#include <avr/io.h>

#define K_MODULE K_MODULE_APPLICATION

#if CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT == 0
#error CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT == 0 while PULSE support is enabled
#endif

static uint32_t pulse_last_process = 0u;

K_MEM_SLAB_DEFINE(ctx_mems,
                  sizeof(struct pulse_event),
                  CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT);

static DEFINE_TQUEUE(ev_queue);

#if CONFIG_GPIO_PULSE_THREAD_SAFE
static K_MUTEX_DEFINE(mutex);
#define PULSE_CONTEXT_LOCK()   k_mutex_lock(&mutex, K_FOREVER)
#define PULSE_CONTEXT_UNLOCK() k_mutex_unlock(&mutex)
#else
#define PULSE_CONTEXT_LOCK()
#define PULSE_CONTEXT_UNLOCK()
#endif

#define EVENT_FROM_TIE(tie_p) CONTAINER_OF(tie_p, struct pulse_event, _tie)

#define EVENT_TO_PIN(ev) (ev - events)

static struct pulse_event *alloc_context(void)
{
    void *mem;

    if (k_mem_slab_alloc(&ctx_mems, &mem, K_NO_WAIT) == 0) {
        ((struct pulse_event *)mem)->_iallocated = 1u;
        return mem;
    }

    return NULL;
}

static void free_context(struct pulse_event *ctx)
{
    if (ctx->_iallocated) {
        k_mem_slab_free(&ctx_mems, (void *)ctx);
    }
}

static inline void output_set_state(pin_descr_t descr, bool state)
{
    bsp_descr_gpio_output_write(descr, state ? GPIO_HIGH : GPIO_LOW);
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
        tqueue_remove(&ev_queue, &ev->_tie);
        ev->scheduled = 0U;
    }
}

void pulse_init(void)
{
    pulse_last_process = k_uptime_get();
}

struct pulse_event *
pulse_trigger(pin_descr_t descr, bool state, uint32_t duration_ms, struct pulse_event *ev)
{
    if (duration_ms == 0) {
        goto exit;
    }

    /* Try to allocate a context if not provided */
    if (ev == NULL) {
        ev = alloc_context();
    } else {
        ev->_iallocated = 0u;
    }

    PULSE_CONTEXT_LOCK();

    if (ev != NULL) {
        output_set_state(descr, state);
        ev->scheduled   = 1u;
        ev->descr       = descr;
        ev->reset_state = !state;
        tqueue_schedule(&ev_queue, &ev->_tie, duration_ms);
    }

    PULSE_CONTEXT_UNLOCK();

exit:
    return ev;
}

void pulse_cancel(struct pulse_event *ev, bool do_reset_state)
{
    PULSE_CONTEXT_LOCK();

    if (ev != NULL) {
        cancel_event(ev);
        if (do_reset_state) output_set_state(ev->descr, ev->reset_state);
        free_context(ev);
    }

    PULSE_CONTEXT_UNLOCK();
}

bool pulse_is_active(struct pulse_event *ev)
{
    if (ev != NULL) {
        return ev->scheduled == 1U;
    }

    return false;
}

bool pulse_process(uint32_t now_ms)
{
    struct titem *_tie = NULL;
    bool least_one     = false;

    PULSE_CONTEXT_LOCK();

    /* Time passed since last process */
    uint32_t time_passed_ms = now_ms - pulse_last_process;
    pulse_last_process      = now_ms;

    tqueue_shift(&ev_queue, time_passed_ms);

    while ((_tie = tqueue_pop(&ev_queue)) != NULL) {
        struct pulse_event *ev = EVENT_FROM_TIE(_tie);

        ev->scheduled = 0U;
        output_set_state(ev->descr, ev->reset_state);
        free_context(ev);

        least_one = true;
    }

    PULSE_CONTEXT_UNLOCK();

    return least_one;
}

uint32_t pulse_remaining(void)
{
    uint32_t remaining = -1;

    PULSE_CONTEXT_LOCK();

    if (ev_queue != NULL) {
        remaining = ev_queue->timeout;
    }

    PULSE_CONTEXT_UNLOCK();

    return remaining;
}

#endif