#ifndef _BSP_V1
#define _BSP_V1

#include "bsp/bsp.h"

#define OC1 BSP_GPIO_PC0
#define OC2 BSP_GPIO_PC1
#define RL1 BSP_GPIO_PC2
#define RL2 BSP_GPIO_PC3

#define IN1 BSP_GPIO_PB0
#define IN2 BSP_GPIO_PD4
#define IN3 BSP_GPIO_PD5
#define IN4 BSP_GPIO_PD6

#define IN1_PIN BSP_GPIO_PIN_GET(IN1)
#define IN2_PIN BSP_GPIO_PIN_GET(IN2)
#define IN3_PIN BSP_GPIO_PIN_GET(IN3)
#define IN4_PIN BSP_GPIO_PIN_GET(IN4)

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

void bsp_v1_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_V1 */