/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Don't ever use this code, it is fucked up
//
// Todo optimize this code to read only the location in eeprom instead of the whole "diag_reset_context",
// a buffer is allocated on RAM to store the reset context.

#include "config.h"
#include "diag.h"
#include "platform.h"
#include "utils/crc.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/eeprom.h>

#if CONFIG_DIAG

#define LOG_LEVEL CONFIG_DIAG_LOG_LEVEL

#define EEPROM_RESET_STATS_MAX_SIZE 64u
#define EEPROM_RESET_STATS_OFFSET   256u

#define RAM_RESET_CONTEXT_MAGIC 0x444BE182lu

#if EEPROM_RESET_STATS_MAX_SIZE > 64u
#warning                                                                                 \
    "Increasing this value have impact on stack usage with CONFIG_DIAG_RESET_CONTEXT_STATIC_RAMBUFFER disabled"
#endif

#if CONFIG_DIAG_RESET_CONTEXT_PERSISTENT && !CONFIG_DIAG_RESET_REASON
#error "CONFIG_DIAG_RESET_CONTEXT_PERSISTENT requires CONFIG_DIAG_RESET_REASON"
#endif

#if CONFIG_DIAG_RESET_CONTEXT_HISTORY && !CONFIG_DIAG_RESET_CONTEXT_PERSISTENT
#error "CONFIG_DIAG_RESET_CONTEXT_HISTORY requires CONFIG_DIAG_RESET_CONTEXT_RUNTIME"
#endif

#if CONFIG_DIAG_RESET_REASON && !CONFIG_KERNEL_MINICORE_SAVE_RESET_CAUSE
#error "CONFIG_DIAG_RESET_REASON requires CONFIG_KERNEL_MINICORE_SAVE_RESET_CAUSE"
#endif

#if CONFIG_DIAG_RESET_CONTEXT_STATIC_RAMBUFFER
/* Mutex to protect access to the diag_rambuf */
static K_MUTEX_DEFINE(diag_mutex);
static struct eeprom_reset_stats diag_rambuf;
#define DIAG_LOCK()           k_mutex_lock(&diag_mutex, K_FOREVER)
#define DIAG_UNLOCK()         k_mutex_unlock(&diag_mutex)
#define DIAG_DECLARE_RAMBUF() 
#define DIAG_GET_RAMBUF_PTR() &diag_rambuf
#else
#define DIAG_LOCK()
#define DIAG_UNLOCK()
#define DIAG_DECLARE_RAMBUF() struct eeprom_reset_stats z_reset_stats
#define DIAG_GET_RAMBUF_PTR() &z_reset_stats
#endif

#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
struct accross_reset_context {
    uint32_t magic;
    struct diag_runtime_context data;
};

static struct diag_runtime_context previous_run_context = {0u};

__noinit static struct accross_reset_context accross_reset_context;

void diag_reset_context_update(uint32_t uptime)
{
    accross_reset_context.data.uptime = uptime;
    accross_reset_context.magic       = RAM_RESET_CONTEXT_MAGIC;
}

static void reset_context_init(void)
{
    if (accross_reset_context.magic == RAM_RESET_CONTEXT_MAGIC) {
        /* Update the streak and streak uptime with the uptime of the previous run */
        accross_reset_context.data.streak_uptime += accross_reset_context.data.uptime;
        accross_reset_context.data.streak_count++;
        
        /* Copy the previous runtime data to a backup variable */
        previous_run_context.streak_count  = accross_reset_context.data.streak_count;
        previous_run_context.streak_uptime = accross_reset_context.data.streak_uptime;
        previous_run_context.uptime        = accross_reset_context.data.uptime;

        /* Reset the uptime for the current run */
        accross_reset_context.data.uptime = 0u;
    } else {
        diag_reset_context_clear();
    }

    LOG_DBG("diag: prev ctx streak: %u uptime: %u s tot: %u s",
            (uint16_t)previous_run_context.streak_count,
            (uint16_t)previous_run_context.uptime,
            (uint16_t)previous_run_context.streak_uptime);
}

void diag_reset_context_clear(void)
{
    accross_reset_context.data.streak_count  = 0u;
    accross_reset_context.data.uptime        = 0u;
    accross_reset_context.data.streak_uptime = 0u;
    accross_reset_context.magic              = RAM_RESET_CONTEXT_MAGIC;

    LOG_DBG("diag: ctx cleared");
}
#endif // CONFIG_DIAG_RESET_CONTEXT_RUNTIME

#if CONFIG_DIAG_RESET_CONTEXT_PERSISTENT

struct eeprom_reset_stats {
    uint16_t counter[PLATFORM_RESET_REASON_COUNT];
#if CONFIG_DIAG_RESET_CONTEXT_HISTORY
    struct diag_reset_context contexts[CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT];
#endif
    /* Structure size, used as a marker to make sure the structure is valid
     * This garentees that the stats are cleared if the configuration changes
     */
    uint8_t size;

    /* Checksum of the structure */
    uint8_t checksum;
} __packed;

#define EEPROM_RESET_STATS_SIZE sizeof(struct eeprom_reset_stats)

__STATIC_ASSERT(EEPROM_RESET_STATS_SIZE <= EEPROM_RESET_STATS_MAX_SIZE,
                "EEPROM_RESET_STATS_SIZE too big");

// Read the reset reset_stats from EEPROM and store them in the reset_stats struct.
static bool read_reset_stats(struct eeprom_reset_stats *reset_stats)
{
    eeprom_read_block(
        reset_stats, (void *)EEPROM_RESET_STATS_OFFSET, EEPROM_RESET_STATS_SIZE);

    LOG_HEXDUMP_DBG(reset_stats, EEPROM_RESET_STATS_SIZE);

    return (reset_stats->size == EEPROM_RESET_STATS_SIZE) &&
           (crc8((const uint8_t *)reset_stats, EEPROM_RESET_STATS_SIZE) == 0u);
}

// Compute the checksum of the reset_stats struct and write it to EEPROM.
static bool write_reset_stats(struct eeprom_reset_stats *reset_stats)
{
    reset_stats->size = EEPROM_RESET_STATS_SIZE;
    reset_stats->checksum =
        crc8((const uint8_t *)reset_stats, EEPROM_RESET_STATS_SIZE - 1u);

    eeprom_update_block(
        reset_stats, (void *)EEPROM_RESET_STATS_OFFSET, EEPROM_RESET_STATS_SIZE);

    return true;
}

bool diag_reset_stats_eeprom_clear(void)
{
    struct eeprom_reset_stats reset_stats;
    memset(&reset_stats, 0u, sizeof(reset_stats));
    return write_reset_stats(&reset_stats);
}

uint16_t diag_reset_get_count_by_reason(diag_reset_reason_t reason)
{
    uint16_t count = 0u;

    DIAG_LOCK();
    DIAG_DECLARE_RAMBUF();
    struct eeprom_reset_stats *p_reset_stats = DIAG_GET_RAMBUF_PTR();
    if (reason < PLATFORM_RESET_REASON_COUNT && read_reset_stats(p_reset_stats)) {
        count = p_reset_stats->counter[reason];
    }

    DIAG_UNLOCK();

    return count;
}

uint16_t diag_reset_get_count(void)
{
    uint16_t count = 0u;

    DIAG_LOCK();
    DIAG_DECLARE_RAMBUF();
    struct eeprom_reset_stats *p_reset_stats = DIAG_GET_RAMBUF_PTR();
    if (read_reset_stats(p_reset_stats)) {
        for (uint8_t i = 0u; i < PLATFORM_RESET_REASON_COUNT; i++) {
            count += p_reset_stats->counter[i];
        }
    }

    DIAG_UNLOCK();

    return count;
}

int8_t diag_reset_count_clear_bm(uint8_t reason_flags)
{
    int8_t ret = -EIO;

    reason_flags &= BIT(PLATFORM_RESET_REASON_COUNT) - 1u;

    DIAG_LOCK();
    DIAG_DECLARE_RAMBUF();
    struct eeprom_reset_stats *p_reset_stats = DIAG_GET_RAMBUF_PTR();

    if (read_reset_stats(p_reset_stats)) {
        for (uint8_t i = 0u; i < PLATFORM_RESET_REASON_COUNT; i++) {
            if (reason_flags & BIT(i)) {
                p_reset_stats->counter[i] = 0u;
            }
        }

        if (write_reset_stats(p_reset_stats)) ret = 0;
    }

    DIAG_UNLOCK();

    return ret;
}

