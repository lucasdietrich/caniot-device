/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ow_ds_drv.h"
#include "ow_ds_meas.h"
#include "tcn75.h"
#include "temp.h"

#include <caniot/datatype.h>

// 0x28 0xd4 0x39 0xb8 0x32 0x20 0x01 0xf2
// 0x28 0x2a 0x06 0x41 0x33 0x20 0x01 0x31
// 0x28 0x3d 0x72 0xbf 0x32 0x20 0x01 0x52

#define OW_DS_SN_NONE()                                                                  \
    {                                                                                    \
        .registered = 0U,                                                                \
    }

#define OW_DS_SN_REGISTER(sn_array)                                                      \
    {                                                                                    \
        .id =                                                                            \
            {                                                                            \
                .addr = {sn_array},                                                      \
            },                                                                           \
        .registered = 1U,                                                                \
    }

#if CONFIG_OW_DS_ENABLED
/* use serial numbers to order sensors */
ow_ds_sensor_t sensors[CONFIG_OW_DS_COUNT] = {
#if defined(CONFIG_OW_DS_SN_1) && (CONFIG_OW_DS_COUNT >= 1)
    OW_DS_SN_REGISTER(CONFIG_OW_DS_SN_1),
#elif (CONFIG_OW_DS_COUNT >= 1)
    OW_DS_SN_NONE(),
#endif

#if defined(CONFIG_OW_DS_SN_2) && (CONFIG_OW_DS_COUNT >= 2)
    OW_DS_SN_REGISTER(CONFIG_OW_DS_SN_2),
#elif (CONFIG_OW_DS_COUNT >= 2)
    OW_DS_SN_NONE(),
#endif

#if defined(CONFIG_OW_DS_SN_3) && (CONFIG_OW_DS_COUNT >= 3)
    OW_DS_SN_REGISTER(CONFIG_OW_DS_SN_3),
#elif (CONFIG_OW_DS_COUNT >= 3)
    OW_DS_SN_NONE(),
#endif
};
#endif

void temp_start(void)
{
#if CONFIG_OW_DS_ENABLED
    ds_init(sensors, ARRAY_SIZE(sensors));

    ds_discover();
    ds_measure_all();
    ds_meas_start(CONFIG_OW_DS_PROCESS_PERIOD_MS);
#endif
}

int16_t temp_read(temp_sens_t sensor)
{
    int16_t temp = CANIOT_DT_T16_INVALID;

    if (sensor == TEMP_SENS_INT) {
#if CONFIG_TCN75
        /**
         * @brief TCN75 is in continuous measurement mode, so
         * reading the temperature is "instantaneous".
         */
        temp = tcn75_read();
#endif
    } else if (CONFIG_OW_DS_ENABLED) {
#if CONFIG_OW_DS_ENABLED
        /* as threads are cooperative, we don't need
         * a mutex to protect temperature values
         */
        const uint8_t index = sensor - TEMP_SENS_EXT_1;
        if ((index < ARRAY_SIZE(sensors)) && sensors[index].valid) {
            temp = sensors[index].temp;
        }
#endif
    }

    return temp;
}

uint16_t get_t10_temperature(temp_sens_t sens)
{
    uint16_t temp10 = CANIOT_DT_T10_INVALID;

    const int16_t temp16 = temp_read(sens);
    if (temp16 != CANIOT_DT_T16_INVALID) {
        temp10 = caniot_dt_T16_to_T10(temp16);
    }

    return temp10;
}
