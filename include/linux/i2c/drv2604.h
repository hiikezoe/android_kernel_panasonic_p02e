/*
 *  drv2604.h - DRV2604 Haptic Motor driver
 *
 *  Copyright (C) 2012 Panasonic Mobile Communications
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_DRV2604_H
#define __LINUX_DRV2604_H

#define DRV_I2C_VTG_MAX_UV		1800000
#define DRV_I2C_VTG_MIN_UV		1800000
#define DRV_I2C_CURR_UA			9630

struct drv2604_regulator {
	const char *name;
	u32	min_uV;
	u32	max_uV;
	u32	load_uA;
};

struct drv2604_platform_data {
	const char *name;
	unsigned int pwm_ch_id; /* pwm channel id */
	unsigned int max_timeout;
	unsigned int hap_en_gpio;
	unsigned int hap_inttrig_gpio;
	struct drv2604_regulator *regulator_info;
	u8 num_regulators;
	int          duty;
};

#endif /* __LINUX_ISA1200_H */
