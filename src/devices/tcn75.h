/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TCN75_H
#define _TCN75_H

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

// TCN75 freq 0 (min) > 100 (typ) > 400 (max) kHz

/**
 * @brief CONFIG_TCN75_A2A1A0 is the address configuration of the TCN75.
 *
 * Bits:
 *  	0: A0
 * 	1: A1
 * 	2: A2
 */
#ifndef CONFIG_TCN75_A2A1A0
#define CONFIG_TCN75_A2A1A0 0x1u
#endif

#define TCN75_A2A1A0 (CONFIG_TCN75_A2A1A0 & 0x7u)
#define TCN75_ADDR   ((uint8_t)(0b1001000 | TCN75_A2A1A0))

#define TCN75_TEMPERATURE_REGISTER 0U
#define TCN75_CONFIG_REGISTER      1U
#define TCN75_HISTERESYS_REGISTER  2U
#define TCN75_SETPONT_REGISTER     3U

#define TCN75_SHUTDOWN_BIT     0U
#define TCN75_NORMAL_OPERATION (0U << TCN75_SHUTDOWN_BIT)
#define TCN75_SHUTDOWN_MODE    (1U << TCN75_SHUTDOWN_BIT)

#define TCN75_COMPINT_BIT     1U
#define TCN75_COMPARATOR_MODE (0U << TCN75_COMPINT_BIT)
#define TCN75_INTERRUPT_MODE  (1U << TCN75_COMPINT_BIT)

#define TCN75_COMPINT_POLARITY_BIT         2U
#define TCN75_COMPINT_POLARITY_ACTIVE_LOW  (0U << TCN75_COMPINT_POLARITY_BIT)
#define TCN75_COMPINT_POLARITY_ACTIVE_HIGH (1U << TCN75_COMPINT_POLARITY_BIT)

#define TCN75_FAULT_QUEUE_BIT             3U
#define TCN75_FAULT_QUEUE_NB_CONVERSION_1 (0b00 << TCN75_FAULT_QUEUE_BIT)
#define TCN75_FAULT_QUEUE_NB_CONVERSION_2 (0b01 << TCN75_FAULT_QUEUE_BIT)
#define TCN75_FAULT_QUEUE_NB_CONVERSION_4 (0b10 << TCN75_FAULT_QUEUE_BIT)
#define TCN75_FAULT_QUEUE_NB_CONVERSION_6 (0b11 << TCN75_FAULT_QUEUE_BIT)

#define TCN75_RESOLUTION_BIT   5U
#define TCN75_RESOLUTION_9BIT  (0b00 << TCN75_RESOLUTION_BIT)
#define TCN75_RESOLUTION_10BIT (0b01 << TCN75_RESOLUTION_BIT)
#define TCN75_RESOLUTION_11BIT (0b10 << TCN75_RESOLUTION_BIT)
#define TCN75_RESOLUTION_12BIT (0b11 << TCN75_RESOLUTION_BIT)

#define TCN75_ONESHOT_BIT 7U
#define TCN75_CONTINUOUS  (0U << TCN75_ONESHOT_BIT)
#define TCN75_ONESHOT     (1U << TCN75_ONESHOT_BIT)

void tcn75_init(void);

int16_t tcn75_read(void);

float tcn75_temp2float(uint8_t msb, uint8_t lsb);

int16_t tcn75_temp2int16(uint8_t msb, uint8_t lsb);

float tcn75_int16tofloat(int16_t t);

#ifdef __cplusplus
}
#endif

#endif /* _TCN75_H */