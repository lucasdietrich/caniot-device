#include "dev.h"

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

// #if DEBUG
// 	printf_P(PSTR("get_time: sec=%lu sec\n"), *sec);
// #endif /* DEBUG */
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
	int ret;

	if (delay_ms < KERNEL_TICK_PERIOD_MS) {
		can_message msg;

		caniot2msg(&msg, frame);

		ret = can_txq_message(&msg);
	} else {
		struct delayed_msg *dmsg;
		
		ret = k_mem_slab_alloc(&dmsg_slab, (void **)&dmsg, K_NO_WAIT);
		if (ret == 0) {
			caniot2msg(&dmsg->msg, frame);
			k_event_init(&dmsg->ev, dmsg_handler);
			ret = k_event_schedule(&dmsg->ev, K_MSEC(delay_ms));
			if (ret != 0) {
				k_mem_slab_free(&dmsg_slab, (void*) dmsg);
			}
		}
	}

	if (ret == 0) {
		k_show_uptime();
		caniot_explain_frame(frame);
		printf_P(PSTR("\n"));
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

static void sw_reset_work_handler(struct k_work *w)
{	
	printf_P(PSTR("Reset (SW) in 1 SEC\n"));
	k_sleep(K_SECONDS(1));
	printf_P(PSTR("Resetting (SW) ...\n"));
	k_sys_sw_reset();
}

static void wdt_reset_work_handler(struct k_work *w)
{	
	printf_P(PSTR("Reset (WDT) in 1 SEC\n"));
	k_sleep(K_SECONDS(1));
	printf_P(PSTR("Resetting (WDT) ...\n"));
	
	k_sched_lock();
	for(;;) { }
}

static struct k_work sw_reset_work = K_WORK_INIT(sw_reset_work_handler);

static struct k_work wdt_reset_work = K_WORK_INIT(wdt_reset_work_handler);

// usage command_relay(RL1, CANIOT_TS_CMD_TOGGLE);
void command_relay(uint8_t relay, caniot_twostate_cmd_t cmd)
{
	switch (cmd) {
	case CANIOT_TS_CMD_ON:
		ll_relays_set_mask(BIT(relay), BIT(relay));
		break;
	case CANIOT_TS_CMD_OFF:
		ll_relays_set_mask(0, BIT(relay));
		break;
	case CANIOT_TS_CMD_TOGGLE:
		ll_relays_toggle_mask(BIT(relay));
		break;
	default:
		break;
	}
}

// usage command_opencollector(OC2, CANIOT_TS_CMD_TOGGLE);
void command_opencollector(uint8_t oc, caniot_twostate_cmd_t cmd)
{
	switch (cmd) {
	case CANIOT_TS_CMD_ON:
		ll_oc_set_mask(BIT(oc), BIT(oc));
		break;
	case CANIOT_TS_CMD_OFF:
		ll_oc_set_mask(0, BIT(oc));
		break;
	case CANIOT_TS_CMD_TOGGLE:
		ll_oc_toggle_mask(BIT(oc));
		break;
	default:
		break;
	}
}

static int board_control_telemetry_handler(struct caniot_device *dev,
					 char *buf,
					 uint8_t *len)
{
	struct board_dio dio = ll_read();

	AS_BOARD_CONTROL_TELEMETRY(buf)->r1 = dio.r1;
	AS_BOARD_CONTROL_TELEMETRY(buf)->r2 = dio.r2;
	AS_BOARD_CONTROL_TELEMETRY(buf)->oc1 = dio.oc1;
	AS_BOARD_CONTROL_TELEMETRY(buf)->oc2 = dio.oc2;
	AS_BOARD_CONTROL_TELEMETRY(buf)->in1 = dio.in1;
	AS_BOARD_CONTROL_TELEMETRY(buf)->in2 = dio.in2;
	AS_BOARD_CONTROL_TELEMETRY(buf)->in3 = dio.in3;
	AS_BOARD_CONTROL_TELEMETRY(buf)->in4 = dio.in4;

	const int16_t temperature = dev_int_temperature();
	AS_BOARD_CONTROL_TELEMETRY(buf)->int_temperature = caniot_dt_T16_to_Temp(temperature);

	int16_t temp;
	if (CONFIG_APP_OW_EXTTEMP && (ow_ext_get(&temp) == true)) {
		AS_BOARD_CONTROL_TELEMETRY(buf)->ext_temperature =
			caniot_dt_T16_to_Temp(temp);
	} else {
		AS_BOARD_CONTROL_TELEMETRY(buf)->ext_temperature = 
			CANIOT_DT_T10_INVALID;
	}
	
	*len = 8U;

	return 0;
}

static int board_control_command_handler(struct caniot_device *dev,
				       char *buf,
				       uint8_t len)
{
	int ret;

	command_relay(RL1, AS_BOARD_CONTROL_CMD(buf)->r1);
	command_relay(RL2, AS_BOARD_CONTROL_CMD(buf)->r2);
	command_opencollector(OC1, AS_BOARD_CONTROL_CMD(buf)->oc1);
	command_opencollector(OC2, AS_BOARD_CONTROL_CMD(buf)->oc2);

	if (AS_BOARD_CONTROL_CMD(buf)->watchdog_reset == CANIOT_SS_CMD_SET ||
	    AS_BOARD_CONTROL_CMD(buf)->reset == CANIOT_SS_CMD_SET) {
		if (WDTCSR & BIT(WDE)) {
			ret = k_system_workqueue_submit(&wdt_reset_work) == true ? 0 : -EINVAL;
		} else {
			printf_P(PSTR("Watchdog not enabled\n"));
			ret = -EINVAL;
		}
	} else 	if (AS_BOARD_CONTROL_CMD(buf)->software_reset == CANIOT_SS_CMD_SET) {
		ret = k_system_workqueue_submit(&sw_reset_work) == true ? 0 : -EINVAL;
	} else if (AS_BOARD_CONTROL_CMD(buf)->watchdog == CANIOT_TS_CMD_ON) {
		wdt_enable(WATCHDOG_TIMEOUT_WDTO);
		ret = 0;
	} else if (AS_BOARD_CONTROL_CMD(buf)->watchdog == CANIOT_TS_CMD_OFF) {
		wdt_disable();
		ret = 0;
	}

	return ret;
}

extern const caniot_telemetry_handler_t app_telemetry_handler;

extern const caniot_command_handler_t app_command_handler;

int telemetry_handler(struct caniot_device *dev,
		      uint8_t ep, char *buf,
		      uint8_t *len)
{
	if (ep == endpoint_board_control) {
		return board_control_telemetry_handler(dev, buf, len);
	} else {
		return app_telemetry_handler(dev, ep, buf, len);
	}
}

int command_handler(struct caniot_device *dev,
		    uint8_t ep,
		    char *buf,
		    uint8_t len)
{
	if (ep == endpoint_board_control) {
		return board_control_command_handler(dev, buf, len);
	} else {
		return app_command_handler(dev, ep, buf, len);
	}
}

/* contain default config */
extern struct caniot_config config;

const struct caniot_api api = CANIOT_API_STD_INIT(command_handler, telemetry_handler,
						  config_on_read, config_on_write);

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

void trigger_telemetry(void)
{
	device.flags.request_telemetry = 1;

	trigger_process();
}

// compute CRC8
static uint8_t checksum_crc8(const uint8_t *buf, size_t len)
{
	uint8_t crc = 0;

	while (len--) {
		uint8_t inbyte = *buf++;
		uint8_t i;

		for (i = 0x80; i > 0; i >>= 1) {
			uint8_t mix = (crc ^ inbyte) & i;
			crc = (crc >> 1) ^ (mix ? 0x8C : 0x00);
		}
	}

	return crc;
}

static int config_apply_changes(struct caniot_device *dev,
				struct caniot_config *config)
{
	set_zone(config->timezone);

	return 0;
}

/**
 * @brief Indicates whether the configuration is still valid or not.
 */
static bool config_dirty = true;

int config_on_read(struct caniot_device *dev,
		   struct caniot_config *config)
{
	if (config_dirty == true) {
		uint8_t checksum = eeprom_read_byte(0x0000U);

		eeprom_read_block(config, (const void *)0x0001U,
				  sizeof(struct caniot_config));

		uint8_t calculated_checksum = checksum_crc8((const uint8_t *)config,
							    sizeof(struct caniot_config));

		if (checksum != calculated_checksum) {
			return -EINVAL;
		}

		config_dirty = false;
	}

	return 0;
}

int config_on_write(struct caniot_device *dev,
		    struct caniot_config *config)
{
	eeprom_update_block((const void *)config,
			    (void *)0x0001U, sizeof(struct caniot_config));

	uint8_t calculated_checksum = checksum_crc8((const uint8_t *)config,
						    sizeof(struct caniot_config));

	eeprom_update_byte((uint8_t*) 0x0000U, calculated_checksum);

	config_dirty = true;

	return config_apply_changes(dev, config);
}

void config_init(void)
{
	/* sanity check on EEPROM :
	 * read config from EEPROM, without overwriting current configuration */
	struct caniot_config tmp;
	if (config_on_read(&device, &tmp) == 0) {
		/* EEPROM config is valid, we can overwrite the current config */
		memcpy(&config, &tmp, sizeof(struct caniot_config));
	} else {
		printf_P(PSTR("Config reset ... \n"));
		
		config_on_write(&device, &config);
	}
}

void caniot_init(void)
{	
	/* we prepare the system according to the config */
	set_zone(device.config->timezone);
	
	caniot_device_init(&device);
}