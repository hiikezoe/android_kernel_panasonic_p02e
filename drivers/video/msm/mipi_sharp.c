/* Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_sharp.h"
#include "mdp4.h"
#include <linux/cfgdrv.h>
#include <linux/hcm_eep.h>
#include <linux/gpio.h>
#include <linux/leds.h>

#include "../../../arch/arm/mach-msm/devices.h"
#include "../../../arch/arm/mach-msm/board-8064.h"

#if defined(TEST_BUILD_MIPI_PANEL)
#define STATIC
#else
#define STATIC static
#endif

#define TM_GET_PID(id) (((id) & 0xff00)>>8)
DEFINE_MUTEX(mipi_sharp_pow_lock);
DEFINE_MUTEX(mipi_sharp_vblind_lock);
#define MIPI_SHARP_MAX_LOOP 10

STATIC struct pwm_device *bl_lpm;
STATIC struct mipi_dsi_panel_platform_data *mipi_sharp_pdata;
STATIC struct dsi_buf sharp_tx_buf;
STATIC struct dsi_buf sharp_rx_buf;
STATIC int gpio_lcd_dcdc;				/* GPIO:LCD DCDC */
STATIC int gpio_disp_rst_n;				/* GPIO:RESET */
struct timer_list mipi_sharp_refresh_timer;
struct timer_list mipi_sharp_blc_timer;
STATIC struct sharp_workqueue_data *sharp_workqueue_data_p;
STATIC void mipi_sharp_backlight_work_func(struct work_struct *work);
struct sharp_workqueue_data {
	struct workqueue_struct	*mipi_sharp_refresh_workqueue;
	struct work_struct		refresh_workq_initseq;
	struct work_struct		blc_workq_initseq;
	struct work_struct		backlight_workq_initseq;
	struct msm_fb_data_type	*workq_mfd;

	int bl_brightness;
	int bl_brightness_old;
	int bl_blc_brightness;
	u8  bl_mode_status;
	u8  bl_mode_status_old;
};
STATIC struct regulator *reg_l23;

struct msm_fb_data_type *gmfd;

STATIC u8 mipi_sharp_init_flg = FALSE;
STATIC u8 mipi_sharp_wkq_status = MIPI_SHARP_OFF;
#ifdef MIPI_SHARP_FUNC_REFRESH_RESET
STATIC u8 mipi_sharp_hw_reset	= MIPI_SHARP_ON;
#endif
#ifdef MIPI_SHARP_FUNC_REFRESH
STATIC u8 mipi_sharp_refresh_cycle = 15;			/* 15 sec */
#endif
STATIC unsigned long  mipi_sharp_blc_cycle_timer = 20;		/* 20msec */
#ifdef MIPI_SHARP_FUNC_BLC
STATIC unsigned long  mipi_sharp_blc_cycle_timer_delay = 1000;	/* 1sec */
#endif
struct t_mipi_sharp_vblind_info_tbl mipi_sharp_vblind_info;
STATIC u8 refresh_force_rst_flg = MIPI_SHARP_OFF;
#ifdef MIPI_SHARP_CMD_FUNC
STATIC uint32 debug_mipi_sharp_rx_value = 0;
STATIC uint32 debug_mipi_sharp_mode_value = 0;
#endif
STATIC int mipi_sharp_bl_tmp	= MIPI_SHARP_OFF;
STATIC int mipi_sharp_blc_cnt = 0;
STATIC int mipi_sharp_blc_status = MIPI_SHARP_OFF;
STATIC u8  mipi_sharp_backlight_status = MIPI_SHARP_OFF;
extern spinlock_t mdp_spin_lock;


struct t_mipi_sharp_vblind_lut_data {
	uint32_t flags;                     /*enable*/
	struct mdp_ar_gc_lut_data r_data;
	struct mdp_ar_gc_lut_data g_data;
	struct mdp_ar_gc_lut_data b_data;
};

STATIC void mipi_sharp_wkq_timer_stop(u8 change_status, u8 stop_timer);
STATIC int  mipi_sharp_lcd_init(void);
STATIC void mipi_sharp_lcd_power_on_init(void);
STATIC void mipi_sharp_lcd_power_standby_on_init(void);
STATIC void mipi_sharp_refresh_timer_set(void);
STATIC void mipi_sharp_eeprom_read(void);
STATIC void mipi_sharp_refresh_handler(unsigned long data);
STATIC void mipi_sharp_refresh_work_func(struct work_struct *work);
STATIC void mipi_sharp_blc_handler(unsigned long data);
STATIC void mipi_sharp_blc_work_func(struct work_struct *work);
STATIC void mipi_sharp_set_backlight_sub(int bl_level);
STATIC void mipi_sharp_blc_on(struct msm_fb_data_type *mfd);
STATIC u8   mipi_sharp_read_rddisbv(struct msm_fb_data_type *mfd, int brightness_level);
STATIC int  mipi_sharp_set_display_mode(enum leddrv_lcdbl_mode_status led_mode);
STATIC int mipi_sharp_lcd_sysfs_register(struct device *dev);
STATIC void mipi_sharp_lcd_sysfs_unregister(struct device *dev);
STATIC void mipi_sharp_lcd_pre_on(struct msm_fb_data_type *mfd);
STATIC void mipi_sharp_lcd_pre_off(struct msm_fb_data_type *mfd);

STATIC void  mipi_sharp_vblind_on(struct msm_fb_data_type *mfd);
STATIC void  mipi_sharp_vblind_off(struct msm_fb_data_type *mfd);
STATIC void  mipi_sharp_vblind_set_lut( struct msm_fb_data_type *mfd , struct t_mipi_sharp_vblind_lut_data *lut_data);
/****** WakeUp and ShutDown Sequence command  ******/
STATIC char exit_sleep[2]	= {0x11, 0x00};
STATIC char display_on[2]	= {0x29, 0x00};
STATIC char display_off[2]	= {0x28, 0x00};
STATIC char enter_sleep[2]	= {0x10, 0x00};
STATIC char deep_standby1[2]	= {0xb0, 0x02};
STATIC char deep_standby2[2]	= {0xb1, 0x01};
STATIC char NOP[2]		= {0x00, 0x00};
/***************************************************/

/****** BLC Parameter ******/

#define MAX_BLC_MODE 4

#ifdef MIPI_SHARP_FUNC_BLC
STATIC char RDDISBV[2] = {0x52, 0x00};
#endif

STATIC char WRDISBV[3]  = {
	0x51,
	0x0F, 0xFF
};

STATIC char Backlight_CTL_B9[8]  = {
	0xb9,
	0xdf, 0x18, 0x04, 0x40,
	0x9f, 0x1f, 0x80,
};

STATIC char Backlight_CTL_B9_MODE[MAX_BLC_MODE][8]  = {
	{
	0xb9,
	0x02, 0x06, 0x10, 0x40,
	0x9f, 0x1f, 0x80,
	},
	{
	0xb9,
	0x02, 0x06, 0x10, 0x40,
	0x9f, 0x1f, 0x80,
	},
	{
	0xb9,
	0x02, 0x06, 0x10, 0x40,
	0x9f, 0x1f, 0x80,
	},
	{
	0xb9,
	0x20, 0x20, 0x10, 0x40,
	0x9f, 0x1f, 0x80,
	}
};

STATIC char Backlight_CTL_BA[8]  = {
	0xba,
	0xdf, 0x18, 0x04, 0x40,
	0x9f, 0x1f, 0x80,
};

STATIC char Backlight_CTL_CE[8]  = {
	0xce,
	0x00, 0x01, 0x08, 0xc1,
	0x00, 0x00, 0x00,
};

STATIC u8 blc_select[5] = {0x01,0x01,0x01,0x01,0x04};  /*[0]:NOMAL [1]:MOVIE [2]:ONESEG [3]:CAMERA [4]:ECO  ***/

/***************************/

/****** Rupy Wakeup and Shutdown Sequence commands ******/
struct dsi_cmd_desc sharp_hd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on}
};
static struct dsi_cmd_desc sharp_display_off_cmds1[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_off), display_off},
};
 
static struct dsi_cmd_desc sharp_display_off_cmds2[] = {
 	{DTYPE_DCS_WRITE, 1, 0, 0, 140, sizeof(enter_sleep), enter_sleep}
};


STATIC struct dsi_cmd_desc sharp_deep_standby_cmds1 = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(deep_standby1), deep_standby1
};

STATIC struct dsi_cmd_desc sharp_deep_standby_cmds2 = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 40, sizeof(deep_standby2), deep_standby2
};

STATIC struct dsi_cmd_desc sharp_NOP_cmd = {
	DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(NOP), NOP
};
/********************************************************/


/****** Rupy refresh commands ******/
#ifdef MIPI_SHARP_FUNC_REFRESH_RESET
STATIC char power_mode[2]	= {0x0A, 0x00};

STATIC struct dsi_cmd_desc sharp_refresh_cmds = {
	DTYPE_DCS_READ, 1, 0, 1, 0, sizeof(power_mode), power_mode
};
#endif
/***********************************/

