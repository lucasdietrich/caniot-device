#include "temp.h"

#include <caniot/datatype.h>

#include "ow_ds_drv.h"
#include "ow_ds_meas.h"
#include "tcn75.h"



// 0x28 0xd4 0x39 0xb8 0x32 0x20 0x01 0xf2 
// 0x28 0x2a 0x06 0x41 0x33 0x20 0x01 0x31
// 0x28 0x3d 0x72 0xbf 0x32 0x20 0x01 0x52

#define OW_DS_SN_NONE() \
	{ \
		.registered = 0U, \
	}

#define OW_DS_SN_REGISTER(sn_array) \
	{ \
		.id = { \
			.addr = sn_array, \
		}, \
		.registered = 1U, \
	}

#if CONFIG_OW_DS_ENABLED
/* use serial numbers to order sensors */
ow_ds_sensor_t sensors[CONFIG_OW_DS_COUNT] = {
#if defined(CONFIG_OW_DS_SN_1) && (CONFIG_OW_DS_COUNT >= 1)
	OW_DS_SN_REGISTER(CONFIG_OW_DS_SN_1),
#elif (CONFIG_OW_DS_COUNT >= 1)
	OW_DS_SN_NONE(),
#endif

#if defined(CONFIG_OW_DS_SN_2) && (CONFIG_OW_DS_COUNT >= 2)
	OW_DS_SN_REGISTER(CONFIG_OW_DS_SN_2),
#elif (CONFIG_OW_DS_COUNT >= 2)
	OW_DS_SN_NONE(),
#endif

#if defined(CONFIG_OW_DS_SN_3) && (CONFIG_OW_DS_COUNT >= 3)
	OW_DS_SN_REGISTER(CONFIG_OW_DS_SN_3),
#elif (CONFIG_OW_DS_COUNT >= 3)
	OW_DS_SN_NONE(),
#endif
};
#endif

void temp_init(void)
{
	tcn75_init();

#if CONFIG_OW_DS_ENABLED
	/* initialize OW */
	ds_init(OW_DS_ARDUINO_PIN, sensors, ARRAY_SIZE(sensors));

	ds_discover();
	ds_measure_all();
	ds_meas_start(CONFIG_OW_DS_PROCESS_PERIOD_MS);
#endif
}

int16_t temp_read(temp_sens_t sensor)
{
	int16_t temp = CANIOT_DT_T16_INVALID;

	if (sensor == TEMP_SENS_INT) {
		/**
		 * @brief TCN75 is in continuous measurement mode, so 
		 * reading the temperature is "instantaneous".
		 */
		temp = tcn75_read();
	} else if (CONFIG_OW_DS_ENABLED) {
		/* as threads a cooperative, we don't need 
		 * a mutex to protect temperature values 
		 */
		const uint8_t index = sensor - TEMP_SENS_EXT_1;
		if ((index < ARRAY_SIZE(sensors)) && sensors[index].valid) {
			temp = sensors[index].temp;
		}
	}

	return temp;
}