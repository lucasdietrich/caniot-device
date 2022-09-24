/**
 * @file pulse.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Pulse API for generating simple pulses on output pins.
 * 	- Thread-safe.
 * @version 0.1
 * @date 2022-05-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _CUSTOMPCB_PULSE_H_
#define _CUSTOMPCB_PULSE_H_

#include <stdint.h>
#include <stdbool.h>

#include "bsp/bsp.h"

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
 */
void pulse_trigger(output_t pin, bool state, uint32_t duration_ms);

/**
 * @brief Cancel a pulse being process and set pin state to state value
 * 
 * @param output RL1, RL2, OC1, OC2
 * @param state true, false
 */
void pulse_cancel(output_t pin);

/**
 * @brief Check if a pulse is being processed for the pin 
 * 
 * @param output RL1, RL2, OC1, OC2
 */
bool pulse_is_active(output_t pin);

/**
 * @brief Handle pulses if any
 * 
 * @param time_passed_ms 
 */
void pulse_process(uint32_t time_passed_ms);

/**
 * @brief Get time until next event
 * 
 * @return uint32_t 
 */
uint32_t pulse_remaining(void);

#endif /* _CUSTOMPCB_PULSE_H_ */