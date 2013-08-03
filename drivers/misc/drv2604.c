/*
 *  drv2604.c - Haptic Motor
 *
 *  Copyright (C) 2012 Panasonic Mobile Communications
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define  DEBUG				/* <SlimAV1> ADD */
//#define VIB_CALIBRATION
#include <linux/err.h>		/* <SlimAV1> ADD */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c/drv2604.h>
#include "../staging/android/timed_output.h"
#include <linux/io.h>
#include <linux/laputa-fixaddr.h>

/* <SlimAV1> ADD START */
#define DRV2604_DUTY_MAX    127
#define DRV2604_DUTY_MIN   -127
/* <SlimAV1> ADD END */
/* <SlimAV1> merge from ID Power START */
#define VIB_SETTING_DISABLE
#ifdef VIB_SETTING_DISABLE
#define VIB_ENABLE_FLG      0xFFFF0000
#define VIB_DISABLE_FLG     0xEEEE0000
#define VIB_DISABLE_INIT        0
static int vibrator_disable_flg;
#endif /* VIB_SETTING_DISABLE */
static struct mutex brake_lock; /* npdc300056603 */
static bool standby_flg; /* npdc300056603 */
static bool timeout_flg; /* npdc300056603 */
static struct timer_list brake_timer; /* npdc300056603 */
static struct work_struct brake_work; /* npdc300056603 */
static struct workqueue_struct *brake_wq; /* npdc300056603 */

/* <SlimAV1> merge from ID Power END */
/* <SlimAV1> ADD START */
struct _drv2604_data {
	struct i2c_client *client;
};

static struct _drv2604_data* drv2604_data = NULL;
/* <SlimAV1> ADD END */
struct drv2604_chip {
	struct i2c_client *client;
	struct drv2604_platform_data *pdata;
	struct hrtimer timer;
	struct timed_output_dev dev;
	struct work_struct work;
	spinlock_t lock;
	unsigned int enable;
	struct regulator **regs;
	bool timer_enable;			/* <SlimAV1> ADD */
	bool power;					/* <SlimAV1> ADD */
};
/* <SlimAV1> ADD START */
#ifdef    CONFIG_PM
static int drv2604_power_off(struct i2c_client *client, pm_message_t mesg);
static int drv2604_power_on(struct i2c_client *client);
static int drv2604_suspend(struct i2c_client *client, pm_message_t mesg);
static int drv2604_resume(struct i2c_client *client);
#endif /* CONFIG_PM */
/* <SlimAV1> ADD END */
static void brake_timer_handler( unsigned long data ); /* npdc300056603 */
static void brake_work_mainfunc(struct work_struct *work); /* npdc300056603 */
static unsigned char read_val_16; /* npdc300061373 */
static unsigned char read_val_17; /* npdc300061373 */
static unsigned char read_val_18; /* npdc300061373 */
static unsigned char read_val_19; /* npdc300061373 */
static unsigned char read_val_1A; /* npdc300061373 */
static unsigned char read_val_1B; /* npdc300061373 */
static unsigned char read_val_1C; /* npdc300061373 */
static unsigned char read_val_1D; /* npdc300061373 */

static int drv2604_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	pr_debug("%s\n", __func__);

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	pr_debug("read addr 0x%x data 0x%x\n", reg, ret );

	return ret;
}

