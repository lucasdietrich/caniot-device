#include "class/class.h"
#include "dev.h"

#include <avrtos/logging.h>

#include <caniot/fake.h>
#if defined(CONFIG_DEV_LOG_LEVEL)
#define LOG_LEVEL CONFIG_DEV_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define K_MODULE K_MODULE_APPLICATION

K_SIGNAL_DEFINE(caniot_process_sig);

const caniot_did_t did = CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__);

static const struct caniot_identification identification PROGMEM = {
	.did	      = CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__),
	.version      = __FIRMWARE_VERSION__,
	.name	      = __DEVICE_NAME__,
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

	const uint32_t uptime_ms = k_uptime_get_ms32();

	*sec = uptime_ms / 1000U;

	if (ms != NULL) {
		*ms = uptime_ms % 1000U;
	}
}

static void caniot2msg(can_message *msg, const struct caniot_frame *frame)
{
	msg->ext   = 0U;
	msg->isext = 0U;
	msg->rtr   = 0U;
	msg->std   = caniot_id_to_canid(frame->id);
	msg->len   = frame->len;
	memcpy(msg->buf, frame->buf, MIN(frame->len, 8U));
}

static void msg2caniot(struct caniot_frame *frame, const can_message *msg)
{
	frame->id  = caniot_canid_to_id(msg->std);
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

struct delayed_msg {
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
				k_mem_slab_free(&dmsg_slab, (void *)dmsg);
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
	.entropy  = entropy,
	.get_time = get_time,
	.set_time = k_time_set,
	.recv	  = caniot_recv,
	.send	  = caniot_send,
};

struct sys_work {
	enum {
		SYS_NONE      = 0u,
		SYS_SW_RESET  = 1u,
		SYS_WDT_RESET = 2u,
	} action;
	struct k_work _work;
};

static void sys_work_handler(struct k_work *w)
{
	struct sys_work *x = CONTAINER_OF(w, struct sys_work, _work);

	switch (x->action) {
	case SYS_SW_RESET: {
		LOG_DBG("Reset (SW) in 1 SEC");
		k_sleep(K_SECONDS(1));

		k_sys_sw_reset();

		CODE_UNREACHABLE;
	}
	case SYS_WDT_RESET: {
		/* Enable watchdog if off */
		if ((WDTCSR & BIT(WDE)) == 0u) {
			wdt_enable(WATCHDOG_TIMEOUT_WDTO);
		}

		LOG_DBG("Reset (WDT) in 1 SEC");

		k_sleep(K_SECONDS(1));

		irq_disable();

		for (;;) {
			/* wait for WDT reset */
		}

		CODE_UNREACHABLE;
	}
	default:
		break;
	}
}

static struct sys_work sys_work = {
	.action = SYS_NONE,
	._work	= K_WORK_INITIALIZER(sys_work_handler),
};

uint16_t get_t10_temperature(temp_sens_t sens)
{
	uint16_t temp10 = CANIOT_DT_T10_INVALID;

	const int16_t temp16 = temp_read(sens);
	if (temp16 != CANIOT_DT_T16_INVALID) {
		temp10 = caniot_dt_T16_to_T10(temp16);
	}

	return temp10;
}

int dev_apply_blc_sys_command(struct caniot_device *dev,
			      struct caniot_blc_sys_command *sysc)
{
	int ret = 0;

	if (sysc->watchdog_reset == CANIOT_SS_CMD_SET ||
	    sysc->reset == CANIOT_SS_CMD_SET) {
		sys_work.action = SYS_WDT_RESET;
		ret = k_system_workqueue_submit(&sys_work._work) ? 0 : -EINVAL;
	} else if (sysc->software_reset == CANIOT_SS_CMD_SET) {
		sys_work.action = SYS_SW_RESET;
		ret = k_system_workqueue_submit(&sys_work._work) == true ? 0 : -EINVAL;
	} else if (sysc->watchdog == CANIOT_TS_CMD_ON) {
		wdt_enable(WATCHDOG_TIMEOUT_WDTO);
		ret = 0;
	} else if (sysc->watchdog == CANIOT_TS_CMD_OFF) {
		wdt_disable();
		ret = 0;
	} else if (sysc->config_reset == CANIOT_SS_CMD_SET) {
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
		switch (__DEVICE_CLS__) {
		case CANIOT_DEVICE_CLASS0:
			return class0_blc_telemetry_handler(dev, buf, len);
		case CANIOT_DEVICE_CLASS1:
			return class1_blc_telemetry_handler(dev, buf, len);
		default:
			return -CANIOT_ENOTSUP;
		}
	} else {
		return app_telemetry_handler(dev, ep, buf, len);
	}
}

int command_handler(struct caniot_device *dev,
		    caniot_endpoint_t ep,
		    const char *buf,
		    uint8_t len)
{
	int ret = -CANIOT_ENOTSUP;

	switch (ep) {
	case CANIOT_ENDPOINT_BOARD_CONTROL:
		switch (__DEVICE_CLS__) {
#if defined(CONFIG_CLASS0_ENABLED)
		case CANIOT_DEVICE_CLASS0:
			ret = class0_blc_command_handler(dev, buf, len);
			break;
#endif
#if defined(CONFIG_CLASS1_ENABLED)
		case CANIOT_DEVICE_CLASS1:
			ret = class1_blc_command_handler(dev, buf, len);
			break;
#endif
		default:
			break;
		}
		break;
	case CANIOT_ENDPOINT_APP:
	case CANIOT_ENDPOINT_1:
	case CANIOT_ENDPOINT_2:
		ret = app_command_handler(dev, ep, buf, len);
		break;
	default:
		break;
	}

	return ret;
}

__attribute__((section(".noinit"))) static struct caniot_config config;
__STATIC_ASSERT(sizeof(config) <= 0xFF,
		"config too big"); /* EEPROM size depends on MCU */

extern struct caniot_config default_config;

const struct caniot_api api = CANIOT_API_STD_INIT(
	command_handler, telemetry_handler, config_on_read, config_on_write);

struct caniot_device device = {
	.identification = &identification,
	.config		= &config,
	.api		= &api,
	.driv		= &drivers,
	.flags =
		{
			.request_telemetry = 0u,
			.initialized	   = 0u,
		},
};

int command_xps(struct xps_context *xpsc,
		caniot_complex_digital_cmd_t cmd,
		uint32_t duration_ms)
{
	__ASSERT_TRUE(xpsc != NULL);

	LOG_DBG("%x: %u", xpsc->descr, cmd);

	if (BSP_DESCR_STATUS_GET(xpsc->descr) != BSP_DESCR_ACTIVE) {
		return -ENOTSUP;
	}

	switch (cmd) {
	case CANIOT_XPS_SET_ON:
		bsp_descr_gpio_output_write(xpsc->descr, GPIO_HIGH);
		break;
	case CANIOT_XPS_SET_OFF:
		bsp_descr_gpio_output_write(xpsc->descr, GPIO_LOW);
		break;
	case CANIOT_XPS_TOGGLE:
		bsp_descr_gpio_toggle(xpsc->descr);
		break;

#if CONFIG_GPIO_PULSE_SUPPORT
	case CANIOT_XPS_PULSE_ON:
	case CANIOT_XPS_PULSE_OFF:
		xpsc->pev = pulse_trigger(
			xpsc->descr, cmd == CANIOT_XPS_PULSE_ON, duration_ms, NULL);
		LOG_DBG("XPS: descr=%u pev=%p rest=%u cmd=%u dur=%lu",
			xpsc->descr,
			xpsc->pev,
			xpsc->reset_state,
			cmd,
			duration_ms);
		break;
	case CANIOT_XPS_PULSE_CANCEL:
		pulse_cancel(xpsc->pev);
		break;
#endif

	case CANIOT_XPS_RESET:
#if CONFIG_GPIO_PULSE_SUPPORT
		pulse_cancel(xpsc->pev);
#endif
		bsp_descr_gpio_output_write(xpsc->descr, xpsc->reset_state);
	default:
		break;
	}

	return 0;
}

void print_indentification(void)
{
	caniot_print_device_identification(&device);
}

uint32_t get_magic_number(void)
{
	return (uint32_t)pgm_read_dword(&device.identification->magic_number);
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
			crc	    = (crc >> 1) ^ (mix ? 0x8C : 0x00);
		}
	}

	return crc;
}

