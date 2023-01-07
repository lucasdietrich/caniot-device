#if CONFIG_USART_SHELL

#include "bsp/bsp.h"
#include "config.h"
#include "dev.h"
#include "shell.h"

#include <stdint.h>
#include <stdio.h>

#include <avrtos/drivers/usart.h>
#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_INF

void shell_process(void)
{
	int chr = usart0_getc();

	if (chr >= 0) {
		LOG_INF("shell: %c", chr);

		switch ((uint8_t)chr) {
#if CONFIG_WATCHDOG
		case 'W':
			/* Watchdog reset test */
			irq_disable();
			for (;;) {
			}
			break;
#endif
		default:
			break;
		}
	}
}

#endif