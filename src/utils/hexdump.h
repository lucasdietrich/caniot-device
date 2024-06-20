/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HEXDUMP_H_
#define _HEXDUMP_H_

#include <stdio.h>

#include <avrtos.h>

/**
 * @brief Dump a byte in hex format
 *
 * @param offset Offset of the byte, if offset is a multiple of 16, a new line is printed
 * @param byte
 */
void hexdump_byte(uint8_t offset, unsigned char byte);

/**
 * @brief Dump a buffer in hex format
 *
 * @param data
 * @param size
 */
void hexdump(const void *data, size_t size);

#endif /* _HEXDUMP_H_ */