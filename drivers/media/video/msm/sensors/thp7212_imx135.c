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
#include "thp7212_pm8921.h"			/* RED_LED */

#if 0
#undef CDBG
#define CDBG printk
#define CDBG_MTR 1
#endif

#define SENSOR_NAME          "imx135"
#define PLATFORM_DRIVER_NAME "msm_camera_imx135"
#define imx135_obj           imx135_##obj



/* Sysctl registers */

DEFINE_MUTEX(imx135_mut);
static struct msm_sensor_ctrl_t imx135_s_ctrl;

static struct msm_camera_i2c_reg_conf imx135_start_settings[] = {
        {0xF010, 0x01},
};

static struct msm_camera_i2c_reg_conf imx135_stop_settings[] = {
        {0xF010, 0x00},
};

#define EXISP_TWO_OUTPUT

static struct msm_camera_i2c_reg_conf imx135_snap_settings[] = {
        {0xF02A, 0x19},         /* still size   :13M */
#ifdef EXISP_TWO_OUTPUT
		{0xF017, 0x02},         /* shot number 2 set */
		{0xF011, 0x41},         /* sensor mode  : multi shot */
#else
		{0xF011, 0x01},         /* sensor mode  :1 shot */
#endif
};

/* Still Size Change::add S */
static struct msm_camera_i2c_reg_conf imx135_snap_settings_5M[] = {
        {0xF02A, 0x13},         /* still size   :5M */
#ifdef EXISP_TWO_OUTPUT
		{0xF017, 0x02},         /* shot number 2 set */
		{0xF011, 0x41},         /* sensor mode  : multi shot */
#else
		{0xF011, 0x01},         /* sensor mode  :1 shot */
#endif
};

static struct msm_camera_i2c_reg_conf imx135_snap_settings_3M[] = {
        {0xF02A, 0x10},         /* still size   :3M */
#ifdef EXISP_TWO_OUTPUT
		{0xF017, 0x02},         /* shot number 2 set */
		{0xF011, 0x41},         /* sensor mode  : multi shot */
#else
		{0xF011, 0x01},         /* sensor mode  :1 shot */
#endif
};
/* Still Size Change::add E */

static struct msm_camera_i2c_reg_conf imx135_preview_settings[] = {
        {0xF029, 0x15},         /* preview size :3M */
        {0xF02A, 0x19},         /* still size   :13M */
        {0xF011, 0x00},         /* sensor mode  :preview */
};

static struct msm_camera_i2c_reg_conf imx135_video_settings[] = {
        {0xF029, 0x10},         /* preview size :Full-HD 1920x1088 */
        {0xF011, 0x00},         /* sensor mode  :preview */
};

static struct msm_camera_i2c_reg_conf imx135_recommend_settings[] = {
        {0xF00B, 0x11},         /* Flicker              :60Hz */
};

static struct v4l2_subdev_info imx135_subdev_info[] = {
        {
        .code       = V4L2_MBUS_FMT_YUYV8_2X8,
        .colorspace = V4L2_COLORSPACE_JPEG,
        .fmt        = 1,
        .order      = 0,
        },
        /* more can be supported, to be added later */
};

/* sensor ON */
static int32_t imx135_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;

        CDBG("%s : E\n",__func__);

        rc = thp7212_power_up(THP7212_OUT, s_ctrl);

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

static int32_t imx135_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;

        CDBG("%s : E\n",__func__);

        rc = thp7212_power_down(THP7212_OUT, s_ctrl);

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

bool imx135_is_complete_af(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;
        uint16_t buff;
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;

        rc = msm_camera_i2c_read(dev_client, 0xF021, &buff,
                                         MSM_CAMERA_I2C_BYTE_DATA);
        CDBG("%s i2c_read 0xF021:0x%02x rc = %d af:%d\n",__func__,
                 buff, rc, s_ctrl->exisp.state.af);

        if (0 > rc){
                pr_err("%s _i2c_read faild :%d\n",__func__, rc);
                return TRUE;
        }

        if((0x01<<7) & buff){
                return TRUE;
        }

        return FALSE;

}

