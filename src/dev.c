/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "build_info.h"
#include "class/class.h"
#include "dev.h"
#include "devices/gpio_xps.h"
#include "settings.h"

#include "platform.h"

#include <time.h>

#include <avrtos/logging.h>

#include <caniot/fake.h>

#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL
#define K_MODULE  K_MODULE_APPLICATION

K_SIGNAL_DEFINE(caniot_process_sig);

const caniot_did_t did = CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__);

static const struct caniot_device_id identification PROGMEM = {
    .did          = CANIOT_DID(__DEVICE_CLS__, __DEVICE_SID__),
    .version      = __FIRMWARE_VERSION__,
    .name         = __DEVICE_NAME__,
    .magic_number = __MAGIC_NUMBER__,
    .build_date   = __BUILD_DATE__,
    .build_commit = __BUILD_COMMIT__,
    .features     = {0u, 0u, 0u, 0u},
};

const struct caniot_drivers_api drivers = {
    .entropy  = platform_entropy,
    .get_time = platform_get_time,
    .set_time = platform_set_time,
    .recv     = platform_caniot_recv,
    .send     = platform_caniot_send,
};

int dev_apply_blc_sys_command(struct caniot_device *dev,
                              struct caniot_blc_sys_command *sysc)
{
    int ret = 0;

    if ((sysc->software_reset == CANIOT_SS_CMD_SET) ||
        (sysc->watchdog == CANIOT_TS_CMD_TOGGLE)) {
        ret = -ENOTSUP;
    }

    if (sysc->watchdog == CANIOT_TS_CMD_ON) {
        platform_watchdog_enable(true);
        ret = 0;
    } else if (sysc->watchdog == CANIOT_TS_CMD_OFF) {
        platform_watchdog_enable(false);
        ret = 0;
    }

    if (sysc->config_reset == CANIOT_SS_CMD_SET) {
        ret = settings_restore_default(dev);
    }

    if (sysc->watchdog_reset == CANIOT_SS_CMD_SET || sysc->reset == CANIOT_SS_CMD_SET) {
        ret = platform_reset();
    }

    return ret;
}

extern const caniot_telemetry_handler_t app_telemetry_handler;
extern const caniot_command_handler_t app_command_handler;

int telemetry_handler(struct caniot_device *dev,
                      caniot_endpoint_t ep,
                      unsigned char *buf,
                      uint8_t *len)
{
    if (ep == CANIOT_ENDPOINT_BOARD_CONTROL) {
        switch (__DEVICE_CLS__) {
        case CANIOT_DEVICE_CLASS0:
            return class0_blc_telemetry_handler(dev, buf, len);
        case CANIOT_DEVICE_CLASS1:
            return class1_blc_telemetry_handler(dev, buf, len);
        default:
            return -CANIOT_ENOTSUP;
        }
    } else {
        return app_telemetry_handler(dev, ep, buf, len);
    }
}

int command_handler(struct caniot_device *dev,
                    caniot_endpoint_t ep,
                    const unsigned char *buf,
                    uint8_t len)
{
    int ret = -CANIOT_ENOTSUP;

    switch (ep) {
    case CANIOT_ENDPOINT_BOARD_CONTROL:
        switch (__DEVICE_CLS__) {
#if defined(CONFIG_CLASS0_ENABLED)
        case CANIOT_DEVICE_CLASS0:
            ret = class0_blc_command_handler(dev, buf, len);
            break;
#endif
#if defined(CONFIG_CLASS1_ENABLED)
        case CANIOT_DEVICE_CLASS1:
            ret = class1_blc_command_handler(dev, buf, len);
            break;
#endif
        default:
            break;
        }
        break;
    case CANIOT_ENDPOINT_APP:
    case CANIOT_ENDPOINT_1:
    case CANIOT_ENDPOINT_2:
        ret = app_command_handler(dev, ep, buf, len);
        break;
    default:
        break;
    }

    return ret;
}

const struct caniot_device_api api = CANIOT_DEVICE_API_STD_INIT(
    command_handler, telemetry_handler, settings_read, settings_write);

__attribute__((section(".noinit"))) struct caniot_device_config settings_rambuf;
__STATIC_ASSERT(sizeof(settings_rambuf) <= 0xFF,
                "config too big"); /* EEPROM size depends on MCU */

struct caniot_device device = {
    .identification = &identification,
    .config         = &settings_rambuf,
    .api            = &api,
    .driv           = &drivers,
    .flags =
        {
            .request_telemetry_ep = 0u,
            .initialized          = 0u,
        },
};

void print_indentification(void)
{
    caniot_print_device_identification(&device);
}

int caniot_process(void)
{
    return caniot_device_process(&device);
}

uint32_t get_telemetry_timeout(void)
{
    return caniot_device_telemetry_remaining(&device);
}

bool telemetry_requested(void)
{
    return caniot_device_triggered_telemetry_any(&device);
}

void trigger_telemetry(caniot_endpoint_t ep)
{
    caniot_device_trigger_telemetry_ep(&device, ep);

    trigger_process();
}

void trigger_telemetrys(uint8_t endpoints_bitmask)
{
    for (uint8_t ep = CANIOT_ENDPOINT_APP; ep <= CANIOT_ENDPOINT_BOARD_CONTROL; ep++) {
        if (endpoints_bitmask & BIT(ep)) {
            caniot_device_trigger_telemetry_ep(&device, ep);
        }
    }

    trigger_process();
}

void caniot_init(void)
{
    settings_init(&device);
    caniot_app_init(&device);
}