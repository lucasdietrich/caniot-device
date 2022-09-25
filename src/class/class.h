#ifndef _DEV_CANIOT_CLASS_H_
#define _DEV_CANIOT_CLASS_H_

#include <caniot/caniot.h>
#include <caniot/device.h>
#include <caniot/datatype.h>

int class0_blc_command_handler(struct caniot_device *dev,
			       const char *buf,
			       uint8_t len);

int class0_blc_telemetry_handler(struct caniot_device *dev,
				 char *buf,
				 uint8_t *len);

int class0_config_apply(struct caniot_device *dev,
			struct caniot_config *config);

int class1_blc_command_handler(struct caniot_device *dev,
			       const char *buf,
			       uint8_t len);

int class1_blc_telemetry_handler(struct caniot_device *dev,
				 char *buf,
				 uint8_t *len);

int class1_config_apply(struct caniot_device *dev,
			struct caniot_config *config);

#endif /* _DEV_CANIOT_CLASS_H_ */