static int drv2604_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	pr_debug("%s\n", __func__);

	pr_debug("write addr 0x%x data 0x%x\n", reg, value );

	ret = i2c_smbus_write_byte_data(client, reg, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void drv2604_vib_set(struct drv2604_chip *haptic, int enable)
{
	u8  duty;              /* <SlimAV1> ADD */
	int rc = 0;

	pr_debug("%s\n", __func__);

	/* <SlimAV1> ADD START */
	duty =  (u8)haptic->pdata->duty;
/*	dev_info(&haptic->client->dev, "%s enable=0x%x duty=0x%02x\n", __func__,  enable, duty); */
	/* <SlimAV1> ADD END */
	if (enable) {
		/* npdc300061373 Add Start */
		if(( drv2604_read_reg(haptic->client, 0x01) & 0x40 ))
		{
			pr_err("%s: HARD RESET ERROR recovery start\n", __func__);
			pr_err("%s: Register 0x00 read value = 0x%x\n", __func__,drv2604_read_reg(haptic->client, 0x00));

			rc = drv2604_write_reg(haptic->client, 0x01, 0x00);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x01, data 0x00 \n", __func__);
			}

			rc = drv2604_write_reg(haptic->client, 0x02, 0x00);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x02, data 0x00 \n", __func__);
			}

			rc = drv2604_write_reg(haptic->client, 0x16, read_val_16);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x16, data 0x%02x \n", __func__, read_val_16);
			}

			rc = drv2604_write_reg(haptic->client, 0x17, read_val_17);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x17, data 0x%02x \n", __func__, read_val_17);
			}

			rc = drv2604_write_reg(haptic->client, 0x18, read_val_18);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x18, data 0x%02x \n", __func__, read_val_18);
			}

			rc = drv2604_write_reg(haptic->client, 0x19, read_val_19);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x19, data 0x%02x \n", __func__, read_val_19);
			}

			rc = drv2604_write_reg(haptic->client, 0x1A, read_val_1A);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x1A, data 0x%02x \n", __func__, read_val_1A);
			}

			rc = drv2604_write_reg(haptic->client, 0x1B, read_val_1B);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x1B, data 0x%02x \n", __func__, read_val_1B);
			}

			rc = drv2604_write_reg(haptic->client, 0x1C, read_val_1C);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x1C, data 0x%02x \n", __func__, read_val_1C);
			}

			rc = drv2604_write_reg(haptic->client, 0x1D, read_val_1D);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x1D, data 0x%02x \n", __func__, read_val_1D);
			}

			rc = drv2604_write_reg(haptic->client, 0x1e, 0x30);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x1e, data 0x30 \n", __func__);
			}

			rc = drv2604_write_reg(haptic->client, 0x01, 0x05);
			if (rc < 0) {
				pr_err("%s: i2c write failure addr 0x01, data 0x05 \n", __func__);
			}
		}
		/* npdc300061373 Add End */
		rc = drv2604_write_reg(haptic->client, 0x02, duty);
		if (rc < 0) {
			pr_err("%s: start vibartion fail duty=0x%02x\n", __func__, duty);
		}
	} else {
		rc = drv2604_write_reg(haptic->client, 0x02, 0x00);
		if (rc < 0) {
				pr_err("%s: stop vibartion fail\n", __func__);
		}
	}
}

static void drv2604_chip_work(struct work_struct *work)
{
	struct drv2604_chip *haptic;
	pm_message_t		mesg;	/* <SlimAV1> ADD */

	pr_debug("%s\n", __func__);

	haptic = container_of(work, struct drv2604_chip, work);

	/* <SlimAV1> ADD START */
    pr_debug( "%s: timer_enable=%d enable=%d\n", __func__, haptic->timer_enable, haptic->enable );

	if ( haptic->timer_enable ) {
		if ( haptic->enable != 0) {
			drv2604_power_on( haptic->client );
		}
	}
	/* <SlimAV1> ADD END */
	/* <SlimAV1> npdc300035638 MOD START */
	if(haptic->power != false)
	{
		drv2604_vib_set(haptic, haptic->enable);
	} else if (haptic->enable == 1) {
		pr_warning("%s: vibrator is power off\n", __func__);
	} else {}
	/* <SlimAV1> npdc300035638 MOD END */
	/* <SlimAV1> ADD START */
	if ( haptic->timer_enable ) {
		if ( haptic->enable == 0) {
			mesg = PMSG_SUSPEND;
			drv2604_power_off( haptic->client, mesg );
		}
	}
	/* <SlimAV1> ADD END */
}

