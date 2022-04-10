#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <device.h>

#include "custompcb/board.h"
#include "custompcb/ext_temp.h"

#include <time.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "hw.h"
#include "dev.h"
#include "can.h"
#include "supervision.h"
#include "pulse.h"

#include "config.h"

#define K_MODULE K_MODULE_APPLICATION

__attribute__ ((weak)) void app_init(void) { }
__attribute__ ((weak)) void app_process(void) { }

/* 
 * Max interval between two app_process() calls (ms)
 * Note: Should be choiced carefully, because of the watchdog timer.
 */
const uint32_t max_process_interval = MIN(1000, WATCHDOG_TIMEOUT_MS / 2);

K_KERNEL_LINK_INIT();

int main(void)
{
	/* General low-level initialisation */
	hw_ll_init();
	usart_init();
	led_init();

#if CONFIG_WATCHDOG
	/* register the thread a critical, i.e. watchdog-protected thread */
	const uint8_t tid = critical_thread_register();

	/* Enable watchdog */
	wdt_enable(WATCHDOG_TIMEOUT_WDTO);
#endif 

	/* as we don't (always) use mutex/semaphore to synchronize threads 
	 * we need the initialization to not be preempted.
	 */
	__ASSERT_SCHED_LOCKED();

	/* Following initialization require interrupts to be enabled
	 * because they use Arduino millis()/micros() functions to calculate delays.
	 */	
	irq_enable();

	custompcb_hw_init();
	pulse_init();
	can_init();
	config_init();
	caniot_init();

	/* Specific application initialization */
	app_init();

	/* send telemetry on startup */
	trigger_telemetry();

	/* LOG */
	// k_thread_dump_all();
	print_indentification();

	uint32_t pulse_process_time = k_uptime_get_ms32();

	int ret;
	for (;;) {
		/* Estimate time to next event :
		 * - Thread alive (for watchdog timeout)
		 * - Caniot telemetry
		 * - Pulse event
		 */
		const uint32_t timeout_ms = MIN(
			max_process_interval, MIN(
				get_telemetry_timeout(),
				pulse_remaining()
			));
		// printf_P(PSTR("timeout_ms = %lu\n"), timeout_ms);
		
		/* set unready after processing,
		 * as some functions called may trigger the signal 
		 * in order to request telemetry
		 */
		if (telemetry_requested() == false) {
			K_SIGNAL_SET_UNREADY(&caniot_process_sig);
		}
		
		k_poll_signal(&caniot_process_sig, K_MSEC(timeout_ms));

#if CONFIG_WATCHDOG
		/* I'm alive ! */
		alive(tid);
#endif /* CONFIG_WATCHDOG */

		uint32_t now_ms = k_uptime_get_ms32();
		pulse_process(now_ms - pulse_process_time);
		pulse_process_time = now_ms;
			
		/* any processing related to the custom PCB */
		custompcb_hw_process();

		/* Application specific processing before CANIOT process*/
		app_process();

		do {
			ret = caniot_process();
			if (ret != 0 && ret != -CANIOT_EAGAIN) {
				// show error
				caniot_show_error(ret);
			}

#if CONFIG_WATCHDOG
			/* I'm alive ! */
			alive(tid);
#endif /* CONFIG_WATCHDOG */

			/* let CAN TX thread send pending CAN messages if any 
			 * before handling the next message
			 */
			k_yield();

		} while (ret != -CANIOT_EAGAIN);
	}
}