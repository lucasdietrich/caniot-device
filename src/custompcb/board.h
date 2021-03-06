#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_VALID_OUTPUT_PIN(pin) (pin <= RL2)

typedef enum
{
	GPIO_TYPE_OUTPUT = 0,
	GPIO_TYPE_INPUT,
} gpio_type_t;

typedef enum {
	OC1 = 0,
	OC2,
	RL1,
	RL2,
} output_t;

typedef enum {
	IN1 = 0,
	IN2 = 1,
	IN3 = 2,
	IN4 = 3
} input_t;

#define OPENCOLLECTOR1 0
#define OPENCOLLECTOR2 1
#define RELAY1 2
#define RELAY2 3

#define INPUT1 0
#define INPUT2 1
#define INPUT3 2
#define INPUT4 3

struct board_dio
{
union {
	struct {
		uint8_t outputs: 4;
		uint8_t inputs : 4;
	};
	struct {
		uint8_t rl1 : 1;
		uint8_t rl2 : 1;
		uint8_t oc1 : 1;
		uint8_t oc2 : 1;
		uint8_t in1 : 1;
		uint8_t in2 : 1;
		uint8_t in3 : 1;
		uint8_t in4 : 1;
	};
	uint8_t raw;
};
};

void custompcb_hw_init(void);
void custompcb_hw_process(void);

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

#endif /* _BOARD_H_ */