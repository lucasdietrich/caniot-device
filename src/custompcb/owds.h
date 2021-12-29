/**
 * @file owds.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief OneWire driver for DS18B20 temperature sensor
 * @version 0.1
 * @date 2021-12-29
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// ROM = 28 bc 49 9c 32 20 1 83
//   Chip = DS18B20  Data = 173 1 4b 46 7f ff c 10 85  CRC= 85
//   Temperature = 23.19 °C
// No more addresses.

#ifndef _OW_DS_H_
#define _OW_DS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* PB1 */
#define OW_ARDUINO_PIN	9

void ll_ow_ds_init(void);

void ow_ds_read(void);

#ifdef __cplusplus
}
#endif

#endif /* _OW_DS_H_ */