/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/err.h>

#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/pwm.h>
#include <linux/leds-pm8xxx.h>

#ifdef CONFIG_LEDS_PM8960
#include <linux/cfgdrv.h>
#include <linux/hcm_eep.h>
#include "leds.h"
#endif /*CONFIG_LEDS_PM8960*/

#include <linux/delay.h>


#define SSBI_REG_ADDR_DRV_KEYPAD	0x48
#define PM8XXX_DRV_KEYPAD_BL_MASK	0xf0
#define PM8XXX_DRV_KEYPAD_BL_SHIFT	0x04

#define SSBI_REG_ADDR_FLASH_DRV0        0x49
#define PM8XXX_DRV_FLASH_MASK           0xf0
#define PM8XXX_DRV_FLASH_SHIFT          0x04

#define SSBI_REG_ADDR_FLASH_DRV1        0xFB

#define SSBI_REG_ADDR_LED_CTRL_BASE	0x131
#define SSBI_REG_ADDR_LED_CTRL(n)	(SSBI_REG_ADDR_LED_CTRL_BASE + (n))
#define PM8XXX_DRV_LED_CTRL_MASK	0xf8
#define PM8XXX_DRV_LED_CTRL_SHIFT	0x03

#define SSBI_REG_ADDR_WLED_CTRL_BASE	0x25A
#define SSBI_REG_ADDR_WLED_CTRL(n)	(SSBI_REG_ADDR_WLED_CTRL_BASE + (n) - 1)

/* wled control registers */
#define WLED_MOD_CTRL_REG		SSBI_REG_ADDR_WLED_CTRL(1)
#define WLED_MAX_CURR_CFG_REG(n)	SSBI_REG_ADDR_WLED_CTRL(n + 2)
#define WLED_BRIGHTNESS_CNTL_REG1(n)	SSBI_REG_ADDR_WLED_CTRL(n + 5)
#define WLED_BRIGHTNESS_CNTL_REG2(n)	SSBI_REG_ADDR_WLED_CTRL(n + 6)
#define WLED_SYNC_REG			SSBI_REG_ADDR_WLED_CTRL(11)
#define WLED_OVP_CFG_REG		SSBI_REG_ADDR_WLED_CTRL(13)
#define WLED_BOOST_CFG_REG		SSBI_REG_ADDR_WLED_CTRL(14)
#define WLED_HIGH_POLE_CAP_REG		SSBI_REG_ADDR_WLED_CTRL(16)

#define WLED_STRINGS			0x03
#define WLED_OVP_VAL_MASK		0x30
#define WLED_OVP_VAL_BIT_SHFT		0x04
#define WLED_BOOST_LIMIT_MASK		0xE0
#define WLED_BOOST_LIMIT_BIT_SHFT	0x05
#define WLED_BOOST_OFF			0x00
#define WLED_EN_MASK			0x01
#define WLED_CP_SELECT_MAX		0x03
#define WLED_CP_SELECT_MASK		0x03
#define WLED_DIG_MOD_GEN_MASK		0x70
#define WLED_CS_OUT_MASK		0x0E
#define WLED_CTL_DLY_STEP		200
#define WLED_CTL_DLY_MAX		1400
#define WLED_CTL_DLY_MASK		0xE0
#define WLED_CTL_DLY_BIT_SHFT		0x05
#define WLED_MAX_CURR			25
#define WLED_MAX_CURR_MASK		0x1F
#define WLED_OP_FDBCK_MASK		0x1C
#define WLED_OP_FDBCK_BIT_SHFT		0x02

#define WLED_MAX_LEVEL			255
#define WLED_8_BIT_MASK			0xFF
#define WLED_8_BIT_SHFT			0x08
#define WLED_MAX_DUTY_CYCLE		0xFFF

#define WLED_SYNC_VAL			0x07
#define WLED_SYNC_RESET_VAL		0x00

#define SSBI_REG_ADDR_RGB_CNTL1		0x12D
#define SSBI_REG_ADDR_RGB_CNTL2		0x12E

#define PM8XXX_DRV_RGB_RED_LED		BIT(2)
#define PM8XXX_DRV_RGB_GREEN_LED	BIT(1)
#define PM8XXX_DRV_RGB_BLUE_LED		BIT(0)

#define MAX_FLASH_LED_CURRENT		300
#define MAX_LC_LED_CURRENT		40
#define MAX_KP_BL_LED_CURRENT		300

#define PM8XXX_ID_LED_CURRENT_FACTOR	2  /* Iout = x * 2mA */
#define PM8XXX_ID_FLASH_CURRENT_FACTOR	20 /* Iout = x * 20mA */

#define PM8XXX_FLASH_MODE_DBUS1		1
#define PM8XXX_FLASH_MODE_DBUS2		2
#define PM8XXX_FLASH_MODE_PWM		3

#define MAX_LC_LED_BRIGHTNESS		20
#define MAX_FLASH_BRIGHTNESS		15
#define MAX_KB_LED_BRIGHTNESS		15

#define PM8XXX_LED_OFFSET(id) ((id) - PM8XXX_ID_LED_0)

#define PM8XXX_LED_PWM_FLAGS	(PM_PWM_LUT_LOOP | PM_PWM_LUT_RAMP_UP)

#ifdef CONFIG_LEDS_PM8960
#define LED_MAP(_version, _kb, _led0, _led1, _led2, _color, _flash_led0, _flash_led1, \
	_wled, _rgb_led_red, _rgb_led_green, _rgb_led_blue)\
	{\
		.version = _version,\
		.supported = _kb << PM8XXX_ID_LED_KB_LIGHT | \
			_led0 << PM8XXX_ID_LED_0 | _led1 << PM8XXX_ID_LED_1 | \
			_led2 << PM8XXX_ID_LED_2  | \
			_color << PM8XXX_ID_COLOR | \
			_flash_led0 << PM8XXX_ID_FLASH_LED_0 | \
			_flash_led1 << PM8XXX_ID_FLASH_LED_1 | \
			_wled << PM8XXX_ID_WLED | \
			_rgb_led_red << PM8XXX_ID_RGB_LED_RED | \
			_rgb_led_green << PM8XXX_ID_RGB_LED_GREEN | \
			_rgb_led_blue << PM8XXX_ID_RGB_LED_BLUE, \
	}
#else
#define LED_MAP(_version, _kb, _led0, _led1, _led2, _flash_led0, _flash_led1, \
	_wled, _rgb_led_red, _rgb_led_green, _rgb_led_blue)\
	{\
		.version = _version,\
		.supported = _kb << PM8XXX_ID_LED_KB_LIGHT | \
			_led0 << PM8XXX_ID_LED_0 | _led1 << PM8XXX_ID_LED_1 | \
			_led2 << PM8XXX_ID_LED_2  | \
			_flash_led0 << PM8XXX_ID_FLASH_LED_0 | \
			_flash_led1 << PM8XXX_ID_FLASH_LED_1 | \
			_wled << PM8XXX_ID_WLED | \
			_rgb_led_red << PM8XXX_ID_RGB_LED_RED | \
			_rgb_led_green << PM8XXX_ID_RGB_LED_GREEN | \
			_rgb_led_blue << PM8XXX_ID_RGB_LED_BLUE, \
	}
#endif /* CONFIG_LEDS_PM8960 */

/**
 * supported_leds - leds supported for each PMIC version
 * @version - version of PMIC
 * @supported - which leds are supported on version
 */

struct supported_leds {
	enum pm8xxx_version version;
	u32 supported;
};

#ifdef CONFIG_LEDS_PM8960
static const struct supported_leds led_map[] = {
	LED_MAP(PM8XXX_VERSION_8058, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8921, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8018, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8922, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1),
	LED_MAP(PM8XXX_VERSION_8038, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1),
};
#else
static const struct supported_leds led_map[] = {
	LED_MAP(PM8XXX_VERSION_8058, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8921, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8018, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8922, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1),
	LED_MAP(PM8XXX_VERSION_8038, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1),
};
#endif /* CONFIG_LEDS_PM8960 */

/**
 * struct pm8xxx_led_data - internal led data structure
 * @led_classdev - led class device
 * @id - led index
 * @work - workqueue for led
 * @lock - to protect the transactions
 * @reg - cached value of led register
 * @pwm_dev - pointer to PWM device if LED is driven using PWM
 * @pwm_channel - PWM channel ID
 * @pwm_period_us - PWM period in micro seconds
 * @pwm_duty_cycles - struct that describes PWM duty cycles info
 */
struct pm8xxx_led_data {
	struct led_classdev	cdev;
	int			id;
	u8			reg;
	u8			wled_mod_ctrl_val;
	struct device		*dev;
	struct work_struct	work;
	struct mutex		lock;
	struct pwm_device	*pwm_dev;
	int			pwm_channel;
	u32			pwm_period_us;
	struct pm8xxx_pwm_duty_cycles *pwm_duty_cycles;
	struct wled_config_data *wled_cfg;
	int			max_current;

#ifdef CONFIG_LEDS_PM8960
	uint8_t new_ampere;
	enum led_brightness new_brightness;
#endif /*CONFIG_LEDS_PM8960*/
};

#ifdef CONFIG_LEDS_PM8960
/* define */
#define RETRY			(3)
#define LCDBL_TABLE_LEN		(256)

#define LCDBL_TABLE_SIZE	(15)
#define LCDBL_TABLE_DCDC	(12)
#define LCDBL_TABLE_RGBPWM	(18)
#define LCDBL_TABLE_BASE	(LCDBL_TABLE_SIZE - 5)

#define MAX_BACKLIGHT_BRIGHTNESS	(255)

#define LCDBL_DCDC_MAX		(255)

#define LED_NAME_RED		"red"
#define LED_NAME_GREEN		"green"
// <SlimAV1> add S
#define LED_NAME_BLUE		"blue"
// <SlimAV1> add E

#define LED_RED				0
#define LED_GREEN			1
// <SlimAV1> add S
#define LED_BLUE			2
// <SlimAV1> add E

#define PM8960_MAX_TIME			8000
#define PM8960_LED_PWM_DUTY_MS	250
#define PM8960_PWM_TIME_UNIT	PM8960_LED_PWM_DUTY_MS
#define PM8960_PWM_MIN_TIME		2
#define PM8960_PWM_MAX_TIME		(PM8960_MAX_TIME/PM8960_PWM_TIME_UNIT)

/* table initial define */

