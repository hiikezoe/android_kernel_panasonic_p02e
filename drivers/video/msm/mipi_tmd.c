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
#include "mipi_tmd.h"
#include "mdp4.h"
#include <linux/cfgdrv.h>
#include <linux/hcm_eep.h>
#include <linux/gpio.h>
#include <linux/leds.h>

#include "../../../arch/arm/mach-msm/devices.h"
#include "../../../arch/arm/mach-msm/board-8064.h"


#define TM_GET_PID(id) (((id) & 0xff00)>>8)
DEFINE_MUTEX(mipi_tmd_pow_lock);
DEFINE_MUTEX(mipi_tmd_vblind_lock);

#define MIPI_TMD_MAX_LOOP 10

static u8 lcdon_cmd_flg = FALSE;

static struct pwm_device *bl_lpm;
static struct mipi_dsi_panel_platform_data *mipi_tmd_pdata;
static struct dsi_buf tmd_tx_buf;
static struct dsi_buf tmd_rx_buf;
static int gpio_lcd_dcdc;                                    /* GPIO:LCD DCDC */
static int gpio_disp_rst_n;                                  /* GPIO:RESET    */
static int pmic_gpio43;             	                     /* GPIO:TEST    */
static int pmic_gpio59;             	                     /* GPIO:TEST    */
struct timer_list mipi_tmd_refresh_timer;
struct timer_list mipi_tmd_blc_timer;
static struct tmd_workqueue_data *tmd_workqueue_data_p;
static void mipi_tmd_backlight_work_func(struct work_struct *work);
struct tmd_workqueue_data {
	struct workqueue_struct *mipi_tmd_refresh_workqueue;
	struct work_struct       refresh_workq_initseq;
	struct work_struct       blc_workq_initseq;
	struct work_struct       backlight_workq_initseq;
	struct msm_fb_data_type *workq_mfd;
	
	int bl_brightness;
	int bl_brightness_old;
	int bl_blc_brightness;
	u8  bl_mode_status;
	u8  bl_mode_status_old;
};
static struct regulator *reg_l23;

struct msm_fb_data_type *gmfd;

static u8 mipi_tmd_init_flg = FALSE;
static u8 mipi_tmd_wkq_status = MIPI_TMD_OFF;
#ifdef MIPI_TMD_FUMC_REFRESH_REWRITE
static u8 mipi_tmd_reg_rewrite   = MIPI_TMD_ON;
#endif
#ifdef MIPI_TMD_FUMC_REFRESH_RESET 
static u8 mipi_tmd_hw_reset      = MIPI_TMD_ON;
#endif
#ifdef MIPI_TMD_FUMC_REFRESH
static u8 mipi_tmd_refresh_cycle = 15;				/* 15 sec	*/
#endif
static unsigned long  mipi_tmd_blc_cycle_timer = 20;		/* 20msec       */
#ifdef MIPI_TMD_FUNC_BLC
static unsigned long  mipi_tmd_blc_cycle_timer_delay = 1000;	/* 1sec         */
#endif
struct t_mipi_tmd_vblind_info_tbl mipi_tmd_vp_info;
#ifdef MIPI_TMD_CMD_FUNC
static uint32 debug_mipi_tmd_rx_value = 0;
static uint32 debug_mipi_tmd_mode_value = 0;
static int mipi_tmd_bl_tmp      = MIPI_TMD_OFF;
#endif
static int mipi_tmd_blc_cnt = 0;
static int mipi_tmd_blc_status = MIPI_TMD_OFF;
static u8  mipi_tmd_backlight_status = MIPI_TMD_OFF;
extern spinlock_t mdp_spin_lock;

static void mipi_tmd_wkq_timer_stop(u8 change_status, u8 stop_timer);
static int  mipi_tmd_lcd_init(void);
static void mipi_tmd_lcd_power_on_init(void);
static void mipi_tmd_lcd_power_standby_on_init(void);
static void mipi_tmd_refresh_timer_set(void);
static void mipi_tmd_eeprom_read(void);
static void mipi_tmd_refresh_handler(unsigned long data);
static void mipi_tmd_refresh_work_func(struct work_struct *work);
static void mipi_tmd_blc_handler(unsigned long data);
static void mipi_tmd_blc_work_func(struct work_struct *work);
static void mipi_tmd_reg_rewrite_seq(struct msm_fb_data_type *mfd);
static void mipi_tmd_set_backlight_sub(int bl_level);
static void mipi_tmd_blc_on(struct msm_fb_data_type *mfd);
static u8   mipi_tmd_read_rdpwm(struct msm_fb_data_type *mfd, int brightness_level);
static int  mipi_tmd_set_display_mode(enum mipi_tmd_display_mode disp_mode, 
								enum leddrv_lcdbl_mode_status led_mode);
static int mipi_tmd_lcd_sysfs_register(struct device *dev);
static void mipi_tmd_lcd_sysfs_unregister(struct device *dev);
static void mipi_tmd_lcd_pre_on(struct msm_fb_data_type *mfd);
static void mipi_tmd_lcd_pre_off(struct msm_fb_data_type *mfd);
static void mipi_tmd_pfm_on(struct msm_fb_data_type *mfd);
static int mipi_tmd_pfm_off(struct msm_fb_data_type *mfd);

static char exit_sleep[2]    = {0x11, 0x00};
static char display_on[2]    = {0x29, 0x00};
static char display_off[2]   = {0x28, 0x00};
static char enter_sleep[2]   = {0x10, 0x00};
static char deep_standby1[2]  = {0xb0, 0x02};
static char deep_standby2[2]  = {0xb1, 0x01};
static char NOP_cmd[2]  = {0x00, 0x00};


#ifdef MIPI_TMD_FUMC_REFRESH_RESET
static char power_mode[2]    = {0x0A, 0x00};
#endif

#ifdef MIPI_TMD_FUNC_BLC
static char RDPWM[2] = {0xba, 0x00};
#endif
static char CABC_PFM[13]       = {
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x64, 0x68
};
#define MAX_BLC_MODE 4
static char CABC_PFM_BLC_MODE[MAX_BLC_MODE][13]       = {
	{
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x64, 0x68
	},
	{
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x64, 0x68
	},
	{
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x64, 0x68
	},
	{
	0xbe,
	0x1e, 0x00, 0x20, 0x20,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x64, 0x68
	}
};

static char Bbcklight_CTL_B7[15]       = {
	0xb7,
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10
};

static char Bbcklight_CTL_B7_MODE[MAX_BLC_MODE][15]       = {
	{
	0xb7,
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10
	},
	{
	0xb7,
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10
	},
	{
	0xb7,
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10
	},
	{
	0xb7,
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10
	}
};

static char Bbcklight_CTL_B8[14]       = {
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
};

static char Bbcklight_CTL_B8_MODE[MAX_BLC_MODE][14]       = {
	{
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	},
	{
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	},
	{
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	},
	{
	0xb8,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	}
};

static char Bbcklight_CTL_B7B8_MODE[MAX_BLC_MODE][27]       = {
	{
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	},
	{
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	},
	{
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	},
	{
	0x18, 0x00, 0x18, 0x18,
	0x0c, 0x10, 0x5c, 0x10,
	0xac, 0x10, 0x0c, 0x10,
	0x00, 0x10,
	0xf8, 0xda, 0x6d, 0xfb,
	0xff, 0xff, 0xcf, 0x1f,
	0x91, 0xae, 0xca, 0xe5,
	0xff
	}
};

#define MAX_PFM_LEVEL 3
static char CABC_PFM_MODE[MAX_PFM_LEVEL][13]       = {
	{
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x97, 0x38
	},
	{
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x68, 0x68
	},
	{
	0xbe,
	0x1e, 0x00, 0x02, 0x06,
	0x04, 0x40, 0x00, 0x5d,
	0x00, 0x00, 0x38, 0x96
	}
};
static char PrivacyFilterPattern[33] = {
	0xbd,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
	0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static char PrivacyFilterPatternMode[2][33] = {
	{
	0xbd, /* Clear Pattern Mode */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{
	0xbd,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
	0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};
static char BLC_PFM_mode[2] = {0xbb, 0x0b};
static char BLC_on[2] = {0xbb, 0x0b};
static char PFM_on[2] = {0xbb, 0x0d};
static char PFM_off[2] = {0xbb, 0x09};
static u8 blc_select[5] = {0x1,0x2,0x2,0x3,0x4};

struct dsi_cmd_desc tmd_hd_display_on_cmds[] = {
    {DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(exit_sleep), exit_sleep},
    {DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on}
};

static struct dsi_cmd_desc tmd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 140, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc tmd_deep_standby_cmds1 = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(deep_standby1), deep_standby1
};

static struct dsi_cmd_desc tmd_deep_standby_cmds2 = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 40, sizeof(deep_standby2), deep_standby2
};

static struct dsi_cmd_desc tmd_NOP_cmd = {
	DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(NOP_cmd), NOP_cmd
};

#ifdef MIPI_TMD_FUMC_REFRESH_RESET
static struct dsi_cmd_desc tmd_refresh_cmds = {
	DTYPE_DCS_READ, 1, 0, 1, 0, sizeof(power_mode), power_mode
};
#endif

#ifdef MIPI_TMD_FUNC_BLC
static struct dsi_cmd_desc tmd_blc_cmds = {
	DTYPE_GEN_READ1, 1, 0, 1, 0, sizeof(RDPWM), RDPWM
};
#endif
static struct dsi_cmd_desc tmd_cacb_cmds = {
	DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_PFM), CABC_PFM
};
static struct dsi_cmd_desc tmd_backlight_ctl_b7_cmds = {
	DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Bbcklight_CTL_B7), Bbcklight_CTL_B7
};
static struct dsi_cmd_desc tmd_backlight_ctl_b8_cmds = {
	DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Bbcklight_CTL_B8), Bbcklight_CTL_B8
};
static struct dsi_cmd_desc tmd_pfm_pattern_cmds = {
	DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PrivacyFilterPattern), &PrivacyFilterPatternMode[1][0]
};
static struct dsi_cmd_desc tmd_pfm_pattern_clear_cmds = {
	DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PrivacyFilterPattern), &PrivacyFilterPatternMode[0][0]
};
static struct dsi_cmd_desc tmd_pfm_on_cmds = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(PFM_on), PFM_on
};
static struct dsi_cmd_desc tmd_pfm_off_cmds = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(PFM_off), PFM_off
};
static struct dsi_cmd_desc tmd_blc_on_cmds = {
	DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(BLC_on), BLC_on
};

#define LCDBL_TABLE_SIZE	(15)
static u8 led_lcdbl_normal_table[LCDBL_TABLE_SIZE] = {
	0x3F, 0x3F, 0x34, 0x2D, 0x25, 0x1F, 0x1A, 0x15, 0x10, 0x0B, 0x07, 0x07, 0x05, 0xE8, 0x03 };
static u8 led_lcdbl_ecomode_table[LCDBL_TABLE_SIZE] = {
	0x41, 0x41, 0x36, 0x2E, 0x26, 0x21, 0x1C, 0x17, 0x12, 0x0C, 0x0A, 0x0A, 0x03, 0xE8, 0x03 };
static u8 led_lcdbl_highbright_table[LCDBL_TABLE_SIZE] = {
	0x3F, 0x3F, 0x39, 0x34, 0x31, 0x2E, 0x2B, 0x28, 0x1E, 0x14, 0x07, 0x07, 0x05, 0xE8, 0x03 };

static unsigned short mipi_tmd_pwm_period[5] = {0x3e8,0x3e8,0x3e8,0x3e8,0x3e8};

static u8 mipi_tmd_eeprom_flg   = FALSE;

struct t_mipi_tmd_cmd_eeprom_tbl const init_cmd_eeprom[] = {
	/* initial cmd */
//	{D_HCM_E_LCD_DRV_INIT_B2, ARRAY_SIZE(ACR) - 1, &ACR[1]},
//	{D_HCM_E_LCD_DRV_INIT_B3, ARRAY_SIZE(CLK_IF) - 1, &CLK_IF[1]},
//	{D_HCM_E_LCD_DRV_INIT_B4, ARRAY_SIZE(PXL_format) - 1, &PXL_format[1]},
//	{D_HCM_E_LCD_DRV_INIT_B6, ARRAY_SIZE(DSI_ctrl) - 1, &DSI_ctrl[1]},
//	{D_HCM_E_LCD_DRV_INIT_C0, ARRAY_SIZE(panel_driving) - 1, &panel_driving[1]},
//	{D_HCM_E_LCD_DRV_INIT_C1, ARRAY_SIZE(dispH_timing) - 1, &dispH_timing[1]},
//	{D_HCM_E_LCD_DRV_INIT_C2, ARRAY_SIZE(source_output_C2) - 1, &source_output_C2[1]},
//	{D_HCM_E_LCD_DRV_INIT_C3, ARRAY_SIZE(gate_ic_ctrl) - 1, &gate_ic_ctrl[1]},
//	{D_HCM_E_LCD_DRV_INIT_C4, ARRAY_SIZE(LTPS_IF_ctrl_C4) - 1, &LTPS_IF_ctrl_C4[1]},
//	{D_HCM_E_LCD_DRV_INIT_C6, ARRAY_SIZE(source_output_C6) - 1, &source_output_C6[1]},
//	{D_HCM_E_LCD_DRV_INIT_C7, ARRAY_SIZE(LTPS_IF_ctrl_C7) - 1, &LTPS_IF_ctrl_C7[1]},
//	{D_HCM_E_LCD_DRV_INIT_C8, ARRAY_SIZE(gamma_ctrl) - 1, &gamma_ctrl[1]},
//	{D_HCM_E_LCD_DRV_INIT_C9, ARRAY_SIZE(gamma_setA_positive) - 1, &gamma_setA_positive[1]},
//	{D_HCM_E_LCD_DRV_INIT_CA, ARRAY_SIZE(gamma_setA_negative) - 1, &gamma_setA_negative[1]},
//	{D_HCM_E_LCD_DRV_INIT_CB, ARRAY_SIZE(gamma_setB_positive) - 1, &gamma_setB_positive[1]},
//	{D_HCM_E_LCD_DRV_INIT_CC, ARRAY_SIZE(gamma_setB_negative) - 1, &gamma_setB_negative[1]},
//	{D_HCM_E_LCD_DRV_INIT_CD, ARRAY_SIZE(gamma_setC_positive) - 1, &gamma_setC_positive[1]},
//	{D_HCM_E_LCD_DRV_INIT_CE, ARRAY_SIZE(gamma_setC_negative) - 1, &gamma_setC_negative[1]},
//	{D_HCM_E_LCD_DRV_INIT_D0, ARRAY_SIZE(powerSet_1) - 1, &powerSet_1[1]},
//	{D_HCM_E_LCD_DRV_INIT_D1, ARRAY_SIZE(powerSet_2) - 1, &powerSet_2[1]},
//	{D_HCM_E_LCD_DRV_INIT_D3, ARRAY_SIZE(powerSet_Int) - 1, &powerSet_Int[1]},
//	{D_HCM_E_LCD_DRV_INIT_D5, ARRAY_SIZE(VPLVL_VNLVL_set) - 1, &VPLVL_VNLVL_set[1]},
//	{D_HCM_E_LCD_DRV_INIT_D8, ARRAY_SIZE(VCOMDC_set_D8) - 1, &VCOMDC_set_D8[1]},
//	{D_HCM_E_LCD_DRV_INIT_DE, ARRAY_SIZE(VCOMDC_set_DE) - 1, &VCOMDC_set_DE[1]},
//	{D_HCM_E_LCD_DRV_INIT_FD, ARRAY_SIZE(Panel_drive_set) - 1, &Panel_drive_set[1]},
//	{D_HCM_E_LCD_DRV_INIT_B9, ARRAY_SIZE(PFM) - 1, &PFM[1]},
//#ifdef MIPI_TMD_FUMC_REFRESH_REWRITE
	/* LCD Reflesh */
//	{D_HCM_LCD_REFLESH_ON , 1, &mipi_tmd_reg_rewrite},
//#endif
//#ifdef MIPI_TMD_FUMC_REFRESH
//	{D_HCM_LCD_REFLESH_PERIOD , 1, &mipi_tmd_refresh_cycle},
//#endif
//#ifdef MIPI_TMD_FUMC_REFRESH_RESET
//	{D_HCM_LCD_RETRANS_ON , 1, &mipi_tmd_hw_reset},
//#endif
	/* BLC */
//	{D_HCM_E_LCD_DRV_BLC_BE_MODE1 , 6, &CABC_PFM_BLC_MODE[0][3]},
//	{D_HCM_E_LCD_DRV_BLC_BE_MODE2 , 6, &CABC_PFM_BLC_MODE[1][3]},
//	{D_HCM_E_LCD_DRV_BLC_BE_MODE3 , 6, &CABC_PFM_BLC_MODE[2][3]},
//	{D_HCM_E_LCD_DRV_BLC_BE_MODE4 , 6, &CABC_PFM_BLC_MODE[3][3]},
//	{D_HCM_E_LCD_DRV_BLC_B7B8_MODE1 , 27, &Bbcklight_CTL_B7B8_MODE[0][0]},
//	{D_HCM_E_LCD_DRV_BLC_B7B8_MODE2 , 27, &Bbcklight_CTL_B7B8_MODE[1][0]},
//	{D_HCM_E_LCD_DRV_BLC_B7B8_MODE3 , 27, &Bbcklight_CTL_B7B8_MODE[2][0]},
//	{D_HCM_E_LCD_DRV_BLC_B7B8_MODE4 , 27, &Bbcklight_CTL_B7B8_MODE[3][0]},
//	{D_HCM_E_LCD_DRV_BLC_BE_SELECT , ARRAY_SIZE(blc_select), &blc_select[0]},
//	{D_HCM_E_LCD_DRV_INIT_BB, 1, &BLC_PFM_mode[1]},
//	{D_HCM_E_LCD_DRV_INIT_BB, 1, &BLC_on[1]},
	/* PFM */
//	{D_HCM_E_LCD_DRV_PFM_BE_MODE1 , 4, &CABC_PFM_MODE[0][9]},
//	{D_HCM_E_LCD_DRV_PFM_BE_MODE2 , 4, &CABC_PFM_MODE[1][9]},
//	{D_HCM_E_LCD_DRV_PFM_BE_MODE3 , 4, &CABC_PFM_MODE[2][9]},
//	{D_HCM_E_LCD_DRV_PFM_BD , ARRAY_SIZE(PrivacyFilterPattern) - 1, &PrivacyFilterPatternMode[1][1]},
	
	/* period(LED) */
	{D_HCM_LPWMON , ARRAY_SIZE(led_lcdbl_normal_table), led_lcdbl_normal_table},
	{D_HCM_LPWMON_ECO , ARRAY_SIZE(led_lcdbl_ecomode_table), led_lcdbl_ecomode_table},
	{D_HCM_LPWMON_HIGH , ARRAY_SIZE(led_lcdbl_highbright_table), led_lcdbl_highbright_table}
};

void mipi_tmd_lcd_pre_on(struct msm_fb_data_type *mfd)
{
	LCD_DEBUG_IN_LOG
	
		
	LCD_DEBUG_OUT_LOG
	
	return;
}

void mipi_tmd_lcd_pre_off(struct msm_fb_data_type *mfd)
{
	LCD_DEBUG_IN_LOG

	mipi_tmd_wkq_timer_stop(MIPI_TMD_OFF, MIPI_TMD_TIMER_ALL);
	
	cancel_work_sync(&tmd_workqueue_data_p->backlight_workq_initseq);

	mfd->power_on_drow_flg = FALSE;
		
	LCD_DEBUG_OUT_LOG
	
	return;
}

static void mipi_tmd_wkq_timer_stop(u8 change_status, u8 stop_timer)
{	
	LCD_DEBUG_IN_LOG
	
	mutex_lock(&mipi_tmd_pow_lock);
	
	if( mipi_tmd_wkq_status == MIPI_TMD_OFF )
	{
		mutex_unlock(&mipi_tmd_pow_lock);
		LCD_DEBUG_OUT_LOG
		return;
	}
	
	mipi_tmd_wkq_status = MIPI_TMD_OFF;

	mutex_unlock(&mipi_tmd_pow_lock);
#ifdef MIPI_TMD_FUNC_BLC
	if( stop_timer & MIPI_TMD_TIMER_BLC )
	{
		//BLC function is suspended. 
		del_timer(&mipi_tmd_blc_timer);
	
		cancel_work_sync(&tmd_workqueue_data_p->blc_workq_initseq);
	}
#endif

#ifdef MIPI_TMD_FUMC_REFRESH
	if( stop_timer & MIPI_TMD_TIMER_REFRESH )
	{
		//Refresh function is suspended. 
		del_timer(&mipi_tmd_refresh_timer);
	
		cancel_work_sync(&tmd_workqueue_data_p->refresh_workq_initseq);
	}
#endif

	mipi_tmd_wkq_status = change_status;
	
	LCD_DEBUG_OUT_LOG
}


static void mipi_tmd_eeprom_read(void)
{
    int err = 0;
    int cnt;


	LCD_DEBUG_IN_LOG

	if(mipi_tmd_eeprom_flg)
	{
		/* do nothing */
		LCD_DEBUG_OUT_LOG
		return;
	}

//    /* initial cmd */
    for(cnt = 0;
        cnt < (sizeof(init_cmd_eeprom)/sizeof(init_cmd_eeprom[0]));
        cnt++)
    {
		err = cfgdrv_read(init_cmd_eeprom[cnt].type,
						  init_cmd_eeprom[cnt].data_size,
						  init_cmd_eeprom[cnt].data);

		if (err) {
			pr_err("lcd can't read D_HCM_E, error=%d,cnt=%d\n", err, cnt);
			LCD_DEBUG_OUT_LOG
			return;
		}
    }
    
    mipi_tmd_pwm_period[MIPI_TMD_NORMAL_MODE] = 
        (unsigned short)(led_lcdbl_normal_table[14] << 8 ) | led_lcdbl_normal_table[13];
    mipi_tmd_pwm_period[MIPI_TMD_ECO_MODE] = 
        (unsigned short)(led_lcdbl_ecomode_table[14] << 8 ) | led_lcdbl_ecomode_table[13];
    mipi_tmd_pwm_period[MIPI_TMD_CAMERA_MODE] = 
        (unsigned short)(led_lcdbl_highbright_table[14] << 8 ) | led_lcdbl_highbright_table[13];
        
	memcpy(&Bbcklight_CTL_B7_MODE[0][1],
	       &Bbcklight_CTL_B7B8_MODE[0][0],
	       14);
	memcpy(&Bbcklight_CTL_B7_MODE[1][1],
	       &Bbcklight_CTL_B7B8_MODE[1][0],
	       14);
	memcpy(&Bbcklight_CTL_B7_MODE[2][1],
	       &Bbcklight_CTL_B7B8_MODE[2][0],
	       14);
	memcpy(&Bbcklight_CTL_B7_MODE[3][1],
	       &Bbcklight_CTL_B7B8_MODE[3][0],
	       14);
	       
	memcpy(&Bbcklight_CTL_B8_MODE[0][1],
	       &Bbcklight_CTL_B7B8_MODE[0][14],
	       13);
	memcpy(&Bbcklight_CTL_B8_MODE[1][1],
	       &Bbcklight_CTL_B7B8_MODE[1][14],
	       13);
	memcpy(&Bbcklight_CTL_B8_MODE[2][1],
	       &Bbcklight_CTL_B7B8_MODE[2][14],
	       13);
	memcpy(&Bbcklight_CTL_B8_MODE[3][1],
	       &Bbcklight_CTL_B7B8_MODE[3][14],
	       13);
	
	mipi_tmd_eeprom_flg = TRUE;

	LCD_DEBUG_OUT_LOG

	return;
}

static void mipi_tmd_refresh_handler(unsigned long data)
{
	queue_work(tmd_workqueue_data_p->mipi_tmd_refresh_workqueue,
	           &tmd_workqueue_data_p->refresh_workq_initseq);
}

static void mipi_tmd_blc_handler(unsigned long data)
{
	unsigned long flag;
	
	spin_lock_irqsave(&mdp_spin_lock, flag);
	if(mipi_tmd_blc_cnt != 0)
	{
		mipi_tmd_blc_cnt--;
		spin_unlock_irqrestore(&mdp_spin_lock, flag);
		
		queue_work(tmd_workqueue_data_p->mipi_tmd_refresh_workqueue,
		           &tmd_workqueue_data_p->blc_workq_initseq);
	}
	else
	{
		spin_unlock_irqrestore(&mdp_spin_lock, flag);
	}
}

static void mipi_tmd_reg_rewrite_seq(struct msm_fb_data_type *mfd)
{
	LCD_DEBUG_IN_LOG
#ifdef MIPI_TMD_FUMC_REFRESH_REWRITE
	
	if( mipi_tmd_reg_rewrite == MIPI_TMD_ON )
	{
#ifndef MIPI_TMD_FUMC_REFRESH_RESET
		mipi_dsi_tmd_cmd_sync(mfd); //vsync wait
#endif
	}
	else
	{
		/* Disable reg rewrite */
		/* do nothing */
	}
#endif
	LCD_DEBUG_OUT_LOG
	
	return;
}

static void mipi_tmd_refresh_timer_set(void)
{
#ifdef MIPI_TMD_FUMC_REFRESH
	LCD_DEBUG_LOG_DETAILS(" refresh_timer_set \n");
	
	del_timer(&mipi_tmd_refresh_timer);

	if(mipi_tmd_wkq_status == MIPI_TMD_OFF)
		return;
		
	mipi_tmd_refresh_timer.expires =
		jiffies + (mipi_tmd_refresh_cycle * HZ);

	add_timer(&mipi_tmd_refresh_timer);
	
#endif

	return;
}

static void mipi_tmd_blc_timer_set( unsigned long blc_time)
{
#ifdef MIPI_TMD_FUNC_BLC
	unsigned long mipi_tmd_blc_cycle;
	del_timer(&mipi_tmd_blc_timer);
	
	if((mipi_tmd_wkq_status == MIPI_TMD_OFF) ||
	   (mipi_tmd_vp_info.status == MIPI_TMD_ON))
	{
		return;
	}

	mipi_tmd_blc_cycle = jiffies + msecs_to_jiffies( blc_time );

	mipi_tmd_blc_timer.expires =
		mipi_tmd_blc_cycle;

	add_timer(&mipi_tmd_blc_timer);
#endif

	return;
}

void mipi_tmd_vsync_on()
{
	if(mipi_tmd_blc_status == MIPI_TMD_OFF)
	{
		mipi_tmd_blc_cnt = 0;
		
		return;
	}
	
	if(mipi_tmd_blc_cnt == 0)
	{
		mipi_tmd_blc_timer_set( mipi_tmd_blc_cycle_timer );
	}
	
	mipi_tmd_blc_cnt = MIPI_TMD_BLC_CNT_INIT;
}

static void mipi_tmd_refresh_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *lmfd;
#ifdef MIPI_TMD_FUMC_REFRESH_RESET
	uint32 *data;
	unsigned char rd_data;
#endif


	LCD_DEBUG_IN_LOG
	
	lmfd = tmd_workqueue_data_p->workq_mfd;
	
	if( lmfd->panel_power_on == FALSE )
	{
		LCD_DEBUG_OUT_LOG
		return;
	}

	mipi_tmd_wkq_timer_stop(MIPI_TMD_ON, MIPI_TMD_TIMER_BLC);

#ifdef MIPI_TMD_FUMC_REFRESH_RESET
	if( mipi_tmd_hw_reset == MIPI_TMD_ON )
	{
		/* Enable force reset */

		mipi_dsi_tmd_cmd_sync(lmfd); //vsync wait
		
		mipi_dsi_cmds_rx(lmfd, &tmd_tx_buf, &tmd_rx_buf, &tmd_refresh_cmds, 1);

		data = (uint32 *)tmd_rx_buf.data;

		rd_data = (unsigned char)*data;

		if(!((rd_data & MIPI_TMD_POWER_MODE_NORMAL) == 
						MIPI_TMD_POWER_MODE_NORMAL))
		{
			pr_info("LCD HW refresh reset! : power_mode = 0x%X\n",rd_data);

			lmfd->hw_force_reset = TRUE;
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

	mipi_tmd_reg_rewrite_seq(lmfd);

	msleep(20);

#ifdef MIPI_TMD_FUNC_BLC
	//BLC function is resumed. 
	mipi_tmd_blc_timer_set( mipi_tmd_blc_cycle_timer );
#endif

#ifdef MIPI_TMD_FUMC_REFRESH
	mipi_tmd_refresh_timer_set();
#endif
	LCD_DEBUG_OUT_LOG
}

static u8 mipi_tmd_read_rdpwm(struct msm_fb_data_type *mfd, int brightness_level)
{
#ifdef MIPI_TMD_FUNC_BLC
	uint32 *data;
#endif
	unsigned char rd_blc_level;


	LCD_DEBUG_IN_LOG
	
	rd_blc_level = 0;

	if(brightness_level == 0 )
	{
		return rd_blc_level;
	}

#ifdef MIPI_TMD_FUNC_BLC
	CABC_PFM[1] = (u8)brightness_level;
	
	mipi_dsi_cmds_tx(&tmd_tx_buf, &tmd_cacb_cmds,1);

	if(mipi_tmd_bl_tmp)
	{
		mipi_tmd_set_backlight_sub(mipi_tmd_bl_tmp);
	}

	mipi_dsi_cmds_rx(mfd, &tmd_tx_buf, &tmd_rx_buf, &tmd_blc_cmds, 1);

	data = (uint32 *)tmd_rx_buf.data;
	
	rd_blc_level = (unsigned char)*data;
	
	LCD_DEBUG_LOG_DETAILS("rd_blc_level = %d\n",rd_blc_level);
#endif

	LCD_DEBUG_OUT_LOG
	
	/* resolution 0`255 */
	return rd_blc_level;
}

static void mipi_tmd_blc_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *lmfd;
	u8 blc_level;
	
	
	lmfd = tmd_workqueue_data_p->workq_mfd;
	
	if( lmfd->panel_power_on == FALSE )
	{
		return;
	}
	
	mipi_dsi_tmd_cmd_sync(lmfd); //vsync wait
	
	blc_level = mipi_tmd_read_rdpwm(lmfd, tmd_workqueue_data_p->bl_brightness);

	if( blc_level != tmd_workqueue_data_p->bl_blc_brightness )
	{
		mipi_tmd_set_backlight_sub(blc_level);
	}
	
	tmd_workqueue_data_p->bl_blc_brightness = blc_level;

	msleep(20);

	mipi_tmd_blc_timer_set(mipi_tmd_blc_cycle_timer);
}

static void mipi_tmd_backlight_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *lmfd;
	u8 bl_level;
#ifdef MIPI_TMD_FUNC_BLC
	u8 bl_blc_level;
	unsigned char loop_cnt;
	int bl_tmp;
#endif

	mipi_tmd_bl_tmp = MIPI_TMD_OFF;

	lmfd = tmd_workqueue_data_p->workq_mfd;
	
	bl_level = tmd_workqueue_data_p->bl_brightness;
	
	if((lmfd->panel_power_on == FALSE) ||
	   ((tmd_workqueue_data_p->bl_brightness_old == bl_level) &&
	   (tmd_workqueue_data_p->bl_mode_status ==
	        tmd_workqueue_data_p->bl_mode_status_old)))
	{
		LCD_DEBUG_OUT_LOG

		return;
	}

	mipi_tmd_wkq_timer_stop(MIPI_TMD_ON, MIPI_TMD_TIMER_ALL);
	
#ifdef MIPI_TMD_FUNC_BLC
	if(mipi_tmd_vp_info.status == MIPI_TMD_OFF)
	{
		if(tmd_workqueue_data_p->bl_mode_status != 
			tmd_workqueue_data_p->bl_mode_status_old)
		{
			tmd_workqueue_data_p->bl_mode_status_old =
				tmd_workqueue_data_p->bl_mode_status;

			//BLC function is re-set up. 
			mipi_tmd_blc_on(lmfd);
		}
		else
		{			
			mipi_dsi_tmd_cmd_sync(lmfd); //vsync wait
			
			for(loop_cnt = 0 ; loop_cnt < MIPI_TMD_MAX_LOOP ;loop_cnt++)
			{
				bl_blc_level = mipi_tmd_read_rdpwm(lmfd, bl_level);

				mipi_tmd_set_backlight_sub(bl_blc_level);
				
				mipi_tmd_bl_tmp = MIPI_TMD_OFF;
			
				if( bl_level == tmd_workqueue_data_p->bl_brightness)
				{
					break;
				}
				else
				{
					if(tmd_workqueue_data_p->bl_brightness > bl_level)
					{
						bl_tmp = (tmd_workqueue_data_p->bl_brightness - bl_level) / 2;
						mipi_tmd_bl_tmp = bl_blc_level + bl_tmp;
						
						if(mipi_tmd_bl_tmp > 255)
							mipi_tmd_bl_tmp = MIPI_TMD_OFF;
					}
					else
					{
						bl_tmp = (bl_level - tmd_workqueue_data_p->bl_brightness) / 2;
						mipi_tmd_bl_tmp = bl_blc_level - bl_tmp;
						
						if(mipi_tmd_bl_tmp <= 0)
							mipi_tmd_bl_tmp = MIPI_TMD_OFF;
					}
					
					bl_level = tmd_workqueue_data_p->bl_brightness;
				}
			}


			mipi_tmd_set_backlight_sub(bl_blc_level);

			msleep(20);
		}
	}
	else
	{
		mipi_tmd_set_backlight_sub(bl_level);
	}
#else
	tmd_workqueue_data_p->bl_mode_status_old = 
		tmd_workqueue_data_p->bl_mode_status;

	mipi_tmd_set_backlight_sub(bl_level);
#endif

#if 1 /*RUPY_add*/
	if( (0 < bl_level) &&(tmd_workqueue_data_p->bl_brightness_old == 0))
	{
		leddrv_lcdbl_led_on();
	}
#endif /*RUPY_add*/

	tmd_workqueue_data_p->bl_blc_brightness = 0;
	tmd_workqueue_data_p->bl_brightness_old = bl_level;
	
#ifdef MIPI_TMD_FUMC_REFRESH
	//Refresh function is resumed. 
	mipi_tmd_refresh_timer_set();
#endif
#ifdef MIPI_TMD_FUNC_BLC
	//BLC function is resumed. 
	mipi_tmd_blc_timer_set(mipi_tmd_blc_cycle_timer_delay);
