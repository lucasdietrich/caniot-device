/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
    *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DIAG_H_
#define _DIAG_H_

#include <stdint.h>

/**
 * @brief Initialize the diagnostic functions.
 */
void diag_init(void);

typedef enum diag_reset_reason {
    /* Unknown reset reason */
    PLATFORM_RESET_REASON_UNKNOWN = 0,
    /* Board powered on */
    PLATFORM_RESET_REASON_POWER_ON,
    /* Watchdog reset */
    PLATFORM_RESET_REASON_WATCHDOG,
    /* External reset, e.g. reset pin/serial */
    PLATFORM_RESET_REASON_EXTERNAL,
    /* Number of reset reasons, must be last */
    PLATFORM_RESET_REASON_COUNT
} diag_reset_reason_t;

/**
 * @brief Return the last reset reason.
 * 
 * @return diag_reset_reason_t 
 */
diag_reset_reason_t diag_reset_get_reason(void);

/**
 * @brief Return the number of reset for a given reason.
 * 
 * @param reason 
 * @return uint16_t 
 */
uint16_t diag_reset_get_count_by_reason(diag_reset_reason_t reason);

/**
 * @brief Return the total number of reset.
 * 
 * @return uint16_t 
 */
uint16_t diag_reset_get_count(void);

/**
 * @brief Reset the reset counter(s) for the reason(s) specified in the given bitfield.
 * 
 * @param reason_flags 
 * @return int8_t 
 */
int8_t diag_reset_count_clear_bm(uint8_t reason_flags);

/**
 * @brief Reset all the reset counters.
 * 
 * @return int8_t 
 */
static inline int8_t diag_reset_count_clear_bm_all(void)
{
    return diag_reset_count_clear_bm(0xFFu);
}

/**
 * @brief Initialize and update the reset counters on startup.
 * 
 * @return int8_t 
 */
int8_t diag_reset_counters_init_update(void);

char diag_thread_get_name(uint8_t index);

uint16_t diag_thread_get_state(uint8_t index);

uint16_t diag_thread_get_stack_usage(uint8_t index);

uint16_t diag_thread_get_stack_size(uint8_t index);

uint16_t diag_thread_get_max_stack_usage(uint8_t index);

uint32_t diag_os_ticks(void);

uint8_t diag_cpu_usage(void);


#endif /* _DIAG_H_ */