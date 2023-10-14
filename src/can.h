/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DEV_CAN_H_
#define _CANIOT_DEV_CAN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_CAN_THREAD_STACK_SIZE 110u

typedef struct {
    union {
        uint32_t std : 11;
        uint32_t ext : 29;
        uint32_t id;
    };
    struct {
        uint8_t buf[8];
        uint8_t len;
    };

    uint8_t isext : 1;
    uint8_t rtr : 1;

} __attribute__((packed)) can_message;

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
int can_recv(can_message *msg);

int can_txq_message(can_message *msg);

void can_print_msg(can_message *msg);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_CAN_H_ */