/**
 * @file measurement.c
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Measurement loop
 * @version 0.1
 * @date 2022-01-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <avrtos/kernel.h>

// #include <OneWire.h>

#include <custompcb/board.h>
#include <custompcb/owds.h>

#define OW_EXT_TMP_RETRY_PERIOD 5000LU
#define OW_EXT_TMP_MEASURE_PERIOD 5000LU

void measurement_loop(void *ctx);

K_THREAD_DEFINE(tmeasurement, measurement_loop, 0x80, K_COOPERATIVE, NULL, '?');

volatile int16_t ow_ext_tmp = 0;

void measurement_loop(void *ctx)
{
	int16_t raw;

	while (ll_ow_ds_init() == false) {
		printf_P(PSTR("OW DS init failed\n"));
		
		k_sleep(K_MSEC(OW_EXT_TMP_RETRY_PERIOD));
	}

	for (;;) {
		if (ow_ds_read(&raw)) {
			ow_ext_tmp = ow_ds_raw_to_T16(raw);
#if DEBUG
			printf_P(PSTR("DS18B20: Temperature : %.2f °C\n"), 
				 ow_ds_raw2float(raw));
#endif /* DEBUG */
		} else {
			printf_P(PSTR("OW DS read failed\n"));
		}

		k_sleep(K_MSEC(OW_EXT_TMP_MEASURE_PERIOD));
	}
}