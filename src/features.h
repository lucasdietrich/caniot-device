/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DEV_FEATURES_H_
#define _CANIOT_DEV_FEATURES_H_

#include <stdint.h>

#include <avr/pgmspace.h>

#include <avrtos/kernel.h>

#define FEATURE_VAL(_id, _val)                                                    \
    {                                                                                    \
        _id, _val                                                                 \
    }

#define FEATURE(_id) FEATURE_VAL(_id, FEATURE_VAL_NONE)

#define FEATURE_VAL_NONE 0xFF

typedef enum {
    FEATURE_TYPE_NONE,
    FEATURE_WATCHDOG,
    FEATURE_CAN_CLOCKSET,
} feature_type_t;

struct feature {
    feature_type_t id;
    uint8_t val;
    const char *farp_name;
};

const char *feature_type_to_string(feature_type_t type);

struct feature *feature_get(feature_type_t id);

typedef void (*feature_callback_t)(struct feature *feature);

void feature_iterate(feature_callback_t callback);

#endif /* _CANIOT_DEV_FEATURES_H_ */