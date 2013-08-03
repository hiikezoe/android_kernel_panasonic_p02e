
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

#ifndef MIPI_SHARP_H
#define MIPI_SHARP_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>
void mipi_sharp_vsync_on(void);
int mipi_sharp_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);


extern int  mipi_sharp_lcd_disp_on(struct platform_device *pdev);
extern void mipi_sharp_lcd_off_cmd(struct platform_device *pdev);

#define MIPI_SHARP_PWM_FREQ_HZ 1000
#define MIPI_SHARP_PWM_PERIOD_USEC (USEC_PER_SEC / MIPI_SHARP_PWM_FREQ_HZ)
#define MIPI_SHARP_PWM_LEVEL 255
#define MIPI_SHARP_PWM_DUTY_CALC 10

#define MIPI_SHARP_POWER_MODE_NORMAL 0x14

#define MIPI_SHARP_ON  1
#define MIPI_SHARP_OFF 0

#define MIPI_SHARP_BLC_CNT_INIT     2

#define MIPI_SHARP_TIMER_BLC     0x01
#define MIPI_SHARP_TIMER_REFRESH 0x02
#define MIPI_SHARP_TIMER_ALL (MIPI_SHARP_TIMER_BLC | MIPI_SHARP_TIMER_REFRESH)

/* SHARP Display function */
/*#define MIPI_SHARP_FUNC_REFRESH *//* npdc300085685 */
#define MIPI_SHARP_FUNC_REFRESH_RESET
#define MIPI_SHARP_FUNC_REFRESH_REWRITE
#define MIPI_SHARP_FUNC_REFRESH_LP
/* #define MIPI_SHARP_FUNC_BLC *//* npdc300085685 */
#define MIPI_SHARP_CMD_FUNC

struct t_mipi_sharp_cmd_eeprom_tbl {
	unsigned short type;                          /* Access ID                */
	unsigned short data_size;                     /* data Size                */
	unsigned char  *data;                         /* data Adr                 */
};

struct t_mipi_sharp_vblind_info_tbl {
	int status;                                   /* vblind status               */
	int strength;                                 /* vblind strength             */
};

enum mipi_sharp_display_mode {
    DISP_STANDBY = 0,
    DISP_MOVIE,
    DISP_CAMERA,
    DISP_ONESEG,
    DISP_MODE_NUM,
};

enum mipi_sharp_led_display_mode {
    MIPI_SHARP_NORMAL_MODE = 0,
    MIPI_SHARP_MOVIE_MODE,
    MIPI_SHARP_ONESEG_MODE,
    MIPI_SHARP_CAMERA_MODE,
    MIPI_SHARP_ECO_MODE,
    MIPI_SHARP_MODE_NUM,
};

/* debug */
//#define MIPI_SHARP_DEBUG_LOG
//#define MIPI_SHARP_DEBUG_LOG_ALL

#ifdef MIPI_SHARP_DEBUG_LOG
#define LCD_DEBUG_IN_LOG                                                       \
{                                                                              \
    printk("[SHARP_LCD  IN ][%d %s]\n", __LINE__, __FUNCTION__);                 \
}
#define LCD_DEBUG_OUT_LOG                                                      \
{                                                                              \
    printk("[SHARP_LCD OUT ][%d %s]\n", __LINE__, __FUNCTION__);                 \
}
#define LCD_DEBUG_LOG(format, arg...)                                          \
{                                                                              \
    printk("[SHARP_LCD LOG ][%s]: " format "\n" , __FUNCTION__ , ## arg);        \
}
#else //MIPI_SHARP_DEBUG_LOG
#define LCD_DEBUG_IN_LOG                                                   { ; }
#define LCD_DEBUG_OUT_LOG                                                  { ; }
#define LCD_DEBUG_LOG(format, arg...)                                      { ; }
#endif //MIPI_SHARP_DEBUG_LOG

#ifdef MIPI_SHARP_DEBUG_LOG_ALL
#define LCD_DEBUG_LOG_DETAILS(format, arg...)                                  \
{                                                                              \
    printk("[SHARP_LCD LOG ][%s]: " format "\n" , __FUNCTION__ , ## arg);        \
}
#else //MIPI_SHARP_DEBUG_LOG_ALL
#define LCD_DEBUG_LOG_DETAILS(format, arg...)                              { ; }
#endif //MIPI_SHARP_DEBUG_LOG_ALL

#endif  /* MIPI_SHARP_H */