#if CONFIG_DIAG_RESET_CONTEXT_HISTORY
static void reset_context_save(struct diag_reset_context *store,
                               struct diag_reset_context *ctx)
{
    K_ASSERT_APP((ctx != NULL) && (ctx->reason != PLATFORM_RESET_REASON_UNKNOWN));

    bool saved = false;
    struct diag_reset_context *cur;

    for (cur = store; cur < store + CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT; cur++) {
        if (saved == false) {
            if (cur->reset_reason == PLATFORM_RESET_REASON_UNKNOWN) {
                *cur  = *ctx;
                saved = true;
            }
        } else {
            /* Exit as cur contains the slot after the last saved context
             * this slot must be cleared to mark the end of the store
             */
            break;
        }
    }

    /* Wrap around if we reached the end of the store */
    if (cur == store + CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT) {
        cur = store;
    }

    /* If we didn't save the context at this point, the store is malformed
     * and should be *cleared*
     */
    if (!saved) {
        store[0u] = *ctx;
        cur       = &store[1u];
    }

    /* clear next slot by writing PLATFORM_RESET_REASON_UNKNOWN
     * we assume this is not a valid reset reason
     */
    cur->reset_reason = PLATFORM_RESET_REASON_UNKNOWN;
}

static struct diag_reset_context *
reset_context_read_last(struct diag_reset_context *store, uint8_t ago)
{
    if (ago + 1u >= CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT) return NULL;

#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
    for (uint8_t i = 0u; i < CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT; i++) {
        LOG_DBG("DIAG: [%u] reason: %u streak: %u uptime: %u s total: %u s",
                i,
                (uint16_t)store[i].reset_reason,
                (uint16_t)store[i].last_runtime.streak_count,
                (uint16_t)store[i].last_runtime.uptime,
                (uint16_t)store[i].last_runtime.streak_uptime);
    }
#endif // CONFIG_DIAG_RESET_CONTEXT_RUNTIME

    uint8_t last_index;
    for (last_index = 0u; last_index < CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT;
         last_index++) {
        if (store[last_index].reset_reason == PLATFORM_RESET_REASON_UNKNOWN) {
            break;
        }
    }

    last_index = (last_index - 1u + CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT - ago) %
                 CONFIG_DIAG_RESET_CONTEXT_SAVED_COUNT;

    struct diag_reset_context *ctx = &store[last_index];
    if (ctx->reset_reason == PLATFORM_RESET_REASON_UNKNOWN) {
        ctx = NULL;
    }

    LOG_DBG("diag: ctx read last-%u -> ctx: %p", ago, ctx);

    return ctx;
}

static bool get_previous_run_context(struct diag_reset_context *ctx)
{
    ctx->reset_reason = diag_reset_get_reason();

#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
    ctx->last_runtime.streak_count  = previous_run_context.streak_count;
    ctx->last_runtime.streak_uptime = previous_run_context.streak_uptime;
    ctx->last_runtime.uptime        = previous_run_context.uptime;
#endif // CONFIG_DIAG_RESET_CONTEXT_RUNTIME

    return ctx->reset_reason != PLATFORM_RESET_REASON_UNKNOWN;
}
#endif // CONFIG_DIAG_RESET_CONTEXT_HISTORY

static bool diag_reset_context_initialized = false;

static int8_t diag_reset_stats_init_update(void)
{
    /* Function should be called only once */
    if (diag_reset_context_initialized) return -EAGAIN;

    int8_t ret = -EIO;

    DIAG_LOCK();
    DIAG_DECLARE_RAMBUF();
    struct eeprom_reset_stats *p_reset_stats = DIAG_GET_RAMBUF_PTR();

    if (read_reset_stats(p_reset_stats) == false) {
        /* Failed to read EEPROM, initialize reset_stats and write the to EEPROM */
        memset(p_reset_stats, 0u, sizeof(*p_reset_stats));
    }

    diag_reset_reason_t last_reason = diag_reset_get_reason();

    /* Increment the counter corresponding to the last reset reason */
    p_reset_stats->counter[last_reason]++;

#if CONFIG_DIAG_RESET_CONTEXT_HISTORY
    struct diag_reset_context ctx = {0u};
    if (get_previous_run_context(&ctx) == true) {
        reset_context_save(p_reset_stats->contexts, &ctx);
    }
#endif // CONFIG_DIAG_RESET_CONTEXT_HISTORY

    if (write_reset_stats(p_reset_stats)) {
        ret                            = 0;
        diag_reset_context_initialized = true;
    }

    DIAG_UNLOCK();

    return ret;
}
#endif // CONFIG_DIAG_RESET_CONTEXT_PERSISTENT

