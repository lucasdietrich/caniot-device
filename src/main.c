#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <device.h>

#include "hw.h"
#include "dev.h"
#include "can.h"

extern void device_init(void);

int main(void)
{
	hw_ll_init();
	usart_init();
	led_init();
	can_init();

	device_init();

	k_thread_dump_all();
	print_indentification();

	irq_enable();

	int ret;

	for (;;) {
		/* estimate time to next periodic telemetry event */
		const uint32_t timeout = get_timeout();

		k_poll_signal(&caniot_process_sig, K_MSEC(timeout));
		K_SIGNAL_SET_UNREADY(&caniot_process_sig);

		do {
			ret = caniot_process();

			if (ret != -CANIOT_EAGAIN) {
				// show error
				caniot_show_error(ret);
			}

		} while (ret != -CANIOT_EAGAIN);
	}
}