/****** LED TABLE ******/
#define LCDBL_TABLE_SIZE	(15)
STATIC u8 led_lcdbl_normal_table[LCDBL_TABLE_SIZE] = {
	0x5F, 0x5F, 0x5D, 0x5B, 0x53, 0x49, 0x3C, 0x35, 0x30, 0x27, 0x1A, 0x1A, 0x0C, 0x7D, 0x00 };
STATIC u8 led_lcdbl_ecomode_table[LCDBL_TABLE_SIZE] = {
	0x5A, 0x5A, 0x57, 0x54, 0x4C, 0x47, 0x3E, 0x34, 0x2B, 0x22, 0x1A, 0x1A, 0x07, 0x7D, 0x00 };
STATIC u8 led_lcdbl_highbright_table[LCDBL_TABLE_SIZE] = {
	0x5A, 0x5A, 0x57, 0x54, 0x4C, 0x47, 0x3E, 0x34, 0x2B, 0x22, 0x1A, 0x1A, 0x07, 0x7D, 0x00 };

STATIC unsigned short mipi_sharp_pwm_period[5] = {0x7D,0x7D,0x7D,0x7D,0x7D};
/**********************/

/****** Rupy LCD Commands ******/

STATIC char MCAP[2]		= {0xb0, 0x00};
STATIC char NVM_LOAD[2]		= {0xd6, 0x01};
STATIC char WCTRLDIS[2]		= {0x53, 0x00};
/*STATIC char CABC[2]		= {0x55, 0x02};*//* npdc300085685 */
STATIC char CABC[2]		= {0x55, 0x00};/* npdc300085685 */
STATIC char CABC_MIN[3]		= {0x5e, 0x00, 0x00};
STATIC char MCAP_BAN[2]		= {0xb0, 0x02};

STATIC struct dsi_cmd_desc sharp_blc_setting_cmds[] ={
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(MCAP), MCAP},				/* B0 MCAP APP */
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(NOP), NOP},				/* 00 NOP */
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(NOP), NOP},				/* 00 NOP */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(NVM_LOAD), NVM_LOAD},			/* D6 NVMLOAD SETTING */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Backlight_CTL_B9), Backlight_CTL_B9},	/* B9 BACKLIGHT CTRL2 */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Backlight_CTL_BA), Backlight_CTL_BA},	/* BA BACKLIGHT CTRL4 */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Backlight_CTL_CE), Backlight_CTL_CE},	/* CE BACKLIGHT CTRL6 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(WRDISBV), WRDISBV},			/* 51 WRITE DISBV */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(WCTRLDIS), WCTRLDIS},			/* 53 WRITE CTRL DISPLAY */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},				/* 55 CABC 00:FF 01:UI 02:STILL 03:VIDEO */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(CABC_MIN), CABC_MIN},			/* 5E CABC MINIMUM BRIGHTNESS */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(MCAP_BAN), MCAP_BAN},			/* B0 MCAP Access BAN */
};
/*******************************/


/****** Rupy BLC commands ******/
#ifdef MIPI_SHARP_FUNC_BLC
/* for read display brightness value command */
STATIC struct dsi_cmd_desc sharp_blc_cmds = {
	DTYPE_DCS_READ, 1, 0, 1, 0, sizeof(RDDISBV), RDDISBV
};
#endif
/*******************************/


/****** Rupy BLC Mode Change Commands ******/
STATIC struct dsi_cmd_desc sharp_blc_mode_change_cmds[] ={
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(MCAP), MCAP},				/* B0 MCAP APP */
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(NOP), NOP},				/* 00 NOP */
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(NOP), NOP},				/* 00 NOP */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Backlight_CTL_B9), Backlight_CTL_B9},	/* B9 BACKLIGHT CTRL2 */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Backlight_CTL_CE), Backlight_CTL_CE},	/* CE BACKLIGHT CTRL6 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(WRDISBV), WRDISBV},			/* 51 WRITE DISBV */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(WCTRLDIS), WCTRLDIS},			/* 53 WRITE CTRL DISPLAY */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},				/* 55 CABC 00:FF 01:UI 02:STILL 03:VIDEO */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(CABC_MIN), CABC_MIN},			/* 5E CABC MINIMUM BRIGHTNESS */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(MCAP_BAN), MCAP_BAN},			/* B0 MCAP Access BAN */
};
/*******************************************/


/****** ViewBlind Parameter ******/
#define VIEWBLIND_COLOR_MAX 3
#define VIEWBLIND_STRENGTH_LEVEL_MAX 3

STATIC unsigned char ViewBlind_MDP_DMAP_GC[VIEWBLIND_COLOR_MAX][42]  = {/*Sysdes 0.46*/
	{                                   /* COLOR0*/
	0x00, 0x00, 0x00, 0x24, 0x5D, 0xC0, /*strong*/
	0x00, 0x00, 0x00, 0x32, 0x55, 0xF0, /*normal*/
	0x00, 0x00, 0x00, 0x4E, 0x4A, 0x38, /*weak  */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode1 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode2 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode3 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /*gamma mode4 unused */
	},
	{                                   /* COLOR1*/
	0x00, 0x00, 0x00, 0x24, 0x5D, 0xC0, /*strong*/
	0x00, 0x00, 0x00, 0x32, 0x55, 0xF0, /*normal*/
	0x00, 0x00, 0x00, 0x4E, 0x4A, 0x38, /*weak  */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode1 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode2 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode3 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /*gamma mode4 unused */
	},
	{                                   /* COLOR2*/
	0x00, 0x00, 0x00, 0x24, 0x5D, 0xC0, /*strong*/
	0x00, 0x00, 0x00, 0x32, 0x55, 0xF0, /*normal*/
	0x00, 0x00, 0x00, 0x4E, 0x4A, 0x38, /*weak  */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode1 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode2 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*gamma mode3 unused */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /*gamma mode4 unused */
	}
};

#define MDP_DMA_P_OFFSET       0x90000
#define MDP_DMA_GC_OFFSET       0x8800
#define MDP_GC_COLOR_OFFSET      0x100
#define MDP_GC_PARMS_OFFSET       0x80



/****** Rupy EEPROM read ******/

STATIC u8 mipi_sharp_eeprom_flg   = FALSE;

struct t_mipi_sharp_cmd_eeprom_tbl const init_cmd_eeprom[] = {

	/* initial cmd */
	{D_HCM_E_LCD_DRV_INIT_55 , 1, &CABC[1]},

	{D_HCM_E_LCD_DRV_BLC_B9_SELECT , ARRAY_SIZE(blc_select), blc_select},

#ifdef MIPI_SHARP_FUNC_REFRESH
	{D_HCM_LCD_REFLESH_PERIOD , 1, &mipi_sharp_refresh_cycle},
#endif

#ifdef MIPI_SHARP_FUNC_REFRESH_RESET
	{D_HCM_LCD_RETRANS_ON , 1, &mipi_sharp_hw_reset},
#endif

	/* BLC */
	{D_HCM_E_LCD_DRV_BLC_B9_MODE1 , 7, &Backlight_CTL_B9_MODE[0][1]},
	{D_HCM_E_LCD_DRV_BLC_B9_MODE2 , 7, &Backlight_CTL_B9_MODE[1][1]},
	{D_HCM_E_LCD_DRV_BLC_B9_MODE3 , 7, &Backlight_CTL_B9_MODE[2][1]},
	{D_HCM_E_LCD_DRV_BLC_B9_MODE4 , 7, &Backlight_CTL_B9_MODE[3][1]},

	/* period(LED) */
	{D_HCM_LPWMON , ARRAY_SIZE(led_lcdbl_normal_table), led_lcdbl_normal_table},
	{D_HCM_LPWMON_ECO , ARRAY_SIZE(led_lcdbl_ecomode_table), led_lcdbl_ecomode_table},
	{D_HCM_LPWMON_HIGH , ARRAY_SIZE(led_lcdbl_highbright_table), led_lcdbl_highbright_table},

	/* ViewBlind */
	{D_HCM_E_MDP_DMAP_GC_COLOR0 , 42, &ViewBlind_MDP_DMAP_GC[0][0]},
	{D_HCM_E_MDP_DMAP_GC_COLOR1 , 42, &ViewBlind_MDP_DMAP_GC[1][0]},
	{D_HCM_E_MDP_DMAP_GC_COLOR2 , 42, &ViewBlind_MDP_DMAP_GC[2][0]}
};
/******************************/


void mipi_sharp_lcd_pre_on(struct msm_fb_data_type *mfd)
{
	LCD_DEBUG_IN_LOG

	LCD_DEBUG_OUT_LOG

	return;
}

void mipi_sharp_lcd_pre_off(struct msm_fb_data_type *mfd)
{
	LCD_DEBUG_IN_LOG

	mipi_sharp_wkq_timer_stop(MIPI_SHARP_OFF, MIPI_SHARP_TIMER_ALL);

	cancel_work_sync(&sharp_workqueue_data_p->backlight_workq_initseq);

	LCD_DEBUG_OUT_LOG

	return;
}

