/* 
 * Battery Safety IC driver
 * Copyright(C) 2011 NTT-Data MSE Co., Ltd.
 * Written by M.M.
 * Based on spider2.c
 */
/* #define SAFETYIC_DEBUG */
#ifdef    SAFETYIC_DEBUG       /* Jessie add start */
#ifndef   DEBUG
#define   DEBUG
#endif
#endif /* SAFETYIC_DEBUG */    /* Jessie add end */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
/* #include <linux/gpio_ex.h> */  /* IDPower */
#include <linux/gpio.h>           /* IDPower */
/* #include <linux/i2c/beryl.h> *//* IDPower */
#include <asm/mach-types.h>
#include <linux/interrupt.h>
/* #include <plat/mux.h> */       /* IDPower */
/* #include <plat/control.h> */   /* IDPower */
#include <linux/power_supply.h>
#include <linux/rtc.h>
#include <linux/cfgdrv.h>
#include <linux/safety_ic.h>
/* #include <linux/charger.h> */  /* IDPower */
#include <linux/slab.h>           /* IDPower */
#include <linux/mfd/pm8xxx/pm8xxx-adc.h> /* IDPower */
#if    0 /* SlimID */
#include <linux/reboot.h>
#endif 
/* Jessie mod start */
#ifdef SAFETYIC_DEBUG
#define DEBUG_SAFETY(X)            printk X
#define SAFETYIC_DEBUG_LOG(dev, ...)    dev_err(dev, ## __VA_ARGS__)
#else /* SAFETYIC_DEBUG */
#define DEBUG_SAFETY(X)
#define SAFETYIC_DEBUG_LOG(dev, ...)
#endif /* SAFETYIC_DEBUG */
/* Jessie mod end */
/* Jessie add start */
#define INT_VTH6_SOFT    (1<<1)
#define NO_FACTOR        (0)
#if 1 /*RUPY*/
#define PM8921_GPIO_BASE            NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)  (pm_gpio - 1 + PM8921_GPIO_BASE)
#define GPIO_CHG_SUSPEND (PM8921_GPIO_PM_TO_SYS(10))
#else
#define GPIO_CHG_SUSPEND (76)
#endif /*RUPY*/
/* Jessie add end */
/* Quad add start */
#define CHG_SUSPEND_WAIT_MSEC 21     /* More than 20ms */
/* Quad add end */

static struct workqueue_struct *safety_ic_wq;

struct safety_ic_device_info {
        struct i2c_client  *client;
/* IDPower        struct decice      *dev; */
        struct device      *dev; /* IDPower */
        struct work_struct work;
	int                func_state;
        int                id;
};

/*
 * Safety-IC Addr-Table
 */
static unsigned char safety_ic_addr[] = {
	DSAFETY_CURRENT_FACTOR1,    /* 0xF5 */
	DSAFETY_CURRENT_FACTOR2,    /* 0xF6 */
	DSAFETY_TEMP_FACTOR1,       /* 0xF7 */
	DSAFETY_TEMP_FACTOR2,       /* 0xF8 */
	DSAFETY_ETC_FACTOR,         /* 0xF9 */
	DSAFETY_INT_FACTOR,         /* 0xFA */
	DSAFETY_CPUMODE,            /* 0xFB */
	DSAFETY_PROTECT,            /* 0xFC */
	DSAFETY_SHUTDOWN            /* 0xFD */
};

/*
 * Safety-IC Register-Value-Table(Addr From 0xF5 to 0xFD )
 */
static unsigned char safety_ic_val[9];

static unsigned char safety_ic_protect_buf[] ={ DSAFETY_PROTECT,   0x80 };
static unsigned char safety_ic_shutdown_buf[] ={ DSAFETY_SHUTDOWN, 0x01 };

static struct i2c_msg intfactor_msg[] = {
	{ 0x08, 0,        1,    &safety_ic_addr[5] }, /* INTFACOTR ADDR */        
	{ 0x08, I2C_M_RD, 1,    &safety_ic_val[5]  }
};

