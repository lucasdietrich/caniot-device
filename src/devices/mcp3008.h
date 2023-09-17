#ifndef _MCP3008_H_
#define _MCP3008_H_

#include "config.h"

#include <stdint.h>

#define MCP3008_SINGLE_ENDED 1u
#define MCP3008_DIFFERENTIAL 0u

#define MCP3008_ADC_RESOLUTION 10u
#define MCP3008_ADC_MAX_VALUE  ((1u << MCP3008_ADC_RESOLUTION) - 1u)

/**
 * @brief Initialize MCP3008 driver
 */
void mcp3008_init(void);

/**
 * @brief Read a single channel from MCP3008
 *
 * @param channel
 * @return uint16_t Measured value (10 bits)
 */
uint16_t mcp3008_read(uint8_t channel);

/**
 * @brief Read all channels from MCP3008
 *
 * @param values Array of 8 elements to store the measured values (10 bits)
 * @return int
 */
int mcp3008_read_all(uint16_t *values);

#endif /* _MCP3008_H_ */