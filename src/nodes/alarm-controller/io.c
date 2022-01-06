#include <custompcb/board.h>
#include <custompcb/tcn75.h>

#include <avrtos/kernel.h>

static void thread(void *ctx);

K_THREAD_DEFINE(tio, thread, 0x100, K_COOPERATIVE, NULL, '3');

static void thread(void *ctx)
{
	for(;;) {
		struct board_dio bs = ll_read();

		custompcb_print_io(bs);

		// bs.relays++;
		// bs.opencollectors++;

		ll_set(bs);

		k_sleep(K_SECONDS(10));
	}
}