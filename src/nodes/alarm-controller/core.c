#include <caniot.h>
#include <device.h>
#include <datatype.h>


#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "core.h"

#include "custompcb/board.h"
#include "custompcb/ext_temp.h"

#include "dev.h"

#include "alarm.h"

struct caniot_CRTHPT_ALARM {
	union {
		struct {
			uint8_t light1 : 1;
			uint8_t light2 : 1;
			alarm_state_t alarm_state : 2;
			uint8_t alarm_mode : 1;
			uint8_t siren : 1;
			uint8_t alarm_inputs_status: 1;
			uint8_t _unused1: 1;
			uint8_t _unused: 8;
		} status;
		struct {
			uint8_t _unused : 6;
			caniot_twostate_cmd_t siren : 2;

			caniot_light_cmd_t light1 : 2;
			caniot_light_cmd_t light2 : 2;
			alarm_cmd_t alarm : 2;
			alarm_mode_cmd_t alarm_mode : 2;
		} commands;
	};

	struct {
		uint16_t int_temperature : 10;
		uint16_t humidity : 10;
		uint16_t pressure : 10;
		uint16_t ext_temperature : 10;
	} measurements;
};

#define INTERPRET_CMD(buf) \
	AS(buf, caniot_CRTHPT_ALARM)

int app_telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	const uint8_t lights = ll_outputs_read() & (BIT(OC1) | BIT(OC2));
	INTERPRET_CMD(buf)->status.light1 = (lights >> OUTDOOR_LIGHT_1) & 1;
	INTERPRET_CMD(buf)->status.light2 = (lights >> OUTDOOR_LIGHT_2) & 1;
	INTERPRET_CMD(buf)->status.alarm_state = alarm_get_state();
	INTERPRET_CMD(buf)->status.alarm_mode = alarm_get_mode();
	INTERPRET_CMD(buf)->status.siren = alarm_get_siren_state();
	INTERPRET_CMD(buf)->status.alarm_inputs_status = alarm_inputs_status() ? 1 : 0;

	const int16_t temperature = dev_int_temperature();
	INTERPRET_CMD(buf)->measurements.int_temperature = caniot_dt_T16_to_Temp(temperature);

#if DEBUG_TCN75
	print_T16(temperature);
#endif /* DEBUG */

#if CONFIG_APP_OW_EXTTEMP
	int16_t temp;
	const bool ow_status = ow_ext_get(&temp);
	INTERPRET_CMD(buf)->measurements.ext_temperature = ow_status ?
		caniot_dt_T16_to_Temp(temp) : CANIOT_DT_T10_INVALID;
#else
	INTERPRET_CMD(buf)->measurements.ext_temperature = CANIOT_DT_T10_INVALID;
#endif 

	*len = 8;

	return 0;
}

static enum ctrl_source lights_control_source[3] = {SRC_USER, SRC_USER, SRC_USER};

void commands_lights_from(uint8_t oc, caniot_light_cmd_t cmd, enum ctrl_source src)
{
	/* CANIOT_LIGHT_CMD_NONE command has no effect so is ignored */
	if ((oc > OC2) || (cmd == CANIOT_LIGHT_CMD_NONE)) {
		return;
	}

	/* alarm don't have the priority over user */
	if ((src == SRC_ALARM_CONTROLLER) && (cmd == CANIOT_LIGHT_CMD_OFF)
	    && (lights_control_source[oc] == SRC_USER)) {
		    printf_P(PSTR("alarm don't have the priority over user\n"));
		return;
	}

	lights_control_source[oc] = src;

	
	command_output(oc, (caniot_complex_digital_cmd_t) cmd);
}

static void command_alarm(alarm_cmd_t cmd)
{
	switch (cmd) {
	case ALARM_CMD_ENABLE:
		alarm_enable();
		break;
	case ALARM_CMD_DISABLE:
		alarm_disable();
		break;
	case ALARM_CMD_RECOVER:
		alarm_recover();
		break;
	default:
		break;
	}
}

static void command_alarm_mode(alarm_mode_cmd_t mode_cmd)
{
	switch (mode_cmd) {
	case ALARM_MODE_CMD_NORMAL:
		alarm_set_mode(ALARM_MODE_NORMAL);
		break;
	case ALARM_MODE_CMD_SILENT:
		alarm_set_mode(ALARM_MODE_SILENT);
		break;
	default:
		break;
	}
}

static void command_siren(caniot_twostate_cmd_t cmd)
{
	switch (cmd) {
	case CANIOT_TS_CMD_ON:
		alarm_test_siren_start();
		break;
	case CANIOT_TS_CMD_OFF:
		alarm_test_siren_stop();
		break;
	case CANIOT_TS_CMD_TOGGLE:
		alarm_test_siren_toggle();
		break;
	default:
		break;
	}
}

int app_command_handler(struct caniot_device *dev,
				uint8_t ep,
				char *buf,
				uint8_t len)
{
	ARG_UNUSED(dev);

	commands_lights_from(OUTDOOR_LIGHT_1, INTERPRET_CMD(buf)->commands.light1, SRC_USER);
	commands_lights_from(OUTDOOR_LIGHT_2, INTERPRET_CMD(buf)->commands.light2, SRC_USER);
	command_alarm(INTERPRET_CMD(buf)->commands.alarm);
	command_alarm_mode(INTERPRET_CMD(buf)->commands.alarm_mode);
	command_siren(INTERPRET_CMD(buf)->commands.siren);

	/* yield to allow alarm thread to change state before returning telemetry */
	k_yield();

	return 0;
}

struct caniot_config config = {
	.telemetry = {
		.period = CANIOT_TELEMETRY_PERIOD_DEFAULT,
		.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT,
		.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT,
	},
	.flags = {
		.error_response = 1u,
		.telemetry_delay_rdm = 1u,
		.telemetry_endpoint = endpoint_app
	},
	.timezone = CANIOT_TIMEZONE_DEFAULT,
	.location = {
		.region = CANIOT_LOCATION_REGION_DEFAULT,
		.country = CANIOT_LOCATION_COUNTRY_DEFAULT,
	}, 
};