/*
 * To receive it continuously, the address setting is noted. 
 */
static struct i2c_msg etcfactor_msg[] = {
        { 0x08, 0,        1,    &safety_ic_addr[0] }, /* ETCFACOTR ADDR */
        { 0x08, I2C_M_RD, 6,    &safety_ic_val[0]  }  /* continuous receive */
};

/* Quad add start <npdc300053027> */
static struct i2c_msg etcfactor_only_msg[] = {
        { 0x08, 0,        1,    &safety_ic_addr[4] }, /* ETCFACOTR ADDR */
        { 0x08, I2C_M_RD, 1,    &safety_ic_val[4]  }
};
/* Quad add end <npdc300053027> */

/*
 * SHUTDOWN Register writes.
 */
static struct i2c_msg shutdown_msg[] = {
        { 0x08, 0,        2,    &safety_ic_protect_buf[0] },   /* PROTECTION */
        { 0x08, 0,        2,    &safety_ic_shutdown_buf[0]  }  /* SHUTDOWN   */
};

/* IDPower start */
bool safety_adc_flg;
static unsigned char safety_ic_cpumode_buf[] ={ DSAFETY_CPUMODE, 0x00 };
/*
 * CPUMODE Register writes.
 */
static struct i2c_msg cpumode_write_msg[] = {
	{ 0x08, 0,        2,    &safety_ic_cpumode_buf[0] } /* CPUMODE */
};

#if 0
static struct i2c_msg cpumode_read_msg[] = {
	{ 0x08, 0,        1,    &safety_ic_addr[6] }, /* CPUMODE ADDR */
	{ 0x08, I2C_M_RD, 1,    &safety_ic_val[6]  }
};
#endif

/*
 * Safety-IC Addr-Table
 */
static unsigned char safety_ic_channel[] = {
	DSAFETY_CHANNEL_SET_STA,    /* (1 << 0) */
	DSAFETY_CHANNEL_SET_S1,     /* (1 << 1) */
	DSAFETY_CHANNEL_SET_S2,     /* (1 << 2) */
	DSAFETY_CHANNEL_SET_S3,     /* (1 << 3) */
	DSAFETY_CHANNEL_SET_S4,     /* (1 << 4) */
	DSAFETY_CHANNEL_SET_S5,     /* (1 << 5) */
	DSAFETY_CHANNEL_SET_S6,     /* (1 << 6) */
	DSAFETY_CHANNEL_SET_S7      /* (1 << 7) */
};
/* IDPower end */

/*
 * Timer List
 */
static struct timer_list safety_ic_fctget_timer;
static struct timer_list safety_ic_shutdown_timer;

/* Jessie add start */
/*
 * charge status
 */
unsigned char safety_ic_charge_sts;
/* Jessie add end */

/* syserr_TimeStamp is a temporary function. It should be replaced with a function which uses RTC.*/
#define MONTH_OF_YEAR 12
#define LEAPMONTH     29
#define NORMALMONTH   28
#define FEBRUARY       1

#define DRTC_RST_MONTH_SHIFT        22	/* shift for month */
#define DRTC_RST_DAY_SHIFT          17	/* shift for day */
#define DRTC_RST_HOUR_SHIFT         12	/* shift for hour */
#define DRTC_RST_MIN_SHIFT          6	/* shift for min */

/* IDPower GPIO */
#define GPIO_PORT_MSM8960_77 77  //Quad

/* IDPower i2c for ADC */
static struct i2c_client *mI2cClient = NULL;

static bool adc_enable_flg = true;      /* IDPower npdc300029442 */
static DEFINE_MUTEX(safety_i2c_mutex);  /* IDPower npdc300034091 */

