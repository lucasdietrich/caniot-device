/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"
#include "platform.h"
#include "utils/crc.h"

#include <avr/eeprom.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include "diag.h"

void diag_init(void)
{
#if CONFIG_DIAG_RESET_COUNTERS
    diag_reset_counters_init_update();
#endif
}

#if CONFIG_DIAG_RESET_COUNTERS
#if CONFIG_KERNEL_MINICORE_SAVE_RESET_CAUSE == 0
#error "CONFIG_KERNEL_MINICORE_SAVE_RESET_CAUSE must be enabled"
#endif

#define EEPROM_RESET_COUNTERS_OFFSET 256u

struct eeprom_reset_counters {
    uint16_t counter[PLATFORM_RESET_REASON_COUNT];
    uint8_t checksum;
};

#define EEPROM_RESET_COUNTERS_SIZE sizeof(struct eeprom_reset_counters)

// Read the reset counters from EEPROM and store them in the counters struct.
static bool read_reset_counters(struct eeprom_reset_counters *counters)
{
    eeprom_read_block(
        counters, (void *)EEPROM_RESET_COUNTERS_OFFSET, EEPROM_RESET_COUNTERS_SIZE);

    return crc8((const uint8_t *)counters, EEPROM_RESET_COUNTERS_SIZE) == 0u;
}

// Compute the checksum of the counters struct and write it to EEPROM.
static bool write_reset_counters(struct eeprom_reset_counters *counters)
{
    counters->checksum = crc8((const uint8_t *)counters, EEPROM_RESET_COUNTERS_SIZE - 1u);

    eeprom_update_block(
        counters, (void *)EEPROM_RESET_COUNTERS_OFFSET, EEPROM_RESET_COUNTERS_SIZE);

    return true;
}

diag_reset_reason_t diag_reset_get_reason(void)
{
    uint8_t mcusr = z_get_mcusr();

    if (mcusr & BIT(PORF)) {
        return PLATFORM_RESET_REASON_POWER_ON;
    } else if (mcusr & BIT(EXTRF)) {
        return PLATFORM_RESET_REASON_EXTERNAL;
    } else if (mcusr & BIT(WDRF)) {
        return PLATFORM_RESET_REASON_WATCHDOG;
    } else {
        return PLATFORM_RESET_REASON_UNKNOWN;
    }
}

uint16_t diag_reset_get_count_by_reason(diag_reset_reason_t reason)
{
    uint16_t count = 0u;
    struct eeprom_reset_counters counters;

    if (reason < PLATFORM_RESET_REASON_COUNT && read_reset_counters(&counters)) {
        count = counters.counter[reason];
    }

    return count;
}

uint16_t diag_reset_get_count(void)
{
    uint16_t count = 0u;
    struct eeprom_reset_counters counters;

    if (read_reset_counters(&counters)) {
        for (uint8_t i = 0u; i < PLATFORM_RESET_REASON_COUNT; i++) {
            count += counters.counter[i];
        }
    }

    return count;
}

int8_t diag_reset_count_clear_bm(uint8_t reason_flags)
{
    struct eeprom_reset_counters counters;
    int8_t ret = -EIO;

    reason_flags &= BIT(PLATFORM_RESET_REASON_COUNT) - 1u;

    if (read_reset_counters(&counters)) {
        for (uint8_t i = 0u; i < PLATFORM_RESET_REASON_COUNT; i++) {
            if (reason_flags & BIT(i)) {
                counters.counter[i] = 0u;
            }
        }

        if (write_reset_counters(&counters)) ret = 0;
    }

    return ret;
}

static bool diag_reset_counters_initialized = false;

int8_t diag_reset_counters_init_update(void)
{
    /* Function should be called only once */
    if (diag_reset_counters_initialized) return -EAGAIN;

    int8_t ret = -EIO;
    struct eeprom_reset_counters counters;

    if (read_reset_counters(&counters)) {
        diag_reset_reason_t last_reason = diag_reset_get_reason();

        /* Increment the counter corresponding to the last reset reason */
        counters.counter[last_reason]++;
    } else {
        /* Failed to read EEPROM, initialize counters and write them to EEPROM */
        memset(&counters, 0u, sizeof(counters));
    }

    if (write_reset_counters(&counters)) {
        ret                             = 0;
        diag_reset_counters_initialized = true;
    }

    return ret;
}

#endif // CONFIG_DIAG_RESET_COUNTERS

#if CONFIG_DIAG_THREAD_INFO

char diag_thread_get_name(uint8_t index)
{
    k_dump_stack_canaries()
}

uint16_t diag_thread_get_state(uint8_t index)
{

}

uint16_t diag_thread_get_stack_usage(uint8_t index)
{

}

uint16_t diag_thread_get_stack_size(uint8_t index)
{

}

uint16_t diag_thread_get_max_stack_usage(uint8_t index)
{

}

uint32_t diag_os_ticks(void)
{

}

uint8_t diag_cpu_usage(void)
{
    
}

#endif // CONFIG_DIAG_THREAD_INFO