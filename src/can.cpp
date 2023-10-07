/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can.h"
#include "dev.h"

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

#define CAN_SPEEDSET     CAN_500KBPS
#define CAN_TX_MSGQ_SIZE 2u

static mcp2515_can can(BSP_CAN_SS_ARDUINO_PIN);

/* maybe unecessary */

#if CONFIG_CAN_CONTEXT_LOCK
static K_MUTEX_DEFINE(can_mutex_if);

#define CAN_CONTEXT_LOCK()   k_mutex_lock(&can_mutex_if, K_FOREVER);
#define CAN_CONTEXT_UNLOCK() k_mutex_unlock(&can_mutex_if);

#else

#define CAN_CONTEXT_LOCK()
#define CAN_CONTEXT_UNLOCK()

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
    const unsigned long mask             = caniot_device_get_mask();
    const unsigned long filter_self      = caniot_device_get_filter(did);
    const unsigned long filter_broadcast = caniot_device_get_filter_broadcast(did);

    can.init_Mask(0u, CAN_STDID, mask);
    can.init_Filt(0u, CAN_STDID, filter_self);
    can.init_Filt(1u, CAN_STDID, filter_self);

    can.init_Mask(1u, CAN_STDID, mask);
    can.init_Filt(2u, CAN_STDID, filter_broadcast);
    can.init_Filt(3u, CAN_STDID, filter_broadcast);
    can.init_Filt(4u, CAN_STDID, filter_broadcast);
    can.init_Filt(5u, CAN_STDID, filter_broadcast);
#else
    can.init_Mask(0u, CAN_EXTID, 0x0ul);
    can.init_Mask(1u, CAN_EXTID, 0x0ul);
#endif

    CAN_CONTEXT_UNLOCK();
}

ISR(BSP_CAN_INT_vect)
{
#if DEBUG_INT
    serial_transmit('%');
#endif

    struct k_thread *ready = trigger_process();

    /* Immediately yield to schedule main thread */
    k_yield_from_isr_cond(ready);
}

int can_recv(can_message *msg)
{
    __ASSERT_NOTNULL(msg);

    int8_t rc;
    uint8_t isext, rtr;

    CAN_CONTEXT_LOCK();

    if (can.checkReceive() != CAN_MSGAVAIL) {
        rc = -EAGAIN;
        goto exit;
    }

    rc = can.readMsgBufID(can.readRxTxStatus(),
                          (unsigned long *)&msg->id,
                          &isext,
                          &rtr,
                          &msg->len,
                          msg->buf);
    if (rc != 0) {
        LOG_ERR("CAN readMsgBufID failed err: %d", rc);
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

#if CONFIG_CAN_SOFT_FILTERING
    if (!caniot_device_targeted(did, msg->isext, msg->rtr, msg->id)) {
        LOG_WRN("CAN drop can msg");
        rc = -EAGAIN;
    }
#endif

    /* Valid message directed to us */

exit:
    CAN_CONTEXT_UNLOCK();

    return rc;
}

static int can_send(can_message *msg)
{
    __ASSERT_NOTNULL(msg);

    CAN_CONTEXT_LOCK();

    int rc = can.sendMsgBuf(msg->id, msg->isext, msg->rtr, msg->len, msg->buf, true);

    CAN_CONTEXT_UNLOCK();

    return rc;
}

K_MSGQ_DEFINE(txq, sizeof(can_message), CAN_TX_MSGQ_SIZE);

static void can_tx_entry(void *arg);

K_THREAD_DEFINE(
    can_tx_thread, can_tx_entry, CONFIG_CAN_THREAD_STACK_SIZE, K_COOPERATIVE, NULL, 'C');

static void can_tx_entry(void *arg)
{
    can_message msg;
    while (1) {
        if (k_msgq_get(&txq, &msg, K_FOREVER) == 0) {
            // can_print_msg(&msg);
            can_send(&msg);
        }
    }
}

int can_txq_message(can_message *msg)
{
    return k_msgq_put(&txq, msg, K_NO_WAIT);
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