#if 1 /*RUPY_chg*/
/*ここの値は初期値。実際の値はcfgdrv_readで不揮発から取得する値を使用している*/
#define LOCAL_D_HCM_LPWMON			{ 0x3F, 0x3F, 0x36, 0x2D, 0x23, 0x1A, 0x15, 0x12, 0x0F, 0x0C, 0x09, 0x09, 0x16, 0xFA, 0x00 }
#define LOCAL_D_HCM_LPWMON_ECO		{ 0x41, 0x41, 0x38, 0x2F, 0x27, 0x1E, 0x1A, 0x17, 0x13, 0x10, 0x0C, 0x0C, 0x0D, 0xFA, 0x00 }
#define LOCAL_D_HCM_LPWMON_HIGH		{ 0x3F, 0x3F, 0x39, 0x34, 0x31, 0x2E, 0x2B, 0x28, 0x1E, 0x14, 0x09, 0x09, 0x16, 0xFA, 0x00 }
#if 1 //npdc300052586 start
#define LOCAL_D_HCM_LPWMON_POWERMODE	{ 0x58 }
#else
#define LOCAL_D_HCM_LPWMON_POWERMODE	{ 0x00 }
#endif //npdc300052586 end
#else
#define LOCAL_D_HCM_LPWMON			{ 0x3F, 0x3F, 0x34, 0x2D, 0x25, 0x1F, 0x1A, 0x15, 0x10, 0x0B, 0x07, 0x07, 0x05, 0xE8, 0x03 }
#define LOCAL_D_HCM_LPWMON_ECO		{ 0x41, 0x41, 0x36, 0x2E, 0x26, 0x21, 0x1C, 0x17, 0x12, 0x0C, 0x0A, 0x0A, 0x03, 0xE8, 0x03 }
#define LOCAL_D_HCM_LPWMON_HIGH		{ 0x3F, 0x3F, 0x39, 0x34, 0x31, 0x2E, 0x2B, 0x28, 0x1E, 0x14, 0x07, 0x07, 0x05, 0xE8, 0x03 }
#define LOCAL_D_HCM_LPWMON_POWERMODE	{ 0x00 }
#endif /*RUPY_chg*/

#if 1 //RUPY_add start
/*初期値はmaxLvと違う値に設定する*/
#define LCDBLT_INITIAL_VAL 11;
#endif //RUPY_add end



// <SlimAV1> mod S
#if 0
#define LOCAL_D_HCM_RGBPWM		{ 0x02, 0x02, 0x04, 0x05, 0xE8, 0x03, 0x03, 0x01, 0x04, 0x04, 0xE8, 0x03, \
								  0x00, 0x00, 0x00, 0x00, 0x00, 0x00  }
#else
#define LOCAL_D_HCM_RGBPWM		{ 0x02, 0x02, 0x04, 0x05, 0xE8, 0x03, 0x03, 0x01, 0x04, 0x04, 0xE8, 0x03, \
								  0x04, 0x03, 0x03, 0x06, 0xE8, 0x03  }
#endif
// <SlimAV1> mod E