int32_t imx135_set_af_mode(struct msm_sensor_ctrl_t *s_ctrl, 
                                exisp_af_mode_type_t exisp_af_mode)
{
        int32_t rc = 0;
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;
        uint8_t buff;

        CDBG("%s : E\n",__func__);

        if(s_ctrl->exisp.state.af) {
                pr_err("%s failed %d\n", __func__, __LINE__);
                return -EFAULT;
        }

        if(s_ctrl->exisp.state.af_cont) {
                CDBG("AFD %s : %d stop_caf\n",__func__, __LINE__);
                rc = thp7212_sensor_select_caf(dev_client, FALSE);
                if (rc){
                        pr_err("%s _select_af_direct faild :%d\n",
                                __func__,rc);
                        return rc;
                }
                else {
                        s_ctrl->exisp.state.af_cont = 0;
                }
        }

        switch(exisp_af_mode) {
        case EXISP_AF_MODE_AUTO:
        case EXISP_AF_MODE_INFINITY:
        case EXISP_AF_MODE_NORMAL:
                CDBG("AFD %s : %d _MODE_AUTO\n",__func__, __LINE__);
                buff = 0x01;
                break;
        case EXISP_AF_MODE_MACRO:
                CDBG("AFD %s : %d _MODE_MACRO\n",__func__, __LINE__);
                buff = 0x02;
                break;
        default :
                pr_err("%s failed %d\n", __func__, __LINE__);
                return -EINVAL;
        }

        rc = msm_camera_i2c_write_seq(dev_client, 0xF020, &buff, 1);
        CDBG("i2c_write 0xF020:0x%02x rc = %d \n", buff,rc);

        if (0 > rc){
                pr_err("%s _i2c_write faild :%d\n",__func__, rc);
        }
        else {
                int cnt = 0;
                bool complete_af;

                do {
                        complete_af = imx135_is_complete_af(s_ctrl);

                        if(complete_af){
                                break;
                        }
                        mdelay(10);
                        cnt++;
                } while (10 > cnt);

                if(complete_af){
                        CDBG(" %s complete_af : %d ms \n",__func__, 10*cnt);
                }
                else {
                        pr_err("%s move faild wait :%d ms\n",__func__, 10*cnt);
                }

                rc = 0;
        }

        s_ctrl->exisp.af_mode = exisp_af_mode;

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

int32_t imx135_set_caf_mode(struct msm_sensor_ctrl_t *s_ctrl, 
                                exisp_caf_mode_type_t exisp_caf_mode)
{
        int32_t rc = 0;
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;

        CDBG("%s : E\n",__func__);

        if(s_ctrl->exisp.state.af) {
                pr_err("%s failed %d\n", __func__, __LINE__);
                return -EFAULT;
        }

        if (exisp_caf_mode) {
                CDBG("AFD %s : %d _MODE_CAF on \n",__func__, __LINE__);

                s_ctrl->exisp.doing.af_cont = 1;
                if(s_ctrl->exisp.state.af_cont) {
                        goto exit_imx135_set_caf_mode;
                }

                CDBG("AFD %s : %d _MODE_CAF\n",__func__, __LINE__);

                rc = thp7212_sensor_select_caf(dev_client, TRUE);

                if (rc){
                        pr_err("%s _select_af_direct faild :%d\n",__func__,rc);
                }
                else {
                        s_ctrl->exisp.state.af_cont = 1;
                }
        }
        else {
                CDBG("AFD %s : %d _MODE_CAF off \n",__func__, __LINE__);

                s_ctrl->exisp.doing.af_cont = 0;
                if(!s_ctrl->exisp.state.af_cont) {
                        goto exit_imx135_set_caf_mode;
                }

                CDBG("AFD %s : %d stop_caf\n",__func__, __LINE__);
                rc = thp7212_sensor_select_caf(dev_client, FALSE);
                if (rc){
                        pr_err("%s _select_af_direct faild :%d\n",__func__,rc);
                }
                else {
                        s_ctrl->exisp.state.af_cont = 0;
                }
        }

exit_imx135_set_caf_mode:
		s_ctrl->exisp.caf_mode = exisp_caf_mode;
        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

#define PAR_RATIO 1000

typedef struct exisp_reg_high_low_s {
        uint16_t high_addr;
        uint16_t low_addr;
} exisp_reg_high_low_t;

typedef struct exisp_af_mtr_area_regster_s {
        exisp_reg_high_low_t x1;
        exisp_reg_high_low_t y1;
        exisp_reg_high_low_t x2;
        exisp_reg_high_low_t y2;
} exisp_af_mtr_area_regster_t;

static inline uint8_t exisp_get_low_byte(uint16_t b16)
{
        uint8_t b8;

        b8 = (uint8_t)(b16 & 0xff);

        return b8;
}
static inline uint8_t exisp_get_high_byte(uint16_t b16)
{
        uint8_t b8;

        b8 = (uint8_t)((b16 >> 8) & 0xff);

        return b8;
}

static int32_t exisp_i2c_write_af_mtr_area(struct msm_camera_i2c_client *dev_client, 
                                exisp_reg_high_low_t reg_adder, uint16_t b16)
{
        int32_t  rc = 0;
        uint8_t b8;

        b8 = exisp_get_high_byte(b16);

        rc = msm_camera_i2c_write_seq(dev_client,reg_adder.high_addr, &b8, 1);
        CDBG("%s high 0x%04x : 0x%02x rc = %d \n",__func__, reg_adder.high_addr, b8, rc);

        if (0 > rc){
                pr_err("%s _i2c_write_af_mtr_area faild :%d\n",__func__,rc);
                goto exit_exisp_i2c_write_af_mtr_area;
        }

        b8 = exisp_get_low_byte(b16);

        rc = msm_camera_i2c_write_seq(dev_client,reg_adder.low_addr, &b8, 1);
        CDBG("%s low  0x%04x : 0x%02x rc = %d \n",__func__, reg_adder.low_addr, b8, rc);

        if (0 > rc){
                pr_err("%s _i2c_write_af_mtr_area faild :%d\n",__func__,rc);
        }
        else
                rc = 0;

exit_exisp_i2c_write_af_mtr_area:

        return rc;
}

int32_t imx135_set_af_mtr_area(struct msm_sensor_ctrl_t *s_ctrl, 
                                exisp_af_mtr_area_t exisp_af_mtr_area)
{
        int32_t  rc = 0;
        int x_output = s_ctrl->msm_sensor_reg->
                                output_settings[MSM_SENSOR_RES_QTR].x_output;
        int y_output = s_ctrl->msm_sensor_reg->
                                output_settings[MSM_SENSOR_RES_QTR].y_output;
        int previewWidth  = exisp_af_mtr_area.previewWidth;
        int previewHeight = exisp_af_mtr_area.previewHeight;
        int x1, y1, x2, y2;
        int i = 0;
        uint16_t x_offset_ratio = PAR_RATIO;
        uint16_t y_offset_ratio = PAR_RATIO;
        const exisp_af_mtr_area_regster_t reg_adder[EXISP_MAX_ROI] = {
        {{0xF060,0xF061},{0xF062,0xF063},{0xF064,0xF065},{0xF066,0xF067}},
        {{0xF0B0,0xF0B1},{0xF0B2,0xF0B3},{0xF0B4,0xF0B5},{0xF0B6,0xF0B7}},
        {{0xF0B8,0xF0B9},{0xF0BA,0xF0BB},{0xF0BC,0xF0BD},{0xF0BE,0xF0BF}},
        {{0xF0C0,0xF0C1},{0xF0C2,0xF0C3},{0xF0C4,0xF0C5},{0xF0C6,0xF0C7}},
        {{0xF0C8,0xF0C9},{0xF0CA,0xF0CB},{0xF0CC,0xF0CD},{0xF0CE,0xF0CF}}
        };
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;
        uint8_t buff;

        CDBG("%s : E\n",__func__);

        if(!exisp_af_mtr_area.num_area) {
                CDBG(" %s num_area : %d \n",__func__, exisp_af_mtr_area.num_area);
                goto activation_imx135_set_af_mtr_area;
        }
#if 1	/* Center AF */
        if((exisp_af_mtr_area.num_area == 1) &&
        		(exisp_af_mtr_area.mtr_area[i].x1 == 0) &&
        		(exisp_af_mtr_area.mtr_area[i].y1 == 0) &&
        		(exisp_af_mtr_area.mtr_area[i].x2 == 0) &&
        		(exisp_af_mtr_area.mtr_area[i].y2 == 0)	)
        {
        	i = 0;
        	goto activation_imx135_set_af_mtr_area;
        }
#endif
        if(MSM_SENSOR_INVALID_RES != s_ctrl->curr_res) {
                x_output = s_ctrl->msm_sensor_reg->
                               output_settings[s_ctrl->curr_res].x_output;
                y_output = s_ctrl->msm_sensor_reg->
                               output_settings[s_ctrl->curr_res].y_output;
        }

        if (1088 == y_output) {
                y_output = 1080;
        }
        if (1088 == previewHeight) {
                previewHeight = 1080;
        }

        /* output 4:3 */
        if (x_output/4*3 == y_output) {
                /* preview 16:9 */
                if (previewWidth/16*9 == previewHeight) {
                        y_offset_ratio = (previewHeight*PAR_RATIO)
                                         / (previewWidth/4*3);
                }
        }
        /* output 16:9 */
        else {
                /* preview 4:3 */
                if (previewWidth/4*3 == previewHeight) {
                        x_offset_ratio = (previewWidth*PAR_RATIO)
                                         / (previewHeight/9*16);
                }
        }

#ifdef CDBG_MTR
        {
                CDBG("output %d:%d preview %d:%d \n", x_output,
                                                      y_output,
                                                      previewWidth,
                                                      previewHeight);
                CDBG("output %d:%d preview %d:%d Center\n", x_output/2,
                                                            y_output/2,
                                                      previewWidth/2,
                                                      previewHeight/2);
                CDBG("offset_ratio %d:%d \n", x_offset_ratio, y_offset_ratio);
        }
#endif

        x_output*=PAR_RATIO;
        y_output*=PAR_RATIO;

        while (exisp_af_mtr_area.num_area > i) {

                int x1pr = (int)((exisp_af_mtr_area.mtr_area[i].x1
                           * x_offset_ratio)/PAR_RATIO);
                int y1pr = (int)((exisp_af_mtr_area.mtr_area[i].y1
                           * y_offset_ratio)/PAR_RATIO);
                int x2pr = (int)((exisp_af_mtr_area.mtr_area[i].x2
                           * x_offset_ratio)/PAR_RATIO);
                int y2pr = (int)((exisp_af_mtr_area.mtr_area[i].y2
                           * y_offset_ratio)/PAR_RATIO);

#ifdef CDBG_MTR
                {
                        CDBG("output  par %d:%d:%d:%d \n",x1pr, y1pr,
                                                          x2pr, y2pr);
                        CDBG("preview par %d:%d:%d:%d \n",
                                exisp_af_mtr_area.mtr_area[i].x1,
                                exisp_af_mtr_area.mtr_area[i].y1,
                                exisp_af_mtr_area.mtr_area[i].x2,
                                exisp_af_mtr_area.mtr_area[i].y2);

                }
#endif

                x1 = (uint16_t)(((x1pr + 1000)*(x_output/2000))
                        /PAR_RATIO);
                y1 = (uint16_t)(((y1pr + 1000)*(y_output/2000))
                        /PAR_RATIO);
                x2 = (uint16_t)(((x2pr + 1000)*(x_output/2000))
                        /PAR_RATIO);
                y2 = (uint16_t)(((y2pr + 1000)*(y_output/2000))
                        /PAR_RATIO);

#ifdef CDBG_MTR
                {
                        int pWidth  = previewWidth*PAR_RATIO;
                        int pHeight = previewHeight*PAR_RATIO;
                        int px1p = exisp_af_mtr_area.mtr_area[i].x1;
                        int py1p = exisp_af_mtr_area.mtr_area[i].y1;
                        int px2p = exisp_af_mtr_area.mtr_area[i].x2;
                        int py2p = exisp_af_mtr_area.mtr_area[i].y2;
                        int px1 = (uint16_t)(((px1p + 1000)*(pWidth /2000))/PAR_RATIO);
                        int py1 = (uint16_t)(((py1p + 1000)*(pHeight/2000))/PAR_RATIO);
                        int px2 = (uint16_t)(((px2p + 1000)*(pWidth /2000))/PAR_RATIO);
                        int py2 = (uint16_t)(((py2p + 1000)*(pHeight/2000))/PAR_RATIO);

                        CDBG("output 0x %04x:%04x:%04x:%04x \n",x1, y1, x2, y2);
                        CDBG("output  %d:%d:%d:%d %d*%d \n",x1, y1, x2, y2, x2-x1, y2-y1);
                        CDBG("preview %d:%d:%d:%d %d*%d \n",px1, py1, px2, py2, px2-px1, py2-py1);
                }
#endif

                rc = exisp_i2c_write_af_mtr_area(dev_client, reg_adder[i].x1, x1);

                if (rc){
                        pr_err("%s _i2c_write_af_mtr_area faild %d:x1 :%d\n",
                                                        __func__, i, rc);
                        goto exit_imx135_set_af_mtr_area;
                }

                rc = exisp_i2c_write_af_mtr_area(dev_client, reg_adder[i].y1, y1);

                if (rc){
                        pr_err("%s _i2c_write_af_mtr_area faild %d:y1 :%d\n",
                                                        __func__, i, rc);
                        goto exit_imx135_set_af_mtr_area;
                }

                rc = exisp_i2c_write_af_mtr_area(dev_client, reg_adder[i].x2, x2);

                if (rc){
                        pr_err("%s _i2c_write_af_mtr_area faild %d:x2 :%d\n",
                                                        __func__, i, rc);
                        goto exit_imx135_set_af_mtr_area;
                }

                rc = exisp_i2c_write_af_mtr_area(dev_client, reg_adder[i].y2, y2);

                if (rc){
                        pr_err("%s _i2c_write_af_mtr_area faild %d:y2 :%d\n",
                                                        __func__, i, rc);
                        goto exit_imx135_set_af_mtr_area;
                }

                i++;
        }

activation_imx135_set_af_mtr_area:

        buff = 0x80 + i;
        rc = msm_camera_i2c_write_seq(dev_client, 0xF023, &buff, 1);
        CDBG("i2c_write 0xF020:0x%02x rc = %d \n", buff,rc);

        if (0 > rc){
                pr_err("%s _i2c_write faild :%d\n",__func__, rc);
        }
        else
                rc = 0;

        s_ctrl->exisp.af_mtr_area = exisp_af_mtr_area;

exit_imx135_set_af_mtr_area:

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

int32_t imx135_set_led_mode(struct msm_sensor_ctrl_t *s_ctrl, 
                                exisp_led_mode_t exisp_led_mode)

{
        int32_t  rc = 0;
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;
        uint8_t buff;

        CDBG("%s : E\n",__func__);

        switch(exisp_led_mode) {
        case LED_MODE_OFF:
                CDBG("LDD %s : %d LED_MODE_OFF\n",__func__, __LINE__);
                buff = 0x00;
                break;
        case LED_MODE_AUTO:
                CDBG("LDD %s : %d LED_MODE_AUTO\n",__func__, __LINE__);
                buff = 0x01;
                break;
        case LED_MODE_ON:
                CDBG("LDD %s : %d LED_MODE_ON\n",__func__, __LINE__);
                buff = 0x02;
                break;
        case LED_MODE_TORCH:
                CDBG("LDD %s : %d LED_MODE_TORCH\n",__func__, __LINE__);
                buff = 0x03;
                break;
        default :
                pr_err("%s failed %d\n", __func__, __LINE__);
                rc = -EINVAL;
                goto exit_imx135_set_led_mode;
        }

        rc = msm_camera_i2c_write_seq(dev_client, 0xF089, &buff, 1);
        CDBG("i2c_write 0xF020:0x%02x rc = %d \n", buff,rc);

        if (0 > rc){
                pr_err("%s _i2c_write faild 0xF089 :%d\n",__func__, rc);
                goto exit_imx135_set_led_mode;
        }

        s_ctrl->exisp.led_mode = exisp_led_mode;

exit_imx135_set_led_mode:

        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

static struct msm_camera_i2c_conf_array imx135_init_conf[] = {
        {&imx135_recommend_settings[0],
        ARRAY_SIZE(imx135_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array imx135_confs[] = {
        {imx135_snap_settings, 
        ARRAY_SIZE(imx135_snap_settings),    0, MSM_CAMERA_I2C_BYTE_DATA},
        {imx135_preview_settings, 
        ARRAY_SIZE(imx135_preview_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
        {imx135_video_settings,
        ARRAY_SIZE(imx135_video_settings),   0, MSM_CAMERA_I2C_BYTE_DATA},
        {imx135_snap_settings_5M, 
        ARRAY_SIZE(imx135_snap_settings_5M), 0, MSM_CAMERA_I2C_BYTE_DATA},	// Still Size Change::add
        {imx135_snap_settings_3M, 
        ARRAY_SIZE(imx135_snap_settings_3M), 0, MSM_CAMERA_I2C_BYTE_DATA}	// Still Size Change::add
};

struct msm_sensor_v4l2_ctrl_info_t imx135_v4l2_ctrl_info[] = {
#if 0
        {
                .ctrl_id           = CFG_SET_EXPOSURE_COMPENSATION,
                .min               = MSM_V4L2_EXPOSURE_L0,
                .max               = MSM_V4L2_EXPOSURE_L12,
                .step              = 1,
                .enum_cfg_settings = &imx135_exposure_enum_confs,
                .s_v4l2_ctrl       = msm_sensor_s_ctrl_by_enum,
        },
#endif
};

static struct msm_sensor_output_info_t imx135_dimensions[] = {
        {       /* still 4160x3120 */
                        .x_output           = 0x1040,
                        .y_output           = 0xC30,
                        .line_length_pclk   = 0x1040,
                        .frame_length_lines = 0xC30,
                        .vt_pixel_clk       = 194688000, /* 4160x3120x15 = 194688000 */
                        .op_pixel_clk       = 320000000, /* TODO 400Mhz */
                        .binning_factor     = 1,
        },
        {       /* preview 2048x1536 */
                .x_output           = 0x800,
                .y_output           = 0x600,
                .line_length_pclk   = 0x800,
                .frame_length_lines = 0x600,
                .vt_pixel_clk       = 94371840,  /* 2048x1536x30 = 94371840 */
                .op_pixel_clk       = 320000000, /* TODO 216Mhz */
                .binning_factor     = 1,
        },
        {       /* Full-HD preview 1920x1088 */
                .x_output           = 0x780,
                .y_output           = 0x440,
                .line_length_pclk   = 0x780,
                .frame_length_lines = 0x440,
                .vt_pixel_clk       = 62668800,  /* 1920x1088x30 = 62668800 */
                .op_pixel_clk       = 320000000, /* TODO 216Mhz */
                .binning_factor     = 1,
        },
/* Still Size Change::add S */
        {       /* still 5M 2592x1944 */
                .x_output           = 0xA20,
                .y_output           = 0x798,
                .line_length_pclk   = 0xA20,
                .frame_length_lines = 0x798,
                .vt_pixel_clk       = 75582720,  /* 2592x1944x15 = 75582720 */
                .op_pixel_clk       = 320000000, /* TODO 400Mhz */
                .binning_factor     = 1,
        },
        {       /* still 3M 2048x1536 */
                .x_output           = 0x800,
                .y_output           = 0x600,
                .line_length_pclk   = 0x800,
                .frame_length_lines = 0x600,
                .vt_pixel_clk       = 94371840,  /* 2048x1536x30 = 94371840 */
                .op_pixel_clk       = 320000000, /* TODO 400Mhz */
                .binning_factor     = 1,
        },
/* Still Size Change::add E */
};

static struct msm_camera_csid_vc_cfg imx135_cid_cfg[] = {
        {0, CSI_YUV422_8,   CSI_DECODE_8BIT},
        {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx135_csi_params = {
        .csid_params = {
                .lane_assign = 0xe4,
                .lane_cnt    = 4,
                .lut_params = {
                        .num_cid = 2,
                        .vc_cfg  = imx135_cid_cfg,
                },
        },
        .csiphy_params = {
                .lane_cnt   = 4,
                .settle_cnt = 0x18, /* TODO:Q This setting value is correct? */
        },
};

static struct msm_camera_csi2_params *imx135_csi_params_array[] = {
        &imx135_csi_params, /* Full scan */
        &imx135_csi_params, /* still preview */
        &imx135_csi_params, /* Movie Full-HD preview */
        &imx135_csi_params, /* 5M */	//Still Size Change::add
        &imx135_csi_params, /* 3M */	//Still Size Change::add
};

/* use thp7212_sensor_match_id() */
static struct msm_sensor_id_info_t imx135_id_info = {
        .sensor_id_reg_addr = 0xF003,
        .sensor_id = 0x000D,
};

static const struct i2c_device_id imx135_i2c_id[] = {
        {SENSOR_NAME, (kernel_ulong_t)&imx135_s_ctrl},
        { }
};

static struct i2c_driver imx135_i2c_driver = {
        .id_table = imx135_i2c_id,
        .probe    = thp7212_sensor_i2c_probe,
        .driver = {
                .name = SENSOR_NAME,
        },
};

static struct msm_camera_i2c_client imx135_sensor_i2c_client = {
        .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
        int ret;

        CDBG("%s : E\n",__func__);

        ret = i2c_add_driver(&imx135_i2c_driver);

        CDBG("%s : X %d\n",__func__, ret);

        return ret;
}

static struct v4l2_subdev_core_ops imx135_subdev_core_ops = {
        .s_ctrl    = msm_sensor_v4l2_s_ctrl,
        .queryctrl = msm_sensor_v4l2_query_ctrl,
        .ioctl     = msm_sensor_subdev_ioctl,
        .s_power   = msm_sensor_power,
};

static struct v4l2_subdev_video_ops imx135_subdev_video_ops = {
        .enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx135_subdev_ops = {
        .core   = &imx135_subdev_core_ops,
        .video  = &imx135_subdev_video_ops,
};

static struct msm_sensor_fn_t imx135_func_tbl = {
        .sensor_start_stream          = thp7212_start_stream,       /* stream ON */
        .sensor_stop_stream           = thp7212_stop_stream,        /* stream OFF */
        .sensor_setting               = thp7212_sensor_setting,     /* TODO initial setting */
        .sensor_set_sensor_mode       = msm_sensor_set_sensor_mode, /* sensor mode setting */
        .sensor_mode_init             = msm_sensor_mode_init,
        .sensor_get_output_info       = msm_sensor_get_output_info,
        .sensor_config                = thp7212_sensor_config,
        .sensor_power_up              = imx135_power_up,
        .sensor_power_down            = imx135_power_down,
        .sensor_match_id              = thp7212_sensor_match_id_dmy,
        .sensor_adjust_frame_lines    = thp7212_sensor_adjust_frame_lines_dmy,
        .sensor_get_csi_params        = msm_sensor_get_csi_params,
        .sensor_write_exp_gain        = thp7212_sensor_write_exp_gain_dmy,
        .exisp_sensor_set_af_mode     = imx135_set_af_mode,
        .exisp_sensor_set_caf_mode    = imx135_set_caf_mode,
        .exisp_sensor_set_af_mtr_area = imx135_set_af_mtr_area,
        .exisp_sensor_set_led_mode    = imx135_set_led_mode,
};

static struct msm_sensor_reg_t imx135_regs = {
        .default_data_type      = MSM_CAMERA_I2C_BYTE_DATA,
        .start_stream_conf      = imx135_start_settings,
        .start_stream_conf_size = ARRAY_SIZE(imx135_start_settings),
        .stop_stream_conf       = imx135_stop_settings,
        .stop_stream_conf_size  = ARRAY_SIZE(imx135_stop_settings),
        .init_settings          = &imx135_init_conf[0],
        .init_size              = ARRAY_SIZE(imx135_init_conf),
        .mode_settings          = &imx135_confs[0],
        .output_settings        = &imx135_dimensions[0],
        .num_conf               = ARRAY_SIZE(imx135_confs),
};

static struct msm_sensor_ctrl_t imx135_s_ctrl = {
        .msm_sensor_reg               = &imx135_regs,
        .msm_sensor_v4l2_ctrl_info    = imx135_v4l2_ctrl_info,
        .num_v4l2_ctrl                = ARRAY_SIZE(imx135_v4l2_ctrl_info),
        .sensor_i2c_client            = &imx135_sensor_i2c_client,
        .sensor_i2c_addr              = 0x60 << 1, /* I2C Slave address */
        .sensor_id_info               = &imx135_id_info,
        .cam_mode                     = MSM_SENSOR_MODE_INVALID,
        .csi_params                   = &imx135_csi_params_array[0],
        .msm_sensor_mutex             = &imx135_mut,
        .sensor_i2c_driver            = &imx135_i2c_driver,
        .sensor_v4l2_subdev_info      = imx135_subdev_info,
        .sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx135_subdev_info),
        .sensor_v4l2_subdev_ops       = &imx135_subdev_ops,
        .func_tbl                     = &imx135_func_tbl,
#if USE_NEMO_BOARD
        .clk_rate                     = MSM_SENSOR_MCLK_19HZ,
#else
        .clk_rate                     = MSM_SENSOR_MCLK_24HZ,
#endif
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 13MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
