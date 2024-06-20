/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp/bsp.h"
#include "multi.h"

#include <stdio.h>

#include <avrtos/avrtos.h>
#include <avrtos/drivers/timer.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

void app_init(void)
{
}

int app_command_handler(struct caniot_device *dev,
                        caniot_endpoint_t ep,
                        char *buf,
                        uint8_t len)
{
    return 0;
}

int app_telemetry_handler(struct caniot_device *dev,
                          caniot_endpoint_t ep,
                          char *buf,
                          uint8_t *len)
{
    return 0;
}

const struct caniot_device_config default_config[CONFIG_DEVICE_INSTANCES_COUNT] PROGMEM =
    {
        [0] =
            {
                .telemetry =
                    {
                        .period    = 10000u,
                        .delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT_MS,
                        .delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT_MS,
                    },
                .flags =
                    {
                        .error_response             = 1u,
                        .telemetry_delay_rdm        = 0u,
                        .telemetry_endpoint         = CANIOT_ENDPOINT_BOARD_CONTROL,
                        .telemetry_periodic_enabled = 1u,
                    },
                .timezone = CANIOT_TIMEZONE_DEFAULT,
                .location =
                    {
                        .region  = CANIOT_LOCATION_REGION_DEFAULT,
                        .country = CANIOT_LOCATION_COUNTRY_DEFAULT,
                    },
#if defined(CONFIG_BOARD_V1)
                .cls0_gpio =
                    {
                        .pulse_durations     = {0u, 0u, 0u, 0u},
                        .telemetry_on_change = 0u,
                        .outputs_default     = 0u,
                    },
#elif defined(CONFIG_BOARD_TINY_REVA)
                .cls1_gpio =
                    {
                        .self_managed = 0x00000000lu,
                        .pulse_durations =
                            {
                                [0] = 0x0u,     [1] = 0x100u,   [2] = 0x200u,
                                [3] = 0x300u,   [4] = 0x400u,   [5] = 0x500u,
                                [6] = 0x600u,   [7] = 0x700u,   [8] = 0x800u,
                                [9] = 0x900u,   [10] = 0x1000u, [11] = 0x1100u,
                                [12] = 0x1200u, [13] = 0x1300u, [14] = 0x1400u,
                                [15] = 0x1500u, [16] = 0x1600u, [17] = 0x1700u,
                                [18] = 0x1800u,
                            },
                        .outputs_default     = 0x0007fffflu, /* state high */
                        .directions          = 0x0007fffflu, /* outputs*/
                        .telemetry_on_change = 0x0,          /* No Change */
                    },
#endif
            },
        [1] = DEFAULT_CONFIG(),
        [2] = DEFAULT_CONFIG(),
        [3] = DEFAULT_CONFIG(),
        [4] = DEFAULT_CONFIG(),
        [5] = DEFAULT_CONFIG(),
        // [6] = DEFAULT_CONFIG(),
        // [7] = DEFAULT_CONFIG(),
};