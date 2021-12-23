#include "device.h"

#include <avr/pgmspace.h>

#include <avrtos/kernel.h>

#include <caniot.h>
#include <device.h>

#include "can.h"
#include "dev.h"
#include "caniot_drv_api.h"



static const struct caniot_identification identification PROGMEM =
{
	.did = {
		.cls = __DEVICE_CLS__,
		.dev = __DEVICE_DEV__,
	},
	.version = __FIRMWARE_VERSION__,
	.name = __DEVICE_NAME__,
	.magic_number = __MAGIC_NUMBER__,
};

static struct caniot_config config = {
	.error_response = 1,
	.telemetry_min = 0,
	.telemetry_rdm_delay = 100,
	.telemetry_period = 60000,
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


struct caniot_device device = {
	.identification = &identification,
	.config = &config,
	.api = &dev_api,
	.driv = &drivers
};

void print_indentification(void)
{
	caniot_print_device_identification(&device);
}

uint32_t get_magic_number(void)
{
	return (uint32_t) pgm_read_dword(&device.identification->magic_number);
}

int process_rx_frame(can_message *msg)
{
	int ret;
	struct caniot_frame frame;
	
	frame.id.raw = msg->id;
	frame.len = msg->len;
	memcpy(frame.buf, msg->buf, msg->len);

	ret = caniot_device_process_rx_frame(&device, &frame);

	printf_P(PSTR("caniot_device_process returned = %d\n"), ret);
	
	return ret;
}