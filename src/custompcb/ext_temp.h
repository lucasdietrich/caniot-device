#ifndef _ALARM_CONTROLLER_EXT_TEMP_H_
#define _ALARM_CONTROLLER_EXT_TEMP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ow_ext_get(int16_t *temp);

#ifdef __cplusplus
}
#endif

#endif /* _ALARM_CONTROLLER_EXT_TEMP_H_ */