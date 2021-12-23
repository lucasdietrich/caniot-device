#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include "can.h"

void print_indentification(void);

uint32_t get_magic_number(void);

int process_rx_frame(can_message *msg);

#endif /* _CANIOT_DEV_APPLICATION_H_ */