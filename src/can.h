#ifndef _CANIOT_DEV_CAN_H_
#define _CANIOT_DEV_CAN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
        union {
                uint32_t std: 11;
                uint32_t ext: 29;
                uint32_t id;
        };
        struct {
                uint8_t buf[8];
                uint8_t len;
        };

        uint8_t isext: 1;
        uint8_t rtr: 1;
	
} __attribute__((packed)) can_message;

void can_init(void);

uint8_t can_recv(can_message *msg);

int can_txq_message(can_message *msg);

void can_print_msg(can_message *msg);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_CAN_H_ */