#if CONFIG_DIAG_RESET_REASON
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

diag_reset_reason_t diag_reset_get_last_reason(uint8_t ago)
{
    diag_reset_reason_t reason = PLATFORM_RESET_REASON_UNKNOWN;

    if (ago == 0u) {
        reason = diag_reset_get_reason();
    } else {
#if CONFIG_DIAG_RESET_CONTEXT_HISTORY
        DIAG_LOCK();
        DIAG_DECLARE_RAMBUF();
        struct eeprom_reset_stats *p_reset_stats = DIAG_GET_RAMBUF_PTR();
        if (read_reset_stats(p_reset_stats)) {
            struct diag_reset_context *ctx =
                reset_context_read_last(p_reset_stats->contexts, ago);
            if (ctx != NULL) {
                reason = ctx->reset_reason;
            }
        }
        DIAG_UNLOCK();
#endif // CONFIG_DIAG_RESET_CONTEXT_HISTORY
    }

    LOG_DBG("diag: last-%u reset reason: %u", ago, reason);

    return reason;
}
#endif // CONFIG_DIAG_RESET_REASON

#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
/**
 * @brief Get the reset context for the reset that occured "ago" resets ago.
 *
 * @param ago Get the nth last reset context.
 * @param ctx Pointer to the reset context to fill.
 * @return int8_t
 */
int8_t diag_reset_context_get_last(uint8_t ago, struct diag_reset_context *ctx)
{
    if (ctx == NULL) return -EINVAL;

    int8_t ret;

    if (ago == 0u) {
        ctx->reset_reason               = diag_reset_get_reason();
        ctx->last_runtime.streak_count  = previous_run_context.streak_count;
        ctx->last_runtime.streak_uptime = previous_run_context.streak_uptime;
        ctx->last_runtime.uptime        = previous_run_context.uptime;
        ret = 0u;
    } else {
#if CONFIG_DIAG_RESET_CONTEXT_HISTORY
        DIAG_LOCK();
        DIAG_DECLARE_RAMBUF();
        struct eeprom_reset_stats *p_reset_stats = DIAG_GET_RAMBUF_PTR();
        if (read_reset_stats(p_reset_stats)) {
            struct diag_reset_context *ref_ctx =
                reset_context_read_last(p_reset_stats->contexts, ago);
            if (ref_ctx != NULL) {
                memcpy(ctx, ref_ctx, sizeof(*ctx));
                ret = 0;
            } else {
                ret = -ENOENT;
            }
        } else {
            ret = -EIO;
        }
        DIAG_UNLOCK();
#else
        ret = -ENOTSUP;
#endif // CONFIG_DIAG_RESET_CONTEXT_HISTORY
    }

    LOG_DBG("diag: ctx get last-%u -> ret: %d %p(reason: %u streak: %u uptime: %u s total: "
            "%u s)",
            ago,
            ret,
            ctx,
            ctx->reset_reason,
            (uint16_t)ctx->last_runtime.streak_count,
            (uint16_t)ctx->last_runtime.uptime,
            (uint16_t)ctx->last_runtime.streak_uptime);

    return ret;
}
#endif // CONFIG_DIAG_RESET_CONTEXT_RUNTIME

void diag_init(void)
{
#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
    reset_context_init();
#endif // CONFIG_DIAG_RESET_CONTEXT_RUNTIME

#if CONFIG_DIAG_RESET_CONTEXT_PERSISTENT
    diag_reset_stats_init_update();
#endif // CONFIG_DIAG_RESET_CONTEXT_PERSISTENT
}

#endif // CONFIG_DIAG