/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* PCC: Phase Crossing Counter */

#ifndef _HEATING_CONTROLLER_PHEASE_CROSSING_COUNTER_H_
#define _HEATING_CONTROLLER_PHEASE_CROSSING_COUNTER_H_

#include <stdint.h>

/**
 * @brief Initialize the phease crossing counter.
 */
void pcc_init(void);

/**
 * @brief Get the current calculated frequency.
 *
 * @return uint8_t
 */
uint8_t pcc_get_get_frequency(void);

/**
 * @brief Get whether the power is ok or not.
 *
 * @return uint8_t
 */
bool pcc_get_power_status(void);

#endif /* _HEATING_CONTROLLER_PHEASE_CROSSING_COUNTER_H_ */