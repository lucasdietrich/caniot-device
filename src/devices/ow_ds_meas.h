/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OW_DS_MEAS_H_
#define _OW_DS_MEAS_H_

#include <stdbool.h>
#include <stdint.h>

#include <avrtos/avrtos.h>

#include <devices/ow_ds_drv.h>

// Max errors before trying to discover again
#define OW_DS_MAX_CONSECUTIVE_ERRORS 4U

// Number of measurements for each sensor before triggering a discovery
// default 10U
#define OW_DS_DISCOVERIES_PERIODICITY 5U

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /**
     * @brief Sensor address and type
     */
    ow_ds_id_t id;

    /**
     * @brief Temperature in 1e-2 °C
     */
    int16_t temp;

    /**
     * @brief Indicates if sensor id is active
     */
    uint8_t registered : 1;

    /**
     * @brief Indicates if sensor id has been discovered
     *   Only active/discovered sensor can be measured
     */
    uint8_t active : 1;

    /**
     * @brief Indicates if temperature is valid
     */
    uint8_t valid : 1;

    /**
     * @brief Indicates how many errors occured for this sensor
     */
    uint8_t errors : 4;

    /**
     * @brief Indicates if a measurement is in progress for the current sensor
     */
    uint8_t in_progress : 1;
} ow_ds_sensor_t;

/**
 * @brief Initialize the context, reference sensors array
 *
 * @param array
 * @param count Number of sensors expected
 * @return int8_t
 */
int8_t ds_init(ow_ds_sensor_t *array, uint8_t count);

/**
 * @brief Starts the periodic measurements process
 *
 * @param period_ms period between two measurements in milliseconds
 * @return int8_t
 */
int8_t ds_meas_start(uint16_t period_ms);

/**
 * @brief Stops the periodic measurements process
 *
 * @return int8_t
 */
int8_t ds_meas_stop(void);

/**
 * @brief Indicates whether the periodic measurements process is running
 *
 * @return true
 * @return false
 */
bool ds_meas_running(void);

/**
 * @brief Do a single discovery outside the context of the periodic measurements process
 *
 * @return int8_t Sensors found or error
 */
int8_t ds_discover(void);

/**
 * @brief Measure the temperature of all the sensor active sensors
 *   (outside the context of the periodic measurements process)
 *
 * @return int8_t Measurements retrieved or error
 */
int8_t ds_measure_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _OW_DS_MEAS_H_ */