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
#include "watchdog.h"

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/pgmspace.h>
#include <caniot/caniot.h>
#include <caniot/device.h>
#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL

#if CONFIG_CAN_SOFT_FILTERING && !CONFIG_CANIOT_DEVICE_FILTER_FRAME
#error "CONFIG_CANIOT_DEVICE_FILTER_FRAME must be enabled when CONFIG_CAN_SOFT_FILTERING is enabled"
#endif

K_SIGNAL_DEFINE(dev_process_sig);

extern const caniot_telemetry_handler_t app_telemetry_handler;
extern const caniot_command_handler_t app_command_handler;

static int dev_restore_default(struct caniot_device *dev);
static int dev_inhibit(struct caniot_device *dev);

int dev_apply_blc_sys_command(struct caniot_device *dev,
                              struct caniot_blc_sys_command *sysc)
{
    int ret = 0;

    if ((sysc->_software_reset == CANIOT_SS_CMD_SET) ||
        (sysc->watchdog == CANIOT_TS_CMD_TOGGLE) ||
        (sysc->_watchdog_reset == CANIOT_SS_CMD_SET)) {
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
        ret = dev_restore_default(dev);
    }

    if (sysc->reset == CANIOT_SS_CMD_SET) {
        ret = platform_reset();
    }

    if (sysc->inhibit == CANIOT_TSP_CMD_PULSE) {
        ret = dev_inhibit(dev);
    }

    return ret;
}

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
    .command_handler     = command_handler,
    .telemetry_handler   = telemetry_handler,
    .config.on_read      = settings_read,
    .config.on_write     = settings_write,
    .custom_attr.read    = NULL,
    .custom_attr.write   = NULL,
};

#if CONFIG_DEVICE_SINGLE_INSTANCE

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

const struct caniot_drivers_api platform_caniot_drivers = {
    .entropy  = platform_entropy,
    .get_time = platform_get_time,
    .set_time = platform_set_time,
    .recv     = platform_caniot_recv,
    .send     = platform_caniot_send,
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

uint8_t dev_get_instance_index(struct caniot_device *dev)
{
    return 0u;
}

int dev_process(uint8_t tid)
{
    int ret;

    do {
        ret = caniot_device_process(&device);
        if (ret == 0) {
            /* When CAN message "sent" (actually queued to TX queue),
             * immediately yield after having queued the CAN message
             * so that it can be immediately sent.
             */
            k_yield();

        } else if (ret != -CANIOT_EAGAIN) {
            // show error
            caniot_show_error(ret);

            k_sleep(K_MSEC(100u));
        }

#if CONFIG_WATCHDOG
        /* I'm alive ! */
        alive(tid);
#endif /* CONFIG_WATCHDOG */

    } while (ret != -CANIOT_EAGAIN);

    return ret;
}

static int dev_restore_default(struct caniot_device *dev)
{
    return settings_restore_default(dev, &default_config);
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

static int dev_inhibit(struct caniot_device *dev)
{
    // do APP inhibit
    // do BSP inhibit
    return -ENOTSUP;
}

void dev_settings_restore_default(void)
{
    dev_restore_default(&device);
}

#else

#if !CONFIG_CANIOT_DEVICE_FILTER_FRAME
#error "CONFIG_CANIOT_DEVICE_FILTER_FRAME must be enabled in multi instance mode"
#endif

/* Default configuration */
extern struct caniot_device_config default_config[CONFIG_DEVICE_INSTANCES_COUNT];

#define CONFIG_DEVICE_MULTI_DID(_n)                                                      \
    CANIOT_DID(__DEVICE_CLS__, CONFIG_DEVICE_MULTI_SID(_n))

// clang-format off
/* Declare device as :
 * - base name + instance number
 * - magic number base + instance number
 * - did: class + instance number as sid
 */
#if CONFIG_CANIOT_BUILD_INFOS
#define DECL_DEV(_n) {                            \
    .did = CONFIG_DEVICE_MULTI_DID(_n),           \
    .version = __FIRMWARE_VERSION__,              \
    .name = __DEVICE_NAME__ Z_STRINGIFY(_n),      \
    .magic_number = __MAGIC_NUMBER__ + _n,        \
    .build_date = __BUILD_DATE__,                 \
    .build_commit = __BUILD_COMMIT__,             \
    .features = {0u, 0u, 0u, 0u},                 \
}
#else
#define DECL_DEV(_n) {                            \
    .did = CONFIG_DEVICE_MULTI_DID(_n),           \
    .version = __FIRMWARE_VERSION__,              \
    .name = __DEVICE_NAME__ Z_STRINGIFY(_n),      \
    .magic_number = __MAGIC_NUMBER__ + _n,        \
    .features = {0u, 0u, 0u, 0u},                 \
}
#endif

static const struct caniot_device_id identification[CONFIG_DEVICE_INSTANCES_COUNT] PROGMEM = {
    DECL_DEV(0),
#if CONFIG_DEVICE_INSTANCES_COUNT > 1
    DECL_DEV(1),
#endif
#if CONFIG_DEVICE_INSTANCES_COUNT > 2
    DECL_DEV(2),
#endif
#if CONFIG_DEVICE_INSTANCES_COUNT > 3
    DECL_DEV(3),
#endif
#if CONFIG_DEVICE_INSTANCES_COUNT > 4
    DECL_DEV(4),
#endif
#if CONFIG_DEVICE_INSTANCES_COUNT > 5
    DECL_DEV(5),
#endif
#if CONFIG_DEVICE_INSTANCES_COUNT > 6
    DECL_DEV(6),
#endif
#if CONFIG_DEVICE_INSTANCES_COUNT > 7
    DECL_DEV(7),
#endif
};
// clang-format on