STATIC void mipi_sharp_wkq_timer_stop(u8 change_status, u8 stop_timer)
{
	LCD_DEBUG_IN_LOG

	mutex_lock(&mipi_sharp_pow_lock);

	if( mipi_sharp_wkq_status == MIPI_SHARP_OFF )
	{
		mutex_unlock(&mipi_sharp_pow_lock);
		LCD_DEBUG_OUT_LOG
		return;
	}

	mipi_sharp_wkq_status = MIPI_SHARP_OFF;

	mutex_unlock(&mipi_sharp_pow_lock);
#ifdef MIPI_SHARP_FUNC_BLC
	if( stop_timer & MIPI_SHARP_TIMER_BLC )
	{
		/* BLC function is suspended. */
		del_timer(&mipi_sharp_blc_timer);

		cancel_work_sync(&sharp_workqueue_data_p->blc_workq_initseq);
	}
#endif

#ifdef MIPI_SHARP_FUNC_REFRESH
	if( stop_timer & MIPI_SHARP_TIMER_REFRESH )
	{
		/* Refresh function is suspended. */
		del_timer(&mipi_sharp_refresh_timer);

		cancel_work_sync(&sharp_workqueue_data_p->refresh_workq_initseq);
	}
#endif

	mipi_sharp_wkq_status = change_status;

	LCD_DEBUG_OUT_LOG
}

STATIC void mipi_sharp_eeprom_read(void)
{
	int err = 0;
	int cnt;

	LCD_DEBUG_IN_LOG

	if(mipi_sharp_eeprom_flg)
	{
		/* do nothing */
		LCD_DEBUG_OUT_LOG
		return;
	}

	/* initial cmd */
	for(cnt = 0; cnt < (sizeof(init_cmd_eeprom)/sizeof(init_cmd_eeprom[0])); cnt++)
	{
		err = cfgdrv_read(init_cmd_eeprom[cnt].type,
						  init_cmd_eeprom[cnt].data_size,
						  init_cmd_eeprom[cnt].data);
		if (err) {
			pr_err("######## LCD can't read D_HCM_E, error=%d,cnt=%d\n", err, cnt);
			LCD_DEBUG_OUT_LOG
			return;
		}

	}

	mipi_sharp_pwm_period[MIPI_SHARP_NORMAL_MODE] =
		(unsigned short)(led_lcdbl_normal_table[14] << 8 ) | led_lcdbl_normal_table[13];
	mipi_sharp_pwm_period[MIPI_SHARP_ECO_MODE] =
		(unsigned short)(led_lcdbl_ecomode_table[14] << 8 ) | led_lcdbl_ecomode_table[13];
	mipi_sharp_pwm_period[MIPI_SHARP_CAMERA_MODE] =
		(unsigned short)(led_lcdbl_highbright_table[14] << 8 ) | led_lcdbl_highbright_table[13];

	mipi_sharp_eeprom_flg = TRUE;

	LCD_DEBUG_LOG("##### LCD EEPROM READ OK #####\n");

	LCD_DEBUG_OUT_LOG

	return;
}

STATIC void mipi_sharp_refresh_handler(unsigned long data)
{
	queue_work(sharp_workqueue_data_p->mipi_sharp_refresh_workqueue,
				&sharp_workqueue_data_p->refresh_workq_initseq);
}

STATIC void mipi_sharp_blc_handler(unsigned long data)
{
	unsigned long flag;

	spin_lock_irqsave(&mdp_spin_lock, flag);
	if(mipi_sharp_blc_cnt != 0)
	{
		mipi_sharp_blc_cnt--;
		spin_unlock_irqrestore(&mdp_spin_lock, flag);

		queue_work(sharp_workqueue_data_p->mipi_sharp_refresh_workqueue,
					&sharp_workqueue_data_p->blc_workq_initseq);
	}
	else
	{
		spin_unlock_irqrestore(&mdp_spin_lock, flag);
	}
}

STATIC void mipi_sharp_refresh_timer_set(void)
{
#ifdef MIPI_SHARP_FUNC_REFRESH
	LCD_DEBUG_LOG_DETAILS(" refresh_timer_set \n");

	del_timer(&mipi_sharp_refresh_timer);

	if(mipi_sharp_wkq_status == MIPI_SHARP_OFF)
		return;

	mipi_sharp_refresh_timer.expires =
		jiffies + (mipi_sharp_refresh_cycle * HZ);

	add_timer(&mipi_sharp_refresh_timer);

#endif

	return;
}

STATIC void mipi_sharp_blc_timer_set( unsigned long blc_time)
{
#ifdef MIPI_SHARP_FUNC_BLC
	unsigned long mipi_sharp_blc_cycle;
	del_timer(&mipi_sharp_blc_timer);

	if(mipi_sharp_wkq_status == MIPI_SHARP_OFF)
	{
		return;
	}

	mipi_sharp_blc_cycle = jiffies + msecs_to_jiffies( blc_time );

	mipi_sharp_blc_timer.expires =
		mipi_sharp_blc_cycle;

	add_timer(&mipi_sharp_blc_timer);

#endif

	return;
}

void mipi_sharp_vsync_on()
{
	if(mipi_sharp_blc_status == MIPI_SHARP_OFF)
	{
		mipi_sharp_blc_cnt = 0;

		return;
	}

	if(mipi_sharp_blc_cnt == 0)
	{
		mipi_sharp_blc_timer_set( mipi_sharp_blc_cycle_timer );
	}

	mipi_sharp_blc_cnt = MIPI_SHARP_BLC_CNT_INIT;
}

STATIC void mipi_sharp_refresh_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *lmfd;
#ifdef MIPI_SHARP_FUNC_REFRESH_RESET
	uint32 *data;
	unsigned char rd_data;
#endif
	LCD_DEBUG_IN_LOG

	lmfd = sharp_workqueue_data_p->workq_mfd;

	if( lmfd->panel_power_on == FALSE )
	{
		LCD_DEBUG_OUT_LOG
		return;
	}

	mipi_sharp_wkq_timer_stop(MIPI_SHARP_ON, MIPI_SHARP_TIMER_BLC);

#ifdef MIPI_SHARP_FUNC_REFRESH_RESET
	if( mipi_sharp_hw_reset == MIPI_SHARP_ON )
	{
		/* Enable force reset */

		mipi_dsi_sharp_cmd_sync(lmfd); /* vsync wait */

		mipi_dsi_cmds_rx(lmfd, &sharp_tx_buf, &sharp_rx_buf, &sharp_refresh_cmds, 1);

		data = (uint32 *)sharp_rx_buf.data;

		rd_data = (unsigned char)*data;

		if(!((rd_data & MIPI_SHARP_POWER_MODE_NORMAL) ==
						MIPI_SHARP_POWER_MODE_NORMAL))
		{
			pr_info("####### LCD HW refresh reset! : power_mode = 0x%X #######\n",rd_data);
			lmfd->hw_force_reset = TRUE;
			refresh_force_rst_flg = TRUE;
			mipi_sharp_init_flg = FALSE;
			LCD_DEBUG_OUT_LOG
			return;
		}
	}
	else
	{
		/* Disable force reset */
		/* do nothing */
	}
#endif

#ifdef MIPI_SHARP_FUNC_BLC
	/* BLC function is resumed. */
	mipi_sharp_blc_timer_set( mipi_sharp_blc_cycle_timer );
#endif

#ifdef MIPI_SHARP_FUNC_REFRESH
	mipi_sharp_refresh_timer_set();
#endif

	LCD_DEBUG_OUT_LOG
}

STATIC u8 mipi_sharp_read_rddisbv(struct msm_fb_data_type *mfd, int brightness_level)
{
#ifdef MIPI_SHARP_FUNC_BLC
	int ret;
	char data1;
	char data2;
	int rddisbv = 0;
	int wrdisbv = 0;
#endif
	unsigned char set_blc_level;

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  IN ][%d %s]\n", __LINE__, __FUNCTION__);

	set_blc_level = 0;

	if(brightness_level == 0 )
	{
		LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s]\n", __LINE__, __FUNCTION__);
		return set_blc_level;
	}

#ifdef MIPI_SHARP_FUNC_BLC

	wrdisbv = ((WRDISBV[1] & 0x0F) << 8);
	wrdisbv |= WRDISBV[2] & 0xFF;

	if(mipi_sharp_bl_tmp)
	{
		mipi_sharp_set_backlight_sub(mipi_sharp_bl_tmp);
	}

	mipi_dsi_sharp_cmd_sync(mfd); /* vsync wait */

	/* RDDISBV */
	ret = mipi_dsi_cmds_rx(mfd, &sharp_tx_buf, &sharp_rx_buf, &sharp_blc_cmds, 2);

	data1 = sharp_rx_buf.data[0];
	data2 = sharp_rx_buf.data[1];

	rddisbv = data1;
	rddisbv = ((rddisbv & 0x0F) << 8);
	rddisbv |= data2 & 0xFF;

	set_blc_level = (brightness_level * ((rddisbv * 100) / wrdisbv)) / 100;

	LCD_DEBUG_LOG_DETAILS("###### BLC - brightness level [%02x](%d) -> [%02x](%d) , RDDISBV = [%04x]\n", brightness_level,brightness_level, set_blc_level, set_blc_level, rddisbv);
#endif

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s]\n", __LINE__, __FUNCTION__);
	/* resolution 0`255 */
	return set_blc_level;
}

