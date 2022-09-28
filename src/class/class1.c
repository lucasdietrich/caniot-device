#include <stdint.h>

#include <avrtos/kernel.h>

#include <caniot/classes/class1.h>

#include "class.h"
#include "pulse.h"
#include "devices/temp.h"
#include "dev.h"

#if defined(CONFIG_CLASS1_ENABLED)

int class1_blc_telemetry_handler(struct caniot_device *dev,
				 char *buf,
				 uint8_t *len)
{
	return -CANIOT_ENOTSUP;
}

int class1_blc_command_handler(struct caniot_device *dev,
			       const char *buf,
			       uint8_t len)
{
	struct caniot_blc_command *const cmd = AS_BLC_COMMAND(buf);

	return dev_apply_blc_sys_command(dev, &cmd->sys);
}

int class1_config_apply(struct caniot_device *dev,
			struct caniot_config *config)
{
	return -CANIOT_ENOTSUP;
}

#endif /* CONFIG_CLASS1_ENABLED */