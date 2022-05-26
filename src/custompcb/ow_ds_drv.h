/**
 * @file ow_ds_drv.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief OneWire driver for DS18B20 temperature sensor
 * @version 0.1
 * @date 2021-12-29
 * 
 * @copyright Copyright (c) 2021
 * 
 * - https://www.pjrc.com/teensy/td_libs_OneWire.html
 * 
 */

// ROM = 28 bc 49 9c 32 20 1 83
//   Chip = DS18B20  Data = 173 1 4b 46 7f ff c 10 85  CRC= 85
//   Temperature = 23.19 °C
// No more addresses.

#ifndef _OW_DS_DRV_H_
#define _OW_DS_DRV_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t addr[8];
	uint8_t type;
} ow_ds_id_t;

enum {
	OW_DS_DRV_SUCCESS = 0,
	OW_DS_DRV_INVALID_PARAMS,
	OW_DS_DRV_CRC_ERROR,
	OW_DS_DRV_UNKOWN_DEVICE,
	OW_DS_DRV_NO_DEVICES,
	OW_DS_DRV_NULL_DATA,
	OW_DS_DRV_TIMEOUT,
	OW_DS_DRV_PERIODIC_MEAS_STARTED,
	OW_DS_DRV_PERIODIC_MEAS_NOT_STARTED,
	OW_DS_DRV_SENS_INACTIVE,
	OW_DS_DRV_SENS_MEAS_FAILED,
	OW_DS_DRV_UNKNOWN_ERROR,
};

/**
 * @brief Initialize OneWire pin
 * 
 * @param pin
 */
void ow_ds_drv_init(uint8_t pin);

/**
 * @brief Discover DS devices on the one wire "bus", 
 *       and call callback for each device found
 * 
 * @param discovered_cb 
 * @return int8_t 
 */
int8_t ow_ds_drv_discover_iter(uint8_t max_devices,
			       void (*discovered_cb)(ow_ds_id_t *, void *),
			       void *user_data);

/**
 * @brief Discover DS devices on the one wire "bus", and store the addresses in the array
 *
 * @param array Empty array of ow_ds_id_t
 * @param size Size of the array
 * @return int8_t Number of devices found, negative value on error
 */
int8_t ow_ds_drv_discover(ow_ds_id_t *array, uint8_t size);

/**
 * @brief Read temperature from the given DS temperature sensor
 *
 * @param id Device address
 * @param temperature Variable to store the temperature in (in 1e-2 °C)
 * @return int8_t 0 on success, negative value on error
 */
int8_t ow_ds_drv_read(ow_ds_id_t *id, int16_t *temperature);

#ifdef __cplusplus
}
#endif

#endif /* _OW_DS_DRV_H_ */