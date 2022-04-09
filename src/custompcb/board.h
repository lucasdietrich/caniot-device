#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RL1	0
#define RL2	1
#define IN1	0
#define IN2	1
#define IN3	2
#define IN4	3
#define OC1	0
#define OC2	1

#define RELAY1 RL1
#define RELAY2 RL2
#define INPUT1 IN1
#define INPUT2 IN2
#define INPUT3 IN3
#define INPUT4 IN4
#define OPENCOLLECTOR1 OC1
#define OPENCOLLECTOR2 OC2

	struct board_dio
	{
		union {
			struct {
				uint8_t relays : 2;
				uint8_t inputs : 4;
				uint8_t opencollectors : 2;
			};
			struct {
				uint8_t r1 : 1;
				uint8_t r2 : 1;
				uint8_t in1 : 1;
				uint8_t in2 : 1;
				uint8_t in3 : 1;
				uint8_t in4 : 1;
				uint8_t oc1 : 1;
				uint8_t oc2 : 1;
			};
			uint8_t raw;
		};
	};

void custompcb_hw_init(void);

void custompcb_hw_process(void);

void ll_relays_set(uint8_t state);
void ll_relays_set_mask(uint8_t state, uint8_t mask);
uint8_t ll_relays_read(void);
void ll_relays_toggle_mask(uint8_t mask);

void ll_oc_set(uint8_t state);
void ll_oc_set_mask(uint8_t state, uint8_t mask);
void ll_oc_toggle_mask(uint8_t mask);
uint8_t ll_oc_read(void);

uint8_t ll_inputs_read(void);
void ll_inputs_enable_pcint(uint8_t mask);

struct board_dio ll_read(void);
void ll_set(struct board_dio state);
void ll_set_mask(struct board_dio state, struct board_dio mask);

int16_t tcn75_read(void);

static inline int16_t dev_int_temperature(void)
{
	return tcn75_read();
}

void print_T16(int16_t temp);

void custompcb_print_io(struct board_dio io);

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_H_ */