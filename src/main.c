#include <avrtos/kernel.h>
#include <avrtos/debug.h>

#include "bsp/bsp.h"

#include <time.h>
#include "devices/tcn75.h"

#include <stdlib.h>

#include "config.h"

#include "logging.h"
#define LOG_LEVEL LOG_LEVEL_DBG

#define K_MODULE K_MODULE_APPLICATION

K_KERNEL_LINK_INIT();

extern char _k_main_stack[THREAD_MAIN_STACK_SIZE];

int main(void)
{
	/* Board Support Package */
	bsp_init();
		// __malloc_heap_end = __malloc_heap_start;

	void *p = malloc(0x50);

	LOG_DBG("malloc(0x50) = %p", p);

	_k_main_stack[0] = 0x55;

	for (;;) {
		const int16_t temp = tcn75_read();
		LOG_DBG("temp: %d", temp);

		_delay_ms(1000);
	}
}
