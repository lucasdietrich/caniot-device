#if CONFIG_PCF8574

#include "PCF8574.h"

#include <avrtos/kernel.h>

#include <Wire.h>

void pcf8574_init(uint8_t addr)
{
	
}

void pcf8574_set(uint8_t addr, uint8_t value)
{
	Wire.beginTransmission((uint8_t) (addr & 0x7Fu));
	Wire.write(value);
	Wire.endTransmission();
}

uint8_t pcf8574_get(uint8_t addr)
{
	uint8_t value = 0;
	Wire.requestFrom((int) (addr & 0x7Fu), 1u, true);
	if (Wire.available() == 1) {
		value = Wire.read();
	}
	return value;
}

#endif