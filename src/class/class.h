#ifndef _DEV_CANIOT_CLASS_H_
#define _DEV_CANIOT_CLASS_H_

#include "config.h"

#include <caniot/caniot.h>
#include <caniot/classes.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

#if defined(CONFIG_CLASS0_ENABLED)

#define OC1_IDX 0u
#define OC2_IDX 1u
#define RL1_IDX 2u
#define RL2_IDX 3u

#define IN1_IDX 4u
#define IN2_IDX 5u
#define IN3_IDX 6u
#define IN4_IDX 7u

#elif defined(CONFIG_CLASS1_ENABLED)

#define PC0_IDX 0u
#define PC1_IDX 1u
#define PC2_IDX 2u
#define PC3_IDX 3u
#define PD4_IDX 4u
#define PD5_IDX 5u
#define PD6_IDX 6u
#define PD7_IDX 7u

#define EIO0_IDX 8u
#define EIO1_IDX 9u
#define EIO2_IDX 10u
#define EIO3_IDX 11u
#define EIO4_IDX 12u
#define EIO5_IDX 13u
#define EIO6_IDX 14u
#define EIO7_IDX 15u

#define PB0_IDX 16u
#define PE0_IDX 17u
#define PE1_IDX 18u

#endif

int class0_blc_command_handler(struct caniot_device *dev,
			       const unsigned char *buf,
			       uint8_t len);

int class0_blc_telemetry_handler(struct caniot_device *dev,
				 unsigned char *buf,
				 uint8_t *len);

int class0_config_apply(struct caniot_device *dev, struct caniot_device_config *config);

int class1_blc_command_handler(struct caniot_device *dev,
			       const unsigned char *buf,
			       uint8_t len);

int class1_blc_telemetry_handler(struct caniot_device *dev,
				 unsigned char *buf,
				 uint8_t *len);

int class1_config_apply(struct caniot_device *dev, struct caniot_device_config *config);

#endif /* _DEV_CANIOT_CLASS_H_ */