/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tcn75.h"
#include "bsp/bsp.h"

#include <avrtos/avrtos.h>
#include <avrtos/drivers/i2c.h>

#define K_MODULE_TCN75 0x23
#define K_MODULE       K_MODULE_TCN75

#include <avrtos/logging.h>
#if defined(CONFIG_TCN75_LOG_LEVEL)
#define LOG_LEVEL CONFIG_TCN75_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#define TCN75_ALWAYS_SELECT_DATA_REGISTER 0x00

static void tcn75_configure(void)
{
    const uint8_t value = TCN75_NORMAL_OPERATION | TCN75_COMPARATOR_MODE |
                          TCN75_COMPINT_POLARITY_ACTIVE_LOW |
                          TCN75_FAULT_QUEUE_NB_CONVERSION_6 | TCN75_RESOLUTION_12BIT |
                          TCN75_CONTINUOUS;

    const uint8_t buf[2u] = {TCN75_CONFIG_REGISTER, value};
    i2c_master_transmit(BSP_I2C, TCN75_ADDR, buf, 2u);
}

static void tcn75_select_data_register(void)
{
    __ASSERT_INTERRUPT();

    const uint8_t buf[1u] = {TCN75_TEMPERATURE_REGISTER};
    i2c_master_transmit(BSP_I2C, TCN75_ADDR, buf, 1u);
}

void tcn75_init(void)
{
    __ASSERT_INTERRUPT();

    tcn75_configure();

#if !TCN75_ALWAYS_SELECT_DATA_REGISTER
    tcn75_select_data_register();
#endif
}

int16_t tcn75_read(void)
{
    __ASSERT_INTERRUPT();

    int16_t temperature = INT16_MAX;

#if TCN75_ALWAYS_SELECT_DATA_REGISTER
    tcn75_select_data_register();
#endif

    uint8_t buf[2u];
    int8_t ret = i2c_master_receive(BSP_I2C, TCN75_ADDR, buf, 2u);
    
    if (ret == 0u) {
        temperature = tcn75_temp2int16(buf[0u], buf[1u]);
        LOG_DBG("TCN75 read: %d", temperature);
    } else {
        LOG_ERR("TCN75 read error");
    }

    return temperature;
}

/* works for all 9, 10, 11 or 12 bits conversion */
float tcn75_temp2float(uint8_t msb, uint8_t lsb)
{
    float f_temp;

    const uint8_t neg = msb >> 7u;

    /* Resolution of abs is 2^-4 °C */
    uint16_t abs = (msb << 4u) | (lsb >> 4u);
    if (neg) { /* 2s complement if negative value */
        abs = ~abs + 1u;
    }
    /* cast to 12 bits value */
    abs &= 0x7ffu;

    /* i16_temp resolution is 0.01°C */
    f_temp = abs / 16.0;

    if (neg) {
        f_temp = -f_temp;
    }

    return f_temp;
}

int16_t tcn75_temp2int16(uint8_t msb, uint8_t lsb)
{
    int16_t i16_temp;

    const uint8_t neg = msb >> 7u;

    /* Resolution of abs is 2^-4 °C */
    uint16_t abs = (msb << 4u) | (lsb >> 4u);
    if (neg) { /* 2s complement if negative value */
        abs = ~abs + 1u;
    }
    /* cast to 12 bits value */
    abs &= 0x7ffu;

    /* i16_temp resolution is 0.01°C */
    i16_temp = (100.0 / 16) * abs;

    if (neg) {
        i16_temp = -i16_temp;
    }

    return i16_temp;
}

float tcn75_int16tofloat(int16_t t)
{
    return t / 100.0;
}