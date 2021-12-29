#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void custompcb_hw_init(void);

void ll_relays_set(uint8_t state);
uint8_t ll_relays_read(void);

void ll_oc_set(uint8_t state);
uint8_t ll_oc_read(void);

uint8_t ll_inputs_read(void);

int16_t dev_tcn75_read(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_H_ */