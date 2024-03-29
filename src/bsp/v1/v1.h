/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BSP_V1
#define _BSP_V1

#include "bsp/bsp.h"

#define BSP_OC1 BSP_GPIO_DESCR_PC0
#define BSP_OC2 BSP_GPIO_DESCR_PC1
#define BSP_RL1 BSP_GPIO_DESCR_PC2
#define BSP_RL2 BSP_GPIO_DESCR_PC3

#define BSP_IN1 BSP_GPIO_DESCR_PB0
#define BSP_IN2 BSP_GPIO_DESCR_PD4
#define BSP_IN3 BSP_GPIO_DESCR_PD5
#define BSP_IN4 BSP_GPIO_DESCR_PD6

#define BSP_IN1_PIN BSP_DESCR_GPIO_PIN_GET(IN1)
#define BSP_IN2_PIN BSP_DESCR_GPIO_PIN_GET(IN2)
#define BSP_IN3_PIN BSP_DESCR_GPIO_PIN_GET(IN3)
#define BSP_IN4_PIN BSP_DESCR_GPIO_PIN_GET(IN4)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize V1 board BSP
 *
 */
void bsp_v1_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_V1 */