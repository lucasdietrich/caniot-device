/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

/* General configuration */

/* Tells whether the device is a multi-instance device */
#if !defined(__MULTI_INSTANCES__)
#define __MULTI_INSTANCES__ 0u
#endif

#if !defined(__MULTI_INSTANCES_COUNT__)
#define __MULTI_INSTANCES_COUNT__ 0u
#endif

#if !__MULTI_INSTANCES__
#elif __MULTI_INSTANCES_COUNT__ > 8
#error "With __MULTI_INSTANCES__ enabled, maximum number of instances is 8"
#elif __MULTI_INSTANCES_COUNT__ <= 1
#error "With __MULTI_INSTANCES__ enabled, minimum number of instances is 2"
#endif

#if !defined(CONFIG_GPIO_PULSE_SUPPORT)
#define CONFIG_GPIO_PULSE_SUPPORT 0U
#endif

#if !defined(CONFIG_MAIN_LOG_LEVEL)
#define CONFIG_MAIN_LOG_LEVEL LOG_LEVEL_NONE
#endif

#if !defined(CONFIG_DEVICE_LOG_LEVEL)
#define CONFIG_DEVICE_LOG_LEVEL LOG_LEVEL_NONE
#endif

#if !defined(CONFIG_BOARD_LOG_LEVEL)
#define CONFIG_BOARD_LOG_LEVEL LOG_LEVEL_NONE
#endif

#if !defined(CONFIG_CAN_LOG_LEVEL)
#define CONFIG_CAN_LOG_LEVEL LOG_LEVEL_NONE
#endif

#if !defined(CONFIG_PCF8574_LOG_LEVEL)
#define CONFIG_PCF8574_LOG_LEVEL LOG_LEVEL_NONE
#endif

#if !defined(CONFIG_MCP3008_LOG_LEVEL)
#define CONFIG_MCP3008_LOG_LEVEL LOG_LEVEL_NONE
#endif

#if !defined(CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT)
#define CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT 8u
#endif

#if !defined(CONFIG_WATCHDOG)
#define CONFIG_WATCHDOG 0u
#endif

#if !defined(CONFIG_CAN_CLOCKSET_16MHZ)
#define CONFIG_CAN_CLOCKSET_16MHZ 1U
#endif

#if !defined(CONFIG_CAN_SOFT_FILTERING)
#define CONFIG_CAN_SOFT_FILTERING 0
#endif

#if !defined(CONFIG_FORCE_RESTORE_DEFAULT_CONFIG)
#define CONFIG_FORCE_RESTORE_DEFAULT_CONFIG 0U
#endif

#if !defined(CONFIG_APP_MAX_PROCESS_INTERVAL_MS)
#define CONFIG_APP_MAX_PROCESS_INTERVAL_MS 1000U
#endif

#if !defined(CONFIG_OW_DS_COUNT)
#define CONFIG_OW_DS_COUNT 0U
#endif

#if !defined(CONFIG_OW_DS_ENABLED)
#define CONFIG_OW_DS_ENABLED 0U
#endif

#if !defined(CONFIG_HEATERS_COUNT)
#define CONFIG_HEATERS_COUNT 0U
#endif

#if !defined(CONFIG_SHUTTERS_COUNT)
#define CONFIG_SHUTTERS_COUNT 0U
#endif

#if (CONFIG_OW_DS_ENABLED == 0U) && (CONFIG_OW_DS_COUNT != 0U)
#warning CONFIG_OW_DS_COUNT > 0 but OW sensors are disabled
#endif

#if !defined(CONFIG_OW_DS_PROCESS_PERIOD_MS)
#define CONFIG_OW_DS_PROCESS_PERIOD_MS 10000U
#endif

#if !defined(CONFIG_SHELL)
#define CONFIG_SHELL 0u
#endif

#if !defined(CONFIG_SHELL_WORKQ_OFFLOADED)
#define CONFIG_SHELL_WORKQ_OFFLOADED 1u
#endif

#if !defined(CONFIG_TEST_STRESS)
#define CONFIG_TEST_STRESS 0u
#endif

#if !defined(CONFIG_PCF8574)
#define CONFIG_PCF8574 0u
#endif

