#include <custompcb/board.h>
#include <custompcb/tcn75.h>

#include <avrtos/kernel.h>

void thread(void *ctx);

// K_THREAD_DEFINE(tm, thread, 0x100, K_COOPERATIVE, NULL, '2');

void thread(void *ctx)
{
	uint8_t counter = 0;

	for(;;) {
		const int16_t temp = dev_tcn75_read();

		irq_disable();
		printf_P(PSTR("Temperature : %.1f °C\n"), tcn75_int162float(temp));
		irq_enable();

		ll_relays_set((uint8_t)(counter << PORTC2));
		ll_oc_set((uint8_t) counter++);

		printf_P(PSTR("%lu\n"), k_uptime_get_ms32());

		k_sleep(K_SECONDS(60));
	}
}