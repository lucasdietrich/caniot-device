/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DEV_CAN_H_
#define _CANIOT_DEV_CAN_H_

#include <stdint.h>

#include <avrtos/drivers/can.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_CAN_THREAD_STACK_SIZE 110u

/**
 * @brief Initialize CAN bus
 */
void can_init(void);

/**
 * @brief Receive a CAN message
 *
 * @param msg Buffer to store the message
 * @return int
 *  * 0 if no message is available
 *  * -EAGAIN if no message is available
 *  * -EIO if device error occured
 */
int can_recv(struct can_frame *msg);

int can_txq_message(const struct can_frame *msg);

void can_print_msg(const struct can_frame *msg);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_CAN_H_ */