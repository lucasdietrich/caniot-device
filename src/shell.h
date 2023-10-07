/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SHELL_H_
#define _SHELL_H_

/**
 * @brief Initialize shell
 */
void shell_init(void);

/**
 * @brief Process received characters
 */
void shell_process(void);

#endif /* _SHELL_H_ */