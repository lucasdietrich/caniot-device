#include "device.h"

#include <avr/pgmspace.h>

#include <avrtos/kernel.h>

#include <caniot.h>
#include <device.h>

#include "can.h"
#include "dev.h"

#include <time.h>

K_SIGNAL_DEFINE(caniot_process_sig);

const union deviceid did = {
	.cls = __DEVICE_CLS__,
	.sid = __DEVICE_SID__
};

static const struct caniot_identification identification PROGMEM =
{
	.did = {
		.cls = __DEVICE_CLS__,
		.sid = __DEVICE_SID__,
	},
	.version = __FIRMWARE_VERSION__,
	.name = __DEVICE_NAME__,
	.magic_number = __MAGIC_NUMBER__,
};

void entropy(uint8_t *buf, size_t len)
{
	static K_PRNG_DEFINE(prng, __MAGIC_NUMBER__, __MAGIC_NUMBER__ >> 1);

	k_prng_get_buffer(&prng, buf, len);
}

void get_time(uint32_t *sec, uint16_t *ms)
{
	if (sec == NULL) {
		return;
	}

	*sec = k_time_get();

	if (ms != NULL) {
		*ms = 0x0000U;
	}

#if DEBUG
	printf_P(PSTR("get_time: sec=%lu sec\n"), *sec);
#endif /* DEBUG */
}

static void caniot2msg(can_message *msg, const struct caniot_frame *frame)
{
	msg->ext = 0;
	msg->rtr = 0;
	msg->std = frame->id.raw,
	msg->len = frame->len;
	memcpy(msg->buf, frame->buf, frame->len);
}

static void msg2caniot(struct caniot_frame *frame, const can_message *msg)
{
	frame->id.raw = msg->std;
	frame->len = msg->len;
	memcpy(frame->buf, msg->buf, msg->len);
}

static int caniot_recv(struct caniot_frame *frame)
{
	int ret;
	can_message req;

	ret = can_recv(&req);
	if (ret == 0) {
		// can_print_msg(&req);
		msg2caniot(frame, &req);
		k_show_uptime();
		caniot_explain_frame(frame);
		printf_P(PSTR("\n"));
		
	} else if (ret == -EAGAIN) {
		ret = -CANIOT_EAGAIN;
	}

	return ret;
}

struct delayed_msg
{
	struct k_event ev;
	can_message msg;
};


K_MEM_SLAB_DEFINE(dmsg_slab, sizeof(struct delayed_msg), 8);

static void dmsg_handler(struct k_event *ev)
{
	struct delayed_msg *dmsg = CONTAINER_OF(ev, struct delayed_msg, ev);

	can_txq_message(&dmsg->msg);

	k_mem_slab_free(&dmsg_slab, dmsg);
}

static int caniot_send(const struct caniot_frame *frame, uint32_t delay_ms)
{
	int ret = -EINVAL;

	CANIOT_DBG(PSTR("send delay = %lu\n"), delay_ms);

	k_show_uptime();
	caniot_explain_frame(frame);
	printf_P(PSTR("\n"));

	if (delay_ms == 0) {
		can_message msg;

		caniot2msg(&msg, frame);

		ret = can_txq_message(&msg);
	} else if (delay_ms > 0) {
		struct delayed_msg *dmsg;
		
		ret = k_mem_slab_alloc(&dmsg_slab, (void **)&dmsg, K_NO_WAIT);
		if (ret == 0) {
			caniot2msg(&dmsg->msg, frame);
			k_event_init(&dmsg->ev, dmsg_handler);
			ret = k_event_schedule(&dmsg->ev, K_MSEC(delay_ms));
		}
	}

	return ret;
}

const struct caniot_drivers_api drivers = {
	.entropy = entropy,
	.get_time = get_time,
	.set_time = k_time_set,
	.recv = caniot_recv,
	.send = caniot_send,
};

extern struct caniot_config config;
extern const struct caniot_api api;

struct caniot_device device = {
	.identification = &identification,
	.config = &config,
	.api = &api,
	.driv = &drivers,
	.flags = {
		.request_telemetry = 0,
	},
};

void print_indentification(void)
{
	caniot_print_device_identification(&device);
}

uint32_t get_magic_number(void)
{
	return (uint32_t) pgm_read_dword(&device.identification->magic_number);
}

int caniot_process(void)
{
	return caniot_device_process(&device);
}

uint32_t get_timeout(void)
{
	return caniot_device_telemetry_remaining(&device);
}

bool telemetry_requested(void)
{
	return device.flags.request_telemetry == 1;
}

void request_telemetry(void)
{
	device.flags.request_telemetry = 1;

	trigger();
}