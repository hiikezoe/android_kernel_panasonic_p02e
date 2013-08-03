
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

#ifndef MIPI_TMD_H
#define MIPI_TMD_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>
void mipi_tmd_vsync_on(void);
int mipi_tmd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

void mipi_tmd_lcd_on_cmd(void);
void mipi_tmd_lcd_off_cmd(void);

#define MIPI_TMD_PWM_FREQ_HZ 1000
#define MIPI_TMD_PWM_PERIOD_USEC (USEC_PER_SEC / MIPI_TMD_PWM_FREQ_HZ)
#define MIPI_TMD_PWM_LEVEL 255
#define MIPI_TMD_PWM_DUTY_CALC 10

#define MIPI_TMD_POWER_MODE_NORMAL 0x14

#define MIPI_TMD_ON  1
#define MIPI_TMD_OFF 0

#define MIPI_TMD_BLC_CNT_INIT     2

#define MIPI_TMD_TIMER_BLC     0x01
#define MIPI_TMD_TIMER_REFRESH 0x02
#define MIPI_TMD_TIMER_ALL (MIPI_TMD_TIMER_BLC | MIPI_TMD_TIMER_REFRESH)

/* TMD Display function */
#define MIPI_TMD_FUMC_REFRESH
#define MIPI_TMD_FUMC_REFRESH_RESET
#define MIPI_TMD_FUMC_REFRESH_REWRITE
#define MIPI_TMD_FUMC_REFRESH_LP
/* #define MIPI_TMD_FUNC_BLC */ /* ZANTEI VerUp */
#define MIPI_TMD_CMD_FUNC

struct t_mipi_tmd_cmd_eeprom_tbl {
	unsigned short type;                          /* Access ID                */
	unsigned short data_size;                     /* data Size                */
	unsigned char  *data;                         /* data Adr                 */
};

struct t_mipi_tmd_vblind_info_tbl {
	int status;                                   /* PFM status               */
	int strength;                                 /* PFM strength             */
	int convolute;                                /* convolute On/Off         */
	int display_mode;                             /* display mode   f         */
};

enum mipi_tmd_display_mode {
    DISP_STANDBY = 0,
    DISP_MOVIE,
    DISP_CAMERA,
    DISP_ONESEG,
    DISP_MODE_NUM,
};

enum mipi_tmd_led_display_mode {
    MIPI_TMD_NORMAL_MODE = 0,
    MIPI_TMD_MOVIE_MODE,
    MIPI_TMD_ONESEG_MODE,
    MIPI_TMD_CAMERA_MODE,
    MIPI_TMD_ECO_MODE,
    MIPI_TMD_MODE_NUM,
};

/* debug */
//#define MIPI_TMD_DEBUG_LOG
//#define MIPI_TMD_DEBUG_LOG_ALL

#ifdef MIPI_TMD_DEBUG_LOG
#define LCD_DEBUG_IN_LOG                                                       \
{                                                                              \
    printk("[TMD_LCD  IN ][%d %s]\n", __LINE__, __FUNCTION__);                 \
}
#define LCD_DEBUG_OUT_LOG                                                      \
{                                                                              \
    printk("[TMD_LCD OUT ][%d %s]\n", __LINE__, __FUNCTION__);                 \
}
#define LCD_DEBUG_LOG(format, arg...)                                          \
{                                                                              \
    printk("[TMD_LCD LOG ][%s]: " format "\n" , __FUNCTION__ , ## arg);        \
}
#else //MIPI_TMD_DEBUG_LOG
#define LCD_DEBUG_IN_LOG                                                   { ; }
#define LCD_DEBUG_OUT_LOG                                                  { ; }
#define LCD_DEBUG_LOG(format, arg...)                                      { ; }
#endif //MIPI_TMD_DEBUG_LOG

#ifdef MIPI_TMD_DEBUG_LOG_ALL
#define LCD_DEBUG_LOG_DETAILS(format, arg...)                                  \
{                                                                              \
    printk("[TMD_LCD LOG ][%s]: " format "\n" , __FUNCTION__ , ## arg);        \
}
#else //MIPI_TMD_DEBUG_LOG_ALL
#define LCD_DEBUG_LOG_DETAILS(format, arg...)                              { ; }
#endif //MIPI_TMD_DEBUG_LOG_ALL

#endif  /* MIPI_TMD_H */
