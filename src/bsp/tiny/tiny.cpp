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

#include "devices/PCF8574.h"

NOINLINE void bsp_tiny_init(void)
{
	pcf8574_init();
}

#endif /* CONFIG_BOARD_TINY */