static void drv2604_chip_enable(struct timed_output_dev *dev, int value)
{
	struct drv2604_chip *haptic = container_of(dev, struct drv2604_chip,
					dev);
	unsigned long flags;

	pr_debug("%s\n", __func__);

/* <SlimAV1> merge from ID Power START */
#ifdef VIB_SETTING_DISABLE
	pr_debug( "%s: value = %d vibrator_disable_flg = %d\n", __func__, value, vibrator_disable_flg );
	spin_lock_irqsave(&haptic->lock, flags);
	if(( value == VIB_ENABLE_FLG ) || ( value == VIB_DISABLE_FLG ))
	{
		if( value == VIB_ENABLE_FLG )
		{
			if( vibrator_disable_flg == VIB_DISABLE_INIT )
			{
				pr_err("vibrator_disable_flg is already set 0\n");
				spin_unlock_irqrestore(&haptic->lock, flags);
				return;
			}
			vibrator_disable_flg--;
			pr_debug( "Decrease vibrator_disable_flg = %d\n", vibrator_disable_flg );
			spin_unlock_irqrestore(&haptic->lock, flags);
			return;
		}
		else
		{
			vibrator_disable_flg++;
			pr_debug( "Increase vibrator_disable_flg = %d\n", vibrator_disable_flg );
		}
	}
	spin_unlock_irqrestore(&haptic->lock, flags);
#endif /* VIB_SETTING_DISABLE */
	spin_lock_irqsave(&haptic->lock, flags);
	hrtimer_cancel(&haptic->timer);
#ifdef VIB_SETTING_DISABLE
	if((( value == 0 ) || ( value == VIB_DISABLE_FLG )))
	{
		haptic->enable = 0;
		pr_debug( "haptic->enable = %d\n", haptic->enable );
	}
	else
	{
		if( vibrator_disable_flg == VIB_DISABLE_INIT )
		{
			value = (value > haptic->pdata->max_timeout ?
					 haptic->pdata->max_timeout : value);
			haptic->enable = 1;
			pr_debug( "%s: haptic->timer_enable = %d\n", __func__ , haptic->timer_enable );
			/* <SlimAV1> MOD START */
			if ( haptic->timer_enable ) {
				hrtimer_start(&haptic->timer,
					      ktime_set(value / 1000, (value % 1000) * 1000000),
					      HRTIMER_MODE_REL);
			}
			/* <SlimAV1> MOD END */
		}
		else
		{
			haptic->enable = 0;
			pr_debug( "haptic->enable = %d\n", haptic->enable );
		}
	}
#else  /* VIB_SETTING_DISABLE */
	if (value == 0)
		haptic->enable = 0;
	else {
		value = (value > haptic->pdata->max_timeout ?
				haptic->pdata->max_timeout : value);
		haptic->enable = 1;
		hrtimer_start(&haptic->timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
#endif /* VIB_SETTING_DISABLE */
/* <SlimAV1> merge from ID Power END */
	spin_unlock_irqrestore(&haptic->lock, flags);
	schedule_work(&haptic->work);
}
/* <SlimAV1> ADD START */
static void drv2604_chip_enable_timer_on(struct timed_output_dev *dev, int value)
{
	struct drv2604_chip *haptic = container_of(dev, struct drv2604_chip, dev);
	unsigned long flags;

    pr_debug( "%s\n", __func__ );

	spin_lock_irqsave(&haptic->lock, flags);
	/* <SlimAV1> npdc300035642 ADD START */
	haptic->pdata->duty = 127;
	/* <SlimAV1> npdc300035642 ADD END */
	haptic->timer_enable = true;
	spin_unlock_irqrestore(&haptic->lock, flags);
	drv2604_chip_enable(dev, value);
}
/* <SlimAV1> ADD END */

static int drv2604_chip_get_time(struct timed_output_dev *dev)
{
	struct drv2604_chip *haptic = container_of(dev, struct drv2604_chip,
					dev);

	pr_debug("%s\n", __func__);

	if (hrtimer_active(&haptic->timer)) {
		ktime_t r = hrtimer_get_remaining(&haptic->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
		return 0;
}

static enum hrtimer_restart drv2604_vib_timer_func(struct hrtimer *timer)
{
	struct drv2604_chip *haptic = container_of(timer, struct drv2604_chip,
					timer);

	pr_debug("%s\n", __func__);

	haptic->enable = 0;
	schedule_work(&haptic->work);

	return HRTIMER_NORESTART;
}
/* <SlimAV1> ADD START */
static int drv2604_set_enable(bool flag)
{
	struct drv2604_chip*	haptic;
	int						ret = 0;
	pm_message_t			mesg;

	pr_debug("%s flag=%d\n", __func__, flag );
	if ( drv2604_data == NULL ) {
		pr_err("%s error! driver is not initialized.\n", __func__);
		ret = -ENODATA;
		goto func_end;
	}

	haptic = i2c_get_clientdata( drv2604_data->client );

	dev_dbg(&haptic->client->dev, "%s flag=%d haptic->power=%d\n",
								__func__, flag, haptic->power );

	if (haptic->power == flag) {
		// No Process
		goto func_end;
	}

	if( flag ) {
		/* Power ON */
		drv2604_resume( haptic->client );
	} else {
		/* Power OFF */
		mesg = PMSG_SUSPEND;
		drv2604_suspend( haptic->client, mesg );
	}

func_end:
	return ret;
}

/* for Interface */
int hap_vib_amp_enable( void )
{
    int ret = 0; /* npdc300056603 */

	pr_debug("%s IN\n", __func__);

    mutex_lock( &brake_lock );      /* npdc300056603 */

    del_timer(&brake_timer);        /* npdc300056603 */
    timeout_flg = false;            /* npdc300056603 */
    standby_flg = false;            /* npdc300056603 */
    ret = drv2604_set_enable(true); /* npdc300056603 */

    mutex_unlock( &brake_lock );    /* npdc300056603 */

	pr_debug("%s OUT\n", __func__);

	return ret; /* npdc300056603 */
}
EXPORT_SYMBOL(hap_vib_amp_enable);


int hap_vib_amp_disable( void )
{
    int ret = 0; /* npdc300056603 */

	pr_debug("%s IN\n", __func__);

    /* npdc300056603 ADD START */
    mutex_lock( &brake_lock );

    if( timeout_flg == false )
    {
        standby_flg = true;
    }
    else
    {
        ret = drv2604_set_enable(false);
    }

    mutex_unlock( &brake_lock );
    /* npdc300056603 ADD END */

	pr_debug("%s OUT\n", __func__);

	return ret; /* npdc300056603 */
}
EXPORT_SYMBOL(hap_vib_amp_disable);


int hap_vib_vibrator_control(signed char pwm_duty)
{
	struct drv2604_chip*	haptic;
	int						ret = 0;
    unsigned long			flags;

	pr_debug("%s pwm_duty=%d\n", __func__, pwm_duty);
	if ( drv2604_data == NULL ) {
		pr_err("%s error! driver is not initialized.\n", __func__);
		ret = -ENODATA;
		goto func_end;
	}

	haptic = i2c_get_clientdata( drv2604_data->client );

	/* check power */
	if ( haptic->power == false) {
		pr_err("%s error! device is power off.\n", __func__);
		ret = -EAGAIN;
		goto func_end;
	}

	dev_dbg(&haptic->client->dev, "%s duty=%d\n", __func__, pwm_duty);
	if( ( pwm_duty < DRV2604_DUTY_MIN ) || ( pwm_duty > DRV2604_DUTY_MAX ) ) {
		dev_err(&haptic->client->dev, "%s error! duty is out of range.\n", __func__);
		ret = -EINVAL;
		goto func_end;
	}
    if( pwm_duty < 0 )
    {
        pwm_duty = 0;
    }
	spin_lock_irqsave(&haptic->lock, flags);
	haptic->pdata->duty = pwm_duty;
	haptic->timer_enable = false;
	spin_unlock_irqrestore(&haptic->lock, flags);

	/* request vib start/stop */
	if (pwm_duty == 0) {

        mutex_lock( &brake_lock );      /* npdc300056603 */

        del_timer(&brake_timer);        /* npdc300056603 */
        timeout_flg = false;            /* npdc300056603 */
        brake_timer.expires = jiffies + (100*HZ/1000); /* npdc300056603 */
        add_timer(&brake_timer);        /* npdc300056603 */

        mutex_unlock( &brake_lock );    /* npdc300056603 */

		/* set work queue */
		drv2604_chip_enable(&haptic->dev, 0);
	}
	else {

        mutex_lock( &brake_lock );      /* npdc300056603 */

        del_timer(&brake_timer);        /* npdc300056603 */
        timeout_flg = false;            /* npdc300056603 */

        mutex_unlock( &brake_lock );    /* npdc300056603 */

		/* set work queue */
		drv2604_chip_enable(&haptic->dev, 1);
	}

func_end:
	return ret;
}
EXPORT_SYMBOL(hap_vib_vibrator_control);
/* <SlimAV1> ADD END */

/* npdc300056603 ADD START */
static void brake_timer_handler( unsigned long data )
{
	pr_debug("%s\n", __func__);
    queue_work( brake_wq, & brake_work );
    return;
}

static void brake_work_mainfunc(struct work_struct *work)
{

	pr_debug("%s IN\n", __func__);

    mutex_lock( &brake_lock );

    if( standby_flg == false )
    {
        timeout_flg = true;
    }
    else
    {
        drv2604_set_enable(false);
    }

    mutex_unlock( &brake_lock );

	pr_debug("%s OUT\n", __func__);

}

/* npdc300056603 ADD END */

static void dump_drv2604_reg(char *str, struct i2c_client *client)
{
	/* <SlimAV1> MOD START */
	pr_debug("%s reg0x%02x=0x%02x, reg0x%02x=0x%02x, "
			 "reg0x%02x=0x%02x, reg0x%02x=0x%02x, "
			 "reg0x%02x=0x%02x, reg0x%02x=0x%02x, "
			 "reg0x%02x=0x%02x, reg0x%02x=0x%02x, "
			 "reg0x%02x=0x%02x, reg0x%02x=0x%02x, reg0x%02x=0x%02x\n", str,
		0x01, drv2604_read_reg(client, 0x01),
		0x02, drv2604_read_reg(client, 0x02),
		0x16, drv2604_read_reg(client, 0x16),
		0x17, drv2604_read_reg(client, 0x17),
		0x18, drv2604_read_reg(client, 0x18),
		0x19, drv2604_read_reg(client, 0x19),
		0x1A, drv2604_read_reg(client, 0x1A),
		// update npdc300060021 -S
		0x1B, drv2604_read_reg(client, 0x1B),
		0x1C, drv2604_read_reg(client, 0x1C),
		0x1D, drv2604_read_reg(client, 0x1D),
		0x1E, drv2604_read_reg(client, 0x1E));
		// update npdc300060021 -E
	/* <SlimAV1> MOD END */
}

static int drv2604_setup(struct i2c_client *client)
{
	struct drv2604_chip *haptic = i2c_get_clientdata(client);
//	int rc;  /* Quad DEL for Sysdes5.27 v0.53 */

	pr_debug("%s\n", __func__);

/*	udelay(250); */

	gpio_set_value_cansleep(haptic->pdata->hap_en_gpio, 1);
	pr_debug("%s : hap_en_gpio is High.\n", __func__);

#if 0 /* Quad DEL for Sysdes5.27 v0.53 */
    /* Initial Setting */
	rc = drv2604_write_reg(client, 0x01, 0x00);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x01, data 0x00 \n", __func__);
		goto reset_gpios;
	}
	rc = drv2604_write_reg(client, 0x1D, 0x80);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x1D, data 0x80 \n", __func__);
		goto reset_gpios;
	}
	rc = drv2604_write_reg(client, 0x02, 0x00);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x02, data 0x00 \n", __func__);
		goto reset_gpios;
	}
	rc = drv2604_write_reg(client, 0x01, 0x45);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x01, data 0x45 \n", __func__);
		goto reset_gpios;
	}
	rc = drv2604_write_reg(client, 0x01, 0x05);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x01, data 0x05 \n", __func__);
		goto reset_gpios;
	}
