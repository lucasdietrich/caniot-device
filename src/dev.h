#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include "can.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const union deviceid did;

extern struct k_signal caniot_process_sig;

void print_indentification(void);

uint32_t get_magic_number(void);

void request_telemetry(void);

static inline void trigger(void)
{
	k_signal_raise(&caniot_process_sig, 0);
}

int caniot_process(void);

uint32_t get_timeout(void);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */