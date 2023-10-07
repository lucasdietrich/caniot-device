/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Pulse API for generating simple pulses on output pins.
 * 	- Thread-safe. if CONFIG_GPIO_PULSE_THREAD_SAFE is set to 1
 */

#ifndef _GPIO_PULSE_H_
#define _GPIO_PULSE_H_

#include "bsp/bsp.h"

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_GPIO_PULSE_THREAD_SAFE 1u

struct pulse_event {
    struct titem _tie;

    /* State of the pin when not active */
    uint8_t reset_state : 1;

    /* Tells wether the pulse is active or not */
    uint8_t scheduled : 1;

    /* Tells wether the pulse context has been allocated internally */
    uint8_t _iallocated;

    /* GPIO descriptor */
    pin_descr_t descr;
};

/**
 * @brief Init pulse module
 */
void pulse_init(void);

/**
 * @brief Trigger a pulse on an output
 *
 * @param output RL1, RL2, OC1, OC2
 * @param state true, false
 * @param duration_ms Duration of the pulse in ms
 * @param pe Pointer to a pulse_event struct. If NULL, a new one will be allocated
 */
struct pulse_event *pulse_trigger(pin_descr_t descr,
                                  bool state,
                                  uint32_t duration_ms,
                                  struct pulse_event *ev);

/**
 * @brief Cancel a pulse being process and set pin state to state value
 *
 * @param output RL1, RL2, OC1, OC2
 * @param state true, false
 */
void pulse_cancel(struct pulse_event *ev);

/**
 * @brief Check if a pulse is being processed for the pin
 *
 * @param output RL1, RL2, OC1, OC2
 */
bool pulse_is_active(struct pulse_event *ev);

/**
 * @brief Handle pulses if any
 *
 * @param time_passed_ms
 * @return true if at least one pulse was processed
 */
bool pulse_process(uint32_t time_passed_ms);

/**
 * @brief Get time until next event
 *
 * @return uint32_t
 */
uint32_t pulse_remaining(void);

#endif /* _GPIO_PULSE_H_ */