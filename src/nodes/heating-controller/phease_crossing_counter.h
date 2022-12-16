#ifndef _HEATING_CONTROLLER_PHEASE_CROSSING_COUNTER_H_
#define _HEATING_CONTROLLER_PHEASE_CROSSING_COUNTER_H_

#include <stdint.h>

/**
 * @brief Initialize the phease crossing counter.
 */
void pcc_init(void);

/**
 * @brief Get the current calculated frequency.
 * 
 * @return uint8_t 
 */
uint8_t pcc_get_current_frequency(void);

#endif /* _HEATING_CONTROLLER_PHEASE_CROSSING_COUNTER_H_ */