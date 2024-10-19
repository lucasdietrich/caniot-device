/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp/bsp.h"
#include "can.h"
#include "dev.h"
#include "platform.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <mcp2515_can.h>
#include <mcp2515_can_dfs.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL

#define K_MODULE_CAN 0x21
#define K_MODULE     K_MODULE_CAN

#if CONFIG_CAN_CLOCKSET_16MHZ
#define CAN_CLOCKSET MCP_16MHz
#else
#define CAN_CLOCKSET MCP_8MHz
#endif

#define CAN_SPEEDSET CAN_500KBPS

static mcp2515_can can(BSP_CAN_SS_ARDUINO_PIN);

#define CONFIG_CAN_THREAD_OFFLOADED !CONFIG_CAN_WORKQ_OFFLOADED

#if CONFIG_CAN_CONTEXT_LOCK
static K_MUTEX_DEFINE(can_mutex_if);
#define CAN_CONTEXT_LOCK()   k_mutex_lock(&can_mutex_if, K_FOREVER);
#define CAN_CONTEXT_UNLOCK() k_mutex_unlock(&can_mutex_if);
#else
#define CAN_CONTEXT_LOCK()
#define CAN_CONTEXT_UNLOCK()
#endif

K_MSGQ_DEFINE(txq, sizeof(can_message), CONFIG_CAN_TX_MSGQ_SIZE);

#if CONFIG_CAN_WORKQ_OFFLOADED
static void can_tx_wq_cb(struct k_work *work);
static K_WORK_DEFINE(can_tx_work, can_tx_wq_cb);
#else
static void can_tx_entry(void *arg);
K_THREAD_DEFINE(
    can_tx_thread, can_tx_entry, CONFIG_CAN_THREAD_STACK_SIZE, K_COOPERATIVE, NULL, 'C');
#endif

void can_init(void)
{
    __ASSERT_INTERRUPT();

    CAN_CONTEXT_LOCK();

    while (CAN_OK != can.begin(CAN_SPEEDSET, CAN_CLOCKSET)) {
        LOG_ERR("can init failed");
        k_sleep(K_MSEC(500));
    }

#if CONFIG_CAN_SOFT_FILTERING == 0
    const unsigned long filter_broadcast = caniot_device_get_filter_broadcast();
#if CONFIG_DEVICE_SINGLE_INSTANCE
    const unsigned long mask        = caniot_device_get_mask();
    const unsigned long filter_self = caniot_device_get_filter(DEVICE_DID);
#else
    const unsigned long mask        = caniot_device_get_mask_by_cls();
    const unsigned long filter_self = caniot_device_get_filter_by_cls(__DEVICE_CLS__);
#endif

    can.init_Mask(0u, CAN_STDID, mask);
    can.init_Filt(0u, CAN_STDID, filter_self);
    can.init_Filt(1u, CAN_STDID, filter_self);

    can.init_Mask(1u, CAN_STDID, mask);
    can.init_Filt(2u, CAN_STDID, filter_broadcast);
    can.init_Filt(3u, CAN_STDID, filter_broadcast);
    can.init_Filt(4u, CAN_STDID, filter_broadcast);
    can.init_Filt(5u, CAN_STDID, filter_broadcast);
#elif CONFIG_DEVICE_SINGLE_INSTANCE
    can.init_Mask(0u, CAN_EXTID, 0x0ul);
    can.init_Mask(1u, CAN_EXTID, 0x0ul);
#else
#error "CONFIG_CAN_SOFT_FILTERING not supported for multi instance devices"
#endif

    CAN_CONTEXT_UNLOCK();
}

ISR(BSP_CAN_INT_vect)
{
#if DEBUG_INT
    serial_transmit('%');
#endif

    int8_t ready = dev_trigger_process();

    /* Immediately yield to schedule main thread */
    if (ready > 0)
        k_yield_from_isr();
}

int can_recv(can_message *msg)
{
    __ASSERT_NOTNULL(msg);

    int8_t rc;
    uint8_t isext, rtr;

    CAN_CONTEXT_LOCK();

    rc = can.readMsgBufID(can.readRxTxStatus(),
                          (unsigned long *)&msg->id,
                          &isext,
                          &rtr,
                          &msg->len,
                          msg->buf);
    if (rc == CAN_NOMSG) {
        rc = -EAGAIN;
        goto exit;
    } else if (rc != 0) {
        LOG_ERR("CAN readMsgBufID failed err: %d", rc);
        rc = -EIO;
        goto exit;
    }

    msg->isext = isext ? CAN_EXTID : CAN_STDID;
    msg->rtr   = rtr ? 1 : 0;

    LOG_DBG_RAW("CAN RX ext: %u rtr: %u id: %04x%04x: ",
                isext,
                rtr,
                (uint16_t)(msg->id >> 16u),
                (uint16_t)(msg->id & 0xFFFFu));
    LOG_HEXDUMP_DBG(msg->buf, msg->len);

exit:
    CAN_CONTEXT_UNLOCK();

    return rc;
}

#if CONFIG_CAN_WTD_MAX_ERROR_COUNT != -1
static void can_watchdog(bool ok)
{
    static uint8_t can_wtd_error_count = 0u;

    if (ok) {
        can_wtd_error_count = 0u;
    } else {
        can_wtd_error_count++;
        LOG_ERR("CAN watchdog: %u", can_wtd_error_count);
        if (can_wtd_error_count >= CONFIG_CAN_WTD_MAX_ERROR_COUNT) {
            platform_reset(false);
        }
    }
}
#endif

static uint8_t can_send(can_message *msg)
{
    __ASSERT_NOTNULL(msg);

    // can_print_msg(&msg);

    CAN_CONTEXT_LOCK();
    uint8_t rc = can.sendMsgBuf(msg->id, msg->isext, msg->rtr, msg->len, msg->buf, true);
    CAN_CONTEXT_UNLOCK();

    // LOG_ERR("CAN sendMsgBuf err: %d", rc);

    return rc;
}

// print can_message
void can_print_msg(can_message *msg)
{
    LOG_DBG("id: %08lx, isext: %d, rtr: %d, len: %d : ",
            msg->id,
            msg->isext,
            msg->rtr,
            msg->len);

    LOG_HEXDUMP_DBG(msg->buf, MIN(msg->len, 8U));
}

int can_txq_message(can_message *msg)
{
    int ret = k_msgq_put(&txq, msg, K_NO_WAIT);

    if (ret == 0) {
#if CONFIG_CAN_WORKQ_OFFLOADED
        k_system_workqueue_submit(&can_tx_work);
#endif
    } else if (ret == -ENOMEM) {
        LOG_ERR("can txq full");
    }

    return ret;
}

#if CONFIG_CAN_WORKQ_OFFLOADED
static void can_tx_wq_cb(struct k_work *work)
{
    can_message msg;
    while (k_msgq_get(&txq, &msg, K_NO_WAIT) == 0) {
        uint8_t rc = can_send(&msg);

#if CONFIG_CAN_WTD_MAX_ERROR_COUNT != -1
        can_watchdog(rc == CAN_OK);
#else
        (void)rc;
#endif
    }
}
#else
static void can_tx_entry(void *arg)
{
    can_message msg;
    while (1) {
        if (k_msgq_get(&txq, &msg, K_FOREVER) == 0) {
            uint8_t rc = can_send(&msg);

#if CONFIG_CAN_WTD_MAX_ERROR_COUNT != -1
            can_watchdog(rc == CAN_OK);
#else
            (void)rc;
#endif
        }
    }
}
#endif