#include "os.h"

#include <avrtos/kernel.h>

static __attribute__((naked, used, section(".init8"))) void os_init()
{
	k_avrtos_init();
}