/* define macro */
#ifdef LED_LOG_ENABLE
#define LED_LOG( fmt, ... )		\
{					\
	struct timeval tp;		\
	do_gettimeofday( &tp );		\
					\
    printk( KERN_INFO "###LED Drv### %ld:%6ld %04d %s "fmt,         \
		tp.tv_sec, tp.tv_usec, __LINE__, __FUNCTION__, ##__VA_ARGS__ ); \
}
#else
#define LED_LOG( fmt, ...)/* mrc40302 */
#endif /* LED_LOG_ENABLE */

/* mrc40302 */#define LED_ERRLOG( fmt, ... )		\
{					\
	struct timeval tp;		\
	do_gettimeofday( &tp );		\
					\
    printk( KERN_INFO "###LED Drv ERROR### %ld:%6ld %04d %s "fmt,       \
		tp.tv_sec, tp.tv_usec, __LINE__, __FUNCTION__, ##__VA_ARGS__ ); \
}

#define TIME_BASE( msecs )	(msecs_to_jiffies((unsigned int)(msecs)))

/* enum */
enum LEDDRV_CFG{
	LED_LCDBL_NORMAL = 0,
	LED_LCDBL_ECOMODE,
	LED_LCDBL_HIGHBRIGHT,
	LED_LCDBL_POWERMODE,
	LED_LED_RGBPWM,
	LED_ENUM_MAX
};

/* prototype */
static int	leddrv_lcdbl_no_handler(int ampere, enum leddrv_lcdbl_mode_status status);
static void leddrv_static_init(void);
static void lcdbl_status_allmode_set(void);
static void pm8xxx_led_func_lcdbl(struct led_classdev *led);
static int pm8xxx_set_led_mode_and_max_brightness(struct pm8xxx_led_data *led,
		enum pm8xxx_led_modes led_mode, int max_current);

/* static status */
static LEDDRV_LCDBL_HANDLER lcdbl_change = &leddrv_lcdbl_no_handler;
static enum leddrv_lcdbl_mode_status mode_status = LEDDRV_NORMAL_MODE;
static unsigned long red_delay_on;
static unsigned long red_delay_off;
static struct led_classdev *lcdbl_status_struct = NULL;
static struct mutex handler_locked;


#if 1 //RUPY_add start
/*初期値はmaxLvと違う値に設定する*/
static int oldmaxLV = LCDBLT_INITIAL_VAL;
#endif //RUPY_add end



// <SlimAV1> mod S
#if 0
#define PM8XXX_LED_NUM 2
static struct pm8xxx_led_data *led_keep[PM8XXX_LED_NUM] = {NULL, NULL};
#else
#define PM8XXX_LED_NUM 3
static struct pm8xxx_led_data *led_keep[PM8XXX_LED_NUM] = {NULL, NULL, NULL};
#endif
// <SlimAV1> mod E

/* static flags */
static u8 lcdbl_init = 0;
static u8 set_cfgtable;
// <SlimAV1> mod S
#if 0
static u8 led_setting[2] = { 0, 0};
#else
static u8 led_setting[3] = { 0, 0, 0};
#endif
// <SlimAV1> mod E
static u8 lcdbl_i2c = 0;	/* I2C制御フラグ	*/

/* static arrangement */
static u8 led_lcdbl_normal_table[LCDBL_TABLE_SIZE] = LOCAL_D_HCM_LPWMON;/*D_HCM_LPWMON*//* mrc21401 */

static u8 led_lcdbl_ecomode_table[LCDBL_TABLE_SIZE] = LOCAL_D_HCM_LPWMON_ECO;/*D_HCM_LPWMON_ECO*//* mrc21401 */

static u8 led_lcdbl_highbright_table[LCDBL_TABLE_SIZE] = LOCAL_D_HCM_LPWMON_HIGH;/*D_HCM_LPWMON_HIGH*//* mrc21401 */

static u8 led_lcdbl_powermode_table[1] = LOCAL_D_HCM_LPWMON_POWERMODE;/*D_HCM_LPWMON_POWERMODE*//* mrc21401 */

static u8 led_lcdbl_normal_status[LCDBL_TABLE_LEN] = { 0 };/* mrc21401 */

static u8 led_lcdbl_ecomode_status[LCDBL_TABLE_LEN] = { 0 };/* mrc21401 */

static u8 led_lcdbl_highbright_status[LCDBL_TABLE_LEN] = { 0 };/* mrc21401 */

static u8 led_lcdbl_powermode_status[1] = { 0 };/* mrc21401 */


#if 1 //RUPY_chg start
#define LCDBL_TABLE_MAXDCSIZE 11
/*Max電流値の不揮発初期値*/
static u8 led_lcdblmax_normal_table[LCDBL_TABLE_MAXDCSIZE]     = { 0x0C,0x09,0x07,0x05,0x03,0x02,0x01,0x01,0x01,0x01,0x01 };
static u8 led_lcdblmax_ecomode_table[LCDBL_TABLE_MAXDCSIZE]    = { 0x07,0x05,0x03,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x01 };
static u8 led_lcdblmax_highbright_table[LCDBL_TABLE_MAXDCSIZE] = { 0x0C,0x0A,0x09,0x08,0x07,0x06,0x06,0x03,0x01,0x01,0x01 };

#if 1 //npdc300052586 start
/*パワーモードMax電流値*/
static u8 led_lcdblmax_powermode_table[1] = { 0x16 };
static u8 *led_lcdbl_dcdc[LEDDRV_MODE_NUM] = {led_lcdblmax_normal_table,led_lcdblmax_ecomode_table,led_lcdblmax_highbright_table,led_lcdblmax_powermode_table};/* mrc21401 */

#else //npdc300052586 start

/*DCDCのMax電流設定値(Powerモード以外)*/
static u8 *led_lcdbl_dcdc[LEDDRV_MODE_NUM -1] = {led_lcdblmax_normal_table,led_lcdblmax_ecomode_table,led_lcdblmax_highbright_table};/* mrc21401 */

#endif //npdc300052586 end

#else//RUPY_chg
/*DCDCのMax電流設定値*/
static u8 led_lcdbl_dcdc[LEDDRV_MODE_NUM] = { 22, 13, 22, 0 };/* mrc21401 */
#endif //RUPY_chg end

static u8 *led_lcdbl_mode_status[3] = {
		led_lcdbl_normal_status,
		led_lcdbl_ecomode_status,
		led_lcdbl_highbright_status
};

static u8 led_rgbpwm_table[LCDBL_TABLE_RGBPWM] = LOCAL_D_HCM_RGBPWM;/*D_HCM_RGBPWM*//* mrc21401 */

static int pm8921_redled_pwm_duty_pcts[PM_PWM_LUT_SIZE];

static struct pm8xxx_pwm_duty_cycles pm8921_led0_pwm_duty_cycles = {
	.duty_pcts     = NULL,
	.num_duty_pcts = 0,
	.duty_ms       = PM8960_LED_PWM_DUTY_MS,
	.start_idx     = 0,
};

struct cfg_table{
	unsigned short id;
	unsigned short size;
	u8 *table;
};

static struct cfg_table led_table[] = {/* mrc21401 */
	[LED_LCDBL_NORMAL] = {
		.id = D_HCM_LPWMON,
		.size = LCDBL_TABLE_SIZE,
		.table = led_lcdbl_normal_table
	},
	[LED_LCDBL_ECOMODE] = {
		.id = D_HCM_LPWMON_ECO,
		.size = LCDBL_TABLE_SIZE,
		.table = led_lcdbl_ecomode_table
	},
	[LED_LCDBL_HIGHBRIGHT] = {
		.id = D_HCM_LPWMON_HIGH,
		.size = LCDBL_TABLE_SIZE,
		.table = led_lcdbl_highbright_table
	},
	[LED_LCDBL_POWERMODE] = {
		.id = D_HCM_LPWMON_POWERMODE,
		.size = 1,
		.table = led_lcdbl_powermode_table
	},
	[LED_LED_RGBPWM] = {
		.id = D_HCM_RGBPWM,
		.size = LCDBL_TABLE_RGBPWM,
		.table = led_rgbpwm_table
	}
};


#if 1 //RUPY_add start
/*Max電流値格納テーブル*/
static struct cfg_table led_maxtable[] = {/* mrc21401 */
	[LED_LCDBL_NORMAL] = {
		.id = D_HCM_FULL_SCALE_CURRENT,
		.size = LCDBL_TABLE_MAXDCSIZE,
		.table = led_lcdblmax_normal_table
	},
	[LED_LCDBL_ECOMODE] = {
		.id = D_HCM_FULL_SCALE_CURRENT_ECO,
		.size = LCDBL_TABLE_MAXDCSIZE,
		.table = led_lcdblmax_ecomode_table
	},
	[LED_LCDBL_HIGHBRIGHT] = {
		.id = D_HCM_FULL_SCALE_CURRENT_HIGH,
		.size = LCDBL_TABLE_MAXDCSIZE,
		.table = led_lcdblmax_highbright_table
	}
};
#endif //RUPY_add end



/* カーネル空間IF関数 */
int leddrv_lcdbl_set_handler(enum leddrv_event event, 
		LEDDRV_LCDBL_HANDLER handler)
{
	int lcdbl_init_local;

	LED_LOG("\n");

	switch (event) {
	case LEDDRV_EVENT_LCD_BACKLIGHT_CHANGED:
		break;
	default:
		LED_ERRLOG("unknown ID error\n");
		return -1;
	}

	if(handler == NULL){
		handler = &leddrv_lcdbl_no_handler;
	}

	lcdbl_init_local = lcdbl_init;
	if (lcdbl_init_local){
		mutex_lock(&handler_locked);
	}

	lcdbl_change = handler;

	if (lcdbl_init_local) {
		pm8xxx_led_func_lcdbl(lcdbl_status_struct);
		mutex_unlock(&handler_locked);
	}

	return 0;
}
EXPORT_SYMBOL( leddrv_lcdbl_set_handler);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */

void leddrv_lcdbl_set_ampere(int ampere)
{
	if(!lcdbl_init) {
		LED_ERRLOG("Don't initialize lcdbl yet\n");
		return;
	}

	if( mode_status == LEDDRV_POWER_MODE ) {
		LED_LOG( "Power Mode\n" );
		lcdbl_status_struct->brightness = led_lcdbl_powermode_status[0];
		pm8xxx_led_func_lcdbl(lcdbl_status_struct);
	} else if(ampere <= (int) led_lcdbl_mode_status[mode_status][LCDBL_TABLE_LEN - 1]) {
		LED_LOG("Status Changing\n");

		lcdbl_status_struct->brightness = ampere;
		pm8xxx_led_func_lcdbl(lcdbl_status_struct);
	} else {
		if(led_lcdbl_mode_status[mode_status][LCDBL_TABLE_LEN - 1] != 0) {
			LED_ERRLOG("Status over for maximum\n");
		}
	}
}
EXPORT_SYMBOL( leddrv_lcdbl_set_ampere);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */

int leddrv_lcdbl_set_mode(enum leddrv_lcdbl_mode_set mode)
{
	static u8 set_mode = 0x00;
	u8 offset = 0x00;
	u8 bit = 0x00;
	u8 status = 0x00;
	enum leddrv_lcdbl_mode_status old_mode_status = mode_status;
	int	rc;
#if 1 //RUPY_add start
	int maxLV = 0;
#endif 	//RUPY_add end
	
	if((u8)mode != (((u8)mode) & (u8)LEDDRV_MODE_SCOPE)) {	/* mrc22803 */
		LED_ERRLOG("argument error\n");
		return -1;
	} 

	LED_LOG("mode=0x%02x\n",mode);
	offset = (u8)(((u8) mode) >> 4);
	bit = (u8)(((u8) mode) & 0x0F);
	status = (u8)(0x01 << offset);

	if (bit){
		set_mode = (u8)(set_mode | status);
	} else {
		set_mode = (u8)(set_mode & (u8)~status);
	}

	if (set_mode == 0x00) {
		mode_status = LEDDRV_NORMAL_MODE;
	} else if (set_mode == 0x01) {
		mode_status = LEDDRV_ECO_MODE;
	} else {
		mode_status = LEDDRV_HIGHBRIGHT_MODE;
	}
	if( set_mode & 0x04 ) {
		mode_status = LEDDRV_POWER_MODE;
	} else if( set_mode & 0x02 ) {
		mode_status = LEDDRV_HIGHBRIGHT_MODE;
	} else if( set_mode & 0x01 ) {
		mode_status = LEDDRV_ECO_MODE;
	} else {
		mode_status = LEDDRV_NORMAL_MODE;
	}
	LED_LOG("set_mode=0x%02x mode_status=%d\n",set_mode, mode_status);

	if( (mode_status != old_mode_status) && (lcdbl_i2c == 1) ){
#if 1 //RUPY_chg start
		/*輝度レベル 微灯の場合*/
		if(lcdbl_status_struct->brightness < 20)
		{
			maxLV = 10;
		}
		/*輝度レベル 1の場合*/
		else if(lcdbl_status_struct->brightness < 55 )
		{
			maxLV = 9;
		}
		/*輝度レベル 2の場合*/
		else if(lcdbl_status_struct->brightness < 80)
		{
			maxLV = 8;
		}
		/*輝度レベル 3の場合*/
		else if(lcdbl_status_struct->brightness < 105)
		{
			maxLV = 7;
		}
		/*輝度レベル 4の場合*/
		else if(lcdbl_status_struct->brightness < 130)
		{
			maxLV = 6;
		}
		/*輝度レベル 5の場合*/
		else if(lcdbl_status_struct->brightness < 155)
		{
			maxLV = 5;
		}
		/*輝度レベル 6の場合*/
		else if(lcdbl_status_struct->brightness < 180)
		{
			maxLV = 4;
		}
		/*輝度レベル 7の場合*/
		else if(lcdbl_status_struct->brightness < 205)
		{
			maxLV = 3;
		}
		/*輝度レベル 8の場合*/
		else if(lcdbl_status_struct->brightness < 230)
		{
			maxLV = 2;
		}
		/*輝度レベル 9の場合*/
		else if(lcdbl_status_struct->brightness < 255)
		{
			maxLV = 1;
		}
		/*輝度レベル 10の場合*/
		else
		{
			maxLV = 0;
		}
#if 1 //npdc300052586 start
		/*パワーモードの場合Max輝度値は1つだけ*/
		if(mode_status == LEDDRV_POWER_MODE)
		{
			maxLV = 0;
		}
#endif //npdc300052586 end
		rc = pm8xxx_ledi2c_on(led_lcdbl_dcdc[mode_status][maxLV]);

#else //RUPY_chg
		rc = pm8xxx_ledi2c_on(led_lcdbl_dcdc[mode_status]);
#endif //RUPY_chg end
	}
	return 0;
}
EXPORT_SYMBOL( leddrv_lcdbl_set_mode);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */

int leddrv_keybl_key_press(void)
{
	int ret = 0;

	LED_LOG("Not be initialized of KeyBacklight yet.\n");

	return ret;
}
EXPORT_SYMBOL(leddrv_keybl_key_press);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */

static void get_cfgtbl(void)
{
	int ret = 0;
	int loop = 0;
#if 1 //RUPY_add start
	int loop_maxdata = 0; /*Max電流値取得用*/
#endif //RUPY_add end

	LED_LOG("\n");
	for (loop = 0; loop < LED_ENUM_MAX; loop++) {
		ret = cfgdrv_read(led_table[loop].id, led_table[loop].size,
				led_table[loop].table);
		if (ret != 0) {
/*			LED_ERRLOG("Can't cfg read number=%d, ret=%d\n",loop, ret);*/
			break;
		}
#ifdef LED_LOG_ENABLE
		{
			int loop_log = 0;
			
			LED_LOG("cfgdrv_read id=0x%x size=0x%x\n", led_table[loop].id, led_table[loop].size);
			for (loop_log = 0; loop_log < led_table[loop].size; loop_log++) {
				LED_LOG("data %2d=0x%x\n", loop_log, led_table[loop].table[loop_log]);
			}
		}
#endif
	}

	
#if 1 //RUPY_chg start
	/*不揮発からMax電流値を取得*/
	for (loop_maxdata = 0; loop_maxdata < (LEDDRV_MODE_NUM - 1) ; loop_maxdata++) {
		ret = cfgdrv_read(led_maxtable[loop_maxdata].id, led_maxtable[loop_maxdata].size,
				led_maxtable[loop_maxdata].table);
		if (ret != 0) {
			break;
		}
	}

	if ((loop == LED_ENUM_MAX) && (loop_maxdata == (LEDDRV_MODE_NUM - 1))) {
#else //RUPY_chg
//	if (loop == LED_ENUM_MAX){
#endif //RUPY_chg end
		set_cfgtable = 1;

		lcdbl_status_allmode_set();

		LED_LOG( "##### NORM: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					led_lcdbl_normal_table[0],led_lcdbl_normal_table[1],led_lcdbl_normal_table[2],led_lcdbl_normal_table[3],led_lcdbl_normal_table[4],led_lcdbl_normal_table[5],led_lcdbl_normal_table[6],led_lcdbl_normal_table[7],led_lcdbl_normal_table[8],led_lcdbl_normal_table[9],led_lcdbl_normal_table[10],led_lcdbl_normal_table[11],led_lcdbl_normal_table[12],led_lcdbl_normal_table[13],led_lcdbl_normal_table[14]);
		LED_LOG( "##### ECO : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					led_lcdbl_ecomode_table[0],led_lcdbl_ecomode_table[1],led_lcdbl_ecomode_table[2],led_lcdbl_ecomode_table[3],led_lcdbl_ecomode_table[4],led_lcdbl_ecomode_table[5],led_lcdbl_ecomode_table[6],led_lcdbl_ecomode_table[7],led_lcdbl_ecomode_table[8],led_lcdbl_ecomode_table[9],led_lcdbl_ecomode_table[10],led_lcdbl_ecomode_table[11],led_lcdbl_ecomode_table[12],led_lcdbl_ecomode_table[13],led_lcdbl_ecomode_table[14]);
		LED_LOG( "##### HIGH: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					led_lcdbl_highbright_table[0],led_lcdbl_highbright_table[1],led_lcdbl_highbright_table[2],led_lcdbl_highbright_table[3],led_lcdbl_highbright_table[4],led_lcdbl_highbright_table[5],led_lcdbl_highbright_table[6],led_lcdbl_highbright_table[7],led_lcdbl_highbright_table[8],led_lcdbl_highbright_table[9],led_lcdbl_highbright_table[10],led_lcdbl_highbright_table[11],led_lcdbl_highbright_table[12],led_lcdbl_highbright_table[13],led_lcdbl_highbright_table[14]);
		LED_LOG( "##### POW : %d\n", led_lcdbl_powermode_table[0]);


		LED_LOG( "##### PWM : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					led_rgbpwm_table[0],led_rgbpwm_table[1],led_rgbpwm_table[2],led_rgbpwm_table[3],led_rgbpwm_table[4],led_rgbpwm_table[5],led_rgbpwm_table[6],led_rgbpwm_table[7],led_rgbpwm_table[8],led_rgbpwm_table[9],led_rgbpwm_table[10],led_rgbpwm_table[11],led_rgbpwm_table[12],led_rgbpwm_table[13],led_rgbpwm_table[14],led_rgbpwm_table[15],led_rgbpwm_table[16],led_rgbpwm_table[17]);
		
		for(loop=0;loop<(LCDBL_TABLE_LEN/2);loop++){
			LED_LOG( "##### Amp %3d: %3d %3d %3d   %3d: %3d %3d %3d\n",
						loop, led_lcdbl_normal_status[loop],led_lcdbl_ecomode_status[loop], led_lcdbl_highbright_status[loop],
						(loop+128), led_lcdbl_normal_status[(loop+128)],led_lcdbl_ecomode_status[(loop+128)], led_lcdbl_highbright_status[(loop+128)] );
		}
	}

	LED_LOG("cfg read ALL complete\n");

	return;
}

static int try_cfgtbl(void)
{
	int loop = 0;

	LED_LOG("\n");

	while ((loop < RETRY) && (!set_cfgtable)) {
		get_cfgtbl();
		loop++;
	}

	if (!set_cfgtable) {
/*		LED_ERRLOG("cfgtable cannot read\n");*/
		return -EINVAL;
	}

	return 0;
}

/* mode set sysfs */
static ssize_t leddrv_mode_show(struct device *dev,/* mrc24602 */
		struct device_attribute *attr, char *buf)/* mrc20101 *//* mrc24602 */
{
	unsigned long state = mode_status;
	int ret = 0;
	
	if (buf == NULL) {
		LED_ERRLOG("NULL error\n");
		return -1;
	}

	LED_LOG("\n");

	ret = sprintf(buf, "%d\n", (int) state);
	if (ret != (int)strlen(buf)) {
		LED_ERRLOG("error\n");
		return -EIO;
	}
	return ret;
}

static ssize_t leddrv_mode_store(struct device *dev,/* mrc24602 */
		struct device_attribute *attr, const char *buf, size_t size)/* mrc20101 *//* mrc24602 */
{
	char *after;/* mrc20101 */
	unsigned long state = simple_strtoul(buf, &after, 10);
	int ret;
	int ampere;

	LED_LOG("state=0x%02x\n",(int)state);

	ret = leddrv_lcdbl_set_mode(state);
	if (ret != 0) {
		LED_ERRLOG("mode_set error\n");
		return -EINVAL;
	}

	if( mode_status == LEDDRV_POWER_MODE ) {
		ampere = (int)led_lcdbl_powermode_status[0];
	} else {
		ampere = (int)led_lcdbl_mode_status[mode_status][lcdbl_status_struct->brightness];
	}
	if(lcdbl_change != NULL) {
		lcdbl_change(ampere, mode_status);
	}
	return size;
}
static DEVICE_ATTR(mode_set, 0644, leddrv_mode_show, leddrv_mode_store);/* mrc22102 *//* mrc21401 *//* mrc20801 *//* mrc24301 */

/* LCD Backlight LED */
static void lcdbl_status_set(u8 table[], u8 status[])
{
	int count  = 0;
	int LvCnt  = 1;
	int data   = 0;
	int Curr   = 0;
	int ctrl   = 0;
	int Lv[12] = { 0, 20, 30, 55, 80, 105, 130, 155, 180, 205, 230, 255 };

	LED_LOG("\n");

	if ((table == NULL)||(status == NULL)) { /* mrc30301 */
		LED_ERRLOG("NULL error\n");
		return;
	}

	status[0] = 0;
	for (count = 1; count < LCDBL_TABLE_LEN; count++) {

		/*** 計算式 *************************************************************************************/
		/* 			  輝度制御値(n+1)-輝度制御値(n)														*/
		/* Current=────────────────×(輝度設定値(Ⅹ)－輝度設定値(n+1))＋輝度制御値(n+1)	*/
		/* 			  輝度設定値(n+1)-輝度設定値(n)														*/
		/************************************************************************************************/

		/* 輝度制御値(n+1)-輝度制御値(n) */
		if ( LvCnt == 1 ) {
			/* Lv0(微灯)で計算する場合	*/
			ctrl = (table[LCDBL_TABLE_BASE - LvCnt + 2] - table[LCDBL_TABLE_BASE - LvCnt + 2]);
		} else {
		/* Lv1以降で計算する場合		*/
			ctrl = (table[LCDBL_TABLE_BASE - LvCnt + 2] - table[LCDBL_TABLE_BASE - LvCnt + 3]);
		}

		/* Current[%]/電流設定値の演算	*/
		Curr = (((count-Lv[LvCnt])*ctrl)/(Lv[LvCnt]-Lv[LvCnt-1])) + table[LCDBL_TABLE_BASE-LvCnt+2];
		data = (((Curr*LCDBL_DCDC_MAX)+50)/100);

		/* 範囲外の場合は最大値とする	*/
		if (data > MAX_BACKLIGHT_BRIGHTNESS) {
			status[count] = MAX_BACKLIGHT_BRIGHTNESS;
		} else {
			status[count] = data;
		}

		/* Lv切り替えタイミングの場合	*/
		if (Lv[LvCnt] == count) {
			LvCnt++;
		}
	}
}

static void lcdbl_status_allmode_set(void)
{
	lcdbl_status_set(led_lcdbl_normal_table, led_lcdbl_normal_status);
	lcdbl_status_set(led_lcdbl_ecomode_table, led_lcdbl_ecomode_status);
	lcdbl_status_set(led_lcdbl_highbright_table, led_lcdbl_highbright_status);
#if 1 //npdc300052586 start
	/*MAX_BACKLIGHT_BRIGHTNESS(255)×パワーモード輝度制御値(%)/100 = パワーモード時の電流値(0～255)*/
	led_lcdbl_powermode_status[0] = (( MAX_BACKLIGHT_BRIGHTNESS * led_lcdbl_powermode_table[0])/ 100) ;
	led_lcdbl_dcdc[LEDDRV_POWER_MODE][0] = led_lcdbl_highbright_table[LCDBL_TABLE_DCDC];
#else
	led_lcdbl_powermode_status[0] = led_lcdbl_powermode_table[0];
#endif //npdc300052586 end

#if 0 //RUPY_del start
	/* 不揮発から取得したDCDC電流値を保持	*/
	led_lcdbl_dcdc[LEDDRV_NORMAL_MODE]	   = led_lcdbl_normal_table[LCDBL_TABLE_DCDC];
	led_lcdbl_dcdc[LEDDRV_ECO_MODE]		   = led_lcdbl_ecomode_table[LCDBL_TABLE_DCDC];
	led_lcdbl_dcdc[LEDDRV_HIGHBRIGHT_MODE] = led_lcdbl_highbright_table[LCDBL_TABLE_DCDC];
/*	led_lcdbl_dcdc[LEDDRV_POWER_MODE]	   = led_lcdbl_powermode_table[LCDBL_TABLE_DCDC];*/
#endif //RUPY_del end
}

static int leddrv_lcdbl_no_handler(int ampere, enum leddrv_lcdbl_mode_status status)/* mrc24602 */
{
	LED_LOG("ampere=%d\tmode=%d\n",ampere,status);

	leddrv_lcdbl_set_ampere(ampere);
	return 0;
}

static void leddrv_static_init(void)
{
	mutex_init(&handler_locked);/* mrc24004 */

	red_delay_on  = 0;
	red_delay_off = 0;
	set_cfgtable  = 0;
}

static void pm8xxx_led_func_lcdbl(struct led_classdev *led)
{
	int ampere;

	LED_LOG("\n");

	if (led == NULL) {
		LED_ERRLOG("NULL error\n");
		return ;
	}

	if( mode_status == LEDDRV_POWER_MODE ) {
		if( led->brightness == 0 ) {
			LED_LOG( "Forced Power Mode OFF\n" );
			leddrv_lcdbl_set_mode( LEDDRV_POWER_MODE_OFF );
			if( mode_status == LEDDRV_HIGHBRIGHT_MODE ) {
				LED_LOG( "Forced HighBright Mode OFF\n" );
				leddrv_lcdbl_set_mode( LEDDRV_HIGHBRIGHT_MODE_OFF );
			}
			ampere = (int) led_lcdbl_mode_status[mode_status][led->brightness];
		} else {
			ampere = led_lcdbl_powermode_status[0];
		}
	} else if( mode_status == LEDDRV_HIGHBRIGHT_MODE ) {
		if( led->brightness == 0 ) {
			LED_LOG( "Forced HighBright Mode OFF\n" );
			leddrv_lcdbl_set_mode( LEDDRV_HIGHBRIGHT_MODE_OFF );
		}
		ampere = (int) led_lcdbl_mode_status[mode_status][led->brightness];
	} else {
		ampere = (int) led_lcdbl_mode_status[mode_status][led->brightness];
	}
	if(lcdbl_change != NULL) {
		lcdbl_change(ampere, mode_status);
	}
}

static int pm8xxx_blink_set(struct led_classdev *led_cdev,
		unsigned long *delay_on, unsigned long *delay_off)
{
	struct pm8xxx_led_data *led = container_of(led_cdev, struct pm8xxx_led_data, cdev);/* mrc23704 *//* mrc60302 */

	if (((delay_on == NULL)||(delay_off == NULL))||(led_cdev == NULL)) {
		LED_ERRLOG("NULL error\n");
		return -1;
	}

	LED_LOG("id=%d delay_on=%d delay_off=%d\n", led->id, (int)*delay_on, (int)*delay_off);

	switch (led->id) {
	case PM8XXX_ID_LED_0:	/* RED		*/
		break;
	case PM8XXX_ID_LED_1:	/* GREEN	*/
	case PM8XXX_ID_LED_2:	/* BLUE		*/
		return 0;
	default:
		LED_ERRLOG("Unknown ID\n");
		return -1;
	}

	if (*delay_on != red_delay_on) {
		red_delay_on = *delay_on;
		schedule_work(&led->work);
	}
	if (*delay_off != red_delay_off) {
		red_delay_off = *delay_off;
		schedule_work(&led->work);
	}

	return 0;
}
#endif /*CONFIG_LEDS_PM8960*/

static void led_kp_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;

	level = (value << PM8XXX_DRV_KEYPAD_BL_SHIFT) &
				 PM8XXX_DRV_KEYPAD_BL_MASK;

	led->reg &= ~PM8XXX_DRV_KEYPAD_BL_MASK;
	led->reg |= level;

	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_DRV_KEYPAD,
								led->reg);
	if (rc < 0)
		dev_err(led->cdev.dev,
			"can't set keypad backlight level rc=%d\n", rc);
}

static void led_lc_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc, offset;
	u8 level;

	level = (value << PM8XXX_DRV_LED_CTRL_SHIFT) &
				PM8XXX_DRV_LED_CTRL_MASK;

	offset = PM8XXX_LED_OFFSET(led->id);

	led->reg &= ~PM8XXX_DRV_LED_CTRL_MASK;
	led->reg |= level;

	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset),
								led->reg);
	if (rc)
		dev_err(led->cdev.dev, "can't set (%d) led value rc=%d\n",
				led->id, rc);
}

static void
led_flash_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;
	u16 reg_addr;

	level = (value << PM8XXX_DRV_FLASH_SHIFT) &
				 PM8XXX_DRV_FLASH_MASK;

	led->reg &= ~PM8XXX_DRV_FLASH_MASK;
	led->reg |= level;

	if (led->id == PM8XXX_ID_FLASH_LED_0)
		reg_addr = SSBI_REG_ADDR_FLASH_DRV0;
	else
		reg_addr = SSBI_REG_ADDR_FLASH_DRV1;

	rc = pm8xxx_writeb(led->dev->parent, reg_addr, led->reg);
	if (rc < 0)
		dev_err(led->cdev.dev, "can't set flash led%d level rc=%d\n",
			 led->id, rc);
}

