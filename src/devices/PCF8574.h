#ifndef _PCF8574_H_
#define _PCF8574_H_

#include <stdint.h>

/* PCF8574 */

#define PCF8574_ADDR_BASE	(0x20u)

#define CONFIG_PCF8574_A0	(0u)
#define CONFIG_PCF8574_A1	(0u)
#define CONFIG_PCF8574_A2	(0u)

#define PCF8574_ADDR       	(PCF8574_ADDR_BASE | \
				(CONFIG_PCF8574_A2 << 2) |\
				(CONFIG_PCF8574_A1 << 1) |\
				(CONFIG_PCF8574_A0))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the PCF8574 device
 * 
 */
void pcf8574_init(void);

/**
 * @brief Write value to pcf8574
 * 
 * @return uint8_t 
 */
void pcf8574_set(uint8_t value);

/**
 * @brief Read from pcf8574
 * 
 * @return uint8_t 
 */
uint8_t pcf8574_get(void);

#ifdef __cplusplus
}
#endif


#endif /* _PCF8574_H_ */