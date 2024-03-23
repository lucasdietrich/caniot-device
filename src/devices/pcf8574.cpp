/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"
#include "pcf8574.h"
#include "bsp/bsp.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>
#include <avrtos/drivers/i2c.h>

#if CONFIG_PCF8574_ENABLED

#define LOG_LEVEL CONFIG_PCF8574_LOG_LEVEL

void pcf8574_init(struct pcf8574_state *pcf, uint8_t i2c_addr)
{
    pcf->i2c_address = i2c_addr;
#if CONFIG_PCF8574_BUFFERED_READ
    pcf->read_buffer_valid = 0u;
#endif
#if CONFIG_PCF8574_BUFFERED_WRITE
    pcf->write_buffer = 0u;
#endif
}

void pcf8574_set(struct pcf8574_state *pcf, uint8_t value)
{
    size_t w;

#if CONFIG_PCF8574_BUFFERED_WRITE
    if (pcf->write_buffer == value) return;
#endif

    const uint8_t buf[1u] = {value};
    int8_t res = i2c_master_write(BSP_I2C, pcf->i2c_address, buf, 1u);

    LOG_DBG("PCF8574 I2C w x%02x ok: %d", value, res);

#if CONFIG_PCF8574_BUFFERED_WRITE
    if (w != 0) pcf->write_buffer = value;
#else
    (void)w;
#endif
}

uint8_t pcf8574_get(struct pcf8574_state *pcf)
{
#if CONFIG_PCF8574_BUFFERED_READ
    if (pcf->read_buffer_valid) return pcf->read_buffer;
#endif

    uint8_t value = 0u;
    int8_t res = i2c_master_read(BSP_I2C, pcf->i2c_address, &value, 1u);

    LOG_DBG("PCF8574 I2C r x%02X ret: %d", value, res);

#if CONFIG_PCF8574_BUFFERED_READ
    pcf->read_buffer       = value;
    pcf->read_buffer_valid = res == 0;
#endif

    return value;
}

#endif