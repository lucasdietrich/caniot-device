/**
 * @file pulse.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Pulse API for generating simple pulses on output pins.
 * 	- Thread-safe. if CONFIG_GPIO_PULSE_THREAD_SAFE is set to 1
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

#define CONFIG_GPIO_PULSE_SIMULTANEOUS_COUNT 	4u
#define CONFIG_GPIO_PULSE_THREAD_SAFE 		1u

struct pulse_event
{
	struct titem _tie;

	/* State of the pin when not active */
	uint8_t reset_state: 1;

	/* Tells wether the pulse is active or not */
	uint8_t scheduled: 1;

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
 */
struct pulse_event *pulse_trigger(pin_descr_t descr, bool state, uint32_t duration_ms);

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

#endif /* _CUSTOMPCB_PULSE_H_ */