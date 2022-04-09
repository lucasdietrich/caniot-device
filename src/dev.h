#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include <time.h>

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include <avrtos/kernel.h>

#include <caniot.h>
#include <device.h>
#include <datatype.h>

#include "custompcb/board.h"
#include "custompcb/ext_temp.h"

#include "can.h"
#include "config.h"
#include "pulse.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WATCHDOG_TIMEOUT_MS 	8000
#define WATCHDOG_TIMEOUT_WDTO 	WDTO_8S

extern const union deviceid did;

extern struct k_signal caniot_process_sig;

void print_indentification(void);

uint32_t get_magic_number(void);

bool telemetry_requested(void);

void trigger_telemetry(void);

static inline void trigger_process(void)
{
	k_signal_raise(&caniot_process_sig, 0);
}

int caniot_process(void);

uint32_t get_telemetry_timeout(void);
						  
int config_on_read(struct caniot_device *dev,
		   struct caniot_config *config);

int config_on_write(struct caniot_device *dev,
		   struct caniot_config *config);

void config_init(void);

void caniot_init(void);

void command_output(output_t pin,
		    caniot_complex_digital_cmd_t cmd);
		    
#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */