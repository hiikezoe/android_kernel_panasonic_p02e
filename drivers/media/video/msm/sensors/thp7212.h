/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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

#include "msm_sensor.h"
#include "msm_spi_thp7212.h"

#ifndef THP7212_H
#define THP7212_H

#define USE_NEMO_BOARD 0

int32_t thp7212_sensor_config(struct msm_sensor_ctrl_t *, void __user *);
void thp7212_stop_stream(struct msm_sensor_ctrl_t *);
void thp7212_start_stream(struct msm_sensor_ctrl_t *);
int32_t thp7212_sensor_setting(struct msm_sensor_ctrl_t *,int, int);
int32_t thp7212_sensor_i2c_probe(struct i2c_client *, const struct i2c_device_id *);
int32_t thp7212_power_up(cam_id_t, struct msm_sensor_ctrl_t *);
int32_t thp7212_power_down(cam_id_t, struct msm_sensor_ctrl_t *);
int32_t thp7212_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl);
int32_t thp7212_sensor_match_id_dmy(struct msm_sensor_ctrl_t *s_ctrl);
int32_t thp7212_sensor_adjust_frame_lines_dmy(struct msm_sensor_ctrl_t *, uint16_t);
int32_t thp7212_sensor_write_exp_gain_dmy(struct msm_sensor_ctrl_t *, uint16_t, uint32_t);
void exisp_sof_notify_chk(struct v4l2_subdev *);
int thp7212_camera_reset(struct msm_sensor_ctrl_t *);
/* function for setparamter to isp */
int thp7212_sensor_set_antibanding(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_exposure(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_wb(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_bestshot_mode(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_special_effect(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_iso(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_aec_lock(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_awb_lock(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_select_caf(struct msm_camera_i2c_client *, bool);
bool imx135_is_complete_af(struct msm_sensor_ctrl_t *);
int thp7212_sensor_set_face_af(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_sequential_shooting(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_1shotaf_light(struct msm_sensor_ctrl_t *, int);
int thp7212_sensor_set_image_strength(struct msm_sensor_ctrl_t *, int);
int32_t imx135_set_af_mtr_area(struct msm_sensor_ctrl_t *, exisp_af_mtr_area_t);

#endif /* THP7212_H */
