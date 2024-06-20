/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"
#include "gpio_pulse.h"
#include "gpio_xps.h"

#include <avrtos/logging.h>

#include <avrtos.h>

#define LOG_LEVEL CONFIG_BOARD_LOG_LEVEL

int command_xps(struct xps_context *xpsc,
                caniot_complex_digital_cmd_t cmd,
                uint32_t duration_ms)
{
    __ASSERT_TRUE(xpsc != NULL);

    LOG_DBG("0x%x: %u", xpsc->descr, cmd);

    if (BSP_DESCR_STATUS_GET(xpsc->descr) != BSP_DESCR_ACTIVE) {
        return -ENOTSUP;
    }

    switch (cmd) {
    case CANIOT_XPS_SET_ON:
        bsp_descr_gpio_output_write(xpsc->descr, GPIO_HIGH);
        break;
    case CANIOT_XPS_SET_OFF:
        bsp_descr_gpio_output_write(xpsc->descr, GPIO_LOW);
        break;
    case CANIOT_XPS_TOGGLE:
        bsp_descr_gpio_toggle(xpsc->descr);
        break;

#if CONFIG_GPIO_PULSE_SUPPORT
    case CANIOT_XPS_PULSE_ON:
    case CANIOT_XPS_PULSE_OFF:
        /* If a pulse is already active on this pin, cancel it
         * without setting the pin to its reset state. Then
         * trigger a new pulse.
         */
        pulse_cancel(xpsc->pev, false);
        xpsc->pev =
            pulse_trigger(xpsc->descr, cmd == CANIOT_XPS_PULSE_ON, duration_ms, NULL);

        LOG_DBG("XPS: descr=%u pev=%p rest=%u cmd=%u dur=%lu",
                xpsc->descr,
                xpsc->pev,
                xpsc->reset_state,
                cmd,
                duration_ms);
        break;
    case CANIOT_XPS_PULSE_CANCEL:
        pulse_cancel(xpsc->pev, true);
        break;
#endif

    case CANIOT_XPS_RESET:
#if CONFIG_GPIO_PULSE_SUPPORT
        pulse_cancel(xpsc->pev, true);
        xpsc->pev = NULL;
#endif
        bsp_descr_gpio_output_write(xpsc->descr, xpsc->reset_state);
    default:
        break;
    }

    return 0;
}