#endif
}

static int mipi_tmd_ledset_backlight(int brightness, enum leddrv_lcdbl_mode_status mode_status)
{
	int err = 0;

	LCD_DEBUG_IN_LOG

	LCD_DEBUG_LOG_DETAILS("brightness = %d mode_status = %d\n",brightness,mode_status);

	mutex_lock(&mipi_tmd_vblind_lock);
	
	err = mipi_tmd_set_display_mode(mipi_tmd_vp_info.display_mode, mode_status);
	
	mutex_unlock(&mipi_tmd_vblind_lock);
	
	if (err) {
		LCD_DEBUG_OUT_LOG
		return err;
	}
	
	tmd_workqueue_data_p->bl_brightness = brightness;

	mutex_lock(&mipi_tmd_pow_lock);
	
	if(mipi_tmd_wkq_status == MIPI_TMD_OFF)
	{
		mutex_unlock(&mipi_tmd_pow_lock);
		return err;
	}

	mutex_unlock(&mipi_tmd_pow_lock);
	
	queue_work(tmd_workqueue_data_p->mipi_tmd_refresh_workqueue,
	           &tmd_workqueue_data_p->backlight_workq_initseq);

	LCD_DEBUG_OUT_LOG
	
	return err;
}

static int mipi_tmd_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	uint32 dsi_ctrl;
	int video_mode;
	unsigned long flag;


	LCD_DEBUG_IN_LOG

        pr_info("### %s, %d ###\n", __func__, __LINE__);

	msleep(40);

	mfd = platform_get_drvdata(pdev);
	
	if (!mfd)
		return -ENODEV;

        gmfd = mfd;

	mipi_tmd_eeprom_read();
	
	if(mfd->hw_force_reset)
	{
	/***   Rupy Sysdes sequence1 CALL   ***/
		mipi_tmd_lcd_power_on_init();
		
		mfd->hw_force_reset = FALSE;
	}

	/***   Rupy Sysdes sequence6 CALL   ***/
	mipi_tmd_lcd_power_standby_on_init();

/***   Rupy Sysdes sequence2 START   ***/

	mipi_set_tx_power_mode(1); /* LP11 mode */

	/***   Rupy Sysdes sequence2 - "Send DSI command" insert here  ***/

	mipi_set_tx_power_mode(0); /* HS mode   */

	mdp4_dsi_video_blackscreen_overlay(mfd);

	tmd_workqueue_data_p->workq_mfd = mfd;
	
	tmd_workqueue_data_p->bl_mode_status_old = LEDDRV_NORMAL_MODE;
	tmd_workqueue_data_p->bl_mode_status = LEDDRV_NORMAL_MODE;

	mutex_lock(&mipi_tmd_pow_lock);
	
	mipi_tmd_wkq_status = MIPI_TMD_ON;
	
	mutex_unlock(&mipi_tmd_pow_lock);
	
	mutex_lock(&mipi_tmd_vblind_lock);
	if( mipi_tmd_vp_info.status == MIPI_TMD_ON )
	{
		mipi_tmd_pfm_on(mfd);
	}
	else
	{
		mipi_tmd_blc_on(mfd);
	
		spin_lock_irqsave(&mdp_spin_lock, flag);
		mipi_tmd_blc_cnt = MIPI_TMD_BLC_CNT_INIT;
		mipi_tmd_blc_status = MIPI_TMD_ON;
		spin_unlock_irqrestore(&mdp_spin_lock, flag);
	}
	mutex_unlock(&mipi_tmd_vblind_lock);
	
	mipi_tmd_blc_timer_set(mipi_tmd_blc_cycle_timer);
	
	mipi_tmd_refresh_timer_set();

	lcdon_cmd_flg = FALSE;

        dsi_ctrl = MIPI_INP(MDP_BASE + 0xE0000 + 0x0000);
        video_mode = dsi_ctrl & 0x01; /* VIDEO_MODE_EN */
        pr_info("### %s, %d: MDP_DSI_VIDEO_EN = 0x%X, DSI_VIDEO_EN = 0x%X ###\n", __func__, __LINE__, dsi_ctrl, video_mode);


	LCD_DEBUG_OUT_LOG
	
	return 0;
}

void mipi_tmd_lcd_on_cmd(void)
{
	uint32 dsi_ctrl;
	int video_mode;

	LCD_DEBUG_IN_LOG

        dsi_ctrl = MIPI_INP(MDP_BASE + DSI_VIDEO_BASE + 0x0000);
        video_mode = dsi_ctrl & 0x01; /* VIDEO_MODE_EN */
        pr_info("### %s, %d: MDP_DSI_VIDEO_EN = 0x%X, DSI_VIDEO_EN = 0x%X ###\n", __func__, __LINE__, dsi_ctrl, video_mode);

/***   Rupy Sysdes sequence2 RESTART   ***/

	mipi_dsi_cmds_tx(&tmd_tx_buf,
		tmd_hd_display_on_cmds, ARRAY_SIZE(tmd_hd_display_on_cmds));

	LCD_DEBUG_LOG("\n### LED ON ###\n\n");
	leddrv_lcdbl_led_on();

/***   Rupy Sysdes sequence2 END   ***/

	LCD_DEBUG_OUT_LOG
	
	return;
}

static int mipi_tmd_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;


	LCD_DEBUG_IN_LOG

        pr_info("### %s, %d ###\n", __func__, __LINE__);
	
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

        gmfd = mfd;

/***   Rupy Sysdes sequence5 START   ***/
	mipi_dsi_cmds_tx(&tmd_tx_buf, &tmd_deep_standby_cmds1,1);
	mipi_dsi_cmds_tx(&tmd_tx_buf, &tmd_NOP_cmd,1);
	mipi_dsi_cmds_tx(&tmd_tx_buf, &tmd_NOP_cmd,1);
	mipi_dsi_cmds_tx(&tmd_tx_buf, &tmd_deep_standby_cmds2,1);

	gpio_set_value_cansleep(gpio_lcd_dcdc, 0);

	msleep(20);
/***   Rupy Sysdes sequence5 END   ***/

	LCD_DEBUG_OUT_LOG

	return 0;
}

void mipi_tmd_lcd_off_cmd(void)
{
	unsigned long flag;

	LCD_DEBUG_IN_LOG

    pr_info("### %s, %d ###\n", __func__, __LINE__);


/***   Rupy Sysdes sequence3 START   ***/
	mipi_tmd_set_backlight_sub(0);

	spin_lock_irqsave(&mdp_spin_lock, flag);
	mipi_tmd_blc_status = MIPI_TMD_OFF;
	spin_unlock_irqrestore(&mdp_spin_lock, flag);

	mipi_dsi_cmds_tx(&tmd_tx_buf,
		tmd_display_off_cmds, ARRAY_SIZE(tmd_display_off_cmds));

/***   Rupy Sysdes sequence3 END   ***/

	LCD_DEBUG_OUT_LOG

	return;
}

static void mipi_tmd_pfm_on(struct msm_fb_data_type *mfd)
{
	struct dsi_cmd_desc tmd_pfm_cmds;
	unsigned long flag;


	LCD_DEBUG_IN_LOG
	

	mipi_tmd_vp_info.status = MIPI_TMD_ON;
	
	mutex_lock(&mipi_tmd_pow_lock);

	if(mipi_tmd_wkq_status == MIPI_TMD_OFF)
	{
		mutex_unlock(&mipi_tmd_pow_lock);
		LCD_DEBUG_OUT_LOG
		return;
	}

	mutex_unlock(&mipi_tmd_pow_lock);
	
	spin_lock_irqsave(&mdp_spin_lock, flag);
	mipi_tmd_blc_status = MIPI_TMD_OFF;
	spin_unlock_irqrestore(&mdp_spin_lock, flag);
	
	mipi_tmd_wkq_timer_stop(MIPI_TMD_ON, MIPI_TMD_TIMER_ALL);

	cancel_work_sync(&tmd_workqueue_data_p->backlight_workq_initseq);

	mipi_tmd_set_backlight_sub(tmd_workqueue_data_p->bl_brightness);
	
	tmd_pfm_cmds = tmd_cacb_cmds;
	
	tmd_pfm_cmds.payload = &CABC_PFM_MODE[mipi_tmd_vp_info.strength][0];
	
	mipi_dsi_tmd_cmd_sync(mfd); //vsync wait
	
	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_pfm_cmds,
		1);

	memcpy(CABC_PFM,
	       &CABC_PFM_MODE[mipi_tmd_vp_info.strength][0],
	       sizeof(CABC_PFM));
	
	if(mipi_tmd_vp_info.convolute)
	{
		mipi_dsi_cmds_tx(&tmd_tx_buf,
			&tmd_pfm_pattern_cmds,
			1);
		
		memcpy(PrivacyFilterPattern,
		       &PrivacyFilterPatternMode[1][0],
		       sizeof(PrivacyFilterPattern));
	}
	else
	{
		mipi_dsi_cmds_tx(&tmd_tx_buf,
			&tmd_pfm_pattern_clear_cmds,
			1);
		
		memcpy(PrivacyFilterPattern,
		       &PrivacyFilterPatternMode[0][0],
		       sizeof(PrivacyFilterPattern));
	}

	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_pfm_on_cmds,
		1);

	memcpy(BLC_PFM_mode,&PFM_on,sizeof(BLC_PFM_mode));

