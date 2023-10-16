/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "build_info.h"
#include "class/class.h"
#include "config.h"
#include "dev.h"
#include "platform.h"
#include "settings.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/device.h>
#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL

K_SIGNAL_DEFINE(dev_process_sig);

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
        ret = dev_restore_default();
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
        switch (__DEVICE_CLS__) { /* TODO get device class dynamically */
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

__attribute__((
    section(".noinit"))) static struct caniot_device_config device_settings_rambuf;
__STATIC_ASSERT(sizeof(device_settings_rambuf) <= 1024u,
                "config too big"); /* EEPROM size depends on MCU */

static const struct caniot_device_api device_caniot_api = {
    .command_handler   = command_handler,
    .telemetry_handler = telemetry_handler,
    .config.on_read    = settings_read,
    .config.on_write   = settings_write,
    .custom_attr.read  = NULL,
    .custom_attr.write = NULL,
};

#if DEVICE_SINGLE_INSTANCE

/* Default configuration */
extern struct caniot_device_config default_config;

static const struct caniot_device_id identification PROGMEM = {
    .did          = DEVICE_DID,
    .version      = __FIRMWARE_VERSION__,
    .name         = __DEVICE_NAME__,
    .magic_number = __MAGIC_NUMBER__,
#if CONFIG_CANIOT_BUILD_INFOS
    .build_date   = __BUILD_DATE__,
    .build_commit = __BUILD_COMMIT__,
#endif
    .features = {0u, 0u, 0u, 0u},
};

struct caniot_device device = {
    .identification = &identification,
    .config         = &device_settings_rambuf,
    .api            = &device_caniot_api,
    .driv           = &platform_caniot_drivers,
};

void dev_init(void)
{
    caniot_app_init(&device);
    settings_init(&device, &default_config);
}

void dev_print_indentification(void)
{
    caniot_print_device_identification(&device);
}

int dev_process(void)
{
    return caniot_device_process(&device);
}

int dev_restore_default(void)
{
    return settings_restore_default(&device, &default_config);
}

uint32_t dev_get_process_timeout(void)
{
    return caniot_device_telemetry_remaining(&device);
}

bool dev_telemetry_is_requested(void)
{
    return caniot_device_triggered_telemetry_any(&device);
}

void dev_trigger_telemetry(caniot_endpoint_t ep)
{
    caniot_device_trigger_telemetry_ep(&device, ep);

    dev_trigger_process();
}

void dev_trigger_telemetrys(uint8_t endpoints_bitmask)
{
    for (uint8_t ep = CANIOT_ENDPOINT_APP; ep <= CANIOT_ENDPOINT_BOARD_CONTROL; ep++) {
        if (endpoints_bitmask & BIT(ep)) {
            caniot_device_trigger_telemetry_ep(&device, ep);
        }
    }

    dev_trigger_process();
}

#else

/* Default configuration */
extern struct caniot_device_config default_config[DEVICE_INSTANCES_COUNT];

static const struct caniot_device_id identification[DEVICE_INSTANCES_COUNT] PROGMEM = {{
    .did          = DEVICE_DID,
    .version      = __FIRMWARE_VERSION__,
    .name         = __DEVICE_NAME__,
    .magic_number = __MAGIC_NUMBER__,
#if CONFIG_CANIOT_BUILD_INFOS
    .build_date   = __BUILD_DATE__,
    .build_commit = __BUILD_COMMIT__,
#endif
    .features     = {0u, 0u, 0u, 0u},
}}; /* TODO, define other devices */

static struct caniot_device devices[DEVICE_INSTANCES_COUNT] = {0u};

void dev_init(void)
{
    for (uint8_t i = 0u; i < DEVICE_INSTANCES_COUNT; i++) {
        struct caniot_device *dev = &devices[i];

        dev->identification = &identification[i];
        dev->driv           = &platform_caniot_drivers;
        dev->api            = &device_caniot_api;
        dev->config         = &device_settings_rambuf;

        caniot_app_init(dev);
        settings_init(dev, &default_config[i]);
    }
}

void dev_print_indentification(void)
{
    for (uint8_t i = 0u; i < DEVICE_INSTANCES_COUNT; i++) {
        caniot_print_device_identification(&devices[i]);
    }
}

int dev_process(void)
{
    return -ENOTSUP;
}

int dev_restore_default(void)
{
    return -ENOTSUP;
}

uint32_t dev_get_process_timeout(void)
{
    return -1;
}

bool dev_telemetry_is_requested(void)
{
    return false;
}

void dev_trigger_telemetry(caniot_endpoint_t ep)
{
    (void)ep;
}

void dev_trigger_telemetrys(uint8_t endpoints_bitmask)
{
    (void)endpoints_bitmask;
}

#endif /* DEVICE_SINGLE_INSTANCE */