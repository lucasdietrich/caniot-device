/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include <avrtos/avrtos.h>

#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_DID CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__)

/**
 * @brief Print the device CANIOT identification.
 */
void dev_print_indentification(void);

/**
 * @brief Process the CANIOT device.
 *
 * @return int
 */
int dev_process(void);

/**
 * @brief Initialize the CANIOT device.
 */
void dev_init(void);

/**
 * @brief Restore the default settings for the device.
 *
 * @return int
 */
int dev_restore_default(void);

/**
 * @brief Get the time to wait for the next process.
 *
 * @return uint32_t
 */
uint32_t dev_get_process_timeout(void);

/**
 * @brief Return whether telemetry is requested
 *
 * @return true
 * @return false
 */
bool dev_telemetry_is_requested(void);

/**
 * @brief Trigger the telemetry for the given endpoint.
 *
 * @param endpoint
 * @return true
 * @return false
 */
void dev_trigger_telemetry(caniot_endpoint_t endpoint);

/**
 * @brief Trigger the telemetry for the given endpoints bitmask.
 *
 * @param endpoints_bitmask
 */
void dev_trigger_telemetrys(uint8_t endpoints_bitmask);

/**
 * @brief Apply a board level control system command to the device.
 *
 * @param dev
 * @param sysc
 * @return int
 */
int dev_apply_blc_sys_command(struct caniot_device *dev,
                              struct caniot_blc_sys_command *sysc);

/**
 * @brief Signal used to trigger the device process.
 */
extern struct k_signal dev_process_sig;

/**
 * @brief Trigger the device process.
 *
 * This function must be inlined as it is often called from an ISR.
 *
 * @return struct k_thread*
 */
static inline struct k_thread *dev_trigger_process(void)
{
    return k_signal_raise(&dev_process_sig, 0);
}

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */