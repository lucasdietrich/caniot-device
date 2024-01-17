/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "features.h"

#if CONFIG_FEATURES

static const struct feature features[] PROGMEM = {
#if CONFIG_WATCHDOG
    FEATURE(FEATURE_WATCHDOG),
#endif
#if CONFIG_CAN_CLOCKSET_16MHZ
    FEATURE(FEATURE_CAN_CLOCKSET),
#endif
};

const char *feature_type_to_string(feature_type_t type)
{
    switch (type)
    {
    case FEATURE_WATCHDOG:
        return "Watchdog";
    case FEATURE_CAN_CLOCKSET:
        return "CAN Clockset 16Mhz";
    case FEATURE_TYPE_NONE:
    default:
        return "unknown";
    }
}

struct feature *feature_get(feature_type_t id)
{

}

void feature_iterate(feature_callback_t callback)
{

}

#endif /* CONFIG_FEATURES */