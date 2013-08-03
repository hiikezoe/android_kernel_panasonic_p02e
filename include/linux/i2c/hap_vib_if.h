/*
 *  hap_vib_if.h - Vibrator IF for Haptics
 *
 *  Copyright (C) 2012 Panasonic Mobile Communications
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_HAP_VIB_IF_H
#define __LINUX_HAP_VIB_IF_H

#define HAP_VIB_DEVICE_NAME	"DRV2604"

int hap_vib_amp_enable(void);
int hap_vib_amp_disable(void);
int hap_vib_vibrator_control(signed char pwm_duty);

#endif /* __LINUX_HAP_VIB_IF_H */
