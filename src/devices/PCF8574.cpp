#include "PCF8574.h"

#include <Wire.h>

void pcf8574_init(void)
{

}

void pcf8574_set(uint8_t value)
{
	Wire.beginTransmission((uint8_t)PCF8574_ADDR);
	Wire.write(value);
	Wire.endTransmission();
}

uint8_t pcf8574_get(void)
{
	uint8_t value = 0;
	Wire.requestFrom(PCF8574_ADDR, 1u, true);
	if (Wire.available() == 1) {
		value = Wire.read();
	}
	return value;
}