#endif

	dump_drv2604_reg("new:", client);
	return 0;

#if 0  /* Quad DEL for Sysdes5.27 v0.53 */
reset_gpios:
	gpio_set_value_cansleep(haptic->pdata->hap_en_gpio, 0);
	return rc;
#endif
}

static int drv2604_reg_power(struct drv2604_chip *haptic, bool on)
{
	const struct drv2604_regulator *reg_info =
				haptic->pdata->regulator_info;
	u8 i, num_reg = haptic->pdata->num_regulators;
	int rc;

	pr_debug("%s\n", __func__);

	for (i = 0; i < num_reg; i++) {
		rc = regulator_set_optimum_mode(haptic->regs[i],
					on ? reg_info[i].load_uA : 0);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed(%d)\n",
							__func__, rc);
			goto regs_fail;
		}

		rc = on ? regulator_enable(haptic->regs[i]) :
			regulator_disable(haptic->regs[i]);
		if (rc < 0) {
			pr_err("%s: regulator %sable fail %d\n", __func__,
					on ? "en" : "dis", rc);
			regulator_set_optimum_mode(haptic->regs[i],
					!on ? reg_info[i].load_uA : 0);
			goto regs_fail;
		}
	}

	return 0;

regs_fail:
	while (i--) {
		regulator_set_optimum_mode(haptic->regs[i],
				!on ? reg_info[i].load_uA : 0);
		!on ? regulator_enable(haptic->regs[i]) :
			regulator_disable(haptic->regs[i]);
	}
	return rc;
}

