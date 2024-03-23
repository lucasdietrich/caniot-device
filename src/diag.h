/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
    *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DIAG_H_
#define _DIAG_H_

#include <stdbool.h>
#include <stdint.h>

#include <avrtos/sys.h>

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
 * @brief Maintain the current reset context state by providing the current uptime.
 * 
 * @return int8_t 
 */
void diag_reset_context_update(uint32_t uptime);

/**
 * @brief Clear the reset stats in EEPROM.
 */
bool diag_reset_stats_eeprom_clear(void);

/**
 * @brief Clear the current reset context.
 * 
 * @return int8_t 
 */
void diag_reset_context_clear(void);

struct diag_runtime_context {
    uint32_t uptime;
    uint32_t streak_uptime;
    uint16_t streak_count;
};

struct diag_reset_context {
    uint8_t reset_reason;
#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
    struct diag_runtime_context last_runtime;
#endif /* CONFIG_DIAG_RESET_CONTEXT_RUNTIME */
} __packed;

/**
 * @brief Return the last reset reason that occured "ago" resets ago.
 * 
 * @param ago Get the nth last reset reason.
 * @return diag_reset_reason_t 
 */
diag_reset_reason_t diag_reset_get_last_reason(uint8_t ago);

/**
 * @brief Get the reset context for the reset that occured "ago" resets ago.
 *
 * @param ago Get the nth last reset context.
 * @param ctx Pointer to the reset context to fill.
 * @return int8_t
 */
int8_t diag_reset_context_get_last(uint8_t ago, struct diag_reset_context *ctx);

#endif /* _DIAG_H_ */