static int config_apply(struct caniot_device *dev, struct caniot_config *cfg)
{
	set_zone(cfg->timezone);

	switch (__DEVICE_CLS__) {
	case CANIOT_DEVICE_CLASS0:
		return class0_config_apply(dev, cfg);
	case CANIOT_DEVICE_CLASS1:
		return class1_config_apply(dev, cfg);
	default:
		return -CANIOT_ENOTSUP;
	}
}

/**
 * @brief Indicates whether the configuration is still valid or not.
 * TODO can be removed
 */
static bool config_dirty = true;

int config_on_read(struct caniot_device *dev, struct caniot_config *cfg)
{
	if (config_dirty == true) {
		uint8_t checksum = eeprom_read_byte(0x0000U);

		eeprom_read_block(
			cfg, (const void *)0x0001U, sizeof(struct caniot_config));

		uint8_t calculated_checksum =
			checksum_crc8((const uint8_t *)cfg, sizeof(struct caniot_config));

		if (checksum != calculated_checksum) {
			return -EINVAL;
		}

		config_dirty = false;
	}

	return 0;
}

int config_on_write(struct caniot_device *dev, struct caniot_config *cfg)
{
	eeprom_update_block(
		(const void *)cfg, (void *)0x0001U, sizeof(struct caniot_config));

	uint8_t calculated_checksum =
		checksum_crc8((const uint8_t *)cfg, sizeof(struct caniot_config));

	eeprom_update_byte((uint8_t *)0x0000U, calculated_checksum);

	config_dirty = true;

	return config_apply(dev, cfg);
}

int config_restore_default(struct caniot_device *dev, struct caniot_config *cfg)
{
	memcpy_P(cfg, &default_config, sizeof(struct caniot_config));

	return config_on_write(&device, cfg);
}

#if CONFIG_FORCE_RESTORE_DEFAULT_CONFIG
#warning "CONFIG_FORCE_RESTORE_DEFAULT_CONFIG" is enabled
#endif

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
	} else {
		config_apply(&device, device.config);
	}
}

void caniot_init(void)
{
	/* we prepare the system according to the config */
	set_zone(device.config->timezone);

	caniot_app_init(&device);

	device.flags.initialized = 1;
}