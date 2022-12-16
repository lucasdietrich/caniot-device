#include "tcn75.h"

#include <Wire.h>
#include <avrtos/kernel.h>

#define K_MODULE_TCN75  0x23
#define K_MODULE K_MODULE_TCN75

#include <avrtos/logging.h>
#if defined(CONFIG_TCN75_LOG_LEVEL)
#	define LOG_LEVEL CONFIG_TCN75_LOG_LEVEL
#else
#	define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define TCN75_ALWAYS_SELECT_DATA_REGISTER 0x00

static void tcn75_configure(void)
{
	const uint8_t value = 
		TCN75_NORMAL_OPERATION |
		TCN75_COMPARATOR_MODE |
		TCN75_COMPINT_POLARITY_ACTIVE_LOW |
		TCN75_FAULT_QUEUE_NB_CONVERSION_6 |
	     	TCN75_RESOLUTION_12BIT |
		TCN75_CONTINUOUS;

	Wire.beginTransmission(TCN75_ADDR);
	Wire.write((uint8_t)TCN75_CONFIG_REGISTER);
	Wire.write(value);
	Wire.endTransmission();
}

static void tcn75_select_data_register(void)
{
	__ASSERT_INTERRUPT();

	Wire.beginTransmission(TCN75_ADDR);
	Wire.write((uint8_t)TCN75_TEMPERATURE_REGISTER);
	Wire.endTransmission();
}

void tcn75_init(void)
{
	__ASSERT_INTERRUPT();

	tcn75_configure();

#if !TCN75_ALWAYS_SELECT_DATA_REGISTER
	tcn75_select_data_register();
#endif
}

int16_t tcn75_read(void)
{
	__ASSERT_INTERRUPT();

	int16_t temperature = INT16_MAX;

#if TCN75_ALWAYS_SELECT_DATA_REGISTER
	tcn75_select_data_register();
#endif
	
	Wire.requestFrom(TCN75_ADDR, 2u);
	if (Wire.available() == 2u) {
		const uint8_t t1 = Wire.read();
		const uint8_t t2 = Wire.read();

		temperature = tcn75_temp2int16(t1, t2);
	} else {
		LOG_ERR("TCN75 read error");
	}

	return temperature;
}

/* works for all 9, 10, 11 or 12 bits conversion */
float tcn75_temp2float(uint8_t t1, uint8_t t2)
{
	float temp;

	const uint8_t sign = t1 >> 7;
	const uint16_t abs = ((t1 & 0x7F) << 4) | (t2 >> 4);

	if (sign) {
		temp = -0.0625 * abs;
	} else {
		temp = 0.0625 * abs;
	}

	return temp;
}

int16_t tcn75_temp2int16(uint8_t t1, uint8_t t2)
{
	int16_t temp;

	const uint8_t sign = t1 >> 7;
	const uint16_t abs = ((t1 & 0x7F) << 4) | (t2 >> 4);

	/* temp < 0 */
	if (sign) {
		temp = -6.25 * abs;
	} else { /* temp >= 0 */
		temp = 6.25 * abs;
	}

	return temp;
}

float tcn75_int16tofloat(int16_t t)
{
	return t / 100.0;
}