static int
led_wled_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc, duty;
	u8 val, i, num_wled_strings;

	if (value > WLED_MAX_LEVEL)
		value = WLED_MAX_LEVEL;

	if (value == 0) {
		rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG,
				WLED_BOOST_OFF);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled ctrl config"
				" register rc=%d\n", rc);
			return rc;
		}
	} else {
		rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG,
				led->wled_mod_ctrl_val);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled ctrl config"
				" register rc=%d\n", rc);
			return rc;
		}
	}

	duty = (WLED_MAX_DUTY_CYCLE * value) / WLED_MAX_LEVEL;

	num_wled_strings = led->wled_cfg->num_strings;

	/* program brightness control registers */
	for (i = 0; i < num_wled_strings; i++) {
		rc = pm8xxx_readb(led->dev->parent,
				WLED_BRIGHTNESS_CNTL_REG1(i), &val);
		if (rc) {
			dev_err(led->dev->parent, "can't read wled brightnes ctrl"
				" register1 rc=%d\n", rc);
			return rc;
		}

		val = (val & ~WLED_MAX_CURR_MASK) | (duty >> WLED_8_BIT_SHFT);
		rc = pm8xxx_writeb(led->dev->parent,
				WLED_BRIGHTNESS_CNTL_REG1(i), val);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled brightness ctrl"
				" register1 rc=%d\n", rc);
			return rc;
		}

		val = duty & WLED_8_BIT_MASK;
		rc = pm8xxx_writeb(led->dev->parent,
				WLED_BRIGHTNESS_CNTL_REG2(i), val);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled brightness ctrl"
				" register2 rc=%d\n", rc);
			return rc;
		}
	}

	/* sync */
	val = WLED_SYNC_VAL;
	rc = pm8xxx_writeb(led->dev->parent, WLED_SYNC_REG, val);
	if (rc) {
		dev_err(led->dev->parent,
			"can't read wled sync register rc=%d\n", rc);
		return rc;
	}

	val = WLED_SYNC_RESET_VAL;
	rc = pm8xxx_writeb(led->dev->parent, WLED_SYNC_REG, val);
	if (rc) {
		dev_err(led->dev->parent,
			"can't read wled sync register rc=%d\n", rc);
		return rc;
	}
	return 0;
}

