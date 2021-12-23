#include "caniot_drv_api.h"

#include <avrtos/kernel.h>

#include "can.h"

#include "dev.h"

static struct k_prng prng;

void caniot_drv_api_init(void)
{
	uint32_t magic = get_magic_number();

	prng.lfsr31 = magic >> 1;
	prng.lfsr32 = magic;
}

static void entropy(uint8_t *buf, size_t len)
{
	/* TODO should be protected by mutex ? */
	k_prng_get_buffer(&prng, buf, len);
}

static void get_time(uint32_t *sec, uint16_t *usec)
{
	struct timespec ts;

	k_timespec_get(&ts);

	*sec = ts.tv_sec;
	*usec = ts.tv_msec / 1000;
}

struct delayed_frame
{
	union {
		struct k_event _ev;
		void *tie;
	};
	struct caniot_frame *frame;
};

struct delayed_msg
{
	struct k_event ev;
	can_message msg;
};

static void caniot2msg(can_message *msg, const struct caniot_frame *frame)
{
	msg->ext = 0;
	msg->rtr = 0;
	msg->std = frame->id.raw,
	msg->len = frame->len;
	memcpy(msg->buf, frame->buf, frame->len);
}

K_MEM_SLAB_DEFINE(dmsg_slab, sizeof(struct delayed_msg), 4);

void dmsg_handler(struct k_event *ev)
{
	struct delayed_msg *dmsg = CONTAINER_OF(ev, struct delayed_msg, ev);

	can_txq_message(&dmsg->msg);

	k_mem_slab_free(&dmsg_slab, dmsg);
}

static int caniot_send(const struct caniot_frame *frame, uint32_t delay_ms)
{
	printf_P(PSTR("caniot_send delay = %lu\n"), delay_ms);

	if (delay_ms == 0) {
		can_message msg;

		caniot2msg(&msg, frame);

		return can_txq_message(&msg);
	} else if (delay_ms > 0) {
		struct delayed_msg *dmsg;

		if (k_mem_slab_alloc(&dmsg_slab, (void **)&dmsg, K_NO_WAIT) == 0) {
			caniot2msg(&dmsg->msg, frame);
			k_event_init(&dmsg->ev, dmsg_handler);
			k_event_schedule(&dmsg->ev, K_MSEC(delay_ms));
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

const struct caniot_drivers_api drivers = {
	.entropy = entropy,
	.get_time = get_time,
	.recv = NULL,
	.send = caniot_send,
};