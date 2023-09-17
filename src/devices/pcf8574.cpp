#include "config.h"
#include "pcf8574.h"

#include <avrtos/avrtos.h>

#include <Wire.h>

#if CONFIG_PCF8574_ENABLED

#include <avrtos/logging.h>

#define LOG_LEVEL CONFIG_PCF8574_LOG_LEVEL

void pcf8574_init(struct pcf8574_state *pcf, uint8_t i2c_addr)
{
	pcf->i2c_address = i2c_addr;
#if CONFIG_PCF8574_BUFFERED_READ
	pcf->read_buffer_valid = 0u;
#endif
#if CONFIG_PCF8574_BUFFERED_WRITE
	pcf->write_buffer = 0u;
#endif
}

void pcf8574_set(struct pcf8574_state *pcf, uint8_t value)
{
#if CONFIG_PCF8574_BUFFERED_WRITE
	if (pcf->write_buffer == value) return;
#endif

	Wire.beginTransmission(pcf->i2c_address);
	size_t w = Wire.write(value);
	Wire.endTransmission();

	LOG_DBG("PCF8574 I2C w x%02x ok: %u", value, w);

#if CONFIG_PCF8574_BUFFERED_WRITE
	if (w != 0) pcf->write_buffer = value;
#endif
}

uint8_t pcf8574_get(struct pcf8574_state *pcf)
{
#if CONFIG_PCF8574_BUFFERED_READ
	if (pcf->read_buffer_valid) return pcf->read_buffer;
#endif

	uint8_t value = 0u;
	Wire.requestFrom((int)pcf->i2c_address, 1u, true);
	if (Wire.available() == 1) {
		value = Wire.read();
		LOG_DBG("PCF8574 I2C r x%02x ok", value);
	}

#if CONFIG_PCF8574_BUFFERED_READ
	pcf->read_buffer       = value;
	pcf->read_buffer_valid = 1u;
#endif

	return value;
}

#endif