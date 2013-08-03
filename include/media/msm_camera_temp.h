/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __LINUX_MSM_CAMERA_TEMP_H
#define __LINUX_MSM_CAMERA_TEMP_H

#define MSM_CAM_TEMP_OK 0
#define MSM_CAM_TEMP_ERR -1
#define MSM_CAM_TEMP_POWEROFF -2

int msm_camera_temperature_get( int *temp );



#endif /* __LINUX_MSM_CAMERA_TEMP_H */
