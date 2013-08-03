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

#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/spinlock.h>
#include <linux/videodev2.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/android_pmem.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/gpio.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/msm_isp.h>
#if 1  /* EXISP code cfglog */
#include <linux/cfgdrv.h>
#include <linux/rtc.h>
#endif /* EXISP code cfglog */

#include "msm_sensor.h"
#include "msm.h"
#include "msm_ispif.h"
#include "msm_camera_i2c_mux.h"
#include "msm_spi_thp7212.h"
#include "thp7212.h"
#include "thp7212_pm8921.h"			/* RED_LED */
#include "media/msm_camera_temp.h"		/* sensor_temperature_get */

#if 0
#undef CDBG
#define CDBG printk
#endif

#if 1	/* exisp_code npdc300053106  */
extern int exisp_err_flag;
#endif

static struct msm_sensor_ctrl_t *g_s_ctrl;
static int g_res= MSM_SENSOR_RES_QTR;

enum exisp_antibanding_freq_status{
	EXISP_ANTIBANDING_STATUS_50HZ,
	EXISP_ANTIBANDING_STATUS_60HZ,
};
static uint8_t flicker_status = EXISP_ANTIBANDING_STATUS_60HZ;

static void __iomem *base;	/* npdc300060875 */

#define CAMERA_AE_AWB_STATUS_MAX 13
static uint8_t ae_awb_status[CAMERA_AE_AWB_STATUS_MAX] = {0,0,0,0,0,0,0,0,0,0,0,0,0,};


/* for exposure compensation */
#define CAMERA_EXPOSURE_MAX	6
#define CAMERA_EXPOSURE_MIN	-6

/* message id for exisp_subdev_notify*/
enum msm_camera_exisp_subdev_notify {
        NOTIFY_EXISP_AF,
        NOTIFY_EXISP_INVALID
};

/*
 * This function executes in interrupt context.
 */
static int exisp_notify(struct v4l2_subdev *sd,
        unsigned int notification,  void *arg)
{
        int rc = 0;
        struct v4l2_event v4l2_evt;
        struct msm_isp_event_ctrl *isp_event;
        struct msm_cam_media_controller *pmctl =
                (struct msm_cam_media_controller *)v4l2_get_subdev_hostdata(sd);

//        CDBG("%s : E\n",__func__);

        if (!pmctl) {
                pr_err("%s: no context in dsp callback.\n", __func__);
                rc = -EINVAL;
                return rc;
        }

        isp_event = kzalloc(sizeof(struct msm_isp_event_ctrl), GFP_ATOMIC);
        if (!isp_event) {
                pr_err("%s Insufficient memory. return", __func__);
                return -ENOMEM;
        }

        v4l2_evt.type = V4L2_EVENT_PRIVATE_START +
                                        MSM_CAM_RESP_EXISP_EVT_MSG;

        v4l2_evt.id = 0;

        *((uint32_t *)v4l2_evt.u.data) = (uint32_t)isp_event;

        isp_event->resptype = MSM_CAM_RESP_EXISP_EVT_MSG;
        isp_event->isp_data.isp_msg.type = MSM_CAMERA_MSG;
        isp_event->isp_data.isp_msg.len = 0;

        switch (notification) {
        case NOTIFY_EXISP_AF: {
//              struct isp_msg_event *isp_msg = (struct isp_msg_event *)arg;

                isp_event->isp_data.isp_msg.msg_id = MSG_ID_STATS_AF;
                getnstimeofday(&(isp_event->isp_data.isp_msg.timestamp));
                break;
        }
        default:
                pr_err("%s: Unsupport isp notification %d\n",
                        __func__, notification);
                rc = -EINVAL;
                break;
        }

        v4l2_event_queue(pmctl->config_device->config_stat_event_queue.pvdev,
                         &v4l2_evt);

//        CDBG("%s : X\n",__func__);

        return rc;
}

static int g_af_cnt = 0;

void exisp_sof_notify_chk(struct v4l2_subdev *sd)
{
        struct msm_sensor_ctrl_t *s_ctrl;

        s_ctrl = g_s_ctrl;

        spin_lock(&s_ctrl->exisp.slock);
        if (s_ctrl->exisp.state.af) {

                if (50 < g_af_cnt) {
                        CDBG("%s thp7212_ : X g_af_cnt = %d\n",__func__, g_af_cnt);
                        s_ctrl->exisp.state.af = 0;
                        g_af_cnt = 0;
                }

                exisp_notify(sd, NOTIFY_EXISP_AF, NULL);
                g_af_cnt++;
        }
        spin_unlock(&s_ctrl->exisp.slock);

        return;
}
static int thp7212_sensor_state_af(struct msm_sensor_ctrl_t *s_ctrl, exisp_state_t *state)
{
        int32_t rc = 0;
        bool complete_af;
        unsigned long flags = 0;

//        CDBG("%s : E\n",__func__);

        complete_af = imx135_is_complete_af(s_ctrl);

        state->af = 0;

        spin_lock_irqsave(&s_ctrl->exisp.slock, flags);
        if (s_ctrl->exisp.state.af) {

                if(complete_af){
                        state->af = 1;
                        s_ctrl->exisp.state.af = 0;
                }
        }
        else {
                state->af = 1;
        }
        spin_unlock_irqrestore(&s_ctrl->exisp.slock, flags);

        CDBG("%s af = %d g_af_cnt = %d\n",__func__, state->af, g_af_cnt);

//        CDBG("%s : X\n",__func__);

        return rc;
}
int thp7212_sensor_select_caf(struct msm_camera_i2c_client *dev_client, bool af_cont)
{
        int32_t rc = 0;
        uint8_t buff;

        CDBG("%s : E\n",__func__);

        /* address : F022h
         * 01h : 1shot AF
         * 11h : Continuous AF arbitrary position
         */
        if(af_cont) {
                buff = 0x11;
        }
        else {
                buff = 0x01;
        }

        rc = msm_camera_i2c_write_seq(dev_client, 0xF022, &buff, 1);
        CDBG("i2c_write 0xF022:0x%02x rc = %d \n", buff,rc);

        if (0 > rc){
                pr_err("%s _i2c_write faild :%d\n",__func__, rc);
        }
        else {
                int cnt = 0;
                bool complete_caf = FALSE;
                uint16_t rbuff;

                do {

                        /* address : F025h
                         * Focus setting confirmation
                         * Auto focus selection is set to F022h
                         */
                        rc = msm_camera_i2c_read(dev_client, 0xF025, &rbuff,
                                                 MSM_CAMERA_I2C_BYTE_DATA);
                        CDBG("%s i2c_read 0xF025:0x%02x rc = %d\n",__func__,
                                                                rbuff, rc);

                        if (0 > rc){
                                pr_err("%s _i2c_read faild :%d\n",__func__, rc);
                                goto exit_thp7212_sensor_select_caf;
                        }

                        if (rbuff == (uint16_t)buff) {
                                complete_caf = TRUE;
                                break;
                        }

                        mdelay(10);
                        cnt++;
                } while (20 > cnt);

                if(complete_caf){
                        CDBG(" %s complete_caf : %d ms \n",__func__, 10*cnt);
                }
                else {
                        pr_err("%s caf faild wait :%d ms\n",__func__, 10*cnt);
                }

                rc = 0;
        }

exit_thp7212_sensor_select_caf:

        CDBG("%s : X\n",__func__);

        return rc;
}
static int thp7212_sensor_start_af(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;
        uint8_t buff;

        CDBG("%s : E\n",__func__);

        if(s_ctrl->exisp.state.af_cont) {
                s_ctrl->exisp.doing.af_cont = 0;
                rc = thp7212_sensor_select_caf(dev_client, FALSE);
        }

        if (rc){
                pr_err("%s _select_af_direct faild :%d\n",__func__,rc);
                return rc;
        }

        /* address : F01Fh
         * 01h : Normal
         * 09h : Quick AF
         */
        if(s_ctrl->exisp.state.af_cont) {
                buff = 0x09;
        }
        else {
                buff = 0x01;
        }

        rc = msm_camera_i2c_write_seq(dev_client, 0xF01F, &buff, 1);
        CDBG("i2c_write 0xF01F:0x%02x rc = %d \n", buff,rc);

        if (!rc){
                unsigned long flags = 0;
                spin_lock_irqsave(&s_ctrl->exisp.slock, flags);
                s_ctrl->exisp.state.af = 1;
                g_af_cnt = 0;
                spin_unlock_irqrestore(&s_ctrl->exisp.slock, flags);
        }

        CDBG("%s : X\n",__func__);

        return rc;
}
static int thp7212_sensor_cancel_af(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;
        bool doing_af = FALSE;
        unsigned long flags = 0;
        struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;

        CDBG("%s : E\n",__func__);

        spin_lock_irqsave(&s_ctrl->exisp.slock, flags);
        if (s_ctrl->exisp.state.af) {
                s_ctrl->exisp.state.af = 0;
                doing_af = TRUE;
        }
        spin_unlock_irqrestore(&s_ctrl->exisp.slock, flags);

//        if (doing_af) 
        {
                uint8_t buff;

                /* address : F01Fh
                 * 80h : AF Forced Stop
                 */
                buff = 0x80;
                rc = msm_camera_i2c_write_seq(dev_client, 0xF01F, &buff, 1);
                CDBG("i2c_write 0xF01F:0x%02x rc = %d \n", buff,rc);

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
                                mdelay(50);
                                cnt++;
                        } while (5 > cnt);

                        if(complete_af){
                                CDBG(" %s complete_af : %d ms \n",__func__, 50*cnt);
                        }
                        else {
                                pr_err("%s cansel faild wait :%d ms\n",__func__, 50*cnt);
                        }

                        rc = 0;
                }
        }

        if (s_ctrl->exisp.state.af_cont) {
                rc = thp7212_sensor_select_caf(dev_client, TRUE);

                if (rc){
                        pr_err("%s _select_af_direct faild :%d\n",__func__,rc);
                }
        }

        CDBG("%s : X\n",__func__);

        return rc;
}

static int thp7212_sensor_stop_af(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;

        CDBG("%s : E\n",__func__);

	rc = thp7212_sensor_cancel_af(s_ctrl);

        if (rc){
                pr_err("%s _cancel_af faild :%d\n",__func__,rc);
        }
        CDBG("%s : X\n",__func__);

        return rc;
}

/* EXISP Code::add S */
static int thp7212_sensor_set_direction(struct msm_sensor_ctrl_t *s_ctrl, uint8_t direction)
{
	int32_t rc = 0;

	CDBG("%s: Enter\n",__func__);

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xF01B, (uint16_t)direction, MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->exisp.direction = direction;
	CDBG("%s: Exit %d  writedata(%d)\n",__func__,rc,direction);
	return rc;
}

static int thp7212_sensor_set_hdrmode(struct msm_sensor_ctrl_t *s_ctrl, uint8_t hdrmode)
{
	int32_t rc = 0;
	uint16_t read_data;
	uint16_t write_data = 0;

	CDBG("%s: Enter hdrmode=%d\n",__func__,hdrmode);

	if (s_ctrl->exisp.hdr_mode != hdrmode) {
		s_ctrl->exisp.hdr_mode = hdrmode;
		rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF010, &read_data, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
			return rc;
		}
		if (read_data == 0x01) {
			if (s_ctrl->exisp.doing.af_cont == 1) {
	        	/* C-AF STOP */
            	thp7212_sensor_select_caf(s_ctrl->sensor_i2c_client, FALSE);
			}
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xF010, write_data, MSM_CAMERA_I2C_BYTE_DATA);
			s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		} else {
			CDBG("%s: Exit  Not Register Set\n",__func__);
		}
	}
	CDBG("%s: Exit\n",__func__);
	return 0;
}

static int thp7212_sensor_set_stabilization(struct msm_sensor_ctrl_t *s_ctrl, sensor_stabilization_t *stabilization)
{
	int32_t rc = 0;
	uint16_t addr;

	CDBG("%s: Enter\n",__func__);
	
	if (stabilization->kind == STABILIZATION_KIND_VIDEO) {
		addr = 0xF035;
		s_ctrl->exisp.stabilization_video = stabilization->onoff;
	} else if (stabilization->kind == STABILIZATION_KIND_SNAPSHOT) {
		addr = 0xF05F;
		s_ctrl->exisp.stabilization_photo = stabilization->onoff;
	} else {
		return -EINVAL;
	}
	
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, addr, (uint16_t)stabilization->onoff, MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("%s: Exit %d  writedata(%x:%d)\n",__func__,rc,addr,stabilization->onoff);
	return rc;
}

static int thp7212_sensor_set_iexposure(struct msm_sensor_ctrl_t *s_ctrl, uint8_t i_exposure)
{
	int32_t rc = 0;
	uint16_t data;

	CDBG("%s: Enter\n",__func__);
	
	if (i_exposure == 0) {
		data = 0x00;
	} else {
		data = 0x31;
	}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xF01D, data, MSM_CAMERA_I2C_BYTE_DATA);

	s_ctrl->exisp.supernights = i_exposure;

	CDBG("%s: Exit %d  writedata(0x%x)\n",__func__,rc,data);
	return rc;
}

/* EXISP Code::add E */

/* EXISP Zoom::add S */
static int thp7212_sensor_set_zoom(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int32_t rc = 0;

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xF03E, (uint16_t)value, MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->exisp.zoom = value;

	return rc;
}
/* EXISP Zoom::add E */

/* Exif Support::add S */
static int thp7212_sensor_get_snapshot_info(struct msm_sensor_ctrl_t *s_ctrl, sensor_snapshot_info_t *snapshot_info)
{
	int32_t rc = 0;
	uint16_t read_data[1];

	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF056, &read_data[0], MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
		pr_err("%s: Shutter Speed Read faild  rc=%d\n",__func__,rc);
		snapshot_info->shutter_speed = 0;
	} else {
		snapshot_info->shutter_speed = read_data[0];
	}
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF086, &read_data[0], MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
		pr_err("%s: Iso Speed Read faild  rc=%d\n",__func__,rc);
		snapshot_info->iso_speed = 0;
	} else {
		snapshot_info->iso_speed = read_data[0];
	}
	rc =msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF08a, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: Flash Status Read faild  rc=%d\n",__func__,rc);
		snapshot_info->flash = 0;
	} else {
		snapshot_info->flash = (uint8_t)read_data[0];
	}

	return 0;
}


static struct msm_camera_i2c_reg_conf makernote_reg_info[] = {
	{0xf001, 1},
	{0xf02d, 1},
	{0xf092, 1},
	{0xf014, 1},
	{0xf085, 1},
	{0xf019, 1},
	{0xf01a, 1},
	{0xf01b, 1},
	{0xf01d, 1},
	{0xfe10, 1},
	{0xf060, 2},
	{0xf062, 2},
	{0xf064, 2},
	{0xf066, 2},
	{0xf023, 1},
	{0xf091, 1},
	{0xf19b, 1},
	{0xf19c, 1},
	{0xf00c, 1},
	{0xf021, 1},
	{0xf024, 1},
	{0xf05f, 1},
	{0xf002, 1},
	{0xf04c, 1},
	{0xf04d, 1},
	{0xf089, 1},
	{0xf08f, 1},
	{0xf02e, 1},
	{0xf02f, 1},
	{0xf033, 1},
	{0xf00d, 1},
	{0xf03e, 1},
	{0xf012, 1},
	{0xf01c, 1},
	{0xf00e, 1},
	{0xf00f, 1},
	{0xfc80, 64},
};

static int thp7212_sensor_get_makernote_info(struct msm_sensor_ctrl_t *s_ctrl, sensor_makernote_info_t *makernote_info)
{
	int32_t rc = 0;
	uint16_t index = 0;
	uint16_t lp;
	uint16_t count = ARRAY_SIZE(makernote_reg_info);

	for (lp = 0; lp < count; lp++) {
		if (index + makernote_reg_info[lp].reg_data > SENSOR_MAKERNOTE_SIZE) {
			pr_err("%s: The output buffer is not enough\n",__func__);
			return -EFAULT;
		}
		rc = msm_camera_i2c_read_seq(s_ctrl->sensor_i2c_client,
									makernote_reg_info[lp].reg_addr,
									&makernote_info->data[index],
									makernote_reg_info[lp].reg_data);
		index += makernote_reg_info[lp].reg_data;
		if (rc < 0) {
			pr_err("%s: Maker Note Read faild  rc=%d errorAddr=0x%x\n",__func__,rc,makernote_reg_info[lp].reg_addr);
			memset(makernote_info->data, 0, SENSOR_MAKERNOTE_SIZE);
			break;
		}
	}
	return 0;
}
/* Exif Support::add S */

#if 1  /* EXISP */
static int thp7212_sensor_get_focus_distances_info(struct msm_sensor_ctrl_t *s_ctrl, uint8_t *focus_distance)
{
	int32_t rc = 0;
	uint16_t read_data[1];

	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF024, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);
	
	if (rc < 0) {
//      printk("%s: i2c read error = %d\n",__func__, rc);
        CDBG("%s: i2c read error = %d\n",__func__, rc);
    	*focus_distance = 0xFF;
	}else{
    	*focus_distance = (uint8_t)read_data[0];
    }
	
	return rc;
}

static struct msm_camera_i2c_reg_conf omakase_reg_info[] = {
	{0xF08D, 1},
	{0xF056, 2},
	{0xF086, 2},
	{0xF08C, 1},
	{0xF024, 1},
};

static int thp7212_sensor_get_omakase_info(struct msm_sensor_ctrl_t *s_ctrl, sensor_omakase_info_t *omakase_info)
{
	int32_t rc = 0;
	uint16_t index = 0;
	uint16_t lp;
	uint16_t count = ARRAY_SIZE(omakase_reg_info);

	for (lp = 0; lp < count; lp++) {
		if (index + omakase_reg_info[lp].reg_data > SENSOR_OMAKASE_SIZE) {
			pr_err("%s: The output buffer is not enough\n",__func__);
			return -EFAULT;
		}
		rc = msm_camera_i2c_read_seq(s_ctrl->sensor_i2c_client,
									omakase_reg_info[lp].reg_addr,
									&omakase_info->data[index],
									omakase_reg_info[lp].reg_data);
		index += omakase_reg_info[lp].reg_data;
		if (rc < 0) {
			pr_err("%s: Omakase Info Read faild  rc=%d errorAddr=0x%x\n", __func__, rc, omakase_reg_info[lp].reg_addr);
//			memset(omakase_info->data, 0, SENSOR_OMAKASE_SIZE);
			omakase_info->data[0] = 0xFF;
			omakase_info->data[1] = 0x00;
			omakase_info->data[2] = 0x00;
			omakase_info->data[3] = 0x00;
			omakase_info->data[4] = 0x00;
			omakase_info->data[5] = 0x02;
			omakase_info->data[6] = 0x07;
			break;
		}
	}
	return 0;
}

static struct msm_camera_i2c_reg_conf focus_distance_reg_info[] = {
	{0xF024, 1},
};

static int thp7212_sensor_get_focus_distance_info(struct msm_sensor_ctrl_t *s_ctrl, sensor_focus_distance_info_t *focus_distance_info)
{
	int32_t rc = 0;
	uint16_t index = 0;
	uint16_t lp;
	uint16_t count = ARRAY_SIZE(focus_distance_reg_info);

	for (lp = 0; lp < count; lp++) {
		if (index + focus_distance_reg_info[lp].reg_data > SENSOR_FOCUS_DISTANCE_SIZE) {
			pr_err("%s: The output buffer is not enough\n",__func__);
			return -EFAULT;
		}
		rc = msm_camera_i2c_read_seq(s_ctrl->sensor_i2c_client,
									focus_distance_reg_info[lp].reg_addr,
									&focus_distance_info->data[index],
									focus_distance_reg_info[lp].reg_data);
		index += focus_distance_reg_info[lp].reg_data;
		if (rc < 0) {
			pr_err("%s: Focus Distance Info Read faild  rc=%d errorAddr=0x%x\n", __func__, rc, focus_distance_reg_info[lp].reg_addr);
//			memset(focus_distance_info->data, 0, SENSOR_FOCUS_DISTANCE_SIZE);
			focus_distance_info->data[0] = 0x07;
			break;
		}
	}
	return 0;
}

#endif /* EXISP */

/*=============================================================*/
int32_t thp7212_sensor_config(struct msm_sensor_ctrl_t *s_ctrl, void __user *argp)
{
        struct sensor_cfg_data cdata;
        long rc = 0;
        if (copy_from_user(&cdata,
                (void *)argp,
                sizeof(struct sensor_cfg_data)))
                return -EFAULT;

        CDBG("%s: cfgtype = %d\n",__func__, cdata.cfgtype);

        mutex_lock(s_ctrl->msm_sensor_mutex);

                switch (cdata.cfgtype) {
                case CFG_SET_AUTO_FOCUS:
                        rc = thp7212_sensor_start_af(s_ctrl);
                        if(rc) {
                                pr_err("%s _sensor_start_af faild :%ld\n",__func__,rc);
                        }
                        break;
                case CFG_SET_EXISP_AF_CANCEL:
                        rc = thp7212_sensor_cancel_af(s_ctrl);
                        if(rc) {
                                pr_err("%s _sensor_cancel_af faild :%ld\n",__func__,rc);
                        }
                        break;
                case CFG_SET_EXISP_AF_STOP:
                        rc = thp7212_sensor_stop_af(s_ctrl);
                        if(rc) {
                                pr_err("%s _sensor_stop_af faild :%ld\n",__func__,rc);
                        }
                        break;
                case CFG_GET_EXISP_AF_STATS:
                        rc = thp7212_sensor_state_af(s_ctrl,&(cdata.cfg.exisp_af));
                        if(rc) {
                                pr_err("%s _sensor_state_af faild :%ld\n",__func__,rc);
                        }
                        if (copy_to_user((void *)argp,
                                &cdata,
                                sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;
                case CFG_SET_EXISP_AF_MODE:
                        if (s_ctrl->func_tbl->
                        exisp_sensor_set_af_mode == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                exisp_sensor_set_af_mode(s_ctrl,
                                        cdata.cfg.exisp_af_mode);
                        if(rc) {
                                pr_err("%s _sensor_set_af_mode faild :%ld\n",__func__,rc);
                        }
                        break;
                case CFG_SET_EXISP_CAF_MODE:
                        if (s_ctrl->func_tbl->
                        exisp_sensor_set_caf_mode == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                exisp_sensor_set_caf_mode(s_ctrl,
                                        cdata.cfg.exisp_caf_mode);
                        if(rc) {
                                pr_err("%s _sensor_set_caf_mode faild :%ld\n",__func__,rc);
                        }
                        break;
                case CFG_SET_EXISP_AF_MTR_AREA:
                        if (s_ctrl->func_tbl->
                        exisp_sensor_set_af_mtr_area == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                exisp_sensor_set_af_mtr_area(s_ctrl,
                                        cdata.cfg.exisp_af_mtr_area);
                        if(rc) {
                                pr_err("%s _sensor_set_caf_mode faild :%ld\n",__func__,rc);
                        }
                        break;
                case CFG_SET_EXISP_FLASH_SET_MODE:
                        if (s_ctrl->func_tbl->
                        exisp_sensor_set_led_mode == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                exisp_sensor_set_led_mode(s_ctrl,
                                        cdata.cfg.exisp_led_mode);
                        if(rc) {
                                pr_err("%s _sensor_set_caf_mode faild :%ld\n",__func__,rc);
                        }
                        break;

                /* ISP Zoom::add S */
                case CFG_SET_ZOOM:
                        rc = thp7212_sensor_set_zoom(s_ctrl, cdata.cfg.zoomvalue);
                        break;
                /* ISP Zoom::add E */

                /* Exif Support::add S */
                case CFG_GET_SNAPSHOT_EXISP:
                        rc = thp7212_sensor_get_snapshot_info(s_ctrl,&(cdata.cfg.snapshot_info));
                        if (copy_to_user((void *)argp, &cdata, sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;
                case CFG_GET_MAKERNOTE_EXISP:
                        rc = thp7212_sensor_get_makernote_info(s_ctrl,&(cdata.cfg.makernote_info));
                        if (copy_to_user((void *)argp, &cdata, sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;
                /* Exif Support::add E */

                /* setparameter to ISP */
                case CFG_SET_WB:
                        rc = thp7212_sensor_set_wb(s_ctrl, cdata.cfg.wb_val);
                        break;

                case CFG_SET_ANTIBANDING:
                        rc = thp7212_sensor_set_antibanding(s_ctrl, cdata.cfg.antibanding);
                        break;

                case CFG_SET_EXPOSURE_COMPENSATION:
                        rc = thp7212_sensor_set_exposure(s_ctrl, cdata.cfg.exp_compensation);
                        break;

                case CFG_SET_ISO:
                        rc = thp7212_sensor_set_iso(s_ctrl, cdata.cfg.iso_type);
                        break;

                case CFG_SET_EXISP_BESTSHOT:
                        rc = thp7212_sensor_set_bestshot_mode(s_ctrl, cdata.cfg.exisp_bestshot);
                        break;

                case CFG_SET_EXISP_AEC_LOCK:
                        rc = thp7212_sensor_set_aec_lock(s_ctrl, cdata.cfg.exisp_aec_lock);
                        break;

                case CFG_SET_EXISP_AWB_LOCK:
                        rc = thp7212_sensor_set_awb_lock(s_ctrl, cdata.cfg.exisp_awb_lock);
                        break;

                case CFG_SET_TERMINAL_DIRECTION:
                        rc = thp7212_sensor_set_direction(s_ctrl, cdata.cfg.terminal_direction);
                        break;

                case CFG_SET_HDRMODE:
                        rc = thp7212_sensor_set_hdrmode(s_ctrl, cdata.cfg.hdrmode);
                        break;

                case CFG_SET_STABILIZATION:
                        rc = thp7212_sensor_set_stabilization(s_ctrl, &(cdata.cfg.stabilization));
                        break;

                case CFG_SET_IEXPOSURE:
                        rc = thp7212_sensor_set_iexposure(s_ctrl, cdata.cfg.i_exposure);
                        break;

                case CFG_SET_EXISP_FACE_AF:
                        rc = thp7212_sensor_set_face_af(s_ctrl, cdata.cfg.face_af);
                        break;

                case CFG_SET_EXISP_SEQUENTIAL_SHOOTING:
                		rc = thp7212_sensor_set_sequential_shooting(s_ctrl, cdata.cfg.sequential_shooting);
                		break;

                case CFG_SET_EXISP_1SHOTAF_LIGHT:
                		rc = thp7212_sensor_set_1shotaf_light(s_ctrl, cdata.cfg.oneshotaf_light);
                		break;

                case CFG_SET_EXISP_IMAGE_STRENGTH:
                		rc = thp7212_sensor_set_image_strength(s_ctrl, cdata.cfg.image_strength);
                		break;

                case CFG_CAMERA_RESET:
                        rc = thp7212_camera_reset(s_ctrl);
                        break;

                /* setparameter for ISP */
                case CFG_SET_EFFECT:
                        rc = thp7212_sensor_set_special_effect(s_ctrl, cdata.cfg.effect);
                        break;

                case CFG_SET_FPS:
                case CFG_SET_PICT_FPS:
                        if (s_ctrl->func_tbl->
                        sensor_set_fps == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                sensor_set_fps(
                                s_ctrl,
                                &(cdata.cfg.fps));
                        break;

                case CFG_SET_EXP_GAIN:
                        if (s_ctrl->func_tbl->
                        sensor_write_exp_gain == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc =
                                s_ctrl->func_tbl->
                                sensor_write_exp_gain(
                                        s_ctrl,
                                        cdata.cfg.exp_gain.gain,
                                        cdata.cfg.exp_gain.line);
                        s_ctrl->prev_gain = cdata.cfg.exp_gain.gain;
                        s_ctrl->prev_line = cdata.cfg.exp_gain.line;
                        break;

                case CFG_SET_PICT_EXP_GAIN:
                        if (s_ctrl->func_tbl->
                        sensor_write_snapshot_exp_gain == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc =
                                s_ctrl->func_tbl->
                                sensor_write_snapshot_exp_gain(
                                        s_ctrl,
                                        cdata.cfg.exp_gain.gain,
                                        cdata.cfg.exp_gain.line);
                        break;

                case CFG_SET_MODE:
                        if (s_ctrl->func_tbl->
                        sensor_set_sensor_mode == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                sensor_set_sensor_mode(
                                        s_ctrl,
                                        cdata.mode,
                                        cdata.rs);
                        break;

                case CFG_SENSOR_INIT:
                        if (s_ctrl->func_tbl->
                        sensor_mode_init == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                sensor_mode_init(
                                s_ctrl,
                                cdata.mode,
                                &(cdata.cfg.init_info));
                        break;

                case CFG_GET_OUTPUT_INFO:
                        if (s_ctrl->func_tbl->
                        sensor_get_output_info == NULL) {
                                rc = -EFAULT;
                                break;
                        }
                        rc = s_ctrl->func_tbl->
                                sensor_get_output_info(
                                s_ctrl,
                                &cdata.cfg.output_info);

                        if (copy_to_user((void *)argp,
                                &cdata,
                                sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;

                case CFG_START_STREAM:
                    if (s_ctrl->func_tbl->sensor_start_stream == NULL) {
                        rc = -EFAULT;
                        break;
                    }
//                  printk("CFG_START_STREAM\n");
                    s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
                    break;

                case CFG_STOP_STREAM:
                    if (s_ctrl->func_tbl->sensor_stop_stream == NULL) {
                        rc = -EFAULT;
                        break;
                    }
//                  printk("CFG_STOP_STREAM\n");
                    s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
                    break;

                case CFG_GET_CSI_PARAMS:
                    if (s_ctrl->func_tbl->sensor_get_csi_params == NULL) {
                        rc = -EFAULT;
                        break;
                    }
//                  printk("CFG_GET_CSI_PARAMS\n");
                    rc = s_ctrl->func_tbl->sensor_get_csi_params(
                        s_ctrl,
                        &cdata.cfg.csi_lane_params);

                    if (copy_to_user((void *)argp,
                        &cdata,
                        sizeof(struct sensor_cfg_data)))
                        rc = -EFAULT;
                    break;

#if 1  /* EXISP */
                 case CFG_GET_FOCUS_DISTANCE_EXISP:
                        rc = thp7212_sensor_get_focus_distances_info(s_ctrl,&(cdata.cfg.exisp_focus_distance));
                        if (copy_to_user((void *)argp, &cdata, sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;
                 case CFG_GET_EXISP_OMAKASE_INFO:
                        rc = thp7212_sensor_get_omakase_info(s_ctrl,&(cdata.cfg.exisp_omakase_info));
                        if (copy_to_user((void *)argp, &cdata, sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;
                 case CFG_GET_EXISP_FOCUS_DISTANCE_INFO:
                        rc = thp7212_sensor_get_focus_distance_info(s_ctrl,&(cdata.cfg.exisp_focus_distance_info));
                        if (copy_to_user((void *)argp, &cdata, sizeof(struct sensor_cfg_data)))
                                rc = -EFAULT;
                        break;
#endif /* EXISP */

                default:
                        rc = -EFAULT;
                        break;
                }

        mutex_unlock(s_ctrl->msm_sensor_mutex);

        if(rc < 0) {
                pr_err("%s faild :cfgtype=%d rc=%ld\n",__func__,cdata.cfgtype,rc);
        }

        return rc;
}

/*
 * msm_sensor_ctrl_t functions.
 */
void thp7212_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
        msm_camera_i2c_write_tbl(
                s_ctrl->sensor_i2c_client,
                s_ctrl->msm_sensor_reg->stop_stream_conf,
                s_ctrl->msm_sensor_reg->stop_stream_conf_size,
                s_ctrl->msm_sensor_reg->default_data_type);

        thp7212_pm8921_red_led( PM8921_REDLED_OFF );	/* RED_LED OFF TODO */

        CDBG("###### %s\n",__func__);
}

void thp7212_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t data = 0x01;
    int32_t rc =0;
    uint16_t read_data[1];
    int i;

	if (g_res == MSM_SENSOR_RES_QTR) {	//still preview
		if (s_ctrl->exisp.hdr_mode)
			data = 0x02;
		else
			data = 0x01;
	} else if (g_res == MSM_SENSOR_RES_2) {	//video preview
		if (s_ctrl->exisp.hdr_mode)
			data = 0x04;
		else
			data = 0x03;
	}
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xF012, data, MSM_CAMERA_I2C_BYTE_DATA);

    rc = -1;
    for(i=0;i<300;i++){
        msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF013, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);
        if( read_data[0] ==  0x01){
         	/* status OK */
        	rc = 0;
            break;
        }
        //mdelay(10);
        msleep(10);
    }
    if(rc != 0){
        pr_err("%s  STATUS err \n", __func__);
    }

    msm_camera_i2c_write_tbl(
            s_ctrl->sensor_i2c_client,
            s_ctrl->msm_sensor_reg->start_stream_conf,
            s_ctrl->msm_sensor_reg->start_stream_conf_size,
            s_ctrl->msm_sensor_reg->default_data_type);
    thp7212_pm8921_red_led( PM8921_REDLED_ON );	/* RED_LED ON */

    if(s_ctrl->exisp.doing.af_cont == 1){
    /* C-AF START */
      	struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;
        rc = thp7212_sensor_select_caf(dev_client, TRUE);
        if(rc != 0){
            pr_err("%s  C-AF START ERROR \n", __func__);
        }
    }

    CDBG("###### %s\n",__func__);
}

/* initial setting */
static int32_t thp7212_write_res_settings(struct msm_sensor_ctrl_t *s_ctrl,
        uint16_t res)
{
        int32_t rc =0;
        char write_data[1];
        uint16_t read_data[1];
        int i;

        if(s_ctrl->exisp.doing.af_cont == 1){
        	/* C-AF STOP */
        	struct msm_camera_i2c_client *dev_client = s_ctrl->sensor_i2c_client;
            rc = thp7212_sensor_select_caf(dev_client, FALSE);
            if(rc != 0){
                pr_err("%s  C-AF STOP ERROR \n", __func__);
            	goto exit_thp7212_write_res_settings;
            }
        }

/* Still Size Change::modify S */
//      if(res == MSM_SENSOR_RES_FULL){
		if ((res == MSM_SENSOR_RES_FULL) ||
			(res == MSM_SENSOR_RES_3) ||
			(res == MSM_SENSOR_RES_4)) {
/* Still Size Change::modify E */
                /* preview => still */
                CDBG("###### FULL Scan %s\n",__func__);
                thp7212_pm8921_red_led( PM8921_REDLED_OFF );	/* RED_LED OFF */
                rc = msm_sensor_write_conf_array(
                s_ctrl->sensor_i2c_client,
                s_ctrl->msm_sensor_reg->mode_settings, res);
                if(rc != 0){
                    pr_err("%s  res = %d ERROR \n", __func__,res);
                	goto exit_thp7212_write_res_settings;
                }
                s_ctrl->exisp.doing.still = 1;	/* still flag set */
                CDBG("###### STOP PREVIEW %s\n",__func__);

        }else if((res == MSM_SENSOR_RES_QTR) ||
        		(res == MSM_SENSOR_RES_2)){
        		if(s_ctrl->exisp.doing.still == 0){
        			write_data[0]=0x00;
        			rc = msm_camera_i2c_write_seq(s_ctrl->sensor_i2c_client, 0xF010, &write_data[0], 1); /* STOP preview */
        			if(rc != 0){
        				pr_err("%s  stop preview ERROR \n", __func__);
        				goto exit_thp7212_write_res_settings;
        			}
        		}else{
            		/* still => still preview npdc300059577 */
            		for(i=0;i<300;i++){
            			msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF013, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);
            		    if( read_data[0] ==  0x02){
            		    /* status OK */
            		      	rc = 0;
            		        break;
            		    }
            		    msleep(10);
            		}
            		if(i == 300){
            		/* status check err */
            		  	rc = -1;
            		  	pr_err("%s  Camera Status timeout \n", __func__);
                    	s_ctrl->exisp.doing.still = 0;	/* still flag clear */
                		goto exit_thp7212_write_res_settings;
            		}
        		}
        		s_ctrl->exisp.doing.still = 0;	/* still flag clear */
        		rc = msm_sensor_write_conf_array(
        			s_ctrl->sensor_i2c_client,
        			s_ctrl->msm_sensor_reg->mode_settings, res);
        		if(rc != 0){
        			pr_err("%s  res = %d ERROR \n", __func__,res);
        			goto exit_thp7212_write_res_settings;
        		}
        }else{
                CDBG("##### No change mode %s\n",__func__);
        }

exit_thp7212_write_res_settings:

        return rc;
}

static int32_t thp7212_init_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    /* AE/AWB status */
    rc = msm_camera_i2c_write_seq(s_ctrl->sensor_i2c_client, 0xF09B, &ae_awb_status[0], CAMERA_AE_AWB_STATUS_MAX);

    if(rc != 0){
        CDBG("###### %s I2C ERR \n",__func__);
    }
    CDBG("###### %s\n",__func__);

    return rc;
}

int32_t thp7212_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
                        int update_type, int res)
{
        int32_t rc = 0;
        char write_data[1];
        uint16_t read_data[1];
        int i;

        CDBG("%s : E\n",__func__);

//kesasaki JB        v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
//kesasaki JB                NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
//kesasaki JB                PIX_0, ISPIF_OFF_IMMEDIATELY));

//      s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);   /* TODO need?? */

//        msleep(30);

        if (update_type == MSM_SENSOR_REG_INIT) {
                CDBG("######### update_type == MSM_SENSOR_REG_INIT %s\n",__func__);

                s_ctrl->curr_csi_params = NULL;
//              msm_sensor_enable_debugfs(s_ctrl);
//               msm_sensor_write_init_settings(s_ctrl);
//              s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
                rc = thp7212_init_settings(s_ctrl);
        } else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
                CDBG("######### update_type == MSM_SENSOR_UPDATE_PERIODIC %s\n",__func__);
                rc = thp7212_write_res_settings(s_ctrl, res); /* Sensor Mode setting */
                g_res = res;
                if(rc != 0){
                    pr_err("%s sensor mode set ERROR \n", __func__);
                	goto exit_thp7212_sensor_setting;
                }

                if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
                        s_ctrl->curr_csi_params = s_ctrl->csi_params[res];

#if 1   /* 20120711 */
                        s_ctrl->curr_csi_params->csid_params.lane_assign =
                                s_ctrl->sensordata->sensor_platform_info->csi_lane_params->csi_lane_assign;
                        s_ctrl->curr_csi_params->csiphy_params.lane_mask =
                                s_ctrl->sensordata->sensor_platform_info->csi_lane_params->csi_lane_mask;
#endif
                        v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
                                NOTIFY_CSID_CFG,
                                &s_ctrl->curr_csi_params->csid_params);
//kesasaki JB                        v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
//kesasaki JB                                                NOTIFY_CID_CHANGE, NULL);
                        mb();
                        v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
                                NOTIFY_CSIPHY_CFG,
                                &s_ctrl->curr_csi_params->csiphy_params);
                        mb();
                        msleep(20);
//                        mdelay(20);
                }
                v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
                        NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
                        output_settings[res].op_pixel_clk);
//kesasaki JB                v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
//kesasaki JB                        NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
//kesasaki JB                        PIX_0, ISPIF_ON_FRAME_BOUNDARY));
/* Still Size Change::modify S */
//              if(res == MSM_SENSOR_RES_FULL){
				if ((res == MSM_SENSOR_RES_FULL) ||
					(res == MSM_SENSOR_RES_3) ||
					(res == MSM_SENSOR_RES_4)) {
/* Still Size Change::modify E */
                        /* start full scan */
                        CDBG("######### start still stream %s\n",__func__);

                        rc = -1;
                        for(i=0;i<100;i++){
                                msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF013, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);
                                if( read_data[0] ==  0x01){
                                	/* status OK */
                                		rc = 0;
                                        break;
                                }
                                msleep(10);
//                                mdelay(10);
                        }
                        if(rc != 0){
                        	goto exit_thp7212_sensor_setting;
                        }
                        write_data[0]=0x01;
                        msm_camera_i2c_write_seq(s_ctrl->sensor_i2c_client, 0xF018, &write_data[0], 1); /* Int clear */
                }else{
                        /* preview start */
                        s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
                }
        }
exit_thp7212_sensor_setting:
        CDBG("%s : X %d\n",__func__, rc);

        return rc;
}

static void thp7212_gpio_set_value_cansleep(struct msm_gpio_set_tbl *gpio_tbl, int value)
{
        gpio_set_value_cansleep( gpio_tbl->gpio, value);
        udelay(gpio_tbl->delay);

        return;
}

static struct msm_gpio_set_tbl thp7212_camreset_gpio_set_tbl[] = {
        {86, GPIOF_OUT_INIT_HIGH, 2000},
};

static struct msm_cam_clk_info thp7212_cam_clk_info[] = {
#if USE_NEMO_BOARD
        {"cam_clk", MSM_SENSOR_MCLK_19HZ},
#else
        {"cam_clk", MSM_SENSOR_MCLK_24HZ},
#endif
};

int32_t thp7212_sensor_i2c_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
        int rc = 0;
        struct msm_sensor_ctrl_t *s_ctrl;
        CDBG("%s %s_i2c_probe called\n", __func__, client->name);
        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                pr_err("%s %s i2c_check_functionality failed\n",
                        __func__, client->name);
                rc = -EFAULT;
                return rc;
        }

        s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
        if (s_ctrl->sensor_i2c_client != NULL) {
                s_ctrl->sensor_i2c_client->client = client;
                if (s_ctrl->sensor_i2c_addr != 0)
                        s_ctrl->sensor_i2c_client->client->addr =
                                s_ctrl->sensor_i2c_addr;
        } else {
                pr_err("%s %s sensor_i2c_client NULL\n",
                        __func__, client->name);
                rc = -EFAULT;
                return rc;
        }

        s_ctrl->sensordata = client->dev.platform_data;
        if (s_ctrl->sensordata == NULL) {
                pr_err("%s %s NULL sensor data\n", __func__, client->name);
                return -EFAULT;
        }

        snprintf(s_ctrl->sensor_v4l2_subdev.name,
                sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
        v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
                s_ctrl->sensor_v4l2_subdev_ops);

        msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);

        spin_lock_init(&s_ctrl->exisp.slock);
        CDBG("%s : %d exisp.slock %p\n",__func__,__LINE__,&s_ctrl->exisp.slock);

        return rc;
}

int32_t thp7212_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;
        uint16_t chipid = 0;
        rc = msm_camera_i2c_read(
                        s_ctrl->sensor_i2c_client,
                        s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid,
                        MSM_CAMERA_I2C_BYTE_DATA);
        if (rc < 0) {
                pr_err("%s: %s: read id failed\n", __func__,
                        s_ctrl->sensordata->sensor_name);
                return rc;
        }

        CDBG("msm_sensor id: %d\n", chipid);
        if (chipid != s_ctrl->sensor_id_info->sensor_id) {
                pr_err("msm_sensor_match_id chip id doesnot match\n");
                return -ENODEV;
        }
        return rc;
}

int32_t thp7212_sensor_match_id_dmy(struct msm_sensor_ctrl_t *s_ctrl)
{
        /* always true */
        return 0;
}

/* use struct msm_sensor_output_reg_addr_t members
 *
 * sensor_adjust_frame_lines
 * sensor_write_exp_gain
 */

int32_t thp7212_sensor_adjust_frame_lines_dmy(struct msm_sensor_ctrl_t *s_ctrl,
        uint16_t res)
{
        pr_err("Call %s return True\n", __func__);

        /* always true */
        return 0;
}

int32_t thp7212_sensor_write_exp_gain_dmy(struct msm_sensor_ctrl_t *s_ctrl,
                uint16_t gain, uint32_t line)
{
        pr_err("Call %s return True\n", __func__);

        /* always true */
        return 0;
}

/* sensor ON */
int32_t thp7212_power_up(cam_id_t cam, struct msm_sensor_ctrl_t *s_ctrl)
{
        int32_t rc = 0;
        uint16_t read_data[1];
        struct msm_camera_sensor_info *data = s_ctrl->sensordata;
        int i;
       	int ii;
        
        /* switch sensor_ctrl */
        g_s_ctrl = s_ctrl;

        s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
                        * data->sensor_platform_info->num_vreg, GFP_KERNEL);
        if (!s_ctrl->reg_ptr) {
                pr_err("%s: could not allocate mem for regulators\n",
                        __func__);
                return -ENOMEM;
        }

#if 1	/* Power OFF */
        rc = msm_camera_request_gpio_table(data, 1);
#if 0	/* 0913 */
        thp7212_gpio_set_value_cansleep(thp7212_camreset_gpio_set_tbl, GPIOF_OUT_INIT_LOW);

        msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
                thp7212_cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(thp7212_cam_clk_info), 0);
#endif
        if (rc < 0) {
                pr_err("%s: request gpio failed\n", __func__);
                goto request_gpio_failed;
        }
#if 0	/* 0913 */
        rc = msm_camera_config_gpio_table(data, 0);
        if (rc < 0) {
                pr_err("%s: config gpio failed\n", __func__);
                goto config_gpio_failed;
        }
#endif
        rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                        s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                        s_ctrl->sensordata->sensor_platform_info->num_vreg,
                        s_ctrl->reg_ptr, 1);
        if (rc < 0) {
                pr_err("%s: regulator on failed\n", __func__);
                goto config_vreg_failed;
        }
#if 0	/* 0913 */
        rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                        s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                        s_ctrl->sensordata->sensor_platform_info->num_vreg,
                s_ctrl->reg_ptr, 0);
        if (rc < 0) {
                pr_err("%s: enable regulator failed\n", __func__);
                goto enable_vreg_failed;
        }
#endif
#endif

        rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                        s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                        s_ctrl->sensordata->sensor_platform_info->num_vreg,
                        s_ctrl->reg_ptr, 1);
        if (rc < 0) {
                pr_err("%s: enable regulator failed\n", __func__);
                goto enable_vreg_failed;
        }

        rc = msm_camera_config_gpio_table(data, 1);
        if (rc < 0) {
                pr_err("%s: config gpio failed\n", __func__);
                goto config_gpio_failed;
        }

        if (s_ctrl->clk_rate != 0)
                thp7212_cam_clk_info->clk_rate = s_ctrl->clk_rate;
        rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
                thp7212_cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(thp7212_cam_clk_info), 1);
        if (rc < 0) {
                pr_err("%s: clk enable failed\n", __func__);
                goto enable_clk_failed;
        }


//      usleep_range(1000, 2000);
        mdelay(2);	/* 2ms */
        thp7212_gpio_set_value_cansleep(thp7212_camreset_gpio_set_tbl, GPIOF_OUT_INIT_HIGH);

        if (rc < 0) {
                pr_err("%s: config gpio failed\n", __func__);
                goto config_gpio_failed;
        }

#if 1	/* SPI active npdc300060875 */
		base = ioremap(0x00801330,	0x40);
		msm_camera_io_w(0x00000148, base );
		msm_camera_io_w(0x00000148, base+0x20 );
		msm_camera_io_w(0x00000148, base+0x30 );
#endif
//        mdelay(2);	/* 2ms => 10ms */
        for(i=0; i<3; i++){
        /* TODO Firmware Download */
        	rc = camera_fw_spi_write(cam);
        	if (rc < 0) {
                pr_err("%s: camera_fw_spi_write failed\n", __func__);
#if 1	/* SPI suspend npdc300060875 */
                msm_camera_io_w(0x00000001, base );
				msm_camera_io_w(0x00000001, base+0x20 );
				msm_camera_io_w(0x00000001, base+0x30 );
				iounmap(base);
#endif
                goto enable_clk_failed;
        	}
        	mdelay(70);    /* 70ms wait (T.B.D) */

        	for (ii=0; ii<3 ;ii++) {
	        	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0xF004, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);
        		if (rc >= 0) {
        			break;
        		}
	        	CDBG("######  %s rc=%d read error retry \n",__func__,rc);
	        	mdelay(10);    /* 10ms wait (T.B.D) */
        	}

        	CDBG("######  %s 0xF004 data = %04x \n",__func__,read_data[0]);
        	if((rc >= 0) && (read_data[0]==0x80)){
        		/* CameraFW Success */
        		rc = 0;
                g_s_ctrl->exisp.state.power = 1;
        		break;
        	}else{
        		rc = -1;
        		pr_err("######  %s CameraFW Failed \n",__func__);
                thp7212_gpio_set_value_cansleep(thp7212_camreset_gpio_set_tbl, GPIOF_OUT_INIT_LOW);

                thp7212_gpio_set_value_cansleep(thp7212_camreset_gpio_set_tbl, GPIOF_OUT_INIT_HIGH);
        	}

        }



       	if(rc == 0){
			return rc;
		}

