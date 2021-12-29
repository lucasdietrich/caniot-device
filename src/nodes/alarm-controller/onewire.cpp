#include <OneWire.h>

#include <custompcb/board.h>

#include <avrtos/kernel.h>

static OneWire ds(9);

void thread(void *ctx);

K_THREAD_DEFINE(tm2, thread, 0x100, K_COOPERATIVE, NULL, '3');

void thread(void *ctx)
{
	for (;;) {
		byte i;
		byte present = 0;
		byte type_s;
		byte data[12];
		byte addr[8];
		float celsius;

		if (!ds.search(addr)) {
			printf_P(PSTR("No more addresses.\n\n"));
			ds.reset_search();
			// delay(250);
			k_sleep(K_MSEC(250));
			continue;
		}

		printf_P(PSTR("ROM ="));
		for (i = 0; i < 8; i++) {
			printf_P(PSTR(" %hhx"), (uint8_t)addr[i]);
		}

		if (OneWire::crc8(addr, 7) != addr[7]) {
			printf_P(PSTR("CRC is not valid!"));
			break;
		}
		printf_P(PSTR("\n"));

		// the first ROM byte indicates which chip
		switch (addr[0]) {
		case 0x10:
			printf_P(PSTR("  Chip = DS18S20"));  // or old DS1820
			type_s = 1;
			break;
		case 0x28:
			printf_P(PSTR("  Chip = DS18B20"));
			type_s = 0;
			break;
		case 0x22:
			printf_P(PSTR("  Chip = DS1822"));
			type_s = 0;
			break;
		default:
			printf_P(PSTR("Device is not a DS18x20 family device."));
			return;
		}

		ds.reset();
		ds.select(addr);
		ds.write(0x44, 1);        // start conversion, with parasite power on at the end

		k_sleep(K_MSEC(1000));
		// delay(1000);     // maybe 750ms is enough, maybe not
		// we might do a ds.depower() here, but the reset will take care of it.

		present = ds.reset();
		ds.select(addr);
		ds.write(0xBE);         // Read Scratchpad


		printf_P(PSTR("  Data = %hhx"), (uint8_t) present);

		for (i = 0; i < 9; i++) {           // we need 9 bytes
			data[i] = ds.read();
			printf_P(PSTR("%hhx "), (uint8_t) data[i]);
		}
		printf_P(PSTR(" CRC= %hhx\n"), (uint8_t) OneWire::crc8(data, 8));

		// Convert the data to actual temperature
		// because the result is a 16 bit signed integer, it should
		// be stored to an "int16_t" type, which is always 16 bits
		// even when compiled on a 32 bit processor.
		int16_t raw = (data[1] << 8) | data[0];
		if (type_s) {
			raw = raw << 3; // 9 bit resolution default
			if (data[7] == 0x10) {
			  // "count remain" gives full 12 bit resolution
				raw = (raw & 0xFFF0) + 12 - data[6];
			}
		} else {
			byte cfg = (data[4] & 0x60);
			// at lower res, the low bits are undefined, so let's zero them
			if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
			else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
			else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
			//// default is 12 bit resolution, 750 ms conversion time
		}
		celsius = (float)raw / 16.0;
		
		printf_P(PSTR("  Temperature = %.2f °C\n"), celsius);

		k_sleep(K_SECONDS(10));
	}

	__fault(K_FAULT);
}