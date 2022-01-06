#include <OneWire.h>

#include <custompcb/board.h>
#include <custompcb/owds.h>

#include <avrtos/kernel.h>

static void thread(void *ctx);

K_THREAD_DEFINE(text, thread, 0x60, K_COOPERATIVE, NULL, '3');

static void thread(void *ctx)
{
	int16_t raw;

	if (ll_ow_ds_init() == false) {
		printf_P(PSTR("OW DS init failed\n"));
		__fault(K_FAULT);
	}

	for (;;) {
		if (ow_ds_read(&raw)) {
			float temperature = ow_ds_raw2float(raw);

			printf_P(PSTR("DS18B20: Temperature : %.2f °C\n"), temperature);
		} else {
			printf_P(PSTR("OW DS read failed\n"));
		}

		k_sleep(K_SECONDS(1));
	}
}