#if 1	/* SPI suspend npdc300060875 */
        msm_camera_io_w(0x00000001, base );
		msm_camera_io_w(0x00000001, base+0x20 );
		msm_camera_io_w(0x00000001, base+0x30 );
		iounmap(base);
#endif

enable_clk_failed:
        msm_camera_config_gpio_table(data, 0);

config_gpio_failed:
        msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                        s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                        s_ctrl->sensordata->sensor_platform_info->num_vreg,
                        s_ctrl->reg_ptr, 0);

enable_vreg_failed:
        msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                s_ctrl->sensordata->sensor_platform_info->num_vreg,
                s_ctrl->reg_ptr, 0);

config_vreg_failed:
        msm_camera_request_gpio_table(data, 0);

request_gpio_failed:
        kfree(s_ctrl->reg_ptr);

        return rc;
}

int32_t thp7212_power_down(cam_id_t cam, struct msm_sensor_ctrl_t *s_ctrl)
{
        struct msm_camera_sensor_info *data = s_ctrl->sensordata;
        char write_data[1];

#if 0
        if (data->sensor_platform_info->i2c_conf &&
                data->sensor_platform_info->i2c_conf->use_i2c_mux)
                msm_sensor_disable_i2c_mux(
                        data->sensor_platform_info->i2c_conf);

        if (data->sensor_platform_info->ext_power_ctrl != NULL)
                data->sensor_platform_info->ext_power_ctrl(0);
#endif
        s_ctrl->exisp.doing.still = 0;	/* still flag clear */

#if 1	/* SPI suspend npdc300060875 */
        msm_camera_io_w(0x00000001, base );
		msm_camera_io_w(0x00000001, base+0x20 );
		msm_camera_io_w(0x00000001, base+0x30 );
		iounmap(base);
#endif
        /* Read AE/AWB STATUS */
        msm_camera_i2c_read_seq(s_ctrl->sensor_i2c_client, 0xF09B,
        		&ae_awb_status[0], CAMERA_AE_AWB_STATUS_MAX);

        /* Read Flicker STATUS */
        msm_camera_i2c_read_seq(s_ctrl->sensor_i2c_client, 0xF00C, 	&flicker_status, 1);
        if(flicker_status != EXISP_ANTIBANDING_STATUS_60HZ){
        	flicker_status = EXISP_ANTIBANDING_STATUS_50HZ;
        }

        if(cam == THP7212_OUT){
        	write_data[0] = 0x01;
        	msm_camera_i2c_write_seq(s_ctrl->sensor_i2c_client, 0xF027, &write_data[0], 1); /* VCM reset */
        	mdelay(20);    /* 20ms wait */
        }

        thp7212_gpio_set_value_cansleep(thp7212_camreset_gpio_set_tbl, GPIOF_OUT_INIT_LOW);

        msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
                thp7212_cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(thp7212_cam_clk_info), 0);
        msm_camera_config_gpio_table(data, 0);
        msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                s_ctrl->sensordata->sensor_platform_info->num_vreg,
                s_ctrl->reg_ptr, 0);
        msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
                s_ctrl->sensordata->sensor_platform_info->cam_vreg,
                s_ctrl->sensordata->sensor_platform_info->num_vreg,
                s_ctrl->reg_ptr, 0);
        msm_camera_request_gpio_table(data, 0);

        kfree(s_ctrl->reg_ptr);

        g_s_ctrl->exisp.state.power = 0;

        return 0;
}

#if 1  /* EXISP code cfglog */
#define NUM_CAM_RST_TBL 8

static void thp7212_record_cfglog_camera_reset(unsigned char fac)
{
	/* If this function is called from any context, lock is needed */
	int ret;
	struct timeval tv;
	struct rtc_time tm;
	unsigned char count;
	unsigned char factor[NUM_CAM_RST_TBL];
	unsigned long timestamp[NUM_CAM_RST_TBL];
	int index;

	ret  = cfgdrv_read(D_HCM_E_CAM_STATIC_ELE_RST_CNT, sizeof(count), &count);
	ret |= cfgdrv_read(D_HCM_E_CAM_STATIC_ELE_RST_FACTOR, sizeof(factor), factor);
	ret |= cfgdrv_read(D_HCM_E_CAM_STATIC_ELE_RST_TIMESTAMP,
		sizeof(timestamp), (unsigned char *)timestamp);
	if (ret < 0) {
		printk("%s: cfgdrv_read error ret = %d\n", __func__, ret);
		count = 0;
	}
	if (count >= 0xFF) {
		printk("%s: log counter is MAX\n", __func__);
		return;
	}
	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec, &tm);
	index = count % NUM_CAM_RST_TBL;
	count++;
	factor[index] = fac;
	timestamp[index] =
		((((((tm.tm_mon + 1) << 22) |
			((unsigned long)tm.tm_mday << 17)) |
			((unsigned long)tm.tm_hour << 12)) |
			((unsigned long)tm.tm_min << 6)) |
			(unsigned long)tm.tm_sec);
	printk("%s: count = %hhu, index = %d\n", __func__, count, index);
	printk("%s: [%02d/%02d %02d:%02d:%02d] factor = %hhu\n", __func__,
		tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, fac);
	ret  = cfgdrv_write(D_HCM_E_CAM_STATIC_ELE_RST_CNT, sizeof(count), &count);
	ret |= cfgdrv_write(D_HCM_E_CAM_STATIC_ELE_RST_FACTOR, sizeof(factor), factor);
	ret |= cfgdrv_write(D_HCM_E_CAM_STATIC_ELE_RST_TIMESTAMP,
		sizeof(timestamp), (unsigned char *)timestamp);
	if (ret < 0) {
		printk("%s: cfgdrv_write error\n", __func__);
		return;
	}
	printk("%s: success\n", __func__);
	return;
}
#endif /* EXISP code cfglog */

int thp7212_camera_reset(struct msm_sensor_ctrl_t *s_ctrl)
{
	sensor_stabilization_t stabilization;

	printk("######## %s\n",__func__);
#if 0	/* exisp_code npdc300060330  */
	exisp_err_flag = 0;
#endif
#if 1  /* EXISP code cfglog */
	thp7212_record_cfglog_camera_reset(1);
#endif /* EXISP code cfglog */

	 /* stream off */
//	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
//	msleep(100);
	/* power down */
	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	/* power up */
	s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	/* reg init */
	s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
	s_ctrl->func_tbl->sensor_setting(s_ctrl, MSM_SENSOR_REG_INIT, 0);

#if 1	/* cam_reset_param */
	if(s_ctrl->exisp.effect != CAMERA_EFFECT_OFF){
		thp7212_sensor_set_special_effect(s_ctrl, s_ctrl->exisp.effect);
	}else{
		if(s_ctrl->exisp.scene_mode != 0){
			thp7212_sensor_set_bestshot_mode(s_ctrl, s_ctrl->exisp.scene_mode);
		}else{
			thp7212_sensor_set_iso(s_ctrl, s_ctrl->exisp.iso);
			thp7212_sensor_set_wb(s_ctrl, s_ctrl->exisp.wb);
		}
	}

	thp7212_sensor_set_zoom(s_ctrl, s_ctrl->exisp.zoom);
	thp7212_sensor_set_direction(s_ctrl, s_ctrl->exisp.direction);
	thp7212_sensor_set_exposure(s_ctrl, s_ctrl->exisp.exposure);

	if (s_ctrl->func_tbl->exisp_sensor_set_led_mode != NULL) {
		s_ctrl->func_tbl->exisp_sensor_set_led_mode(s_ctrl, s_ctrl->exisp.led_mode);
	}

	thp7212_sensor_set_1shotaf_light(s_ctrl, s_ctrl->exisp.af_light);

	stabilization.kind = STABILIZATION_KIND_VIDEO;
	stabilization.onoff = s_ctrl->exisp.stabilization_video;
	thp7212_sensor_set_stabilization(s_ctrl, &stabilization);

	stabilization.kind = STABILIZATION_KIND_SNAPSHOT;
	stabilization.onoff = s_ctrl->exisp.stabilization_photo;
	thp7212_sensor_set_stabilization(s_ctrl, &stabilization);

	if (s_ctrl->func_tbl->exisp_sensor_set_af_mtr_area != NULL) {
		s_ctrl->func_tbl->exisp_sensor_set_af_mtr_area(s_ctrl, s_ctrl->exisp.af_mtr_area);
	}

	thp7212_sensor_set_iexposure(s_ctrl, s_ctrl->exisp.supernights);

	thp7212_sensor_set_aec_lock(s_ctrl, s_ctrl->exisp.ae_lock);
	thp7212_sensor_set_awb_lock(s_ctrl, s_ctrl->exisp.wb_lock);

	thp7212_sensor_set_sequential_shooting(s_ctrl, s_ctrl->exisp.ae_diagram);

	thp7212_sensor_set_image_strength(s_ctrl, s_ctrl->exisp.image_strength);

    if (s_ctrl->func_tbl->exisp_sensor_set_af_mode != NULL) {
    	s_ctrl->func_tbl->exisp_sensor_set_af_mode(s_ctrl, s_ctrl->exisp.af_mode);
    }

    if (s_ctrl->func_tbl->exisp_sensor_set_caf_mode != NULL) {
    	if(s_ctrl->exisp.caf_mode == EXISP_CAF_ON){
    		s_ctrl->func_tbl->exisp_sensor_set_caf_mode(s_ctrl, s_ctrl->exisp.caf_mode);
    	}
    }

#endif
	/* periodic */
#if 1 /* EXISP code npdc300060330 */	
	s_ctrl->func_tbl->sensor_setting(s_ctrl, MSM_SENSOR_UPDATE_PERIODIC, g_res);
		
	exisp_err_flag = 0;
#endif
	
	return 0;
}

