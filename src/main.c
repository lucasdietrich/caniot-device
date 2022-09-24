#include <avrtos/kernel.h>
#include <avrtos/debug.h>
#include <avrtos/misc/uart.h>
#include <avrtos/misc/led.h>

#include <caniot/device.h>

#include "bsp/bsp.h"
#include "devices/temp.h"

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

#include "logging.h"
#if defined(CONFIG_MAIN_LOG_LEVEL)
#	define LOG_LEVEL CONFIG_MAIN_LOG_LEVEL
#else
#	define LOG_LEVEL LOG_LEVEL_NONE
#endif

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

#if LOG_LEVEL >= LOG_LEVEL_DBG
	k_thread_dump_all();
	dump_stack_canaries();
#endif

	/* as we don't (always) use mutex/semaphore to synchronize threads 
	 * we need the initialization to not be preempted.
	 */
	__ASSERT_SCHED_LOCKED();

	/* Following initialization require interrupts to be enabled
	 * because they use Arduino millis()/micros() functions to calculate delays.
	 */	
	irq_enable();

	bsp_init();
	temp_init();
#if CONFIG_GPIO_PULSE_SUPPORT
	pulse_init();
#endif
	can_init();
	config_init();
	caniot_init();

#if CONFIG_WATCHDOG
	/* register the thread a critical, i.e. watchdog-protected thread */
	const uint8_t tid = critical_thread_register();

	/* Enable watchdog */
	wdt_enable(WATCHDOG_TIMEOUT_WDTO);
#endif 

	/* Specific application initialization */
	app_init();

	/* send telemetry on startup */
	trigger_telemetry();

	/* LOG */
	// k_thread_dump_all();
	print_indentification();

#if CONFIG_GPIO_PULSE_SUPPORT
	uint32_t pulse_process_time = k_uptime_get_ms32();
#endif

	int ret;
	for (;;) {
		/* Estimate time to next event :
		 * - Thread alive (for watchdog timeout)
		 * - Caniot telemetry
		 * - Pulse event
		 */
		uint32_t timeout_ms = MIN(
			max_process_interval,
			get_telemetry_timeout()
		);

#if CONFIG_GPIO_PULSE_SUPPORT
		timeout_ms = MIN(
			timeout_ms,
			pulse_remaining()
		);
#endif
		
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

#if CONFIG_GPIO_PULSE_SUPPORT
		uint32_t now_ms = k_uptime_get_ms32();
		pulse_process(now_ms - pulse_process_time);
		pulse_process_time = now_ms;
#endif

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
