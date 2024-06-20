/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp/bsp.h"
#include "can.h"
#include "config.h"
#include "dev.h"
#include "devices/gpio_pulse.h"
#include "devices/temp.h"
#include "diag.h"
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

#define LOG_LEVEL CONFIG_MAIN_LOG_LEVEL
#define K_MODULE  K_MODULE_APPLICATION

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
const uint32_t max_process_interval =
    MIN(CONFIG_APP_MAX_PROCESS_INTERVAL_MS, WATCHDOG_TIMEOUT_MS / 2);

K_KERNEL_LINK_INIT();

int main(void)
{
    /* Main thread watchdog id */
    uint8_t tid = 0u;

    /* Interrupt should already be enabled */

    /* as we don't (always) use mutex/semaphore to synchronize threads
     * we need the initialization to not be preempted.
     */
    __ASSERT_SCHED_LOCKED();

    bsp_init();

#if LOG_LEVEL >= LOG_LEVEL_DBG
    k_thread_dump_all();
    k_dump_stack_canaries();
#endif

#if CONFIG_DIAG
    diag_init();
#endif

    temp_start();

#if CONFIG_GPIO_PULSE_SUPPORT
    pulse_init();
#endif
    can_init();

    dev_init();

#if CONFIG_SHELL
    shell_init();
#endif

#if CONFIG_WATCHDOG
    /* register the thread a critical, i.e. watchdog-protected thread */
    tid = critical_thread_register();

    /* Enable watchdog */
    wdt_enable(WATCHDOG_TIMEOUT_WDTO);
#endif

    /* Specific application initialization */
    app_init();

    /* send board level control telemetry on startup */
    dev_trigger_telemetry(CANIOT_ENDPOINT_BOARD_CONTROL);

    /* LOG */
    dev_print_indentification();

    for (;;) {
        /* Estimate time to next event :
         * - Thread alive (for watchdog timeout)
         * - Caniot telemetry
         * - Pulse event
         */
        uint32_t timeout_ms = MIN(max_process_interval, dev_get_process_timeout());

#if CONFIG_GPIO_PULSE_SUPPORT
        timeout_ms = MIN(timeout_ms, pulse_remaining());
#endif

        k_poll_signal(&dev_process_sig, K_MSEC(timeout_ms));

        /* Clear the signal before application functions triggers it */
        if (!dev_telemetry_is_requested()) {
            K_SIGNAL_SET_UNREADY(&dev_process_sig);
        }

#if CONFIG_WATCHDOG
        /* I'm alive ! */
        alive(tid);
#endif /* CONFIG_WATCHDOG */

#if CONFIG_GPIO_PULSE_SUPPORT
        const uint32_t now = k_uptime_get_ms32();
        if (pulse_process(now) == true) {
            dev_trigger_telemetry(CANIOT_ENDPOINT_BOARD_CONTROL);
        }
#endif

        /* Application specific processing before CANIOT process*/
        app_process();

#if CONFIG_SHELL && !CONFIG_SHELL_WORKQ_OFFLOADED
        shell_process();
#endif

        dev_process(tid);

#if CONFIG_DIAG && CONFIG_DIAG_RESET_CONTEXT_RUNTIME
        diag_reset_context_update(k_uptime_get());
#endif // CONFIG_DIAG_RESET_CONTEXT_RUNTIME
    }
}
