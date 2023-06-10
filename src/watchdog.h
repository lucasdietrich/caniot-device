#ifndef _CANIOT_DEV_WATCHDOG_H_
#define _CANIOT_DEV_WATCHDOG_H_

#include <avrtos/atomic.h>

uint8_t critical_thread_register(void);

void alive(uint8_t thread_id);

#endif /* _CANIOT_DEV_WATCHDOG_H_ */