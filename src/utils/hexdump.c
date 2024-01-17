/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hexdump.h"

#define HEXDUMP_LINE_SIZE 16u

void hexdump_byte(uint8_t offset, unsigned char byte)
{
    if (offset % HEXDUMP_LINE_SIZE == 0)
        printf("\n");
    printf("%02X ", byte);
}

void hexdump(const void *data, size_t size)
{
    uint8_t offset = 0;
    const unsigned char *p = data;
    while (size--)
        hexdump_byte(offset++, *p++);
    printf("\n");
}