static void wled_dump_regs(struct pm8xxx_led_data *led)
{
	int i;
	u8 val;

	for (i = 1; i < 17; i++) {
		pm8xxx_readb(led->dev->parent,
				SSBI_REG_ADDR_WLED_CTRL(i), &val);
		pr_debug("WLED_CTRL_%d = 0x%x\n", i, val);
	}
}

static void
led_rgb_write(struct pm8xxx_led_data *led, u16 addr, enum led_brightness value)
{
	int rc;
	u8 val, mask;

	if (led->id != PM8XXX_ID_RGB_LED_BLUE &&
		led->id != PM8XXX_ID_RGB_LED_RED &&
		led->id != PM8XXX_ID_RGB_LED_GREEN)
		return;

	rc = pm8xxx_readb(led->dev->parent, addr, &val);
	if (rc) {
		dev_err(led->cdev.dev, "can't read rgb ctrl register rc=%d\n",
							rc);
		return;
	}

	switch (led->id) {
	case PM8XXX_ID_RGB_LED_RED:
		mask = PM8XXX_DRV_RGB_RED_LED;
		break;
	case PM8XXX_ID_RGB_LED_GREEN:
		mask = PM8XXX_DRV_RGB_GREEN_LED;
		break;
	case PM8XXX_ID_RGB_LED_BLUE:
		mask = PM8XXX_DRV_RGB_BLUE_LED;
		break;
	default:
		return;
	}

	if (value)
		val |= mask;
	else
		val &= ~mask;

	rc = pm8xxx_writeb(led->dev->parent, addr, val);
	if (rc < 0)
		dev_err(led->cdev.dev, "can't set rgb led %d level rc=%d\n",
			 led->id, rc);
}

static void
led_rgb_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	if (value) {
		led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL1, value);
		led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL2, value);
	} else {
		led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL2, value);
		led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL1, value);
	}
}

