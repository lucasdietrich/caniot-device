#ifndef _TEMP_H_
#define _TEMP_H_

#include <stdint.h>

typedef enum {
	TEMP_SENS_INT = 0,
	TEMP_SENS_EXT_1 = 1,
	TEMP_SENS_EXT_2 = 2,
	TEMP_SENS_EXT_3 = 3,
} temp_sens_t;

void temp_init(void);

int16_t temp_read(temp_sens_t sensor);

#endif /* _TEMP_MGMT_H_ */