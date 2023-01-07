#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include "bsp/bsp.h"
#include "can.h"
#include "config.h"
#include "devices/temp.h"
#include "pulse.h"

#include <time.h>

#include <avrtos/kernel.h>

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <util/delay.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WATCHDOG_TIMEOUT_MS   8000
#define WATCHDOG_TIMEOUT_WDTO WDTO_8S

extern const caniot_did_t did;

extern struct k_signal caniot_process_sig;

void print_indentification(void);

uint32_t get_magic_number(void);

bool telemetry_requested(void);

void trigger_telemetry(void);

static inline struct k_thread *trigger_process(void)
{
	return k_signal_raise(&caniot_process_sig, 0);
}

int caniot_process(void);

uint32_t get_telemetry_timeout(void);

int config_on_read(struct caniot_device *dev, struct caniot_config *config);

int config_on_write(struct caniot_device *dev, struct caniot_config *config);

int config_restore_default(struct caniot_device *dev, struct caniot_config *cfg);

void config_init(void);

void caniot_init(void);

struct xps_context {
	pin_descr_t descr;

	uint8_t reset_state : 1;

#if CONFIG_GPIO_PULSE_SUPPORT
	struct pulse_event *pev;
#endif /* CONFIG_GPIO_PULSE_SUPPORT */
};

#define XPS_CONTEXT_INIT(_descr, _reset_state)                                           \
	{                                                                                \
		.descr = _descr, .reset_state = _reset_state,                            \
	}

int command_xps(struct xps_context *xpsc,
		caniot_complex_digital_cmd_t cmd,
		uint32_t duration_ms);

int dev_apply_blc_sys_command(struct caniot_device *dev,
			      struct caniot_blc_sys_command *sysc);

uint16_t get_t10_temperature(temp_sens_t sens);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */