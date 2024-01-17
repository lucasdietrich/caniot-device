/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "class/class.h"
#include "config.h"
#include "settings.h"
#include "dev.h"
#include "utils/crc.h"

#include <stdint.h>
#include <time.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/eeprom.h>
#include <avr/io.h>
#include <caniot/caniot.h>
#include <caniot/device.h>

#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL

#define SETTINGS_CONFIG_SIZE sizeof(struct caniot_device_config)
/* Configuration structure size + 1 byte for the checksum */
#define SETTINGS_BLOCK_SIZE (SETTINGS_CONFIG_SIZE + 1u)

__STATIC_ASSERT(SETTINGS_BLOCK_SIZE*CONFIG_DEVICE_INSTANCES_COUNT <= E2END,
                "Not enough EEPROM space for the settings");

static uint16_t eeprom_config_offset(struct caniot_device *dev)
{
    return dev_get_instance_index(dev) * SETTINGS_BLOCK_SIZE;
}

/* Tell whether the initial configuration needs to be applied or not */
static bool init_config_to_apply = true;

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

    const void *config_base_offset = (void*) eeprom_config_offset(dev);
    const uint8_t actual_checksum  = eeprom_read_byte(config_base_offset);

    eeprom_read_block(dev->config, config_base_offset + 1u, SETTINGS_CONFIG_SIZE);

    uint8_t calculated_checksum =
        crc8((const uint8_t *)dev->config, SETTINGS_CONFIG_SIZE);

    if (actual_checksum != calculated_checksum) {
        return -EINVAL;
    }

    return 0;
}

int settings_write(struct caniot_device *dev)
{
    const uint16_t config_base_offset = eeprom_config_offset(dev);
    eeprom_update_block((const void *)dev->config,
                        (void *)(config_base_offset + 1u),
                        SETTINGS_CONFIG_SIZE);

    const uint8_t calculated_checksum =
        crc8((const uint8_t *)dev->config, SETTINGS_CONFIG_SIZE);

    /* Write checksum */
    eeprom_update_byte((uint8_t *)config_base_offset, calculated_checksum);

    return settings_apply(dev);
}

int settings_restore_default(struct caniot_device *dev,
                             const struct caniot_device_config *farp_default_config)
{
    memcpy_P(dev->config, farp_default_config, SETTINGS_CONFIG_SIZE);

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