static int drv2604_reg_setup(struct drv2604_chip *haptic, bool on)
{
	const struct drv2604_regulator *reg_info =
				haptic->pdata->regulator_info;
	u8 i, num_reg = haptic->pdata->num_regulators;
	int rc = 0;

	pr_debug("%s\n", __func__);

	/* put regulators */
	if (on == false) {
		i = num_reg;
		goto put_regs;
	}

	haptic->regs = kzalloc(num_reg * sizeof(struct regulator *),
							GFP_KERNEL);
	if (!haptic->regs) {
		pr_err("unable to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < num_reg; i++) {
		haptic->regs[i] = regulator_get(&haptic->client->dev,
							reg_info[i].name);
		if (IS_ERR(haptic->regs[i])) {
			rc = PTR_ERR(haptic->regs[i]);
			pr_err("%s:regulator get failed(%d)\n",	__func__, rc);
			goto put_regs;
		}

		if (regulator_count_voltages(haptic->regs[i]) > 0) {
			rc = regulator_set_voltage(haptic->regs[i],
				reg_info[i].min_uV, reg_info[i].max_uV);
			if (rc) {
				pr_err("%s: regulator_set_voltage failed(%d)\n",
								__func__, rc);
				regulator_put(haptic->regs[i]);
				goto put_regs;
			}
		}
	}

	return rc;

put_regs:
	while (i--) {
		if (regulator_count_voltages(haptic->regs[i]) > 0)
			regulator_set_voltage(haptic->regs[i], 0,
						reg_info[i].max_uV);
		regulator_put(haptic->regs[i]);
	}
	kfree(haptic->regs);
	return rc;
}

static int __devinit drv2604_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct drv2604_chip *haptic;
	struct drv2604_platform_data *pdata;
	pm_message_t		mesg;	/* <SlimAV1> ADD */
	int ret;
	int rc;
	unsigned char read_val = 0;
	void *hap_dmem_addr;

	pr_debug("%s\n", __func__);

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "%s: no support for i2c read/write"
				"byte data\n", __func__);
		return -EIO;
	}

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -EINVAL;
	}

	haptic = kzalloc(sizeof(struct drv2604_chip), GFP_KERNEL);
	if (!haptic) {
		ret = -ENOMEM;
		goto mem_alloc_fail;
	}
	haptic->client = client;
	haptic->enable = 0;
	haptic->pdata = pdata;

	if (pdata->regulator_info) {
		ret = drv2604_reg_setup(haptic, true);
		if (ret) {
			dev_err(&client->dev, "%s: regulator setup failed\n",
							__func__);
			goto reg_setup_fail;
		}

		ret = drv2604_reg_power(haptic, true);
		if (ret) {
			dev_err(&client->dev, "%s: regulator power failed\n",
							__func__);
			goto reg_pwr_fail;
		}
	}

	spin_lock_init(&haptic->lock);
	INIT_WORK(&haptic->work, drv2604_chip_work);

	hrtimer_init(&haptic->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	haptic->timer.function = drv2604_vib_timer_func;

	/*register with timed output class*/
	haptic->dev.name = pdata->name;
	haptic->dev.get_time = drv2604_chip_get_time;
	haptic->dev.enable = drv2604_chip_enable_timer_on; 	/* <SlimAV1> MOD */
/* <SlimAV1> merge from ID Power START */
#ifdef VIB_SETTING_DISABLE
	vibrator_disable_flg = VIB_DISABLE_INIT;
	pr_debug( "Initialize vibrator_disable_flg = %d\n", vibrator_disable_flg );
#endif /* VIB_SETTING_DISABLE */ /**/
/* <SlimAV1> merge from ID Power END */
	ret = timed_output_dev_register(&haptic->dev);
	if (ret < 0)
		goto timed_reg_fail;

	i2c_set_clientdata(client, haptic);

	ret = gpio_is_valid(pdata->hap_en_gpio);
	if (ret) {
		ret = gpio_request(pdata->hap_en_gpio, "haptic_en_gpio");
		if (ret) {
			dev_err(&client->dev, "%s: gpio %d request failed\n",
					__func__, pdata->hap_en_gpio);
			goto hen_gpio_fail;
		}
	} else {
		dev_err(&client->dev, "%s: Invalid gpio %d\n", __func__,
					pdata->hap_en_gpio);
		goto hen_gpio_fail;
	}

	ret = gpio_is_valid(haptic->pdata->hap_inttrig_gpio);
	if (ret) {
		ret = gpio_request(pdata->hap_inttrig_gpio,
					"hap_inttrig_gpio");
		if (ret) {
			dev_err(&client->dev,
				"%s: gpio %d request failed\n",
				__func__, pdata->hap_inttrig_gpio);
			goto len_gpio_fail;
		}
	} else {
		dev_err(&client->dev, "%s: gpio is not used/Invalid %d\n",
					__func__, pdata->hap_inttrig_gpio);
	}

    //not use. initial setting.
    gpio_set_value_cansleep(haptic->pdata->hap_inttrig_gpio, 0); 

	/* <SlimAV1> ADD START */
	haptic->timer_enable = false;
	haptic->power = false;
	drv2604_data = kzalloc(sizeof(struct _drv2604_data), GFP_KERNEL);
	if ( drv2604_data == NULL ) {
		dev_err(&client->dev, "%s: can not allocate drv2604_data \n", __func__);
		goto drv2604_data_fail;
	}
	drv2604_data->client = client;
	/* <SlimAV1> ADD END */

/*	udelay(250); */
	gpio_set_value_cansleep(haptic->pdata->hap_en_gpio, 1);
	pr_debug("%s : hap_en_gpio is High.\n", __func__);

#ifdef VIB_CALIBRATION
	rc = drv2604_write_reg(client, 0x01, 0x07);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x01, data 0x07 \n", __func__);
	}
	rc = drv2604_write_reg(client, 0x1A, 0xA4);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x1A, data 0xA4 \n", __func__);
	}
	rc = drv2604_write_reg(client, 0x16, 0x60);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x16, data 0x60 \n", __func__);
	}
	rc = drv2604_write_reg(client, 0x17, 0x65);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x17, data 0x65 \n", __func__);
	}

    while(1)
    {
    	rc = drv2604_write_reg(client, 0x0C, 0x01);
    	if (rc < 0) {
    		pr_err("%s: i2c write failure addr 0x0C, data 0x01 \n", __func__);
    	}

        msleep(3000);

    	rc = drv2604_read_reg(client, 0x00);
    	if ((rc & 0x08) == 0) {
            break;
    	}
    	pr_err("Calibration fail addr 0x00, data 0x%x \n", rc);
    }
	rc = drv2604_read_reg(client, 0x18);
	pr_err("CALIBRATION RESULT 0x18, data 0x%x \n", rc);
	rc = drv2604_read_reg(client, 0x19);
	pr_err("CALIBRATION RESULT 0x19, data 0x%x \n", rc);
	rc = drv2604_read_reg(client, 0x1A);
	pr_err("CALIBRATION RESULT 0x1A, data 0x%x \n", rc);
