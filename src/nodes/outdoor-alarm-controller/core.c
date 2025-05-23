/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp/bsp.h"
#include "class/class.h"

#include <stdio.h>

#include <avrtos/avrtos.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <dev.h>

#if CONFIG_GPIO_PULSE_SUPPORT != 1
#error "CONFIG_GPIO_PULSE_SUPPORT must be enabled"
#endif

#define OUTDOOR_LIGHT_1 OC1_IDX
#define OUTDOOR_LIGHT_2 OC2_IDX
#define SIREN           RL1_IDX

#define PRESENCE_SENSOR_1 IN1_IDX
#define PRESENCE_SENSOR_2 IN2_IDX
#define SABOTAGE          IN4_IDX

int app_telemetry_handler(struct caniot_device *dev,
                          caniot_endpoint_t ep,
                          const char *buf,
                          uint8_t *len)
{
    return -CANIOT_ENIMPL;
}

int app_command_handler(struct caniot_device *dev,
                        caniot_endpoint_t ep,
                        char *buf,
                        uint8_t len)
{
    return -CANIOT_ENIMPL;
}

const struct caniot_device_config default_config PROGMEM = {
    .telemetry =
        {
            .period    = CANIOT_TELEMETRY_PERIOD_DEFAULT_MS,
            .delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT_MS,
            .delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT_MS,
        },
    .flags =
        {
            .error_response             = 1u,
            .telemetry_delay_rdm        = 1u,
            .telemetry_endpoint         = CANIOT_ENDPOINT_BOARD_CONTROL,
            .telemetry_periodic_enabled = 1u,
        },
    .timezone = CANIOT_TIMEZONE_DEFAULT,
    .location =
        {
            .region  = CANIOT_LOCATION_REGION_DEFAULT,
            .country = CANIOT_LOCATION_COUNTRY_DEFAULT,
        },
    .cls0_gpio =
        {
            .pulse_durations =
                {
                    [OUTDOOR_LIGHT_1] = 30000u,
                    [OUTDOOR_LIGHT_2] = 30000u,
                    [SIREN]           = 20000u,
                },
            .outputs_default     = 0u,
            .telemetry_on_change = BIT(OC1_IDX) | BIT(OC2_IDX) | BIT(RL1_IDX) |
                                   BIT(IN1_IDX) | BIT(IN2_IDX) | BIT(IN3_IDX) |
                                   BIT(IN4_IDX), /* OC1, OC2, RL1, IN1 -> IN4 */
        },
};