/* function for setparameter to ISP */
int thp7212_sensor_set_antibanding(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int antibanding = value;
	CDBG("%s : E\n",__func__);
	switch(antibanding){
		case CAMERA_ANTIBANDING_50HZ:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF00B, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
			break;
		}

		case CAMERA_ANTIBANDING_60HZ:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF00B, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
			break;
		}
		case CAMERA_ANTIBANDING_AUTO:{
			if(flicker_status == EXISP_ANTIBANDING_STATUS_50HZ){
				/* AUTO 50Hz */
				rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
						0xF00B, 0x10, MSM_CAMERA_I2C_BYTE_DATA);
			}
			else{
				/* AUTO 60Hz */
				rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
						0xF00B, 0x11, MSM_CAMERA_I2C_BYTE_DATA);
			}
			break;
		}
		default:{
            pr_err("%s failed %d\n", __func__, __LINE__);
            rc = -EINVAL;
			break;
		}
	}
    if (0 > rc){
            pr_err("%s _i2c_write faild 0xF00B :%d\n",__func__, rc);
    }
    CDBG("%s : X %d\n",__func__, rc);
	return rc;

}

const int8_t thp7212_exposure_confs[] = {
		0x00, /* -2EV */
		0x01, /* -5/3EV */
		0x02, /* -4/3EV */
		0x03, /* -1EV */
		0x04, /* -2/3EV */
		0x05, /* -1/3EV */
		0x06, /* 0EV */
		0x07, /* 1/3EV */
		0x08, /* 2/3EV */
		0x09, /* 1EV */
		0x0A, /* 4/3EV */
		0x0B, /* 5/3EV */
		0x0C  /* 2EV */
};

int thp7212_sensor_set_exposure(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int8_t exposure_level;
	CDBG("%s : E\n",__func__);

	if((value >= CAMERA_EXPOSURE_MIN) && (value <= CAMERA_EXPOSURE_MAX )){
		exposure_level = value + 6; /* change for struct */
		rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				0xF02E, thp7212_exposure_confs[exposure_level], MSM_CAMERA_I2C_BYTE_DATA);

	    if (0 > rc){
	            pr_err("%s _i2c_write faild 0xF02E :%d\n",__func__, rc);
	    }

	}else{
        pr_err("%s failed %d\n", __func__, __LINE__);
        rc = -EINVAL;
	}
	s_ctrl->exisp.exposure = value;
	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

const int8_t thp7212_wb_confs[] = {
     0x00,/* CAMERA_WB_MIN_MINUS_1 */
     0x00,/* CAMERA_WB_AUTO */
     0x00,/* CAMERA_WB_CUSTOM */
     0x01,/* CAMERA_WB_INCANDESCENT */
     0x05,/* CAMERA_WB_FLUORESCENT */
     0x02,/* CAMERA_WB_DAYLIGHT */
     0x03,/* CAMERA_WB_CLOUDY_DAYLIGHT */
     0x00,/* CAMERA_WB_TWILIGHT */
     0x00,/* CAMERA_WB_SHADE */
     0x00,/* CAMERA_WB_OFF */
};

int thp7212_sensor_set_wb(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int wb_mode = value;
	CDBG("%s : E\n",__func__);

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0xF02D, thp7212_wb_confs[wb_mode], MSM_CAMERA_I2C_BYTE_DATA);

	if (0 > rc){
	    pr_err("%s _i2c_write faild 0xF02D :%d\n",__func__, rc);
	}
	s_ctrl->exisp.wb = value;
	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

typedef struct {
	int8_t internal_iA_mode;
	int8_t scene_mode;
	int8_t iA_mode;
}bestshot_data_t;

const bestshot_data_t thp7212_bestshot_confs[]={
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_OFF */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_AUTO */
        {0x80, 0x04, 0x00}, /* CAMERA_BESTSHOT_LANDSCAPE */
        {0x80, 0x08, 0x00}, /* CAMERA_BESTSHOT_SNOW */
        {0x80, 0x0C, 0x00}, /* CAMERA_BESTSHOT_BEACH */
        {0x80, 0x09, 0x00}, /* CAMERA_BESTSHOT_SUNSET */
        {0x80, 0x05, 0x00}, /* CAMERA_BESTSHOT_NIGHT */
        {0x80, 0x01, 0x00}, /* CAMERA_BESTSHOT_PORTRAIT */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_BACKLIGHT */
        {0x80, 0x03, 0x00}, /* CAMERA_BESTSHOT_SPORTS */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_ANTISHAKE */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_FLOWERS */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_CANDLELIGHT */
        {0x80, 0x0D, 0x00}, /* CAMERA_BESTSHOT_FIREWORKS */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_PARTY */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_NIGHT_PORTRAIT */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_THEATRE */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_ACTION */
        {0x80, 0x00, 0x00}, /* CAMERA_BESTSHOT_AR */
        {0x80, 0x0B, 0x00}, /* EXISP_CAMERA_BESTSHOT_LOWLIGHT */
        {0x80, 0x06, 0x00}, /* EXISP_CAMERA_BESTSHOT_BACKLIGHT */
        {0x80, 0x07, 0x00}, /* EXISP_CAMERA_BESTSHOT_CHARACTER */
        {0x80, 0x02, 0x00}, /* EXISP_CAMERA_BESTSHOT_FOOD */
        {0x80, 0x0A, 0x00}, /* EXISP_CAMERA_BESTSHOT_PET */
        /* for iA */
        {0x85, 0x00, 0x00}, /* EXISP_CAMERA_BESTSHOT_IA_AUTO */
        {0x85, 0x00, 0x09}, /* EXISP_CAMERA_BESTSHOT_IA_BEACH */
        {0x85, 0x00, 0x0E}, /* EXISP_CAMERA_BESTSHOT_IA_LOWLIGHT */
        {0x85, 0x00, 0x0B}, /* EXISP_CAMERA_BESTSHOT_IA_FIREWORKS */
        {0x85, 0x00, 0x03}, /* EXISP_CAMERA_BESTSHOT_IA_LANDSCAPE */
        {0x85, 0x00, 0x04}, /* EXISP_CAMERA_BESTSHOT_IA_NIGHT */
        {0x85, 0x00, 0x05}, /* EXISP_CAMERA_BESTSHOT_IA_NIGHT_PORTRAIT */
        {0x85, 0x00, 0x01}, /* EXISP_CAMERA_BESTSHOT_IA_PORTRAIT */
        {0x85, 0x00, 0x0A}, /* EXISP_CAMERA_BESTSHOT_IA_SNOW */
        {0x85, 0x00, 0x08}, /* EXISP_CAMERA_BESTSHOT_IA_SUNSET */
        {0x85, 0x00, 0x06}, /* EXISP_CAMERA_BESTSHOT_IA_BACKLIGHT */
        {0x85, 0x00, 0x0C}, /* EXISP_CAMERA_BESTSHOT_IA_CHARACTER */
        {0x85, 0x00, 0x0D}, /* EXISP_CAMERA_BESTSHOT_IA_FOOD */
        {0x85, 0x00, 0x0F}, /* EXISP_CAMERA_BESTSHOT_IA_PET */
        {0x85, 0x00, 0x07}, /* EXISP_CAMERA_BESTSHOT_IA_BACKLIGHT_PORTRAIT */
        {0x85, 0x00, 0x02}, /* EXISP_CAMERA_BESTSHOT_IA_MACRO */
};

int thp7212_sensor_set_bestshot_mode(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int bestshot = value;
	CDBG("%s : E\n",__func__);

	/* Internal iA Mode */
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			0xF090, thp7212_bestshot_confs[bestshot].internal_iA_mode, MSM_CAMERA_I2C_BYTE_DATA);

	if (0 > rc){
	    pr_err("%s _i2c_write faild 0xF091 :%d\n",__func__, rc);
	    goto exit_thp7212_sensor_set_bestshot_mode;
	}

	/* Scene Mode */
	if (s_ctrl->exisp.effect == CAMERA_EFFECT_OFF) {
		rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			0xF014, thp7212_bestshot_confs[bestshot].scene_mode, MSM_CAMERA_I2C_BYTE_DATA);

		if (0 > rc){
	    	pr_err("%s _i2c_write faild 0xF014 :%d\n",__func__, rc);
	    	goto exit_thp7212_sensor_set_bestshot_mode;
		}
	}

	/* iA Mode */
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0xF091, thp7212_bestshot_confs[bestshot].iA_mode, MSM_CAMERA_I2C_BYTE_DATA);

	if (0 > rc){
	    pr_err("%s _i2c_write faild 0xF091 :%d\n",__func__, rc);
	    goto exit_thp7212_sensor_set_bestshot_mode;
	}
	s_ctrl->exisp.scene_mode = value;
exit_thp7212_sensor_set_bestshot_mode:

	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

typedef struct {
	int8_t effect_mode;
	int8_t scene_mode;
	int8_t skin_color_mode;
}effect_data_t;

const effect_data_t thp7212_effect_confs[] = {
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_OFF */
		{0x01, 0x00, 0x01}, /* CAMERA_EFFECT_MONO */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_NEGATIVE */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_SOLARIZE */
		{0x02, 0x00, 0x01}, /* CAMERA_EFFECT_SEPIA */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_POSTERIZE */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_WHITEBOARD */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_BLACKBOARD */
		{0x0A, 0x00, 0x01}, /* CAMERA_EFFECT_AQUA */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_EMBOSS */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_SKETCH */
		{0x00, 0x00, 0x01}, /* CAMERA_EFFECT_NEON */

		{0x00, 0x00, 0x02}, /* EXISP_CAMERA_EFFECT_BEAUTYSKIN */
		{0x00, 0x10, 0x01}, /* EXISP_CAMERA_EFFECT_CHIC */
		{0x06, 0x11, 0x01}, /* EXISP_CAMERA_EFFECT_FANTASY */
		{0x25, 0x00, 0x01}, /* EXISP_CAMERA_EFFECT_NOSTALGIA */
		{0x05, 0x00, 0x01}, /* EXISP_CAMERA_EFFECT_PINHOLE */
		{0x04, 0x00, 0x01}, /* EXISP_CAMERA_EFFECT_POP */
		{0x00, 0x11, 0x01}, /* EXISP_CAMERA_EFFECT_PURE */
		{0x06, 0x00, 0x01}, /* EXISP_CAMERA_EFFECT_SOFTFOCUS */
		{0x45, 0x00, 0x01}, /* EXISP_CAMERA_EFFECT_TOY */
};

int thp7212_sensor_set_special_effect(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int effect_value = value;
	CDBG("%s : E\n",__func__);

	if(effect_value < CAMERA_EFFECT_MAX){
		/* Effect Mode */
		rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				0xF02F, thp7212_effect_confs[effect_value].effect_mode, MSM_CAMERA_I2C_BYTE_DATA);

	    if (0 > rc){
	            pr_err("%s _i2c_write faild 0xF02F :%d\n",__func__, rc);
	            goto exit_thp7212_sensor_set_special_effect;
	    }

	    /* Scene Mode */
		if (effect_value != CAMERA_EFFECT_OFF) {
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF014, thp7212_effect_confs[effect_value].scene_mode, MSM_CAMERA_I2C_BYTE_DATA);
		} else {
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF014, thp7212_bestshot_confs[s_ctrl->exisp.scene_mode].scene_mode, MSM_CAMERA_I2C_BYTE_DATA);
		}

	    if (0 > rc){
	            pr_err("%s _i2c_write faild 0xF014 :%d\n",__func__, rc);
	            goto exit_thp7212_sensor_set_special_effect;
	    }

	    /* Skin Color Mode */
		rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				0xF033 ,thp7212_effect_confs[effect_value].skin_color_mode, MSM_CAMERA_I2C_BYTE_DATA);

	    if (0 > rc){
	            pr_err("%s _i2c_write faild 0xF033 :%d\n",__func__, rc);
	            goto exit_thp7212_sensor_set_special_effect;
	    }

	}else{
        pr_err("%s failed %d\n", __func__, __LINE__);
        rc = -EINVAL;
	}
	s_ctrl->exisp.effect = value;
exit_thp7212_sensor_set_special_effect:

	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

const int8_t thp7212_iso_confs[] = {
	0x00, /* MSM_V4L2_ISO_AUTO */
	0x00, /* MSM_V4L2_ISO_DEBLUR */
	0x00, /* MSM_V4L2_ISO_100 */
	0x01, /* MSM_V4L2_ISO_200 */
	0x02, /* MSM_V4L2_ISO_400 */
	0x03, /* MSM_V4L2_ISO_800 */
	0x04, /* MSM_V4L2_ISO_1600 */
};

int thp7212_sensor_set_iso(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int iso_mode = value;
	CDBG("%s : E\n",__func__);

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0xF085, thp7212_iso_confs[iso_mode], MSM_CAMERA_I2C_BYTE_DATA);

	if (0 > rc){
	    pr_err("%s _i2c_write faild 0xF085 :%d\n",__func__, rc);
	}
	s_ctrl->exisp.iso = value;
	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}


int thp7212_sensor_set_aec_lock(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	uint16_t aec_lock;
	CDBG("%s : E\n",__func__);

	if(value == 0){
		aec_lock = 0x00;
	}else{
		aec_lock = 0x01;
	}

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF019, aec_lock, MSM_CAMERA_I2C_BYTE_DATA);
	if (0 > rc){
		pr_err("%s _i2c_write faild 0xF019 :%d\n",__func__, rc);
	}

	s_ctrl->exisp.ae_lock = value;

	CDBG("%s : X %d\n",__func__, rc);
	return rc;

}

int thp7212_sensor_set_awb_lock(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	uint16_t awb_lock;
	CDBG("%s : E\n",__func__);

	if(value == 0){
		awb_lock = 0x00;
	}else{
		awb_lock = 0x01;
	}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF01A, awb_lock, MSM_CAMERA_I2C_BYTE_DATA);

	if (0 > rc){
		pr_err("%s _i2c_write faild 0xF01A :%d\n",__func__, rc);
	}

	s_ctrl->exisp.wb_lock = value;

	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

int thp7212_sensor_set_face_af(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int face_af = value;
	CDBG("%s : E\n",__func__);

	switch(face_af){
		case EXISP_CAMERA_FACE_AF_OFF:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF091, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF091 :%d\n",__func__, rc);
		    }
			break;
		}

		case EXISP_CAMERA_FACE_AF_ON:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF091, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF091 :%d\n",__func__, rc);
		    }
			break;
		}
		
		case EXISP_CAMERA_FACE_AF_BACKLIGHT:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF091, 0x07, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF091 :%d\n",__func__, rc);
		    }
			break;
		}
		
		default:{
            pr_err("%s failed %d\n", __func__, __LINE__);
            rc = -EINVAL;
			break;
		}
	}
	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

int thp7212_sensor_set_sequential_shooting(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	uint16_t data;
	CDBG("%s : E\n",__func__);
	
	switch(value) {
	case EXISP_CAMERA_SEQUENTIAL_SHOOTING_OFF:
		data = 0x00;
		break;
	case EXISP_CAMERA_SEQUENTIAL_SHOOTING_NORMAL_ON:
		data = 0x01;
		break;
	case EXISP_CAMERA_SEQUENTIAL_SHOOTING_NIGHTSHOT_ON:
		data = 0x02;
		break;
	default:
		pr_err("%s failed %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			0xF01C, data, MSM_CAMERA_I2C_BYTE_DATA);
    if (0 > rc){
		pr_err("%s _i2c_write faild 0xF01C :%d\n",__func__, rc);
    }

    s_ctrl->exisp.ae_diagram = value;

	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

int thp7212_sensor_set_1shotaf_light(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int oneshotaf_light = value;
	CDBG("%s : E\n",__func__);

	switch(oneshotaf_light){
		case EXISP_CAMERA_1SHOTAF_LIGHT_OFF:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF08B, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF08B :%d\n",__func__, rc);
		    }
			break;
		}

		case EXISP_CAMERA_1SHOTAF_LIGHT_AUTO:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF08B, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF08B :%d\n",__func__, rc);
		    }
			break;
		}
		default:{
            pr_err("%s failed %d\n", __func__, __LINE__);
            rc = -EINVAL;
			break;
		}
	}
	s_ctrl->exisp.af_light = value;
	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

int thp7212_sensor_set_image_strength(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int rc = 0;
	int image_strength = value;
	CDBG("%s : E\n",__func__);

	switch(image_strength){
		case EXISP_IMAGE_STRENGTH_WEAK:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF032, 0x03, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF032 :%d\n",__func__, rc);
		    }
			break;
		}

		case EXISP_IMAGE_STRENGTH_STRONG:{
			rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
					0xF032, 0x0C, MSM_CAMERA_I2C_BYTE_DATA);
		    if (0 > rc){
		            pr_err("%s _i2c_write faild 0xF032 :%d\n",__func__, rc);
		    }
			break;
		}
		default:{
            pr_err("%s failed %d\n", __func__, __LINE__);
            rc = -EINVAL;
			break;
		}
	}

	s_ctrl->exisp.image_strength = value;
	CDBG("%s : X %d\n",__func__, rc);
	return rc;
}

/* Camera Sensor's temperature get */
int msm_camera_temperature_get( int *temp )
{
	int16_t read_data[1];
	int32_t rc;

	if( g_s_ctrl == NULL){
		return MSM_CAM_TEMP_POWEROFF;
	}
	if( g_s_ctrl->exisp.state.power == 0){
		return MSM_CAM_TEMP_POWEROFF;
	}

	rc = msm_camera_i2c_read(g_s_ctrl->sensor_i2c_client, 0xF00F, &read_data[0], MSM_CAMERA_I2C_BYTE_DATA);

	if(rc < 0){
		return MSM_CAM_TEMP_ERR;
	}
	*temp = read_data[0];


	return MSM_CAM_TEMP_OK;
}
