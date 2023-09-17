#include "class/class.h"
#include "config.h"
#include "settings.h"

#include <stdint.h>
#include <time.h>

#include <avrtos/avrtos.h>
#include <avrtos/logging.h>

#include <avr/eeprom.h>
#include <caniot/caniot.h>
#include <caniot/device.h>

#define LOG_LEVEL CONFIG_DEVICE_LOG_LEVEL

#define SETTINGS_BLOCK_SIZE sizeof(struct caniot_device_config)

/* Tell whether the initial configuration needs to be applied or not */
static bool init_config_to_apply = true;

/* Default configuration */
extern struct caniot_device_config default_config;

__attribute__((section(".noinit"))) struct caniot_device_config settings_rambuf;
__STATIC_ASSERT(sizeof(settings_rambuf) <= 0xFF,
		"config too big"); /* EEPROM size depends on MCU */

// compute CRC8
static uint8_t checksum_crc8(const uint8_t *buf, size_t len)
{
	uint8_t crc = 0;

	while (len--) {
		uint8_t inbyte = *buf++;
		uint8_t i;

		for (i = 0x80; i > 0; i >>= 1) {
			uint8_t mix = (crc ^ inbyte) & i;
			crc	    = (crc >> 1) ^ (mix ? 0x8C : 0x00);
		}
	}

	return crc;
}

static int
settings_apply(struct caniot_device *dev, struct caniot_device_config *cfg)
{
	set_zone(cfg->timezone);

	switch (__DEVICE_CLS__) {
	case CANIOT_DEVICE_CLASS0:
		return class0_config_apply(dev, cfg, init_config_to_apply);
	case CANIOT_DEVICE_CLASS1:
		return class1_config_apply(dev, cfg, init_config_to_apply);
	default:
		return -CANIOT_ENOTSUP;
	}
}

/**
 * @brief Indicates whether the configuration is still valid or not.
 * TODO can be removed
 */
static bool settings_dirty = true;

int settings_read(struct caniot_device *dev, struct caniot_device_config *cfg)
{
	if (settings_dirty == true) {
		const uint8_t actual_checksum = eeprom_read_byte(0x0000U);

		eeprom_read_block(cfg, (const void *)0x0001U, SETTINGS_BLOCK_SIZE);

		uint8_t calculated_checksum =
			checksum_crc8((const uint8_t *)cfg, SETTINGS_BLOCK_SIZE);

		if (actual_checksum != calculated_checksum) {
			return -EINVAL;
		}

		settings_dirty = false;
	}

	return 0;
}

int settings_write(struct caniot_device *dev, struct caniot_device_config *cfg)
{
	eeprom_update_block((const void *)cfg, (void *)0x0001U, SETTINGS_BLOCK_SIZE);

	const uint8_t calculated_checksum =
		checksum_crc8((const uint8_t *)cfg, SETTINGS_BLOCK_SIZE);

	eeprom_update_byte((uint8_t *)0x0000U, calculated_checksum);

	settings_dirty = true;

	return settings_apply(dev, cfg);
}

int settings_restore_default(struct caniot_device *dev)
{
	memcpy_P(&settings_rambuf, &default_config, SETTINGS_BLOCK_SIZE);

	return settings_write(dev, &settings_rambuf);
}

#if CONFIG_FORCE_RESTORE_DEFAULT_CONFIG
#warning "CONFIG_FORCE_RESTORE_DEFAULT_CONFIG" is enabled
#endif

void settings_load(struct caniot_device *dev)
{
	bool restore = false;

	if (CONFIG_FORCE_RESTORE_DEFAULT_CONFIG == 0) {
		/* sanity check on EEPROM */
		if (settings_read(dev, &settings_rambuf) != 0) {
			restore = true;
		}
	}

	/* if restore is true, we copy the default configuration to EEPROM and RAM */
	if (restore || (CONFIG_FORCE_RESTORE_DEFAULT_CONFIG == 1)) {

		LOG_DBG("Config reset ...");
		memcpy_P(&settings_rambuf,
			 &default_config,
			 sizeof(struct caniot_device_config));

		settings_write(dev, &settings_rambuf);
	} else {
		settings_apply(dev, &settings_rambuf);
	}

	/* the initial configuration has been applied */
	init_config_to_apply = false;
}