#if !defined(CONFIG_PCF8574A)
#define CONFIG_PCF8574A 0u
#endif

#define CONFIG_PCF8574_ENABLED (CONFIG_PCF8574 || CONFIG_PCF8574A)

#if !defined(CONFIG_MCP3008_ENABLED)
#define CONFIG_MCP3008_ENABLED 0u
#endif

#if !defined(CONFIG_PCF8574_INT_ENABLED)
#define CONFIG_PCF8574_INT_ENABLED 0u
#endif

#if !defined(CONFIG_PCF8574_BUFFERED_READ)
#define CONFIG_PCF8574_BUFFERED_READ 0u
#endif

#if !defined(CONFIG_PCF8574_BUFFERED_WRITE)
#define CONFIG_PCF8574_BUFFERED_WRITE 0u
#endif

#if CONFIG_PCF8574_BUFFERED_READ && !CONFIG_PCF8574_INT_ENABLED
#error "Buffered read requires interrupt support"
#endif

#if CONFIG_PCF8574_ENABLED
#define CONFIG_EXTIO_ENABLED 1u
#else
#define CONFIG_EXTIO_ENABLED 0u
#endif

#if !defined(CONFIG_TCN75)
#define CONFIG_TCN75 0u
#endif

#if !defined(CONFIG_CAN_CONTEXT_LOCK)
#define CONFIG_CAN_CONTEXT_LOCK 0u
#endif

#if !defined(CONFIG_CAN_DELAYABLE_TX)
#define CONFIG_CAN_DELAYABLE_TX 1u
#endif

#if !defined(CONFIG_CAN_DELAYABLE_TX_BUFFER)
#define CONFIG_CAN_DELAYABLE_TX_BUFFER 1u
#endif

#if !defined(CONFIG_CAN_WORKQ_OFFLOADED)
#define CONFIG_CAN_WORKQ_OFFLOADED 0u
#endif

#if !defined(CONFIG_CAN_TX_MSGQ_SIZE)
#define CONFIG_CAN_TX_MSGQ_SIZE 1u
#endif

// Enabled if value is different from -1
#if !defined(CONFIG_CAN_WTD_MAX_ERROR_COUNT)
#define CONFIG_CAN_WTD_MAX_ERROR_COUNT -1
#endif
#if CONFIG_CAN_WTD_MAX_ERROR_COUNT == 0
#error "CONFIG_CAN_WTD_MAX_ERROR_COUNT must be different from 0"
#endif

#if !defined(CONFIG_CHECKS)
#define CONFIG_CHECKS 1u
#endif

#define CONFIG_CLASS __DEVICE_CLS__

#if __DEVICE_CLS__ == 0
#define CONFIG_CLASS0_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 1
#define CONFIG_CLASS1_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 2
#define CONFIG_CLASS2_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 3
#define CONFIG_CLASS3_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 4
#define CONFIG_CLASS4_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 5
#define CONFIG_CLASS5_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 6
#define CONFIG_CLASS6_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 7
#define CONFIG_CLASS7_ENABLED 1U
#endif

#if __MULTI_INSTANCES__
#define CONFIG_DEVICE_INSTANCES_COUNT __MULTI_INSTANCES_COUNT__
#define CONFIG_DEVICE_SINGLE_INSTANCE 0

#define CONFIG_DEVICE_MULTI_FIRST_SID 0u
#define CONFIG_DEVICE_MULTI_LAST_SID (__MULTI_INSTANCES_COUNT__ - 1u)
#define CONFIG_DEVICE_MULTI_SID(_n)  (CONFIG_DEVICE_MULTI_FIRST_SID + (_n))
#if defined(__DEVICE_SID__)
#error "__DEVICE_SID__ must not be defined when __MULTI_INSTANCES__ is enabled"
#endif
#else
#define CONFIG_DEVICE_INSTANCES_COUNT 1
#define CONFIG_DEVICE_SINGLE_INSTANCE 1
#endif


#ifndef CONFIG_DIAG_RESET_COUNTERS
#define CONFIG_DIAG_RESET_COUNTERS 1u
#endif

#ifndef CONFIG_DIAG_THREAD_INFO
#define CONFIG_DIAG_THREAD_INFO 1u
#endif

#endif /* _APP_CONFIG_H_ */