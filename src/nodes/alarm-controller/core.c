#include <caniot.h>
#include <device.h>
#include <datatype.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"
#include "alarm.h"

#define OUTDOOR_LIGHT_1 	OC1
#define OUTDOOR_LIGHT_2 	OC2

ISR(PCINT2_vect)
{
	usart_transmit('!');
}

struct caniot_CRTHPT_ALARM {
	struct {
		uint8_t light1: 1;
		uint8_t light2: 1;
		alarm_state_t alarm_state : 2;
	} status;
	struct {
		caniot_light_cmd_t light1 : 2;
		caniot_light_cmd_t light2 : 2;
		caniot_light_cmd_t alarm : 2;
	} commands;
	struct {
		uint16_t int_temperature : 10;
		uint16_t humidity : 10;
		uint16_t pressure : 10;
		uint16_t ext_temperature : 10;
	} measurements;
};

#define INTERPRET_CMD(buf) \
	AS(buf, caniot_CRTHPT_ALARM)

static int telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	const uint8_t lights = ll_oc_read();

	INTERPRET_CMD(buf)->status.light1 = (lights >> OUTDOOR_LIGHT_1) & 1;
	INTERPRET_CMD(buf)->status.light2 = (lights >> OUTDOOR_LIGHT_2) & 1;
	INTERPRET_CMD(buf)->status.alarm_state = alarm_get_state();

	const int16_t temperature = dev_int_temperature();
	AS_CRTHPT(buf)->int_temperature = caniot_dt_T16_to_Temp(temperature);

	*len = 8;

	return 0;
}

static void command_light(uint8_t oc, caniot_light_cmd_t cmd)
{
	const uint8_t mask = BIT(oc);
	switch (cmd)
	{
		case CANIOT_LIGHT_CMD_ON:
			ll_oc_set_mask(mask, mask);
			break;
		case CANIOT_LIGHT_CMD_OFF:
			ll_oc_set_mask(0, mask);
			break;
		case CANIOT_LIGHT_CMD_TOGGLE:
			ll_oc_toggle_mask(mask);
			break;
		default:
			break;
	}
}

static void command_alarm(alarm_cmd_t cmd)
{
	switch (cmd)
	{
		case ALARM_CMD_ENABLE:
			alarm_enable();
			break;
		case ALARM_CMD_DISABLE:
			alarm_disabled();
			break;
		case ALARM_CMD_RESET:
			alarm_recovery_reset();
			break;
		default:
			break;
	}
}

static int command_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t len)
{
	ARG_UNUSED(dev);

	command_light(OUTDOOR_LIGHT_1, INTERPRET_CMD(buf)->commands.light1);
	command_light(OUTDOOR_LIGHT_2, INTERPRET_CMD(buf)->commands.light2);
	command_alarm(INTERPRET_CMD(buf)->commands.alarm);

	return 0;
}

void device_init(void)
{
	// ll_inputs_enable_pcint(BIT(IN0) | BIT(IN1) | BIT(IN2) | BIT(IN3));
}

void device_process(void)
{
	
}

struct caniot_config config = CANIOT_CONFIG_DEFAULT_INIT();

const struct caniot_api api = CANIOT_API_MIN_INIT(command_handler, telemetry_handler);