#endif

  /* Initial Setting */
  pr_debug("START Initial Settings.\n");
  
  rc = drv2604_write_reg(client, 0x01, 0x00);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x01, data 0x00 \n", __func__);
  }

  // update npdc300060021 -S
  rc = drv2604_write_reg(client, 0x02, 0x00);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x02, data 0x00 \n", __func__);
  }
  // update npdc300060021 -E

  hap_dmem_addr = ioremap(DMEM_HAP_RATED_VOL, DMEM_HAP_RATED_VOL_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_16 = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x16, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x16, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_CLAMP_VOL, DMEM_HAP_CLAMP_VOL_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_17 = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x17, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x17, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_LOSS_COMPENSATION, DMEM_HAP_LOSS_COMPENSATION_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_18 = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x18, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x18, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_BACK_EMF, DMEM_HAP_BACK_EMF_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_19 = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x19, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x19, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_FEEDBACK_CNT, DMEM_HAP_FEEDBACK_CNT_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_1A = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x1A, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x1A, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_CONTROL_1, DMEM_HAP_CONTROL_1_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_1B = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x1B, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x1B, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_CONTROL_2, DMEM_HAP_CONTROL_2_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_1C = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x1C, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x1C, data 0x%02x \n", __func__, read_val);
  }

  hap_dmem_addr = ioremap(DMEM_HAP_CONTROL_3, DMEM_HAP_CONTROL_3_SIZE);
  read_val = readl(hap_dmem_addr) & 0xff;
  read_val_1D = read_val; /* npdc300061373 */
  rc = drv2604_write_reg(client, 0x1D, read_val);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x1D, data 0x%02x \n", __func__, read_val);
  }

  // update npdc300060021 -S
  /*rc = drv2604_write_reg(client, 0x02, 0x00);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x02, data 0x00 \n", __func__);
  }*/
  rc = drv2604_write_reg(client, 0x1e, 0x30);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x1e, data 0x30 \n", __func__);
  }
  // update npdc300060021 -E

  rc = drv2604_write_reg(client, 0x01, 0x05);
  if (rc < 0) {
    pr_err("%s: i2c write failure addr 0x01, data 0x05 \n", __func__);
  }

  pr_debug("END Initial Settings.\n");

	gpio_set_value_cansleep(haptic->pdata->hap_en_gpio, 0);
	pr_debug("%s : hap_en_gpio is Low.\n", __func__);

	ret = drv2604_setup(client);
	if (ret) {
		dev_err(&client->dev, "%s: setup fail %d\n", __func__, ret);
		goto setup_fail;
	}

	/* <SlimAV1> ADD START */
	mesg = PMSG_SUSPEND;
	drv2604_suspend( client, mesg );
	/* <SlimAV1> ADD END */

    /* npdc300056603 ADD START */
    timeout_flg = false;
    standby_flg = false;

    mutex_init( &brake_lock );

    init_timer( &brake_timer );
    brake_timer.function = (void*)brake_timer_handler;

	brake_wq = create_singlethread_workqueue("brake_wq");
	if (!brake_wq) {
		return -ENOMEM;
	}

	INIT_WORK(&brake_work, brake_work_mainfunc);
    /* npdc300056603 ADD END */

	pr_debug(KERN_INFO "%s: %s registered\n", __func__, id->name);
	return 0;

