/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Platform specific definitions */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stddef.h>
#include <stdint.h>

#include <caniot/caniot.h>

/**
 * @brief Fill buffer with random data
 *
 * @param buf
 * @param len
 */
void platform_entropy(uint8_t *buf, size_t len);

/**
 * @brief Get platform current time.
 *
 * Difference between two calls is garantied to be the real time elapsed.
 *
 * @param sec
 * @param ms
 */
void platform_get_time(uint32_t *sec, uint16_t *ms);

/**
 * @brief Set platform current time.
 *
 * @param sec
 */
void platform_set_time(uint32_t sec);

/**
 * @brief Platform specific function to receive a caniot frame.
 *
 * @param frame Buffer to store the received frame.
 * @return int 0 on success, -CANIOT_EAGAIN if no frame is available, other negative value
 * on error.
 */
int platform_caniot_recv(struct caniot_frame *frame);

/**
 * @brief Platform specific function to send a caniot frame.
 *
 * @param frame Frame to send.
 * @param delay_ms Delay in milliseconds before sending the frame.
 * @return int 0 on success, negative value on error.
 *
 * If CONFIG_CAN_DELAYABLE_TX option is disabled, the frame is always sent without delay.
 */
int platform_caniot_send(const struct caniot_frame *frame, uint32_t delay_ms);

/**
 * @brief Platform specific function to reset the device.
 */
int platform_reset(void);

/**
 * @brief Platform specific function to enable/disable the watchdog.
 */
int platform_watchdog_enable(bool enable);

#endif /* _PLATFORM_H_ */