#ifdef MIPI_TMD_FUMC_REFRESH
	//Refresh function is resumed. 
	mipi_tmd_refresh_timer_set();
#endif
	
	LCD_DEBUG_OUT_LOG
	
	return;
}


static int mipi_tmd_pfm_off(struct msm_fb_data_type *mfd)
{
	unsigned long flag;


	LCD_DEBUG_IN_LOG

	mutex_lock(&mipi_tmd_pow_lock);

	if(mipi_tmd_wkq_status == MIPI_TMD_OFF)
	{
		mipi_tmd_vp_info.status = MIPI_TMD_OFF;
		
		mutex_unlock(&mipi_tmd_pow_lock);
		LCD_DEBUG_OUT_LOG
		return 0;
	}
	
	mipi_dsi_tmd_cmd_sync(mfd); //vsync wait
	
	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_pfm_off_cmds,
		1);
		
	memcpy(BLC_PFM_mode,&PFM_off,sizeof(PFM_off));
	
	mipi_tmd_blc_on(mfd);
	
	mipi_tmd_vp_info.status = MIPI_TMD_OFF;
	
	mipi_tmd_blc_timer_set(mipi_tmd_blc_cycle_timer);

	mutex_unlock(&mipi_tmd_pow_lock);
	
	spin_lock_irqsave(&mdp_spin_lock, flag);
	mipi_tmd_blc_cnt = MIPI_TMD_BLC_CNT_INIT;
	mipi_tmd_blc_status = MIPI_TMD_ON;
	spin_unlock_irqrestore(&mdp_spin_lock, flag);
	
	LCD_DEBUG_OUT_LOG
	
	return 0;
}

static void mipi_tmd_blc_on(struct msm_fb_data_type *mfd)
{
	struct dsi_cmd_desc tmd_blc_cmds;
	int bl_level;
	u8 mode;
	u8 bl_mode;

	LCD_DEBUG_IN_LOG
	
	LCD_DEBUG_LOG("bl_mode_status = %d: brightness = %d\n",
	     tmd_workqueue_data_p->bl_mode_status, tmd_workqueue_data_p->bl_brightness);

	tmd_workqueue_data_p->bl_blc_brightness = 0;
	bl_level = tmd_workqueue_data_p->bl_brightness;
	bl_mode  = tmd_workqueue_data_p->bl_mode_status;

	mipi_tmd_set_backlight_sub(bl_level);

	mode = blc_select[bl_mode] - 1;

	memcpy(PrivacyFilterPattern,
	       &PrivacyFilterPatternMode[0][0],
	       sizeof(PrivacyFilterPattern));

	tmd_blc_cmds = tmd_cacb_cmds;
	
	CABC_PFM_BLC_MODE[mode][1] = bl_level;
	
	tmd_blc_cmds.payload = &CABC_PFM_BLC_MODE[mode][0];
	
	memcpy(CABC_PFM,
	       &CABC_PFM_BLC_MODE[mode][0],
	       sizeof(CABC_PFM));
	
	mipi_dsi_tmd_cmd_sync(mfd); //vsync wait
	
	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_blc_cmds,
		1);
	
	tmd_blc_cmds = tmd_backlight_ctl_b7_cmds;
	
	tmd_blc_cmds.payload = &Bbcklight_CTL_B7_MODE[mode][0];

	memcpy(Bbcklight_CTL_B7,
	       &Bbcklight_CTL_B7_MODE[mode][0],
	       sizeof(Bbcklight_CTL_B7));

	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_blc_cmds,
		1);
	
	tmd_blc_cmds = tmd_backlight_ctl_b8_cmds;
	
	tmd_blc_cmds.payload = &Bbcklight_CTL_B8_MODE[mode][0];
	
	memcpy(Bbcklight_CTL_B8,
	       &Bbcklight_CTL_B8_MODE[mode][0],
	       sizeof(Bbcklight_CTL_B8));
	       
	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_blc_cmds,
		1);

	tmd_blc_cmds = tmd_blc_on_cmds;

	memcpy(BLC_PFM_mode,&BLC_on,sizeof(BLC_PFM_mode));
	
	mipi_dsi_cmds_tx(&tmd_tx_buf,
		&tmd_blc_cmds,
		1);
	
	LCD_DEBUG_OUT_LOG
	
	return;
}

static int  mipi_tmd_set_display_mode(enum mipi_tmd_display_mode disp_mode, 
								enum leddrv_lcdbl_mode_status led_mode)
{
	int mode;
	
	LCD_DEBUG_IN_LOG
	
    switch (led_mode){
        case LEDDRV_NORMAL_MODE:
            mode = MIPI_TMD_NORMAL_MODE;
            break;
        case LEDDRV_ECO_MODE:
            mode = MIPI_TMD_ECO_MODE;
            break;
        case LEDDRV_HIGHBRIGHT_MODE:
            mode = MIPI_TMD_CAMERA_MODE;
            break;
        default:
            LCD_DEBUG_OUT_LOG
            return(-1);
    }
    
    tmd_workqueue_data_p->bl_mode_status = mode;
    mipi_tmd_vp_info.display_mode = disp_mode;
    
    LCD_DEBUG_LOG("mode = %d, disp_mode = %d, led_mode = %d\n",
    				mode,disp_mode,led_mode);
    
    LCD_DEBUG_OUT_LOG
    
    return 0;
}

static void mipi_tmd_set_backlight_sub(int bl_level)
{
	int ret;
	int pwm_level;
	int pwm_period;

	LCD_DEBUG_IN_LOG
	
	pwm_period = mipi_tmd_pwm_period[tmd_workqueue_data_p->bl_mode_status_old];

	pwm_level = ((((bl_level * MIPI_TMD_PWM_DUTY_CALC) * pwm_period) /
				MIPI_TMD_PWM_LEVEL) + 5) / MIPI_TMD_PWM_DUTY_CALC;

	LCD_DEBUG_LOG_DETAILS("bl_level = %d , pwm_period = %d , pwm_level = %d \n",
							bl_level, pwm_period, pwm_level);

	if (bl_lpm) {
		ret = pwm_config(bl_lpm, pwm_level, pwm_period);
		if (ret) {
			pr_err("pwm_config on lpm failed %d\n", ret);
			return;
		}

		if (bl_level) {
			ret = pwm_enable(bl_lpm);

			mipi_tmd_backlight_status = MIPI_TMD_ON;

			if (ret)
				pr_err("pwm enable/disable on lpm failed"
					"for bl %d\n",	bl_level);
		} else {
			if(mipi_tmd_backlight_status == MIPI_TMD_ON)
			{
				pwm_disable(bl_lpm);
			}
			mipi_tmd_backlight_status = MIPI_TMD_OFF;
		}
	}
	
	LCD_DEBUG_OUT_LOG
}

