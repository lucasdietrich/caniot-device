#ifndef _ALARM_CONTROLLER_ALARM_H_
#define _ALARM_CONTROLLER_ALARM_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ALARM_CMD_NONE = 0,
	ALARM_CMD_ENABLE = 1,
	ALARM_CMD_DISABLE = 2,
	ALARM_CMD_RESET = 3,
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

void alarm_disabled(void);

void alarm_recovery_reset(void);

alarm_state_t alarm_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* _ALARM_CONTROLLER_ALARM_H_ */