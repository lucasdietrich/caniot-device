#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#if !defined(CONFIG_GPIO_PULSE_SUPPORT)
#	define CONFIG_GPIO_PULSE_SUPPORT 0U
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

#if (CONFIG_OW_DS_ENABLED == 0U) && (CONFIG_OW_DS_COUNT != 0U)
#	warning CONFIG_OW_DS_COUNT > 0 but OW sensors are disabled
#endif

#if !defined(CONFIG_OW_DS_PROCESS_PERIOD_MS)
#	define CONFIG_OW_DS_PROCESS_PERIOD_MS 10000U
#endif

#define CONFIG_CLASS __DEVICE_CLS__

#if __DEVICE_CLS__ == 0
#	define CONFIG_CLASS0_ENABLED 1U
#endif

#if __DEVICE_CLS__ == 1
#	define CONFIG_CLASS1_ENABLED 0U
#endif

#if __DEVICE_CLS__ == 2
#	define CONFIG_CLASS2_ENABLED 0U
#endif

#if __DEVICE_CLS__ == 3
#	define CONFIG_CLASS3_ENABLED 0U
#endif

#if __DEVICE_CLS__ == 4
#	define CONFIG_CLASS4_ENABLED 0U
#endif

#if __DEVICE_CLS__ == 5
#	define CONFIG_CLASS5_ENABLED 0U
#endif

#if __DEVICE_CLS__ == 6
#	define CONFIG_CLASS6_ENABLED 0U
#endif

#if __DEVICE_CLS__ == 7
#	define CONFIG_CLASS7_ENABLED 0U
#endif

#endif /* _APP_CONFIG_H_ */