
#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <device.h>

#include "hw.h"
#include "os.h"
#include "dev.h"
#include "can.h"
#include "caniot_drv_api.h"


int main(void)
{
	hw_ll_init();
	usart_init();
	led_init();
	caniot_drv_api_init();
	can_init();

	k_thread_dump_all();
	print_indentification();

	irq_enable();

	static can_message msg;

	for (;;) {
		k_poll_signal(&can_sig_rx, K_FOREVER);
		can_sig_rx.flags = K_POLL_STATE_NOT_READY;

		while (can_recv(&msg) == 0) {
			can_print_msg(&msg);

			process_rx_frame(&msg);

			/* yield to allow tx thread to process
			 * loopback packet if any */
			k_yield();
		}
	}
}