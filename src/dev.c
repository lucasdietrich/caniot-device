#include "device.h"

#include <avr/pgmspace.h>

#include <avrtos/kernel.h>

#include <caniot.h>
#include <device.h>
#include <class.h>

#include "can.h"
#include "dev.h"

K_SIGNAL_DEFINE(caniot_process_sig);

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

static struct caniot_config config = {
	.telemetry = {
		.period = CANIOT_TELEMETRY_PERIOD_DEFAULT,
		// .delay = CANIOT_TELEMETRY_DELAY_DEFAULT,
		.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT,
		.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT,
	},
	.flags = {
		.error_response = 1u,
		.telemetry_delay_rdm = 1u,
		.telemetry_endpoint = CANIOT_TELEMETRY_ENDPOINT_DEFAULT
	}
};

static int telemetry_handler(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	ARG_UNUSED(dev);

	if (ep == 0) {
		*((uint32_t *)buf) = k_uptime_get_ms32();
		*len = 4;
	} else {
		*len = 0;
	}

	return 0;
}


const struct caniot_api dev_api = {
	.update_time = NULL,
	.config = {
		.on_read = NULL,
		.written = NULL,
	},
	.custom_attr = {
		.read = NULL,
		.write = NULL,
	},
	.command_handler = NULL,
	.telemetry = telemetry_handler
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

	struct timespec ts;

	k_timespec_get(&ts);

	*sec = ts.tv_sec;

	if (ms != NULL) {
		*ms = ts.tv_msec;
	}
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
	can_message req;
	if (can_recv(&req) != 0) {
		return -1;
	}

	can_print_msg(&req);
	msg2caniot(frame, &req);

	return 0;
}

struct delayed_msg
{
	struct k_event ev;
	can_message msg;
};


K_MEM_SLAB_DEFINE(dmsg_slab, sizeof(struct delayed_msg), 4);

static void dmsg_handler(struct k_event *ev)
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

struct caniot_drivers_api drivers = {
	.entropy = entropy,
	.get_time = get_time,
	.recv = caniot_recv,
	.send = caniot_send,
};

struct caniot_device device = {
	.identification = &identification,
	.config = &config,
	.api = &dev_api,
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

struct delayed_frame
{
	union {
		struct k_event _ev;
		void *tie;
	};
	struct caniot_frame *frame;
};

int caniot_process(void)
{
	return caniot_device_process(&device);
}

static void schedule_event_cb(struct k_event *ev)
{
	caniot_trigger_event();
}

K_EVENT_DEFINE(sig_event, schedule_event_cb);

void caniot_schedule_event(void)
{
	uint32_t remaining = caniot_device_telemetry_remaining(&device);

	k_event_schedule(&sig_event, K_MSEC(remaining));
}