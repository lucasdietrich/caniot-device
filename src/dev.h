#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include "can.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct k_signal caniot_process_sig;

void print_indentification(void);

uint32_t get_magic_number(void);

static inline void caniot_trigger_event(void)
{
	k_signal_raise(&caniot_process_sig, 0);
}

int caniot_process(void);

void caniot_schedule_event(void);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */