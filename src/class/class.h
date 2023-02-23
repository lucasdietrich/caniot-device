#ifndef _DEV_CANIOT_CLASS_H_
#define _DEV_CANIOT_CLASS_H_

#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

int class0_blc_command_handler(struct caniot_device *dev, const unsigned char *buf, uint8_t len);

int class0_blc_telemetry_handler(struct caniot_device *dev, unsigned char *buf, uint8_t *len);

int class0_config_apply(struct caniot_device *dev, struct caniot_device_config *config);

int class1_blc_command_handler(struct caniot_device *dev, const unsigned char *buf, uint8_t len);

int class1_blc_telemetry_handler(struct caniot_device *dev, unsigned char *buf, uint8_t *len);

int class1_config_apply(struct caniot_device *dev, struct caniot_device_config *config);

#endif /* _DEV_CANIOT_CLASS_H_ */