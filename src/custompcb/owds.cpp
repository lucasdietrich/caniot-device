#include <avrtos/kernel.h>

#include <OneWire.h>

#include "board.h"
#include "owds.h"

struct owdsdev {
	OneWire ow;
	uint8_t addr[8];
	uint8_t type;
};

/**
 * @brief Look for sensor presence
 */
static inline bool lookup_sensor(struct owdsdev *dev)
{
	dev->ow.reset_search();

	if (dev->ow.search(dev->addr) == false) {
		// printf_P(PSTR("OW DS SEARCH failed\n"));
		return false;
	}

	const uint8_t crc8 = OneWire::crc8(dev->addr, 7);
	if (crc8 != dev->addr[7]) {
		// printf_P(PSTR("OW DS CRC failed\n"));
		return false;
	}

	// print ROM
#if DEBUG_OW
	printf_P(PSTR("ROM ="));
	for (int i = 0; i < 8; i++) {
		printf_P(PSTR(" %hhx"), (uint8_t)dev->addr[i]);
	}
#endif /* DEBUG */

	// the first ROM byte indicates which chip
	switch (dev->addr[0]) {
	case 0x10:
#if DEBUG_OW
		printf_P(PSTR(": DS18S20"));  // or old DS1820
#endif
		dev->type = 1;
		break;
	case 0x28:
#if DEBUG_OW
		printf_P(PSTR(": DS18B20")); // current
#endif
		dev->type = 0;
		break;
	case 0x22:
#if DEBUG_OW
		printf_P(PSTR(": DS1822"));
#endif
		dev->type = 0;
		break;
	default:
#if DEBUG_OW
		printf_P(PSTR(": DS???"));
#endif
		return false;
	}

	printf_P(PSTR("\n"));

	return true;
}

static inline bool read_temperature(struct owdsdev *dev, int16_t *raw)
{
	if (dev->ow.reset() != 1U) {
		return false;
	}

	dev->ow.select(dev->addr);
	dev->ow.write(0x44, 1);        // start conversion, with parasite power on at the end

	k_sleep(K_MSEC(1000)); // maybe 750ms is enough, maybe not

	// we might do a ds.depower() here, but the reset will take care of it.

	if (dev->ow.reset() != 1U) {
		return false;
	}
	
	dev->ow.select(dev->addr);
	dev->ow.write(0xBE);         // Read Scratchpad

	// we need 9 bytes
	uint8_t data[9];
	byte dataor = 0x00U;
	for (int i = 0; i < 9; i++) {
		data[i] = dev->ow.read();
		dataor |= data[i];
	}

	/* if all bytes are null, then the read failed */
	if (dataor == 0x00U) {
		return false;
	}

	// Check CRC
	if (OneWire::crc8(data, 8) != data[8]) {
		return false;
	}

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t tmp = (data[1] << 8) | data[0];
	if (dev->type) {
		tmp = tmp << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
		  // "count remain" gives full 12 bit resolution
			tmp = (tmp & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00) tmp = tmp & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) tmp = tmp & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) tmp = tmp & ~1; // 11 bit res, 375 ms
		//// default is 12 bit resolution, 750 ms conversion time
	}

	*raw = tmp;

	return true;
}

float ow_ds_raw2float(int16_t raw)
{
	return (float)raw / 16.0;
}

int16_t ow_ds_raw_to_T16(int16_t raw)
{

	return (100LU * ((int32_t)raw)) / 16;
}

static struct owdsdev ds = {
	.ow = OneWire(9)
};

bool ll_ow_ds_init(void)
{
	return lookup_sensor(&ds);
}

bool ow_ds_read(int16_t *raw)
{
	return read_temperature(&ds, raw);
}