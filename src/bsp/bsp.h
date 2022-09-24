#ifndef _BSP_H_
#define _BSP_H_

#include <stdint.h>

/* Board version
 * 1: v1 (custompcb) : TCN75, integrated Relay, OpenCollector
 * 2: tiny : TCN75, extended GPIO
 */

#if defined(CONFIG_BOARD_V1)
#include "v1/v1.h"
#elif defined(CONFIG_BOARD_TINY)
#include "tiny/tiny.h"
#else
#error "Invalid board version"
#endif

/*____________________________________________________________________________*/

#ifdef __cplusplus
extern "C" {
#endif

void bsp_init(void);

#ifdef __cplusplus
}
#endif

/*____________________________________________________________________________*/

/* V1 */
typedef enum {
	OC1 = 0,
	OC2,
	RL1,
	RL2,
} output_t;

typedef enum {
	IN1 = 0,
	IN2 = 1,
	IN3 = 2,
	IN4 = 3
} input_t;

#define OPENCOLLECTOR1 0
#define OPENCOLLECTOR2 1
#define RELAY1 2
#define RELAY2 3

#define INPUT1 0
#define INPUT2 1
#define INPUT3 2
#define INPUT4 3

/*____________________________________________________________________________*/

/* Tiny */

#endif /* _BSP_H_ */