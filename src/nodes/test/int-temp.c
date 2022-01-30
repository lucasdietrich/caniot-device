#include <custompcb/board.h>
#include <custompcb/tcn75.h>

#include <avrtos/kernel.h>

static void thread(void *ctx);

K_THREAD_DEFINE(tint, thread, 0x100, K_COOPERATIVE, NULL, '2');

void thread(void *ctx)
{
	for(;;) {
		const int16_t temp = dev_tcn75_read();

		printf_P(PSTR("TCN75: Temperature : %.1f °C\n"), tcn75_int16tofloat(temp));

		k_sleep(K_SECONDS(1));
	}
}