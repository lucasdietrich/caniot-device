#ifndef _MULTI_H_
#define _MULTI_H_

#include "dev.h"

#include <stdio.h>

#include <avrtos/avrtos.h>

#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

#define CLS1_CONFIG                                                                      \
    .cls1_gpio = {                                                                       \
        .self_managed = 0xFFFFFFFFlu,                                                    \
        .pulse_durations =                                                               \
            {                                                                            \
                [0] = 0u,  [1] = 0u,  [2] = 0u,  [3] = 0u,  [4] = 0u,                    \
                [5] = 0u,  [6] = 0u,  [7] = 0u,  [8] = 0u,  [9] = 0u,                    \
                [10] = 0u, [11] = 0u, [12] = 0u, [13] = 0u, [14] = 0u,                   \
                [15] = 0u, [16] = 0u, [17] = 0u, [18] = 0u,                              \
            },                                                                           \
        .outputs_default     = 0x00000000lu,                                             \
        .directions          = 0x00000000lu,                                             \
        .telemetry_on_change = 0x00000000lu,                                             \
    }

#define CLS0_CONFIG                                                                      \
    .cls0_gpio = {                                                                       \
        .pulse_durations     = {0u, 0u, 0u, 0u},                                         \
        .telemetry_on_change = 0u,                                                       \
        .outputs_default     = 0u,                                                       \
    }

#if defined(CONFIG_BOARD_V1)
#define CLS_CONFIG CLS0_CONFIG
#elif defined(CONFIG_BOARD_TINY_REVA)
#define CLS_CONFIG CLS1_CONFIG
#endif

#define DEFAULT_CONFIG()                                                                 \
    {                                                                                    \
        .telemetry =                                                                     \
            {                                                                            \
                .period    = 10000u,                                                     \
                .delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT_MS,                      \
                .delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT_MS,                      \
            },                                                                           \
        .flags =                                                                         \
            {                                                                            \
                .error_response             = 1u,                                        \
                .telemetry_delay_rdm        = 0u,                                        \
                .telemetry_endpoint         = CANIOT_ENDPOINT_BOARD_CONTROL,             \
                .telemetry_periodic_enabled = 1u,                                        \
            },                                                                           \
        .timezone = CANIOT_TIMEZONE_DEFAULT,                                             \
        .location =                                                                      \
            {                                                                            \
                .region  = CANIOT_LOCATION_REGION_DEFAULT,                               \
                .country = CANIOT_LOCATION_COUNTRY_DEFAULT,                              \
            },                                                                           \
        CLS_CONFIG,                                                                      \
    }

#endif /* _MULTI_H_ */