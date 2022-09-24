#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

/**
 * @brief CONFIG_INPUTS_INT_MASK is a for IN0, IN1, IN2, IN3
 */
#if !defined(CONFIG_INPUTS_INT_MASK)
#	define CONFIG_INPUTS_INT_MASK 0x00U
#endif

#if !defined(CONFIG_GPIO_PULSE_SUPPORT)
#	define CONFIG_GPIO_PULSE_SUPPORT 0U
#endif 

#if !defined(CONFIG_CAN_INT)
#	define CONFIG_CAN_INT 0U
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

#endif /* _APP_CONFIG_H_ */