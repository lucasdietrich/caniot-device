/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "class/class.h"
#include "config.h"
#include "settings.h"

#include <stdint.h>
#include <time.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/eeprom.h>
#include <caniot/caniot.h>
#include <caniot/device.h>

#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL

#define SETTINGS_BLOCK_SIZE sizeof(struct caniot_device_config)

/* Tell whether the initial configuration needs to be applied or not */
static bool init_config_to_apply = true;

// compute CRC8
static uint8_t checksum_crc8(const uint8_t *buf, size_t len)
{
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *buf++;
        uint8_t i;

        for (i = 0x80; i > 0; i >>= 1) {
            uint8_t mix = (crc ^ inbyte) & i;
            crc         = (crc >> 1) ^ (mix ? 0x8C : 0x00);
        }
    }

    return crc;
}

static int settings_apply(struct caniot_device *dev)
{
    set_zone(dev->config->timezone);

    switch (__DEVICE_CLS__) { /* TODO get device class dynamically */
#if CONFIG_CLASS0_ENABLED
    case CANIOT_DEVICE_CLASS0:
        return class0_config_apply(dev, init_config_to_apply);
#endif
#if CONFIG_CLASS1_ENABLED
    case CANIOT_DEVICE_CLASS1:
        return class1_config_apply(dev, init_config_to_apply);
#endif
    default:
        return -CANIOT_ENOTSUP;
    }
}

int settings_read(struct caniot_device *dev)
{
    (void)dev;

    /* TODO calculate base offset for the device */
    const void *config_base_offset = 0x0000u;
    const uint8_t actual_checksum  = eeprom_read_byte(config_base_offset);

    eeprom_read_block(dev->config, config_base_offset + 1u, SETTINGS_BLOCK_SIZE);

    uint8_t calculated_checksum =
        checksum_crc8((const uint8_t *)dev->config, SETTINGS_BLOCK_SIZE);

    if (actual_checksum != calculated_checksum) {
        return -EINVAL;
    }

    return 0;
}

int settings_write(struct caniot_device *dev)
{
    /* TODO calculate base offset for the device */
    const uint16_t config_base_offset = 0x0000u;
    eeprom_update_block((const void *)dev->config,
                        (void *)(config_base_offset + 1u),
                        SETTINGS_BLOCK_SIZE);

    const uint8_t calculated_checksum =
        checksum_crc8((const uint8_t *)dev->config, SETTINGS_BLOCK_SIZE);

    /* Write checksum */
    eeprom_update_byte((uint8_t *)config_base_offset, calculated_checksum);

    return settings_apply(dev);
}

int settings_restore_default(struct caniot_device *dev,
                             const struct caniot_device_config *farp_default_config)
{
    memcpy_P(dev->config, farp_default_config, SETTINGS_BLOCK_SIZE);

    return settings_write(dev);
}

#if CONFIG_FORCE_RESTORE_DEFAULT_CONFIG
#warning "CONFIG_FORCE_RESTORE_DEFAULT_CONFIG" is enabled
#endif

void settings_init(struct caniot_device *dev,
                   const struct caniot_device_config *farp_default_config)
{
    bool restore = false;

    if (CONFIG_FORCE_RESTORE_DEFAULT_CONFIG == 0) {
        /* sanity check on EEPROM */
        if (settings_read(dev) != 0) {
            restore = true;
        }
    }

    /* if restore is true, we copy the default configuration to EEPROM and RAM */
    if (restore || (CONFIG_FORCE_RESTORE_DEFAULT_CONFIG == 1)) {

        LOG_DBG("Config reset ...");
        settings_restore_default(dev, farp_default_config);
    } else {
        settings_apply(dev);
    }

    /* the initial configuration has been applied */
    init_config_to_apply = false;
}