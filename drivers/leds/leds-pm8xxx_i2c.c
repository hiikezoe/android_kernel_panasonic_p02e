/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/module.h> /* jb_VerUp ADD */

#include <linux/leds-pm8xxx.h>

#define DEVICE_NAME "pm8xxx-i2c"

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

#define HIGH	1
#define LOW		0

#if 1 /*RUPY_chg*/
static int gpio_37;
static int gpio_26;
#else
static int gpio_25;
#endif /*RUPY_chg*/

static struct i2c_client *I2cClient = NULL;
#if 1 /*RUPY_add*/
static struct mutex I2cMutexLock;
static int LedI2cOn = 0;
#endif /*RUPY_add*/

struct pm8xxx_i2c_addr_data{
	u8 addr;
	u8 data;
};

#if 1 /*RUPY_chg*/
static struct pm8xxx_i2c_addr_data LedWk_On[] = {
	{0x10, 0xD4},
	{0x11, 0xC0},
	{0x12, 0xC0},
	{0x13, 0x82},
	{0x14, 0xC2},
	{0x15, 0x82},
	{0x16, 0xF1},
	{0x18, 0xF3},
	{0x1A, 0xF1},
	{0x17, 0xF3},
	{0x19, 0x16},
	{0x1B, 0xF3},
	{0x1C, 0xFF}
};
static struct pm8xxx_i2c_addr_data 	LedWk_Off[] = {
	{0x1D, 0xF8}
};
static struct pm8xxx_i2c_addr_data 	Led_On[] = {
	{0x1D, 0xFA}
};
#else /*RUPY_chg*/
static struct pm8xxx_i2c_addr_data LedWk[] = {
	{ 0x10, 0xD4 },
	{ 0x11, 0xC0 },
	{ 0x12, 0xC0 },
	{ 0x13, 0x82 },
	{ 0x14, 0xC2 },
	{ 0x15, 0x82 },
	{ 0x16, 0xF1 },
	{ 0x18, 0xF3 },
	{ 0x1A, 0xF1 },
	{ 0x17, 0xF3 },
	{ 0x19, 0xF6 },
	{ 0x1B, 0xF3 },
	{ 0x1C, 0xFF },
	{ 0x1D, 0xFA },
};
#endif /*RUPY_chg*/

static int send_i2c_data(struct i2c_client *client,
						 struct pm8xxx_i2c_addr_data *regset,
						 int size)
{
	int i;
	int rc = 0;

	for (i = 0; i < size; i++) {
		rc = i2c_smbus_write_byte_data( client,
										regset[i].addr,
										regset[i].data );
		if (rc) {
			break;
		}
	}
	return rc;
}

#if 0 /*RUPY_del*/
#define DCDC_MASK	(u8)0xE3
#define BIT_SHIFT	(2)
#endif /*RUPY_del*/

