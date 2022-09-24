#include "dev.h"

#include <caniot/fake.h>

#include "logging.h"
#if defined(CONFIG_DEV_LOG_LEVEL)
#	define LOG_LEVEL CONFIG_DEV_LOG_LEVEL
#else
#	define LOG_LEVEL LOG_LEVEL_NONE
#endif


K_SIGNAL_DEFINE(caniot_process_sig);

const caniot_did_t did = CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__);

static const struct caniot_identification identification PROGMEM =
{
	.did = CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__),
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
}

static void caniot2msg(can_message *msg, const struct caniot_frame *frame)
{
	msg->ext = 0U;
	msg->isext = 0U;
	msg->rtr = 0U;
	msg->std = caniot_id_to_canid(frame->id);
	msg->len = frame->len;
	memcpy(msg->buf, frame->buf, MIN(frame->len, 8U));
}

static void msg2caniot(struct caniot_frame *frame, const can_message *msg)
{
	frame->id = caniot_canid_to_id(msg->std);
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

#if LOG_LEVEL >= LOG_LEVEL_INF
		k_show_uptime();
		caniot_explain_frame(frame);
		LOG_INF_RAW("\n");
#endif
		
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

/* Should be increased if "delayed message" feature is used */
K_MEM_SLAB_DEFINE(dmsg_slab, sizeof(struct delayed_msg), 1U);

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
#if LOG_LEVEL >= LOG_LEVEL_INF
		k_show_uptime();
		caniot_explain_frame(frame);
		LOG_INF_RAW("\n");
#endif
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
	LOG_DBG("Reset (SW) in 1 SEC");

	k_sleep(K_SECONDS(1));
	k_sys_sw_reset();
}

static void wdt_reset_work_handler(struct k_work *w)
{	
	LOG_DBG("Reset (WDT) in 1 SEC");

	k_sleep(K_SECONDS(1));

	irq_disable();

	for(;;) {
		/* wait for WDT reset */
	}
}

static struct k_work sw_reset_work = K_WORK_INIT(sw_reset_work_handler);

static struct k_work wdt_reset_work = K_WORK_INIT(wdt_reset_work_handler);

static uint16_t get_t10_temperature(temp_sens_t sens)
{
	uint16_t temp10 = CANIOT_DT_T10_INVALID;

	const int16_t temp16 = temp_read(sens);
	if (temp16 != CANIOT_DT_T16_INVALID) {
		temp10 = caniot_dt_T16_to_T10(temp16);
	}

	return temp10;
}

static int board_control_telemetry_handler(struct caniot_device *dev,
					   char *buf,
					   uint8_t *len)
{
	struct caniot_board_control_telemetry *const data =
		AS_BOARD_CONTROL_TELEMETRY(buf);

#if defined(CONFIG_BOARD_V1)
	struct board_dio dio = ll_read();
	data->dio = dio.raw;
#if CONFIG_GPIO_PULSE_SUPPORT
	data->poc1 = pulse_is_active(OC1);
	data->poc2 = pulse_is_active(OC2);
	data->prl1 = pulse_is_active(RL1);
	data->prl2 = pulse_is_active(RL2);
#else
	data->poc1 = 0U;
	data->poc2 = 0U;
	data->prl1 = 0U;
	data->prl2 = 0U;
#endif /* CONFIG_GPIO_PULSE_SUPPORT */
#endif /* CONFIG_BOARD_V1 */

#if CONFIG_CANIOT_FAKE_TEMPERATURE
	data->ext_temperature =
		caniot_fake_get_temp(dev);
	data->int_temperature =
		caniot_fake_get_temp(dev);
#else
	data->int_temperature = get_t10_temperature(TEMP_SENS_INT);
	data->ext_temperature = get_t10_temperature(TEMP_SENS_EXT_1);
	data->ext_temperature2 = get_t10_temperature(TEMP_SENS_EXT_2);
	data->ext_temperature3 = get_t10_temperature(TEMP_SENS_EXT_3);
#endif

	*len = 8U;

	return 0;
}

static int board_control_command_handler(struct caniot_device *dev,
					 const char *buf,
					 uint8_t len)
{
	int ret = 0;

	struct caniot_board_control_command *const cmd =
		AS_BOARD_CONTROL_CMD(buf);

#if defined(CONFIG_BOARD_V1)
	command_output(OC1, cmd->coc1);
	command_output(OC2, cmd->coc2);
	command_output(RL1, cmd->crl1);
	command_output(RL2, cmd->crl2);
#else
	/* TODO */
#endif

	if (cmd->watchdog_reset == CANIOT_SS_CMD_SET ||
	    cmd->reset == CANIOT_SS_CMD_SET) {
		if (WDTCSR & BIT(WDE)) {
			ret = k_system_workqueue_submit(&wdt_reset_work) ? 0 : -EINVAL;
		} else {
			LOG_WRN("WTD off");
			ret = -EINVAL;
		}
	} else 	if (cmd->software_reset == CANIOT_SS_CMD_SET) {
		ret = k_system_workqueue_submit(&sw_reset_work) == true ? 0 : -EINVAL;
	} else if (cmd->watchdog == CANIOT_TS_CMD_ON) {
		wdt_enable(WATCHDOG_TIMEOUT_WDTO);
		ret = 0;
	} else if (cmd->watchdog == CANIOT_TS_CMD_OFF) {
		wdt_disable();
		ret = 0;
	} else if (cmd->config_reset == CANIOT_SS_CMD_SET) {
		ret = config_restore_default(dev, dev->config);
	}

	return ret;
}

extern const caniot_telemetry_handler_t app_telemetry_handler;

extern const caniot_command_handler_t app_command_handler;

int telemetry_handler(struct caniot_device *dev,
		      caniot_endpoint_t ep, 
		      char *buf,
		      uint8_t *len)
{
	if (ep == CANIOT_ENDPOINT_BOARD_CONTROL) {
		return board_control_telemetry_handler(dev, buf, len);
	} else {
		return app_telemetry_handler(dev, ep, buf, len);
	}
}

int command_handler(struct caniot_device *dev,
		    caniot_endpoint_t ep,
		    const char *buf,
		    uint8_t len)
{
	if (ep == CANIOT_ENDPOINT_BOARD_CONTROL) {
		return board_control_command_handler(dev, buf, len);
	} else {
		return app_command_handler(dev, ep, buf, len);
	}
}

__attribute__((section(".noinit"))) static struct caniot_config config;
__STATIC_ASSERT(sizeof(config) <= 0xFF, "config to big"); /* EEPROM size depends on MCU */

extern struct caniot_config default_config;

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

#define GET_CUSTOM_GPIO_CONFIG() (device.config->custompcb.gpio)

#if CONFIG_GPIO_PULSE_SUPPORT
static uint32_t config_get_pulse_duration_ms(output_t pin)
{
	if (GPIO_VALID_OUTPUT_PIN(pin)) {
		return GET_CUSTOM_GPIO_CONFIG().pulse_duration.array[pin];
	}

	return 0U;
}
#endif /* CONFIG_GPIO_PULSE_SUPPORT */

#if defined(CONFIG_BOARD_V1)
void command_output(output_t pin, caniot_complex_digital_cmd_t cmd)
{
	if (!GPIO_VALID_OUTPUT_PIN(pin)) {
		return;
	}

	switch (cmd) {
	case CANIOT_XPS_SET_ON:
		ll_outputs_set_mask(BIT(pin), BIT(pin));
		break;
	case CANIOT_XPS_SET_OFF:
		ll_outputs_set_mask(0U, BIT(pin));
		break;
	case CANIOT_XPS_TOGGLE:
		ll_outputs_toggle_mask(BIT(pin));
		break;

#if CONFIG_GPIO_PULSE_SUPPORT
	case CANIOT_XPS_PULSE_CANCEL:
		pulse_cancel(pin);
		break;
	case CANIOT_XPS_PULSE_ON:
		pulse_trigger(pin, true, config_get_pulse_duration_ms(pin));
		break;
	case CANIOT_XPS_PULSE_OFF:
		pulse_trigger(pin, false, config_get_pulse_duration_ms(pin));
		break;
#endif 

	case CANIOT_XPS_RESET:
#if CONFIG_GPIO_PULSE_SUPPORT
		pulse_cancel(pin);
#endif
		ll_outputs_set_mask(
			GET_CUSTOM_GPIO_CONFIG().mask.outputs_default.mask & BIT(pin),
			BIT(pin));

	default:
		break;
	}
}
#endif /* #if CONFIG_BOARD_V1 */

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

uint32_t get_telemetry_timeout(void)
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
				struct caniot_config *cfg)
{
	set_zone(cfg->timezone);

	return 0;
}

/**
 * @brief Indicates whether the configuration is still valid or not.
 * TODO can be removed
 */
static bool config_dirty = true;

int config_on_read(struct caniot_device *dev,
		   struct caniot_config *cfg)
{
	if (config_dirty == true) {
		uint8_t checksum = eeprom_read_byte(0x0000U);

		eeprom_read_block(cfg, (const void *)0x0001U,
				  sizeof(struct caniot_config));

		uint8_t calculated_checksum = checksum_crc8((const uint8_t *)cfg,
							    sizeof(struct caniot_config));

		if (checksum != calculated_checksum) {
			return -EINVAL;
		}

		config_dirty = false;
	}

	return 0;
}

int config_on_write(struct caniot_device *dev,
		    struct caniot_config *cfg)
{
	eeprom_update_block((const void *)cfg,
			    (void *)0x0001U, sizeof(struct caniot_config));

	uint8_t calculated_checksum = checksum_crc8((const uint8_t *)cfg,
						    sizeof(struct caniot_config));

	eeprom_update_byte((uint8_t *)0x0000U, calculated_checksum);

	config_dirty = true;

	return config_apply_changes(dev, cfg);
}

int config_restore_default(struct caniot_device *dev,
			   struct caniot_config *cfg)
{
	memcpy_P(cfg, &default_config, sizeof(struct caniot_config));

	return config_on_write(&device, cfg);
}

void config_init(void)
{
	bool restore = false;

	if (CONFIG_FORCE_RESTORE_DEFAULT_CONFIG == 0) {
		/* sanity check on EEPROM */
		if (config_on_read(&device, device.config) != 0) {
			restore = true;
		} 
	}

	/* if restore is true, we copy the default configuration to EEPROM and RAM */
	if (restore || (CONFIG_FORCE_RESTORE_DEFAULT_CONFIG == 1)) {
		LOG_DBG("Config reset ...");
		
		memcpy_P(&config, &default_config, sizeof(struct caniot_config));

		config_on_write(&device, device.config);
	}
}

void caniot_init(void)
{	
	/* we prepare the system according to the config */
	set_zone(device.config->timezone);
	
	caniot_app_init(&device);
}