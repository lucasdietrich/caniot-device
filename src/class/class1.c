/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "class.h"
#include "config.h"
#include "dev.h"
#include "devices/gpio_pulse.h"
#include "devices/gpio_xps.h"
#include "devices/temp.h"

#include <stdint.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#define LOG_LEVEL CONFIG_BOARD_LOG_LEVEL

#if defined(CONFIG_CLASS1_ENABLED)

#if defined(CONFIG_BOARD_TINY_REVA)
static struct xps_context xps_ctx[19u] = {
    [PC0_IDX] = XPS_CONTEXT_INIT(BSP_PC0, GPIO_LOW),
    [PC1_IDX] = XPS_CONTEXT_INIT(BSP_PC1, GPIO_LOW),
    [PC2_IDX] = XPS_CONTEXT_INIT(BSP_PC2, GPIO_LOW),
    [PC3_IDX] = XPS_CONTEXT_INIT(BSP_PC3, GPIO_LOW),
    [PD4_IDX] = XPS_CONTEXT_INIT(BSP_PD4, GPIO_LOW),
    [PD5_IDX] = XPS_CONTEXT_INIT(BSP_PD5, GPIO_LOW),
    [PD6_IDX] = XPS_CONTEXT_INIT(BSP_PD6, GPIO_LOW),
    [PD7_IDX] = XPS_CONTEXT_INIT(BSP_PD7, GPIO_LOW),

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
                                 unsigned char *buf,
                                 uint8_t *len)
{
    struct caniot_blc1_telemetry data = {0};

    data.pcpd |= bsp_descr_gpio_input_read(BSP_PC0) << PC0_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PC1) << PC1_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PC2) << PC2_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PC3) << PC3_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PD4) << PD4_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PD5) << PD5_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PD6) << PD6_IDX;
    data.pcpd |= bsp_descr_gpio_input_read(BSP_PD7) << PD7_IDX;

    /* TODO: Optimize this to read only a single time PCF state */
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO0) << (EIO0_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO1) << (EIO1_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO2) << (EIO2_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO3) << (EIO3_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO4) << (EIO4_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO5) << (EIO5_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO6) << (EIO6_IDX & 0x7u);
    data.eio |= bsp_descr_gpio_input_read(BSP_EIO7) << (EIO7_IDX & 0x7u);

    data.pb0 = bsp_descr_gpio_input_read(BSP_PB0);

#if BSP_PORTE_SUPPORT
    data.pe0 = bsp_descr_gpio_input_read(BSP_PE0);
    data.pe1 = bsp_descr_gpio_input_read(BSP_PE1);
#endif

#if CONFIG_CANIOT_FAKE_TEMPERATURE
    data.ext_temperature = caniot_fake_get_temp(dev);
    data.int_temperature = caniot_fake_get_temp(dev);
#else
    data.int_temperature  = get_t10_temperature(TEMP_SENS_INT);
    data.ext_temperature  = get_t10_temperature(TEMP_SENS_EXT_1);
    data.ext_temperature2 = get_t10_temperature(TEMP_SENS_EXT_2);
    data.ext_temperature3 = get_t10_temperature(TEMP_SENS_EXT_3);
#endif

    return caniot_blc1_telemetry_ser(&data, buf, len);
}

int class1_blc_command_handler(struct caniot_device *dev,
                               const unsigned char *buf,
                               uint8_t len)
{
    caniot_complex_digital_cmd_t xps;
    struct caniot_blc_sys_command sys_cmd  = {0};
    struct caniot_class1_config *const cfg = &dev->config->cls1_gpio;
    uint32_t directions                    = cfg->directions;
    uint32_t self_managed                  = cfg->self_managed;

    for (uint8_t i = 0u; i < CONFIG_IO_COUNT; i++) {
        /* Only apply state on output pins which are not self managed */
        if (!(self_managed & 1u) && (directions & 1u)) {
            caniot_blc1_cmd_parse_xps(&xps, buf, len, i);
            command_xps(&xps_ctx[i], xps, cfg->pulse_durations[i]);
        }

        self_managed >>= 1u;
        directions >>= 1u;
    }

    if (len == 8u) caniot_blc_sys_command_from_byte(&sys_cmd, buf[7u]);

    return dev_apply_blc_sys_command(dev, &sys_cmd);
}

int class1_config_apply(struct caniot_device *dev, bool init)
{
    struct caniot_device_config *const config = dev->config;

    for (uint8_t i = 0u; i < CONFIG_IO_COUNT; i++) {
        if (config->cls1_gpio.self_managed & BIT(i)) continue;

        const uint8_t output_default = (config->cls1_gpio.outputs_default >> i) & 1u;

        /* Set default on initialization */
        if (init)
            bsp_descr_gpio_pin_init(xps_ctx[i].descr,
                                    (config->cls1_gpio.directions >> i) & 1u,
                                    output_default);

        /* Apply default state for XPS */
        xps_ctx[i].reset_state = output_default;
    }

    return 0;
}

#endif /* CONFIG_CLASS1_ENABLED */