setup_fail:
	kfree(drv2604_data);
drv2604_data_fail:
	gpio_free(pdata->hap_inttrig_gpio);
len_gpio_fail:
	gpio_free(pdata->hap_en_gpio);
hen_gpio_fail:
	timed_output_dev_unregister(&haptic->dev);
timed_reg_fail:
	if (pdata->regulator_info)
		drv2604_reg_power(haptic, false);
reg_pwr_fail:
	if (pdata->regulator_info)
		drv2604_reg_setup(haptic, false);
reg_setup_fail:
	kfree(haptic);
mem_alloc_fail:
	return ret;
}

static int __devexit drv2604_remove(struct i2c_client *client)
{
	struct drv2604_chip *haptic = i2c_get_clientdata(client);

	pr_debug("%s\n", __func__);

    /* npdc300056603 ADD START */
	del_timer(&brake_timer);
	if (brake_wq) {
        flush_workqueue( brake_wq );
		destroy_workqueue( brake_wq );
	}
    /* npdc300056603 ADD END */

	hrtimer_cancel(&haptic->timer);
	cancel_work_sync(&haptic->work);

	/* turn-off current vibration */
	drv2604_vib_set(haptic, 0);

	timed_output_dev_unregister(&haptic->dev);

	gpio_set_value_cansleep(haptic->pdata->hap_en_gpio, 0);
	pr_debug("%s : hap_en_gpio is Low.\n", __func__);
	gpio_set_value_cansleep(haptic->pdata->hap_inttrig_gpio, 0);

	gpio_free(haptic->pdata->hap_en_gpio);
	gpio_free(haptic->pdata->hap_inttrig_gpio);

	/* power-off the chip */
	if (haptic->pdata->regulator_info) {
		drv2604_reg_power(haptic, false);
		drv2604_reg_setup(haptic, false);
	}

	/* <SlimAV1> ADD START */
	kfree(drv2604_data);
	/* <SlimAV1> ADD END */
	kfree(haptic);
	return 0;
}