static int safety_ic_log(struct safety_ic_device_info *di);
static void safety_ic_work_mainfunc(struct work_struct *work);
static irqreturn_t safety_ic_irq_handler(int irq, void *dev_id);
static void safety_ic_fctget_timer_handler(unsigned long data);
static void safety_ic_shutdown_timer_handler(unsigned long data);
static int safety_ic_i2c_probe( struct i2c_client *i2c, const struct i2c_device_id *devid );
static int safety_ic_i2c_remove( struct i2c_client *i2c );
static int safety_ic_i2c_suspend( struct i2c_client *i2c, pm_message_t mesg ); /* IDPower npdc300029442 */
static int safety_ic_i2c_resume( struct i2c_client *i2c );                     /* IDPower npdc300029442 */
static int __init safety_ic_init(void);
static void __exit safety_ic_exit(void);
/* Quad add start */
extern struct mutex wrlss_chg_mutex;
/* Quad add end */

/* pdctl_batchg_spider_log must be called after reading spider2 INT factor registers. */
/* Since EEPROM IDs for spider2 are not implemented yet, they are temporarily commented out. */
static int safety_ic_log(struct safety_ic_device_info *di)
{
	unsigned char count;
	unsigned long index;
	unsigned long tmp_index;
	unsigned char spider_reg[SPIDER_REGSAVE_MAX];
	unsigned long timestamp[SPIDER_TIMESAVE_MAX];
	signed int ret;
        signed int i;
	struct rtc_time tm;
	struct timeval tv;
	ret = 0;
	count = 0;
	tm.tm_mon = 0;
	tm.tm_mday = 0;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;

	DEBUG_SAFETY(("safety_ic_log start\n"));
	
	if (di == NULL) {
		return -EIO;
	}

	ret = cfgdrv_read(D_HCM_E_SPIDER3_ERR_CNT, sizeof(count), &count);

	if (ret < 0) {
		count = 0;
	}

	if (count < SPIDER_COUNT_MAX) {

		index = count % SPIDER_TIMESAVE_MAX;

		ret =
		    cfgdrv_read(D_HCM_E_SPIDER3_ERR_REG_VALUE, sizeof(spider_reg),
				&spider_reg[0]);

		if (ret < 0) {
			dev_err(&di->client->dev,
				"cannot read err reg value : ret= %d\n", ret);
		}

		tmp_index = SPIDER_ERR_REG_NUM * index;
		if ((tmp_index + 1) < SPIDER_REGSAVE_MAX) {
			for(i = 0; i < SPIDER_ERR_REG_NUM ;i++ ){
				spider_reg[tmp_index + i] = safety_ic_val[i];
			}
		}

		ret =
		    cfgdrv_write(D_HCM_E_SPIDER3_ERR_REG_VALUE,
				 sizeof(spider_reg), &spider_reg[0]);

		if (ret < 0) {
			dev_err(&di->client->dev,
				"cannot write err reg value ret= %d\n", ret);
		}



		ret =
		    cfgdrv_read(D_HCM_SPIDER3_ERR_TIMESTAMP, sizeof(timestamp),
				(unsigned char *)&(timestamp[0]));

		if (ret < 0) {
			dev_err(&di->client->dev,
				"cannot read err_timestamp ret= %d\n", ret);
		}
		/*
		   syserr_TimeStamp is a temporary function. It should be replaced with a function which uses RTC.
		   syserr_TimeStamp( &year, &month, &day, &hour, &min, &second );
		 */
		/* which should be used jiffies or get_timeofday? */
		/*        rtc_time_to_tm(jiffies, &tm); */
		do_gettimeofday(&tv);
		rtc_time_to_tm(tv.tv_sec, &tm);

		timestamp[index] =
		    ((((((tm.tm_mon +1) << DRTC_RST_MONTH_SHIFT) |
			((unsigned long)tm.tm_mday << DRTC_RST_DAY_SHIFT)) |
		       ((unsigned long)tm.tm_hour << DRTC_RST_HOUR_SHIFT)) |
		      ((unsigned long)tm.tm_min << DRTC_RST_MIN_SHIFT)) |
		     (unsigned long)tm.tm_sec);

		dev_dbg(&di->client->dev, "jiffies:%ld, timestamp%ld\n", jiffies,
			timestamp[index]);
		dev_dbg(&di->client->dev,
			"month:%d day:%d hour:%d min:%d sec:%d\n", tm.tm_mon+1,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		ret =
		    cfgdrv_write(D_HCM_SPIDER3_ERR_TIMESTAMP, sizeof(timestamp),
				 (unsigned char *)&(timestamp[0]));

		if (ret < 0) {
			dev_err(&di->client->dev, "cannot read ret= %d\n", ret);
		}

		count++;

		ret =
		    cfgdrv_write(D_HCM_E_SPIDER3_ERR_CNT, sizeof(count), &count);

		if (ret < 0) {
			dev_err(&di->client->dev, "cannot write err_cnt ret= %d\n",
				ret);
		}
	}
	DEBUG_SAFETY(("safety_ic_log end\n"));
	return 0;
}


static void safety_ic_work_mainfunc(struct work_struct *work)
{
	struct safety_ic_device_info *di;
	 struct i2c_adapter *adap;
	int ret;
	int irq; /*<iD Power>add*/

	di = container_of(work, struct safety_ic_device_info, work);
	adap = di->client->adapter;
	
	DEBUG_SAFETY(("safety_ic_work_mainfunc start\n"));

	/* safety-ic function select */
	if(di->func_state == DSAFETY_FUNC_INTFACTOR){
		/* safety-ic intfactor regget */
		ret = i2c_transfer(adap, intfactor_msg, 2);

		if(ret < 0){
			dev_err(&di->client->dev, "safety_ic_intfacterget error ret = %d \n", ret);
			return ;
		}

        /* Jessie add start */
        SAFETYIC_DEBUG_LOG(&di->client->dev, "%s:%d safety_ic_val[5]=%02x\n",
                           __func__, __LINE__, safety_ic_val[5]);
        /* Jessie add end */
        	/* safety-ic error check */
		if((safety_ic_val[5] & DSAFETY_FCT_THARD) == DSAFETY_FCT_THARD ){
			del_timer(&safety_ic_fctget_timer);	// Quad add
            /* Jessie add start */
            safety_ic_charge_sts = DSAFETY_CHG_FORCE_STOP;
			/* Quad add start */
			mutex_lock(&wrlss_chg_mutex);
			/* Quad add end */
            gpio_set_value(GPIO_CHG_SUSPEND, 1);
            SAFETYIC_DEBUG_LOG(&di->client->dev, "%s:%d GPIO %d:%d\n",
                               __func__, __LINE__, GPIO_CHG_SUSPEND, 1);
            /* Jessie add end */
			/* Quad add start */
			mdelay(CHG_SUSPEND_WAIT_MSEC);
			mutex_unlock(&wrlss_chg_mutex);
			/* Quad add end */

// Quad del			del_timer(&safety_ic_fctget_timer);
		
			safety_ic_fctget_timer.expires = jiffies + (200*HZ/1000);
			safety_ic_fctget_timer.data = (unsigned long )di;

			add_timer(&safety_ic_fctget_timer);
		}
		else if(((safety_ic_val[5] & DSAFETY_FCT_THR) == DSAFETY_FCT_THR) ||
			((safety_ic_val[5] & DSAFETY_FCT_CHARD) == DSAFETY_FCT_CHARD)){
            /* Jessie add start */
            safety_ic_charge_sts = DSAFETY_CHG_FORCE_STOP;
			/* Quad add start */
			mutex_lock(&wrlss_chg_mutex);
			/* Quad add end */
            gpio_set_value(GPIO_CHG_SUSPEND, 1);
            SAFETYIC_DEBUG_LOG(&di->client->dev, "%s:%d GPIO %d:%d\n",
                               __func__, __LINE__, GPIO_CHG_SUSPEND, 1);
            /* Jessie add end */
			/* Quad add start */
			mdelay(CHG_SUSPEND_WAIT_MSEC);
			mutex_unlock(&wrlss_chg_mutex);
			/* Quad add end */

			ret = i2c_transfer(adap, etcfactor_msg, 2);
			if(ret < 0){
        	        	dev_err(&di->client->dev, "safety_ic_etcfactorget error ret = %d \n", ret);
	                	return ;
        		}
			safety_ic_log(di);
		}
#if 1 /* IDPower */
		else if(((safety_ic_val[5] & DSAFETY_FCT_TSOFT) == DSAFETY_FCT_TSOFT) ||
			((safety_ic_val[5] & DSAFETY_FCT_CSOFT1) == DSAFETY_FCT_CSOFT1) ||
			((safety_ic_val[5] & DSAFETY_FCT_CSOFT2) == DSAFETY_FCT_CSOFT2))
#else /* SlimID */
		else if(((safety_ic_val[5] & DSAFETY_FCT_CSOFT1) == DSAFETY_FCT_CSOFT1) ||
			((safety_ic_val[5] & DSAFETY_FCT_CSOFT2) == DSAFETY_FCT_CSOFT2))
#endif
		{
            del_timer(&safety_ic_shutdown_timer);	// Quad add
            /* Jessie add start */
            safety_ic_charge_sts = DSAFETY_CHG_FORCE_STOP;
			/* Quad add start */
			mutex_lock(&wrlss_chg_mutex);
			/* Quad add end */
            gpio_set_value(GPIO_CHG_SUSPEND, 1);
            SAFETYIC_DEBUG_LOG(&di->client->dev, "%s:%d GPIO %d:%d\n",
                               __func__, __LINE__, GPIO_CHG_SUSPEND, 1);
            /* Jessie add end */
			/* Quad add start */
			mdelay(CHG_SUSPEND_WAIT_MSEC);
			mutex_unlock(&wrlss_chg_mutex);
			/* Quad add end */
			ret = i2c_transfer(adap, etcfactor_msg, 2);
			if(ret < 0){
				dev_err(&di->client->dev, "safety_ic_etcfactorget error2 ret = %d \n", ret);
				return ;
			}
			safety_ic_log(di);

            /* Jessie mod start */
            SAFETYIC_DEBUG_LOG(&di->client->dev,
                               "safety_ic_val[0-3] = %02x %02x %02x %02x \n",
                               safety_ic_val[0], safety_ic_val[1],
                               safety_ic_val[2], safety_ic_val[3]);
#if  0 /* Quad del Start */
            if ((safety_ic_val[1] != NO_FACTOR) || /* CURRENT_FACTOR2 */
                (safety_ic_val[3] != INT_VTH6_SOFT)) { /* TEMP_FACTOR2 */
#endif /* Quad del end */
                /*
                 * Wait timer start.
                 * Beacuse, there is time up to finishing write the safety_ic log.
                 */
// Quad del                del_timer(&safety_ic_shutdown_timer);

                safety_ic_shutdown_timer.data = (unsigned long)di;
                safety_ic_shutdown_timer.expires = jiffies + (300*HZ/1000);

                add_timer(&safety_ic_shutdown_timer);
                SAFETYIC_DEBUG_LOG(&di->client->dev, "wait shutdown\n");
#if  0 /* Quad del Start */
            }
            else {
                /* Don't shut down if cause is VTH6 only. */
                dev_info(&di->client->dev, "suspend wireless charger\n");
            }
#endif /* Quad del end */
            /* Jessie mod end */
		}
#if 0   /* SlimID  add start */
		else if((safety_ic_val[5] & DSAFETY_FCT_TSOFT) == DSAFETY_FCT_TSOFT) {
			ret = i2c_transfer(adap, etcfactor_msg, 2);
			if(ret < 0){
				dev_err(&di->client->dev, "safety_ic_etcfactorget error3 ret = %d \n", ret);
				return ;
			}
			safety_ic_log(di);

			/* Shutdown */
			kernel_power_off();
		}
#endif  /* SlimID add end */
/* Quad add start <npdc300053027> */
		else if((safety_ic_val[5] & DSAFETY_FCT_VEXT) == DSAFETY_FCT_VEXT){
			unsigned char	temp_val = safety_ic_val[5];
			
			ret = i2c_transfer(adap, etcfactor_only_msg, 2);
			if(ret < 0){
				dev_err(&di->client->dev, "safety_ic_etcfactorget error4 ret = %d \n", ret);
				return;
			}
			ret = i2c_transfer(adap, etcfactor_msg, 2);
			if(ret < 0){
				dev_err(&di->client->dev, "safety_ic_etcfactorget error5 ret = %d \n", ret);
				return;
			}
			safety_ic_val[5] = temp_val;
			safety_ic_log(di);
			enable_irq(gpio_to_irq(GPIO_PORT_MSM8960_77));
		}
/* Quad add end <npdc300053027> */
		else
		{
			/* without the occurrence of EXT Int */
            /*<iD Power>add start*/
		    dev_err(&di->client->dev, "INTFACOTR error value = %d \n", safety_ic_val[5]);
            irq = gpio_to_irq(GPIO_PORT_MSM8960_77); //Quad
            enable_irq(irq);
            /*<iD Power>add end*/
		}
	}
	else if(di->func_state == DSAFETY_FUNC_FCTGET){
		/* etcfactor read */
		ret = i2c_transfer(adap, etcfactor_msg, 2);
		if(ret < 0){
			dev_err(&di->client->dev, "safety_ic_etcfactorget error3 ret = %d \n", ret);
			return ;
		}
		else
		{
			dev_err(&di->client->dev, "safety_ic_etcfactorget OK ret = %d \n", ret);
		}
		safety_ic_log(di);
	}
	else if(di->func_state == DSAFETY_FUNC_SHUTDOWN){
		/* shutdown register */
		ret = i2c_transfer(adap, shutdown_msg, 2);
		if(ret < 0){
			dev_err(&di->client->dev, "safety_ic_shutdown error ret = %d \n", ret);
                        return ;
		}
	}
	else{
        /*<iD Power>add*/
		dev_err(&di->client->dev, "func_state error value = %d \n", di->func_state);
	}

	DEBUG_SAFETY(("safety_ic_work_mainfunc end\n"));
	return;
	
}

static irqreturn_t safety_ic_irq_handler(int irq, void *dev_id)
{
	struct safety_ic_device_info *di = dev_id; /* mrc23704, mrc60307 */
	
	DEBUG_SAFETY(("safety_ic_irq_handler start\n"));

	di->func_state = DSAFETY_FUNC_INTFACTOR;

	disable_irq_nosync(irq);

	queue_work(safety_ic_wq, &di->work);

	return IRQ_HANDLED;
}

static void safety_ic_fctget_timer_handler(unsigned long data)
{
	struct safety_ic_device_info *di;
	di = (struct safety_ic_device_info *)data;
	
	DEBUG_SAFETY(("safety_ic_fctget_timer_handler start\n"));

	di->func_state = DSAFETY_FUNC_FCTGET;

	queue_work(safety_ic_wq, &di->work);

	return;
}

static void safety_ic_shutdown_timer_handler(unsigned long data)
{
	struct safety_ic_device_info *di;
	
	DEBUG_SAFETY(("safety_ic_shutdown_timer_handler start\n"));

	di = (struct safety_ic_device_info *)data;

	di->func_state = DSAFETY_FUNC_SHUTDOWN;

	queue_work(safety_ic_wq, &di->work);

	return;
}

/* IDPower start */
int safety_ic_current_temp_adc(unsigned int type, unsigned int channel, int *buf)
{
	unsigned int standvalue = 0x00;
	struct pm8xxx_adc_chan_result result;
	int ret;
	int i;
	
	DEBUG_SAFETY(("safety_ic_current_temp_adc start\n"));
	
/* IDPower npdc300029442 start */
	if(adc_enable_flg == false){
		DEBUG_SAFETY(("safety_ic_current_temp_adc error by busy\n"));
		return -EIO;
	}
/* IDPower npdc300029442 end */
	
	mutex_lock(&safety_i2c_mutex); /* IDPower npdc300034091 */
	
	if(type == DSAFETY_CURRENT_OUT)
	{
		/* current adc */
		standvalue = 0x10;
	}
	else if(type == DSAFETY_TEMP_OUT)
	{
		/* temperature adc */
		standvalue = 0x18;
	}
	else
	{
		/* error */
		dev_err(&mI2cClient->dev, "error type = %d \n", type);
		mutex_unlock(&safety_i2c_mutex); /* IDPower npdc300034091 */
		return -EINVAL;
	}
	
	for(i = 0; i < DSAFETY_ADC_MAX_CHANNELS; i++)
	{
		if((channel & safety_ic_channel[i]) == safety_ic_channel[i])
		{
			/* CPUMODE register write */
			safety_ic_cpumode_buf[1] = standvalue + i;
			DEBUG_SAFETY(("channel = %d, safety_ic_cpumode_buf[1] = 0x%x\n", i, safety_ic_cpumode_buf[1]));
			
			ret = i2c_transfer(mI2cClient->adapter, cpumode_write_msg, 1);
			if(ret < 0){
				dev_err(&mI2cClient->dev, "safety_ic_cpumode error ret = %d \n", ret);
				mutex_unlock(&safety_i2c_mutex); /* IDPower npdc300034091 */
	            return -EIO;
			}
			
			/* VMONI2 wait more than 4.81ms */
			msleep(5);
			
			/* ADC read*/
			safety_adc_flg = true;
			ret = pm8xxx_adc_read(ADC_MPP_1_AMUX6, &result);
			safety_adc_flg = false;
			DEBUG_SAFETY(("pm8xxx_adc_read ret = 0x%x\n", ret));
			if (ret < 0) {
				dev_err(&mI2cClient->dev, "error pm8xxx_adc_read, ret = %d\n", ret);
				mutex_unlock(&safety_i2c_mutex); /* IDPower npdc300034091 */
				return -EIO;
			}
			DEBUG_SAFETY(("adc_code = %d, mvolts phy = %lld meas = 0x%llx\n", result.adc_code, result.physical, result.measurement));
			buf[i] = (int)result.physical;
			DEBUG_SAFETY(("buf[%d] = %d\n", i, buf[i]));
		}
	}
	
	/* CPUMODE register write  Auto mode */
	/* CPUMODEレジスタ（アドレス0xFB）に0x00をWrite */
	safety_ic_cpumode_buf[1] = 0x00;
	ret = i2c_transfer(mI2cClient->adapter, cpumode_write_msg, 1);
	if(ret < 0){
		dev_err(&mI2cClient->dev, "safety_ic_cpumode error ret = %d \n", ret);
		mutex_unlock(&safety_i2c_mutex); /* IDPower npdc300034091 */
		return -EIO;
	}
	
	mutex_unlock(&safety_i2c_mutex); /* IDPower npdc300034091 */
	DEBUG_SAFETY(("safety_ic_current_temp_adc end\n"));
	
	return 0;
}
/* IDPower end */

static int safety_ic_i2c_probe( struct i2c_client *i2c, const struct i2c_device_id *devid )
{
	int ret;
	int irq_no;
	struct safety_ic_device_info *di;

	DEBUG_SAFETY(("safety_ic_i2c_probe start\n"));

	if(i2c == NULL){
		return -EIO;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL); /* mrc23704 */
	if(!di){
		dev_err(&i2c->dev, "safety_ic_device_info is NULL \n");
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, di);
	di->dev = &i2c->dev;
	di->client = i2c;
	mI2cClient = i2c; /* IDPower */

	INIT_WORK(&di->work, safety_ic_work_mainfunc);

        ret = gpio_request(GPIO_PORT_MSM8960_77, "safety_ic_drv"); //Quad
        if (ret != 0) {
                dev_err(&i2c->dev, "safety_ic : can't request gpio ret=%d \n",
			ret);
                return ret;
        }

        ret = gpio_direction_input(GPIO_PORT_MSM8960_77); //Quad
        if (ret != 0) {
                dev_err(&i2c->dev, "safety_ic : can't direction gpio ret=%d \n",
                        ret);
                return ret;
        }

        irq_no = gpio_to_irq(GPIO_PORT_MSM8960_77); //Quad
        if (irq_no < 0) {
		dev_err(&i2c->dev, "safety_ic : can't get irq_no ret=%d \n",
                        irq_no);
                ret = irq_no;
                return ret;
        }

    enable_irq_wake(irq_no);

	ret =
	    request_irq(irq_no, safety_ic_irq_handler, IRQF_TRIGGER_LOW,
			"safety_ic_drv", di);

	if (unlikely(ret)) {
		dev_err(&i2c->dev, "safety_ic : cannot request irq ret=%d \n",
			ret);
		goto request_irq_fail;
	}
	
	DEBUG_SAFETY(("safety_ic_i2c_probe end\n"));

	return 0;

request_irq_fail:
	kfree(di);
	return ret;
}

static int safety_ic_i2c_remove( struct i2c_client *i2c )
{
	DEBUG_SAFETY(("safety_ic_i2c_remove start\n"));
	return 0;
}

/* IDPower npdc300029442 start */
static int safety_ic_i2c_suspend( struct i2c_client *i2c, pm_message_t mesg )
{
	DEBUG_SAFETY(("safety_ic_i2c_suspend start\n"));
	
	adc_enable_flg = false;
	return 0;
}
/* IDPower npdc300029442 end */

/* IDPower npdc300029442 start */
static int safety_ic_i2c_resume( struct i2c_client *i2c )
{
	DEBUG_SAFETY(("safety_ic_i2c_resume start\n"));
	
	adc_enable_flg = true;
	return 0;
}
/* IDPower npdc300029442 end */

static const struct i2c_device_id
        safety_ic_id_table[] = {{"Spider3", 0}, {}};


static struct i2c_driver safety_ic_driver = {
	.probe  = safety_ic_i2c_probe,
	.remove = safety_ic_i2c_remove,
	.suspend = safety_ic_i2c_suspend, /* IDPower npdc300029442 */
	.resume = safety_ic_i2c_resume,   /* IDPower npdc300029442 */
	.driver = {
		   .name = "Spider3",
		   .owner = THIS_MODULE,
		   },
	
	.id_table = safety_ic_id_table,
};

/*
 * Safety-IC Initialization
 */
static int __init safety_ic_init(void)
{
	int ret;

	DEBUG_SAFETY(("safety_ic_init start\n"));
	
	safety_ic_wq = create_singlethread_workqueue("safety_ic_wq");
	if (!safety_ic_wq) {
		return -ENOMEM;
	}

	init_timer(&safety_ic_fctget_timer);
	init_timer(&safety_ic_shutdown_timer);
	safety_ic_fctget_timer.function = (void*)safety_ic_fctget_timer_handler;
	safety_ic_shutdown_timer.function = (void*)safety_ic_shutdown_timer_handler;

/* Jessie add start */
	safety_ic_charge_sts = DSAFETY_CHG_NORMAL;
/* Jessie add end */
	ret = i2c_add_driver(&safety_ic_driver);
	
	DEBUG_SAFETY(("safety_ic_init end\n"));

        return ret;
}

/*
 * Safety-IC Exit
 */
static void __exit safety_ic_exit(void)
{
	DEBUG_SAFETY(("safety_ic_exit start\n"));
	if (safety_ic_wq) {
		destroy_workqueue(safety_ic_wq);
	}
	del_timer(&safety_ic_fctget_timer);
	del_timer(&safety_ic_shutdown_timer);
        i2c_del_driver(&safety_ic_driver);
}


module_init(safety_ic_init);
module_exit(safety_ic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SAFETYIC Driver");