static struct caniot_device devices[CONFIG_DEVICE_INSTANCES_COUNT] = {0u};
static uint8_t current_device_index = 0u;
static struct caniot_frame devices_rx_frames[CONFIG_DEVICE_INSTANCES_COUNT];
static uint8_t devices_rx_frames_status = 0u;

static int multi_caniot_recv(struct caniot_frame *frame);

const struct caniot_drivers_api platform_caniot_drivers = {
    .entropy  = platform_entropy,
    .get_time = platform_get_time,
    .set_time = platform_set_time,
    .recv     = multi_caniot_recv,
    .send     = platform_caniot_send,
};

void dev_init(void)
{
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
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
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        caniot_print_device_identification(&devices[i]);
        LOG_INF("\n");
    }
    LOG_INF("Multi instance mode\n");
}

uint8_t dev_get_instance_index(struct caniot_device *dev)
{
    return dev - devices;
}

static int multi_caniot_recv(struct caniot_frame *frame)
{
    /* We use provided buffer to store the received frame so that
     * we save 1 copy operation in case the frame matches the device being
     * processed.
     * If the frame does not match the device being processed, we then copy it
     * to the device frame buffer and return -EAGAIN.
     */
    int ret = platform_caniot_recv(frame);
    if (ret == 0) {
        /* We are sure the received frame is of the current class
         * because class filtering is done by the CAN driver.
         */
        const uint8_t device_index = frame->id.sid;

        /* if we received a frame for another device, we copy it to the device
         * frame buffer, set frame presence flag and return -EAGAIN.
         */
        if (device_index != current_device_index) {
            ret = -CANIOT_EAGAIN;

            /* Check the device id is in supported range */
            if (device_index < CONFIG_DEVICE_INSTANCES_COUNT) {
                memcpy(&devices_rx_frames[device_index], frame, sizeof(*frame));

                // make sure the frame slot is empty for the device
                if (devices_rx_frames_status & BIT(device_index)) {
                    LOG_ERR("device %u frame slot not empty, dropping ...", device_index);
                    devices_rx_frames_status |= BIT(device_index);
                }
            }
        }
    } else if (ret == -CANIOT_EAGAIN) {
        /* If we did not receive a frame, we check if we have a frame stored for the
         * current device.
         */
        if (devices_rx_frames_status & BIT(current_device_index)) {
            memcpy(frame, &devices_rx_frames[current_device_index], sizeof(*frame));
            devices_rx_frames_status &= ~BIT(current_device_index);
            ret = 0;
        }
    }

    return ret;
}

static inline uint8_t next_index(uint8_t index, uint8_t en_map)
{
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        index = (index + 1u) % CONFIG_DEVICE_INSTANCES_COUNT;
        if (en_map & BIT(index)) {
            break;
        }
    }

    return index;
}

int dev_process(uint8_t tid)
{
    /* Bitmask of devices to process
     * When calling dev_process() for the first time, we need to process 
     * all devices at least once.
     */
    uint8_t do_run_map = (1u << CONFIG_DEVICE_INSTANCES_COUNT) - 1u;

    /* Set current_device_index to a value that will be incremented to the
     * first device to process.
     */
    current_device_index = CONFIG_DEVICE_INSTANCES_COUNT - 1u;

    do {
        /* Select next device to process */
        current_device_index = next_index(current_device_index, do_run_map);

        int ret = caniot_device_process(&devices[current_device_index]);
        if (ret == 0) {
            // reprocess the device after
        } else if (ret == -CANIOT_EAGAIN) { // no frame received for the device
            /* clear device bit in map */
            do_run_map &= ~BIT(current_device_index);
        } else { // on error
            caniot_show_error(ret);
        }

#if CONFIG_WATCHDOG
        /* I'm alive ! */
        alive(tid);
#endif /* CONFIG_WATCHDOG */

        /* Yield to allow other threads to run */
        k_yield();
    } while (do_run_map);

    return 0;
}

static int dev_restore_default(struct caniot_device *dev)
{
    return settings_restore_default(dev, &default_config[dev - devices]);
}

uint32_t dev_get_process_timeout(void)
{
    uint32_t timeout = -1;

    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        timeout = MIN(timeout, caniot_device_telemetry_remaining(&devices[i]));
        if (!timeout) break;
    }

    return timeout;
}

bool dev_telemetry_is_requested(void)
{
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        if (caniot_device_triggered_telemetry_any(&devices[i])) {
            return true;
        }
    }

    return false;
}

void dev_trigger_telemetry(caniot_endpoint_t ep)
{
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        caniot_device_trigger_telemetry_ep(&devices[i], ep);
    }

    dev_trigger_process();
}

void dev_trigger_telemetrys(uint8_t endpoints_bitmask)
{
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        for (uint8_t ep = CANIOT_ENDPOINT_APP; ep <= CANIOT_ENDPOINT_BOARD_CONTROL; ep++) {
            if (endpoints_bitmask & BIT(ep)) {
                caniot_device_trigger_telemetry_ep(&devices[i], ep);
            }
        }
    }

    dev_trigger_process();
}

static int dev_inhibit(struct caniot_device *dev)
{
    return -ENOTSUP;
}

void dev_settings_restore_default(void)
{
    for (uint8_t i = 0u; i < CONFIG_DEVICE_INSTANCES_COUNT; i++) {
        dev_restore_default(&devices[i]);
    }
}

#endif /* CONFIG_DEVICE_SINGLE_INSTANCE */