#ifdef CONFIG_PM
/* <SlimAV1> ADD START */
static int drv2604_power_off(struct i2c_client *client, pm_message_t mesg)
{
	struct drv2604_chip *haptic = i2c_get_clientdata(client);
    unsigned long flags;
//	int rc;

    pr_debug( "%s\n", __func__ );

	/* <SlimAV1> npdc300035638 MOD START */
	if(haptic->power != false)
	{
		drv2604_vib_set(haptic, 0);
	}
	/* <SlimAV1> npdc300035638 MOD END */

#if 0  /* Quad DEL for Sysdes5.27 v0.53 */
	rc = drv2604_write_reg(client, 0x01, 0x45);
	if (rc < 0) {
		pr_err("%s: i2c write failure addr 0x01, data 0x45 \n", __func__);
	}
#endif

	gpio_set_value_cansleep(haptic->pdata->hap_en_gpio, 0);
	pr_debug("%s : hap_en_gpio is Low.\n", __func__);
	gpio_set_value_cansleep(haptic->pdata->hap_inttrig_gpio, 0);

	if (haptic->pdata->regulator_info)
		drv2604_reg_power(haptic, false);

	spin_lock_irqsave(&haptic->lock, flags);
	haptic->power = false;
	spin_unlock_irqrestore(&haptic->lock, flags);

	return 0;
}

static int drv2604_power_on(struct i2c_client *client)
{
	struct drv2604_chip *haptic = i2c_get_clientdata(client);
    unsigned long flags;

    pr_debug( "%s\n", __func__ );

	if (haptic->pdata->regulator_info)
		drv2604_reg_power(haptic, true);

	spin_lock_irqsave(&haptic->lock, flags);
	haptic->power = true;
	spin_unlock_irqrestore(&haptic->lock, flags);

	drv2604_setup(client);
	return 0;
}
/* <SlimAV1> ADD END */

static int drv2604_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct drv2604_chip *haptic = i2c_get_clientdata(client);

	pr_debug("%s\n", __func__);

	hrtimer_cancel(&haptic->timer);
	cancel_work_sync(&haptic->work);
	drv2604_power_off( client, mesg );

	return 0;
}

static int drv2604_resume(struct i2c_client *client)
{

	pr_debug("%s\n", __func__);

	drv2604_power_on( client );

	return 0;
}
#else
#define drv2604_suspend		NULL
#define drv2604_resume		NULL
#endif

static const struct i2c_device_id drv2604_id[] = {
	{ "drv2604_1", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, drv2604_id);

static struct i2c_driver drv2604_driver = {
	.driver	= {
		.name	= "drv2604",
	},
	.probe		= drv2604_probe,
	.remove		= __devexit_p(drv2604_remove),
	.suspend	= drv2604_suspend,
	.resume		= drv2604_resume,
	.id_table	= drv2604_id,
};

static int __init drv2604_init(void)
{
    int ret;

	pr_debug("%s\n", __func__);

    ret = i2c_add_driver(&drv2604_driver);
    if(ret)
        pr_err("drv2604_init: failed\n");

	return ret;
}

static void __exit drv2604_exit(void)
{
	pr_debug("%s\n", __func__);

	i2c_del_driver(&drv2604_driver);
}

module_init(drv2604_init);
module_exit(drv2604_exit);

MODULE_AUTHOR("PMCRD");
MODULE_DESCRIPTION("DRV2604 Haptic Motor driver");
MODULE_LICENSE("GPL");