static int __devinit mipi_tmd_lcd_probe(struct platform_device *pdev)
{
	int rc;
	struct tmd_workqueue_data *td;


	LCD_DEBUG_IN_LOG
	
	if (pdev->id == 0) {
		mipi_tmd_pdata = pdev->dev.platform_data;
		LCD_DEBUG_OUT_LOG
		return 0;
	}
	if (mipi_tmd_pdata == NULL) {
		pr_err("%s.invalid platform data.\n", __func__);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	if (mipi_tmd_pdata != NULL)
		bl_lpm = pwm_request(mipi_tmd_pdata->gpio[0],
			"backlight");

	if (bl_lpm == NULL || IS_ERR(bl_lpm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_lpm = NULL;
	}
	
	pr_debug("bl_lpm = %p lpm = %d\n", bl_lpm,
		mipi_tmd_pdata->gpio[0]);

	td = kzalloc(sizeof(*td), GFP_KERNEL);
	if (!td) {
		LCD_DEBUG_OUT_LOG
		return -ENOMEM;
	}
    tmd_workqueue_data_p = td;
    
	msm_fb_add_device(pdev);
	
	mipi_tmd_bl_tmp = MIPI_TMD_OFF;
	mipi_tmd_backlight_status = MIPI_TMD_OFF;
	
	td->bl_brightness = 0;
	td->bl_brightness_old = 0;
	td->bl_blc_brightness = 0;
	td->bl_mode_status = LEDDRV_NORMAL_MODE;
	td->bl_mode_status_old = LEDDRV_NORMAL_MODE;
	
	
	//PFM parameter initialize
	mipi_tmd_vp_info.status = MIPI_TMD_OFF;
	mipi_tmd_vp_info.strength = 1;
	mipi_tmd_vp_info.convolute = MIPI_TMD_ON;
	mipi_tmd_vp_info.display_mode = DISP_STANDBY;

	INIT_WORK(&td->refresh_workq_initseq, mipi_tmd_refresh_work_func);
	INIT_WORK(&td->blc_workq_initseq, mipi_tmd_blc_work_func);
	INIT_WORK(&td->backlight_workq_initseq, mipi_tmd_backlight_work_func);
	
	td->mipi_tmd_refresh_workqueue = 
		create_singlethread_workqueue("mipi_tmd_refresh");
	if (td->mipi_tmd_refresh_workqueue == NULL){
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}
		
	init_timer(&mipi_tmd_refresh_timer);
	
	mipi_tmd_refresh_timer.function =
		mipi_tmd_refresh_handler;
	mipi_tmd_refresh_timer.data = 0;
	
	init_timer(&mipi_tmd_blc_timer);
	
	mipi_tmd_blc_timer.function =
		mipi_tmd_blc_handler;
	mipi_tmd_blc_timer.data = 0;
	
	gpio_lcd_dcdc   = mipi_tmd_pdata->gpio[1];
	gpio_disp_rst_n = mipi_tmd_pdata->gpio[3];
        pmic_gpio43 = PM8921_GPIO_PM_TO_SYS(43);
        pmic_gpio59 = PM8921_GPIO_PM_TO_SYS(59);
	
	rc = gpio_request(gpio_lcd_dcdc, "lcd_dcdc");
	if (rc) {
		pr_err("request gpio lcd_dcdc failed, rc=%d\n", rc);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	rc = gpio_request(pmic_gpio43, "full_hd_lcd_test");
	if (rc) {
		pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
		"GPIO43", 43, rc);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}

	rc = gpio_request(pmic_gpio59, "full_hd_lcd_test59");
	if (rc) {
		pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
		"GPIO59", 59, rc);
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
		return -ENODEV;
	}
		pr_err("-------- get 8921_l23 OK , rc = %ld\n", PTR_ERR(reg_l23));
	rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
	if (rc) {
		pr_err("set_voltage l23 failed, rc=%d\n", rc);
		return -EINVAL;
	}
		pr_err("-------- set_voltage l23 OK , rc=%d\n", rc);

	leddrv_lcdbl_set_handler(LEDDRV_EVENT_LCD_BACKLIGHT_CHANGED,
	                         mipi_tmd_ledset_backlight);


	msleep(200);

	mipi_tmd_lcd_power_on_init();
	
	LCD_DEBUG_OUT_LOG

	return 0;
}

static void mipi_tmd_lcd_power_on_init(void)
{
	int rc;

	LCD_DEBUG_IN_LOG

	pr_err("#############################################\n");
	pr_err("#############################################\n");
	pr_err("#############################################\n");
/***   Rupy Sysdes sequence1 START   ***/
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
	
	gpio_set_value_cansleep(gpio_disp_rst_n, 1);
	
	msleep(20);
/***   Rupy Sysdes sequence1 END   ***/
	LCD_DEBUG_OUT_LOG
	
	return;
}

static void mipi_tmd_lcd_power_standby_on_init(void)
{
	LCD_DEBUG_IN_LOG

	if(mipi_tmd_init_flg)
	{
        /***   Rupy Sysdes sequence6 START   ***/
		gpio_set_value_cansleep(gpio_lcd_dcdc, 1);

		msleep(20);

		gpio_set_value_cansleep(gpio_disp_rst_n, 0);
	
		msleep(20);
	
		gpio_set_value_cansleep(gpio_disp_rst_n, 1);
	
		msleep(20);
        /***   Rupy Sysdes sequence6 END   ***/
	}
	
	mipi_tmd_init_flg = TRUE;
	
	LCD_DEBUG_OUT_LOG
	
	return;
}
static int __devexit mipi_tmd_lcd_remove(struct platform_device *pdev)
{
	int rc;

	LCD_DEBUG_IN_LOG

/***   Rupy Sysdes sequence3(remove) START   ***/

/***   Rupy Sysdes sequence3(remove) END   ***/
/***   Rupy Sysdes sequence4 START   ***/

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
	
	destroy_workqueue(tmd_workqueue_data_p->mipi_tmd_refresh_workqueue);
	
	kfree(tmd_workqueue_data_p);
	
	gpio_free(gpio_lcd_dcdc);
	gpio_free(gpio_disp_rst_n);
	gpio_free(pmic_gpio43);
	gpio_free(pmic_gpio59);

	LCD_DEBUG_OUT_LOG
	
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_tmd_lcd_probe,
	.remove = __devexit_p(mipi_tmd_lcd_remove),
	.driver = {
		.name   = "mipi_tmd",
	},
};

static struct msm_fb_panel_data tmd_panel_data = {
	.on		= mipi_tmd_lcd_on,
	.off		= mipi_tmd_lcd_off,
	.sysfs_register = mipi_tmd_lcd_sysfs_register,
	.sysfs_unregister = mipi_tmd_lcd_sysfs_unregister,
	.pre_on		= mipi_tmd_lcd_pre_on,
	.pre_off	= mipi_tmd_lcd_pre_off,
};

static int ch_used[3];

int mipi_tmd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;


	LCD_DEBUG_IN_LOG

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_tmd_lcd_init();
	if (ret) {
		pr_err("mipi_tmd_lcd_init() failed with ret %u\n", ret);
		LCD_DEBUG_OUT_LOG
		return ret;
	}

	pdev = platform_device_alloc("mipi_tmd", (panel << 8)|channel);
	if (!pdev){
		LCD_DEBUG_OUT_LOG
		return -ENOMEM;
	}

	tmd_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &tmd_panel_data,
		sizeof(tmd_panel_data));
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

static int mipi_tmd_lcd_init(void)
{
	LCD_DEBUG_IN_LOG
	
	mipi_dsi_buf_alloc(&tmd_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&tmd_rx_buf, DSI_BUF_SIZE);
	
	LCD_DEBUG_OUT_LOG
	
	return platform_driver_register(&this_driver);
}

static ssize_t mipi_tmd_store_vblind_setting(struct device *dev,
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
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    if((intbuf == 0) && (mipi_tmd_vp_info.status == MIPI_TMD_ON))
    {
    	//PFM OFF
    	mipi_tmd_pfm_off(mfd);
    }
    else if((intbuf == 1) && (mipi_tmd_vp_info.status == MIPI_TMD_OFF))
    {
    	//PFM ON
    	mipi_tmd_pfm_on(mfd);
    }
    else
    {
    	mutex_unlock(&mipi_tmd_vblind_lock);
    	
    	LCD_DEBUG_OUT_LOG
    	return count;
    }
    mutex_unlock(&mipi_tmd_vblind_lock);
    
	LCD_DEBUG_OUT_LOG
	
    return count;
}

