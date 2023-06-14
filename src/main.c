#include "bsp/bsp.h"
#include "can.h"
#include "config.h"
#include "dev.h"
#include "devices/temp.h"
#include "pulse.h"
#include "shell.h"
#include "watchdog.h"

#include <time.h>

#include <avrtos/avrtos.h>
#include <avrtos/debug.h>
#include <avrtos/logging.h>
#include <avrtos/misc/led.h>
#include <avrtos/misc/serial.h>

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <caniot/device.h>
#if defined(CONFIG_MAIN_LOG_LEVEL)
#define LOG_LEVEL CONFIG_MAIN_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define K_MODULE K_MODULE_APPLICATION

__attribute__((weak)) void app_init(void)
{
}
__attribute__((weak)) void app_process(void)
{
}

/*
 * Max interval between two app_process() calls (ms)
 * Note: Should be choiced carefully, because of the watchdog timer.
 */
const uint32_t max_process_interval = MIN(1000, WATCHDOG_TIMEOUT_MS / 2);

K_KERNEL_LINK_INIT();

int main(void)
{
	/* Interrupt should already be enabled */

	/* as we don't (always) use mutex/semaphore to synchronize threads
	 * we need the initialization to not be preempted.
	 */
	__ASSERT_SCHED_LOCKED();

	/* Board Support Package */
	bsp_init();

#if LOG_LEVEL >= LOG_LEVEL_DBG
	k_thread_dump_all();
	k_dump_stack_canaries();
#endif

	temp_start();

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

	/* send board level control telemetry on startup */
	trigger_telemetry(CANIOT_ENDPOINT_BOARD_CONTROL);

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
		uint32_t timeout_ms = MIN(max_process_interval, get_telemetry_timeout());

#if CONFIG_GPIO_PULSE_SUPPORT
		timeout_ms = MIN(timeout_ms, pulse_remaining());
#endif

		k_poll_signal(&caniot_process_sig, K_MSEC(timeout_ms));

		/* Clear the signal before application functions triggers it */
		if (!telemetry_requested()) {
			K_SIGNAL_SET_UNREADY(&caniot_process_sig);
		}

#if CONFIG_WATCHDOG
		/* I'm alive ! */
		alive(tid);
#endif /* CONFIG_WATCHDOG */

#if CONFIG_GPIO_PULSE_SUPPORT
		uint32_t now_ms = k_uptime_get_ms32();
		if (pulse_process(now_ms - pulse_process_time) == true) {
			trigger_telemetry(CANIOT_ENDPOINT_BOARD_CONTROL);
		}
		pulse_process_time = now_ms;
#endif

		/* Application specific processing before CANIOT process*/
		app_process();

#if CONFIG_USART_SHELL
		shell_process();
#endif

		do {
			ret = caniot_process();
			if (ret == 0) {
				/* When CAN message "sent" (actually queued to TX queue),
				 * immediately yield after having queued the CAN message
				 * so that it can be immediately sent.
				 */
				k_yield();

			} else if (ret != -CANIOT_EAGAIN) {
				// show error
				caniot_show_error(ret);

				k_sleep(K_MSEC(100u));
			}

#if CONFIG_WATCHDOG
			/* I'm alive ! */
			alive(tid);
#endif /* CONFIG_WATCHDOG */

		} while (ret != -CANIOT_EAGAIN);
	}
}
