#include "class/class.h"
#include "dev.h"
#include "settings.h"

#include <time.h>

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

static const struct caniot_device_id identification PROGMEM = {
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

	uint64_t time_ms = k_time_get_ms();

	*sec = time_ms / 1000u;

	if (ms != NULL) {
		*ms = time_ms % 1000u;
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
		ret = -ENOTSUP;
	} else if (sysc->watchdog == CANIOT_TS_CMD_ON) {
		wdt_enable(WATCHDOG_TIMEOUT_WDTO);
		ret = 0;
	} else if (sysc->watchdog == CANIOT_TS_CMD_OFF) {
		wdt_disable();
		ret = 0;
	} else if (sysc->config_reset == CANIOT_SS_CMD_SET) {
		ret = settings_restore_default(dev, dev->config);
	}

	return ret;
}

extern const caniot_telemetry_handler_t app_telemetry_handler;
extern const caniot_command_handler_t app_command_handler;

int telemetry_handler(struct caniot_device *dev,
		      caniot_endpoint_t ep,
		      unsigned char *buf,
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
		    const unsigned char *buf,
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

const struct caniot_device_api api = CANIOT_DEVICE_API_STD_INIT(
	command_handler, telemetry_handler, settings_read, settings_write);

extern struct caniot_device_config settings_rambuf;

struct caniot_device device = {
	.identification = &identification,
	.config		= &settings_rambuf,
	.api		= &api,
	.driv		= &drivers,
	.flags =
		{
			.request_telemetry_ep = 0u,
			.initialized	      = 0u,
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
	return caniot_device_triggered_telemetry_any(&device);
}

void trigger_telemetry(caniot_endpoint_t ep)
{
	caniot_device_trigger_telemetry_ep(&device, ep);

	trigger_process();
}

void caniot_init(void)
{
	/* we prepare the system according to the config */
	set_zone(device.config->timezone);

	caniot_app_init(&device);

	settings_init(&device);
}