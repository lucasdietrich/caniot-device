#ifndef _BSP_V1
#define _BSP_V1

#include "bsp/bsp.h"

#define GPIO_VALID_OUTPUT_PIN(pin) (pin <= RL2)

struct board_dio
{
	union {
		uint8_t outputs : 4;
		uint8_t inputs : 4;
		uint8_t rl1 : 1;
		uint8_t rl2 : 1;
		uint8_t oc1 : 1;
		uint8_t oc2 : 1;
		uint8_t in1 : 1;
		uint8_t in2 : 1;
		uint8_t in3 : 1;
		uint8_t in4 : 1;
		uint8_t raw;
	};
};

#ifdef __cplusplus
extern "C" {
#endif

void ll_outputs_init(void);

void ll_outputs_set(uint8_t state);
void ll_outputs_reset(uint8_t state);
void ll_outputs_set_mask(uint8_t state, uint8_t mask);
uint8_t ll_outputs_read(void);
void ll_outputs_toggle_mask(uint8_t mask);

uint8_t ll_inputs_read(void);
void ll_inputs_enable_pcint(uint8_t mask);

struct board_dio ll_read(void);

void custompcb_print_io(struct board_dio io);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_V1 */