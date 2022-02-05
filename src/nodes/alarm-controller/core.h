#ifndef _ALARM_CONTROLLER_CORE_H_
#define _ALARM_CONTROLLER_CORE_H_

#include <stdint.h>
#include <stdbool.h>

#include <avrtos/kernel.h>
#include <datatype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OUTDOOR_LIGHT_1 	OC1
#define OUTDOOR_LIGHT_2 	OC2

/* tells who was the last to control the lights */
enum ctrl_source
{
	SRC_USER = 0U,
	SRC_ALARM_CONTROLLER = 1U,
};

void commands_lights_from(uint8_t oc, caniot_light_cmd_t cmd, enum ctrl_source src);

#ifdef __cplusplus
}
#endif

#endif /* _ALARM_CONTROLLER_ALARM_H_ */