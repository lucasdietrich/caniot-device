#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <device.h>

#include "custompcb/board.h"

#include "hw.h"
#include "dev.h"
#include "can.h"

#define K_MODULE K_MODULE_APPLICATION

extern void device_init(void);

int main(void)
{
	/* General initialisation */
	hw_ll_init();
	usart_init();
	led_init();

	/* Following initialization require interrupts to be enabled
	 * because they use Arduino millis()/micros() functions to calculate delays.
	 */	
	irq_enable();

	/* as we don't use mutex/semaphore to synchronize threads 
	 * we need the initialization to not be preemptive.
	 */
	__ASSERT_SCHED_LOCKED();

	custompcb_hw_init();
	can_init();
	
	/* Specific application initialization */
	device_init();

	/* LOG */
	k_thread_dump_all();
	print_indentification();

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