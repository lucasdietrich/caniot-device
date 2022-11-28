#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

/* General configuration */

#define CONFIG_CAN_THREAD_STACK_SIZE 110u


#if !defined(CONFIG_GPIO_PULSE_SUPPORT)
#	define CONFIG_GPIO_PULSE_SUPPORT 0U
#endif 

#if !defined(CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT)
#	define CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT 	8u
#endif

#if !defined(CONFIG_WATCHDOG)
#	define CONFIG_WATCHDOG 0u
#endif 

#if !defined(CONFIG_CAN_CLOCKSET_16MHZ)
#	define CONFIG_CAN_CLOCKSET_16MHZ 1U
#endif 

#if !defined(CONFIG_FORCE_RESTORE_DEFAULT_CONFIG)
#	define CONFIG_FORCE_RESTORE_DEFAULT_CONFIG 0U
#endif

#if !defined(CONFIG_OW_DS_COUNT)
#	define CONFIG_OW_DS_COUNT 0U
#endif

#if !defined(CONFIG_OW_DS_ENABLED)
#	define CONFIG_OW_DS_ENABLED 0U
#endif

#if !defined(CONFIG_HEATERS_COUNT)
#	define CONFIG_HEATERS_COUNT 0U
#endif

#if !defined(CONFIG_SHUTTERS_COUNT)
#	define CONFIG_SHUTTERS_COUNT 0U
#endif

#if !defined(CONFIG_WORKQUEUE_HEATERS_EXECUTION)
#	define CONFIG_WORKQUEUE_HEATERS_EXECUTION 1u
#endif

#if !defined(CONFIG_WORKQUEUE_SHUTTERS_EXECUTION)
#	define CONFIG_WORKQUEUE_SHUTTERS_EXECUTION 1u
#endif

#if (CONFIG_OW_DS_ENABLED == 0U) && (CONFIG_OW_DS_COUNT != 0U)
#	warning CONFIG_OW_DS_COUNT > 0 but OW sensors are disabled
#endif

#if !defined(CONFIG_OW_DS_PROCESS_PERIOD_MS)
#	define CONFIG_OW_DS_PROCESS_PERIOD_MS 10000U
#endif

#if !defined(CONFIG_USART_SHELL)
#	define CONFIG_USART_SHELL 0u
#endif

#if !defined(CONFIG_PCF8574)
#	define CONFIG_PCF8574 0u
#endif

#if !defined(CONFIG_PCF8574A)
#	define CONFIG_PCF8574A 0u
#endif

#define CONFIG_PCF8574_ENABLED (CONFIG_PCF8574 || CONFIG_PCF8574A)

#if CONFIG_PCF8574_ENABLED
#	define CONFIG_EXTIO_ENABLED 1u
#else
#	define CONFIG_EXTIO_ENABLED 0u
#endif

#if !defined(CONFIG_TCN75)
#	define CONFIG_TCN75 0u
#endif

#if !defined(CONFIG_LOGGING_ENABLED)
#	define CONFIG_LOGGING_ENABLED 1u
#endif

#if !defined(CONFIG_CAN_CONTEXT_LOCK)
#	define CONFIG_CAN_CONTEXT_LOCK 0u
#endif

#if !defined(CONFIG_CHECKS)
#	define CONFIG_CHECKS 1u
#endif

#define CONFIG_CLASS __DEVICE_CLS__

#if __DEVICE_CLS__ == 0
#	define CONFIG_CLASS0_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 1
#	define CONFIG_CLASS1_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 2
#	define CONFIG_CLASS2_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 3
#	define CONFIG_CLASS3_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 4
#	define CONFIG_CLASS4_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 5
#	define CONFIG_CLASS5_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 6
#	define CONFIG_CLASS6_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 7
#	define CONFIG_CLASS7_ENABLED 1U
#endif

#endif /* _APP_CONFIG_H_ */