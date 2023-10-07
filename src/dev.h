/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DEV_DEV_H_
#define _CANIOT_DEV_DEV_H_

#include "bsp/bsp.h"
#include "can.h"
#include "config.h"
#include "devices/gpio_pulse.h"
#include "devices/temp.h"

#include <avrtos/avrtos.h>

#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>
#include <util/delay.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WATCHDOG_TIMEOUT_MS   8000
#define WATCHDOG_TIMEOUT_WDTO WDTO_8S

extern const caniot_did_t did;

extern struct k_signal caniot_process_sig;

void print_indentification(void);

uint32_t get_magic_number(void);

void trigger_telemetry(caniot_endpoint_t endpoint);
void trigger_telemetrys(uint8_t endpoints_bitmask);

bool telemetry_requested(void);

static inline struct k_thread *trigger_process(void)
{
    return k_signal_raise(&caniot_process_sig, 0);
}

int caniot_process(void);

uint32_t get_telemetry_timeout(void);

void caniot_init(void);

int dev_apply_blc_sys_command(struct caniot_device *dev,
                              struct caniot_blc_sys_command *sysc);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEV_APPLICATION_H_ */