int pm8xxx_ledi2c_on(u8 dcdc)
{
	int rc = -1;

	if (I2cClient != NULL) {
#if 1 /*RUPY_add*/
		mutex_lock(&I2cMutexLock);
#endif /*RUPY_add*/
#if 1 /*RUPY_chg*/
		if (dcdc<=0x1F) {
			/* DCDC電流設定	*/
			gpio_set_value_cansleep(gpio_37, HIGH);
#else
		if (dcdc<=0x07) {
			gpio_set_value_cansleep(gpio_25, HIGH);
#endif /*RUPY_chg*/
			msleep(1);

#if 1 /*RUPY_chg*/
			/*DCDCMAX電流の値を不揮発から取得してきた値に設定*/
			LedWk_On[10].data = dcdc;
			rc = send_i2c_data(I2cClient, LedWk_On, ARRAY_SIZE(LedWk_On));
#else
			LedWk[0].data = ((LedWk[0].data&DCDC_MASK) | (dcdc<<BIT_SHIFT));
			rc = send_i2c_data(I2cClient, LedWk, ARRAY_SIZE(LedWk));
#endif /*RUPY_chg*/
			if (rc) {
				pr_err("BackLightON failed, rc=%d\n", rc);
#if 1 /*RUPY_add*/
				mutex_unlock(&I2cMutexLock);
#endif /*RUPY_add*/
				return -ENODEV;
			}
#if 1 /*RUPY_add*/
			LedI2cOn = 1;
#endif /*RUPY_add*/
		}
#if 1 /*RUPY_add*/
		mutex_unlock(&I2cMutexLock);
#endif /*RUPY_add*/
	}

	return rc;
}
EXPORT_SYMBOL( pm8xxx_ledi2c_on);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */

int pm8xxx_ledi2c_off(void)
{
	int rc = -1;


	if (I2cClient != NULL) {
		mutex_lock(&I2cMutexLock);
#if 1 /*RUPY_chg*/
		rc = send_i2c_data(I2cClient, LedWk_Off, ARRAY_SIZE(LedWk_Off));
		if (rc) {
			pr_err("BackLightOFF failed, rc=%d\n", rc);
#if 1 /*RUPY_add*/
			mutex_unlock(&I2cMutexLock);
#endif /*RUPY_add*/
			return -ENODEV;
		}
		gpio_set_value_cansleep(gpio_26, LOW);
		gpio_set_value_cansleep(gpio_37, LOW);
#else /*RUPY_chg*/
		gpio_set_value_cansleep(gpio_25, LOW);
#endif	/*RUPY_chg*/
#if 1 /*RUPY_add*/
		LedI2cOn = 0;
		mutex_unlock(&I2cMutexLock);
#endif /*RUPY_add*/
	}

	return rc;
}
EXPORT_SYMBOL( pm8xxx_ledi2c_off);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */

static int pm8xxx_ledi2c_enable(struct i2c_client *client)
{
	int rc = 0;

	/* GPIO 設定 */
#if 1 /*RUPY_chg*/
        rc = gpio_request(gpio_37, "disp_backlight_dcdc");
#else
        rc = gpio_request(gpio_25, "disp_backlight_dcdc");
#endif  /*RUPY_chg*/
	if (rc) {
		pr_err("request gpio 25 failed, rc=%d\n", rc);
		return -ENODEV;
	}

	return rc;
}

static int __devinit pm8xxx_ledi2c_probe(struct i2c_client *client,
										 const struct i2c_device_id *id)
{
#if 1 /*RUPY_chg*/
        gpio_37 = PM8921_GPIO_PM_TO_SYS(37);
        gpio_26 = PM8921_GPIO_PM_TO_SYS(26);
#else
        gpio_25 = PM8921_GPIO_PM_TO_SYS(37);
#endif  /*RUPY_chg*/

    I2cClient = client;
#if 1 /*RUPY_add*/
    mutex_init(&I2cMutexLock);
    LedI2cOn = 0;
#endif /*RUPY_add*/

	return pm8xxx_ledi2c_enable( client );
}

static int __devexit pm8xxx_ledi2c_remove(struct i2c_client *client)
{
	pm8xxx_ledi2c_off();
	return 0;
}

#if 1/*RUPY_add*/
int leddrv_lcdbl_led_on(void)
{

	int rc = -1;
	if (I2cClient != NULL) {
		mutex_lock(&I2cMutexLock);

		if (!LedI2cOn) {
			pr_err("BackLightON failed (Can not ON!), rc=%d\n", -ENODEV);
			mutex_unlock(&I2cMutexLock);
			return -ENODEV;
		}

		rc = send_i2c_data(I2cClient, Led_On, ARRAY_SIZE(Led_On));
		if (rc) {
			pr_err("BackLightON failed, rc=%d\n", rc);
			mutex_unlock(&I2cMutexLock);
			return -ENODEV;
		}
		mutex_unlock(&I2cMutexLock);
	}

	return rc;
}
EXPORT_SYMBOL(leddrv_lcdbl_led_on);/* mrc23701 *//* mrc20101 *//* mrc24601 *//* mrc40101 *//* mrc40901 */
#endif /*RUPY_add*/



static const struct i2c_device_id pm8960_id[] = {
	{DEVICE_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, pm8960_id);

static struct i2c_driver pm8xxx_ledi2c_driver = {
	.probe		= pm8xxx_ledi2c_probe,
	.remove		= __devexit_p(pm8xxx_ledi2c_remove),
	.id_table	= pm8960_id,
	.driver		= {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8xxx_ledi2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&pm8xxx_ledi2c_driver);
	if (ret) {
		pr_info( "%s: failed to add i2c driver\n", __func__ );
	}
	return ret;
}
module_init(pm8xxx_ledi2c_init);

static void __exit pm8xxx_ledi2c_exit(void)
{
	i2c_del_driver(&pm8xxx_ledi2c_driver);
}
module_exit(pm8xxx_ledi2c_exit);

MODULE_DESCRIPTION("PM8921 LED I2C driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8xxx-i2c");
