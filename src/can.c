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
#include <avrtos/devices/mcp2515.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL

#define K_MODULE_CAN 0x21
#define K_MODULE     K_MODULE_CAN

#if CONFIG_CAN_CLOCKSET_16MHZ
#define CAN_CLOCKSET MCP_16MHz
#else
#define CAN_CLOCKSET MCP_8MHz
#endif

#define CAN_SPEEDSET CAN_500KBPS

#define CONFIG_CAN_THREAD_OFFLOADED !CONFIG_CAN_WORKQ_OFFLOADED

K_MSGQ_DEFINE(txq, sizeof(struct can_frame), CONFIG_CAN_TX_MSGQ_SIZE);

#if CONFIG_CAN_WORKQ_OFFLOADED
static void can_tx_wq_cb(struct k_work *work);
static K_WORK_DEFINE(can_tx_work, can_tx_wq_cb);
#else
static void can_tx_entry(void *arg);
K_THREAD_DEFINE(
    can_tx_thread, can_tx_entry, CONFIG_CAN_THREAD_STACK_SIZE, K_COOPERATIVE, NULL, 'C');
#endif

static struct mcp2515_device mcp;

void can_init(void)
{
    __ASSERT_INTERRUPT();

	const struct spi_config spi_cfg = {
		.role		 = SPI_ROLE_MASTER,
		.polarity	 = SPI_CLOCK_POLARITY_RISING,
		.phase		 = SPI_CLOCK_PHASE_SAMPLE,
		.prescaler	 = SPI_PRESCALER_4,
		.irq_enabled = 0u,
	};

	struct spi_slave spi_slave = {
		.cs_port	  = BSP_CAN_SS_GPIO_DEVICE,
		.cs_pin		  = BSP_CAN_SS_GPIO_PIN,
		.active_state = GPIO_LOW,
		.regs		  = spi_config_into_regs(spi_cfg),
	};

	const struct mcp2515_config mcp_cfg = {
		.can_speed	 = MCP2515_CAN_SPEED_500KBPS,
		.clock_speed = MCP2515_CLOCK_SET_16MHZ,
		.flags		 = MCP2515_INT_RX,
	};

    spi_init(spi_cfg);

    while (mcp2515_init(&mcp, &mcp_cfg, &spi_slave) != 0) {
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

    mcp2515_set_mask(&mcp, 0u, CAN_STD_ID, mask);
	mcp2515_set_filter(&mcp, 0u, CAN_STD_ID, filter_self);
	mcp2515_set_filter(&mcp, 1u, CAN_STD_ID, filter_self);

	mcp2515_set_mask(&mcp, 1u, CAN_STD_ID, mask);
	mcp2515_set_filter(&mcp, 2u, CAN_STD_ID, filter_broadcast);
	mcp2515_set_filter(&mcp, 3u, CAN_STD_ID, filter_broadcast);
	mcp2515_set_filter(&mcp, 4u, CAN_STD_ID, filter_broadcast);
	mcp2515_set_filter(&mcp, 5u, CAN_STD_ID, filter_broadcast);
#elif CONFIG_DEVICE_SINGLE_INSTANCE
    mcp2515_set_mask(&mcp, 0u, CAN_EXT_ID, 0x0ul);
	mcp2515_set_mask(&mcp, 1u, CAN_EXT_ID, 0x0ul);
#else
#error "CONFIG_CAN_SOFT_FILTERING not supported for multi instance devices"
#endif
}

ISR(BSP_CAN_INT_vect)
{
#if DEBUG_INT
    serial_transmit('%');
#endif

    int8_t ret = dev_trigger_process();

    /* Immediately yield to schedule main thread */
    if (ret > 0) k_yield_from_isr();
}

int can_recv(struct can_frame *msg)
{
    __ASSERT_NOTNULL(msg);

    int8_t rc;

    rc = mcp2515_recv(&mcp, msg);
    if (rc == -ENOMSG) {
        rc = -EAGAIN;
        goto exit;
    } else if (rc != 0) {
        LOG_ERR("mcp2515_recv failed err: %d", rc);
        rc = -EIO;
        goto exit;
    }

    LOG_DBG_RAW("CAN RX ext: %u rtr: %u id: %04x%04x: ",
                msg->is_ext,
                msg->rtr,
                (uint16_t)(msg->id >> 16u),
                (uint16_t)(msg->id & 0xFFFFu));
    LOG_HEXDUMP_DBG(msg->data, msg->len);

exit:
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

static uint8_t can_send(const struct can_frame *msg)
{
    __ASSERT_NOTNULL(msg);

    // can_print_msg(&msg);

    int8_t rc = mcp2515_send(&mcp, msg);

    // LOG_ERR("mcp2515_send err: %d", rc);

    return rc;
}

// print can_frame
void can_print_msg(const struct can_frame *msg)
{
    LOG_DBG("id: %08lx, isext: %d, rtr: %d, len: %d : ",
            msg->id,
            msg->is_ext,
            msg->rtr,
            msg->len);

    LOG_HEXDUMP_DBG(msg->data, MIN(msg->len, 8U));
}

int can_txq_message(const struct can_frame *msg)
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
    struct can_frame msg;
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
    struct can_frame msg;
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