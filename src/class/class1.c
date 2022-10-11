#include <stdint.h>

#include <avrtos/kernel.h>

#include <caniot/classes/class1.h>

#include "class.h"
#include "pulse.h"
#include "devices/temp.h"
#include "dev.h"

#include "logging.h"
#define LOG_LEVEL LOG_LEVEL_DBG

#if defined(CONFIG_CLASS1_ENABLED)


#if defined(CONFIG_BOARD_TINY)
static struct xps_context xps_ctx[19u] = {
	[PC0_IDX] = XPS_CONTEXT_INIT(BSP_PC0, GPIO_LOW),
	[PC1_IDX] = XPS_CONTEXT_INIT(BSP_PC1, GPIO_LOW),
	[PC2_IDX] = XPS_CONTEXT_INIT(BSP_PC2, GPIO_LOW),
	[PC3_IDX] = XPS_CONTEXT_INIT(BSP_PC3, GPIO_LOW),
	[PD0_IDX] = XPS_CONTEXT_INIT(BSP_PD0, GPIO_LOW),
	[PD1_IDX] = XPS_CONTEXT_INIT(BSP_PD1, GPIO_LOW),
	[PD2_IDX] = XPS_CONTEXT_INIT(BSP_PD2, GPIO_LOW),
	[PD3_IDX] = XPS_CONTEXT_INIT(BSP_PD3, GPIO_LOW),

	[EIO0_IDX] = XPS_CONTEXT_INIT(BSP_EIO0, GPIO_LOW),
	[EIO1_IDX] = XPS_CONTEXT_INIT(BSP_EIO1, GPIO_LOW),
	[EIO2_IDX] = XPS_CONTEXT_INIT(BSP_EIO2, GPIO_LOW),
	[EIO3_IDX] = XPS_CONTEXT_INIT(BSP_EIO3, GPIO_LOW),
	[EIO4_IDX] = XPS_CONTEXT_INIT(BSP_EIO4, GPIO_LOW),
	[EIO5_IDX] = XPS_CONTEXT_INIT(BSP_EIO5, GPIO_LOW),
	[EIO6_IDX] = XPS_CONTEXT_INIT(BSP_EIO6, GPIO_LOW),
	[EIO7_IDX] = XPS_CONTEXT_INIT(BSP_EIO7, GPIO_LOW),

	[PB0_IDX] = XPS_CONTEXT_INIT(BSP_PB0, GPIO_LOW),
	[PE0_IDX] = XPS_CONTEXT_INIT(BSP_PE0, GPIO_LOW),
	[PE1_IDX] = XPS_CONTEXT_INIT(BSP_PE1, GPIO_LOW),
};

#elif defined(CONFIG_BOARD_V1)

static struct xps_context xps_ctx[4u] = {
	[PC0_IDX] = XPS_CONTEXT_INIT(BSP_OC1, GPIO_LOW),
	[PC1_IDX] = XPS_CONTEXT_INIT(BSP_OC2, GPIO_LOW),
	[PC2_IDX] = XPS_CONTEXT_INIT(BSP_RL1, GPIO_LOW),
	[PC3_IDX] = XPS_CONTEXT_INIT(BSP_RL2, GPIO_LOW),
};

#endif

int class1_blc_telemetry_handler(struct caniot_device *dev,
				 char *buf,
				 uint8_t *len)
{
	struct caniot_blc1_telemetry *const data =
		AS_BLC1_TELEMETRY(buf);

	data->pcpd |= bsp_descr_gpio_input_read(BSP_PC0) << PC0_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PC1) << PC1_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PC2) << PC2_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PC3) << PC3_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PD0) << PD0_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PD1) << PD1_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PD2) << PD2_IDX;
	data->pcpd |= bsp_descr_gpio_input_read(BSP_PD3) << PD3_IDX;

	data->eio |= bsp_descr_gpio_input_read(BSP_EIO0) << (EIO0_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO1) << (EIO1_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO2) << (EIO2_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO3) << (EIO3_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO4) << (EIO4_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO5) << (EIO5_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO6) << (EIO6_IDX & 0x7u);
	data->eio |= bsp_descr_gpio_input_read(BSP_EIO7) << (EIO7_IDX & 0x7u);

	data->pb0 |= bsp_descr_gpio_input_read(BSP_PB0);
	data->pe0 |= bsp_descr_gpio_input_read(BSP_PE0);
	data->pe1 |= bsp_descr_gpio_input_read(BSP_PE1);

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