STATIC void mipi_sharp_blc_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *lmfd;
	u8 blc_level;

	lmfd = sharp_workqueue_data_p->workq_mfd;

	if( lmfd->panel_power_on == FALSE )
	{
		return;
	}

	blc_level = mipi_sharp_read_rddisbv(lmfd, sharp_workqueue_data_p->bl_brightness);

	if( blc_level != sharp_workqueue_data_p->bl_blc_brightness )
	{
		mipi_sharp_set_backlight_sub(blc_level);
	}

	sharp_workqueue_data_p->bl_blc_brightness = blc_level;

	mipi_sharp_blc_timer_set(mipi_sharp_blc_cycle_timer);
}

STATIC void mipi_sharp_backlight_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *lmfd;
	u8 bl_level;
#ifdef MIPI_SHARP_FUNC_BLC
	u8 bl_blc_level;
	unsigned char loop_cnt;
	int bl_tmp;
#endif

	mipi_sharp_bl_tmp = MIPI_SHARP_OFF;

	lmfd = sharp_workqueue_data_p->workq_mfd;
	bl_level = sharp_workqueue_data_p->bl_brightness;

	if((lmfd->panel_power_on == FALSE) ||
		((sharp_workqueue_data_p->bl_brightness_old == bl_level) &&
		(sharp_workqueue_data_p->bl_mode_status ==
		sharp_workqueue_data_p->bl_mode_status_old)))
	{
		return;
	}

	mipi_sharp_wkq_timer_stop(MIPI_SHARP_ON, MIPI_SHARP_TIMER_ALL);

#ifdef MIPI_SHARP_FUNC_BLC

	if(sharp_workqueue_data_p->bl_mode_status !=
		sharp_workqueue_data_p->bl_mode_status_old)
	{
		sharp_workqueue_data_p->bl_mode_status_old =
			sharp_workqueue_data_p->bl_mode_status;

		/* BLC function is re-set up. */
		mipi_sharp_blc_on(lmfd);
	}
	else
	{

		for(loop_cnt = 0 ; loop_cnt < MIPI_SHARP_MAX_LOOP ;loop_cnt++)
		{
			bl_blc_level = mipi_sharp_read_rddisbv(lmfd, bl_level);
			mipi_sharp_set_backlight_sub(bl_blc_level);

			mipi_sharp_bl_tmp = MIPI_SHARP_OFF;
			if( bl_level == sharp_workqueue_data_p->bl_brightness)
			{
				break;
			}
			else
			{
				if(sharp_workqueue_data_p->bl_brightness > bl_level)
				{
					bl_tmp = (sharp_workqueue_data_p->bl_brightness - bl_level) / 2;
					mipi_sharp_bl_tmp = bl_blc_level + bl_tmp;

					if(mipi_sharp_bl_tmp > 255)
						mipi_sharp_bl_tmp = MIPI_SHARP_OFF;
				}
				else
				{
					bl_tmp = (bl_level - sharp_workqueue_data_p->bl_brightness) / 2;
					mipi_sharp_bl_tmp = bl_blc_level - bl_tmp;

					if(mipi_sharp_bl_tmp <= 0)
						mipi_sharp_bl_tmp = MIPI_SHARP_OFF;
				}

				bl_level = sharp_workqueue_data_p->bl_brightness;
			}
		}

	}


#else
	sharp_workqueue_data_p->bl_mode_status_old =
		sharp_workqueue_data_p->bl_mode_status;

	mipi_sharp_set_backlight_sub(bl_level);
#endif

#if 1 /*RUPY_add*/
	if( (0 < bl_level) &&(sharp_workqueue_data_p->bl_brightness_old == 0))
	{
		leddrv_lcdbl_led_on();
	}
#endif /*RUPY_add*/

	sharp_workqueue_data_p->bl_blc_brightness = 0;
	sharp_workqueue_data_p->bl_brightness_old = bl_level;

#ifdef MIPI_SHARP_FUNC_REFRESH
	/* Refresh function is resumed. */
	mipi_sharp_refresh_timer_set();
#endif
#ifdef MIPI_SHARP_FUNC_BLC
	/* BLC function is resumed. */
	mipi_sharp_blc_timer_set(mipi_sharp_blc_cycle_timer_delay);
#endif
}

STATIC int mipi_sharp_ledset_backlight(int brightness, enum leddrv_lcdbl_mode_status mode_status)
{
	int err = 0;

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  IN ][%d %s]\n", __LINE__, __FUNCTION__);

	LCD_DEBUG_LOG_DETAILS("brightness = [%x] mode_status = [%x]\n",brightness,mode_status);

	err = mipi_sharp_set_display_mode(mode_status);

	if (err) {
		LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s mipi_sharp_set_display_mode err]\n", __LINE__, __FUNCTION__);
		return err;
	}

	sharp_workqueue_data_p->bl_brightness = brightness;

	mutex_lock(&mipi_sharp_pow_lock);

	if(mipi_sharp_wkq_status == MIPI_SHARP_OFF)
	{
		mutex_unlock(&mipi_sharp_pow_lock);
		LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s mipi_sharp_wkq_status OFF]\n", __LINE__, __FUNCTION__);
		return err;
	}

	mutex_unlock(&mipi_sharp_pow_lock);

	queue_work(sharp_workqueue_data_p->mipi_sharp_refresh_workqueue,
				&sharp_workqueue_data_p->backlight_workq_initseq);

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s]\n", __LINE__, __FUNCTION__);

	return err;
}

STATIC int mipi_sharp_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	unsigned long flag;


	LCD_DEBUG_IN_LOG

	mfd = platform_get_drvdata(pdev);

	if (!mfd){
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	gmfd = mfd;

	mipi_sharp_eeprom_read();

	if(mfd->hw_force_reset)
	{
	/***   Rupy Sysdes sequence1 CALL   ***/
		mipi_sharp_lcd_power_on_init();
		mfd->hw_force_reset = FALSE;
		refresh_force_rst_flg = FALSE;
	}

	/***   Rupy Sysdes sequence6 CALL   ***/
	mipi_sharp_lcd_power_standby_on_init();

/***   Rupy Sysdes sequence2 START   ***/
	LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 2-1/2-2 #########");
	mipi_set_tx_power_mode(1); /* LP11 mode */

	mipi_dsi_sharp_cmd_sync(mfd); /* vsync wait */

	/***   Rupy Sysdes sequence2 - "Send DSI command"  ***/
	mipi_dsi_cmds_tx(&sharp_tx_buf,
		sharp_blc_setting_cmds, ARRAY_SIZE(sharp_blc_setting_cmds));

	mipi_set_tx_power_mode(0); /* HS mode   */

	mdp4_dsi_video_blackscreen_overlay(mfd);

	sharp_workqueue_data_p->workq_mfd = mfd;

	sharp_workqueue_data_p->bl_mode_status_old = LEDDRV_NORMAL_MODE;
	sharp_workqueue_data_p->bl_mode_status = LEDDRV_NORMAL_MODE;

	mipi_sharp_blc_on(mfd);

	spin_lock_irqsave(&mdp_spin_lock, flag);
	mipi_sharp_blc_cnt = MIPI_SHARP_BLC_CNT_INIT;
	mipi_sharp_blc_status = MIPI_SHARP_ON;
	spin_unlock_irqrestore(&mdp_spin_lock, flag);

#ifdef MIPI_SHARP_DEBUG_LOG
{
	uint32 dsi_ctrl;
	int video_mode;
	dsi_ctrl = MIPI_INP(MDP_BASE + DSI_VIDEO_BASE + 0x0000);
	video_mode = dsi_ctrl & 0x01; /* VIDEO_MODE_EN */
	pr_info("### %s, %d: MDP_DSI_VIDEO_EN = 0x%X, DSI_VIDEO_EN = 0x%X ###\n", __func__, __LINE__, dsi_ctrl, video_mode);
}
#endif

	LCD_DEBUG_OUT_LOG

	return 0;
}


int mipi_sharp_lcd_disp_on(struct platform_device *pdev)
{

	struct msm_fb_data_type *mfd;
	LCD_DEBUG_IN_LOG
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 2-3.2 #########");
	msleep(60);


	if (TM_GET_PID(mfd->panel.id) == MIPI_DSI_PANEL_HD_PT){
	LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 2-4/2-5 #########");
		mipi_dsi_sharp_cmd_sync(mfd); //vsync wait
		mipi_dsi_cmds_tx( &sharp_tx_buf,
			sharp_hd_display_on_cmds,
			ARRAY_SIZE(sharp_hd_display_on_cmds));
	}
	else{
		return -EINVAL;
	}

	mipi_sharp_set_backlight_sub(sharp_workqueue_data_p->bl_brightness);

	mutex_lock(&mipi_sharp_pow_lock);

	mipi_sharp_wkq_status = MIPI_SHARP_ON;

	mutex_unlock(&mipi_sharp_pow_lock);

	mipi_sharp_blc_timer_set(mipi_sharp_blc_cycle_timer);

	mipi_sharp_refresh_timer_set();

	mutex_lock(&mipi_sharp_vblind_lock);
 	if(mipi_sharp_vblind_info.status == MIPI_SHARP_ON)
	{
		/*ViewBlind ON*/
		mipi_sharp_vblind_on(gmfd);
	}
	mutex_unlock(&mipi_sharp_vblind_lock);
		

	LCD_DEBUG_LOG("######### LCD Sequence 2-6 #########");
	leddrv_lcdbl_led_on();
	LCD_DEBUG_OUT_LOG
	return 0;	

}
	

