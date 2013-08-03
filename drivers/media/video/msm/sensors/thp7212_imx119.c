/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
#include "thp7212.h"

#if 0
#undef CDBG
#define CDBG printk
#endif

#define SENSOR_NAME          "imx119"
#define PLATFORM_DRIVER_NAME "msm_camera_imx119"
#define imx119_obj           imx119_##obj

/* Sysctl registers */

DEFINE_MUTEX(imx119_mut);
static struct msm_sensor_ctrl_t imx119_s_ctrl;

static struct msm_camera_i2c_reg_conf imx119_start_settings[] = {
	{0xF010, 0x01},
};

static struct msm_camera_i2c_reg_conf imx119_stop_settings[] = {
	{0xF010, 0x00},
};

#define EXISP_TWO_OUTPUT

static struct msm_camera_i2c_reg_conf imx119_snap_settings[] = {
#ifdef EXISP_TWO_OUTPUT
	{0xF017, 0x02},         /* shot number 2 set */
	{0xF011, 0x41},         /* sensor mode  : multi shot */
#else
	{0xF011, 0x01},		/* sensor mode	:1 shot */
#endif
};

static struct msm_camera_i2c_reg_conf imx119_preview_settings[] = {
	{0xF011, 0x00},		/* sensor mode	:preview */
};

static struct msm_camera_i2c_reg_conf imx119_video_settings[] = {
	{0xF011, 0x00},		/* sensor mode	:preview */
};

static struct msm_camera_i2c_reg_conf imx119_recommend_settings[] = {
	{0xF029, 0x16},		/* preview size	:1280x960 */
	{0xF02A, 0x0D},		/* still size	:1280x960 */
	{0xF00B, 0x11},		/* Flicker		:60Hz */
};

static struct v4l2_subdev_info imx119_subdev_info[] = {
	{
	.code       = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt        = 1,
	.order      = 0,
	},
	/* more can be supported, to be added later */
};

/* sensor ON */
static int32_t imx119_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;

        CDBG("%s : E\n",__func__);

        rc = thp7212_power_up(THP7212_IN, s_ctrl);

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

static int32_t imx119_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;

        CDBG("%s : E\n",__func__);

        rc = thp7212_power_down(THP7212_IN, s_ctrl);

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

int32_t imx119_set_af_mtr_area(struct msm_sensor_ctrl_t *s_ctrl, 
                                exisp_af_mtr_area_t exisp_af_mtr_area)
{
        int32_t  rc = 0;
        int i = 0;

        CDBG("%s : E\n",__func__);

        if(!exisp_af_mtr_area.num_area) {
                CDBG(" %s num_area : %d \n",__func__, exisp_af_mtr_area.num_area);
                goto activation_imx119_set_af_mtr_area;
        }

        /* Mirror image */
        while (exisp_af_mtr_area.num_area > i) {
                int temp;

                exisp_af_mtr_area.mtr_area[i].x1 *= -1;
                exisp_af_mtr_area.mtr_area[i].x2 *= -1;
                temp = exisp_af_mtr_area.mtr_area[i].x1;
                exisp_af_mtr_area.mtr_area[i].x1 = 
                        exisp_af_mtr_area.mtr_area[i].x2;
                exisp_af_mtr_area.mtr_area[i].x2 = temp;

                i++;
        }

activation_imx119_set_af_mtr_area:

        rc = imx135_set_af_mtr_area(s_ctrl, exisp_af_mtr_area);

        if (0 > rc){
                pr_err("%s faild :%d\n",__func__, rc);
        }

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

static struct msm_camera_i2c_conf_array imx119_init_conf[] = {
	{&imx119_recommend_settings[0],
	ARRAY_SIZE(imx119_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array imx119_confs[] = {
	{imx119_snap_settings, 
	ARRAY_SIZE(imx119_snap_settings),    0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_preview_settings, 
	ARRAY_SIZE(imx119_preview_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_video_settings, 
	ARRAY_SIZE(imx119_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};
#if 0
static struct msm_camera_i2c_reg_conf imx119_exposure[][1] = {
	{{0xF02E, 0x00},},
	{{0xF02E, 0x01},},
	{{0xF02E, 0x02},},
	{{0xF02E, 0x03},},
	{{0xF02E, 0x04},},
	{{0xF02E, 0x05},},
	{{0xF02E, 0x06},},
	{{0xF02E, 0x07},},
	{{0xF02E, 0x08},},
	{{0xF02E, 0x09},},
	{{0xF02E, 0x0A},},
	{{0xF02E, 0x0B},},
	{{0xF02E, 0x0C},},
};

static struct msm_camera_i2c_conf_array imx119_exposure_confs[][1] = {
	{imx119_exposure[0], 	ARRAY_SIZE(imx119_exposure[0]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[1],	ARRAY_SIZE(imx119_exposure[1]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[2],	ARRAY_SIZE(imx119_exposure[2]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[3],	ARRAY_SIZE(imx119_exposure[3]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[4],	ARRAY_SIZE(imx119_exposure[4]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[5],	ARRAY_SIZE(imx119_exposure[5]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[6],	ARRAY_SIZE(imx119_exposure[6]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[7],	ARRAY_SIZE(imx119_exposure[7]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[8],	ARRAY_SIZE(imx119_exposure[8]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[9],	ARRAY_SIZE(imx119_exposure[9]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[10],	ARRAY_SIZE(imx119_exposure[10], 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[11],	ARRAY_SIZE(imx119_exposure[11], 0, MSM_CAMERA_I2C_BYTE_DATA},
	{imx119_exposure[12],	ARRAY_SIZE(imx119_exposure[12], 0, MSM_CAMERA_I2C_BYTE_DATA},	
};

static int imx119_exposure_enum_map[] = {
	MSM_V4L2_EXPOSURE_L0,
	MSM_V4L2_EXPOSURE_L1,
	MSM_V4L2_EXPOSURE_L2,
	MSM_V4L2_EXPOSURE_L3,
	MSM_V4L2_EXPOSURE_L4,
	MSM_V4L2_EXPOSURE_L5,
	MSM_V4L2_EXPOSURE_L6,
	MSM_V4L2_EXPOSURE_L7,
	MSM_V4L2_EXPOSURE_L8,
	MSM_V4L2_EXPOSURE_L9,
	MSM_V4L2_EXPOSURE_L10,
	MSM_V4L2_EXPOSURE_L11,
	MSM_V4L2_EXPOSURE_L12,
};

static struct msm_camera_i2c_enum_conf_array imx119_exposure_enum_confs = {
	.conf      = &imx119_exposure_confs[0][0],
	.conf_enum = imx119_exposure_enum_map,
	.num_enum  = ARRAY_SIZE(imx119_exposure_enum_map),
	.num_index = ARRAY_SIZE(imx119_exposure_confs),
	.num_conf  = ARRAY_SIZE(imx119_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};
#endif
struct msm_sensor_v4l2_ctrl_info_t imx119_v4l2_ctrl_info[] = {
#if 0
	{
		.ctrl_id           = V4L2_CID_EXPOSURE,
		.min               = MSM_V4L2_EXPOSURE_L0,
		.max               = MSM_V4L2_EXPOSURE_L12,
		.step              = 1,
		.enum_cfg_settings = &imx119_exposure_enum_confs,
		.s_v4l2_ctrl       = msm_sensor_s_ctrl_by_enum,
	},
#endif
};

static struct msm_sensor_output_info_t imx119_dimensions[] = {
	{	/* still 1280x960 */
		.x_output           = 0x500,
		.y_output           = 0x3C0,
		.line_length_pclk   = 0x500,
		.frame_length_lines = 0x3C0,
		.vt_pixel_clk       = 36864000,	 /* 1280x960x30 = 36864000 */
//		.op_pixel_clk       = 128000000, /* TODO 128Mhz */
		.op_pixel_clk       = 200000000, /* 200Mhz */
		.binning_factor     = 1,
	},
	{	/* preview 1280x960 */
		.x_output           = 0x500,
		.y_output           = 0x3C0,
		.line_length_pclk   = 0x500,
		.frame_length_lines = 0x3C0,
		.vt_pixel_clk       = 36864000,	 /* 1280x960x30 = 36864000 */
//		.op_pixel_clk       = 128000000, /* TODO 128Mhz */
		.op_pixel_clk       = 200000000, /* 200Mhz */
		.binning_factor     = 1,
	},
		{	/* Movie 1280x960 */
		.x_output           = 0x500,
		.y_output           = 0x3C0,
		.line_length_pclk   = 0x500,
		.frame_length_lines = 0x3C0,
		.vt_pixel_clk       = 36864000,	 /* 1280x960x30 = 36864000 */
//		.op_pixel_clk       = 128000000, /* TODO 128Mhz */
		.op_pixel_clk       = 200000000, /* 200Mhz */
		.binning_factor     = 1,
	},
};


static struct msm_camera_csid_vc_cfg imx119_cid_cfg[] = {
	{0, CSI_YUV422_8,   CSI_DECODE_8BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx119_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt    = 4,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg  = imx119_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt   = 4,
		.settle_cnt = 0x18, /* TODO correct?? */
	},
};

static struct msm_camera_csi2_params *imx119_csi_params_array[] = {
	&imx119_csi_params,
	&imx119_csi_params,
	&imx119_csi_params,
};

/* use thp7212_sensor_match_id() */
static struct msm_sensor_id_info_t imx119_id_info = {
        .sensor_id_reg_addr = 0xF003,
        .sensor_id = 0x000D,
};

static const struct i2c_device_id imx119_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx119_s_ctrl},
	{ }
};

static struct i2c_driver imx119_i2c_driver = {
	.id_table = imx119_i2c_id,
	.probe    = thp7212_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx119_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int ret;

        CDBG("%s : E\n",__func__);

	ret = i2c_add_driver(&imx119_i2c_driver);

        CDBG("%s : X %d\n",__func__, ret);

	return ret;
}

static struct v4l2_subdev_core_ops imx119_subdev_core_ops = {
	.s_ctrl    = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl     = msm_sensor_subdev_ioctl,
	.s_power   = msm_sensor_power,
};

static struct v4l2_subdev_video_ops imx119_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx119_subdev_ops = {
	.core   = &imx119_subdev_core_ops,
	.video  = &imx119_subdev_video_ops,
};


static struct msm_sensor_fn_t imx119_func_tbl = {
        .sensor_start_stream          = thp7212_start_stream,       /* stream ON */
        .sensor_stop_stream           = thp7212_stop_stream,        /* stream OFF */
        .sensor_setting               = thp7212_sensor_setting,     /* TODO initial setting */
        .sensor_set_sensor_mode       = msm_sensor_set_sensor_mode, /* sensor mode setting */
        .sensor_mode_init             = msm_sensor_mode_init,
        .sensor_get_output_info       = msm_sensor_get_output_info,
        .sensor_config                = thp7212_sensor_config,
        .sensor_power_up              = imx119_power_up,
        .sensor_power_down            = imx119_power_down,
        .sensor_match_id              = thp7212_sensor_match_id_dmy,
        .sensor_adjust_frame_lines    = thp7212_sensor_adjust_frame_lines_dmy,
        .sensor_get_csi_params        = msm_sensor_get_csi_params,
        .sensor_write_exp_gain        = thp7212_sensor_write_exp_gain_dmy,
        .exisp_sensor_set_af_mtr_area = imx119_set_af_mtr_area,
};

static struct msm_sensor_reg_t imx119_regs = {
	.default_data_type      = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf      = imx119_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx119_start_settings),
	.stop_stream_conf       = imx119_stop_settings,
	.stop_stream_conf_size  = ARRAY_SIZE(imx119_stop_settings),
	.init_settings          = &imx119_init_conf[0],
	.init_size              = ARRAY_SIZE(imx119_init_conf),
	.mode_settings          = &imx119_confs[0],
	.output_settings        = &imx119_dimensions[0],
	.num_conf               = ARRAY_SIZE(imx119_confs),
};

static struct msm_sensor_ctrl_t imx119_s_ctrl = {
	.msm_sensor_reg               = &imx119_regs,
	.msm_sensor_v4l2_ctrl_info    = imx119_v4l2_ctrl_info,
	.num_v4l2_ctrl                = ARRAY_SIZE(imx119_v4l2_ctrl_info),
	.sensor_i2c_client            = &imx119_sensor_i2c_client,
	.sensor_i2c_addr              = 0x60 << 1,	/* I2C Slave address */
	.sensor_id_info               = &imx119_id_info,
	.cam_mode                     = MSM_SENSOR_MODE_INVALID,
	.csi_params                   = &imx119_csi_params_array[0],
	.msm_sensor_mutex             = &imx119_mut,
	.sensor_i2c_driver            = &imx119_i2c_driver,
	.sensor_v4l2_subdev_info      = imx119_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx119_subdev_info),
	.sensor_v4l2_subdev_ops       = &imx119_subdev_ops,
	.func_tbl                     = &imx119_func_tbl,
	.clk_rate                     = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 1.3MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
