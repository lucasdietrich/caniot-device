/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_SETTINGS_H_
#define _CANIOT_SETTINGS_H_

#include <caniot/device.h>

/**
 * @brief Initialize the settings module.
 *
 * @param dev
 */
void settings_init(struct caniot_device *dev,
                   const struct caniot_device_config *farp_default_config);

/**
 * @brief Read the settings from the EEPROM and copy them to the config struct.
 *
 * @param dev
 * @param config (is actually dev->config)
 * @return int
 */
int settings_read(struct caniot_device *dev);

/**
 * @brief Write the settings from the config struct to the EEPROM.
 *
 * @param dev
 * @param config (is actually dev->config)
 * @return int
 */
int settings_write(struct caniot_device *dev);

/**
 * @brief Restore the default settings for the device.
 *
 * @param dev
 * @return int
 */
int settings_restore_default(struct caniot_device *dev,
                             const struct caniot_device_config *farp_default_config);

#endif /* _CANIOT_SETTINGS_H_ */