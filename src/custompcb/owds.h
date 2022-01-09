/**
 * @file owds.h
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

#ifndef _OW_DS_H_
#define _OW_DS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PB1 */
#define OW_ARDUINO_PIN	9

bool ll_ow_ds_init(void);

bool ow_ds_read(int16_t *raw);

float ow_ds_raw2float(int16_t raw);

int16_t ow_ds_raw_to_T16(int16_t raw);

#ifdef __cplusplus
}
#endif

#endif /* _OW_DS_H_ */