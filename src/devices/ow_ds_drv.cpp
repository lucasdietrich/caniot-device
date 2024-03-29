/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Based on this sample from Collin Kidder:
 *   https://github.com/collin80/DS2480B/blob/master/examples/DS18x20_Temperature/DS18x20_Temperature.ino
 *
 * Other references:
 *   https://www.pjrc.com/teensy/td_libs_OneWire.html
 *   https://github.com/zephyrproject-rtos/zephyr/blob/69468dc52b8e1f4de06e1854d369f3af02107ecc/drivers/sensor/ds18b20/ds18b20.c
 */

#include "../bsp/bsp.h"
#include "ow_ds_drv.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <OneWire.h>
#if defined(CONFIG_OW_LOG_LEVEL)
#define LOG_LEVEL CONFIG_OW_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

struct ow_dev {
    OneWire ow;
};

static struct ow_dev dev = {.ow = OneWire()};

void ow_ds_drv_init(uint8_t pin)
{
    dev.ow.begin(pin);
}

static inline int8_t handle_addr(uint8_t *addr, uint8_t *type)
{
    if (OneWire::crc8(addr, 7U) != addr[7U]) {
        return -OW_DS_DRV_CRC_ERROR;
    }

    // print ROM
    LOG_DBG("ROM: ");
    LOG_HEXDUMP_DBG(addr, 8U);

    // the first ROM byte indicates which chip
    switch (addr[0]) {
    case 0x10:
        LOG_DBG(": DS18S20");
        *type = 1;
        break;
    case 0x28:
        LOG_DBG(": DS18B20");
        *type = 0;
        break;
    case 0x22:
        LOG_DBG(": DS1822");
        *type = 0;
        break;
    default:
        LOG_WRN(": DS ??");
        return -OW_DS_DRV_UNKOWN_DEVICE;
    }

    return OW_DS_DRV_SUCCESS;
}

int8_t ow_ds_drv_discover_iter(uint8_t max_devices,
                               bool (*discovered_cb)(ow_ds_id_t *, void *),
                               void *user_data)
{
    if (!discovered_cb || !max_devices) {
        return -OW_DS_DRV_INVALID_PARAMS;
    }

    dev.ow.reset_search();

    ow_ds_id_t id;
    uint8_t count = 0U;

    while (dev.ow.search(id.addr, true) && (count < max_devices)) {
        if (handle_addr(id.addr, &id.type) == OW_DS_DRV_SUCCESS) {
            if (discovered_cb(&id, user_data)) {
                count++;
            }
        }
    }

    return count;
}

static bool array_iter_cb(ow_ds_id_t *id, void *user_data)
{
    ow_ds_id_t **ppid = (ow_ds_id_t **)user_data;

    memcpy(*ppid, id, sizeof(ow_ds_id_t));

    (*ppid)++;

    return true;
}

int8_t ow_ds_drv_discover(ow_ds_id_t *array, uint8_t size)
{
    int8_t ret = -OW_DS_DRV_INVALID_PARAMS;

    if (array != NULL) {
        ret = ow_ds_drv_discover_iter(size, array_iter_cb, &array);
    }

    return ret;
}

static inline int16_t raw_to_T16(int16_t raw)
{
    return (100LU * ((int32_t)raw)) / 16;
}

int8_t ow_ds_drv_read_start(ow_ds_id_t *id)
{
    if (!id) {
        return -OW_DS_DRV_INVALID_PARAMS;
    }

    if (dev.ow.reset() != 1U) {
        return -OW_DS_DRV_NO_DEVICES;
    }

    dev.ow.select(id->addr);
    dev.ow.write(0x44, 1); // start conversion, with parasite power on at the end

    return OW_DS_DRV_SUCCESS;
}

int8_t ow_ds_drv_read_handle_result(ow_ds_id_t *id, int16_t *temperature)
{
    if (!id || !temperature) {
        return -OW_DS_DRV_INVALID_PARAMS;
    }

    // we might do a ds.depower() here, but the reset will take care of it.

    if (dev.ow.reset() != 1U) {
        return -OW_DS_DRV_NO_DEVICES;
    }

    dev.ow.select(id->addr);
    dev.ow.write(0xBE); // Read Scratchpad

    // we need 9 bytes
    uint8_t data[9];
    byte any = 0x00U;
    for (int i = 0; i < 9; i++) {
        data[i] = dev.ow.read();
        any |= data[i];
    }

    /* if all bytes are null, then the read failed */
    if (any == 0x00U) {
        return -OW_DS_DRV_NULL_DATA;
    }

    // Check CRC
    if (OneWire::crc8(data, 8) != data[8]) {
        return -OW_DS_DRV_CRC_ERROR;
    }

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t tmp = (data[1] << 8) | data[0];
    if (id->type) {
        tmp = tmp << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            tmp = (tmp & 0xFFF0) + 12 - data[6];
        }
    } else {
        uint8_t res = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (res == 0x00)
            tmp = tmp & ~7; // 9 bit resolution, 93.75 ms
        else if (res == 0x20)
            tmp = tmp & ~3; // 10 bit res, 187.5 ms
        else if (res == 0x40)
            tmp = tmp & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time

        LOG_DBG("res: %hhx", res);
    }

    /* TODO handle the case were the resolution is not 12 bits, in which case
     * we need to write the scratchpad */

    *temperature = raw_to_T16(tmp);

    return OW_DS_DRV_SUCCESS;
}

int8_t ow_ds_drv_read(ow_ds_id_t *id, int16_t *temperature)
{
    int8_t ret;

    ret = ow_ds_drv_read_start(id);
    if (ret != OW_DS_DRV_SUCCESS) {
        goto exit;
    }

    k_sleep(K_MSEC(OW_DS_MEAS_DURATION_MS)); // maybe 750ms is enough, maybe not

    ret = ow_ds_drv_read_handle_result(id, temperature);

exit:
    return ret;
}