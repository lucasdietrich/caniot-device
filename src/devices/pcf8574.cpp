#include "config.h"
#include "pcf8574.h"

#include <avrtos/kernel.h>

#include <Wire.h>

#if CONFIG_PCF8574_ENABLED

#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_NONE

void pcf8574_init(uint8_t i2c_addr)
{
}

void pcf8574_set(uint8_t i2c_addr, uint8_t value)
{
	Wire.beginTransmission(i2c_addr);
	size_t w   = Wire.write(value);
	uint8_t et = Wire.endTransmission();

	LOG_DBG("PCF8574 val=%u w=%u et=%u", value, w, et);
}

uint8_t pcf8574_get(uint8_t i2c_addr)
{
	uint8_t value = 0;
	Wire.requestFrom((int)i2c_addr, 1u, true);
	if (Wire.available() == 1) {
		value = Wire.read();
		LOG_DBG("PCF8574 val=%u", value);
	}
	return value;
}

#endif