STATIC int mipi_sharp_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	unsigned long flag;
	long long vtime;


	LCD_DEBUG_IN_LOG

	mfd = platform_get_drvdata(pdev);

	if (!mfd){
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	if(refresh_force_rst_flg == FALSE)
	{
		LCD_DEBUG_LOG("######### LCD Sequence 3-1/3-2/3-3/3-4 #########");
		mipi_sharp_set_backlight_sub(0);
		mdp4_dsi_video_blackscreen_overlay(mfd);

		msleep(40);

		spin_lock_irqsave(&mdp_spin_lock, flag);
		mipi_sharp_blc_status = MIPI_SHARP_OFF;
		spin_unlock_irqrestore(&mdp_spin_lock, flag);

		mipi_set_tx_power_mode(0);  /*HS mode*/

		mipi_dsi_cmds_tx(&sharp_tx_buf, sharp_display_off_cmds1,
			ARRAY_SIZE(sharp_display_off_cmds1));
		mdp4_dsi_video_wait4vsync(0,&vtime);
		usleep(100);

		mipi_dsi_cmds_tx(&sharp_tx_buf, sharp_display_off_cmds2,
			ARRAY_SIZE(sharp_display_off_cmds2));

	}
	
	LCD_DEBUG_OUT_LOG

	return 0;
}

void mipi_sharp_lcd_off_cmd(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	LCD_DEBUG_IN_LOG

	mfd = platform_get_drvdata(pdev);	
	if (!mfd)
		return;

	LCD_DEBUG_LOG("######### LCD Sequence 3-6 #########");
	mipi_set_tx_power_mode(1); /* LP11 mode */

/***   Rupy Sysdes sequence3 END   ***/



/***   Rupy Sysdes sequence5 START   ***/
	if(refresh_force_rst_flg == FALSE){
	    LCD_DEBUG_LOG("######### LCD Sequence 5 #########");

		mipi_dsi_op_mode_config(DSI_CMD_MODE);

		LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 5.1 #########");
		mipi_dsi_sharp_cmd_sync(mfd); //vsync wait
		mipi_dsi_cmds_tx(&sharp_tx_buf, &sharp_deep_standby_cmds1,1);
		
		LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 5.2 #########");
		mipi_dsi_sharp_cmd_sync(mfd); //vsync wait
		mipi_dsi_cmds_tx(&sharp_tx_buf, &sharp_NOP_cmd,1);

		LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 5.3 #########");
		mipi_dsi_sharp_cmd_sync(mfd); //vsync wait
		mipi_dsi_cmds_tx(&sharp_tx_buf, &sharp_NOP_cmd,1);

		LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 5.4/5.5 #########");
		mipi_dsi_sharp_cmd_sync(mfd); //vsync wait
		mipi_dsi_cmds_tx(&sharp_tx_buf, &sharp_deep_standby_cmds2,1);

		LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 5.6 #########");
		gpio_set_value_cansleep(gpio_lcd_dcdc, 0);

		msleep(20);
/***   Rupy Sysdes sequence5 END   ***/
	}


	LCD_DEBUG_OUT_LOG
	return;
}


