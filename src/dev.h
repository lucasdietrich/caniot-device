/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include "config.h"

#include <avrtos/avrtos.h>

#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_DEVICE_SINGLE_INSTANCE
#define DEVICE_DID CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__)
#endif

#define DEVICE_CLASS __DEVICE_CLS__

/**
 * @brief Print the device CANIOT identification.
 */
void dev_print_indentification(void);

/**
 * @brief Return the index of the device instance.
 *
 * @param dev
 * @return uint8_t
 */
uint8_t dev_get_instance_index(struct caniot_device *dev);

/**
 * @brief Process the CANIOT device.
 *
 * @param tid Thread watchdog ID to use when calling alive() function.
 *
 * @return int
 */
int dev_process(uint8_t tid);

/**
 * @brief Initialize the CANIOT device.
 */
void dev_init(void);

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
static inline int8_t dev_trigger_process(void)
{
    return k_signal_raise(&dev_process_sig, 0);
}

/* Utility functions */

/**
 * @brief Get the device configuration size in EEPROM.
 *
 * @param dev
 * @return uint16_t
 */
void dev_settings_restore_default(void);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */