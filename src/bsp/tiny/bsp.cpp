#if defined(CONFIG_BOARD_TINY)

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <avrtos/misc/uart.h>
#include <avrtos/kernel.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/drivers/exti.h>

#include <caniot/datatype.h>

#include <mcp_can.h>
#include <Wire.h>

#include "tiny.h"

void bsp_init(void)
{	
	Wire.begin();
}

#endif /* CONFIG_BOARD_TINY */