STATIC void mipi_sharp_vblind_on(struct msm_fb_data_type *mfd)
{
	struct t_mipi_sharp_vblind_lut_data lut_data;
	unsigned char pmdp_dmap_gc_start = 6;
	LCD_DEBUG_IN_LOG
	
	if(mipi_sharp_vblind_info.strength == 1){      /*normal*/
		pmdp_dmap_gc_start = 6;
	}else if(mipi_sharp_vblind_info.strength == 2){/*strength*/
		pmdp_dmap_gc_start = 0;
	}else if(mipi_sharp_vblind_info.strength == 0){/*weak*/
		pmdp_dmap_gc_start = 12;
	}else{
		LCD_DEBUG_OUT_LOG
		return;
	}
	
	memset(&lut_data, 0x00, sizeof(lut_data));
	lut_data.flags = mipi_sharp_vblind_info.status;

	lut_data.r_data.x_start = ((uint32)(0x0f & ViewBlind_MDP_DMAP_GC[0][pmdp_dmap_gc_start]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[0][pmdp_dmap_gc_start+1]) ;
	lut_data.r_data.slope   = ((uint32)(0x7f & ViewBlind_MDP_DMAP_GC[0][pmdp_dmap_gc_start+2]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[0][pmdp_dmap_gc_start+3]) ;
	lut_data.r_data.offset  = ((uint32)(0x7f & ViewBlind_MDP_DMAP_GC[0][pmdp_dmap_gc_start+4]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[0][pmdp_dmap_gc_start+5]) ;
							   
	lut_data.g_data.x_start = ((uint32)(0x0f & ViewBlind_MDP_DMAP_GC[1][pmdp_dmap_gc_start]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[1][pmdp_dmap_gc_start+1]) ;
	lut_data.g_data.slope   = ((uint32)(0x7f & ViewBlind_MDP_DMAP_GC[1][pmdp_dmap_gc_start+2]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[1][pmdp_dmap_gc_start+3]);
	lut_data.g_data.offset  = ((uint32)(0x7f & ViewBlind_MDP_DMAP_GC[1][pmdp_dmap_gc_start+4]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[1][pmdp_dmap_gc_start+5]);
							   
	lut_data.b_data.x_start = ((uint32)(0x0f & ViewBlind_MDP_DMAP_GC[2][pmdp_dmap_gc_start]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[2][pmdp_dmap_gc_start+1]);
	lut_data.b_data.slope   = ((uint32)(0x7f & ViewBlind_MDP_DMAP_GC[2][pmdp_dmap_gc_start+2]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[2][pmdp_dmap_gc_start+3]);
	lut_data.b_data.offset  = ((uint32)(0x7f & ViewBlind_MDP_DMAP_GC[2][pmdp_dmap_gc_start+4]) << 8) |
							   (uint32)(0xff & ViewBlind_MDP_DMAP_GC[2][pmdp_dmap_gc_start+5]) ;

	LCD_DEBUG_LOG_DETAILS("ViewBlind LUT_R x_start[%x] offset[%x] slope[%x]", 
		lut_data.r_data.x_start , lut_data.r_data.offset ,lut_data.r_data.slope );
	LCD_DEBUG_LOG_DETAILS("ViewBlind LUT_G x_start[%x] offset[%x] slope[%x]", 
		lut_data.g_data.x_start , lut_data.g_data.offset ,lut_data.g_data.slope );
	LCD_DEBUG_LOG_DETAILS("ViewBlind LUT_B x_start[%x] offset[%x] slope[%x]", 
		lut_data.b_data.x_start , lut_data.b_data.offset ,lut_data.b_data.slope );
	
	mipi_sharp_vblind_set_lut( mfd ,&lut_data);

	LCD_DEBUG_OUT_LOG
	return;
}


STATIC void mipi_sharp_vblind_off(struct msm_fb_data_type *mfd)
{
	struct t_mipi_sharp_vblind_lut_data lut_data;

	LCD_DEBUG_IN_LOG
	memset(&lut_data, 0x00, sizeof(lut_data));
	
	lut_data.flags = mipi_sharp_vblind_info.status;
	mipi_sharp_vblind_set_lut( mfd, &lut_data);

	LCD_DEBUG_OUT_LOG
	return ;
}

STATIC void mipi_sharp_vblind_set_lut( struct msm_fb_data_type *mfd , struct t_mipi_sharp_vblind_lut_data *lut_data)
{
	/*MDP_DMA_P_CONFIG*/
	uint32_t *config_offset;
    /*MDP_DMA_P_GC_START_COLOR_0_STAGE0*/
	uint32_t *c0_offset;
	/*MDP_DMA_P_GC_PARAM_COLOR_0_STAGE0*/
	uint32_t *c0_params_offset;
	/*MDP_DMA_P_GC_START_COLOR_1_STAGE0*/
	uint32_t *c1_offset;
	/*MDP_DMA_P_GC_PARAM_COLOR_1_STAGE0*/
	uint32_t *c1_params_offset;
	/*MDP_DMA_P_GC_START_COLOR_2_STAGE0*/
	uint32_t *c2_offset;
	/*MDP_DMA_P_GC_PARAM_COLOR_2_STAGE0*/
	uint32_t *c2_params_offset;
	
	LCD_DEBUG_IN_LOG
	
	if(lut_data == NULL){
		LCD_DEBUG_OUT_LOG
		return;
	}
	
	config_offset = (uint32_t*)(MDP_BASE + MDP_DMA_P_OFFSET);
	
	if(lut_data->flags == MIPI_SHARP_ON){
		c0_offset          = (uint32_t *)(MDP_BASE + MDP_DMA_P_OFFSET + MDP_DMA_GC_OFFSET);
		c0_params_offset   = (uint32_t *)((uint32_t)c0_offset + MDP_GC_PARMS_OFFSET);
		c1_offset          = (uint32_t *)((uint32_t)c0_offset + MDP_GC_COLOR_OFFSET);
		c1_params_offset   = (uint32_t *)((uint32_t)c1_offset + MDP_GC_PARMS_OFFSET);
		c2_offset          = (uint32_t *)((uint32_t)c0_offset + 2*MDP_GC_COLOR_OFFSET);
		c2_params_offset   = (uint32_t *)((uint32_t)c2_offset + MDP_GC_PARMS_OFFSET);

		mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
		/*vsync wait*/
		mipi_dsi_sharp_cmd_sync(mfd);
		outpdw(c0_offset,
				((0xfff & lut_data->r_data.x_start)
				| 0x10000));  /*STAGE_ENABLE*/
		outpdw(c0_params_offset,
				((0x7fff & lut_data->r_data.slope) | 
				((0x7fff & lut_data->r_data.offset) << 16)));
		
		outpdw(c1_offset,
				((0xfff & lut_data->b_data.x_start)
				| 0x10000));  /*STAGE_ENABLE*/
		outpdw(c1_params_offset,
				((0x7fff & lut_data->b_data.slope) |
				((0x7fff & lut_data->b_data.offset) << 16)));
		
		outpdw(c2_offset,
				((0xfff & lut_data->g_data.x_start)
				| 0x10000));  /*STAGE_ENABLE*/

		outpdw(c2_params_offset,
				((0x7fff & lut_data->g_data.slope) |
				((0x7fff & lut_data->g_data.offset) << 16)));
		
		LCD_DEBUG_LOG_DETAILS("  c0_offset:%p = %x", c0_offset, MIPI_INP(c0_offset));
		LCD_DEBUG_LOG_DETAILS("  c0_params_offset:%p = %x\n", c0_params_offset, inpdw(c0_params_offset));
		LCD_DEBUG_LOG_DETAILS("  c1_offset:%p = %x", c1_offset, MIPI_INP(c1_offset));
		LCD_DEBUG_LOG_DETAILS("  c1_params_offset:%p = %x\n", c1_params_offset, inpdw(c1_params_offset));
		LCD_DEBUG_LOG_DETAILS("  c2_offsett:%p = %x", c2_offset, MIPI_INP(c2_offset));
		LCD_DEBUG_LOG_DETAILS("  c2_params_offset:%p = %x\n", c2_params_offset, inpdw(c2_params_offset));
		
		/*PGC_EN:ON*/
		outpdw(config_offset, 
				(inpdw(config_offset) & (uint32_t)(~(0x1<<28))) |
				((0x1 & lut_data->flags) << 28));
		LCD_DEBUG_LOG_DETAILS("  config_offset:%p = %x", config_offset, inpdw(config_offset));
		mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);
	}else{
		mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
		/*vsync wait*/
		mipi_dsi_sharp_cmd_sync(mfd);
		/*PGC_EN:OFF*/
		outpdw(config_offset,
				(inpdw(config_offset) & (uint32_t)(~(0x1<<28))) |
				((0x1 & lut_data->flags) << 28));
		LCD_DEBUG_LOG_DETAILS("  config_offset:%p = %x", config_offset, inpdw(config_offset));
		mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);
	}
	LCD_DEBUG_OUT_LOG
	return ;
}


STATIC void mipi_sharp_blc_on(struct msm_fb_data_type *mfd)
{
	int bl_level;
	u8 mode;
	u8 bl_mode;

	LCD_DEBUG_IN_LOG

	LCD_DEBUG_LOG("bl_mode_status = [%x]: brightness = [%x]\n",
			sharp_workqueue_data_p->bl_mode_status, sharp_workqueue_data_p->bl_brightness);

	sharp_workqueue_data_p->bl_blc_brightness = 0;
	bl_level = sharp_workqueue_data_p->bl_brightness;
	bl_mode  = sharp_workqueue_data_p->bl_mode_status;

	mipi_sharp_set_backlight_sub(bl_level);

	mode = blc_select[bl_mode] - 1;

	memcpy(Backlight_CTL_B9,
		&Backlight_CTL_B9_MODE[mode][0],
		sizeof(Backlight_CTL_B9));

	LCD_DEBUG_LOG_DETAILS("BLC B9 Parameter [%02x][%02x][%02x][%02x][%02x][%02x][%02x]\n",
		Backlight_CTL_B9[1],Backlight_CTL_B9[2],Backlight_CTL_B9[3],Backlight_CTL_B9[4],
		Backlight_CTL_B9[5],Backlight_CTL_B9[6],Backlight_CTL_B9[7]);

	mipi_dsi_sharp_cmd_sync(mfd); /* vsync wait */

	mipi_dsi_cmds_tx(&sharp_tx_buf, sharp_blc_mode_change_cmds, ARRAY_SIZE(sharp_blc_mode_change_cmds));

	LCD_DEBUG_OUT_LOG

	return;
}

STATIC int mipi_sharp_set_display_mode(enum leddrv_lcdbl_mode_status led_mode)
{
	int mode;

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  IN ][%d %s]\n", __LINE__, __FUNCTION__);

	switch (led_mode){
		case LEDDRV_NORMAL_MODE:
			mode = MIPI_SHARP_NORMAL_MODE;
			LCD_DEBUG_LOG_DETAILS("### mode = NORMAL ###");
			break;
		case LEDDRV_HIGHBRIGHT_MODE:
		case LEDDRV_POWER_MODE:
			mode = MIPI_SHARP_CAMERA_MODE;
			LCD_DEBUG_LOG_DETAILS("### mode = CAMERA ###");
			break;
		case LEDDRV_ECO_MODE:
			mode = MIPI_SHARP_ECO_MODE;
			LCD_DEBUG_LOG_DETAILS("### mode = ECO ###");
			break;
		default:
			LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s err]\n", __LINE__, __FUNCTION__);
			return(-1);
	}

	sharp_workqueue_data_p->bl_mode_status = mode;

	LCD_DEBUG_LOG_DETAILS("mode = %d, led_mode = %d\n", mode, led_mode);

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s]\n", __LINE__, __FUNCTION__);

	return 0;
}

STATIC void mipi_sharp_set_backlight_sub(int bl_level)
{
	int ret;
	int pwm_level;
	int pwm_period;

	LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  IN ][%d %s]\n", __LINE__, __FUNCTION__);

	pwm_period = mipi_sharp_pwm_period[sharp_workqueue_data_p->bl_mode_status_old];

	pwm_level = ((((bl_level * MIPI_SHARP_PWM_DUTY_CALC) * pwm_period) /
				MIPI_SHARP_PWM_LEVEL) + 5) / MIPI_SHARP_PWM_DUTY_CALC;

	LCD_DEBUG_LOG_DETAILS("bl_level = %d , pwm_period = %d , pwm_level = %d \n",
							bl_level, pwm_period, pwm_level);

	if (bl_lpm) {
		ret = pwm_config(bl_lpm, pwm_level, pwm_period);
		if (ret) {
			pr_err("pwm_config on lpm failed %d\n", ret);
			LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s]\n", __LINE__, __FUNCTION__);
			return;
		}

		if (bl_level) {
			ret = pwm_enable(bl_lpm);

			mipi_sharp_backlight_status = MIPI_SHARP_ON;

			if (ret)
				pr_err("pwm enable/disable on lpm failed"
					"for bl %d\n",	bl_level);
		} else {
			if(mipi_sharp_backlight_status == MIPI_SHARP_ON)
			{
				pwm_disable(bl_lpm);
			}
			mipi_sharp_backlight_status = MIPI_SHARP_OFF;
		}
	}

        LCD_DEBUG_LOG_DETAILS("[SHARP_LCD  OUT ][%d %s]\n", __LINE__, __FUNCTION__);
}

STATIC int __devinit mipi_sharp_lcd_probe(struct platform_device *pdev)
{
	int rc;
	struct sharp_workqueue_data *td;


	LCD_DEBUG_IN_LOG

	if (pdev->id == 0) {
		mipi_sharp_pdata = pdev->dev.platform_data;
		LCD_DEBUG_OUT_LOG
		return 0;
	}
	if (mipi_sharp_pdata == NULL) {
		pr_err("%s.invalid platform data.\n", __func__);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	if (mipi_sharp_pdata != NULL)
		bl_lpm = pwm_request(mipi_sharp_pdata->gpio[0],
			"backlight");

	if (bl_lpm == NULL || IS_ERR(bl_lpm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_lpm = NULL;
	}

	pr_debug("bl_lpm = %p lpm = %d\n", bl_lpm,
		mipi_sharp_pdata->gpio[0]);

	td = kzalloc(sizeof(*td), GFP_KERNEL);
	if (!td) {
		LCD_DEBUG_OUT_LOG
		return -ENOMEM;
	}
	sharp_workqueue_data_p = td;

	msm_fb_add_device(pdev);

	mipi_sharp_bl_tmp = MIPI_SHARP_OFF;
	mipi_sharp_backlight_status = MIPI_SHARP_OFF;

	td->bl_brightness = 0;
	td->bl_brightness_old = 0;
	td->bl_blc_brightness = 0;
	td->bl_mode_status = LEDDRV_NORMAL_MODE;
	td->bl_mode_status_old = LEDDRV_NORMAL_MODE;

	/*ViewBlind parameter initialize*/
	mipi_sharp_vblind_info.status = MIPI_SHARP_OFF;
	mipi_sharp_vblind_info.strength = 0;  /*weak  */
	
	INIT_WORK(&td->refresh_workq_initseq, mipi_sharp_refresh_work_func);
	INIT_WORK(&td->blc_workq_initseq, mipi_sharp_blc_work_func);
	INIT_WORK(&td->backlight_workq_initseq, mipi_sharp_backlight_work_func);

	td->mipi_sharp_refresh_workqueue =
		create_singlethread_workqueue("mipi_sharp_refresh");
	if (td->mipi_sharp_refresh_workqueue == NULL){
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	init_timer(&mipi_sharp_refresh_timer);

	mipi_sharp_refresh_timer.function =
		mipi_sharp_refresh_handler;
	mipi_sharp_refresh_timer.data = 0;

	init_timer(&mipi_sharp_blc_timer);

	mipi_sharp_blc_timer.function =
		mipi_sharp_blc_handler;
	mipi_sharp_blc_timer.data = 0;

	gpio_lcd_dcdc   = mipi_sharp_pdata->gpio[1];
	gpio_disp_rst_n = mipi_sharp_pdata->gpio[3];

	rc = gpio_request(gpio_lcd_dcdc, "lcd_dcdc");
	if (rc) {
		pr_err("request gpio lcd_dcdc failed, rc=%d\n", rc);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	rc = gpio_request(gpio_disp_rst_n, "disp_rst_n");
	if (rc) {
		pr_err("request gpio disp_rst_n failed, rc=%d\n", rc);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev, "dsi_vddio");
	if (IS_ERR(reg_l23)) {
		pr_err("could not get 8921_l23, rc = %ld\n",
			PTR_ERR(reg_l23));
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}
		pr_err("-------- get 8921_l23 OK , rc = %ld\n", PTR_ERR(reg_l23));
	rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
	if (rc) {
		pr_err("set_voltage l23 failed, rc=%d\n", rc);
		LCD_DEBUG_OUT_LOG
		return -EINVAL;
	}
		pr_err("-------- set_voltage l23 OK , rc=%d\n", rc);


	leddrv_lcdbl_set_handler(LEDDRV_EVENT_LCD_BACKLIGHT_CHANGED,
							 mipi_sharp_ledset_backlight);

	mipi_sharp_lcd_power_on_init();

	LCD_DEBUG_OUT_LOG

	return 0;
}

STATIC void mipi_sharp_lcd_power_on_init(void)
{
	int rc;

	LCD_DEBUG_IN_LOG

/***   Rupy Sysdes sequence1 START   ***/
	LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 1 #########");
	gpio_set_value_cansleep(gpio_disp_rst_n, 0);

	rc = regulator_set_optimum_mode(reg_l23, 100000);
	if (rc < 0) {
		pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
	}
	rc = regulator_enable(reg_l23);
	if (rc) {
		pr_err("enable l23 failed, rc=%d\n", rc);
	}

	msleep(20);

	gpio_set_value_cansleep(gpio_lcd_dcdc, 1);

	msleep(20);
	msleep(20);

	gpio_set_value_cansleep(gpio_disp_rst_n, 1);

	msleep(20);
/***   Rupy Sysdes sequence1 END   ***/
	LCD_DEBUG_OUT_LOG

	return;
}

STATIC void mipi_sharp_lcd_power_standby_on_init(void)
{
	LCD_DEBUG_IN_LOG

	if(mipi_sharp_init_flg)
	{
		/***   Rupy Sysdes sequence6 START   ***/
		LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 6 #########");
		gpio_set_value_cansleep(gpio_lcd_dcdc, 1);

		msleep(20);

		gpio_set_value_cansleep(gpio_disp_rst_n, 0);

		msleep(20);

		gpio_set_value_cansleep(gpio_disp_rst_n, 1);

		msleep(20);
		/***   Rupy Sysdes sequence6 END   ***/
	}

	mipi_sharp_init_flg = TRUE;

	LCD_DEBUG_OUT_LOG

	return;
}
STATIC int __devexit mipi_sharp_lcd_remove(struct platform_device *pdev)
{
	int rc;

	LCD_DEBUG_IN_LOG

/***   Rupy Sysdes sequence4 START   ***/
	LCD_DEBUG_LOG_DETAILS("######### LCD Sequence 4 #########");
	gpio_set_value_cansleep(gpio_disp_rst_n, 0);

	msleep(30);

	gpio_set_value_cansleep(gpio_lcd_dcdc, 0);

	msleep(20);

	mipi_set_tx_power_mode(1); /* LP11 mode */

	rc = regulator_disable(reg_l23);
	if (rc) {
		pr_err("disable reg_l23 failed, rc=%d\n", rc);
	}
	rc = regulator_set_optimum_mode(reg_l23, 100);
	if (rc < 0) {
		pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
	}

/***   Rupy Sysdes sequence4 END   ***/

	pwm_free(bl_lpm);

	destroy_workqueue(sharp_workqueue_data_p->mipi_sharp_refresh_workqueue);

	kfree(sharp_workqueue_data_p);

	gpio_free(gpio_lcd_dcdc);
	gpio_free(gpio_disp_rst_n);

	LCD_DEBUG_OUT_LOG
	return 0;
}

STATIC struct platform_driver this_driver = {
	.probe  = mipi_sharp_lcd_probe,
	.remove = __devexit_p(mipi_sharp_lcd_remove),
	.driver = {
		.name   = "mipi_sharp",
	},
};

STATIC struct msm_fb_panel_data sharp_panel_data = {
	.on		= mipi_sharp_lcd_on,
	.off		= mipi_sharp_lcd_off,
	.sysfs_register = mipi_sharp_lcd_sysfs_register,
	.sysfs_unregister = mipi_sharp_lcd_sysfs_unregister,
	.pre_on		= mipi_sharp_lcd_pre_on,
	.pre_off	= mipi_sharp_lcd_pre_off,
};

STATIC int ch_used[3];

int mipi_sharp_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;


	LCD_DEBUG_IN_LOG

	if ((channel >= 3) || ch_used[channel]){
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	ch_used[channel] = TRUE;

	ret = mipi_sharp_lcd_init();
	if (ret) {
		pr_err("mipi_sharp_lcd_init() failed with ret %u\n", ret);
		LCD_DEBUG_OUT_LOG
		return ret;
	}

	pdev = platform_device_alloc("mipi_sharp", (panel << 8)|channel);
	if (!pdev){
		LCD_DEBUG_OUT_LOG
		return -ENOMEM;
	}

	sharp_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &sharp_panel_data,
		sizeof(sharp_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	LCD_DEBUG_OUT_LOG

	return 0;

err_device_put:
	platform_device_put(pdev);

	LCD_DEBUG_OUT_LOG

	return ret;
}

STATIC int mipi_sharp_lcd_init(void)
{
	LCD_DEBUG_IN_LOG

	mipi_dsi_buf_alloc(&sharp_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&sharp_rx_buf, DSI_BUF_SIZE);

	LCD_DEBUG_OUT_LOG

	return platform_driver_register(&this_driver);
}

static ssize_t mipi_sharp_store_vblind_setting(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)fbi->par;
	int intbuf;
	int r;

	LCD_DEBUG_IN_LOG

	r = kstrtoint(buf, 0, &intbuf);

	if (r != 0){
		LCD_DEBUG_OUT_LOG
		return r;
	}

	if ((intbuf != 0) && (intbuf != 1)){
		LCD_DEBUG_OUT_LOG
		return -EINVAL;
	}

	mutex_lock(&mipi_sharp_vblind_lock);

	if((intbuf == 0) && (mipi_sharp_vblind_info.status == MIPI_SHARP_ON))
	{
		/*ViewBlind OFF*/
		mipi_sharp_vblind_info.status = MIPI_SHARP_OFF;
		mipi_sharp_vblind_off(mfd);
	}
	else if((intbuf == 1) && (mipi_sharp_vblind_info.status == MIPI_SHARP_OFF))
	{
		/*ViewBlind ON*/
		mipi_sharp_vblind_info.status = MIPI_SHARP_ON;
		mipi_sharp_vblind_on(mfd);
	}
	else
	{
		;
	}
	mutex_unlock(&mipi_sharp_vblind_lock);

	LCD_DEBUG_OUT_LOG
	return count;
}

static ssize_t mipi_sharp_show_vblind_setting(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int value;

	LCD_DEBUG_IN_LOG
	mutex_lock(&mipi_sharp_vblind_lock);

	value = mipi_sharp_vblind_info.status;

	mutex_unlock(&mipi_sharp_vblind_lock);

	LCD_DEBUG_OUT_LOG
	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t mipi_sharp_store_vblind_strength(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)fbi->par;
	int intbuf;
	int r;

	LCD_DEBUG_IN_LOG

	r = kstrtoint(buf, 0, &intbuf);

	if (r != 0){
		LCD_DEBUG_OUT_LOG
		return r;
	}

	if ((intbuf < 0) || (intbuf >= VIEWBLIND_STRENGTH_LEVEL_MAX)){
		LCD_DEBUG_OUT_LOG
		return -EINVAL;
	}

	mutex_lock(&mipi_sharp_vblind_lock);

	if(mipi_sharp_vblind_info.status == MIPI_SHARP_ON)
	{
		if(mipi_sharp_vblind_info.strength != intbuf){
			mipi_sharp_vblind_info.strength = intbuf;
			/*ViewBlind ON*/
			mipi_sharp_vblind_on(mfd);
		}
	}else{
		mipi_sharp_vblind_info.strength = intbuf;
	}

	mutex_unlock(&mipi_sharp_vblind_lock);

	LCD_DEBUG_OUT_LOG
	return count;
}

static ssize_t mipi_sharp_show_vblind_strength(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int value;

	LCD_DEBUG_IN_LOG

	mutex_lock(&mipi_sharp_vblind_lock);

	value = mipi_sharp_vblind_info.strength;

	mutex_unlock(&mipi_sharp_vblind_lock);

	LCD_DEBUG_OUT_LOG
	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}



#ifdef MIPI_SHARP_CMD_FUNC
STATIC ssize_t debug_mipi_sharp_store_cmd_tx(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)fbi->par;
	struct dsi_cmd_desc sharp_cmds;
	u8 value;
	int r,i,cnt = 0;
	u8 buff[3];
	unsigned char *data;
	int cmd_len;

	LCD_DEBUG_IN_LOG

	mutex_lock(&mipi_sharp_pow_lock);

	if(mipi_sharp_wkq_status == MIPI_SHARP_OFF)
	{
		mutex_unlock(&mipi_sharp_pow_lock);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	cmd_len = ((count - 1) / 2) - 1;
	data = kmalloc(cmd_len, GFP_KERNEL);

	/* Command initialization */
	sharp_cmds.last = 1;
	sharp_cmds.vc   = 0;
	sharp_cmds.ack  = 0;
	sharp_cmds.wait = 0;
	sharp_cmds.dlen = cmd_len;
	sharp_cmds.payload = data;

	for(i=0 ;i<count-1; i=i+2)
	{
		buff[0] = buf[i];
		buff[1] = buf[i+1];
		buff[2] = '\0';

		r = kstrtou8(buff, 16, &value);
		if (r != 0){
			mutex_unlock(&mipi_sharp_pow_lock);
			kfree(data);
			LCD_DEBUG_OUT_LOG
			return -EINVAL;
		}

		if( i == 0)
		{
			sharp_cmds.dtype = value;
			pr_info("DATA IDENTIFIER:0x%X\n",value);
		}
		else
		{
			data[cnt++] = value;
			pr_info("DATA[%d]:0x%X",cnt,value);
		}
	}


	mipi_set_tx_power_mode(debug_mipi_sharp_mode_value);

	mipi_dsi_sharp_cmd_sync(mfd); /* vsync wait */

	mipi_dsi_cmds_tx(&sharp_tx_buf, &sharp_cmds, 1);

	mipi_set_tx_power_mode(0);

	mutex_unlock(&mipi_sharp_pow_lock);

	kfree(data);

	pr_info("\n Transmission completed !\n");

	LCD_DEBUG_OUT_LOG

	return count;
}
STATIC ssize_t debug_mipi_sharp_show_cmd_tx(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	LCD_DEBUG_IN_LOG

	LCD_DEBUG_OUT_LOG

	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

STATIC ssize_t debug_mipi_sharp_store_cmd_rx(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)fbi->par;
	struct dsi_cmd_desc sharp_cmds;
	u8 value;
	int r,i,cnt = 0;
	u8 buff[3];
	unsigned char *data;
	int cmd_len;


	LCD_DEBUG_IN_LOG

	mutex_lock(&mipi_sharp_pow_lock);

	if(mipi_sharp_wkq_status == MIPI_SHARP_OFF)
	{
		mutex_unlock(&mipi_sharp_pow_lock);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	cmd_len = ((count - 1) / 2) - 1;
	data = kmalloc(cmd_len, GFP_KERNEL);

	/* Command initialization */
	sharp_cmds.last = 1;
	sharp_cmds.vc   = 0;
	sharp_cmds.ack  = 1;
	sharp_cmds.wait = 0;
	sharp_cmds.dlen = cmd_len;
	sharp_cmds.payload = data;

	for(i=0 ;i<count-1; i=i+2)
	{
		buff[0] = buf[i];
		buff[1] = buf[i+1];
		buff[2] = '\0';

		r = kstrtou8(buff, 16, &value);
		if (r != 0){
			mutex_unlock(&mipi_sharp_pow_lock);
			kfree(data);
			LCD_DEBUG_OUT_LOG
			return -EINVAL;
		}

		if( i == 0)
		{
			sharp_cmds.dtype = value;
			pr_info("DATA IDENTIFIER:0x%X\n",value);
		}
		else
		{
			data[cnt++] = value;
			pr_info("DATA[%d]:0x%X",cnt,value);
		}
	}


	mipi_set_tx_power_mode(debug_mipi_sharp_mode_value);

	mipi_dsi_sharp_cmd_sync(mfd); /* vsync wait */

	mipi_dsi_cmds_rx(mfd, &sharp_tx_buf, &sharp_rx_buf, &sharp_cmds, 1);

	mipi_set_tx_power_mode(0);

	debug_mipi_sharp_rx_value = (uint32)*(sharp_rx_buf.data);

	mutex_unlock(&mipi_sharp_pow_lock);

	kfree(data);

	pr_info("\n Reception completed ! Value = 0x%X\n",(uint32)*(sharp_rx_buf.data));

	LCD_DEBUG_OUT_LOG

	return count;
}
STATIC ssize_t debug_mipi_sharp_show_cmd_rx(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	LCD_DEBUG_IN_LOG

	LCD_DEBUG_OUT_LOG

	return snprintf(buf, PAGE_SIZE, "%d\n", debug_mipi_sharp_rx_value);
}

STATIC ssize_t debug_mipi_sharp_store_cmd_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int intbuf;
	int r;

	LCD_DEBUG_IN_LOG

	r = kstrtoint(buf, 0, &intbuf);

	if (r != 0){
		LCD_DEBUG_OUT_LOG
		return -EINVAL;
	}

	if(intbuf == 1)
	{
		debug_mipi_sharp_mode_value = intbuf;
		pr_info("MIPI:LP mode\n");
	}
	else if(intbuf == 0)
	{
		mipi_set_tx_power_mode(0); /* HS mode   */
		debug_mipi_sharp_mode_value = intbuf;
		pr_info("MIPI:LP mode\n");
	}
	else
	{
		LCD_DEBUG_OUT_LOG
		return -EINVAL;
	}

	LCD_DEBUG_OUT_LOG

	return count;
}
STATIC ssize_t debug_mipi_sharp_show_cmd_mode(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	LCD_DEBUG_IN_LOG

	LCD_DEBUG_OUT_LOG

	return snprintf(buf, PAGE_SIZE, "%d\n", debug_mipi_sharp_mode_value);
}
#endif

STATIC struct device_attribute mipi_sharp_lcd_attributes[] = {
	__ATTR(vblind_setting, S_IRUGO|S_IWUSR,
			mipi_sharp_show_vblind_setting,  mipi_sharp_store_vblind_setting),
	__ATTR(vblind_strength, S_IRUGO|S_IWUSR,
			mipi_sharp_show_vblind_strength, mipi_sharp_store_vblind_strength),
#ifdef MIPI_SHARP_CMD_FUNC
	__ATTR(mipi_cmd_tx, S_IRUGO|S_IWUSR,
			debug_mipi_sharp_show_cmd_tx, debug_mipi_sharp_store_cmd_tx),
	__ATTR(mipi_cmd_rx, S_IRUGO|S_IWUSR,
			debug_mipi_sharp_show_cmd_rx, debug_mipi_sharp_store_cmd_rx),
	__ATTR(mipi_cmd_mode, S_IRUGO|S_IWUSR,
			debug_mipi_sharp_show_cmd_mode, debug_mipi_sharp_store_cmd_mode),
#endif /* MIPI_SHARP_CMD_FUNC */
};

STATIC int mipi_sharp_lcd_sysfs_register(struct device *dev)
{
	int i;


	LCD_DEBUG_IN_LOG

	for (i = 0; i < ARRAY_SIZE(mipi_sharp_lcd_attributes); i++)
		if (device_create_file(dev, &mipi_sharp_lcd_attributes[i]))
			goto error;

	LCD_DEBUG_OUT_LOG

	return 0;

error:
	for (; i >= 0 ; i--)
		device_remove_file(dev, &mipi_sharp_lcd_attributes[i]);
	pr_err("%s: Unable to create interface\n", __func__);

	LCD_DEBUG_OUT_LOG

	return -ENODEV;
}

STATIC void mipi_sharp_lcd_sysfs_unregister(struct device *dev)
{
	int i;

	LCD_DEBUG_IN_LOG

	for (i = 0; i < ARRAY_SIZE(mipi_sharp_lcd_attributes); i++)
		device_remove_file(dev, &mipi_sharp_lcd_attributes[i]);

	LCD_DEBUG_OUT_LOG
}
