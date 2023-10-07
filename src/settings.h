/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_SETTINGS_H_
#define _CANIOT_SETTINGS_H_

void settings_load(struct caniot_device *dev);

int settings_read(struct caniot_device *dev, struct caniot_device_config *config);

int settings_write(struct caniot_device *dev, struct caniot_device_config *config);

int settings_restore_default(struct caniot_device *dev);

#endif /* _CANIOT_SETTINGS_H_ */