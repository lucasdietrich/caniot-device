#include <avrtos/kernel.h>
#include <avrtos/debug.h>

#include "bsp/bsp.h"

#include <time.h>
#include "devices/tcn75.h"

#include "config.h"

#include "logging.h"
#define LOG_LEVEL LOG_LEVEL_DBG

#define K_MODULE K_MODULE_APPLICATION

K_KERNEL_LINK_INIT();

int main(void)
{
	/* Board Support Package */
	bsp_init();

	for (;;) {
		const int16_t temp = tcn75_read();
		LOG_DBG("temp: %d", temp);

		k_sleep(K_SECONDS(1));
	}
}
