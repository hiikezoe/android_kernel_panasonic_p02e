/*
 * include/linux/synaptics_i2c_rmi.h - platform data structure for f75375s sensor
 *
 * Copyright (C) 2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SYNAPTICS_I2C_RMI_H
#define _LINUX_SYNAPTICS_I2C_RMI_H

#define SYNAPTICS_I2C_RMI_NAME "synaptics-rmi-ts"
#define SYNAPTICS_S7020_NAME "synaptics-s7020"

enum {
	SYNAPTICS_FLIP_X = 1UL << 0,
	SYNAPTICS_FLIP_Y = 1UL << 1,
	SYNAPTICS_SWAP_XY = 1UL << 2,
	SYNAPTICS_SNAP_TO_INACTIVE_EDGE = 1UL << 3,
};

enum {
	SYNAPTICS_POWER_OFF      = 0,
	SYNAPTICS_POWER_ON       = 1,
	SYNAPTICS_POWER_SET_PS   = 2, /* Set regulator power save mode */
	SYNAPTICS_POWER_SET_NM   = 3, /* Set regulator normal mode     */
	SYNAPTICS_POWER_DEV_UP   = 4, /* Enable device                 */
	SYNAPTICS_POWER_DEV_DOWN = 5, /* Disable device                */
	SYNAPTICS_POWER_RESTART  = 6  /* Restart system                */
};

struct synaptics_i2c_rmi_platform_data {
	uint32_t version;	/* Use this entry for panels with */
				/* (major << 8 | minor) version or above. */
				/* If non-zero another array entry follows */
	int (*power)(int mode);	/* Only valid in first array entry */
	int (*attn)(void);	/* read ATTN pin state */
	uint32_t flags;
	unsigned long irqflags;
	uint32_t inactive_left; /* 0x10000 = screen width */
	uint32_t inactive_right; /* 0x10000 = screen width */
	uint32_t inactive_top; /* 0x10000 = screen height */
	uint32_t inactive_bottom; /* 0x10000 = screen height */
	uint32_t snap_left_on; /* 0x10000 = screen width */
	uint32_t snap_left_off; /* 0x10000 = screen width */
	uint32_t snap_right_on; /* 0x10000 = screen width */
	uint32_t snap_right_off; /* 0x10000 = screen width */
	uint32_t snap_top_on; /* 0x10000 = screen height */
	uint32_t snap_top_off; /* 0x10000 = screen height */
	uint32_t snap_bottom_on; /* 0x10000 = screen height */
	uint32_t snap_bottom_off; /* 0x10000 = screen height */
	uint32_t fuzz_x; /* 0x10000 = screen width */
	uint32_t fuzz_y; /* 0x10000 = screen height */
	int fuzz_p;
	int fuzz_w;
	int8_t sensitivity_adjust;
};

#endif /* _LINUX_SYNAPTICS_I2C_RMI_H */