static int pm8xxx_led_pwm_work(struct pm8xxx_led_data *led)
{
	int duty_us;
	int rc = 0;

#ifdef CONFIG_LEDS_PM8960
	int loop;
	int on_num;
	int off_num;
	int led_num=0;

	if (led->id == PM8XXX_ID_LED_0) {
		/* 値を保持する	*/
		led_keep[0] = led;
		led_num     = LED_RED;
	} else if (led->id == PM8XXX_ID_LED_1) {
		/* 値を保持する	*/
		led_keep[1] = led;
		led_num     = LED_GREEN;
// <SlimAV1> add S
	} else if (led->id == PM8XXX_ID_LED_2) {
		/* 値を保持する	*/
		led_keep[2] = led;
		led_num     = LED_BLUE;
// <SlimAV1> add E
	} else if (led->id == PM8XXX_ID_COLOR) {
		
		for(loop=0;loop<PM8XXX_LED_NUM;loop++) {
			/* 設定データがなければ未実施	*/
			if( led_keep[loop] == NULL ) {
				continue;
			}
			
			if (led_keep[loop]->pwm_duty_cycles != NULL) {
                /*LED消灯*/
				pwm_disable(led_keep[loop]->pwm_dev);
			}
		}
		msleep(5);
		
		for(loop=0;loop<PM8XXX_LED_NUM;loop++) {
			/* 設定データがなければ未実施	*/
			if( led_keep[loop] == NULL ) {
				continue;
			}

			/* 消灯指定なら"brightness"を0にする	*/
			if( led->cdev.brightness == 0 ) {
				led_keep[loop]->cdev.brightness = 0;
			}
			/* pwm_duty_cycles指定の有無で処理を分岐	*/
			if (led_keep[loop]->pwm_duty_cycles == NULL) {
				duty_us = (led_keep[loop]->pwm_period_us * led_keep[loop]->cdev.brightness)/LED_FULL;
				rc = pwm_config(led_keep[loop]->pwm_dev, duty_us, led_keep[loop]->pwm_period_us);
				if (led_keep[loop]->cdev.brightness) {
					rc = pwm_enable(led_keep[loop]->pwm_dev);
				} else {
					pwm_disable(led_keep[loop]->pwm_dev);
				}
			} else {
//				pwm_disable(led_keep[loop]->pwm_dev);

				rc = pm8xxx_pwm_lut_config( led_keep[loop]->pwm_dev,
											led_keep[loop]->pwm_period_us,
											led_keep[loop]->pwm_duty_cycles->duty_pcts,
											led_keep[loop]->pwm_duty_cycles->duty_ms,
											led_keep[loop]->pwm_duty_cycles->start_idx,
											led_keep[loop]->pwm_duty_cycles->num_duty_pcts,
											0, 0, PM8XXX_LED_PWM_FLAGS );
//				rc = pm8xxx_pwm_lut_enable(led_keep[loop]->pwm_dev, led_keep[loop]->cdev.brightness);
			}
		}
		msleep(5);
		for(loop=0;loop<PM8XXX_LED_NUM;loop++) {
			/* 設定データがなければ未実施	*/
			if( led_keep[loop] == NULL ) {
				continue;
			}
			
			if (led_keep[loop]->pwm_duty_cycles != NULL) {
				rc = pm8xxx_pwm_lut_enable(led_keep[loop]->pwm_dev, led_keep[loop]->cdev.brightness);
				if(rc < 0 )
				{
					pr_err("pm8xxx_pwm_lut_enable failed %d\n", rc);
				}
			}
		}
	}

	/* 点滅設定	*/
// <SlimAV1> mod S
#if 0
	if ( (led->id == PM8XXX_ID_LED_0) || (led->id == PM8XXX_ID_LED_1)) {
#else
	if ( (led->id == PM8XXX_ID_LED_0) || (led->id == PM8XXX_ID_LED_1) || (led->id == PM8XXX_ID_LED_2)) {
#endif
// <SlimAV1> mod E
		/* delay_onもdelay_offも設定済みでかつ輝度が0以外ならテーブル作成＆登録	*/
		if ((red_delay_on != 0) && (red_delay_off != 0) && (led->cdev.brightness != 0)) {
			on_num  = (red_delay_on  / PM8960_PWM_TIME_UNIT);
			off_num = (red_delay_off / PM8960_PWM_TIME_UNIT);

			/* ON時間指定が500ms未満の場合は500msとする */
			if( on_num  < PM8960_PWM_MIN_TIME ) {
				on_num  = PM8960_PWM_MIN_TIME;
			}
			/* ON時間指定が8secを超える場合は8secとする */
			else if( on_num  > PM8960_PWM_MAX_TIME ) {
				on_num  = PM8960_PWM_MAX_TIME;
			}
			/* OFF時間指定が500ms未満の場合は500msとする */
			if( off_num < PM8960_PWM_MIN_TIME ) {
				off_num = PM8960_PWM_MIN_TIME;
			}
			/* OFF時間指定が8secを超える場合は8secとする */
			else if( off_num > PM8960_PWM_MAX_TIME ) {
				off_num = PM8960_PWM_MAX_TIME;
			}

			/* エリア初期化	*/
			memset(&pm8921_redled_pwm_duty_pcts[0], 0, sizeof(pm8921_redled_pwm_duty_pcts));

			/* 点灯設定		*/
			for(loop=0;loop<on_num;loop++){
				pm8921_redled_pwm_duty_pcts[loop] = ((led_keep[led_num]->cdev.brightness*100)/LED_FULL);
			}
			/* 消灯設定		*/
			for(loop=on_num;loop<(on_num+off_num);loop++){
				pm8921_redled_pwm_duty_pcts[loop] = 0;
			}

			/* 点滅設定		*/
			pm8921_led0_pwm_duty_cycles.duty_pcts     = (int *)&pm8921_redled_pwm_duty_pcts;
			pm8921_led0_pwm_duty_cycles.num_duty_pcts = (on_num+off_num);

			led_keep[led_num]->pwm_duty_cycles = &pm8921_led0_pwm_duty_cycles;
		} else {
			led_keep[led_num]->pwm_duty_cycles = NULL;
		}
	}
#else  /*CONFIG_LEDS_PM8960*/
	if (led->pwm_duty_cycles == NULL) {
		duty_us = (led->pwm_period_us * led->cdev.brightness) /
								LED_FULL;
		rc = pwm_config(led->pwm_dev, duty_us, led->pwm_period_us);
		if (led->cdev.brightness) {
			led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL1,
				led->cdev.brightness);
			rc = pwm_enable(led->pwm_dev);
		} else {
			pwm_disable(led->pwm_dev);
			led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL1,
				led->cdev.brightness);
		}
	} else {
		if (led->cdev.brightness)
			led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL1,
				led->cdev.brightness);
		rc = pm8xxx_pwm_lut_enable(led->pwm_dev, led->cdev.brightness);
		if (!led->cdev.brightness)
			led_rgb_write(led, SSBI_REG_ADDR_RGB_CNTL1,
				led->cdev.brightness);
	}
#endif /*CONFIG_LEDS_PM8960*/

	return rc;
}

static void __pm8xxx_led_work(struct pm8xxx_led_data *led,
					enum led_brightness level)
{
	int rc;

	mutex_lock(&led->lock);

	switch (led->id) {
	case PM8XXX_ID_LED_KB_LIGHT:
		led_kp_set(led, level);
		break;
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		led_lc_set(led, level);
		break;
	case PM8XXX_ID_FLASH_LED_0:
	case PM8XXX_ID_FLASH_LED_1:
		led_flash_set(led, level);
		break;
	case PM8XXX_ID_WLED:
		rc = led_wled_set(led, level);
		if (rc < 0)
			pr_err("wled brightness set failed %d\n", rc);
		break;
	case PM8XXX_ID_RGB_LED_RED:
	case PM8XXX_ID_RGB_LED_GREEN:
	case PM8XXX_ID_RGB_LED_BLUE:
		led_rgb_set(led, level);
		break;
	default:
		dev_err(led->cdev.dev, "unknown led id %d", led->id);
		break;
	}

	mutex_unlock(&led->lock);
}

static void pm8xxx_led_work(struct work_struct *work)
{
	int rc;

	struct pm8xxx_led_data *led = container_of(work,
					 struct pm8xxx_led_data, work);

	if (led->pwm_dev == NULL) {
#ifdef CONFIG_LEDS_PM8960
		if(led->id == PM8XXX_ID_COLOR) {
			rc = pm8xxx_led_pwm_work(led);
			if (rc) {
				pr_err("could not configure PWM mode for LED:%d\n", led->id);
			}
		} else {
			__pm8xxx_led_work(led, led->cdev.brightness);
		}
#else /*CONFIG_LEDS_PM8960*/
		__pm8xxx_led_work(led, led->cdev.brightness);
#endif /*CONFIG_LEDS_PM8960*/
	} else {
		rc = pm8xxx_led_pwm_work(led);
		if (rc)
			pr_err("could not configure PWM mode for LED:%d\n",
								led->id);
	}
}

static void pm8xxx_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct	pm8xxx_led_data *led;
#ifdef CONFIG_LEDS_PM8960
	int		rc   = 0;
	int		mode = 0;
	int		max_current = 0;
#endif /*CONFIG_LEDS_PM8960*/

	led = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	if (value < LED_OFF || value > led->cdev.max_brightness) {
		dev_err(led->cdev.dev, "Invalid brightness value exceeds");
		return;
	}

	led->cdev.brightness = value;

#ifdef CONFIG_LEDS_PM8960
	led->new_ampere = (u8)led->new_brightness;

	if (!set_cfgtable) {
		rc = try_cfgtbl();
		if (rc) {
			lcdbl_status_allmode_set();
/*			LED_ERRLOG("cfg_table error\n");*/
			return;
		}
	} else {
		/* 不揮発の読み出しが完了した場合のみ処理	*/
		if ( (strcmp(led->cdev.name , LED_NAME_RED) == 0) && !(led_setting[0]) ) {
			led_setting[0]     =  1;
			led->id            =  led_rgbpwm_table[0];
			mode               =  led_rgbpwm_table[1];
			max_current        =  led_rgbpwm_table[2];
			led->pwm_channel   =  led_rgbpwm_table[3];
			led->pwm_period_us = (led_rgbpwm_table[4] | (led_rgbpwm_table[5]<<8) );

			rc = pm8xxx_set_led_mode_and_max_brightness(led, mode, max_current);

			__pm8xxx_led_work(led, led->cdev.max_brightness);

			/* max_brightness補正	*/
			led->cdev.max_brightness = LED_FULL;

		} else if ( (strcmp(led->cdev.name , LED_NAME_GREEN) == 0) && !(led_setting[1]) ) {
			led_setting[1]     =  1;
			led->id            =  led_rgbpwm_table[6];
			mode               =  led_rgbpwm_table[7];
			max_current        =  led_rgbpwm_table[8];
			led->pwm_channel   =  led_rgbpwm_table[9];
			led->pwm_period_us = (led_rgbpwm_table[10] | (led_rgbpwm_table[11]<<8) );

			rc = pm8xxx_set_led_mode_and_max_brightness(led, mode, max_current);

			__pm8xxx_led_work(led, led->cdev.max_brightness);

			/* max_brightness補正	*/
			led->cdev.max_brightness = LED_FULL;
// <SlimAV1> add S
		} else if ( (strcmp(led->cdev.name , LED_NAME_BLUE) == 0) && !(led_setting[2]) ) {
			led_setting[2]     =  1;
			led->id            =  led_rgbpwm_table[12];
			mode               =  led_rgbpwm_table[13];
			max_current        =  led_rgbpwm_table[14];
			led->pwm_channel   =  led_rgbpwm_table[15];
			led->pwm_period_us = (led_rgbpwm_table[16] | (led_rgbpwm_table[17]<<8) );

			rc = pm8xxx_set_led_mode_and_max_brightness(led, mode, max_current);

			__pm8xxx_led_work(led, led->cdev.max_brightness);

			/* max_brightness補正	*/
			led->cdev.max_brightness = LED_FULL;
		}
// <SlimAV1> add E

		if (rc) {
			LED_ERRLOG("pm8xxx_set_led_mode_and_max_brightness error rc=%d\n", rc);
			return;
		}
	}
#endif /*CONFIG_LEDS_PM8960*/

	schedule_work(&led->work);
}

static int pm8xxx_set_led_mode_and_max_brightness(struct pm8xxx_led_data *led,
		enum pm8xxx_led_modes led_mode, int max_current)
{
	switch (led->id) {
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		led->cdev.max_brightness = max_current /
						PM8XXX_ID_LED_CURRENT_FACTOR;
		if (led->cdev.max_brightness > MAX_LC_LED_BRIGHTNESS)
			led->cdev.max_brightness = MAX_LC_LED_BRIGHTNESS;
		led->reg = led_mode;
		break;
	case PM8XXX_ID_LED_KB_LIGHT:
	case PM8XXX_ID_FLASH_LED_0:
	case PM8XXX_ID_FLASH_LED_1:
		led->cdev.max_brightness = max_current /
						PM8XXX_ID_FLASH_CURRENT_FACTOR;
		if (led->cdev.max_brightness > MAX_FLASH_BRIGHTNESS)
			led->cdev.max_brightness = MAX_FLASH_BRIGHTNESS;

		switch (led_mode) {
		case PM8XXX_LED_MODE_PWM1:
		case PM8XXX_LED_MODE_PWM2:
		case PM8XXX_LED_MODE_PWM3:
			led->reg = PM8XXX_FLASH_MODE_PWM;
			break;
		case PM8XXX_LED_MODE_DTEST1:
			led->reg = PM8XXX_FLASH_MODE_DBUS1;
			break;
		case PM8XXX_LED_MODE_DTEST2:
			led->reg = PM8XXX_FLASH_MODE_DBUS2;
			break;
		default:
			led->reg = PM8XXX_LED_MODE_MANUAL;
			break;
		}
		break;
	case PM8XXX_ID_WLED:
		led->cdev.max_brightness = WLED_MAX_LEVEL;
		break;
	case PM8XXX_ID_RGB_LED_RED:
	case PM8XXX_ID_RGB_LED_GREEN:
	case PM8XXX_ID_RGB_LED_BLUE:
		led->cdev.max_brightness = LED_FULL;
		break;
#ifdef CONFIG_LEDS_PM8960
        case PM8XXX_ID_COLOR:
                led->reg = PM8XXX_LED_MODE_MANUAL;
                break;
#endif /*CONFIG_LEDS_PM8960*/
	default:
		dev_err(led->cdev.dev, "LED Id is invalid");
		return -EINVAL;
	}

