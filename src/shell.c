#include <stdio.h>
#include <stdint.h>

#include <avrtos/drivers/usart.h>

#include "config.h"
#include "dev.h"
#include "bsp/bsp.h"

#include <logging.h>
#define LOG_LEVEL LOG_LEVEL_INF

void shell_process(void)
{
	int chr = usart0_drv_getc();
	if (chr >= 0) {
		LOG_INF("shell: %c", chr);
	}
}

#include "shell.h"