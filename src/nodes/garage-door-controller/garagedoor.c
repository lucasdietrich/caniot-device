#include <caniot.h>
#include <device.h>
#include <datatype.h>

#include <avrtos/kernel.h>

#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

#include <dev.h>

#define RELAY_PULSE_DURATION_MS     500

static volatile bool inputs_changed = false;
static volatile struct board_dio dio;

ISR(PCINT2_vect)
{
	inputs_changed = true;
	dio.inputs = ll_inputs_read();
}

typedef enum {
	open = 1,
	close = 0,
} door_state_t;

struct door
{
	door_state_t state: 1; /* open/close */
	uint8_t inx: 2; /* input */
	uint8_t rx: 1; /* relay */
	uint8_t controllable: 1; /* can be controlled */
	struct k_event event; /* internal event for pulse */
};

/* print door struct */
// static void print_door(struct door *door)
// {
// 	printf_P(PSTR("door state %d, inx %d, rx %d, controllable %d\n"),
// 		 door->state, door->inx, door->rx, door->controllable);
// }

#define DOOR_INIT(in, r, cont) { .state = open, .inx = in, .rx = r, .controllable = cont }
#define GARAGE_DOOR_INIT(in, r) DOOR_INIT(in, r, 1)
#define GATE_DOOR_INIT(in) DOOR_INIT(in, 0, 0)

static void pulse_finished_handler(struct k_event *ev)
{
	const uint8_t relay = CONTAINER_OF(ev, struct door, event)->rx;

	ll_relays_set_mask(0, BIT(relay));
}

static union {
	struct door list[3];
	struct {
		struct door right;
		struct door left;
		struct door gate;
	};
} doors = {
	.right = GARAGE_DOOR_INIT(IN3, RL2),
	.left = GARAGE_DOOR_INIT(IN2, RL1),
	.gate = GATE_DOOR_INIT(IN1),
};


void device_init(void)
{
	k_event_init(&doors.left.event, pulse_finished_handler);
	k_event_init(&doors.right.event, pulse_finished_handler);

	/* initialize doors state */
	dio.inputs = ll_inputs_read();
	for (struct door *door = doors.list; door < doors.list +
	     ARRAY_SIZE(doors.list); door++) {
		door->state = dio.inputs & BIT(door->inx) ? open : close;
	}

	ll_inputs_enable_pcint(BIT(IN1) | BIT(IN2) | BIT(IN3));
}

static void actuate_door(struct door *door)
{
	// print_door(door);

	if (door->controllable) {
		ll_relays_set_mask(BIT(door->rx), BIT(door->rx));

		k_event_schedule(&door->event, K_MSEC(RELAY_PULSE_DURATION_MS));
	}
}

int command_handler(struct caniot_device *dev,
			   uint8_t ep, char *buf,
			   uint8_t len)
{
	if (AS_CRTHPT(buf)->r1) 
		actuate_door(&doors.left);
	
	if (AS_CRTHPT(buf)->r2)
		actuate_door(&doors.right);

	return 0;
}

void inputs_polling_loop(void *ctx);

K_THREAD_DEFINE(meast, inputs_polling_loop, 64, K_COOPERATIVE, NULL, 'M');

void inputs_polling_loop(void *ctx)
{
	for (;;) {
		/* atomic */
		if (inputs_changed == true) {
			// custompcb_print_io(dio);

			for (struct door *d = doors.list; d < doors.list +
			     ARRAY_SIZE(doors.list); d++) {

				const door_state_t prev = d->state;
				const door_state_t cur = dio.inputs & BIT(d->inx)
					? open : close;

				if (prev != cur) {
					d->state = cur;

					trigger_telemetry();
				}
			}

			/* atomic */
			inputs_changed = false;
		}

		k_sleep(K_MSEC(25));
	}
}

int telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	ARG_UNUSED(dev);

	AS_CRTHPT(buf)->c1 = doors.left.state;
	AS_CRTHPT(buf)->c2 = doors.right.state;
	AS_CRTHPT(buf)->c3 = doors.gate.state;
	int16_t temperature = dev_int_temperature();
	AS_CRTHPT(buf)->int_temperature = caniot_dt_T16_to_Temp(temperature);

	// print_T16(temperature);

	/* TODO required ? */
	*len = 8;

	return 0;
}

struct caniot_config config = CANIOT_CONFIG_DEFAULT_INIT();