	return 0;
}

static enum led_brightness pm8xxx_led_get(struct led_classdev *led_cdev)
{
	struct pm8xxx_led_data *led;

	led = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	return led->cdev.brightness;
}

static int __devinit init_wled(struct pm8xxx_led_data *led)
{
	int rc, i;
	u8 val, num_wled_strings;

	num_wled_strings = led->wled_cfg->num_strings;

	/* program over voltage protection threshold */
	if (led->wled_cfg->ovp_val > WLED_OVP_37V) {
		dev_err(led->dev->parent, "Invalid ovp value");
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, WLED_OVP_CFG_REG, &val);
	if (rc) {
		dev_err(led->dev->parent, "can't read wled ovp config"
			" register rc=%d\n", rc);
		return rc;
	}

	val = (val & ~WLED_OVP_VAL_MASK) |
		(led->wled_cfg->ovp_val << WLED_OVP_VAL_BIT_SHFT);

	rc = pm8xxx_writeb(led->dev->parent, WLED_OVP_CFG_REG, val);
	if (rc) {
		dev_err(led->dev->parent, "can't write wled ovp config"
			" register rc=%d\n", rc);
		return rc;
	}

	/* program current boost limit and output feedback*/
	if (led->wled_cfg->boost_curr_lim > WLED_CURR_LIMIT_1680mA) {
		dev_err(led->dev->parent, "Invalid boost current limit");
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, WLED_BOOST_CFG_REG, &val);
	if (rc) {
		dev_err(led->dev->parent, "can't read wled boost config"
			" register rc=%d\n", rc);
		return rc;
	}

	val = (val & ~WLED_BOOST_LIMIT_MASK) |
		(led->wled_cfg->boost_curr_lim << WLED_BOOST_LIMIT_BIT_SHFT);

	val = (val & ~WLED_OP_FDBCK_MASK) |
		(led->wled_cfg->op_fdbck << WLED_OP_FDBCK_BIT_SHFT);

	rc = pm8xxx_writeb(led->dev->parent, WLED_BOOST_CFG_REG, val);
	if (rc) {
		dev_err(led->dev->parent, "can't write wled boost config"
			" register rc=%d\n", rc);
		return rc;
	}

	/* program high pole capacitance */
	if (led->wled_cfg->cp_select > WLED_CP_SELECT_MAX) {
		dev_err(led->dev->parent, "Invalid pole capacitance");
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, WLED_HIGH_POLE_CAP_REG, &val);
	if (rc) {
		dev_err(led->dev->parent, "can't read wled high pole"
			" capacitance register rc=%d\n", rc);
		return rc;
	}

	val = (val & ~WLED_CP_SELECT_MASK) | led->wled_cfg->cp_select;

	rc = pm8xxx_writeb(led->dev->parent, WLED_HIGH_POLE_CAP_REG, val);
	if (rc) {
		dev_err(led->dev->parent, "can't write wled high pole"
			" capacitance register rc=%d\n", rc);
		return rc;
	}

	/* program activation delay and maximum current */
	for (i = 0; i < num_wled_strings; i++) {
		rc = pm8xxx_readb(led->dev->parent,
				WLED_MAX_CURR_CFG_REG(i + 2), &val);
		if (rc) {
			dev_err(led->dev->parent, "can't read wled max current"
				" config register rc=%d\n", rc);
			return rc;
		}

		if ((led->wled_cfg->ctrl_delay_us % WLED_CTL_DLY_STEP) ||
			(led->wled_cfg->ctrl_delay_us > WLED_CTL_DLY_MAX)) {
			dev_err(led->dev->parent, "Invalid control delay\n");
			return rc;
		}

		val = val / WLED_CTL_DLY_STEP;
		val = (val & ~WLED_CTL_DLY_MASK) |
			(led->wled_cfg->ctrl_delay_us << WLED_CTL_DLY_BIT_SHFT);

		if ((led->max_current > WLED_MAX_CURR)) {
			dev_err(led->dev->parent, "Invalid max current\n");
			return -EINVAL;
		}

		val = (val & ~WLED_MAX_CURR_MASK) | led->max_current;

		rc = pm8xxx_writeb(led->dev->parent,
				WLED_MAX_CURR_CFG_REG(i + 2), val);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled max current"
				" config register rc=%d\n", rc);
			return rc;
		}
	}

	/* program digital module generator, cs out and enable the module */
	rc = pm8xxx_readb(led->dev->parent, WLED_MOD_CTRL_REG, &val);
	if (rc) {
		dev_err(led->dev->parent, "can't read wled module ctrl"
			" register rc=%d\n", rc);
		return rc;
	}

	if (led->wled_cfg->dig_mod_gen_en)
		val |= WLED_DIG_MOD_GEN_MASK;

	if (led->wled_cfg->cs_out_en)
		val |= WLED_CS_OUT_MASK;

	val |= WLED_EN_MASK;

	rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG, val);
	if (rc) {
		dev_err(led->dev->parent, "can't write wled module ctrl"
			" register rc=%d\n", rc);
		return rc;
	}
	led->wled_mod_ctrl_val = val;

	/* dump wled registers */
	wled_dump_regs(led);

	return 0;
}

static int __devinit get_init_value(struct pm8xxx_led_data *led, u8 *val)
{
	int rc, offset;
	u16 addr;

	switch (led->id) {
	case PM8XXX_ID_LED_KB_LIGHT:
		addr = SSBI_REG_ADDR_DRV_KEYPAD;
		break;
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		offset = PM8XXX_LED_OFFSET(led->id);
		addr = SSBI_REG_ADDR_LED_CTRL(offset);
		break;
#ifdef CONFIG_LEDS_PM8960
	case PM8XXX_ID_COLOR:
		return 1;
#endif	/* CONFIG_LEDS_PM8960 */
	case PM8XXX_ID_FLASH_LED_0:
		addr = SSBI_REG_ADDR_FLASH_DRV0;
		break;
	case PM8XXX_ID_FLASH_LED_1:
		addr = SSBI_REG_ADDR_FLASH_DRV1;
		break;
	case PM8XXX_ID_WLED:
		rc = init_wled(led);
		if (rc)
			dev_err(led->cdev.dev, "can't initialize wled rc=%d\n",
								rc);
		return rc;
	case PM8XXX_ID_RGB_LED_RED:
	case PM8XXX_ID_RGB_LED_GREEN:
	case PM8XXX_ID_RGB_LED_BLUE:
		addr = SSBI_REG_ADDR_RGB_CNTL1;
		break;
	default:
		dev_err(led->cdev.dev, "unknown led id %d", led->id);
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, addr, val);
	if (rc)
		dev_err(led->cdev.dev, "can't get led(%d) level rc=%d\n",
							led->id, rc);

	return rc;
}

static int pm8xxx_led_pwm_configure(struct pm8xxx_led_data *led)
{
	int start_idx, idx_len, duty_us, rc;

	led->pwm_dev = pwm_request(led->pwm_channel,
					led->cdev.name);

	if (IS_ERR_OR_NULL(led->pwm_dev)) {
		pr_err("could not acquire PWM Channel %d, "
			"error %ld\n", led->pwm_channel,
			PTR_ERR(led->pwm_dev));
		led->pwm_dev = NULL;
		return -ENODEV;
	}

	if (led->pwm_duty_cycles != NULL) {
		start_idx = led->pwm_duty_cycles->start_idx;
		idx_len = led->pwm_duty_cycles->num_duty_pcts;

		if (idx_len >= PM_PWM_LUT_SIZE && start_idx) {
			pr_err("Wrong LUT size or index\n");
			return -EINVAL;
		}
		if ((start_idx + idx_len) > PM_PWM_LUT_SIZE) {
			pr_err("Exceed LUT limit\n");
			return -EINVAL;
		}

		rc = pm8xxx_pwm_lut_config(led->pwm_dev, led->pwm_period_us,
				led->pwm_duty_cycles->duty_pcts,
				led->pwm_duty_cycles->duty_ms,
				start_idx, idx_len, 0, 0,
				PM8XXX_LED_PWM_FLAGS);
	} else {
		duty_us = led->pwm_period_us;
		rc = pwm_config(led->pwm_dev, duty_us, led->pwm_period_us);
	}

	return rc;
}

#ifdef CONFIG_LEDS_PM8960
static int lcd_backlight_registered;

static void pm8xxx_set_lcdbl_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	int	rc;
	
#if 1 //RUPY_add start
	int maxLV = 0;
