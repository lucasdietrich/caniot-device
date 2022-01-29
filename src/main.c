#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <device.h>

#include "custompcb/board.h"

#include <time.h>
#include <avr/io.h>


#include "hw.h"
#include "dev.h"
#include "can.h"

#define K_MODULE K_MODULE_APPLICATION

__attribute__ ((weak)) void device_init(void) { }
__attribute__ ((weak)) void device_process(void) { }

/* Max interval between two device_process() calls (ms) */
const uint32_t max_process_interval = 10000;

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
	caniot_init();
	
	/* Specific application initialization */
	device_init();

	/* send telemetry on startup */
	request_telemetry();

	/* LOG */
	// k_thread_dump_all();
	print_indentification();

	int ret;

	for (;;) {
		/* Estimate time to next periodic telemetry event.
		 * - Timeout is majorated by the maximum interval between two device_process() calls.
		 */
		const uint32_t timeout_ms = MIN(get_timeout(), max_process_interval);
		
		/* set unready after processing,
		 * as some functions called may trigger the signal 
		 * in order to request telemetry
		 */
		if (telemetry_requested() == false) {
			K_SIGNAL_SET_UNREADY(&caniot_process_sig);
		}
		
		k_poll_signal(&caniot_process_sig, K_MSEC(timeout_ms));

		device_process();

		do {
			ret = caniot_process();

			if (ret != 0 &&ret != -CANIOT_EAGAIN) {
				// show error
				caniot_show_error(ret);
			}

			/* let CAN TX thread to send pending CAN messages if any */
			k_yield();

		} while (ret != -CANIOT_EAGAIN);
	}
}