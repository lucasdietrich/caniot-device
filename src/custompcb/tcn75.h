#ifndef _TCN75_H
#define _TCN75_H

#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif

// TCN75 freq 0 (min) > 100 (typ) > 400 (max) kHz

// A2, A1, A0 = 0, 0, 1
#define TCN75_A2A1A0    0b001
#define TCN75_ADDR      (0b1001000 | TCN75_A2A1A0)

#define TCN75_TEMPERATURE_REGISTER 0
#define TCN75_CONFIG_REGISTER 1
#define TCN75_HISTERESYS_REGISTER 2
#define TCN75_SETPONT_REGISTER 3

float tcn75_temp2float(uint8_t t1, uint8_t t2);

int16_t tcn75_temp2int16(uint8_t t1, uint8_t t2);

float tcn75_int16tofloat(int16_t t);

#ifdef __cplusplus
}
#endif


#endif /* _TCN75_H */