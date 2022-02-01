#ifndef _ALARM_CONTROLLER_ALARM_H_
#define _ALARM_CONTROLLER_ALARM_H_

#include <stdint.h>
#include <stdbool.h>
#include <avrtos/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	ALARM_MODE_NORMAL = 0,
	ALARM_MODE_SILENT = 1,
} alarm_mode_t;

typedef enum
{
	ALARM_MODE_CMD_NONE = 0,
	ALARM_MODE_CMD_NORMAL = 1,
	ALARM_MODE_CMD_SILENT = 2,
} alarm_mode_cmd_t;

#define ALARM_TESTING_MODE ALARM_MODE_SILENT

typedef enum {
	ALARM_CMD_NONE = 0,
	ALARM_CMD_ENABLE = 1,
	ALARM_CMD_DISABLE = 2,
	ALARM_CMD_RECOVER = 3,
} alarm_cmd_t;

typedef enum 
{
	inactive = 0,
	observing,
	sounding,
	recovering
} alarm_state_t;

void alarm_init(void);

void alarm_enable(void);

void alarm_disable(void);

void alarm_recover(void);

alarm_state_t alarm_get_state(void);

void alarm_set_mode(alarm_mode_t mode);

alarm_mode_t alarm_get_mode(void);

void alarm_test_siren_start(void);

void alarm_test_siren_stop(void);

void alarm_test_siren_toggle(void);

bool alarm_get_siren_state(void);

bool alarm_inputs_status(void);

#ifdef __cplusplus
}
#endif

#endif /* _ALARM_CONTROLLER_ALARM_H_ */