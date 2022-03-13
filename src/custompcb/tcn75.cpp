#include "tcn75.h"

#include <Wire.h>
#include <avrtos/kernel.h>

static void tcn75_configure(void)
{
	Wire.beginTransmission(TCN75_ADDR);
	Wire.write(TCN75_CONFIG_REGISTER);
	Wire.write(TCN75_NORMAL_OPERATION |
		   TCN75_COMPARATOR_MODE |
		   TCN75_COMPINT_POLARITY_ACTIVE_LOW |
		   TCN75_FAULT_QUEUE_NB_CONVERSION_6 |
		//    TCN75_RESOLUTION_12BIT |
		   TCN75_CONTINUOUS);
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

	tcn75_select_data_register();
}

int16_t tcn75_read(void)
{
	__ASSERT_INTERRUPT();

	uint8_t data[2];
	int16_t temperature = INT16_MIN;

	tcn75_select_data_register();

	Wire.requestFrom(TCN75_ADDR, 2);
	if (Wire.available() == 2) {
		data[0] = Wire.read();
		data[1] = Wire.read();

		temperature = tcn75_temp2int16(data[0], data[1]);
	} else {
#if DEBUG_TCN75
		printf_P(PSTR("Error reading Wire bytes\n"));
#endif /* DEBUG_TCN75 */
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