#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#if !defined(CONFIG_APP_OW_EXTTEMP)
#	define CONFIG_APP_OW_EXTTEMP 0U
#endif

#if !defined(CONFIG_APP_OW_EXTTEMP_INIT_DELAY_MS)
#	define CONFIG_APP_OW_EXTTEMP_INIT_DELAY_MS 10000U
#endif 

#if !defined(CONFIG_APP_PCINT_MASK)
#	define CONFIG_APP_PCINT_MASK 0x00U
#endif

#endif /* _APP_CONFIG_H_ */