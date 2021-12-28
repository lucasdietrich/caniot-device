
#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <device.h>

#include "hw.h"
#include "dev.h"
#include "can.h"

int main(void)
{
	hw_ll_init();
	usart_init();
	led_init();
	can_init();

	k_thread_dump_all();
	print_indentification();

	irq_enable();

	int ret;

	for (;;) {
		k_poll_signal(&caniot_process_sig, K_SECONDS(10));
		K_SIGNAL_SET_UNREADY(&caniot_process_sig);

		do {
			ret = caniot_process();

			// show error
			caniot_show_error(ret);

		} while (ret != -CANIOT_EAGAIN);
	}
}