int class1_blc_command_handler(struct caniot_device *dev,
			       const char *buf,
			       uint8_t len)
{
	struct caniot_blc_command *const cmd = AS_BLC_COMMAND(buf);
	struct caniot_class1_config *const cfg = &dev->config->cls1_gpio;

	uint64_t cmd_u64 = *(uint64_t *)cmd;
	cmd_u64 &= 0x07ffffffffffffffllu;

	LOG_DBG("cls1 cmd_u64: 0x%llx", cmd_u64);

	uint32_t directions = cfg->directions;

	for (uint8_t i = 0u; i < CONFIG_IO_COUNT; i++) {
		if (directions & 1u) {
			command_xps(
				&xps_ctx[i],
				cmd_u64 & 0x7llu,
				cfg->pulse_durations[i]
			);
		}

		directions >>= 1u;
		cmd_u64 >>= 3u;
	}

	
/*
	if (directions & (1llu << (PC0_IDX))) {
		command_xps(&xps_ctx[PC0_IDX],
			    cmd->blc1.cpc0,
			    cfg->pulse_durations[PC0_IDX]);
	}

	if (directions & (1llu << (PC1_IDX))) {
		command_xps(&xps_ctx[PC1_IDX],
			    cmd->blc1.cpc1,
			    cfg->pulse_durations[PC1_IDX]);
	}

	if (directions & (1llu << (PC2_IDX))) {
		command_xps(&xps_ctx[PC2_IDX],
			    cmd->blc1.cpc2,
			    cfg->pulse_durations[PC2_IDX]);
	}

	if (directions & (1llu << (PC3_IDX))) {
		command_xps(&xps_ctx[PC3_IDX],
			    cmd->blc1.cpc3,
			    cfg->pulse_durations[PC3_IDX]);
	}

	if (directions & (1llu << (PD0_IDX))) {
		command_xps(&xps_ctx[PD0_IDX],
			    cmd->blc1.cpd0,
			    cfg->pulse_durations[PD0_IDX]);
	}

	if (directions & (1llu << (PD1_IDX))) {
		command_xps(&xps_ctx[PD1_IDX],
			    cmd->blc1.cpd1,
			    cfg->pulse_durations[PD1_IDX]);
	}

	if (directions & (1llu << (PD2_IDX))) {
		command_xps(&xps_ctx[PD2_IDX],
			    cmd->blc1.cpd2,
			    cfg->pulse_durations[PD2_IDX]);
	}

	if (directions & (1llu << (PD3_IDX))) {
		command_xps(&xps_ctx[PD3_IDX],
			    cmd->blc1.cpd3,
			    cfg->pulse_durations[PD3_IDX]);
	}

	if (directions & (1llu << (EIO0_IDX))) {
		command_xps(&xps_ctx[EIO0_IDX],
			    cmd->blc1.ceio0,
			    cfg->pulse_durations[EIO0_IDX]);
	}

	if (directions & (1llu << (EIO1_IDX))) {
		command_xps(&xps_ctx[EIO1_IDX],
			    cmd->blc1.ceio1,
			    cfg->pulse_durations[EIO1_IDX]);
	}

	if (directions & (1llu << (EIO2_IDX))) {
		command_xps(&xps_ctx[EIO2_IDX],
			    cmd->blc1.ceio2,
			    cfg->pulse_durations[EIO2_IDX]);
	}

	if (directions & (1llu << (EIO3_IDX))) {
		command_xps(&xps_ctx[EIO3_IDX],
			    cmd->blc1.ceio3,
			    cfg->pulse_durations[EIO3_IDX]);
	}

	if (directions & (1llu << (EIO4_IDX))) {
		command_xps(&xps_ctx[EIO4_IDX],
			    cmd->blc1.ceio4,
			    cfg->pulse_durations[EIO4_IDX]);
	}

	if (directions & (1llu << (EIO5_IDX))) {
		command_xps(&xps_ctx[EIO5_IDX],
			    cmd->blc1.ceio5,
			    cfg->pulse_durations[EIO5_IDX]);
	}

	if (directions & (1llu << (EIO6_IDX))) {
		command_xps(&xps_ctx[EIO6_IDX],
			    cmd->blc1.ceio6,
			    cfg->pulse_durations[EIO6_IDX]);
	}

	if (directions & (1llu << (EIO7_IDX))) {
		command_xps(&xps_ctx[EIO7_IDX],
			    cmd->blc1.ceio7,
			    cfg->pulse_durations[EIO7_IDX]);
	}

	if (directions & (1llu << (PB0_IDX))) {
		command_xps(&xps_ctx[PB0_IDX],
			    cmd->blc1.cpb0,
			    cfg->pulse_durations[PB0_IDX]);
	}

	if (directions & (1llu << (PE0_IDX))) {
		command_xps(&xps_ctx[PE0_IDX],
			    cmd->blc1.cpe0,
			    cfg->pulse_durations[PE0_IDX]);
	}

	if (directions & (1llu << (PE1_IDX))) {
		command_xps(&xps_ctx[PE1_IDX],
			    cmd->blc1.cpe1,
			    cfg->pulse_durations[PE1_IDX]);
	}
*/

	return dev_apply_blc_sys_command(dev, &cmd->sys);
}

int class1_config_apply(struct caniot_device *dev,
			struct caniot_config *config)
{
	/* Initialize IO if not initialized */
	if (!dev->flags.initialized) {
		for (uint8_t i = 0u; i < CONFIG_IO_COUNT; i++) {
			bsp_descr_gpio_init(
				xps_ctx[i].descr,
				(config->cls1_gpio.directions >> i) & 1u,
				(config->cls1_gpio.outputs_default >> i) & 1u);
		}
	}

	/* Apply default state */
	for (uint8_t i = 0u; i < CONFIG_IO_COUNT; i++) {
		xps_ctx[i].reset_state = (config->cls1_gpio.outputs_default >> i) & 1u;
	}

	return 0;
}

#endif /* CONFIG_CLASS1_ENABLED */