#endif //RUPY_add end
	
	if (value > MAX_BACKLIGHT_BRIGHTNESS) {
		value = MAX_BACKLIGHT_BRIGHTNESS;
	}

	if (!set_cfgtable) {
		rc = try_cfgtbl();
		if (rc) {
			lcdbl_status_allmode_set();
			LED_ERRLOG("cfg_table error\n");
#if 0 /* IDP AlarmWatch: Tentative fix to enable backlight at initlogo. */
			return;
#endif
		}
	}

	if(lcdbl_status_struct != NULL) {
		/* I2CコマンドによるLCDバックライトON/OFF制御	*/
		if ((led_cdev->brightness == 0) && (lcdbl_i2c == 1)) {
			lcdbl_i2c = 0;
#if 1 //RUPY_add start
			oldmaxLV = LCDBLT_INITIAL_VAL;
#endif //RUPY_add end
			rc = pm8xxx_ledi2c_off();
#if 1 //RUPY_chg start
		} else if (led_cdev->brightness != 0) {
			lcdbl_i2c = 1;
			
			/*輝度レベル 微灯の場合*/
			if(lcdbl_status_struct->brightness < 20)
			{
				maxLV = 10;
			}
			/*輝度レベル 1の場合*/
			else if(lcdbl_status_struct->brightness < 55)
			{
				maxLV = 9;
			}
			/*輝度レベル 2の場合*/
			else if(lcdbl_status_struct->brightness < 80)
			{
				maxLV = 8;
			}
			/*輝度レベル 3の場合*/
			else if(lcdbl_status_struct->brightness < 105)
			{
				maxLV = 7;
			}
			/*輝度レベル 4の場合*/
			else if(lcdbl_status_struct->brightness < 130)
			{
				maxLV = 6;
			}
			/*輝度レベル 5の場合*/
			else if(lcdbl_status_struct->brightness < 155)
			{
				maxLV = 5;
			}
			/*輝度レベル 6の場合*/
			else if(lcdbl_status_struct->brightness < 180)
			{
				maxLV = 4;
			}
			/*輝度レベル 7の場合*/
			else if(lcdbl_status_struct->brightness < 205)
			{
				maxLV = 3;
			}
			/*輝度レベル 8の場合*/
			else if(lcdbl_status_struct->brightness < 230)
			{
				maxLV = 2;
			}
			/*輝度レベル 9の場合*/
			else if(lcdbl_status_struct->brightness < 255)
			{
				maxLV = 1;
			}
			/*輝度レベル 10の場合*/
			else
			{
				maxLV = 0;
			}

#if 1 //npdc300052586 start
			/*パワーモードの場合Max輝度値は1つだけ*/
			if(mode_status == LEDDRV_POWER_MODE)
			{
				maxLV = 0;
			}
#endif //npdc300052586 end
			
			if(maxLV != oldmaxLV)
			{
				oldmaxLV = maxLV;
				rc = pm8xxx_ledi2c_on(led_lcdbl_dcdc[mode_status][maxLV]);
			}
			
#else //RUPY_chg
		} else if ((led_cdev->brightness != 0) && (lcdbl_i2c == 0)) {
			lcdbl_i2c = 1;
			rc = pm8xxx_ledi2c_on(led_lcdbl_dcdc[mode_status]);
#endif //RUPY_chg end
		}
		pm8xxx_led_func_lcdbl(lcdbl_status_struct);
	}
}

static struct led_classdev backlight_led = {
	.name			= "lcd-backlight",
	.brightness		= MAX_BACKLIGHT_BRIGHTNESS,
	.brightness_set	= pm8xxx_set_lcdbl_brightness,
};
#endif /*CONFIG_LEDS_PM8960*/

static int __devinit pm8xxx_led_probe(struct platform_device *pdev)
{
	const struct pm8xxx_led_platform_data *pdata = pdev->dev.platform_data;
	const struct led_platform_data *pcore_data;
	struct led_info *curr_led;
	struct pm8xxx_led_data *led, *led_dat;
	struct pm8xxx_led_config *led_cfg;
	enum pm8xxx_version version;
	bool found = false;
	int rc, i, j;

	if (pdata == NULL) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	pcore_data = pdata->led_core;

	if (pcore_data->num_leds != pdata->num_configs) {
		dev_err(&pdev->dev, "#no. of led configs and #no. of led"
				"entries are not equal\n");
		return -EINVAL;
	}

	led = kcalloc(pcore_data->num_leds, sizeof(*led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&pdev->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < pcore_data->num_leds; i++) {
		curr_led	= &pcore_data->leds[i];
		led_dat		= &led[i];
		led_cfg		= &pdata->configs[i];

		led_dat->id     = led_cfg->id;
		led_dat->pwm_channel = led_cfg->pwm_channel;
		led_dat->pwm_period_us = led_cfg->pwm_period_us;
		led_dat->pwm_duty_cycles = led_cfg->pwm_duty_cycles;
		led_dat->wled_cfg = led_cfg->wled_cfg;
		led_dat->max_current = led_cfg->max_current;

		if (!((led_dat->id >= PM8XXX_ID_LED_KB_LIGHT) &&
				(led_dat->id < PM8XXX_ID_MAX))) {
			dev_err(&pdev->dev, "invalid LED ID(%d) specified\n",
				led_dat->id);
			rc = -EINVAL;
			goto fail_id_check;

		}

		found = false;
		version = pm8xxx_get_version(pdev->dev.parent);
		for (j = 0; j < ARRAY_SIZE(led_map); j++) {
			if (version == led_map[j].version
			&& (led_map[j].supported & (1 << led_dat->id))) {
				found = true;
				break;
			}
		}

		if (!found) {
			dev_err(&pdev->dev, "invalid LED ID(%d) specified\n",
				led_dat->id);
			rc = -EINVAL;
			goto fail_id_check;
		}

		led_dat->cdev.name		= curr_led->name;
		led_dat->cdev.default_trigger   = curr_led->default_trigger;
		led_dat->cdev.brightness_set    = pm8xxx_led_set;
		led_dat->cdev.brightness_get    = pm8xxx_led_get;
		led_dat->cdev.brightness	= LED_OFF;
		led_dat->cdev.flags		= curr_led->flags;
		led_dat->dev			= &pdev->dev;

		rc =  get_init_value(led_dat, &led_dat->reg);
		if (rc < 0)
			goto fail_id_check;

		rc = pm8xxx_set_led_mode_and_max_brightness(led_dat,
					led_cfg->mode, led_cfg->max_current);
		if (rc < 0)
			goto fail_id_check;

		mutex_init(&led_dat->lock);
		INIT_WORK(&led_dat->work, pm8xxx_led_work);

#ifdef CONFIG_LEDS_PM8960
// <SlimAV1> mod S
#if 0
		if((led_dat->id >= PM8XXX_ID_LED_0) && (led_dat->id <= PM8XXX_ID_LED_1)) {/* mrc22802 *//* mrc24001 */
#else
		if((led_dat->id >= PM8XXX_ID_LED_0) && (led_dat->id <= PM8XXX_ID_LED_2)) {
#endif
// <SlimAV1> mod E
			led_dat->cdev.blink_set = pm8xxx_blink_set;
		} else {
			led_dat->cdev.blink_set = NULL;
		}
#endif /*CONFIG_LEDS_PM8960*/

		rc = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (rc) {
			dev_err(&pdev->dev, "unable to register led %d,rc=%d\n",
						 led_dat->id, rc);
			goto fail_id_check;
		}

		/* configure default state */
		if (led_cfg->default_state)
			led->cdev.brightness = led_dat->cdev.max_brightness;
		else
			led->cdev.brightness = LED_OFF;

		if (led_cfg->mode != PM8XXX_LED_MODE_MANUAL) {
			if (led_dat->id == PM8XXX_ID_RGB_LED_RED ||
				led_dat->id == PM8XXX_ID_RGB_LED_GREEN ||
				led_dat->id == PM8XXX_ID_RGB_LED_BLUE)
				__pm8xxx_led_work(led_dat, 0);
			else
				__pm8xxx_led_work(led_dat,
					led_dat->cdev.max_brightness);

			if (led_dat->pwm_channel != -1) {
				led_dat->cdev.max_brightness = LED_FULL;
				rc = pm8xxx_led_pwm_configure(led_dat);
				if (rc) {
					dev_err(&pdev->dev, "failed to "
					"configure LED, error: %d\n", rc);
					goto fail_id_check;
				}
			schedule_work(&led->work);
			}
		} else {
			__pm8xxx_led_work(led_dat, led->cdev.brightness);
		}
	}

	platform_set_drvdata(pdev, led);

#ifdef CONFIG_LEDS_PM8960
	lcdbl_init = 1;

	/* android supports only one lcd-backlight/lcd for now */
	if (!lcd_backlight_registered) {
		lcdbl_status_struct = &backlight_led;

		if (led_classdev_register(&pdev->dev, &backlight_led)) {
			printk(KERN_ERR "led_classdev_register failed\n");
		} else {
			lcd_backlight_registered = 1;
		}

		rc = device_create_file(&pdev->dev, &dev_attr_mode_set);
		if (rc) {
			LED_ERRLOG("cannot create mode_set\n");
			led_classdev_unregister(&backlight_led);/* mrc30601 */
			kfree(led);/* mrc30601 */
			LED_ERRLOG("error\n");/* mrc30601 */
			return rc;
		}
	}

	if(lcdbl_status_struct != NULL) {
		mutex_lock(&handler_locked);
		if( lcdbl_change != &leddrv_lcdbl_no_handler ) {/* mrc40901 */
			lcdbl_change(lcdbl_status_struct->brightness, mode_status);
		}
		mutex_unlock(&handler_locked);
	}
#endif /*CONFIG_LEDS_PM8960*/

	return 0;

fail_id_check:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			mutex_destroy(&led[i].lock);
			led_classdev_unregister(&led[i].cdev);
			if (led[i].pwm_dev != NULL)
				pwm_free(led[i].pwm_dev);
		}
	}
	kfree(led);
	return rc;
}

static int __devexit pm8xxx_led_remove(struct platform_device *pdev)
{
	int i;
	const struct led_platform_data *pdata =
				pdev->dev.platform_data;
	struct pm8xxx_led_data *led = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		cancel_work_sync(&led[i].work);
		mutex_destroy(&led[i].lock);
		led_classdev_unregister(&led[i].cdev);
		if (led[i].pwm_dev != NULL)
			pwm_free(led[i].pwm_dev);
	}

#ifdef CONFIG_LEDS_PM8960
	if (lcd_backlight_registered) {
		lcd_backlight_registered = 0;
		led_classdev_unregister(&backlight_led);
	}
#endif /*CONFIG_LEDS_PM8960*/

	kfree(led);

	return 0;
}

static struct platform_driver pm8xxx_led_driver = {
	.probe		= pm8xxx_led_probe,
	.remove		= __devexit_p(pm8xxx_led_remove),
	.driver		= {
		.name	= PM8XXX_LEDS_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8xxx_led_init(void)
{
#ifdef CONFIG_LEDS_PM8960
    leddrv_static_init();
#endif /*CONFIG_LEDS_PM8960*/

	return platform_driver_register(&pm8xxx_led_driver);
}
subsys_initcall(pm8xxx_led_init);

static void __exit pm8xxx_led_exit(void)
{
	platform_driver_unregister(&pm8xxx_led_driver);
}
module_exit(pm8xxx_led_exit);

MODULE_DESCRIPTION("PM8XXX LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8xxx-led");