static ssize_t mipi_tmd_show_vblind_setting(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int value;
    
    
    LCD_DEBUG_IN_LOG
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    value = mipi_tmd_vp_info.status;
    
    mutex_unlock(&mipi_tmd_vblind_lock);
    
    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t mipi_tmd_store_vblind_strength(struct device *dev,
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
	
    if ((intbuf < 0) || (intbuf >= MAX_PFM_LEVEL)){
    	LCD_DEBUG_OUT_LOG
        return -EINVAL;
	}

    mipi_tmd_vp_info.strength = intbuf;
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    if(mipi_tmd_vp_info.status == MIPI_TMD_ON)
    {
    	mipi_tmd_pfm_on(mfd);
    }
    
    mutex_unlock(&mipi_tmd_vblind_lock);
    
    LCD_DEBUG_OUT_LOG
    
    return count;
}

static ssize_t mipi_tmd_show_vblind_strength(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int value;
    
    
    LCD_DEBUG_IN_LOG
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    value = mipi_tmd_vp_info.strength;
    
    mutex_unlock(&mipi_tmd_vblind_lock);
    
    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t mipi_tmd_store_vblind_convolute(struct device *dev,
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
	
    if ((intbuf != 1) && (intbuf != 0)){
    	LCD_DEBUG_OUT_LOG
        return -EINVAL;
    }
    
    mipi_tmd_vp_info.convolute = intbuf;
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    if(mipi_tmd_vp_info.status == MIPI_TMD_ON)
    {
    	mipi_tmd_pfm_on(mfd);
    }
	
	mutex_unlock(&mipi_tmd_vblind_lock);
	
	LCD_DEBUG_OUT_LOG
	
    return count;
    
}

static ssize_t mipi_tmd_show_vblind_convolute(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int value;
    
    
    LCD_DEBUG_IN_LOG
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    value =  mipi_tmd_vp_info.convolute;
    
    mutex_unlock(&mipi_tmd_vblind_lock);
    
    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t mipi_tmd_store_dispstate_setting(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	LCD_DEBUG_IN_LOG
	LCD_DEBUG_OUT_LOG
	
    return count;
}

static ssize_t mipi_tmd_show_dispstate_setting(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int value;
    
    
    LCD_DEBUG_IN_LOG
    
    mutex_lock(&mipi_tmd_vblind_lock);
    
    value = mipi_tmd_vp_info.display_mode;
    
    mutex_unlock(&mipi_tmd_vblind_lock);
    
    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

#ifdef MIPI_TMD_CMD_FUNC
static ssize_t debug_mipi_tmd_store_cmd_tx(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)fbi->par;
	struct dsi_cmd_desc tmd_cmds;
    u8 value;
    int r,i,cnt = 0;
    u8 buff[3];
    unsigned char *data;
    int cmd_len;

	LCD_DEBUG_IN_LOG
	
	mutex_lock(&mipi_tmd_pow_lock);

	if(mipi_tmd_wkq_status == MIPI_TMD_OFF)
	{
		mutex_unlock(&mipi_tmd_pow_lock);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}
	
	cmd_len = ((count - 1) / 2) - 1;
	data = kmalloc(cmd_len, GFP_KERNEL);
	
	//Command initialization
	tmd_cmds.last = 1;
	tmd_cmds.vc   = 0;
	tmd_cmds.ack  = 0;
	tmd_cmds.wait = 0;
	tmd_cmds.dlen = cmd_len;
	tmd_cmds.payload = data;
	
	for(i=0 ;i<count-1; i=i+2)
	{
		buff[0] = buf[i];
		buff[1] = buf[i+1];
		buff[2] = '\0';
		
		r = kstrtou8(buff, 16, &value);
		if (r != 0){
			mutex_unlock(&mipi_tmd_pow_lock);
			kfree(data);
    		LCD_DEBUG_OUT_LOG
			return -EINVAL;
		}
		
		if( i == 0)
		{
			tmd_cmds.dtype = value;
			pr_info("DATA IDENTIFIER:0x%X\n",value);
		}
		else
		{
			data[cnt++] = value;
			pr_info("DATA[%d]:0x%X",cnt,value);
		}
	}
	

	mipi_set_tx_power_mode(debug_mipi_tmd_mode_value);
	
	mipi_dsi_tmd_cmd_sync(mfd); //vsync wait
	
	mipi_dsi_cmds_tx(&tmd_tx_buf, &tmd_cmds, 1);
	
	mipi_set_tx_power_mode(0);
	
	mutex_unlock(&mipi_tmd_pow_lock);
	
	kfree(data);
	
	pr_info("\n Transmission completed !\n");

	LCD_DEBUG_OUT_LOG
	
    return count;
}
static ssize_t debug_mipi_tmd_show_cmd_tx(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    LCD_DEBUG_IN_LOG

    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t debug_mipi_tmd_store_cmd_rx(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)fbi->par;
	struct dsi_cmd_desc tmd_cmds;
    u8 value;
    int r,i,cnt = 0;
    u8 buff[3];
    unsigned char *data;
    int cmd_len;
    
	
	LCD_DEBUG_IN_LOG

	mutex_lock(&mipi_tmd_pow_lock);

	if(mipi_tmd_wkq_status == MIPI_TMD_OFF)
	{
		mutex_unlock(&mipi_tmd_pow_lock);
		LCD_DEBUG_OUT_LOG
		return -ENODEV;
	}
	
	cmd_len = ((count - 1) / 2) - 1;
	data = kmalloc(cmd_len, GFP_KERNEL);
	
	//Command initialization
	tmd_cmds.last = 1;
	tmd_cmds.vc   = 0;
	tmd_cmds.ack  = 1;
	tmd_cmds.wait = 0;
	tmd_cmds.dlen = cmd_len;
	tmd_cmds.payload = data;
	
	for(i=0 ;i<count-1; i=i+2)
	{
		buff[0] = buf[i];
		buff[1] = buf[i+1];
		buff[2] = '\0';
		
		r = kstrtou8(buff, 16, &value);
		if (r != 0){
			mutex_unlock(&mipi_tmd_pow_lock);
			kfree(data);
    		LCD_DEBUG_OUT_LOG
			return -EINVAL;
		}
		
		if( i == 0)
		{
			tmd_cmds.dtype = value;
			pr_info("DATA IDENTIFIER:0x%X\n",value);
		}
		else
		{
			data[cnt++] = value;
			pr_info("DATA[%d]:0x%X",cnt,value);
		}
	}
	

	mipi_set_tx_power_mode(debug_mipi_tmd_mode_value);

	mipi_dsi_tmd_cmd_sync(mfd); //vsync wait
	
	mipi_dsi_cmds_rx(mfd, &tmd_tx_buf, &tmd_rx_buf, &tmd_cmds, 1);
	
	mipi_set_tx_power_mode(0);
	
	debug_mipi_tmd_rx_value = (uint32)*(tmd_rx_buf.data);
	
	mutex_unlock(&mipi_tmd_pow_lock);
	
	kfree(data);
	
	pr_info("\n Reception completed ! Value = 0x%X\n",(uint32)*(tmd_rx_buf.data));

	LCD_DEBUG_OUT_LOG
	
    return count;
}
static ssize_t debug_mipi_tmd_show_cmd_rx(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    LCD_DEBUG_IN_LOG
    
    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", debug_mipi_tmd_rx_value);
}

static ssize_t debug_mipi_tmd_store_cmd_mode(struct device *dev,
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
		debug_mipi_tmd_mode_value = intbuf;
		pr_info("MIPI:LP mode\n");
	}
	else if(intbuf == 0)
	{
		mipi_set_tx_power_mode(0); /* HS mode   */
		debug_mipi_tmd_mode_value = intbuf;
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
static ssize_t debug_mipi_tmd_show_cmd_mode(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    LCD_DEBUG_IN_LOG
    
    LCD_DEBUG_OUT_LOG
    
    return snprintf(buf, PAGE_SIZE, "%d\n", debug_mipi_tmd_mode_value);
}
#endif

static struct device_attribute mipi_tmd_lcd_attributes[] = {
	__ATTR(vblind_setting, S_IRUGO|S_IWUSR, 
			mipi_tmd_show_vblind_setting, mipi_tmd_store_vblind_setting),
	__ATTR(vblind_convolute, S_IRUGO|S_IWUSR, 
			mipi_tmd_show_vblind_convolute, mipi_tmd_store_vblind_convolute),
	__ATTR(vblind_strength, S_IRUGO|S_IWUSR, 
			mipi_tmd_show_vblind_strength, mipi_tmd_store_vblind_strength),
	__ATTR(dispstate_setting, S_IRUGO|S_IWUSR, 
			mipi_tmd_show_dispstate_setting, mipi_tmd_store_dispstate_setting),
#ifdef MIPI_TMD_CMD_FUNC
	__ATTR(mipi_cmd_tx, S_IRUGO|S_IWUSR, 
			debug_mipi_tmd_show_cmd_tx, debug_mipi_tmd_store_cmd_tx),
	__ATTR(mipi_cmd_rx, S_IRUGO|S_IWUSR, 
			debug_mipi_tmd_show_cmd_rx, debug_mipi_tmd_store_cmd_rx),
	__ATTR(mipi_cmd_mode, S_IRUGO|S_IWUSR, 
			debug_mipi_tmd_show_cmd_mode, debug_mipi_tmd_store_cmd_mode),
#endif //MIPI_TMD_CMD_FUNC
};

static int mipi_tmd_lcd_sysfs_register(struct device *dev)
{
	int i;


	LCD_DEBUG_IN_LOG
	
	for (i = 0; i < ARRAY_SIZE(mipi_tmd_lcd_attributes); i++)
		if (device_create_file(dev, &mipi_tmd_lcd_attributes[i]))
			goto error;

	LCD_DEBUG_OUT_LOG
	
	return 0;

error:
	for (; i >= 0 ; i--)
		device_remove_file(dev, &mipi_tmd_lcd_attributes[i]);
	pr_err("%s: Unable to create interface\n", __func__);

	LCD_DEBUG_OUT_LOG

	return -ENODEV;
}

static void mipi_tmd_lcd_sysfs_unregister(struct device *dev)
{
	int i;
	
	
	LCD_DEBUG_IN_LOG
	
	for (i = 0; i < ARRAY_SIZE(mipi_tmd_lcd_attributes); i++)
		device_remove_file(dev, &mipi_tmd_lcd_attributes[i]);

	LCD_DEBUG_OUT_LOG
}
