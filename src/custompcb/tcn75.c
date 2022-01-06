#include "tcn75.h"

// @see DS21490D-page 11
float tcn75_temp2float(uint8_t t1, uint8_t t2)
{
	float temp;

	const uint8_t msb = t1 >> 7;
	const uint8_t value = (t1 << 1) | (t2 >> 7);

	/* temp < 0 */
	if (msb) {
		temp = -0.5 * ~value;
	} else { /* temp >= 0 */
		temp = 0.5 * value;
	}

	return temp;
}

int16_t tcn75_temp2int16(uint8_t t1, uint8_t t2)
{
	int16_t scaled;
	const uint8_t msb = t1 >> 7;
	const uint8_t abs = (t1 << 1) | (t2 >> 7);
	if (msb) {
		scaled = -50 * ~abs;
	} else {
		scaled = 50 * abs;
	}
	return scaled;
}

float tcn75_int16tofloat(int16_t t)
{
	return t / 100.0;
}