/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_SHELL

#include "bsp/bsp.h"
#include "config.h"
#include "dev.h"
#include "devices/heater.h"
#include "diag.h"
#include "platform.h"
#include "shell.h"
#include "utils/hexdump.h"
#include "watchdog.h"

#include <stdint.h>
#include <stdio.h>

#include <avrtos/debug.h>
#include <avrtos/drivers/usart.h>
#include <avrtos/logging.h>

#include <avr/eeprom.h>
#include <caniot/caniot.h>
#define LOG_LEVEL LOG_LEVEL_INF

#define SHELL_USART         BSP_USART
#define SHELL_USART_RX_vect BSP_USART_RX_vect

#if CONFIG_SHELL_WORKQ_OFFLOADED
static void shell_handler(struct k_work *work);

K_SEM_DEFINE(shell_sem, 1u, 1u);
K_WORK_DEFINE(shell_work, shell_handler);

static void shell_handler(struct k_work *work)
{
    (void)work;

    shell_process();
}
#endif

K_MSGQ_DEFINE(shell_msgq, 1u, 16u);

ISR(SHELL_USART_RX_vect)
{
    char chr = SHELL_USART->UDRn;
    int ret  = k_msgq_put(&shell_msgq, &chr, K_NO_WAIT);

#if CONFIG_SHELL_WORKQ_OFFLOADED
    if ((ret == 0) && (k_sem_take(&shell_sem, K_NO_WAIT) == 0)) {
        k_system_workqueue_submit(&shell_work);
    }
#endif
}

void shell_init(void)
{
    ll_usart_enable_rx_isr(BSP_USART);
}

static void dump_ram(void)
{
    uint8_t *ptr   = (uint8_t *)RAMSTART;
    uint8_t *end   = (uint8_t *)RAMEND + 1u;
    uint8_t offset = 0u;

    while (ptr < end) {
        hexdump_byte(offset++, *ptr++);
    }
}

static void dump_flash(void)
{
    uint8_t *ptr   = (uint8_t *)FLASHSTART;
    uint8_t *end   = (uint8_t *)FLASHEND + 1u;
    uint8_t offset = 0u;

    while (ptr < end) {
        hexdump_byte(offset++, pgm_read_byte(ptr++));
    }
}

static void dump_eeprom(void)
{
    uint8_t *ptr   = (uint8_t *)E2START;
    uint8_t *end   = (uint8_t *)E2END + 1u;
    uint8_t offset = 0u;

    while (ptr < end) {
        hexdump_byte(offset++, eeprom_read_byte(ptr++));
    }
}

#if CONFIG_TEST_STRESS
static void stress_thread_task(void *arg)
{
    /* Stop itself and wait for the stress test to be enabled */
    k_stop();

    LOG_INF("stress test starting ...");

    for (;;) {
        k_wait(K_SECONDS(1), K_WAIT_MODE_ACTIVE);
        LOG_INF("stress test running ...");
    }
}

K_THREAD_DEFINE(stress_test_thread, stress_thread_task, 128, K_PREEMPTIVE, NULL, 's');

void stress_test_toggle(void)
{
    static bool enabled = false;

    enabled = !enabled;

    if (enabled) {
        k_thread_start(&stress_test_thread);
    } else {
        k_thread_stop(&stress_test_thread);
    }
}
#endif

void shell_process(void)
{
    char chr;

    while (k_msgq_get(&shell_msgq, &chr, K_NO_WAIT) == 0) {
        LOG_INF("shell: %c", chr);

        switch ((uint8_t)chr) {
        case 'W':
        case 'w':
            // Reset
            platform_reset(false);
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case 't':
        case 'T': {
            caniot_endpoint_t ep;
            if (IN_RANGE(chr, '0', '3')) {
                ep = (caniot_endpoint_t)(chr - '0');
            } else {
                ep = CANIOT_ENDPOINT_BOARD_CONTROL;
            }
#if CONFIG_SHELL_WORKQ_OFFLOADED
            k_sched_lock();
#endif
            dev_trigger_telemetry(ep);
#if CONFIG_SHELL_WORKQ_OFFLOADED
            k_sched_unlock();
#endif
            break;
        }
        case 'U':
        case 'u':
            k_show_uptime();
            k_show_ticks();
            printf_P(PSTR("\n"));
            break;
        case 'C':
        case 'c':
            k_dump_stack_canaries();
            break;
        case 'K':
        case 'k':
            k_thread_dump_all();
            break;
#if CONFIG_HEATERS_COUNT
        case 'H':
        case 'h': {
            static heater_mode_t mode = HEATER_MODE_OFF;

            for (uint8_t i = 0u; i < CONFIG_HEATERS_COUNT; i++) {
                heater_set_mode(i, mode);
            }

            switch (mode) {
            case HEATER_MODE_OFF:
                mode = HEATER_MODE_COMFORT;
                break;
            case HEATER_MODE_COMFORT:
                mode = HEATER_MODE_ENERGY_SAVING;
                break;
            case HEATER_MODE_ENERGY_SAVING:
                mode = HEATER_MODE_FROST_FREE;
                break;
            case HEATER_MODE_FROST_FREE:
                mode = HEATER_MODE_OFF;
                break;
            default:
                break;
            }
            break;
        }
#endif
#if CONFIG_TEST_STRESS
        case 'L':
        case 'l':
            stress_test_toggle();
            break;
#endif
        case 's':
        case 'S':
            dev_settings_restore_default();
            break;
        case 'r':
        case 'R':
            dump_ram();
            break;
        case 'e':
        case 'E':
            dump_eeprom();
            break;
        case 'f':
        case 'F':
            dump_flash();
            break;
#if CONFIG_DIAG
#if CONFIG_DIAG_RESET_CONTEXT_RUNTIME
        case 'x':
        case 'X':
            diag_reset_context_clear();
            break;
#endif /* CONFIG_DIAG_RESET_CONTEXT_RUNTIME */
#if CONFIG_DIAG_RESET_REASON
        case 'z':
        case 'Z':
            diag_reset_stats_eeprom_clear();
            break;
#endif /* CONFIG_DIAG_RESET_CONTEXT_RUNTIME */
#endif /* CONFIG_DIAG */
        default:
            break;
        }

#if CONFIG_SHELL_WORKQ_OFFLOADED
        k_sem_give(&shell_sem);
#endif

        k_yield();
    }
}

#endif