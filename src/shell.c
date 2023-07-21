#if CONFIG_SHELL

#include "bsp/bsp.h"
#include "config.h"
#include "dev.h"
#include "shell.h"

#include <stdint.h>
#include <stdio.h>

#include <avrtos/debug.h>
#include <avrtos/drivers/usart.h>
#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_INF

#define SHELL_USART	    BSP_USART
#define SHELL_USART_RX_vect BSP_USART_RX_vect

#if CONFIG_SHELL_WORKQ_OFFLOADED
static void shell_handler(struct k_work *work);

K_SEM_DEFINE(shell_sem, 1u, 1u);
K_WORK_DEFINE(shell_work, shell_handler);

static void shell_handler(struct k_work *work)
{
	(void)work;

	shell_process();
}
#endif

K_MSGQ_DEFINE(shell_msgq, 1u, 16u);

ISR(SHELL_USART_RX_vect)
{
	char chr = SHELL_USART->UDRn;
	int ret	 = k_msgq_put(&shell_msgq, &chr, K_NO_WAIT);

#if CONFIG_SHELL_WORKQ_OFFLOADED
	if ((ret == 0) && (k_sem_take(&shell_sem, K_NO_WAIT) == 0)) {
		k_system_workqueue_submit(&shell_work);
	}
#endif
}

void shell_init(void)
{
	ll_usart_enable_rx_isr(BSP_USART);
}

void shell_process(void)
{
	char chr;

	while (k_msgq_get(&shell_msgq, &chr, K_NO_WAIT) == 0) {
		LOG_INF("shell: %c", chr);

		switch ((uint8_t)chr) {
#if CONFIG_WATCHDOG
		case 'W':
		case 'w':
			/* Watchdog reset test */
			irq_disable();
			for (;;) {
			}
			break;
#endif
		case 't':
		case 'T':
#if CONFIG_SHELL_WORKQ_OFFLOADED
			k_sched_lock();
#endif
			trigger_telemetry(CANIOT_ENDPOINT_BOARD_CONTROL);
#if CONFIG_SHELL_WORKQ_OFFLOADED
			k_sched_unlock();
#endif
			break;
		case 'U':
		case 'u':
			k_show_uptime();
			k_show_ticks();
			printf_P(PSTR("\n"));
			break;
		case 'C':
		case 'c':
			k_dump_stack_canaries();
			break;
		case 'K':
		case 'k':
			k_thread_dump_all();
			break;
		default:
			break;
		}

#if CONFIG_SHELL_WORKQ_OFFLOADED
		k_sem_give(&shell_sem);
#endif

		k_yield();
	}
}

#endif