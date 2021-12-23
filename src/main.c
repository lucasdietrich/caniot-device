#include <avrtos/kernel.h>

#include "init.h"

#include "can.h"


int main(void)
{
	dev_init();

	irq_enable();

	can_message m = {
		.buf = {0xAA, 0xBB},
		.len = 2,
		.id = 0xAA,
		.isext = 0,
		.rtr = 0,
	};

	for(;;) {
		printf_P(PSTR("can_send = %hhu\n"), can_send(&m));
		k_sleep(K_SECONDS(1));
	}
}