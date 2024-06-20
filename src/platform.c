/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can.h"
#include "config.h"
#include "platform.h"
#include "watchdog.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <caniot/caniot.h>
#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL

// Let time for the device to send the CAN response if pending
#define DEFERRED_RESET_DELAY K_SECONDS(1)

void platform_entropy(uint8_t *buf, size_t len)
{
    static K_PRNG_DEFINE(prng, __MAGIC_NUMBER__, __MAGIC_NUMBER__ >> 1);

    k_prng_get_buffer(&prng, buf, len);
}

void platform_get_time(uint32_t *sec, uint16_t *ms)
{
    if (!sec) return;

    const uint64_t time_ms = k_time_get_ms();
    *sec                   = time_ms / MSEC_PER_SEC;

    if (ms) *ms = time_ms % MSEC_PER_SEC;
}

void platform_set_time(uint32_t sec)
{
    k_time_set(sec);
}

static void caniot2msg(can_message *msg, const struct caniot_frame *frame)
{
    msg->ext   = 0U;
    msg->isext = 0U;
    msg->rtr   = 0U;
    msg->std   = caniot_id_to_canid(frame->id);
    msg->len   = frame->len;
    memcpy(msg->buf, frame->buf, MIN(frame->len, 8U));
}

static void msg2caniot(struct caniot_frame *frame, const can_message *msg)
{
    frame->id  = caniot_canid_to_id(msg->std);
    frame->len = msg->len;
    memcpy(frame->buf, msg->buf, msg->len);
}

int platform_caniot_recv(struct caniot_frame *frame)
{
    int ret;
    can_message req;

    ret = can_recv(&req);
    if (ret == 0) {
        // can_print_msg(&req);
        msg2caniot(frame, &req);

#if LOG_LEVEL >= LOG_LEVEL_INF
        k_show_uptime();
        caniot_explain_frame(frame);
        LOG_INF_RAW("\n");
#endif
    } else if (ret == -EAGAIN) {
        ret = -CANIOT_EAGAIN;
    }

    return ret;
}

#if CONFIG_CAN_DELAYABLE_TX
struct delayed_msg {
    struct k_event ev;
    can_message msg;
};

/* Should be increased if "delayed message" feature is used */
K_MEM_SLAB_DEFINE(dmsg_slab, sizeof(struct delayed_msg), CONFIG_CAN_DELAYABLE_TX_BUFFER);

static void dmsg_handler(struct k_event *ev)
{
    struct delayed_msg *dmsg = CONTAINER_OF(ev, struct delayed_msg, ev);

    (void)can_txq_message(&dmsg->msg);

    k_mem_slab_free(&dmsg_slab, dmsg);
}
#endif

int platform_caniot_send(const struct caniot_frame *frame, uint32_t delay_ms)
{
    int ret;
    if (!CONFIG_CAN_DELAYABLE_TX || (delay_ms < KERNEL_TICK_PERIOD_MS)) {
        can_message msg;

        caniot2msg(&msg, frame);

        ret = can_txq_message(&msg);
    } else {
#if CONFIG_CAN_DELAYABLE_TX
        struct delayed_msg *dmsg;

        ret = k_mem_slab_alloc(&dmsg_slab, (void **)&dmsg, K_NO_WAIT);
        if (ret == 0) {
            caniot2msg(&dmsg->msg, frame);
            k_event_init(&dmsg->ev, dmsg_handler);
            ret = k_event_schedule(&dmsg->ev, K_MSEC(delay_ms));
            if (ret != 0) {
                k_mem_slab_free(&dmsg_slab, (void *)dmsg);
            }
        }
#endif
    }

#if LOG_LEVEL >= LOG_LEVEL_INF
    if (ret == 0) {
        k_show_uptime();
        caniot_explain_frame(frame);
        LOG_INF_RAW("\n");
    }
#endif

    return ret;
}

struct sys_work {
    enum {
        SYS_NONE      = 0u,
        SYS_WDT_RESET = 2u,
    } action;
    struct k_work_delayable _work;
};

static void reset_now(void)
{
    /* Enable watchdog if off */
    if ((WDTCSR & BIT(WDE)) == 0u) {
        wdt_enable(WATCHDOG_TIMEOUT_WDTO);
    }

    irq_disable();

    for (;;) {
        /* wait for WDT reset */
    }

    CODE_UNREACHABLE;
}

static void sys_work_handler(struct k_work *w)
{
    struct sys_work *x = CONTAINER_OF(w, struct sys_work, _work);

    switch (x->action) {
    case SYS_WDT_RESET: {
        reset_now();
    }
    default:
        break;
    }
}

static struct sys_work sys_work = {
    .action = SYS_NONE,
    ._work  = K_WORK_DELAYABLE_INIT(sys_work_handler),
};

int platform_reset(bool deferred)
{
    int ret = 0;

    if (deferred) {
        LOG_DBG("Reset (WDT) in 1 SEC");

        /* When requesting device request, we delay the reset in order to let the
         * device time to send the CAN response if needed.
         */
        sys_work.action = SYS_WDT_RESET;
        ret = k_system_work_delayable_schedule(&sys_work._work, DEFERRED_RESET_DELAY);
    } else {
        reset_now();
    }

    return ret;
}

int platform_watchdog_enable(bool enable)
{
    if (enable) {
        wdt_enable(WATCHDOG_TIMEOUT_WDTO);
    } else {
        wdt_disable();
    }

    return 0;
}