#include <caniot.h>
#include <device.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

ISR(PCINT2_vect)
{
	usart_transmit('!');
}

static int telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	ARG_UNUSED(dev);

	if (ep == 0) {
		*((uint32_t *)buf) = k_uptime_get_ms32();
		*len = 4;
	} else {
		*len = 0;
	}

	return 0;
}

void device_init(void)
{
	ll_inputs_enable_pcint(BIT(IN1) | BIT(IN2) | BIT(IN3) | BIT(IN4));
}

void device_process(void)
{
	printf_P(PSTR("device_process()\n"));	
}

struct caniot_config config = CANIOT_CONFIG_DEFAULT_INIT();

const struct caniot_api api = CANIOT_API_MIN_INIT(NULL, telemetry_handler);