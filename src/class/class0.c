#include <stdint.h>

#include <avrtos/kernel.h>

#include <caniot/classes/class0.h>

#include "class.h"
#include "pulse.h"
#include "devices/temp.h"
#include "dev.h"

#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_DBG

#if defined(CONFIG_CLASS0_ENABLED)


/* Class 0 defines:
 * - 2 open collector outputs
 * - 2 relay outputs
 * - 4 digital inputs
 */

#if defined(CONFIG_BOARD_V1)
static struct xps_context xps_ctx[4u] = {
	[OC1_IDX] = XPS_CONTEXT_INIT(BSP_OC1, GPIO_LOW),
	[OC2_IDX] = XPS_CONTEXT_INIT(BSP_OC2, GPIO_LOW),
	[RL1_IDX] = XPS_CONTEXT_INIT(BSP_RL1, GPIO_LOW),
	[RL2_IDX] = XPS_CONTEXT_INIT(BSP_RL2, GPIO_LOW),
};

#elif defined(CONFIG_BOARD_V2)

static struct xps_context xps_ctx[] = {

};

#endif

int class0_blc_telemetry_handler(struct caniot_device *dev,
				 char *buf,
				 uint8_t *len)
{
	struct caniot_blc0_telemetry *const data =
		AS_BLC0_TELEMETRY(buf);

	data->dio |= bsp_descr_gpio_input_read(BSP_OC1) << OC1_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_OC2) << OC2_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_RL1) << RL1_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_RL2) << RL2_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_IN1) << IN1_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_IN2) << IN2_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_IN3) << IN3_IDX;
	data->dio |= bsp_descr_gpio_input_read(BSP_IN4) << IN4_IDX;

#if CONFIG_GPIO_PULSE_SUPPORT
	data->pdio |= pulse_is_active(xps_ctx[OC1_IDX].pev);
	data->pdio |= pulse_is_active(xps_ctx[OC2_IDX].pev);
	data->pdio |= pulse_is_active(xps_ctx[RL1_IDX].pev);
	data->pdio |= pulse_is_active(xps_ctx[RL2_IDX].pev);
#endif 

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

int class0_blc_command_handler(struct caniot_device *dev,
			       const char *buf,
			       uint8_t len)
{
	struct caniot_blc_command *const cmd = AS_BLC_COMMAND(buf);
	struct caniot_class0_config *const cfg = &dev->config->cls0_gpio;

	command_xps(&xps_ctx[OC1_IDX],
		    cmd->blc0.coc1,
		    cfg->pulse_durations[OC1_IDX]);

	command_xps(&xps_ctx[OC2_IDX],
		    cmd->blc0.coc2,
		    cfg->pulse_durations[OC2_IDX]);
		
	command_xps(&xps_ctx[RL1_IDX],
		    cmd->blc0.crl1,
		    cfg->pulse_durations[RL1_IDX]);

	command_xps(&xps_ctx[RL2_IDX],
		    cmd->blc0.crl2,
		    cfg->pulse_durations[RL2_IDX]);

	return dev_apply_blc_sys_command(dev, &cmd->sys);
}

int class0_config_apply(struct caniot_device *dev,
			struct caniot_config *config)
{
	/* TODO if !dev->flags.initialized, set port default value */
	struct caniot_class0_config *const c0 = &config->cls0_gpio;

	/* Initialize IO if not initialized */
	if (!dev->flags.initialized) {
		for (uint8_t i = 0u; i < ARRAY_SIZE(xps_ctx); i++) {
			bsp_descr_gpio_pin_init(
				xps_ctx[i].descr,
				GPIO_OUTPUT,
				(c0->outputs_default >> i) & 1u);
		}
	}

	const uint32_t mask = c0->telemetry_on_change;

	bsp_pin_pci_set_enabled(BSP_OC1, mask & BIT(OC1_IDX));
	bsp_pin_pci_set_enabled(BSP_OC2, mask & BIT(OC2_IDX));
	bsp_pin_pci_set_enabled(BSP_RL1, mask & BIT(RL1_IDX));
	bsp_pin_pci_set_enabled(BSP_RL2, mask & BIT(RL2_IDX));
	bsp_pin_pci_set_enabled(BSP_IN1, mask & BIT(IN1_IDX));
	bsp_pin_pci_set_enabled(BSP_IN2, mask & BIT(IN2_IDX));
	bsp_pin_pci_set_enabled(BSP_IN3, mask & BIT(IN3_IDX));
	bsp_pin_pci_set_enabled(BSP_IN4, mask & BIT(IN4_IDX));

	LOG_DBG("c0 mask=%x PCMSK reg 0=%hhx 1=%hhx 2=%hhx",
		mask,
		PCI_CTRL_DEVICE->PCMSK[0u],
		PCI_CTRL_DEVICE->PCMSK[1u],
		PCI_CTRL_DEVICE->PCMSK[2u]);

	// /* TODO make sure to not break a running pulse ? */
	// xps_ctx[OC1_IDX].reset_state = (c0->outputs_default >> OC1_IDX) & 1u;
	// xps_ctx[OC2_IDX].reset_state = (c0->outputs_default >> OC2_IDX) & 1u;
	// xps_ctx[RL1_IDX].reset_state = (c0->outputs_default >> RL1_IDX) & 1u;
	// xps_ctx[RL2_IDX].reset_state = (c0->outputs_default >> RL2_IDX) & 1u;

	return 0;
}

#endif /* CONFIG_CLASS0_ENABLED */