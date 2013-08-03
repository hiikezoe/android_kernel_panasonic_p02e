/*
*    temperature linux driver
* 
* Usage:    temperature sensor driver by i2c for linux
*
*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
/*#include <asm/uaccess.h> */
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
/*#include <mach/gpio.h>  */
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/poll.h>
/* iD Power DEL start */
/*                */
#if 0
#include <linux/i2c/twl6030-gpadc.h>
#include <linux/i2c/twl.h>
#endif
/* iD Power DEL end */
/* iD Power ADD start */
#include <linux/safety_ic.h>
/* iD Power ADD end */
#include <linux/temperature_driver.h> 
#include <linux/spinlock.h>
/*#include <asm-generic/rtc.h> */
#include <linux/time.h>
#include <linux/tick.h>
#include <asm-generic/percpu.h>
#include <linux/sysfs.h>
#include <linux/cpufreq.h>
#include <linux/gpio.h> /*<npdc300036023>*/
#include <media/msm_camera_temp.h> /* <Quad> */
#include <linux/rtc.h> /* <Quad> *//* npdc300050359 */
#include <linux/rh-fgc.h> /* <Quad> *//* PTC */

/*local define  */
/*#define _TEMP_NVM_DEBUG *//*nvm local             */
/*#define _TEMP_DEBUG  *//*temperature log define   */
/* #define TEMPDD_DEBUG_TEST */

/*console log define */
/* #define HARD_REQUEST_DEBUG */
/* #define HARD_REQUEST_DEBUG_VTH *//* <Quad> *//* npdc300050359 */

/*Time stamp define from foma syserr_timstamp*/
#define FEBRUARY       1
#define MONTH_OF_YEAR 12
#define LEAPMONTH     29
#define NORMALMONTH   28

/*<npdc300036023> start */
/* MSM8960 GPIO Port No */
//#define TEMPDD_LOCAL_MSM_GPIONO1			1 /* <Quad> *//* npdc300051869 *//* disuse */
//#define TEMPDD_LOCAL_MSM_GPIONO15		15 /* <Quad> *//* npdc300051869 *//* disuse */
#define TEMPDD_LOCAL_MSM_GPIONO86		86 /* <Quad> *//* npdc300051869 *//* states of in-camera and out-camera */
/*<npdc300036023> end   */

/* schedule status */
static int omap_schedule_state = 0;
static int temp_schedule_state = 0;

/* Quad ADD start */
extern int pm8921_batt_temperature(void); /* <Quad> *//* PTC */
static int battery_temprature = 0; /* <Quad> *//* PTC */
static int fgic_current = 0; /* <Quad> *//* PTC */
static int vth1_temperature = 0; /* <Quad> *//* npdc300056882 */
module_param_named(batt_temp, battery_temprature, int, 0644); /* <Quad> *//* PTC */
module_param_named(fgic_curr, fgic_current, int, 0644); /* <Quad> *//* PTC */
module_param_named(vth1_temp, vth1_temperature, int, 0644); /* <Quad> *//* npdc300056882 */
/* Quad ADD end */

/******************************************************************************/

struct dev_state_local_t tdev_sts = {            
    .dev_sts = {
                D_TEMPDD_DEV_STS_NORMAL,      /*status battery                */
                D_TEMPDD_DEV_STS_NORMAL,      /*status WRAN                   */
                D_TEMPDD_DEV_STS_NORMAL,      /*status DTV and BackLight      */
                D_TEMPDD_DEV_STS_NORMAL,      /*status Camera                 */
                D_TEMPDD_DEV_STS_NORMAL,      /*status battery_arm            */
                D_TEMPDD_DEV_STS_NORMAL,      /*status battery(WLAN mode)     */
                D_TEMPDD_DEV_STS_NORMAL,      /*status battery(Camera)        */
                D_TEMPDD_DEV_STS_NORMAL,      /*status Camera(WLAN mode)      */
                D_TEMPDD_DEV_STS_NORMAL,      /*status battery_dtv            *//* <Quad> */
                D_TEMPDD_DEV_STS_NORMAL,      /*status Camera(Camera)         *//* <Quad> */
                D_TEMPDD_DEV_STS_NORMAL       /*status OMAP                   */
    },

	.temperature_val = {
		.t_sensor1 = 0,			/*sensor1 temperature           */
		.t_sensor2 = 0,			/*sensor2 temperature           */
		.t_sensor3 = 0,			/*sensor3 temperature           */
		.t_sensor4 = 0				/*sensor4 temperature           */
	},
	.ad_val          ={                           
		.ad_sensor1 = 0,			/*sensor1  AD val               */
		.ad_sensor2 = 0,			/*sensor2  AD val               */
		.ad_sensor3 = 0,			/*sensor3  AD val               */
		.ad_sensor4 = 0 			/*sensor4  AD val               */
	}
};

/*sensor1 status table      */
struct sensor_state_t sensor1_sts={           
    .sensor_sts_old ={                        /*old status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */
    },                                       
    .sensor_sts_new ={                        /*new status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */

    },
    .changing_num={0,0,0,0,0,0,0,0}           /*sensor status chang num       */
};
/*sensor2 status table      */
struct sensor_state_t sensor2_sts={
    .sensor_sts_old ={                        /*old status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */
    },                                       
    .sensor_sts_new ={                        /*new status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */

    },
    .changing_num={0,0,0,0,0,0,0,0}           /*sensor status chang num       */
};
/*sensor3 status table      */
struct sensor_state_t sensor3_sts={       
    .sensor_sts_old ={                        /*old status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */
    },                                       
    .sensor_sts_new ={                        /*new status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */

    },
    .changing_num={0,0,0,0,0,0,0,0}           /*sensor status chang num       */
};
/* Quad ADD start */
/*sensor4 status table      */
struct sensor_state_t sensor4_sts={       
    .sensor_sts_old ={                        /*old status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */
    },                                       
    .sensor_sts_new ={                        /*new status                    */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery                */
        D_TEMPDD_NORMAL_SENSOR,               /*status WRAN                   */
        D_TEMPDD_NORMAL_SENSOR,               /*status DTV and BackLight      */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera                 */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_arm            */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(WLAN mode)     */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery(Camera)        */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(WLAN mode)      */
        D_TEMPDD_NORMAL_SENSOR,               /*status battery_dtv            *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR,               /*status Camera(Camera)         *//* <Quad> */
        D_TEMPDD_NORMAL_SENSOR                /*status OMAP                   */

    },
    .changing_num={0,0,0,0,0,0,0,0}           /*sensor status chang num       */
};
/* Quad ADD end */
/*nvm local table       */
#ifdef  _TEMP_NVM_DEBUG
struct hcmparam temp_hcmparam={
    .vp_heat_suppress = 15,
    .vp_period = 10,
    .enable_time_3g = 20,
    .enable_time_3g_p = 20,
    .enable_time_wlan = 20,
    .enable_time_camera = 20,
    .dev_hcmparam = {
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp={40,40,40},
         .enable_time={3,3,3},
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp={40,40,40},
         .enable_time={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp={40,40,40},
         .enable_time={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp={40,40,40},
         .enable_time={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp ={40,40,40},
         .enable_time ={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp ={40,40,40},
         .enable_time ={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp ={40,40,40},
         .enable_time ={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp ={40,40,40},
         .enable_time ={3,3,3}
        },
        {.disable_temp={50,50,50},
         .disable_time={3,3,3},
         .enable_temp ={40,40,40},
         .enable_time ={3,3,3}
        },
    }
};
#else
struct hcmparam temp_hcmparam;
#endif /*_TEMP_NVM_DEBUG */

struct hcmsensor_omap_param temperature_omap_hcmparam[] = {
	{
		.id = D_TEMPDD_DEV_STS_NORMAL,
		.abs_temp_th = 0,
		.rel_temp_th = 0,
		.abs_cnt_th = 0,
		.rel_cnt_th = 0,
		.timer = 4
	},
	{
		.id = D_TEMPDD_DEV_STS_ABNORMAL,
		.abs_temp_th = 0x0,
		.rel_temp_th = 0x0,
		.abs_cnt_th = 0xFF,
		.rel_cnt_th = 0xFF,
		.timer = 2
	},
	{
		.id = D_TEMPDD_DEV_STS_ABNORMAL_L2,
		.abs_temp_th = 0x0,
		.rel_temp_th = 0x0,
		.abs_cnt_th = 0xFF,
		.rel_cnt_th = 0xFF,
		.timer = 1
	},
	{
		.id = D_TEMPDD_DEV_STS_ABNORMAL_L3,
		.abs_temp_th = 0x0,
		.rel_temp_th = 0x0,
		.abs_cnt_th = 0xFF,
		.rel_cnt_th = 0xFF,
		.timer = 1
	}
};

/*counter of over threshold*/
static unsigned int temperature_over_counter[] = {
	0,	/*abs_temp & disable */
	0,	/*rel_temp & disable */
	0,	/*abs_temp & enable */
	0	/*rel_temp & enable */
};

/*millor table for omap temperatuer detection*/
static unsigned int temperature_millor_max = 0;
static struct hcmsensor_omap_param 
			*temperature_millor_param[D_TEMPDD_DEV_STS_NUM];

#ifdef _TEMP_NVM_DEBUG
#define D_HCM_E_VP_HEAT_SUPPRESS 0
#define D_HCM_E_VP_PERIOD 0
#define D_HCM_E_VP_CHG_ENABLE_OFFHOOK_TIME 0
#define D_HCM_E_VP_CHG_DISABLE_TEMP1 0
#define D_HCM_E_VP_CHG_DISABLE_TEMP2 0
#define D_HCM_E_VP_CHG_DISABLE_TEMP3 0
#define D_HCM_E_VP_CHG_DISABLE_TIME1 0
#define D_HCM_E_VP_CHG_DISABLE_TIME2 0
#define D_HCM_E_VP_CHG_DISABLE_TIME3 0
#define D_HCM_E_VP_CHG_ENABLE_TEMP1 0
#define D_HCM_E_VP_CHG_ENABLE_TEMP2 0
#define D_HCM_E_VP_CHG_ENABLE_TEMP3 0
#define D_HCM_E_VP_CHG_ENABLE_TIME1 0
#define D_HCM_E_VP_CHG_ENABLE_TIME2 0
#define D_HCM_E_VP_CHG_ENABLE_TIME3 0
#define D_HCM_E_VP_WLAN_DISABLE_TEMP1 0
#define D_HCM_E_VP_WLAN_DISABLE_TEMP2 0
#define D_HCM_E_VP_WLAN_DISABLE_TEMP3 0
#define D_HCM_E_VP_WLAN_DISABLE_TIME1 0
#define D_HCM_E_VP_WLAN_DISABLE_TIME2 0
#define D_HCM_E_VP_WLAN_DISABLE_TIME3 0
#define D_HCM_E_VP_WLAN_ENABLE_TEMP1 0
#define D_HCM_E_VP_WLAN_ENABLE_TEMP2 0
#define D_HCM_E_VP_WLAN_ENABLE_TEMP3 0
#define D_HCM_E_VP_WLAN_ENABLE_TIME1 0
#define D_HCM_E_VP_WLAN_ENABLE_TIME2 0
#define D_HCM_E_VP_WLAN_ENABLE_TIME3 0
#define D_HCM_E_VP_OMAP_DISABLE_TEMP1 0
#define D_HCM_E_VP_OMAP_DISABLE_TEMP2 0
#define D_HCM_E_VP_OMAP_DISABLE_TEMP3 0
#define D_HCM_E_VP_OMAP_DISABLE_TIME1 0
#define D_HCM_E_VP_OMAP_DISABLE_TIME2 0
#define D_HCM_E_VP_OMAP_DISABLE_TIME3 0
#define D_HCM_E_VP_OMAP_ENABLE_TEMP1 0
#define D_HCM_E_VP_OMAP_ENABLE_TEMP2 0
#define D_HCM_E_VP_OMAP_ENABLE_TEMP3 0
#define D_HCM_E_VP_OMAP_ENABLE_TIME1 0
#define D_HCM_E_VP_OMAP_ENABLE_TIME2 0
#define D_HCM_E_VP_OMAP_ENABLE_TIME3 0
#define D_HCM_E_VP_CAM_DISABLE_TEMP1 0
#define D_HCM_E_VP_CAM_DISABLE_TEMP2 0
#define D_HCM_E_VP_CAM_DISABLE_TEMP3 0
#define D_HCM_E_VP_CAM_DISABLE_TIME1 0
#define D_HCM_E_VP_CAM_DISABLE_TIME2 0
#define D_HCM_E_VP_CAM_DISABLE_TIME3 0
#define D_HCM_E_VP_CAM_ENABLE_TEMP1 0
#define D_HCM_E_VP_CAM_ENABLE_TEMP2 0
#define D_HCM_E_VP_CAM_ENABLE_TEMP3 0
#define D_HCM_E_VP_CAM_ENABLE_TIME1 0
#define D_HCM_E_VP_CAM_ENABLE_TIME2 0
#define D_HCM_E_VP_CAM_ENABLE_TIME3 0

#define D_HCM_E_VP_AMRCHG_DISABLE_TEMP1 0
#define D_HCM_E_VP_AMRCHG_DISABLE_TEMP2 0
#define D_HCM_E_VP_AMRCHG_DISABLE_TEMP3 0
#define D_HCM_E_VP_AMRCHG_DISABLE_TIME1 0
#define D_HCM_E_VP_AMRCHG_DISABLE_TIME2 0
#define D_HCM_E_VP_AMRCHG_DISABLE_TIME3 0
#define D_HCM_E_VP_AMRCHG_ENABLE_TEMP1 0
#define D_HCM_E_VP_AMRCHG_ENABLE_TEMP2 0
#define D_HCM_E_VP_AMRCHG_ENABLE_TEMP3 0
#define D_HCM_E_VP_AMRCHG_ENABLE_TIME1 0
#define D_HCM_E_VP_AMRCHG_ENABLE_TIME2 0
#define D_HCM_E_VP_AMRCHG_ENABLE_TIME3 0
#define D_HCM_E_VP_CHG_DISABLE_TEMP4 0
#define D_HCM_E_VP_CHG_DISABLE_TEMP5 0
#define D_HCM_E_VP_CHG_DISABLE_TEMP6 0
#define D_HCM_E_VP_CHG_DISABLE_TIME4 0
#define D_HCM_E_VP_CHG_DISABLE_TIME5 0
#define D_HCM_E_VP_CHG_DISABLE_TIME6 0
#define D_HCM_E_VP_CHG_ENABLE_TEMP4 0
#define D_HCM_E_VP_CHG_ENABLE_TEMP5 0
#define D_HCM_E_VP_CHG_ENABLE_TEMP6 0
#define D_HCM_E_VP_CHG_ENABLE_TIME4 0
#define D_HCM_E_VP_CHG_ENABLE_TIME5 0
#define D_HCM_E_VP_CHG_ENABLE_TIME6 0
#define D_HCM_E_VP_OMAP_DISABLE_TEMP4 0
#define D_HCM_E_VP_OMAP_DISABLE_TEMP5 0
#define D_HCM_E_VP_OMAP_DISABLE_TEMP6 0
#define D_HCM_E_VP_OMAP_DISABLE_TIME4 0
#define D_HCM_E_VP_OMAP_DISABLE_TIME5 0
#define D_HCM_E_VP_OMAP_DISABLE_TIME6 0
#define D_HCM_E_VP_OMAP_ENABLE_TEMP4 0
#define D_HCM_E_VP_OMAP_ENABLE_TEMP5 0
#define D_HCM_E_VP_OMAP_ENABLE_TEMP6 0
#define D_HCM_E_VP_OMAP_ENABLE_TIME4 0
#define D_HCM_E_VP_OMAP_ENABLE_TIME5 0
#define D_HCM_E_VP_OMAP_ENABLE_TIME6 0
#define D_HCM_E_VP_CAM_DISABLE_TEMP4 0
#define D_HCM_E_VP_CAM_DISABLE_TEMP5 0
#define D_HCM_E_VP_CAM_DISABLE_TEMP6 0
#define D_HCM_E_VP_CAM_DISABLE_TIME4 0
#define D_HCM_E_VP_CAM_DISABLE_TIME5 0
#define D_HCM_E_VP_CAM_DISABLE_TIME6 0
#define D_HCM_E_VP_CAM_ENABLE_TEMP4 0
#define D_HCM_E_VP_CAM_ENABLE_TEMP5 0
#define D_HCM_E_VP_CAM_ENABLE_TEMP6 0
#define D_HCM_E_VP_CAM_ENABLE_TIME4 0
#define D_HCM_E_VP_CAM_ENABLE_TIME5 0
#define D_HCM_E_VP_CAM_ENABLE_TIME6 0
int cfgdrv_read( unsigned short id, unsigned short no, unsigned char *data )
{
    return 0;
}
#endif /*_TEMP_NVM_DEBUG */
#ifndef _TEMP_NVM_DEBUG
#include <linux/cfgdrv.h>
#endif /*_TEMP_NVM_DEBUG */
              

/* temperature driver main status       */
int temperature_main_sts =D_TEMPDD_TM_STS_NORMAL;  
int temperature_omap_main_sts = D_TEMPDD_TM_STS_NORMAL;

/*wait queue             */
static wait_queue_head_t tempwait_q;
static wait_queue_head_t temperature_omap_wait_q;

/*timer list            */
static struct timer_list temperature_pollingtimer; /*polling          */
static struct timer_list temperature_1sectimer;    /*1sec timer       */
static struct timer_list temperatuer_waittimer;    /*wait timer       */
static struct timer_list temperature_omap_timer; 
					/*polling for OMAP      */
static struct timer_list temperature_adconv_timer;
					/*AD conversion timer*/
/*work queue           */
static struct work_struct tempwork_1secpollingtask; 
static struct work_struct tempwork_pollingtask; 
static struct work_struct temperature_omap_worktask; 
static struct work_struct temperature_retry_worktask;
static struct work_struct temperature_retry_worktask_omap;
/*AD conversion flag*/
static unsigned int temperature_adconv_flg = 0;
static unsigned int temperature_adconv_type = 0;

/*spin lock*/
static spinlock_t     temp_spinlock;

/* multi open control */
static int access_num = D_TEMPDD_FLG_OFF;

/*timer flg */
int pollingtimer_stopflg = D_TEMPDD_FLG_OFF;
int onesectimer_stopflg    = D_TEMPDD_FLG_OFF;

/*previous state*/
static unsigned int temperature_prev_state_abs = D_TEMPDD_DEV_STS_NORMAL;
static unsigned int temperature_prev_state_rel = D_TEMPDD_DEV_STS_NORMAL;

/*write log flag*/
static unsigned int temperature_write_log_flg = 0;

/*previous cam power state*//*<npdc300036023>*/
static unsigned int temperature_prev_cam_power_state = 0; /*<npdc300036023>*/

static DEFINE_PER_CPU(struct cpu_dbs_info_s, hp_cpu_dbs_info);

#ifdef CONFIG_SYSFS
#ifdef TEMPDD_DEBUG_TEST
/*kobject*/
static struct kobject *temperature_kobj;
#endif /*TEMPDD_DEBUG_TEST*/
#endif /*CONFIG_SYSFS*/

/*device check flag */
static int check_dev_flg = 0x00;

/* device monitoring target table */
static const int judge_watch_device_table[D_TEMPDD_DEVNUM_LOCAL] =
{
    M_BATTERY,
    M_WLAN,
    M_DTV_BKL,
    M_CAMERA,
    M_BATTERY_AMR,
    M_BATTERY_WLAN,
    M_BATTERY_CAMERA,
    M_CAMERA_WLAN,
/* Quad ADD start */
    M_BATTERY_DTV,   /* <Quad> *//* npdc300050675 */
    M_CAMERA_CAMERA, /* <Quad> *//* npdc300050675 */
/* Quad ADD end */
    M_OMAP
};

/* wakeup flag */
static unsigned int wakeup_flg_3g = 0;
static unsigned int wakeup_flg_omap = 0;
static unsigned int timestamp_write_log_flg = 0;     /* <Quad> */

/*--------------------------------------------------------------------------*
 * print
 *--------------------------------------------------------------------------*/
#define _debug( f, a ) \
{   \
    /*printk(__FILE__": ");*/  \
    printk( (f) );    \
    printk( (a) );    \
    printk( "\n" );    \
}

#ifdef  _TEMP_DEBUG
#define     _DBG_MSG1
#define     _DBG_MSG2
#define     _DBG_MSG3
#define     _DUMP_MSG
#define     _UNUSUAL
#define     _ERR_MSG
#endif  /*_TEMP_DEBUG*/

#ifdef _DBG_MSG1
#define MODULE_TRACE_START(void)   _debug((__func__), " Function Start")
#define MODULE_TRACE_END(void)     _debug((__func__), " Function End")
#else
#define MODULE_TRACE_START(void)
#define MODULE_TRACE_END(void)
#endif  /*_DBG_MSG1*/

#ifdef _DBG_MSG2
#define ROUTE_CHECK(a)       _debug((__func__), (a))
#else
#define ROUTE_CHECK(a)
#endif  /*_DBG_MSG2*/

#ifdef _DBG_MSG3
#define DBG_MSG(a, b)    \
{   \
    /*printk(__FILE__": ");*/   \
    printk( (__func__) );    \
    printk( (a) );    \
    /*printk("0x%08x\n", b );*/ printk("%d\n", (int)b ); \
}
#define DBG_PRINT(fmt, arg...) {printk(__func__ ); printk(" " fmt, ##arg);}
#define DBG_LMSG(a, b)    \
{   \
    /*printk(__FILE__": ");*/   \
    printk( (__func__) );    \
    printk( (a) );    \
    printk("0x%lx\n", b );    \
}
#define DBG_HEXMSG(a, b)    \
{   \
    /*printk(__FILE__": ");*/   \
    printk( (__func__) );    \
    printk( (a) );    \
    printk("0x%x\n", b );    \
}
#else
#define DBG_MSG(a, b)
#define DBG_PRINT(fmt, arg...)
#define DBG_LMSG(a, b)
#define DBG_HEXMSG(a, b)
#endif  /*_DBG_MSG3*/


/******************************************************************************/
/* ROM table                                                                 */
/******************************************************************************/


/* iD Power MOD start */
/* AD                 */
static const _D_TEMPDD_ADCHANGE_TABLE AdChangeTable1[] = /* VTH 1/4/7    */
{
    { 0x00D ,   1 },
    { 0x00E ,   2 },
    { 0x00E ,   3 },
    { 0x00F ,   4 },
    { 0x010 ,   5 },
    { 0x011 ,   6 },
    { 0x012 ,   7 },
    { 0x013 ,   8 },
    { 0x014 ,   9 },
    { 0x015 ,  10 },
    { 0x016 ,  11 },
    { 0x017 ,  12 },
    { 0x019 ,  13 },
    { 0x01A ,  14 },
    { 0x01C ,  15 },
    { 0x01D ,  16 },
    { 0x01F ,  17 },
    { 0x020 ,  18 },
    { 0x022 ,  19 },
    { 0x024 ,  20 },
    { 0x026 ,  21 },
    { 0x028 ,  22 },
    { 0x02A ,  23 },
    { 0x02C ,  24 },
    { 0x02E ,  25 },
    { 0x030 ,  26 },
    { 0x033 ,  27 },
    { 0x035 ,  28 },
    { 0x038 ,  29 },
    { 0x03B ,  30 },
    { 0x03E ,  31 },
    { 0x041 ,  32 },
    { 0x044 ,  33 },
    { 0x047 ,  34 },
    { 0x04B ,  35 },
    { 0x04E ,  36 },
    { 0x052 ,  37 },
    { 0x056 ,  38 },
    { 0x05A ,  39 },
    { 0x05F ,  40 },
    { 0x063 ,  41 },
    { 0x068 ,  42 },
    { 0x06C ,  43 },
    { 0x071 ,  44 },
    { 0x077 ,  45 },
    { 0x07C ,  46 },
    { 0x082 ,  47 },
    { 0x088 ,  48 },
    { 0x08E ,  49 },
    { 0x094 ,  50 },
    { 0x09B ,  51 },
    { 0x0A1 ,  52 },
    { 0x0A9 ,  53 },
    { 0x0B0 ,  54 },
    { 0x0B8 ,  55 },
    { 0x0C0 ,  56 },
    { 0x0C8 ,  57 },
    { 0x0D1 ,  58 },
    { 0x0DA ,  59 },
    { 0x0E3 ,  60 },
    { 0x0ED ,  61 },
    { 0x0F7 ,  62 },
    { 0x101 ,  63 },
    { 0x10C ,  64 },
    { 0x117 ,  65 },
    { 0x122 ,  66 },
    { 0x12E ,  67 },
    { 0x13B ,  68 },
    { 0x147 ,  69 },
    { 0x155 ,  70 },
    { 0x163 ,  71 },
    { 0x171 ,  72 },
    { 0x17F ,  73 },
    { 0x18E ,  74 },
    { 0x19E ,  75 },
    { 0x1AE ,  76 },
    { 0x1BF ,  77 },
    { 0x1D0 ,  78 },
    { 0x1E2 ,  79 },
    { 0x1F4 ,  80 },
    { 0x208 ,  81 },
    { 0x21B ,  82 },
    { 0x230 ,  83 },
    { 0x245 ,  84 },
    { 0x25A ,  85 },
    { 0x271 ,  86 },
    { 0x288 ,  87 },
    { 0x2A0 ,  88 },
    { 0x2B8 ,  89 },
    { 0x2D2 ,  90 },
    { 0x2EC ,  91 },
    { 0x307 ,  92 },
    { 0x323 ,  93 },
    { 0x340 ,  94 },
    { 0x35D ,  95 },
    { 0x37B ,  96 },
    { 0x39B ,  97 },
    { 0x3BB ,  98 },
    { 0x3DC ,  99 },
    { 0x3FE , 100 },
    { 0x422 , 101 },
    { 0x446 , 102 },
    { 0x46B , 103 },
    { 0x491 , 104 },
    { 0x4B8 , 105 },
    { 0x4E0 , 106 },
    { 0x509 , 107 },
    { 0x533 , 108 },
    { 0x55E , 109 },
    { 0x58B , 110 },
    { 0x5B8 , 111 },
    { 0x5E8 , 112 },
    { 0x618 , 113 },
    { 0x64A , 114 },
    { 0x67D , 115 },
    { 0x6B1 , 116 },
    { 0x6E7 , 117 },
    { 0x71F , 118 },
    { 0x758 , 119 },
    { 0x792 , 120 },
    {     0 ,   0 }
};
/* iD Power MOD end */

static unsigned int temperature_adtable1_num = 0;

/******************************************************************************/
/* function                                                                   */
/******************************************************************************/
                                               /*driver open                  */
static int temperature_open(struct inode *inode, struct file *filp);
                                               /*1sec timer handler           */
static void temperature_1sectimer_handler(unsigned long data);
                                               /*polling timer handler        */
static void temperature_pollingtimer_handler( unsigned long data );
                                               /*wait timer handler           */
static void temperature_waittimer_handler( unsigned long data );
                                               /*ioctl                        */
/* iD Power MOD start */
/* Kernel                                 */
/* static int temperature_ioctl(struct inode *inode, struct file *filp,
            unsigned int cmd, unsigned long arg); */
static long temperature_ioctl(struct file *filp,
            unsigned int cmd, unsigned long arg);
/* iD Power MOD end */
static int temperature_hcm_read(void);                /*nvm read                     */
static int temperature_check_start(void);             /*temperature check start      */
static int temperature_check_workcb(int len, int channels, int *buf); 
                                               /*twl6030 callback              */
static int temperature_adchange(int buf, unsigned int channel); /*AD -> temperature change*/
static int temperature_twl_conv(void);                /*AD get                       */

static int __init temperature_init(void);      /*init                     */
static void __exit temperature_cleanup(void);  /*cleanup                  */
                                               /*driver poll             */
static unsigned int temperature_poll(struct file *filp, poll_table *wait);
                                               /*close                    */
static int temperature_release(struct inode *inode, struct file *filp);
                                               /*time stamp       */
void syserr_TimeStamp2( unsigned long *year,
                       unsigned long *month,
                       time_t *day,
                       time_t *hour,
                       time_t *min,
                       time_t *sec );


static void temperature_omap_timer_handler( unsigned long data );
static int temperature_omap_twl_conv(void);
static int temperature_omap_check_workcb(int len, int channels, int *buf);
static inline cputime64_t get_cpu_idle_time(unsigned int cpu,
					      cputime64_t *wall);
static unsigned int temperature_get_cpu_load(void);
static void temperature_set_millor_table(void);
static signed int temperature_omap_disable(signed int temp,
					     unsigned int prev_state,
					     unsigned int type);
static signed int temperature_omap_enable(signed int temp,
					     unsigned int prev_state,
					     unsigned int type);
static signed int temperature_omap_decide_state(signed short omap_temp,
						   signed short env_temp);
static void temperature_retry_handler(unsigned long data);
static int temperature_write_log(unsigned int state, int cnt_enable);

static void temperature_ioctl_start(int status);
static void temperature_ioctl_end(int enable_time);

static unsigned int temperature_w_cfg_log_chg_normal_omap(void);    /* <Quad> */
static unsigned int temperature_w_cfg_log_chg_normal(int dev_type); /* <Quad> */

/******************************************************************************/
/* debug function							      */
/******************************************************************************/
#ifdef CONFIG_SYSFS
#ifdef TEMPDD_DEBUG_TEST
static ssize_t temperature_debug_store_oppnitro(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_store_oppturbo(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_store_opp100(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_store_opptime(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_show_status(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf);
static ssize_t temperature_debug_store_start(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_store_test(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_store_data_init(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t temperature_debug_show_debugdata(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf);

static ssize_t temperature_debug_store_oppnitro(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	signed   int   buf_digit[2];
	unsigned int   buf_digit_u[2];

	sscanf(buf, "%d%d%u%u", &buf_digit[0], &buf_digit[1],
				 &buf_digit_u[0], &buf_digit_u[1]);

	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.abs_temp_th = buf_digit[0];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.rel_temp_th = buf_digit[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
				.abs_cnt_th = (unsigned char)buf_digit_u[0];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
				.rel_cnt_th = (unsigned char)buf_digit_u[1];

	printk(KERN_EMERG"OPP_NITRO抑止開始/停止用閾値を設定しました\n");
	printk(KERN_EMERG"絶対温度閾値 = %d\n",(signed int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].abs_temp_th);
	printk(KERN_EMERG"相対温度閾値 = %d\n",(signed int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].rel_temp_th);
	printk(KERN_EMERG"絶対温度確定回数 = %u\n",(unsigned int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].abs_cnt_th);
	printk(KERN_EMERG"相対温度確定回数 = %u\n",(unsigned int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].rel_cnt_th);

	return count;
}

static ssize_t temperature_debug_store_oppturbo(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	signed   int   buf_digit[2];
	unsigned int   buf_digit_u[2];

	sscanf(buf, "%d%d%u%u", &buf_digit[0], &buf_digit[1],
				 &buf_digit_u[0], &buf_digit_u[1]);

	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.abs_temp_th = buf_digit[0];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.rel_temp_th = buf_digit[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
				.abs_cnt_th = (unsigned char)buf_digit_u[0];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
				.rel_cnt_th = (unsigned char)buf_digit_u[1];

	printk(KERN_EMERG"OPP_TURBO抑止開始/停止用閾値を設定しました\n");
	printk(KERN_EMERG"絶対温度閾値 = %d\n",(signed int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].abs_temp_th);
	printk(KERN_EMERG"相対温度閾値 = %d\n",(signed int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].rel_temp_th);
	printk(KERN_EMERG"絶対温度確定回数 = %u\n",(unsigned int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].abs_cnt_th);
	printk(KERN_EMERG"相対温度確定回数 = %u\n",(unsigned int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].rel_cnt_th);

	return count;

}

static ssize_t temperature_debug_store_opp100(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	signed   int   buf_digit[2];
	unsigned int   buf_digit_u[2];

	sscanf(buf, "%d%d%u%u", &buf_digit[0], &buf_digit[1],
				 &buf_digit_u[0], &buf_digit_u[1]);

	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.abs_temp_th = buf_digit[0];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.rel_temp_th = buf_digit[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
				.abs_cnt_th = (unsigned char)buf_digit_u[0];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
				.rel_cnt_th = (unsigned char)buf_digit_u[1];

	printk(KERN_EMERG"OPP_100抑止開始/停止用閾値を設定しました\n");
	printk(KERN_EMERG"絶対温度閾値 = %d\n",(signed int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].abs_temp_th);
	printk(KERN_EMERG"相対温度閾値 = %d\n",(signed int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].rel_temp_th);
	printk(KERN_EMERG"絶対温度確定回数 = %u\n",(unsigned int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].abs_cnt_th);
	printk(KERN_EMERG"相対温度確定回数 = %u\n",(unsigned int)
	   temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].rel_cnt_th);
 
	return count;

}

static ssize_t temperature_debug_store_opptime(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	signed int buf_digit[D_TEMPDD_DEV_STS_NUM];
	int i;
	
	sscanf(buf, "%d%d%d%d", &buf_digit[0], &buf_digit[1],
				&buf_digit[2], &buf_digit[3] );

	for ( i = 0; i < D_TEMPDD_DEV_STS_NUM; i++ )
		temperature_omap_hcmparam[ i ].timer = buf_digit[ i ];

	printk(KERN_EMERG"温度センサ取得周期一覧\n");
	printk(KERN_EMERG"-----------------------------------------------------"
			"--------------------------------------------\n");
	printk(KERN_EMERG"|                 |  通常  \t|  Nitoro\t|"
			"  Turbo\t| OPP100\t|\n");
	printk(KERN_EMERG"------------------+----------------------------------"
			"--------------------------------------------\n");
	printk(KERN_EMERG"| 周期(秒)        |  %d\t\t|  %d\t\t|"
			"  %d\t\t|  %d\t\t|\n",
			(unsigned int)temperature_omap_hcmparam[ 0 ].timer,
			(unsigned int)temperature_omap_hcmparam[ 1 ].timer,
			(unsigned int)temperature_omap_hcmparam[ 2 ].timer,
			(unsigned int)temperature_omap_hcmparam[ 3 ].timer);

	return count;
}

static ssize_t temperature_debug_show_status(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	printk(KERN_EMERG"OPP抑止閾値一覧\n");
	printk(KERN_EMERG"-----------------------------------------------------"
			"------------\n");
	printk(KERN_EMERG"|                 |  Nitoro\t|"
			"  Turbo\t| OPP100\t|\n");
	printk(KERN_EMERG"-----------------------------------------------------"
			"------------\n");
	printk(KERN_EMERG"| 絶対温度閾値    |  %d\t\t|  %d\t\t|  %d\t\t|\n",
				(signed int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL].abs_temp_th,
				(signed int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L2].abs_temp_th,
				(signed int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L3].abs_temp_th );
	printk(KERN_EMERG"| 相対温度閾値    |  %d\t\t|  %d\t\t|  %d\t\t|\n",
				(signed int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL].rel_temp_th,
				(signed int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L2].rel_temp_th,
				(signed int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L3].rel_temp_th );
	printk(KERN_EMERG"| 確定回数(絶対)  |  %u\t\t|  %u\t\t|  %u\t\t|\n",
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL].abs_cnt_th,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L2].abs_cnt_th,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L3].abs_cnt_th );
	printk(KERN_EMERG"| 確定回数(相対)  |  %u\t\t|  %u\t\t|  %u\t\t|\n",
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL].rel_cnt_th,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L2].rel_cnt_th,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L3].rel_cnt_th );    
	printk(KERN_EMERG"-----------------------------------------------------"
			"-------------\n");
	printk(KERN_EMERG"温度センサ取得周期一覧\n");
	printk(KERN_EMERG"-----------------------------------------------------"
			"-----------------------\n");
	printk(KERN_EMERG"|                 |  通常  \t|  Nitoro\t|"
			"  Turbo\t| OPP100\t|\n");
	printk(KERN_EMERG"-----------------------------------------------------"
			"-----------------------\n");
	printk(KERN_EMERG"| 周期(秒)        |  %d\t\t|  %d\t\t|"
				"  %d\t\t|  %d\t\t|\n",
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_NORMAL].timer,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL].timer,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L2].timer,
				(unsigned int)temperature_omap_hcmparam
				[D_TEMPDD_DEV_STS_ABNORMAL_L3].timer);
	printk(KERN_EMERG"-----------------------------------------------------"
			"-----------------------\n");

	printk(KERN_EMERG"----------------内部情報------------------------------"
			"--------\n");
	printk(KERN_EMERG"OMAP センサ値 = 0x%x  周辺センサ値 = 0x%x \n",
		tdev_sts.ad_val.ad_sensor3 , tdev_sts.ad_val.ad_sensor4 );
	printk(KERN_EMERG"OPP 抑止状態");
	switch(tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX])
	{
		case D_TEMPDD_DEV_STS_NORMAL:
			printk(KERN_EMERG"------>通常状態\n");
			break;
		case D_TEMPDD_DEV_STS_ABNORMAL:
			printk(KERN_EMERG"------>OPP-Nitoro抑止状態\n");
			break;
		case D_TEMPDD_DEV_STS_ABNORMAL_L2:
			printk(KERN_EMERG"------>OPP-Turbo抑止状態\n");
			break;
		case D_TEMPDD_DEV_STS_ABNORMAL_L3:
			printk(KERN_EMERG"------>OPP-100抑止状態\n");
			break;
		default:
			printk(KERN_EMERG"------>ERROR!!\n");
			break;
	}

	return 0;
}

static ssize_t temperature_debug_store_start(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long init_flg;
	unsigned long flags;
	
	sscanf(buf, "%u", (unsigned int*)&init_flg);

	if (init_flg == 1) {
		temperature_set_millor_table();
		printk(KERN_EMERG"***********TWL6030 AD---->START \n");
		spin_lock_irqsave(&temp_spinlock,flags);
		/*main status chenge to WATCH            */
		temperature_omap_main_sts = D_TEMPDD_TM_STS_WATCH; 
		spin_unlock_irqrestore(&temp_spinlock,flags);
		/*start OMAP temperature detection           */
		temperature_omap_twl_conv();
	} else if(init_flg == 0) {
		spin_lock_irqsave(&temp_spinlock,flags);
		/*change main status of omap detection*/
		temperature_omap_main_sts = D_TEMPDD_TM_STS_NORMAL; 
		spin_unlock_irqrestore(&temp_spinlock,flags);
		
		/*initialize previous state*/
		temperature_prev_state_abs = D_TEMPDD_DEV_STS_NORMAL;
		temperature_prev_state_rel = D_TEMPDD_DEV_STS_NORMAL;
		/*stop timer*/                
		del_timer(&temperature_omap_timer);
		printk(KERN_EMERG"***********TWL6030 AD---->END \n");
#ifdef CONFIG_CPU_FREQ_GOV_HOTPLUG
		/*reset cpufreq*//*debug*/
		cpufreq_frequency_opp_stop(D_TEMPDD_DEV_STS_NORMAL);
#endif /* CONFIG_CPU_FREQ_GOV_HOTPLUG */
	} else {
		printk(KERN_EMERG"usage:\n  echo 1 > temperture_start : "
				"start omap temperature detection\n");
		printk(KERN_EMERG"  echo 0 > temperture_start : stop omap "
				"temperature detection\n");
	}

	return count;

}

static ssize_t temperature_debug_store_test(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	signed int abs_temp;
	signed int rel_temp;
	signed int result;
 
	sscanf(buf, "%d%d", &abs_temp, &rel_temp);
	
	printk(KERN_EMERG"abs_temp = %d, rel_temp = %d\n", abs_temp, rel_temp );
	
	temperature_set_millor_table();
    /* iD Power MOD start */
    /*                      */
/*	result = temperature_omap_decide_state((signed short)abs_temp,
					(signed short)abs_temp - rel_temp);*/
	result = temperature_omap_decide_state((signed short)abs_temp,
					0);
    /* iD Power MOD end */
	tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX] = result;
	printk(KERN_EMERG"result = %d\n", result );
	
	return count;

}

static ssize_t temperature_debug_show_debugdata(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	int i;

	printk(KERN_EMERG"adtable1_num = %d\n",temperature_adtable1_num);
	printk(KERN_EMERG"prev_state_abs = %u\n",temperature_prev_state_abs);
	printk(KERN_EMERG"prev_state_rel = %u\n",temperature_prev_state_rel);

	for (i = 0; i < 4; i++)
		printk(KERN_EMERG"count[%d] = %u\n",
						i,temperature_over_counter[i]);

	temperature_set_millor_table();

	printk("\n millor table\n ");
	for(i = 0; i < temperature_millor_max; i++){
		printk(KERN_EMERG"************ millor_table[%u] ***************"
				"*******\n", (unsigned int) i);
		printk(KERN_EMERG"id = %d",temperature_millor_param[i]->id);
		printk(KERN_EMERG"abs_temp_th = %d\n",
				temperature_millor_param[i]->abs_temp_th);
		printk(KERN_EMERG"rel_temp_th = %u\n",
				temperature_millor_param[i]->rel_temp_th);
		printk(KERN_EMERG"abs_cnt_th = %u\n",
				temperature_millor_param[i]->abs_cnt_th);
		printk(KERN_EMERG"rel_cnt_th = %u\n",
				temperature_millor_param[i]->rel_cnt_th);
	}
	printk(KERN_EMERG"millor_num = %d\n",temperature_millor_max);
	
	return 0;
}

static ssize_t temperature_debug_store_data_init(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int init_flg;
	unsigned int i;
	unsigned long flags;

	sscanf(buf, "%u", &init_flg);

	if( init_flg == 1 )
	{
		printk(KERN_EMERG"initialize temperature data\n");
		spin_lock_irqsave(&temp_spinlock,flags);
		/*change main status of omap detection*/
		temperature_omap_main_sts = D_TEMPDD_TM_STS_NORMAL; 
		spin_unlock_irqrestore(&temp_spinlock,flags);
		
		temperature_prev_state_abs = D_TEMPDD_DEV_STS_NORMAL;
		temperature_prev_state_rel = D_TEMPDD_DEV_STS_NORMAL;
		tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX] = D_TEMPDD_DEV_STS_NORMAL;
		for(i=0;i<4;i++)
			temperature_over_counter[i] = 0;
		
		tdev_sts.temperature_val.t_sensor3 = 0;
		tdev_sts.temperature_val.t_sensor4 = 0;
		tdev_sts.ad_val.ad_sensor3 = 0;
		tdev_sts.ad_val.ad_sensor4 = 0;
		
		temperature_hcm_read();

#ifdef CONFIG_CPU_FREQ_GOV_HOTPLUG
		cpufreq_frequency_opp_stop(D_TEMPDD_DEV_STS_NORMAL);
#endif /* CONFIG_CPU_FREQ_GOV_HOTPLUG */
	} else {
		printk(KERN_EMERG"usage:\n  echo 1 > temperture_init"
				" : initialize data\n");
	}

	return count;

}

static struct kobj_attribute temperature_oppnitro_attribute
				= __ATTR(temperature_oppnitro, 0666, NULL,
				temperature_debug_store_oppnitro);
static struct kobj_attribute temperature_oppturbo_attribute
				= __ATTR(temperature_oppturbo, 0666, NULL,
				temperature_debug_store_oppturbo);
static struct kobj_attribute temperature_opp100_attribute
				= __ATTR(temperature_opp100, 0666, NULL,
				temperature_debug_store_opp100);
static struct kobj_attribute temperature_opptime_attribute
				= __ATTR(temperature_opptime, 0666, NULL,
				temperature_debug_store_opptime);
static struct kobj_attribute temperature_status_attribute
				= __ATTR(temperature_status, 0666,
				temperature_debug_show_status, NULL);
static struct kobj_attribute temperature_start_attribute
				= __ATTR(temperature_start, 0666, NULL,
				temperature_debug_store_start);
static struct kobj_attribute temperature_test_attribute
				= __ATTR(temperature_test, 0666, NULL,
				temperature_debug_store_test);
static struct kobj_attribute temperature_debugdata_attribute
				= __ATTR(temperature_debugdata, 0666,
				temperature_debug_show_debugdata, NULL);
static struct kobj_attribute temperature_data_init_attribute
				= __ATTR(temperature_init, 0666, NULL,
				temperature_debug_store_data_init);
				
static struct attribute *temperature_attrs[] = {
	&temperature_oppnitro_attribute.attr,
	&temperature_oppturbo_attribute.attr,
	&temperature_opp100_attribute.attr,
	&temperature_opptime_attribute.attr,
	&temperature_status_attribute.attr,
	&temperature_start_attribute.attr,
	&temperature_test_attribute.attr,
	&temperature_debugdata_attribute.attr,
	&temperature_data_init_attribute.attr,
	NULL
};

static struct attribute_group temperature_attr_group = {
	.attrs = temperature_attrs,
};
#endif /*TEMPDD_DEBUG_TEST*/
#endif /*CONFIG_SYSFS*/
	                               
/******************************************************************************/
/*  function    : temperature driver open                                     */
/******************************************************************************/
static int temperature_open(struct inode *inode, struct file *filp)
{
    MODULE_TRACE_START();

    access_num++; /* <Quad> *//* npdc300058416, from Nemo */
    if ( access_num > D_TEMPDD_FLG_ON ) { /*multi open control*//* <Quad> *//* npdc300058416, from Nemo */
        printk("multi open path\n"); /* <Quad> *//* npdc300058416, from Nemo */
        return D_TEMPDD_RET_OK; /* <Quad> *//* npdc300058416, from Nemo */
    } /* <Quad> *//* npdc300058416 *//* npdc300058416, from Nemo */
    
    /*nvm read */
    temperature_hcm_read();

    /*initialize millor table*/
    temperature_set_millor_table();
    
    MODULE_TRACE_END();
    return D_TEMPDD_RET_OK;
}

/******************************************************************************/
/*  function    : temperature driver release                                  */
/******************************************************************************/
static int temperature_release(struct inode *inode, struct file *filp)
{
    int i;
    unsigned long flags;                         /* spin lock flg             */
    
    MODULE_TRACE_START();

    access_num--; /* <Quad> *//* npdc300058416, from Nemo */
    if(access_num == D_TEMPDD_FLG_OFF) { /* <Quad> *//* npdc300058416, from Nemo */
        spin_lock_irqsave(&temp_spinlock,flags); /* spin lock *//* <Quad> *//* npdc300058416, from Nemo */

        temperature_main_sts = D_TEMPDD_TM_STS_NORMAL; /* <Quad> *//* npdc300058416, from Nemo */
        temperature_omap_main_sts = D_TEMPDD_TM_STS_NORMAL;  /* <Quad> *//* npdc300058416, from Nemo */
        spin_unlock_irqrestore(&temp_spinlock,flags); /* spin unlock */ /* <Quad> *//* npdc300058416, from Nemo */

        for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++) /* sensor status all clear *//* <Quad> *//* npdc300058416, from Nemo */
        {
            sensor1_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; /* <Quad> *//* npdc300058416, from Nemo */
            sensor2_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; /* <Quad> *//* npdc300058416, from Nemo */
            sensor3_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; /* <Quad> *//* npdc300058416, from Nemo */
            sensor4_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; /* <Quad> */
        }
        for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++) /* <Quad> *//* npdc300058416, from Nemo */
        { /* <Quad> *//* npdc300058416, from Nemo */
            /*device status all clear */ /* <Quad> *//* npdc300058416, from Nemo */
            tdev_sts.dev_sts[i] =  D_TEMPDD_DEV_STS_NORMAL; /* <Quad> *//* npdc300058416, from Nemo */
        } /* <Quad> *//* npdc300058416, from Nemo */
    } /* <Quad> *//* npdc300058416, from Nemo */

    MODULE_TRACE_END();
    return D_TEMPDD_RET_OK;
}

/* <Quad> add start -->> */
/******************************************************************************/
/*  functoin     : temperature_w_cfg_log_chg_normal_omap                      */
/******************************************************************************/
static unsigned int temperature_w_cfg_log_chg_normal_omap(void)
{
    int ret = 0;
    unsigned short totalnum = 0;
    unsigned char timer_log_omap[8*4];
    unsigned char write_offset = 0;
    unsigned long year;
    unsigned long month;
    time_t   day;
    time_t   hour;
    time_t   min;
    time_t   second;
    unsigned int timer;

    memset(&timer_log_omap, 0, sizeof(timer_log_omap));
    /*The transition number of times*/
    ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_DISABLE_CNT, 1, (unsigned char *)&totalnum);
    ret |= cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_DISABLE_TIMESTAMP , 8*4, (unsigned char*)timer_log_omap);

    DBG_MSG("ret  -> ",ret);
    if(ret != 0 )
    {
        printk("Err opp cfg read error!! \n");
        return D_TEMPDD_RET_NG;
    }

    if(totalnum != 0xff){
        totalnum++;
        cfgdrv_write( D_HCM_E_VP_OPP_BLOCK_DISABLE_CNT, 1, (char * )&(totalnum));
    }else{
        totalnum = 1;
        cfgdrv_write( D_HCM_E_VP_OPP_BLOCK_DISABLE_CNT, 1, (char * )&(totalnum));
    }

    /* 8 ring buffer */
    write_offset = totalnum % 8;
    (write_offset == 0) ? (write_offset =(8-1)) : (write_offset -= 1);


    /*time stamp*/
    syserr_TimeStamp2( &year, &month, &day, &hour, &min, &second );

    DBG_MSG("mon  -> ",month);
    DBG_MSG("mday -> ",day);
    DBG_MSG("hour -> ",hour);
    DBG_MSG("min  -> ",min);
    DBG_MSG("sec  -> ",second);

    timer =
           ((((( month << 22) |
           (( unsigned long ) day << 17 ))  |
           (( unsigned long ) hour << 12 )) |
           (( unsigned long ) min << 6 ))   |
           ( unsigned long ) second );

    memcpy(&(timer_log_omap[write_offset*4]), (unsigned char *)&timer, 4);
    cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_DISABLE_TIMESTAMP, 8*4, timer_log_omap);

    return D_TEMPDD_RET_OK;
}

/******************************************************************************/
/*  functoin     : temperature_w_cfg_log_timestamp                            */
/******************************************************************************/
static unsigned int temperature_w_cfg_log_timestamp(int cnt,
                                                    unsigned short cnt_offset,
                                                    unsigned short time_offset)
{
    unsigned short totalnum;
    unsigned char write_offset;

    int cfg_ret = 0;
    unsigned int nvm_timer;
    unsigned long year;
    unsigned long month;
    time_t   day;
    time_t   hour;
    time_t   min;
    time_t   second;
    unsigned char time_log_3r[3*4];
    unsigned char time_log_8r[8*4];


    cfg_ret  = cfgdrv_read(cnt_offset, 2, (unsigned char *)&(totalnum));

    if(cnt == 3){
        cfg_ret |= cfgdrv_read(time_offset, cnt*4, time_log_3r);
    }else if(cnt == 8){
        cfg_ret |= cfgdrv_read(time_offset, cnt*4, time_log_8r);
    }else{
        printk("ERR cnt is bad value \n");
        return D_TEMPDD_RET_NG;
    }

    if(cfg_ret != 0 ){
        printk("Err cfg read error!! \n");
        return D_TEMPDD_RET_NG;
    }

    /* cnt ring buffer    */
    write_offset = totalnum % cnt;
    (write_offset == 0) ? (write_offset =(cnt - 1)) : (write_offset -= 1);

    DBG_MSG("totalnum ->", totalnum);
    DBG_MSG("cnt -> ", cnt);
    DBG_MSG("write_offset -> ", write_offset);

    /*date get */
    syserr_TimeStamp2( &year, &month, &day, &hour, &min, &second);

    DBG_MSG("mon  -> ",month);
    DBG_MSG("mday -> ",day);
    DBG_MSG("hour -> ",hour);
    DBG_MSG("min  -> ",min);
    DBG_MSG("sec  -> ",second);

    /* sysdes log transform           */
    nvm_timer =((((( month << 22) |
                (( unsigned long ) day << 17 ))  |
                (( unsigned long ) hour << 12 )) |
                (( unsigned long ) min << 6 ))   |
                 ( unsigned long ) second ) ;

    if(cnt == 3){
        memcpy(&(time_log_3r[write_offset*4]),(unsigned char *)&nvm_timer, 4);
        cfgdrv_write( time_offset, cnt*4, time_log_3r);
    }else if(cnt == 8){
        memcpy(&(time_log_8r[write_offset*4]),(unsigned char *)&nvm_timer, 4);
        cfgdrv_write( time_offset, cnt*4, time_log_8r);
    }else{
        printk("ERR cnt is bad value \n");
        return D_TEMPDD_RET_NG;
    }

    return D_TEMPDD_RET_OK;

}

/******************************************************************************/
/*  functoin     : temperature_w_cfg_log_chg_normal                           */
/******************************************************************************/
static unsigned int temperature_w_cfg_log_chg_normal(int type)
{
    int ret = 0;
    unsigned short nvm_cnt_offset = 0;
    unsigned short nvm_time_offset = 0;
    int ring_cnt = 0;


    switch (type)
    {
        case  D_TEMPDD_BATTERY_INDEX :
            ROUTE_CHECK("D_TEMPDD_BATTERY\n");
            nvm_cnt_offset = D_HCM_E_VP_CHG_DISABLE_CNT;
            nvm_time_offset= D_HCM_E_VP_CHG_ERR_DISABLE_TIMESTAMP;
            ring_cnt = 8;
            break;
        case  D_TEMPDD_WLAN_INDEX :
            ROUTE_CHECK("D_TEMPDD_WLAN\n");
            nvm_cnt_offset = D_HCM_E_VP_WLAN_DISABLE_CNT;
            nvm_time_offset= D_HCM_E_VP_WLAN_ERR_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_CAMERA_INDEX :
            ROUTE_CHECK("D_TEMPDD_CAMERA\n");
            nvm_cnt_offset = D_HCM_E_VP_CAM_DISABLE_CNT;
            nvm_time_offset= D_HCM_E_VP_CAM_ERR_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_BATTERY_AMR_INDEX :
            ROUTE_CHECK("D_TEMPDD_BATTERY_AMR_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_AMRCHG_DISABLE_CNT;
            nvm_time_offset= D_HCM_E_VP_AMRCHG_ERR_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_BATTERY_WLAN_INDEX :
            ROUTE_CHECK("D_TEMPDD_BATTERY_WLAN_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_CHG_DISABLE2_CNT;
            nvm_time_offset=D_HCM_E_VP_CHG_WLAN_ERR_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_CAMERA_WLAN_INDEX :
            ROUTE_CHECK("D_TEMPDD_CAMERA_WLAN_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_CAM_DISABLE2_CNT;
            nvm_time_offset= D_HCM_E_VP_CAM_WLAN_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_DTV_BKL_INDEX :
            ROUTE_CHECK("D_TEMPDD_DTV_BKL_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_DTV_BL_DISABLE_CNT;
            nvm_time_offset= D_HCM_E_VP_DTV_BL_ERR_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_BATTERY_CAMERA_INDEX :
            ROUTE_CHECK("D_TEMPDD_BATTERY_CAMERA_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_CHG_DISABLE3_CNT;
            nvm_time_offset= D_HCM_E_VP_CHG_CAMERA_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_BATTERY_DTV_INDEX :
            ROUTE_CHECK("D_TEMPDD_BATTERY_DTV_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_MMCHG_DISABLE3_CNT;
            nvm_time_offset= D_HCM_E_VP_CHG_DTV_DISABLE_TIMESTAMP;
            ring_cnt = 3;
            break;
        case  D_TEMPDD_CAMERA_CAMERA_INDEX :
            ROUTE_CHECK("D_TEMPDD_CAMERA_CAMERA_INDEX\n");
            nvm_cnt_offset = D_HCM_E_VP_CAM_DISABLE3_CNT;
            nvm_time_offset= D_HCM_E_VP_CAMERA_CAMERA_DISABLE_TIMESTAMP;
            ring_cnt = 8;
            break;
        default:
            printk("ERR CHECK INDEX \n");
            return D_TEMPDD_RET_NG;
    }

    /* write cfg_log timestamp */
    ret = temperature_w_cfg_log_timestamp( ring_cnt, nvm_cnt_offset, nvm_time_offset);
    if( ret !=  D_TEMPDD_RET_OK)
    {
        printk("ERR write timestamp \n");
    }

    return ret;
}
/* <Quad> add end <<---- */

/******************************************************************************/
/*  functoin     : driver poll                                                */
/******************************************************************************/
static unsigned int temperature_poll(struct file *filp, poll_table *wait)
{
    unsigned int       ret;
    int i;
                                                 /* nvm log                   */
    unsigned short totalnum;
    unsigned char write_offset;
    unsigned char sensor1_log[8];
    unsigned char sensor2_log[8];
    unsigned char sensor3_log[8];
    unsigned char sensor4_log[8]; /* <Quad> */
    unsigned char time_log[8*4];

    int  nvm_ret =0 ; 

    unsigned short nvm_s1_offset,nvm_s2_offset,nvm_s3_offset,nvm_s4_offset; /* <Quad> */
    unsigned short nvm_cnt_offset,nvm_time_offset;
    unsigned int nvm_timer;
    unsigned long year;
    unsigned long month;
    time_t   day;
    time_t   hour;
    time_t   min;
    time_t   second;
	/*unsigned int minor;*/
	static unsigned int prev_omap_sts = D_TEMPDD_DEV_STS_NORMAL;
	int enable;
		
    MODULE_TRACE_START();

    ret = D_TEMPDD_CLEAR;    
    
    if( filp == NULL )                          
    {
        return -EINVAL;                          /* illegal return            */
    }
        
    /*minor = MINOR( filp->f_dentry->d_inode->i_rdev );*/ /* get minor num    */

    ret = D_TEMPDD_CLEAR;                       /*return reclear              */
    
	if (wakeup_flg_omap == 0 || wakeup_flg_3g == 0)
		poll_wait(filp,&tempwait_q,wait);

	
	if (wakeup_flg_omap == 1) {
		wakeup_flg_omap = 0;
		if (tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX] != prev_omap_sts){
			ret = POLLIN | POLLRDNORM;
			prev_omap_sts = tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX];
			
			if (temperature_write_log_flg >= 1){
				if (temperature_write_log_flg == 2){
					enable = 1;
				} else {
					enable = 0;
				} 
				temperature_write_log(tdev_sts.dev_sts
					[D_TEMPDD_OMAP_INDEX], enable);
				temperature_write_log_flg = 0;
			}
		}
		/* <Quad> MOD start */
		if(timestamp_write_log_flg == 1){
			temperature_w_cfg_log_chg_normal_omap();
			timestamp_write_log_flg = 0;
		}
		/* <Quad> MOD end */
	}
	
	if (wakeup_flg_3g == 1) {
		wakeup_flg_3g = 0;
		/*illegal temperature -> nomal temperature check */
		for (i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++) {
			/*if the device is OMAP,don't check the sensor*/
 			if( i == D_TEMPDD_OMAP_INDEX )
				continue;
	
			/*enable time all 0xFF -> no check */
/* Quad MOD start */
			if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&   /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&   /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) || /* <Quad> */
				 ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&  /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&  /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) &&  /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) ) /* <Quad> */
/* Quad MOD end */
			{
				ROUTE_CHECK("enable_time nvm =all 0xff \n");
				/*no action path*/
			}
			else 
			{ 
			   /*now sensor status are all nomal and device status are illegal temperature */
			   /*and nvm enable_time equal 0xff  -> no check                               */
				
/* Quad MOD start */
				if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
					   ( ( sensor1_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor2_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor3_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) && /* <Quad> */
  					   ( tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_ABNORMAL ) ) || /* <Quad> */
					 ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
					   ( ( sensor1_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor2_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor3_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor4_sts.sensor_sts_new[i] == D_TEMPDD_NORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) && /* <Quad> */
					   ( tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_ABNORMAL ) ) ) /* <Quad> */
/* Quad MOD end */
						 /*all true -> illegal temperature change  nomal temperature */
				{
					   ROUTE_CHECK("poll state normal \n");
											 /*device status change normal */
                                          tdev_sts.dev_sts[i] =  D_TEMPDD_DEV_STS_NORMAL;
                                          temperature_w_cfg_log_chg_normal(i); /* <Quad> */
					   ret = POLLIN | POLLRDNORM;   /*poll_wait return     */
				}
				else 
				{
				   ROUTE_CHECK("no action path1 ");
				}
			}
	    }
	    if(ret != D_TEMPDD_CLEAR)/*illegal temperature -> normal temperature */
	    {
        	return ret;
	    }
	    /*nomal temperature -> illegal temperature check */
	    for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
	    {    		
			/*if the device is OMAP, don't check the sensor*/
 			if( i == D_TEMPDD_OMAP_INDEX )
 				continue;
		    
		    /*disable time all 0xff -> no check*/
/* Quad MOD start */
			if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&   /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&   /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) || /* <Quad> */
				 ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&  /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&  /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) &&  /* <Quad> */
				   ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) ) /* <Quad> */
/* Quad MOD end */
		    {
		        ROUTE_CHECK("disable_time nvm =all 0xff \n");
		    }
		    else
		    {
		       /*now sensor status are all illegal and device status are nomal status*/
		       /*and disable time is 0xff -> no check                                */
/* Quad MOD start */
				if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
					   ( ( sensor1_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor2_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor3_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) && /* <Quad> */
  					   ( tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_NORMAL ) ) || /* <Quad> */
					 ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
					   ( ( sensor1_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor2_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor3_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) && /* <Quad> */
					   ( ( sensor4_sts.sensor_sts_new[i] == D_TEMPDD_ABNORMAL_SENSOR ) || ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) && /* <Quad> */
					   ( tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_NORMAL ) ) ) /* <Quad> */
/* Quad MOD end */
		                 /*all true -> nomal temperature change illegal temperature  */
		        {
		            ROUTE_CHECK("poll state abnormal\n");
		            tdev_sts.dev_sts[i] = D_TEMPDD_DEV_STS_ABNORMAL; 
		            ret = POLLIN | POLLRDNORM; /*poll_wait return                    */


		            /*illegal temperature log save fuction */
		            switch (i)
		            {
		                case  D_TEMPDD_BATTERY_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_BATTERY\n");
		                    nvm_cnt_offset = D_HCM_E_VP_CHG_DISABLE_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_CHG_DISABLE_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_CHG_DISABLE_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_CHG_DISABLE_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_CHG_DISABLE_TIMESTAMP;
		                break;
		                case  D_TEMPDD_WLAN_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_WLAN\n");
		                    nvm_cnt_offset = D_HCM_E_VP_WLAN_DISABLE_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_WLAN_DISABLE_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_WLAN_DISABLE_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_WLAN_DISABLE_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_WLAN_DISABLE_TIMESTAMP;
		                break;
	#if 0 /* Totoro unused */
		                case  D_TEMPDD_OMAP_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_OMAP\n");
		                    nvm_cnt_offset = D_HCM_E_VP_OMAP_DISABLE_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_OMAP_DISABLE_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_OMAP_DISABLE_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_OMAP_DISABLE_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_OMAP_DISABLE_TIMESTAMP;
		                break;
	#endif /* Totoro */
		                case  D_TEMPDD_CAMERA_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_CAMERA\n");
		                    nvm_cnt_offset = D_HCM_E_VP_CAM_DISABLE_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_CAM_DISABLE_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_CAM_DISABLE_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_CAM_DISABLE_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_CAM_DISABLE_TIMESTAMP;
		                break;
		                case  D_TEMPDD_BATTERY_AMR_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_BATTERY_AMR_INDEX\n");
		                    nvm_cnt_offset = D_HCM_E_VP_AMRCHG_DISABLE_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_AMRCHG_DISABLE_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_AMRCHG_DISABLE_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_AMRCHG_DISABLE_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_AMRCHG_DISABLE_TIMESTAMP;
		                break;
		                case  D_TEMPDD_BATTERY_WLAN_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_BATTERY_WLAN_INDEX\n");
		                    nvm_cnt_offset = D_HCM_E_VP_CHG_DISABLE2_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_CHG_DISABLE2_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_CHG_DISABLE2_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_CHG_DISABLE2_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_CHG_DISABLE2_TIMESTAMP;
		                break;
	#if 0 /* Totoro unused */
		                case  D_TEMPDD_OMAP_WLAN_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_OMAP_WLAN_INDEX\n");
		                    nvm_cnt_offset = D_HCM_E_VP_OMAP_DISABLE2_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_OMAP_DISABLE2_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_OMAP_DISABLE2_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_OMAP_DISABLE2_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_OMAP_DISABLE2_TIMESTAMP;
		                break;
	#endif /* Totoro*/
		                case  D_TEMPDD_CAMERA_WLAN_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_CAMERA_WLAN_INDEX\n");
		                    nvm_cnt_offset = D_HCM_E_VP_CAM_DISABLE2_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_CAM_DISABLE2_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_CAM_DISABLE2_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_CAM_DISABLE2_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_CAM_DISABLE2_TIMESTAMP;
		                break;
		                case  D_TEMPDD_DTV_BKL_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_DTV_BKL_INDEX\n");
		                    nvm_cnt_offset = D_HCM_E_VP_DTV_BL_DISABLE_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_DTV_BL_DISABLE_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_DTV_BL_DISABLE_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_DTV_BL_DISABLE_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_DTV_BL_DISABLE_TIMESTAMP;
		                break;
		                case  D_TEMPDD_BATTERY_CAMERA_INDEX :
		                    ROUTE_CHECK("D_TEMPDD_BATTERY_CAMERA_INDEX\n");
		                    nvm_cnt_offset = D_HCM_E_VP_CHG_DISABLE3_CNT;
		                    nvm_s1_offset  = D_HCM_E_VP_CHG_DISABLE3_TEMP_LOG_S1;
		                    nvm_s2_offset  = D_HCM_E_VP_CHG_DISABLE3_TEMP_LOG_S2;
		                    nvm_s3_offset  = D_HCM_E_VP_CHG_DISABLE3_TEMP_LOG_S3;
		                    nvm_time_offset= D_HCM_E_VP_CHG_DISABLE3_TIMESTAMP;
		                break;
/* Quad ADD start */
		                case  D_TEMPDD_BATTERY_DTV_INDEX : /* <Quad> */
		                    ROUTE_CHECK("D_TEMPDD_BATTERY_DTV_INDEX\n"); /* <Quad> */
		                    nvm_cnt_offset = D_HCM_E_VP_MMCHG_DISABLE3_CNT; /* <Quad> */
		                    nvm_s1_offset  = D_HCM_E_VP_MMCHG_DISABLE3_TEMP_LOG_S1; /* <Quad> */
		                    nvm_s2_offset  = D_HCM_E_VP_MMCHG_DISABLE3_TEMP_LOG_S2; /* <Quad> */
		                    nvm_s3_offset  = D_HCM_E_VP_MMCHG_DISABLE3_TEMP_LOG_S3; /* <Quad> */
		                    nvm_time_offset= D_HCM_E_VP_MMCHG_DISABLE3_TIMESTAMP; /* <Quad> */
		                break;
		                case  D_TEMPDD_CAMERA_CAMERA_INDEX : /* <Quad> */
		                    ROUTE_CHECK("D_TEMPDD_CAMERA_CAMERA_INDEX\n"); /* <Quad> */
		                    nvm_cnt_offset = D_HCM_E_VP_CAM_DISABLE3_CNT; /* <Quad> */
		                    nvm_s1_offset  = D_HCM_E_VP_CAM_DISABLE3_TEMP_LOG_S1; /* <Quad> */
		                    nvm_s2_offset  = D_HCM_E_VP_CAM_DISABLE3_TEMP_LOG_S2; /* <Quad> */
		                    nvm_s3_offset  = D_HCM_E_VP_CAM_DISABLE3_TEMP_LOG_S3; /* <Quad> */
		                    nvm_s4_offset  = D_HCM_E_VP_CAM_DISABLE3_TEMP_LOG_S4; /* <Quad> */
		                    nvm_time_offset= D_HCM_E_VP_CAM_DISABLE3_TIMESTAMP; /* <Quad> */
		                break;
/* Quad ADD end */
		                default:
		                    return D_TEMPDD_RET_NG;
		            }                    
		                                                    /*num read            */
		            nvm_ret  = cfgdrv_read(nvm_cnt_offset,2,(unsigned char *)&(totalnum));
		            nvm_ret |= cfgdrv_read(nvm_s1_offset,8,sensor1_log);
		            nvm_ret |= cfgdrv_read(nvm_s2_offset,8,sensor2_log);
		            nvm_ret |= cfgdrv_read(nvm_s3_offset,8,sensor3_log);
/* Quad ADD start */
					if ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) { /* <Quad> */
	                    nvm_ret |= cfgdrv_read(nvm_s4_offset,8,sensor4_log); /* <Quad> */
		            } /* Quad */
/* Quad ADD end */
		            nvm_ret |= cfgdrv_read(nvm_time_offset,8*4,time_log);
		            DBG_MSG("nvm_ret  -> ",nvm_ret); 
		            if(nvm_ret != 0 )
		            {
		                printk("nvm_read error!! \n");
		                return ret;
		            }

		            if(totalnum != 0xffff)
		            {
		                totalnum++;
		                cfgdrv_write(nvm_cnt_offset,2,
		                          (char * )&(totalnum));
		            }

		            DBG_MSG("totalnum  -> ",totalnum);
		            
		                                                      /* 8 ring buffer    */
		            write_offset = totalnum % 8;
		            (write_offset == 0) ? (write_offset =(8-1)) : (write_offset -= 1);

		            DBG_MSG("write_offset  -> ",write_offset);
		            sensor1_log[write_offset] = (unsigned char)tdev_sts.temperature_val.t_sensor1;
		            sensor2_log[write_offset] = (unsigned char)tdev_sts.temperature_val.t_sensor2;
		            sensor3_log[write_offset] = (unsigned char)tdev_sts.temperature_val.t_sensor3;
/* Quad ADD start */
					if ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) { /* <Quad> */
	                    sensor4_log[write_offset] = (unsigned char)tdev_sts.temperature_val.t_sensor4; /* <Quad> */
		            } /* Quad */
/* Quad ADD end */

		            cfgdrv_write(nvm_s1_offset,8,sensor1_log);
		            cfgdrv_write(nvm_s2_offset,8,sensor2_log);
		            cfgdrv_write(nvm_s3_offset,8,sensor3_log);
/* Quad ADD start */
					if ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) { /* <Quad> */
	                    cfgdrv_write(nvm_s4_offset,8,sensor4_log); /* <Quad> */
		            } /* Quad */
/* Quad ADD end */
		                                                                /*date get */
		            syserr_TimeStamp2( &year, &month, &day, &hour, &min, &second ); 

		            DBG_MSG("mon  -> ",month);
		            DBG_MSG("mday -> ",day);
		            DBG_MSG("hour -> ",hour);             
		            DBG_MSG("min  -> ",min);
		            DBG_MSG("sec  -> ",second);
		                                        /* sysdes log transform           */
		            nvm_timer =
		            ((((( month << 22) | 
		                  (( unsigned long ) day << 17 ))  |
		                  (( unsigned long ) hour << 12 )) |
		                  (( unsigned long ) min << 6 ))   |
		                ( unsigned long ) second ) ;

		            DBG_MSG("nvm_timer  ->",nvm_timer);
		            memcpy(&(time_log[write_offset*4]),(unsigned char *)&nvm_timer,4);
		            cfgdrv_write(nvm_time_offset,8*4,time_log);
		        }
		        else 
		        {
		           ROUTE_CHECK("no action path2 ");
		        }
		    }
	    }
	}
    MODULE_TRACE_END();
    return ret;
}

/******************************************************************************/
/*  fuction     : nvm read fuction                                            */
/******************************************************************************/
static int    temperature_hcm_read(void)
{
    int ret;
    int total=0;
    unsigned char buf[4];
    
    MODULE_TRACE_START();

    ret = cfgdrv_read(D_HCM_E_VP_HEAT_SUPPRESS, 1, (unsigned char *)&(temp_hcmparam.vp_heat_suppress));
    DBG_PRINT("HEAT_SUPPRESS = %d\n", temp_hcmparam.vp_heat_suppress);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_PERIOD, 1, (unsigned char *)&(temp_hcmparam.vp_period));
    DBG_PRINT("PERIOD = %d\n", temp_hcmparam.vp_period);
    total |= ret;
/* Quad ADD start */
    ret = cfgdrv_read(D_HCM_E_VP_HEAT_SUPPRESS2, 1, (unsigned char *)&buf); /* <Quad> */
    temp_hcmparam.vp_heat_suppress = temp_hcmparam.vp_heat_suppress | ((unsigned short)buf[0] << 8); /* <Quad> */
    DBG_PRINT("HEAT_SUPPRESS2 = %d\n", temp_hcmparam.vp_heat_suppress); /* <Quad> */
    total |= ret; /* <Quad> */
/* Quad ADD end */
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_OFFHOOK_TIME, 1, (unsigned char *)&(temp_hcmparam.enable_time_3g));
    DBG_PRINT("AMRCHG_ENABLE_OFFHOOK_TIME = %d\n", temp_hcmparam.enable_time_3g);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_OFFHOOK_TIME, 1, (unsigned char *)&(temp_hcmparam.enable_time_3g_p));
    DBG_PRINT("CHG_ENABLE_OFFHOOK_TIME = %d\n", temp_hcmparam.enable_time_3g_p);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_OFFHOOK_TIME2, 1, (unsigned char *)&(temp_hcmparam.enable_time_wlan));
    DBG_PRINT("CHG_ENABLE_OFFHOOK_TIME2 = %d\n", temp_hcmparam.enable_time_wlan);
    total |= ret; 
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_OFFHOOK_TIME3, 1, (unsigned char *)&(temp_hcmparam.enable_time_camera));
    DBG_PRINT("CHG_ENABLE_OFFHOOK_TIME3 = %d\n", temp_hcmparam.enable_time_camera);
    total |= ret; 
/* Quad ADD start */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_OFFHOOK_TIME, 1, (unsigned char *)&(temp_hcmparam.enable_time_dtv)); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE3_OFFHOOK_TIME = %d\n", temp_hcmparam.enable_time_dtv); /* <Quad> */
    total |= ret;  /* <Quad> */
/* Quad ADD end */
    /* iD Power ADD start */
    ret = cfgdrv_read(D_HCM_HKADC_ADJUST2, 2, (unsigned char*)&temp_hcmparam.factory_adj_val);
    DBG_PRINT("HKADC_ADJUST2 = %d\n", temp_hcmparam.factory_adj_val);
    total |= ret;
    /* iD Power ADD end */
    
    /* BATTERY(3G mode) */
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_DISABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_DISABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_DISABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_DISABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_DISABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_DISABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_ENABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_ENABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_ENABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_ENABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_ENABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_ENABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;

    /* WLAN */
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_DISABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("WLAN_DISABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_DISABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("WLAN_DISABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_DISABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("WLAN_DISABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_DISABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("WLAN_DISABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_DISABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("WLAN_DISABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_DISABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("DISABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_ENABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("WLAN_ENABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_ENABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("WLAN_ENABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_ENABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("WLAN_ENABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_ENABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("WLAN_ENABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_ENABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("WLAN_ENABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_WLAN_ENABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("WLAN_ENABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;

#if 0 /* Totoro unused */
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    total |= ret;
#endif /* Totoro */

    /* CAMERA(3G mode) */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_DISABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_DISABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_DISABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_DISABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_DISABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_DISABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_ENABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_ENABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_ENABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_ENABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_ENABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_ENABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;

    /* BATTERY_ARM */
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_DISABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("AMRCHG_DISABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_DISABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("AMRCHG_DISABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_DISABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("AMRCHG_DISABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_DISABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("AMRCHG_DISABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_DISABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("AMRCHG_DISABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_DISABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("AMRCHG_DISABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("AMRCHG_ENABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("AMRCHG_ENABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("AMRCHG_ENABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("AMRCHG_ENABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("AMRCHG_ENABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_AMRCHG_ENABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("AMRCHG_ENABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;

    /* BATTERY(WLAN mode) */
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_DISABLE_TEMP4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("DISABLE_TEMP5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_DISABLE_TEMP6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_DISABLE_TIME4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_DISABLE_TIME5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_DISABLE_TIME6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_ENABLE_TEMP4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_ENABLE_TEMP5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_ENABLE_TEMP6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_ENABLE_TIME4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_ENABLE_TIME5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_ENABLE_TIME6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;

#if 0 /* Totoro unused */
    /*OMAP            (WLAN      )*/
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TEMP4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TEMP5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TEMP6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    total |= ret;
    /*        1  3          */
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TIME4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TIME5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_DISABLE_TIME6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    total |= ret;
    /*        1  3          */
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TEMP4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TEMP5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TEMP6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    total |= ret;
    /*        1  3          */
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TIME4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TIME5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_OMAP_ENABLE_TIME6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    total |= ret;
#endif /* Totoro */

    /* CAMERA(WLAN mode) */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_DISABLE_TEMP4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_DISABLE_TEMP5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_DISABLE_TEMP6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_DISABLE_TIME4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_DISABLE_TIME5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_DISABLE_TIME6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_ENABLE_TEMP4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_ENABLE_TEMP5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_ENABLE_TEMP6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME4, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CAM_ENABLE_TIME4 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME5, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CAM_ENABLE_TIME5 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME6, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CAM_ENABLE_TIME6 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;    

    /* DTV and BackLight */
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_DISABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("DTV_BL_DISABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_DISABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("DTV_BL_DISABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_DISABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("DTV_BL_DISABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_DISABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("DTV_BL_DISABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_DISABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("DTV_BL_DISABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_DISABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("DTV_BL_DISABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_ENABLE_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("DTV_BL_ENABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_ENABLE_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("DTV_BL_ENABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_ENABLE_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("DTV_BL_ENABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_ENABLE_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("DTV_BL_ENABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_ENABLE_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("DTV_BL_ENABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_DTV_BL_ENABLE_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("DTV_BL_ENABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;    

    /* BATTERY(CAMERA) */
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP7, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_DISABLE_TEMP7 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP8, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_DISABLE_TEMP8 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TEMP9, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_DISABLE_TEMP9 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME7, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_DISABLE_TIME7 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME8, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_DISABLE_TIME8 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_DISABLE_TIME9, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_DISABLE_TIME9 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP7, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_ENABLE_TEMP7 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP8, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_ENABLE_TEMP8 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TEMP9, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_ENABLE_TEMP9 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME7, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]));
    DBG_PRINT("CHG_ENABLE_TIME7 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME8, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]));
    DBG_PRINT("CHG_ENABLE_TIME8 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]);
    total |= ret;
    ret = cfgdrv_read(D_HCM_E_VP_CHG_ENABLE_TIME9, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]));
    DBG_PRINT("CHG_ENABLE_TIME9 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]);
    total |= ret;

/* Quad ADD start */
    /* BATTERY_DTV *//* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_DISABLE3_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("MMCHG_DISABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_DISABLE3_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("MMCHG_DISABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_DISABLE3_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("MMCHG_DISABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_DISABLE3_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("MMCHG_DISABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_DISABLE3_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("MMCHG_DISABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_DISABLE3_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("MMCHG_DISABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_TEMP1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE_TEMP1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_TEMP2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE_TEMP2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_TEMP3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE_TEMP3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_TIME1, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE_TIME1 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_TIME2, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE_TIME2 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_MMCHG_ENABLE3_TIME3, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("MMCHG_ENABLE_TIME3 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */

    /* CAMERA(CAMERA) *//* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP7,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TEMP7 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP8,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TEMP8 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP9,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TEMP9 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TEMP10, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR4])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TEMP10 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR4]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME7,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TIME7 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME8,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TIME8 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME9,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TIME9 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_DISABLE_TIME10, 1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR4])); /* <Quad> */
    DBG_PRINT("CAM_DISABLE_TIME10 = %d\n", temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR4]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP7,   1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TEMP7 = %d\n",   temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP8,   1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TEMP8 = %d\n",   temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP9,   1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TEMP9 = %d\n",   temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TEMP10,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR4])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TEMP10 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR4]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME7,   1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TIME7 = %d\n",   temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME8,   1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TIME8 = %d\n",   temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME9,   1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TIME9 = %d\n",   temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]); /* <Quad> */
    total |= ret; /* <Quad> */
    ret = cfgdrv_read(D_HCM_E_VP_CAM_ENABLE_TIME10,  1, (unsigned char *)&(temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR4])); /* <Quad> */
    DBG_PRINT("CAM_ENABLE_TIME10 = %d\n",  temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR4]); /* <Quad> */
    total |= ret; /* <Quad> */
/* Quad ADD end */

	/* read parametor for OMAP */
	ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_ABS_TEMP,
    					3, (unsigned char *)&buf);
	total |= ret;
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.abs_temp_th = buf[0];	
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.abs_temp_th = buf[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.abs_temp_th = buf[2];
    DBG_PRINT("OPP_BLOCK_ABS_TEMP = %d, %d, %d\n",
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].abs_temp_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].abs_temp_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].abs_temp_th);

/* iD Power DEL start */
/*                      */
#if 0	
	ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_REL_TEMP,
    					3, (unsigned char *)&buf);
	total |= ret;
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.rel_temp_th = buf[0];	
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.rel_temp_th = buf[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.rel_temp_th = buf[2];
    DBG_PRINT("OPP_BLOCK_REL_TEMP = %d, %d, %d\n",
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].rel_temp_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].rel_temp_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].rel_temp_th);
#endif
/* iD Power DEL end */

	ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_ABS_NUM,
    					3, (unsigned char *)&buf);
	total |= ret;
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.abs_cnt_th = buf[0];	
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.abs_cnt_th = buf[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.abs_cnt_th = buf[2];  
    DBG_PRINT("OPP_BLOCK_ABS_NUM = %d, %d, %d\n",
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].abs_cnt_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].abs_cnt_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].abs_cnt_th);
 
/* iD Power DEL start */
/*                      */
#if 0
 	ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_REL_NUM,
    					3, (unsigned char *)&buf);
	total |= ret;
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.rel_cnt_th = buf[0];	
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.rel_cnt_th = buf[1];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.rel_cnt_th = buf[2];
    DBG_PRINT("OPP_BLOCK_REL_NUM = %d, %d, %d\n",
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].rel_cnt_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].rel_cnt_th,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].rel_cnt_th);
#endif
/* iD Power DEL end */

 	ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_TEMP_MEAS_FREQ,
    					4, (unsigned char *)&buf);
	total |= ret;
//	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_NORMAL]
//							.timer = buf[0];	
//	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
//							.timer = buf[1];	
//	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
//							.timer = buf[2];
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_NORMAL].timer = buf[3]; /* <Quad> *//* PTC */
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].timer = buf[3]; /* <Quad> *//* PTC */
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].timer = buf[3]; /* <Quad> *//* PTC */
	temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
							.timer = buf[3];
    DBG_PRINT("OPP_BLOCK_REL_NUM = %d, %d, %d, %d\n",
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_NORMAL].timer,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL].timer,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2].timer,
        temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3].timer);
     
    if(total != 0) /*read error */
    {
        printk("temperature DD : nvm_read error     \n");
        temp_hcmparam.vp_heat_suppress = 0x00;
        temp_hcmparam.vp_period        = 0x01;
        temp_hcmparam.enable_time_3g   = 0xFF;
        temp_hcmparam.enable_time_3g_p = 0xFF;
        temp_hcmparam.enable_time_wlan = 0xFF;
        temp_hcmparam.enable_time_camera = 0xFF;
        temp_hcmparam.enable_time_dtv = 0xFF; /* <Quad> */
        temp_hcmparam.factory_adj_val = 0x1F5;  /* iD Power ADD */
 
        /*battery(3G mode)*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR1] = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR2] = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_temp[D_TEMPDD_SENSOR3] = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR1] = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR2] = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].disable_time[D_TEMPDD_SENSOR3] = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR1]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR2]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_temp[D_TEMPDD_SENSOR3]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR1]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR2]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_INDEX].enable_time[D_TEMPDD_SENSOR3]  = 0xFF;

        /*wlan   */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]    = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]    = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]    = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]    = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]    = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]    = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]     = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]     = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]     = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]     = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]     = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]     = 0xFF;

        /*OMAP   */
#if 0 /* Totoro unused */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_temp[D_TEMPDD_SENSOR1]    = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_temp[D_TEMPDD_SENSOR2]    = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_temp[D_TEMPDD_SENSOR3]    = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_time[D_TEMPDD_SENSOR1]    = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_time[D_TEMPDD_SENSOR2]    = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].disable_time[D_TEMPDD_SENSOR3]    = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_temp[D_TEMPDD_SENSOR1]     = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_temp[D_TEMPDD_SENSOR2]     = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_temp[D_TEMPDD_SENSOR3]     = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_time[D_TEMPDD_SENSOR1]     = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_time[D_TEMPDD_SENSOR2]     = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_INDEX].enable_time[D_TEMPDD_SENSOR3]     = 0xFF;
#endif /* Totoro */

        /*CAMERA(3G mode)*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]   = 0xFF;
        
        /*battery AMR*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR1]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR2]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_temp[D_TEMPDD_SENSOR3]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR1]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR2]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_AMR_INDEX].enable_time[D_TEMPDD_SENSOR3]   = 0xFF;
        
        /*battery(WLAN mode)*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]   = 0xFF;

#if 0 /* Totoro unused */
        /*OMAP WLAN*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_OMAP_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]   = 0xFF;
#endif /* Totoro */

        /*CAMERA(WLAN mode)*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR1]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR2]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_temp[D_TEMPDD_SENSOR3]   = 0x00;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR1]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR2]   = 0xFF;
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_WLAN_INDEX].enable_time[D_TEMPDD_SENSOR3]   = 0xFF;

    /* DTV and BackLight */
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF; 
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR1]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR2]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_temp[D_TEMPDD_SENSOR3]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR1]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR2]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_DTV_BKL_INDEX].enable_time[D_TEMPDD_SENSOR3]  = 0xFF;

    /* BATTERY(CAMERA) */
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1]  = 0xFF; 
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]  = 0x00;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]  = 0xFF;
       temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]  = 0xFF;

/* Quad ADD start */
        /* BATTERY_DTV *//* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR1] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR2] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_temp[D_TEMPDD_SENSOR3] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR1] = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR2] = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].disable_time[D_TEMPDD_SENSOR3] = 0xFF;  /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR1]  = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR2]  = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_temp[D_TEMPDD_SENSOR3]  = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR1]  = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR2]  = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_BATTERY_DTV_INDEX].enable_time[D_TEMPDD_SENSOR3]  = 0xFF; /* <Quad> */

        /* CAMERA(CaAMERA) *//* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR1] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR2] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR3] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_temp[D_TEMPDD_SENSOR4] = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR1] = 0xFF;  /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR2] = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR3] = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].disable_time[D_TEMPDD_SENSOR4] = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR1]  = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR2]  = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR3]  = 0x00; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_temp[D_TEMPDD_SENSOR4]  = 0x00; /* <Quad>*/
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR1]  = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR2]  = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR3]  = 0xFF; /* <Quad> */
        temp_hcmparam.dev_hcmparam[D_TEMPDD_CAMERA_CAMERA_INDEX].enable_time[D_TEMPDD_SENSOR4]  = 0xFF; /* <Quad>*/
/* Quad ADD end */

	/* read parametor for OMAP */
		
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.abs_temp_th = 0x00;	
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.abs_temp_th = 0x00;
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.abs_temp_th = 0x00;
/* iD Power DEL start */
/*                      */
#if 0
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.rel_temp_th = 0x00;	
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.rel_temp_th = 0x00;
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.rel_temp_th = 0x00;
#endif
/* iD Power DEL end */
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.abs_cnt_th = 0xFF;	
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.abs_cnt_th = 0xFF;
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.abs_cnt_th = 0xFF;  
/* iD Power DEL start */
/*                      */
#if 0
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
						.rel_cnt_th = 0xFF;	
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
						.rel_cnt_th = 0xFF;
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
						.rel_cnt_th = 0xFF;
#endif
/* iD Power DEL end */
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_NORMAL]
							.timer = 0x04;	
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL]
							.timer = 0x02;	
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L2]
							.timer = 0x01;
		temperature_omap_hcmparam[D_TEMPDD_DEV_STS_ABNORMAL_L3]
							.timer = 0x01;
    }
    MODULE_TRACE_END();
    return 0;
}

/******************************************************************************/
/*  function    : temperature check function                                  */
/******************************************************************************/
static int    temperature_check_start()
{
    int ret;
     
    MODULE_TRACE_START();

    cancel_work_sync( &tempwork_1secpollingtask );
    cancel_work_sync( &tempwork_pollingtask );

    ret = D_TEMPDD_CLEAR;
    ret = temperature_twl_conv();             /*AD value get fuction          */
    if (ret == D_TEMPDD_RET_NG)
    {
        printk("temperature DD : twl get error \n");
        return D_TEMPDD_RET_NG;
    }

    temperature_pollingtimer.expires  = jiffies +temp_hcmparam.vp_period*HZ ;
    temperature_pollingtimer.data     = ( unsigned long )jiffies;
    temperature_pollingtimer.function = temperature_pollingtimer_handler;
                                    /*polling timer start                     */
	del_timer(&temperature_pollingtimer);
    add_timer( &temperature_pollingtimer );
    pollingtimer_stopflg = D_TEMPDD_FLG_ON;
    
    MODULE_TRACE_END();
    return D_TEMPDD_RET_OK;
}

/******************************************************************************/
/*  function    : twl4030 fuction call back                                   */
/******************************************************************************/
static int temperature_check_workcb(int len, int channels, int *buf)
{
    int i,j;
    struct sensor_state_t * sensor_sts_wp;
    int sensor_num;
    int sensor_temperature;
    int timer1sec_reqflg_ntoa= 0;      /*1sec timer flg normal -> illegal     */
    int timer1sec_reqflg_aton= 0;      /*1sec timer flg illegal-> normal      */
    int channel;
    unsigned long flags;

/* Quad ADD start *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
    int s_year, s_month, s_day, s_hour, s_min, s_sec; /* <Quad> *//* npdc300050359 */
    unsigned long s_msec; /* <Quad> *//* npdc300050359 */
    struct timespec ts; /* <Quad> *//* npdc300050359 */
    struct rtc_time tm; /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
/* Quad ADD end *//* npdc300050359 */

    MODULE_TRACE_START();

/* Quad ADD start *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
    getnstimeofday(&ts); /* <Quad> *//* npdc300050359 */
    rtc_time_to_tm(ts.tv_sec + 9*60*60, &tm); /* <Quad> *//* npdc300050359 *//* UTC->JST */
    s_year = tm.tm_year + 1900; /* <Quad> *//* npdc300050359 */
    s_month =  tm.tm_mon + 1; /* <Quad> *//* npdc300050359 */
    s_day = tm.tm_mday; /* <Quad> *//* npdc300050359 */
    s_hour = tm.tm_hour; /* <Quad> *//* npdc300050359 */
    s_min = tm.tm_min; /* <Quad> *//* npdc300050359 */
    s_sec = tm.tm_sec; /* <Quad> *//* npdc300050359 */
    s_msec = ts.tv_nsec / 1000000; /* <Quad> *//* npdc300050359 *//* nsec->msec */
#endif /* <Quad> *//* npdc300050359 */
/* Quad ADD end *//* npdc300050359 */

	/*end AD conversion*/
	temperature_adconv_flg = 0;

    /* schedule state off */
    temp_schedule_state = 0;

    if(buf ==NULL)
    {
        return D_TEMPDD_RET_NG;
    }

    if(temperature_main_sts != D_TEMPDD_TM_STS_WATCH)
    {
        /*no action */
    }
    else
    {
    /*sensor status*/
        for(j=0;j<D_TEMPDD_SENSOR_NUM;j++)
        {
            if(j == D_TEMPDD_SENSOR1)
            {
                channel  = D_TEMPDD_CNANNEL_S1;
                sensor_sts_wp = &sensor1_sts;    
                sensor_num    = D_TEMPDD_SENSOR1; 
                /* iD Power MOD start */
                tdev_sts.ad_val.ad_sensor1 =
                		(unsigned int)(buf[channel]);
                /* AD                                                         */
                if(tdev_sts.ad_val.ad_sensor1 > D_TEMPDD_MAX_ADVAL)
                {
                    tdev_sts.ad_val.ad_sensor1 = D_TEMPDD_MAX_ADVAL;
                }
                /* iD Power MOD end */
                tdev_sts.temperature_val.t_sensor1 = temperature_adchange
                       			(tdev_sts.ad_val.ad_sensor1, channel); 
                sensor_temperature = 
                       		tdev_sts.temperature_val.t_sensor1;
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
                printk(KERN_EMERG"TEMPERATURE SENSOR: %d-%02d-%02d %02d:%02d:%02d.%03lu VTH7/6 AD=0x%4x, VAL=%4d\n", /* <Quad> *//* npdc300050359 */
                        s_year, s_month, s_day, s_hour, s_min, s_sec, s_msec, /* <Quad> *//* npdc300050359 */
                        tdev_sts.ad_val.ad_sensor1, /* <Quad> *//* npdc300050359 */
                        tdev_sts.temperature_val.t_sensor1); /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG   /* HARD_REQUEST_DEBUG */
                printk("VTH7/6 AD=0x%4x,TEMPERATURE SENSOR VAL=%4d\n", /* <Quad> */
                        tdev_sts.ad_val.ad_sensor1,
                        tdev_sts.temperature_val.t_sensor1);
#endif                      /* HARD_REQUEST_DEBUG */
    
            }
            else if(j == D_TEMPDD_SENSOR2)
            {
                channel = D_TEMPDD_CNANNEL_S2;
                sensor_sts_wp = &sensor2_sts;    
                sensor_num    = D_TEMPDD_SENSOR2;
                /* iD Power MOD start */
                tdev_sts.ad_val.ad_sensor2 =
                		(unsigned int)(buf[channel]);
                /* AD                                                         */
                if(tdev_sts.ad_val.ad_sensor2 > D_TEMPDD_MAX_ADVAL)
                {
                    tdev_sts.ad_val.ad_sensor2 = D_TEMPDD_MAX_ADVAL;
                }
                /* iD Power MOD end */
                tdev_sts.temperature_val.t_sensor2 = temperature_adchange
                        		(tdev_sts.ad_val.ad_sensor2, channel); 
                sensor_temperature = 
                        	tdev_sts.temperature_val.t_sensor2;                  
    
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
                printk(KERN_EMERG"TEMPERATURE SENSOR: %d-%02d-%02d %02d:%02d:%02d.%03lu VTH4 AD=0x%4x, VAL=%4d\n", /* <Quad> *//* npdc300050359 */
                            s_year, s_month, s_day, s_hour, s_min, s_sec, s_msec, /* <Quad> *//* npdc300050359 */
                            tdev_sts.ad_val.ad_sensor2, /* <Quad> *//* npdc300050359 */
                            tdev_sts.temperature_val.t_sensor2); /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG     /* HARD_REQUEST_DEBUG */
                printk("VTH4 AD=0x%4x,TEMPERATURE SENSOR VAL=%4d\n", /* <Quad> */
                            tdev_sts.ad_val.ad_sensor2,
                            tdev_sts.temperature_val.t_sensor2);
#endif                       /* HARD_REQUEST_DEBUG */
    
            }
            else if(j == D_TEMPDD_SENSOR3) /*sensor 3  */
            {
				
                channel = D_TEMPDD_CNANNEL_S3;
                sensor_sts_wp = &sensor3_sts;    
                sensor_num    = D_TEMPDD_SENSOR3;
                /* iD Power MOD start */
                tdev_sts.ad_val.ad_sensor3 =
                		(unsigned int)(buf[channel]);
                /* AD                                                         */
                if(tdev_sts.ad_val.ad_sensor3 > D_TEMPDD_MAX_ADVAL)
                {
                    tdev_sts.ad_val.ad_sensor3 = D_TEMPDD_MAX_ADVAL;
                }
                /* iD Power MOD end */
                tdev_sts.temperature_val.t_sensor3 = temperature_adchange
                        		(tdev_sts.ad_val.ad_sensor3, channel); 
                sensor_temperature = 
                		tdev_sts.temperature_val.t_sensor3;
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
                printk(KERN_EMERG"TEMPERATURE SENSOR: %d-%02d-%02d %02d:%02d:%02d.%03lu VTH1 AD=0x%4x, VAL=%4d\n", /* <Quad> *//* npdc300050359 */
                        s_year, s_month, s_day, s_hour, s_min, s_sec, s_msec, /* <Quad> *//* npdc300050359 */
                        tdev_sts.ad_val.ad_sensor3, /* <Quad> *//* npdc300050359 */
                        tdev_sts.temperature_val.t_sensor3); /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG      /* HARD_REQUEST_DEBUG */
                printk("VTH1 AD=0x%4x,TEMPERATURE SENSOR VAL=%4d\n",
                        tdev_sts.ad_val.ad_sensor3,
                        tdev_sts.temperature_val.t_sensor3);
#endif                         /* HARD_REQUEST_DEBUG */
            }
/* Quad ADD start */
            else if(j == D_TEMPDD_SENSOR4) /* sensor 4: カメラモジュール温度 *//* <Quad> */
            { /* <Quad> */
                int ret; /* <Quad> */
                int temp; /* <Quad> */
                sensor_sts_wp = &sensor4_sts; /* <Quad> */
                sensor_num    = D_TEMPDD_SENSOR4; /* <Quad> */
                ret = msm_camera_temperature_get(&temp); /* <Quad> */
                if ( ret == 0 ) ; /* <Quad> */
                else temp = tdev_sts.temperature_val.t_sensor4; /* <Quad> */
                tdev_sts.temperature_val.t_sensor4 = temp; /* <Quad> */
                sensor_temperature = tdev_sts.temperature_val.t_sensor4; /* <Quad> */
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
                printk(KERN_EMERG"TEMPERATURE SENSOR: %d-%02d-%02d %02d:%02d:%02d.%03lu CAM RET=%d, VAL=%4d\n", /* <Quad> *//* npdc300050359 */
                        s_year, s_month, s_day, s_hour, s_min, s_sec, s_msec, /* <Quad> *//* npdc300050359 */
                        ret, tdev_sts.temperature_val.t_sensor4); /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG      /* HARD_REQUEST_DEBUG */ /* <Quad> */
                printk("CAM RET=%d,TEMPERATURE SENSOR VAL=%4d\n", /* <Quad> */
                        ret, tdev_sts.temperature_val.t_sensor4); /* <Quad> */
#endif                         /* HARD_REQUEST_DEBUG */ /* <Quad> */
            } /* <Quad> */
/* Quad ADD end */
            else
            {
            	continue;
            }

            /*device status Update */
            for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
            {
				
				/*if the device is OMAP, don't check the sensor*/
				if( i == D_TEMPDD_OMAP_INDEX )
				{
					continue;
				}
/* Quad ADD start *//* Check sensor4 only for D_CAMERA_CAMERA_INDEX  */
				if ( ( sensor_num == D_TEMPDD_SENSOR4 ) && ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) ) continue; /* <Quad> */ 
                else ; /* <Quad> */
/* Quad ADD end */
#if 0
                if((i==0) && ((temp_hcmparam.vp_heat_suppress & M_BATTERY) == 0))
                {
                    i++;        /*no check battery                            */
                    ROUTE_CHECK("temp_hcmparam.vp_heat_suppress & M_BATTERY \n");
                }                    
                if((i==1) && ((temp_hcmparam.vp_heat_suppress & M_WLAN) == 0))
                {
                    i++;        /*no check WLAN                               */
                    ROUTE_CHECK("temp_hcmparam.vp_heat_suppress & M_WLAN \n");
                }                   
                if((i==2) && ((temp_hcmparam.vp_heat_suppress & M_OMAP) == 0))
                {
                    i++;        /*no check OMAP                               */
                    ROUTE_CHECK("temp_hcmparam.vp_heat_suppress & M_OMAP \n");
                }
                                /*no chekc camera                             */
                if((i==3) && ((temp_hcmparam.vp_heat_suppress & M_CAMERA) == 0))
                {
                    ROUTE_CHECK("temp_hcmparam.vp_heat_suppress & M_CAMERA \n");
                    break;
                }
#endif
                spin_lock_irqsave(&temp_spinlock,flags);
                /* check monitoring flag */
                if(((temp_hcmparam.vp_heat_suppress & judge_watch_device_table[i]) != 0) 
                    && ((check_dev_flg & judge_watch_device_table[i]) != 0))
                {
		spin_unlock_irqrestore(&temp_spinlock,flags);
                /* sensor status back up */
                sensor_sts_wp->sensor_sts_old[i] = sensor_sts_wp->sensor_sts_new[i];
    
                DBG_MSG("sensor_sts_wp->sensor_sts_old[i] ->",sensor_sts_wp->sensor_sts_old[i]);
                /*normal status */
                if(sensor_sts_wp->sensor_sts_old[i] == D_TEMPDD_NORMAL_SENSOR) 
                {
                    ROUTE_CHECK("path1  \n");
                    if(sensor_temperature < 
                        temp_hcmparam.dev_hcmparam[i].disable_temp[sensor_num]) 
                    {
                        ROUTE_CHECK("path2 \n");
                                            /*nomal status continue           */
                        sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; 
                    }
                    else
                    {
                        ROUTE_CHECK("path3\n");
                                    /*status change normal to illegal  */
                        sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_NORMALTOAB_SENSOR;
                        sensor_sts_wp->changing_num[i] = 1; /* <Quad> *//* npdc300050668 */
                    }
                }
                /*normalstatus -> illegal status */
                else if (sensor_sts_wp->sensor_sts_old[i] == D_TEMPDD_NORMALTOAB_SENSOR)
                {
                    ROUTE_CHECK("path4 \n");
                    if(sensor_temperature < 
                        temp_hcmparam.dev_hcmparam[i].disable_temp[sensor_num]) 
                    {
                        ROUTE_CHECK("path5 \n");
                                    /*status change  normal                   */
                        sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; 
                        sensor_sts_wp->changing_num[i]=0;
                    }
                    else             /*illegal temperature continue            */
                    {  
                        ROUTE_CHECK("path6\n");
                        if( ++(sensor_sts_wp->changing_num[i]) >= 
                            temp_hcmparam.dev_hcmparam[i].disable_time[sensor_num])
                        {
                            ROUTE_CHECK("path7 \n");
                                    /*status change illegal                     */
                            sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_ABNORMAL_SENSOR;
                            wakeup_flg_3g = 1;
                            wake_up_interruptible( &tempwait_q );
                        }
                        else         /*status retention     */
                        {
                            ROUTE_CHECK("path8 \n");
                        }
                    }
                }
                /*illegal temperature status        */
                else if (sensor_sts_wp->sensor_sts_old[i] == D_TEMPDD_ABNORMAL_SENSOR)
                {
                    ROUTE_CHECK("path9 \n");
                    if(sensor_temperature >
                        temp_hcmparam.dev_hcmparam[i].enable_temp[sensor_num]) 
                    {
                    ROUTE_CHECK("path10 \n");
                                    /*illegal temperature status retention    */
                        sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_ABNORMAL_SENSOR; 
                    }
                    else
					{    
          				ROUTE_CHECK("path11 \n");
                                 /*status change illegal to normal */
                        sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_ABNORMALTONO_SENSOR; 
                        sensor_sts_wp->changing_num[i] = 1; /* <Quad> *//* npdc300050668 */
                    }
                }
                /*status illegal to normal  */
                else if(sensor_sts_wp->sensor_sts_old[i] == D_TEMPDD_ABNORMALTONO_SENSOR) 
                {
                    ROUTE_CHECK("path12 \n");
                    if(sensor_temperature >
                        temp_hcmparam.dev_hcmparam[i].enable_temp[sensor_num]) 
                    {
                    ROUTE_CHECK("path13 \n");
                                    /*status change illegal                   */
                        sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_ABNORMAL_SENSOR; 
                        sensor_sts_wp->changing_num[i]=0;
                    }
                    else
                    {
                        ROUTE_CHECK("path14 \n");
                        if( ++(sensor_sts_wp->changing_num[i]) >= 
                            temp_hcmparam.dev_hcmparam[i].enable_time[sensor_num])
                        {
                                    /*status change normal                   */
                            ROUTE_CHECK("path15 \n");
                            sensor_sts_wp->sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR;
                            wakeup_flg_3g = 1;
                            wake_up_interruptible( &tempwait_q );
                        }    
                        else
                        {
                            ROUTE_CHECK("path16 \n");
                        }
                    }
                }
                else
                {
                    /*no action path*/
                }
                
                }
		else
		{
			spin_unlock_irqrestore(&temp_spinlock,flags);
		}
           }
        }
    }

    for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++) 
    {
		
		/*if the device is OMAP, don't check the sensor*/
		if( i == D_TEMPDD_OMAP_INDEX )
		{
			continue;
		}

        DBG_MSG(" i ->",i);
		
        DBG_MSG(" sensor1_sts.changing_num[i] ->",sensor1_sts.changing_num[i]);
        DBG_MSG(" sensor2_sts.changing_num[i] ->",sensor2_sts.changing_num[i]);
        DBG_MSG(" sensor3_sts.changing_num[i] ->",sensor3_sts.changing_num[i]);
        DBG_MSG(" sensor4_sts.changing_num[i] ->",sensor4_sts.changing_num[i]); /* <Quad> */
/* Quad MOD start */
        if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&   /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&   /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) || /* <Quad> */
             ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&  /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&  /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) &&  /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) ) /* <Quad> */
/* Quad MOD end */
        {
            ROUTE_CHECK("1sec timer no start enable_sensor time all 0xff \n");
        }
        else
        {
            /*if all sensor status changing status. 1sec timer start flg ON        */
			
/* Quad MOD start */
            if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
                   ( (sensor1_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) &&   /* <Quad> */
                   ( (sensor2_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) &&   /* <Quad> */
                   ( (sensor3_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) ) || /* <Quad> */
                 ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
                   ( (sensor1_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) &&  /* <Quad> */
                   ( (sensor2_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) &&  /* <Quad> */
                   ( (sensor3_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) &&  /* <Quad> */
                   ( (sensor4_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].enable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) ) ) /* <Quad> */
/* Quad MOD end */
                {
                    timer1sec_reqflg_ntoa=D_TEMPDD_FLG_ON;
                }
        }
    }
    for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
    {
/* Quad MOD start */
        if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&   /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&   /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) || /* <Quad> */
             ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) &&  /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) &&  /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) &&  /* <Quad> */
               ( temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) ) /* <Quad> */
/* Quad MOD end */
        {
            ROUTE_CHECK("1sec timer no start disable_sensor time all 0xff \n");
        }
        else
        {
/* Quad MOD start */
            if ( ( ( i != D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
                   ( (sensor1_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) &&   /* <Quad> */
                   ( (sensor2_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) &&   /* <Quad> */
                   ( (sensor3_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) ) || /* <Quad> */
                 ( ( i == D_TEMPDD_CAMERA_CAMERA_INDEX ) && /* <Quad> */
                   ( (sensor1_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR1] == 0xFF ) ) &&  /* <Quad> */
                   ( (sensor2_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR2] == 0xFF ) ) &&  /* <Quad> */
                   ( (sensor3_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR3] == 0xFF ) ) &&  /* <Quad> */
                   ( (sensor4_sts.changing_num[i] != 0) || (temp_hcmparam.dev_hcmparam[i].disable_time[D_TEMPDD_SENSOR4] == 0xFF ) ) ) ) /* <Quad> */
/* Quad MOD End */
                {
                    timer1sec_reqflg_aton=D_TEMPDD_FLG_ON;
                }
        }
    }
                                  /*if normal status -> illegal status changing or   */
                                  /*illegal status -> normal status start 1sec timer */
    if((timer1sec_reqflg_ntoa == D_TEMPDD_FLG_ON) || 
        (timer1sec_reqflg_aton == D_TEMPDD_FLG_ON))
    {
        if(onesectimer_stopflg == D_TEMPDD_FLG_OFF)
        {
            onesectimer_stopflg = D_TEMPDD_FLG_ON;           /*1sec timer       */
            temperature_1sectimer.expires  = jiffies +HZ*1;  /* HZ×1 =1sec     */ 
            temperature_1sectimer.data     = ( unsigned long )jiffies;
            temperature_1sectimer.function = temperature_1sectimer_handler;
		del_timer(&temperature_1sectimer);
            add_timer( &temperature_1sectimer );        
        }
        else
        {
            ROUTE_CHECK("1sec timer double start check \n");
        }
    }    

    MODULE_TRACE_END();
    return D_TEMPDD_RET_OK;
}

/******************************************************************************/
/*  function    : 1sec timer handler                                          */
/******************************************************************************/
static void temperature_1sectimer_handler( unsigned long data )
{
    int ret;
    
    MODULE_TRACE_START();

    /* schedule state on */
    temp_schedule_state = 1;
    ret = schedule_work( &tempwork_1secpollingtask );
    if( ret == 1 )
    {
        ROUTE_CHECK("temperature DD : workque create \n");
    }
    else
    {
        printk("temperature DD : workque crate error(temperature_1sectimer_handler)\n");
    }
    onesectimer_stopflg = D_TEMPDD_FLG_OFF;
    
    MODULE_TRACE_END();
}

/******************************************************************************/
/*  fuction     : temperature check fuction                                   */
/******************************************************************************/
/**
 * iD Power              
 * - AD          safety_ic_current_temp_adc()      
 * - CB                              
 */
static int temperature_twl_conv(void)
{
    int val;
    /* iD Power MOD start */
    int rbuf[DSAFETY_ADC_MAX_CHANNELS];
    int len=0;
    int channels=0;
	unsigned long flags;
	
    MODULE_TRACE_START();
	
	/*prevent competition*/
	spin_lock_irqsave(&temp_spinlock,flags);
	if(temperature_adconv_flg == 1){
		/*timer set*/
		temperature_adconv_timer.expires = jiffies + HZ / 100;/*10msec*/ 
		temperature_adconv_timer.data     = ( unsigned long )jiffies;
		temperature_adconv_timer.function
					= (void *)temperature_retry_handler;
		del_timer(&temperature_adconv_timer);
		add_timer(&temperature_adconv_timer);
		temperature_adconv_type = D_TEMPDD_ADCONV_3G;
		
		spin_unlock_irqrestore(&temp_spinlock,flags);
		return D_TEMPDD_RET_OK;
	} else{
		temperature_adconv_flg = 1; /*begining AD conversion*/
	}
	spin_unlock_irqrestore(&temp_spinlock,flags);

    /* AD             */
    /* bit1,4,7       */
/* Quad MOD start */
    channels = D_TEMPDD_CNANNEL_SET_S1 | D_TEMPDD_CNANNEL_SET_S1_2 | D_TEMPDD_CNANNEL_SET_S2 | D_TEMPDD_CNANNEL_SET_S3; /* <Quad> *//* VTH7,6,4,1 */
/* Quad MOD end */
    val = safety_ic_current_temp_adc(DSAFETY_TEMP_OUT, channels, rbuf);

    if(val < 0)                 
    {                          
        ROUTE_CHECK("temperature DD : safety_ic_current_temp_adc error  \n");
        rbuf[D_TEMPDD_CNANNEL_S1] = tdev_sts.ad_val.ad_sensor1;
        rbuf[D_TEMPDD_CNANNEL_S2] = tdev_sts.ad_val.ad_sensor2;
        rbuf[D_TEMPDD_CNANNEL_S3] = tdev_sts.ad_val.ad_sensor3;
        temperature_check_workcb(len,channels,rbuf);
        return D_TEMPDD_RET_NG;
    }

/* Quad ADD start */
#ifdef HARD_REQUEST_DEBUG /* <Quad> */
    printk(KERN_EMERG"VTH7/6/4/1: channels:%2x, S1(VTH7):%4x, S1_2(VTH6):%4x, S2(VTH4):%4x, S3(VTH1):%4x\n", channels, rbuf[D_TEMPDD_CNANNEL_S1], rbuf[D_TEMPDD_CNANNEL_S1_2], rbuf[D_TEMPDD_CNANNEL_S2], rbuf[D_TEMPDD_CNANNEL_S3]); /* <Quad> */
#endif /* <Quad> */
    if ( ( unsigned int )rbuf[D_TEMPDD_CNANNEL_S1] < ( unsigned int )rbuf[D_TEMPDD_CNANNEL_S1_2] ) { /* <Quad> *//* VTH7(3G_PA) < VTH6(2G_PA) */
        rbuf[D_TEMPDD_CNANNEL_S1] = rbuf[D_TEMPDD_CNANNEL_S1_2]; /* <Quad> */
    } /* <Quad> */
#ifdef HARD_REQUEST_DEBUG /* <Quad> */
    printk(KERN_EMERG"VTH7/6/4/1: channels:%2x, S1(VTH7):%4x, S1_2(VTH6):%4x, S2(VTH4):%4x, S3(VTH1):%4x\n", channels, rbuf[D_TEMPDD_CNANNEL_S1], rbuf[D_TEMPDD_CNANNEL_S1_2], rbuf[D_TEMPDD_CNANNEL_S2], rbuf[D_TEMPDD_CNANNEL_S3]); /* <Quad> */
#endif /* <Quad> */
/* Quad ADD end */

    /* AD             */
    if(temp_hcmparam.factory_adj_val > 0)
    {
        DBG_PRINT("before adjustment S1=0x%4x, S2=0x%4x, S3=0x%4x\n",
            rbuf[D_TEMPDD_CNANNEL_S1], rbuf[D_TEMPDD_CNANNEL_S2], rbuf[D_TEMPDD_CNANNEL_S3]);
        rbuf[D_TEMPDD_CNANNEL_S1] = 
            rbuf[D_TEMPDD_CNANNEL_S1] * D_TEMPDD_TYPICAL_VAL / temp_hcmparam.factory_adj_val;
        rbuf[D_TEMPDD_CNANNEL_S2] = 
            rbuf[D_TEMPDD_CNANNEL_S2] * D_TEMPDD_TYPICAL_VAL / temp_hcmparam.factory_adj_val;
        rbuf[D_TEMPDD_CNANNEL_S3] = 
            rbuf[D_TEMPDD_CNANNEL_S3] * D_TEMPDD_TYPICAL_VAL / temp_hcmparam.factory_adj_val;
        DBG_PRINT("after adjustment S1=0x%4x, S2=0x%4x, S3=0x%4x\n",
            rbuf[D_TEMPDD_CNANNEL_S1], rbuf[D_TEMPDD_CNANNEL_S2], rbuf[D_TEMPDD_CNANNEL_S3]);
    }
    else
    {
        DBG_PRINT("not adjust AD value\n");
    }

    /* CB                       */
    temperature_check_workcb(len,channels,rbuf);
    /* iD Power MOD end */
    
    MODULE_TRACE_END();
    return D_TEMPDD_RET_OK;
}

/******************************************************************************/
/*  function    : AD value  ->    transform                                   */
/******************************************************************************/
static int temperature_adchange(int buf, unsigned int channel)
{
    int ret;
    _D_TEMPDD_ADCHANGE_TABLE *chgtable;
	signed int table_num = 0;

    MODULE_TRACE_START(); 

    ret = D_TEMPDD_RET_NG;
	chgtable = ( _D_TEMPDD_ADCHANGE_TABLE * )&AdChangeTable1[0];
	table_num = temperature_adtable1_num;
	
    /* iD Power MOD start */
    /* AD                                                         */
    if( buf >= chgtable[table_num - 1].ad_val )
    {
        ret = chgtable[table_num - 1].temp;
        ROUTE_CHECK("MAX TEMPERATURE Path");
        return ret;
    }
    
    /* AD                                                         */
    if( chgtable[0].ad_val >= buf )
    {
        ret = chgtable[0].temp;
        ROUTE_CHECK("MIN TEMPERATURE Path");
        return ret;
    }

    for( ; chgtable->ad_val != 0 ; chgtable++ )
    {
        /* AD                                                         */
        if(( buf > (chgtable->ad_val)) && ( buf <= ((chgtable+1)->ad_val) ))
        {
            ret = (chgtable+1)->temp;
            break;
        }
    }    
    /* iD Power MOD end */
    MODULE_TRACE_END();
    return  ret;
}

/******************************************************************************/
/*  function    : polling timer                                               */
/******************************************************************************/
static void temperature_pollingtimer_handler( unsigned long data )
{
    int i,ret;
    
    MODULE_TRACE_START();
     for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
     {
        sensor1_sts.changing_num[i]=0;
        sensor2_sts.changing_num[i]=0;
        sensor3_sts.changing_num[i]=0;        
        sensor4_sts.changing_num[i]=0; /* <Quad> */
                                   /*  if device status is normal.            */
                                   /*  sensor status change normal.           */
        if(tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_NORMAL)
        {
            sensor1_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
            sensor2_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
            sensor3_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
            sensor4_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR; /* <Quad> */
        }
                                   /*if device status is illegal.             */
                                   /*sensor status change illegal.            */
        else if (tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_ABNORMAL)
        {
			
            sensor1_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
            sensor2_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
            sensor3_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
            sensor4_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR; /* <Quad> */
        }
        else
        {
            /*no action path*/
        }
    }
                                            /*                                */
    /* schedule state on */
    temp_schedule_state = 1;
    ret = schedule_work( &tempwork_pollingtask ); 
    if( ret == 1 )
    {
        ROUTE_CHECK("temperature DD : workque create \n");
    }
    else
    {
        printk("temperature DD : workque crate error(temperature_pollingtimer_handler)\n");
    }

    if(pollingtimer_stopflg == D_TEMPDD_FLG_OFF)
    {
        printk("polling timer start NG\n"); 
    }
    else
    {
        temperature_pollingtimer.expires  = jiffies +temp_hcmparam.vp_period*HZ ;
        temperature_pollingtimer.data     = ( unsigned long )jiffies;
        temperature_pollingtimer.function = temperature_pollingtimer_handler;
		del_timer(&temperature_pollingtimer);
        add_timer( &temperature_pollingtimer );
    }
    
    MODULE_TRACE_END();
}


/******************************************************************************/
/*  fuction     : ioctl                                                       */
/******************************************************************************/
/* iD Power MOD start */
/* Kernel                                 */
/* static int temperature_ioctl(struct inode *inode, struct file *filp,
            unsigned int cmd, unsigned long arg) */
static long temperature_ioctl(struct file *filp,
            unsigned int cmd, unsigned long arg)
/* iD Power MOD end */
{
    signed int         ret;
    unsigned long flags;
    struct dev_state_t tdev_sts_work;
/*    int                waittime;*/
/*    int                i;*/
    
    MODULE_TRACE_START();

    ret = D_TEMPDD_CLEAR;

    /* iD Power MOD start */
    /* Kernel                                 */
    /* if(( inode == NULL ) || ( filp == NULL )) */
    if( filp == NULL )
    /* iD Power MOD end */
    {
        ROUTE_CHECK("temperature DD: param error \n"); 
        return -EINVAL;
    }

    DBG_MSG("ioctl_cmd ->",cmd);
    DBG_HEXMSG(" [in] check_dev_flg ->",check_dev_flg);
 
    switch(cmd){
        case D_TEMPDD_IOCTL_GET :

            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_GET!!!");
            
            spin_lock_irqsave(&temp_spinlock,flags);
            tdev_sts_work.dev_sts[D_TEMPDD_BATTERY] =
	            	tdev_sts.dev_sts[D_TEMPDD_BATTERY_INDEX]
	            	| tdev_sts.dev_sts[D_TEMPDD_BATTERY_WLAN_INDEX]
	            	| tdev_sts.dev_sts[D_TEMPDD_BATTERY_CAMERA_INDEX];
            tdev_sts_work.dev_sts[D_TEMPDD_WLAN] =
            		tdev_sts.dev_sts[D_TEMPDD_WLAN_INDEX];
            tdev_sts_work.dev_sts[D_TEMPDD_OMAP] =
            		tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX];
            tdev_sts_work.dev_sts[D_TEMPDD_CAMERA] =
	            	tdev_sts.dev_sts[D_TEMPDD_CAMERA_INDEX]
/* Quad MOD/ADD start */
	            	| tdev_sts.dev_sts[D_TEMPDD_CAMERA_WLAN_INDEX]    /* <Quad> */
	            	| tdev_sts.dev_sts[D_TEMPDD_CAMERA_CAMERA_INDEX]; /* <Quad> */
/* Quad MOD/ADD end */
	        tdev_sts_work.dev_sts[D_TEMPDD_BATTERY_AMR] =
            		tdev_sts.dev_sts[D_TEMPDD_BATTERY_AMR_INDEX];
	        tdev_sts_work.dev_sts[D_TEMPDD_DTV_BKL] =
            		tdev_sts.dev_sts[D_TEMPDD_DTV_BKL_INDEX];
/* Quad ADD start */
	        tdev_sts_work.dev_sts[D_TEMPDD_BATTERY_DTV] =
	            	tdev_sts.dev_sts[D_TEMPDD_BATTERY_DTV_INDEX]; /* <Quad> */
/* Quad ADD end */
            tdev_sts_work.temperature_val = tdev_sts.temperature_val;
            tdev_sts_work.ad_val = tdev_sts.ad_val;
            spin_unlock_irqrestore(&temp_spinlock,flags);
            
            ret = copy_to_user((void __user *)arg,&tdev_sts_work,sizeof(tdev_sts_work));
            if(ret != 0)
            {
                    printk("temperature DD: copy_to_user error \n"); 
                    return -EINVAL; 
            }
            
            break;

	case D_TEMPDD_IOCTL_3G_ENABLE :
        
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_3G_ENABLE!!!");
            temperature_ioctl_start(D_M_3G);
#if 0 /* move temperature_ioctl_start() */            
            if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
            {
                ROUTE_CHECK("temperature DD: temp watch start \n"); 
                spin_lock_irqsave(&temp_spinlock,flags);
                temperature_main_sts = D_TEMPDD_TM_STS_WATCH; 
                spin_unlock_irqrestore(&temp_spinlock,flags);
                temperature_check_start();
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
            {
                del_timer(&temperatuer_waittimer);

                spin_lock_irqsave(&temp_spinlock,flags);
                temperature_main_sts = D_TEMPDD_TM_STS_WATCH; 
                spin_unlock_irqrestore(&temp_spinlock,flags);
                temperature_check_start();    
            }
            else
            {
                /*no action path*/
            }
#endif /* move */ 
            break;
       

        case D_TEMPDD_IOCTL_3G_P_ENABLE :
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_3G_P_ENABLE!!!");
            temperature_ioctl_start(D_M_3G_P);
            break;

        case D_TEMPDD_IOCTL_WLAN_ENABLE :
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_WLAN_ENABLE!!!");
            temperature_ioctl_start(D_M_WLAN);
            break;

        case D_TEMPDD_IOCTL_CAMERA_ENABLE :
#if 0 /*<npdc300036023> start */
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_CAMERA_ENABLE!!!");
            temperature_ioctl_start(D_M_CAMERA);
#endif /*<npdc300036023> end   */
            break;
            		
/* Quad ADD start */
        case D_TEMPDD_IOCTL_DTV_ENABLE :                    /* <Quad> */
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_DTV_ENABLE!!!"); /* <Quad> */
            temperature_ioctl_start(D_M_DTV);               /* <Quad> */
            break;
/* Quad ADD end */
            		
		case D_TEMPDD_IOCTL_3G_DISABLE :
             ROUTE_CHECK("!!!D_TEMPDD_IOCTL_3G_DISABLE!!!");
            if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
            {
                spin_lock_irqsave(&temp_spinlock,flags);
                if(check_dev_flg & M_3G_MODE)
                {
                    if(check_dev_flg & M_3G_P_MODE)
                    {
			/* npdc300035875 start */
                        check_dev_flg &= ~M_3G_MODE;
                        //check_dev_flg &= ~(M_BATTERY_AMR + M_3G_MODE);
			/* npdc300035875 end */
                    }
                    else
                    {
/* Quad MOD start */
                        if ( check_dev_flg & M_WLAN_MODE ) { /* <Quad> */
                            check_dev_flg = ((check_dev_flg & ~D_M_3G) | M_BATTERY_AMR); /* <Quad> */
                        } /* <Quad> */
                        else { /* <Quad> */
                            check_dev_flg &= ~D_M_3G; /* <Quad> */
                        } /* <Quad> */
/* Quad MOD end */
                    }
                }
                else
                {
                    spin_unlock_irqrestore(&temp_spinlock,flags);
                    ROUTE_CHECK("temperature DD: no action path \n"); 
                    break;
                }
                spin_unlock_irqrestore(&temp_spinlock,flags);
/* Quad MOD start */
                if ( tdev_sts.dev_sts[D_TEMPDD_BATTERY_AMR_INDEX] == D_TEMPDD_DEV_STS_ABNORMAL ) /* <Quad> *//* npdc300050671 */
                { /* <Quad> *//* npdc300050671 */
                    temperature_ioctl_end(temp_hcmparam.enable_time_3g); /* <Quad> *//* npdc300050671 */
                } /* <Quad> *//* npdc300050671 */
                else /* <Quad> *//* npdc300050671 */
                { /* <Quad> *//* npdc300050671 */
                    temperature_ioctl_end(temp_hcmparam.enable_time_3g_p); /* <Quad> *//* npdc300050671 */
                } /* <Quad> *//* npdc300050671 */
/* Quad MOD end */
#if 0 /* move temperature_ioctl_end() */
                spin_lock_irqsave(&temp_spinlock,flags);
                temperature_main_sts = D_TEMPDD_TM_STS_WAITTIMER; 
                spin_unlock_irqrestore(&temp_spinlock,flags);
                pollingtimer_stopflg = D_TEMPDD_FLG_OFF;
                del_timer(&temperature_pollingtimer);
                for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
                {
                    sensor1_sts.changing_num[i]=0;
                    sensor2_sts.changing_num[i]=0;
                    sensor3_sts.changing_num[i]=0;
                    if(tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_NORMAL)
                    {
                        sensor1_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
                        sensor2_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
                        sensor3_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
                     }
                        else if (tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_ABNORMAL)
                    {
                        sensor1_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
                        sensor2_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
                        sensor3_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
                    }
                    else
                    {
                             /*no action path*/
                    }
                } 
                del_timer(&temperature_1sectimer);
                onesectimer_stopflg = D_TEMPDD_FLG_OFF;
                
                if(temp_hcmparam.enable_time_3g == 0xFF)
                {
                    waittime =0x00;
                }
                else 
                {
                    waittime = temp_hcmparam.enable_time_3g;
                }    
                temperatuer_waittimer.expires  = 
                    jiffies + waittime*HZ  ;
                temperatuer_waittimer.data     = ( unsigned long )jiffies;
                temperatuer_waittimer.function = temperature_waittimer_handler;
				del_timer(&temperatuer_waittimer);
                add_timer( &temperatuer_waittimer );
#endif /* move */
            }

            else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
            {
                                    /*no action path                          */
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else
            {
                                    /*no action path                          */
            }    
            break;

        case D_TEMPDD_IOCTL_3G_P_DISABLE :
             ROUTE_CHECK("!!!D_TEMPDD_IOCTL_3G_P_DISABLE!!!");
            if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
            {
                spin_lock_irqsave(&temp_spinlock,flags);
                if(check_dev_flg & M_3G_P_MODE)
                {
                    if(check_dev_flg & M_3G_MODE)
                    {
                        check_dev_flg &= ~M_3G_P_MODE;
                    }
                    else
                    {
/* Quad MOD start */
                        if ( check_dev_flg & M_WLAN_MODE ) { /* <Quad> */
                            check_dev_flg = ((check_dev_flg & ~D_M_3G_P) | M_BATTERY_AMR); /* <Quad> */
                        } /* <Quad> */
                        else { /* <Quad> */
                            check_dev_flg &= ~D_M_3G_P; /* <Quad> */
                        } /* <Quad> */
/* Quad MOD end */
                    }
                }
                else
                {
                    spin_unlock_irqrestore(&temp_spinlock,flags);
                    ROUTE_CHECK("temperature DD: no action path \n"); 
                    break;
                }
                spin_unlock_irqrestore(&temp_spinlock,flags);
/* Quad MOD start */
                if ( tdev_sts.dev_sts[D_TEMPDD_BATTERY_AMR_INDEX] == D_TEMPDD_DEV_STS_ABNORMAL ) /* <Quad> *//* npdc300050671 */
                { /* <Quad> *//* npdc300050671 */
                    temperature_ioctl_end(temp_hcmparam.enable_time_3g); /* <Quad> *//* npdc300050671 */
                } /* <Quad> *//* npdc300050671 */
                else /* <Quad> *//* npdc300050671 */
                { /* <Quad> *//* npdc300050671 */
                    temperature_ioctl_end(temp_hcmparam.enable_time_3g_p); /* <Quad> *//* npdc300050671 */
                } /* <Quad> *//* npdc300050671 */
/* Quad MOD end */
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else
            {
                    /*no action path*/
            }    
            break;

        case D_TEMPDD_IOCTL_WLAN_DISABLE :
             ROUTE_CHECK("!!!D_TEMPDD_IOCTL_WLAN_DISABLE!!!");
            if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
            {
                spin_lock_irqsave(&temp_spinlock,flags);
                if(check_dev_flg & M_WLAN_MODE)
                {
/* Quad MOD start */
                    if ( (check_dev_flg & M_3G_MODE) || (check_dev_flg & M_3G_P_MODE) ) /* <Quad> */
                    { /* <Quad> */
                        check_dev_flg = ((check_dev_flg & ~D_M_WLAN) | M_BATTERY_AMR); /* <Quad> */
                    } /* <Quad> */
                    else /* <Quad> */
                    { /* <Quad> */
                        check_dev_flg &= ~D_M_WLAN; /* <Quad> */
                    } /* <Quad> */
/* Quad MOD end */
                }
                else
                {
                    spin_unlock_irqrestore(&temp_spinlock,flags);
                    ROUTE_CHECK("temperature DD: no action path \n"); 
                    break;
                }
                spin_unlock_irqrestore(&temp_spinlock,flags);

/* Quad MOD start */
                if ( tdev_sts.dev_sts[D_TEMPDD_BATTERY_AMR_INDEX] == D_TEMPDD_DEV_STS_ABNORMAL ) /* <Quad> *//* npdc300050671 */
                { /* <Quad> *//* npdc300050671 */
                    temperature_ioctl_end(temp_hcmparam.enable_time_3g); /* <Quad> *//* npdc300050671 */
                } /* <Quad> *//* npdc300050671 */
                else /* <Quad> *//* npdc300050671 */
                { /* <Quad> *//* npdc300050671 */
                    temperature_ioctl_end(temp_hcmparam.enable_time_wlan); /* <Quad> *//* npdc300050671 */
                } /* <Quad> *//* npdc300050671 */
/* Quad MOD end */
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else
            {
                    /*no action path*/
            }    
            break;

        case D_TEMPDD_IOCTL_CAMERA_DISABLE :
#if 0 /*<npdc300036023> start */
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_CAMERA_DISABLE!!!");
            if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
            {
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
            {
                spin_lock_irqsave(&temp_spinlock,flags);
                if(check_dev_flg & M_CAMERA_MODE)
                {
                    check_dev_flg &= ~D_M_CAMERA;
                    spin_unlock_irqrestore(&temp_spinlock,flags);
                    temperature_ioctl_end(temp_hcmparam.enable_time_camera);
                }
                else
                {
                    spin_unlock_irqrestore(&temp_spinlock,flags);
                    ROUTE_CHECK("temperature DD: no action path \n"); 
                    break;
                }
            }
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
            {
                                    /*no action path                          */
                ROUTE_CHECK("temperature DD: no action path \n"); 
            }
            else
            {
                                    /*no action path                          */
            }    
#endif /*<npdc300036023> end   */
            break;

/* Quad ADD start */
        case D_TEMPDD_IOCTL_DTV_DISABLE : /* <Quad> */
            ROUTE_CHECK("!!!D_TEMPDD_IOCTL_DTV_DISABLE!!!"); /* <Quad> */
            if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL) /* <Quad> */
            { /* <Quad> */
                ROUTE_CHECK("temperature DD: no action path \n");  /* <Quad> */
            } /* <Quad> */
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH) /* <Quad> */
            { /* <Quad> */
                spin_lock_irqsave(&temp_spinlock,flags); /* <Quad> */
                if(check_dev_flg & M_DTV_MODE) /* <Quad> */
                { /* <Quad> */
                    check_dev_flg &= ~D_M_DTV; /* <Quad> */
                    spin_unlock_irqrestore(&temp_spinlock,flags); /* <Quad> */
                    temperature_ioctl_end(temp_hcmparam.enable_time_dtv); /* <Quad> */
                } /* <Quad> */
                else /* <Quad> */
                { /* <Quad> */
                    spin_unlock_irqrestore(&temp_spinlock,flags); /* <Quad> */
                    ROUTE_CHECK("temperature DD: no action path \n");  /* <Quad> */
                    break; /* <Quad> */
                } /* <Quad> */
            } /* <Quad> */
            else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER) /* <Quad> */
            { /* <Quad> */
                                    /*no action path                          */ /* <Quad> */
                ROUTE_CHECK("temperature DD: no action path \n");  /* <Quad> */
            } /* <Quad> */
            else /* <Quad> */
            { /* <Quad> */
                                    /*no action path                          */ /* <Quad> */
            }     /* <Quad> */
            break; /* <Quad> */
/* Quad ADD end */

		case D_TEMPDD_IOCTL_OMAP_ENABLE:
			/*start OMAP temperature detection*/
			ROUTE_CHECK("!!!D_TEMPDD_IOCTL_OMAP_ENABLE!!!");
			if( temperature_omap_main_sts == D_TEMPDD_TM_STS_NORMAL){
				ROUTE_CHECK("temperature DD: temp watch start \n"); 
				spin_lock_irqsave(&temp_spinlock,flags);
				/*main status chenge to WATCH            */
				temperature_omap_main_sts = D_TEMPDD_TM_STS_WATCH; 
				spin_unlock_irqrestore(&temp_spinlock,flags);
				/*start OMAP temperature detection           */
				temperature_omap_twl_conv();
			}
			break;

		case D_TEMPDD_IOCTL_OMAP_DISABLE:
			/*end OMAP temperature detection*/
			ROUTE_CHECK("!!!D_TEMPDD_IOCTL_OMAP_DISABLE!!!");
			if(temperature_omap_main_sts == D_TEMPDD_TM_STS_WATCH){/*          */
				spin_lock_irqsave(&temp_spinlock,flags);     /*               */
				/*change main status of omap detection*/
				temperature_omap_main_sts = D_TEMPDD_TM_STS_NORMAL; 
				spin_unlock_irqrestore(&temp_spinlock,flags);/*               */
				
				/*initialize previous state*/
				temperature_prev_state_abs = D_TEMPDD_DEV_STS_NORMAL;
				temperature_prev_state_rel = D_TEMPDD_DEV_STS_NORMAL;

				/*stop omap temperature detection*/                
				del_timer(&temperature_omap_timer);/*polling           */

			}
			break;
        default:
            return -EINVAL;
    }
    DBG_HEXMSG(" [out] check_dev_flg ->",check_dev_flg);
    MODULE_TRACE_END();
    return  D_TEMPDD_RET_OK;
}

/******************************************************************************/
/*  fuction    : wait timer handler                                           */
/******************************************************************************/
static void temperature_waittimer_handler( unsigned long data )
{
    int i;
    unsigned long flags;
    MODULE_TRACE_START();
    spin_lock_irqsave(&temp_spinlock,flags);
    if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
        temperature_main_sts = D_TEMPDD_TM_STS_NORMAL;

    for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
    {
        if (~check_dev_flg & judge_watch_device_table[i]){
            sensor1_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR;
            sensor2_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR;
            sensor3_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR;
            sensor4_sts.sensor_sts_new[i]=D_TEMPDD_NORMAL_SENSOR; /* <Quad> */
        }
    }
    spin_unlock_irqrestore(&temp_spinlock,flags);
    wakeup_flg_3g = 1;
    wake_up_interruptible( &tempwait_q );
    
    MODULE_TRACE_END();
}

/******************************************************************************/
/* File Operation                                                             */
/******************************************************************************/
static struct file_operations temperature_fops = {
    .owner      = THIS_MODULE,
/*    .read        = temperature_read,      */
/*    .write        = temperature_write,    */
    .poll       = temperature_poll,
/* iD Power MOD start */
/* Kernel                      IF     */
/*  .ioctl      = temperature_ioctl, */
    .unlocked_ioctl = temperature_ioctl, 
/* iD Power MOD end */
    .open       = temperature_open,
    .release    = temperature_release
};

static struct miscdevice temperature_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "temperature",
    .fops = &temperature_fops
};


/******************************************************************************/
/*  function    : init                                                        */
/******************************************************************************/
static int __init temperature_init(void)
{
    int ret;

    MODULE_TRACE_START();
     
    ret = misc_register(&temperature_dev);
    if (ret) {
        ROUTE_CHECK("temperature DD: fail to misc_register (MISC_DYNAMIC_MINOR)\n");
        return ret;
    }

    init_waitqueue_head( &tempwait_q );
    spin_lock_init( &temp_spinlock );
    init_timer( &temperature_1sectimer );
    init_timer( &temperature_pollingtimer );
    init_timer( &temperatuer_waittimer );
	init_waitqueue_head( &temperature_omap_wait_q );
	init_timer( &temperature_omap_timer );
	init_timer( &temperature_adconv_timer );

    INIT_WORK( &tempwork_1secpollingtask,(void *)&temperature_twl_conv );
    INIT_WORK( &tempwork_pollingtask,(void *)&temperature_twl_conv );
    INIT_WORK( &temperature_omap_worktask,(void *)&temperature_omap_twl_conv );
    INIT_WORK( &temperature_retry_worktask_omap,(void *)&temperature_omap_twl_conv );
    INIT_WORK( &temperature_retry_worktask,(void *)&temperature_twl_conv );

	/*count number of AD table*/
	while(AdChangeTable1[temperature_adtable1_num].ad_val != 0)
		temperature_adtable1_num++;

#ifdef CONFIG_SYSFS
#ifdef TEMPDD_DEBUG_TEST
	/*initialize kobject*/
	temperature_kobj = kobject_create_and_add("temperature",NULL);
	if(!temperature_kobj)
		return -ENOMEM;
    ret = sysfs_create_group(temperature_kobj, &temperature_attr_group);
#endif /*TEMPDD_DEBUG_TEST*/
#endif /*CONFIG_SYSFS*/

    ROUTE_CHECK("temperature DD :Succefully loaded.\n");
    MODULE_TRACE_END();
    return 0;
}

/******************************************************************************/
/*  function    : cleanup                                                     */
/******************************************************************************/

static void __exit temperature_cleanup(void)
{
    MODULE_TRACE_START();
    misc_deregister(&temperature_dev);
     ROUTE_CHECK("temperature DD : Unloaded.\n"); 
    MODULE_TRACE_END();
}

MODULE_DESCRIPTION("TEMPERATURE driver");
MODULE_LICENSE("GPL");
module_init(temperature_init);
module_exit(temperature_cleanup);


/******************************************************************************/
/*  function    : device status interface                                     */
/******************************************************************************/
int temperature_getdevsts(int device,int * devsts)
{
    if(devsts == NULL)                               
    {
        return D_TEMPDD_RET_PARAM_NG;
    }
    switch(device)
    {
        case D_TEMPDD_BATTERY :
        *devsts = tdev_sts.dev_sts[D_TEMPDD_BATTERY_INDEX]
                  | tdev_sts.dev_sts[D_TEMPDD_BATTERY_WLAN_INDEX]
                  | tdev_sts.dev_sts[D_TEMPDD_BATTERY_CAMERA_INDEX];
        break;
        case D_TEMPDD_WLAN :
        *devsts = tdev_sts.dev_sts[D_TEMPDD_WLAN_INDEX];
        break;        
        case D_TEMPDD_OMAP :
        *devsts = tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX];
        break;
        case D_TEMPDD_CAMERA :
        *devsts = tdev_sts.dev_sts[D_TEMPDD_CAMERA_INDEX]
/* Quad MOD/ADD start */
                  | tdev_sts.dev_sts[D_TEMPDD_CAMERA_WLAN_INDEX]    /* <Quad> */
                  | tdev_sts.dev_sts[D_TEMPDD_CAMERA_CAMERA_INDEX]; /* <Quad> */
/* Quad MOD/ADD end */
        break;
        case D_TEMPDD_BATTERY_AMR :
        *devsts = tdev_sts.dev_sts[D_TEMPDD_BATTERY_AMR_INDEX];
        break;  
        case D_TEMPDD_DTV_BKL :
        *devsts = tdev_sts.dev_sts[D_TEMPDD_DTV_BKL_INDEX];
        break;  
/* Quad ADD start */
        case D_TEMPDD_BATTERY_DTV : /* < Quad> */
        *devsts = tdev_sts.dev_sts[D_TEMPDD_BATTERY_DTV_INDEX]; /* <Quad> */
        break;  
/* Quad ADD end */
        default:
        return D_TEMPDD_RET_PARAM_NG;
    }
    return D_TEMPDD_RET_OK;
}
EXPORT_SYMBOL(temperature_getdevsts);

int temperature_gpadcsuspend(int state)
{
    int  ret;
    unsigned long flags;
    unsigned int cur_state;

    spin_lock_irqsave(&temp_spinlock,flags);
    if(state == 0){
        if( temperature_omap_main_sts == D_TEMPDD_TM_STS_WATCH ){
            del_timer(&temperature_omap_timer);
        }

        if( pollingtimer_stopflg != D_TEMPDD_FLG_OFF){
            del_timer(&temperature_pollingtimer);
        }

        if( onesectimer_stopflg != D_TEMPDD_FLG_OFF){
            del_timer(&temperature_1sectimer);
        }
    } else{
        if( temperature_omap_main_sts == D_TEMPDD_TM_STS_WATCH ){
            cur_state = tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX];
            temperature_omap_timer.expires = jiffies +
                temperature_omap_hcmparam[cur_state].timer * HZ;
            temperature_omap_timer.data     = ( unsigned long )jiffies;
            temperature_omap_timer.function = temperature_omap_timer_handler;
            del_timer(&temperature_omap_timer);
            add_timer(&temperature_omap_timer);
        }

        if( pollingtimer_stopflg != D_TEMPDD_FLG_OFF ){
            temperature_pollingtimer.expires  = jiffies +temp_hcmparam.vp_period*HZ ;
            temperature_pollingtimer.data     = ( unsigned long )jiffies;
            temperature_pollingtimer.function = temperature_pollingtimer_handler;
            del_timer(&temperature_pollingtimer);
            add_timer(&temperature_pollingtimer);
        }

        if( onesectimer_stopflg != D_TEMPDD_FLG_OFF ){
            temperature_1sectimer.expires  = jiffies +HZ*1;
            temperature_1sectimer.data     = ( unsigned long )jiffies;
            temperature_1sectimer.function = temperature_1sectimer_handler;
            del_timer(&temperature_1sectimer);
            add_timer(&temperature_1sectimer);
        }
    }
    ret = (omap_schedule_state | temp_schedule_state);
    spin_unlock_irqrestore(&temp_spinlock,flags);

    return ret;
}
EXPORT_SYMBOL(temperature_gpadcsuspend);

static void temperature_ioctl_start(int status)
{
    unsigned long flags;

    ROUTE_CHECK("!!!D_TEMPDD_IOCTL_ENABLE!!!");
    if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
    {
        ROUTE_CHECK("temperature DD: temp watch start \n"); 

        spin_lock_irqsave(&temp_spinlock,flags);
        temperature_main_sts = D_TEMPDD_TM_STS_WATCH; 
        check_dev_flg |= status;
        spin_unlock_irqrestore(&temp_spinlock,flags);
        temperature_check_start();
    }
    else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
    {
        ROUTE_CHECK("temperature DD: temp watch continue \n"); 
        spin_lock_irqsave(&temp_spinlock,flags);
        check_dev_flg |= status;
        spin_unlock_irqrestore(&temp_spinlock,flags);
/*        ROUTE_CHECK("temperature DD: no action path \n");  */
    }
    else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
    {
        ROUTE_CHECK("temperature DD: temp wait watch start \n"); 
        del_timer(&temperatuer_waittimer);

        spin_lock_irqsave(&temp_spinlock,flags);
        temperature_main_sts = D_TEMPDD_TM_STS_WATCH; 
        check_dev_flg |= status;
        spin_unlock_irqrestore(&temp_spinlock,flags);
        temperature_check_start();    
    }
    else
    {
        ;
    }
}

static void temperature_ioctl_end(int enable_time)
{
    int i;
    unsigned long flags;
    int waittime;

    spin_lock_irqsave(&temp_spinlock,flags);
    if(check_dev_flg == 0)
    {
        temperature_main_sts = D_TEMPDD_TM_STS_WAITTIMER;
        pollingtimer_stopflg = D_TEMPDD_FLG_OFF;
        del_timer_sync(&temperature_pollingtimer);
        del_timer(&temperature_1sectimer);
        onesectimer_stopflg = D_TEMPDD_FLG_OFF;
        temp_schedule_state = 0;
    }

    for(i=0;i<D_TEMPDD_DEVNUM_LOCAL;i++)
    {
        if(~check_dev_flg & judge_watch_device_table[i]) { /* <Quad> *//* npdc300050673 */
            sensor1_sts.changing_num[i]=0;
            sensor2_sts.changing_num[i]=0;
            sensor3_sts.changing_num[i]=0;
            sensor4_sts.changing_num[i]=0; /* <Quad> */

            if(tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_NORMAL)
            {
                sensor1_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
                sensor2_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
                sensor3_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR;
                sensor4_sts.sensor_sts_new[i] = D_TEMPDD_NORMAL_SENSOR; /* <Quad> */
            }

            else if (tdev_sts.dev_sts[i] == D_TEMPDD_DEV_STS_ABNORMAL)
            {
                sensor1_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
                sensor2_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
                sensor3_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR;
                sensor4_sts.sensor_sts_new[i] = D_TEMPDD_ABNORMAL_SENSOR; /* <Quad> */
            }
            else
            {
                ;
            }
        }
    }
    spin_unlock_irqrestore(&temp_spinlock,flags);

    del_timer( &temperatuer_waittimer );
    if(enable_time == 0xFF)
    {
        waittime =0x00;
    }
    else
    {
        waittime = enable_time;
    }
    temperatuer_waittimer.expires  =
        jiffies + waittime*HZ  ;
    temperatuer_waittimer.data     = ( unsigned long )jiffies;
    temperatuer_waittimer.function = temperature_waittimer_handler;
    add_timer( &temperatuer_waittimer );

}
/*time stamp function  from FOMA*/
/*******************************************************************************
 * MODULE       : syserr_timstamp
 * ABSTRACT     :                 
 * FUNCTION     : syserr_timstamp( unsigned long *year,
 *                                 unsigned long *month,
 *                                 time_t *day,
 *                                 time_t *hour,
 *                                 time_t *min,
 *                                 time_t *sec )
 * PARAMETER    : year  :       
 *                month :   
 *                day   :   
 *                hour  :   
 *                min   :   
 *                sec   :   
 * RETURN       : year  :       
 *                month :   
 *                day   :   
 *                hour  :   
 *                min   :   
 *                sec   :   
 * NOTE         : 
 * CREATE       : 04.06.14 Ver0.1 GT odaka
 * UPDATE       : 2005/07/13 PMSE      <D1IO092> ERRCTL                      
 *              : 2005/09/16 PMSE      <8602253> RuleC    
 ******************************************************************************/
 
void syserr_TimeStamp2( unsigned long *year,
                       unsigned long *month,
                       time_t *day,
                       time_t *hour,
                       time_t *min,
                       time_t *sec )
{
    time_t         work;
    unsigned short i,tmp_month=1;
    unsigned long  tmp_year = 1970;
    unsigned char  day_of_month[ MONTH_OF_YEAR ];/*<8602253>                  */


    /*                  */
    day_of_month[ 0 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 1 ]  = 28;                     /*<8602253>                  */
    day_of_month[ 2 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 3 ]  = 30;                     /*<8602253>                  */
    day_of_month[ 4 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 5 ]  = 30;                     /*<8602253>                  */
    day_of_month[ 6 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 7 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 8 ]  = 30;                     /*<8602253>                  */
    day_of_month[ 9 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 10 ] = 30;                     /*<8602253>                  */
    day_of_month[ 11 ] = 31;                     /*<8602253>                  */

    /* NULL         */
    if(( year == NULL ) ||
       ( month == NULL )||
       ( day == NULL )  ||
       ( hour == NULL ) ||
       ( min == NULL )  ||
       ( sec  == NULL ))
    {
        return;
    }

    /*                    */
    /* iD Power MOD start */
    /* Kernel                      xtime                     */
    /* work = xtime.tv_sec; */
    work = current_kernel_time().tv_sec;
    /* iD Power MOD end */

    /*                     ? */
    if (work < 0)
    {
        /* 2004/01/01 00:00:00 */
        *year = 2004;
        *month = 1;
        *day = 1;
        *hour = 0;
        *min = 0;
        *sec = 0;
    }

    /*                + 9                     */
    work += 9 * 60 * 60;

    /*          */
    *sec = work % 60;

    /*          */
    work -= *sec;
    work /= 60;
    *min = work % 60;

    /*            */
    work -= *min;
    work /= 60;
    *hour = work % 24;

    work -= *hour;           /*          */
    *day = work / 24;        /* 1970                            */

    while( *day >= 366 )   /* 1970                            *//*<D1IO092>  M*/
    {
        /*       ? */
        if((( tmp_year % 4 ) == 0 ) &&
           ((( tmp_year % 100 ) != 0 ) || (( tmp_year % 400 ) == 0 )))
        {
            *day -= 366;    /*         366       */
        }
        else
        {
            *day -= 365;        /*                 365        */
        }

        tmp_year++;
    }

    *day += 1;                  /* 1    1      0        */


    if((( tmp_year % 4 ) == 0 ) &&
       ((( tmp_year % 100 ) != 0 ) || (( tmp_year % 400 ) == 0 )))
    {
        /*                              */
        day_of_month[ FEBRUARY ] = LEAPMONTH;    /*<8602253>                  */
    }
    else
    {
        /*                                    */
        day_of_month[ FEBRUARY ] = NORMALMONTH;  /*<8602253>                  */
    }

    /* day                              */
    for( i = 0; i < MONTH_OF_YEAR; i++ )          /*<D1IO092>  D              */
    {
        if(( unsigned long )*day <= *( day_of_month + ( tmp_month - 1 )))
        {
            break;
        }
        else 
        {
            *day -= *( day_of_month + ( tmp_month - 1 ));
            tmp_month++;
        }
    }

    /* 1  1       */
    if( tmp_month == 13 )
    {
        tmp_month = 1;
        tmp_year += 1;
    }

    if( tmp_year < 2004 )
    {
        /* 2004/01/01 00:00:00 */
        *year = 2004;
        *month = 1;
        *day = 1;
        *hour = 0;
        *min = 0;
        *sec = 0;

        return;
    }

    *year = tmp_year;                                              /* pgr0541 */
    *month = tmp_month;                                            /* pgr0541 */
}

/**
 *  temperature_omap_timer_handler - timer handler for temperature detection
 *  @date 2011.03.02
 *  @author NTTD-MSE T.higashikawa
 *  @data unused
 *
 *  This function creates work queue for AD conversion of omap temperature
 *  detection.
**/
static void temperature_omap_timer_handler( unsigned long data )
{
	int ret;

	MODULE_TRACE_START();

	/* schedule state on */
	omap_schedule_state = 1;

	/*create work queue*/
	ret = schedule_work( &temperature_omap_worktask ); 
	if( ret == 1 ){
		ROUTE_CHECK("temperature DD : workque create \n");
	} else{ 
		printk("temperature DD : workque crate error(temperature_omap_timer_handler)\n");/*ERROR     */
	}
	
	MODULE_TRACE_END();
}

/**
 *  temperature_retry_handler - timer handler for AD conversion
 *  @date 2011.03.02
 *  @author NTTD-MSE T.higashikawa
 *  @data unused
 *
 *  This function creates work queue, when AD conversion is competing.
**/
static void temperature_retry_handler(unsigned long data)
{
	int ret = 0;

	MODULE_TRACE_START();
	
	/*create work queue*/
	if(temperature_adconv_type == D_TEMPDD_ADCONV_OMAP){
                /* schedule state on */
		omap_schedule_state = 1;
		ret = schedule_work( &temperature_retry_worktask_omap );
	} else if(temperature_adconv_type == D_TEMPDD_ADCONV_3G){
                /* schedule state on */
                temp_schedule_state = 1;
		ret = schedule_work( &temperature_retry_worktask );
	} else{
		printk("adconv_type error\n");
		return;
	}
	if( ret == 1 ){
		ROUTE_CHECK("temperature DD : workque create \n");
	} else{ 
		printk("temperature DD : workque crate error(temperature_retry_handler)\n");/*ERROR     */
	}

	MODULE_TRACE_END();
}

/**
 *  temperature_omap_twl_conv - AD conversion to TWL6030
 *  @date 2011.03.02
 *  @author NTTD-MSE T.higashikawa
 *
 *  This function requests AD conversion to TWL6030 for omap temperature
 *  detection.
**/
/**
 * iD Power              
 * - AD          safety_ic_current_temp_adc()      
 * - CB                              
 * -                             
 */
static int temperature_omap_twl_conv(void)
{
	int val;
	int rbuf[DSAFETY_ADC_MAX_CHANNELS];
	int len=0;
	int channels=0;
	unsigned long flags;

	 MODULE_TRACE_START();
	 
	/*prevent competition*/
	spin_lock_irqsave(&temp_spinlock,flags);
	if(temperature_adconv_flg == 1){
		/*timer set*/
		temperature_adconv_timer.expires = jiffies + HZ / 100;/*10msec*/ 
		temperature_adconv_timer.data     = ( unsigned long )jiffies;
		temperature_adconv_timer.function
					= (void *)temperature_retry_handler;
		del_timer(&temperature_adconv_timer);
		add_timer(&temperature_adconv_timer);
		temperature_adconv_type = D_TEMPDD_ADCONV_OMAP;
		spin_unlock_irqrestore(&temp_spinlock,flags);
		return D_TEMPDD_RET_OK;
	} else{
		temperature_adconv_flg = 1; /*begining AD conversion*/
	}
	spin_unlock_irqrestore(&temp_spinlock,flags);
	
    /* iD Power MOD start */
    /* AD             */
    /* bit1(VTH1, MSM)       */
    channels = D_TEMPDD_CNANNEL_SET_S3;
    val = safety_ic_current_temp_adc(DSAFETY_TEMP_OUT, channels, rbuf);

	/*if AD conversion failed, the last value is used */
	/*and call cb_function                            */
	if(val < 0){  
		ROUTE_CHECK("temperature DD : safety_ic_current_temp_adc error  \n");
		rbuf[D_TEMPDD_CNANNEL_S3] = tdev_sts.ad_val.ad_sensor3;
	    /*                      */
		/* rbuf[D_TEMPDD_CNANNEL_S4] = tdev_sts.ad_val.ad_sensor4; */
		temperature_omap_check_workcb(len,channels,rbuf);
		return D_TEMPDD_RET_NG;
	}

    /* AD             */
    if(temp_hcmparam.factory_adj_val > 0)
    {
        DBG_PRINT("before adjustment S3=0x%4x\n", rbuf[D_TEMPDD_CNANNEL_S3]);
        rbuf[D_TEMPDD_CNANNEL_S3] = 
            rbuf[D_TEMPDD_CNANNEL_S3] * D_TEMPDD_TYPICAL_VAL / temp_hcmparam.factory_adj_val;
        DBG_PRINT("after adjustment S3=0x%4x\n", rbuf[D_TEMPDD_CNANNEL_S3]);
    }
    else
    {
        DBG_PRINT("not adjust AD value\n");
    }

    /* CB                       */
    temperature_omap_check_workcb(len, channels, rbuf);
    /* iD Power MOD end */

	MODULE_TRACE_END();
	return D_TEMPDD_RET_OK;
}

/**
 *  temperature_omap_check_workcb - callback function for AD conversion
 *  @date 2011.03.02
 *  @author NTTD-MSE T.higashikawa
 *
 *  This function is callback function for TWL6030 AD conversion. It gets
 *  the AD vaule and deside the state of omap temperature detection.
**/
static int temperature_omap_check_workcb(int len, int channels, int *buf)
{
	signed int channel;
	unsigned int new_state, cur_state;
    int ret;    /* iDPower ADD coverity 12400 */
	unsigned long flags;         /*<npdc300036023>*/
	int in_cam_power_state  = 0; /*<npdc300036023>*/
	int out_cam_power_state = 0; /*<npdc300036023>*/

/* Quad ADD start *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
    int s_year, s_month, s_day, s_hour, s_min, s_sec; /* <Quad> *//* npdc300050359 */
    unsigned long s_msec; /* <Quad> *//* npdc300050359 */
    struct timespec ts; /* <Quad> *//* npdc300050359 */
    struct rtc_time tm; /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
/* Quad ADD end *//* npdc300050359 */
	int tret, curr; /* <Quad> *//* PTC */
	
	MODULE_TRACE_START();

/* Quad ADD start *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
    getnstimeofday(&ts); /* <Quad> *//* npdc300050359 */
    rtc_time_to_tm(ts.tv_sec + 9*60*60, &tm); /* <Quad> *//* npdc300050359 *//* UTC->JST */
    s_year = tm.tm_year + 1900; /* <Quad> *//* npdc300050359 */
    s_month =  tm.tm_mon + 1; /* <Quad> *//* npdc300050359 */
    s_day = tm.tm_mday; /* <Quad> *//* npdc300050359 */
    s_hour = tm.tm_hour; /* <Quad> *//* npdc300050359 */
    s_min = tm.tm_min; /* <Quad> *//* npdc300050359 */
    s_sec = tm.tm_sec; /* <Quad> *//* npdc300050359 */
    s_msec = ts.tv_nsec / 1000000; /* <Quad> *//* npdc300050359 *//* nsec->msec */
#endif /* <Quad> *//* npdc300050359 */
/* Quad ADD end *//* npdc300050359 */
	/*end AD conversion*/
	temperature_adconv_flg = 0;

        /* schedule state off */
	omap_schedule_state = 0;
	
/* Quad MOD start */
//	cur_state = tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX]; /* <Quad> *//* npdc300056882 */
  	cur_state = 0; /* <Quad> *//* npdc300056882 */
#ifdef HARD_REQUEST_DEBUG /* HARD_REQUEST_DEBUG *//* npdc300056882 */
	printk("PTC: temperature_omap_check_workcb: cur_state: %u\n", cur_state); /* <Quad> *//* npdc300056882 */
#endif /* npdc300056882 */
/* Quad MOD end */
	
	if(buf == NULL)
	{
		/*timer set*/
		temperature_omap_timer.expires = jiffies 
			+ temperature_omap_hcmparam[cur_state].timer * HZ;
		temperature_omap_timer.data     = ( unsigned long )jiffies;
		temperature_omap_timer.function = temperature_omap_timer_handler;
		del_timer(&temperature_omap_timer);
		add_timer(&temperature_omap_timer);
		
		MODULE_TRACE_END();
		return D_TEMPDD_RET_NG;
	} 
	
	if(temperature_omap_main_sts == D_TEMPDD_TM_STS_WATCH){
		/*get AD value of sensor3(OMP_TMP) and sensor4(EMV_TMP)*/
		channel = D_TEMPDD_CNANNEL_S3;
         /* iD Power MOD start */
         tdev_sts.ad_val.ad_sensor3 =
         		(unsigned int)(buf[channel]);
         /* AD                                                         */
         if(tdev_sts.ad_val.ad_sensor3 > D_TEMPDD_MAX_ADVAL)
         {
             tdev_sts.ad_val.ad_sensor3 = D_TEMPDD_MAX_ADVAL;
         }
         /* iD Power MOD end */
		/*when temperature value calculates, subtract offset value*/
		/* from AD value.                                         */
		tdev_sts.temperature_val.t_sensor3 = temperature_adchange
					(tdev_sts.ad_val.ad_sensor3, channel);
#ifdef HARD_REQUEST_DEBUG_VTH /* <Quad> *//* npdc300050359 */
		printk(KERN_EMERG"TEMPERATURE SENSOR: %d-%02d-%02d %02d:%02d:%02d.%03lu VTH1 AD=0x%03x, val=%4d\n" /* <Quad> *//* npdc300050359 */
                ,s_year, s_month, s_day, s_hour, s_min, s_sec, s_msec /* <Quad> *//* npdc300050359 */
				,tdev_sts.ad_val.ad_sensor3 /* <Quad> *//* npdc300050359 */
				,tdev_sts.temperature_val.t_sensor3); /* <Quad> *//* npdc300050359 */
#endif /* <Quad> *//* npdc300050359 */
#ifdef HARD_REQUEST_DEBUG
		printk(KERN_EMERG"VTH1 AD=0x%03x,TEMPERATURE SENSOR=%4d\n"
				,tdev_sts.ad_val.ad_sensor3
				,tdev_sts.temperature_val.t_sensor3);
#endif	/* HARD_REQUEST_DEBUG */

/* Quad ADD start */
		vth1_temperature = tdev_sts.temperature_val.t_sensor3; /* <Quad> *//* npdc300056882 */
		tret = pm8921_batt_temperature(); /* <Quad> *//* PTC */
		if ( tret < 0 ) battery_temprature = 0; /* <Quad> *//* PTC */
		else battery_temprature = tret; /* <Quad> *//* PTC *//* 10*degree Celsius (xxx=10*xx.x) */
		ret = pfgc_fgc_mstate(D_FGC_CURRENT_REQ, &curr); /* <Quad> *//* PTC */
		if ( ret == 0 ) { /* <Quad> *//* PTC */
		    fgic_current = curr; /* <Quad> *//* PTC *//* -:discharge, +:charge */
		} /* <Quad> *//* PTC */
#ifdef HARD_REQUEST_DEBUG /* HARD_REQUEST_DEBUG *//* PTC *//* npdc300056882 */
		printk("PTC: temperature_omap_check_workcb: (tret: %d Bt: %d)(ret: %d Fc: %d)(VTH1: %d)\n", tret, battery_temprature, ret, fgic_current, vth1_temperature); /* <Quad> *//* PTC *//* npdc300056882 */
#endif /* PTC *//* npdc300056882 */
/* Quad ADD end */

/* iD Power DEL start */
#if 0
		channel = D_TEMPDD_CNANNEL_S4;
		if( buf[channel] > 0x3FF )
			buf[channel] = 0x3FF;
		tdev_sts.ad_val.ad_sensor4 =
					(unsigned char)(buf[channel] >> 2);
		/*when temperature value calculates, subtract offset value*/
		/*from AD value.                                          */
		tdev_sts.temperature_val.t_sensor4 = temperature_adchange
					(tdev_sts.ad_val.ad_sensor4,channel);
#ifdef HARD_REQUEST_DEBUG
		printk(KERN_EMERG"GPADCIN6 AD=0x%03x,TEMPERATURE SENSOR=%4d\n"
				,tdev_sts.ad_val.ad_sensor4
				,tdev_sts.temperature_val.t_sensor4);
#endif	/* HARD_REQUEST_DEBUG */
#endif
/* iD Power DEL end */
	    
		/*decide OMAP state*/
            /* iD Power MOD start */
            /*                      */
			/* new_state = temperature_omap_decide_state(
					tdev_sts.temperature_val.t_sensor3,
					tdev_sts.temperature_val.t_sensor4); */
			/* coverity 12400 start */
			ret = temperature_omap_decide_state(
					tdev_sts.temperature_val.t_sensor3,
					0);
/* Quad MOD start */
			ret = 0; /* <Quad> *//* npdc300056882 */
#ifdef HARD_REQUEST_DEBUG /* HARD_REQUEST_DEBUG *//* <Quad> *//* npdc300056882 */
			printk("PTC: temperature_omap_decide_state: ret(->new_state): %d\n", ret);  /* <Quad> *//* npdc300056882 */
#endif /* <Quad> *//* npdc300056882 */
/* Quad MOD end */
			if(ret < 0)
			{
				/*timer set*/
				temperature_omap_timer.expires = jiffies 
					+ temperature_omap_hcmparam[cur_state].timer * HZ;
				temperature_omap_timer.data     = ( unsigned long )jiffies;
				temperature_omap_timer.function = temperature_omap_timer_handler;
				del_timer(&temperature_omap_timer);
				add_timer(&temperature_omap_timer);
				
				MODULE_TRACE_END();
				return D_TEMPDD_RET_NG;
			}
			new_state = (unsigned int)ret;
            /* iD Power MOD end */
#ifdef HARD_REQUEST_DEBUG   /* HARD_REQUEST_DEBUG */
		printk(KERN_EMERG"omap_state = %u\n",new_state);
#endif	/* HARD_REQUEST_DEBUG */
		if(new_state != cur_state){
			tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX] = new_state;
			/*awake poll_wait*/
			/*wake_up_interruptible( &temperature_omap_wait_q );*/
			wakeup_flg_omap = 1;
			wake_up_interruptible( &tempwait_q );

#ifdef TEMPDD_DEBUG_TEST			
#ifdef CONFIG_CPU_FREQ_GOV_HOTPLUG
			/*notify cpufreq of omap state*//*debug*/
			cpufreq_frequency_opp_stop(tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX]);
#endif /* CONFIG_CPU_FREQ_GOV_HOTPLUG */
#endif /*TEMPDD_DEBUG_TEST*/			

			/*write log*/
			if(new_state > cur_state){
				temperature_write_log_flg = 2;
			} else if(new_state > D_TEMPDD_DEV_STS_NORMAL){
				temperature_write_log_flg = 1;
			} else{
                            timestamp_write_log_flg = 1; /* <Quad>  */
			}
		}

		/*timer set*/
		temperature_omap_timer.expires = jiffies 
			+ temperature_omap_hcmparam[new_state].timer * HZ;
		temperature_omap_timer.data     = ( unsigned long )jiffies;
		temperature_omap_timer.function = temperature_omap_timer_handler;
		del_timer(&temperature_omap_timer);
		add_timer(&temperature_omap_timer);
		
		/*<npdc300036023> start */
		/* IN Camera */
		in_cam_power_state = gpio_get_value(TEMPDD_LOCAL_MSM_GPIONO86); /* <Quad> *//* npdc300051869 *//* in_cam_power_state includes the states of in-camera and out-camera */
#ifdef HARD_REQUEST_DEBUG   /* HARD_REQUEST_DEBUG */
		printk(KERN_EMERG"in_cam_power_state=0x%x\n",in_cam_power_state);
#endif	/* HARD_REQUEST_DEBUG */
		/* OUT Camera */
//		out_cam_power_state = gpio_get_value(TEMPDD_LOCAL_MSM_GPIONO15); /* <Quad> *//* npdc300051869 *//* disuse */
#ifdef HARD_REQUEST_DEBUG   /* HARD_REQUEST_DEBUG */
		printk(KERN_EMERG"out_cam_power_state=0x%x\n",out_cam_power_state);
#endif	/* HARD_REQUEST_DEBUG */
		DBG_HEXMSG(" [in] check_dev_flg ->",check_dev_flg);
		if (((in_cam_power_state != 0) || (out_cam_power_state != 0)) &&
			(temperature_prev_cam_power_state == 0)) {
			temperature_prev_cam_power_state = 1;
			ROUTE_CHECK("!!!D_TEMPDD_IOCTL_CAMERA_ENABLE!!!");
			temperature_ioctl_start(D_M_CAMERA);
		}
		else if (((in_cam_power_state == 0) && (out_cam_power_state == 0)) &&
				 (temperature_prev_cam_power_state == 1)) {
			ROUTE_CHECK("!!!D_TEMPDD_IOCTL_CAMERA_DISABLE!!!");
			if( temperature_main_sts == D_TEMPDD_TM_STS_NORMAL)
			{
				ROUTE_CHECK("temperature DD: no action path \n"); 
			}
			else if(temperature_main_sts == D_TEMPDD_TM_STS_WATCH)
			{
				spin_lock_irqsave(&temp_spinlock,flags);
				if(check_dev_flg & M_CAMERA_MODE)
				{
					check_dev_flg &= ~D_M_CAMERA;
					spin_unlock_irqrestore(&temp_spinlock,flags);
					temperature_ioctl_end(temp_hcmparam.enable_time_camera);
				}
				else
				{
					spin_unlock_irqrestore(&temp_spinlock,flags);
					ROUTE_CHECK("temperature DD: no action path \n"); 
				}
			}
			else if(temperature_main_sts == D_TEMPDD_TM_STS_WAITTIMER)
			{
				/* no action path */
				ROUTE_CHECK("temperature DD: no action path \n"); 
			}
			else
			{
				/* no action path */
			}    
			temperature_prev_cam_power_state = 0;
		}
		else {
			/* no action path */
		}
		DBG_HEXMSG(" [out] check_dev_flg ->",check_dev_flg);
		/*<npdc300036023> end   */
	}	

	MODULE_TRACE_END();
	return D_TEMPDD_RET_OK;
}

static int temperature_write_log(unsigned int state, int cnt_enable)
{
	unsigned short totalnum = 0;
	static unsigned int buf_id = 0;
	unsigned char cpu_log[8];
	unsigned char temp_log1[8];
	unsigned char temp_log2[8];
	unsigned char type_log[8];
	unsigned char timer_log[32];
	unsigned long year;
	unsigned long month;
	time_t   day;
	time_t   hour;
	time_t   min;
	time_t   second;
	int ret = 0;
	unsigned int timer;
		
	/*The transition number of times*/
	ret = cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_CNT,
				2, (unsigned char *)&totalnum);
	if ((cnt_enable == 1) && (totalnum < 0xFFFF)) {
		totalnum++;
		ret |= cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_CNT,
				2, (unsigned char *)&totalnum);
	}
				
	/*sensor value*/
	ret |= cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_TEMP_LOG_S3,
					8, (unsigned char*)temp_log1);
	ret |= cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_TEMP_LOG_S4,
					8, (unsigned char*)temp_log2);
	temp_log1[buf_id] = tdev_sts.temperature_val.t_sensor3;
	temp_log2[buf_id] = tdev_sts.temperature_val.t_sensor4;
	ret |= cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_TEMP_LOG_S3, 8, temp_log1);
	ret |= cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_TEMP_LOG_S4, 8, temp_log2);
	
	/*state type*/
	cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_TYPE,8, (unsigned char*)type_log);
	type_log[buf_id] = state;
	cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_TYPE,8, type_log);
	
	/*CPU load*/
	ret |= cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_CPU_LOG,
					8, (unsigned char*)cpu_log);
	cpu_log[buf_id] = temperature_get_cpu_load();
	ret |= cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_CPU_LOG, 8, cpu_log);
	
	/*time stamp*/
	syserr_TimeStamp2( &year, &month, &day,
				&hour, &min, &second ); 

	DBG_MSG("mon  -> ",month);
	DBG_MSG("mday -> ",day);
	DBG_MSG("hour -> ",hour);             
	DBG_MSG("min  -> ",min);
	DBG_MSG("sec  -> ",second);
	timer =
		((((( month << 22) | 
		(( unsigned long ) day << 17 ))  |
		(( unsigned long ) hour << 12 )) |
		(( unsigned long ) min << 6 ))   |
		( unsigned long ) second ) ;
	ret |= cfgdrv_read(D_HCM_E_VP_OPP_BLOCK_TIMESTAMP,
					32, (unsigned char*)timer_log);
	memcpy(&(timer_log[buf_id*4]),(unsigned char *)&timer,4);
	DBG_MSG("timer_log  ->",timer);

	ret |= cfgdrv_write(D_HCM_E_VP_OPP_BLOCK_TIMESTAMP, 32, timer_log);
	
	if(ret != 0 )
		printk(KERN_EMERG"error : NVdata write\n");

	/*update ring buffer id*/
	buf_id++;
	if(buf_id % 8 == 0)
		buf_id = 0;

	return ret;
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time;
	u64 iowait_time;

	MODULE_TRACE_START();
	
	/* cpufreq-hotplug always assumes CONFIG_NO_HZ */
	idle_time = get_cpu_idle_time_us(cpu, wall);

	/* add time spent doing I/O to idle time */
	iowait_time = get_cpu_iowait_time_us(cpu, wall);
	/* cpufreq-hotplug always assumes CONFIG_NO_HZ */
	if (iowait_time != -1ULL && idle_time >= iowait_time)
		idle_time -= iowait_time;

	MODULE_TRACE_END();
	return idle_time;
}

/* jb_VerUp ADD START */
#define cputime64_sub(__a, __b)		((__a) - (__b))
/* jb_VerUp ADD END */

static unsigned int temperature_get_cpu_load(void)
{
	signed int i;
	unsigned int load;
	unsigned int total_load = 0;
	unsigned int idle_time, wall_time;
	cputime64_t cur_wall_time, cur_idle_time;
	struct cpu_dbs_info_s *j_dbs_info;
	unsigned int avg;
	int num_cpu;    /* iDPower ADD coverity 31905 */
	
	MODULE_TRACE_START();	
	for(i = 0; i < 2; i++) {
		j_dbs_info = &per_cpu(hp_cpu_dbs_info, i);

		/* update both cur_idle_time and cur_wall_time */
		cur_idle_time = get_cpu_idle_time(i, &cur_wall_time);

		/* how much wall time has passed since last iteration? */
		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		/* how much idle time has passed since last iteration? */
		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		/* load is the percentage of time not spent in idle */
		load = 100 * (wall_time - idle_time) / wall_time;

		/* keep track of combined load across all CPUs */
		total_load += load;
	}

	/* calculate the average load across all related CPUs */
	/* iDPower MOD start */
	/* coverity 31905 */
	num_cpu = (int)num_online_cpus();
	if(num_cpu > 0)
	{
		avg = (int)(total_load / num_cpu);
	}
	else
	{
		avg = 0;
	}
	/* iDPower MOD end */
	
	MODULE_TRACE_END();
	return avg;
}

static void temperature_set_millor_table( void )
{
	unsigned long i;    /* index */
	
	MODULE_TRACE_START();
	
	temperature_millor_max = 0;
	/* create millor table */	
	for( i = 0 ; i < D_TEMPDD_DEV_STS_NUM ; i++ )
	{
		/* if abs_cnt_th is 0xFF, this threshold unused */
		if(( temperature_omap_hcmparam[ i ].abs_cnt_th != 0xFF ) ||
		   ( temperature_omap_hcmparam[ i ].rel_cnt_th != 0xFF ))
		{
			temperature_millor_param[ temperature_millor_max ] 
					= &temperature_omap_hcmparam[ i ];
			temperature_millor_max++;
		}
	}
	
	MODULE_TRACE_END();
	return;
}

/**
 *  temperature_omap_check_workcb - callback function for AD conversion
 *  @date 2011.03.02
 *  @author NTTD-MSE T.higashikawa
 *  @omap_temp temperature of omap sensor
 *  @env_temp temperature of environment sensor
 *
 *  This function decides state of omap temperature detection from temperature
 *  of omap and environment sensors.
**/
static signed int temperature_omap_decide_state(signed short omap_temp,
						   signed short env_temp)
{
	signed int abs_temp, rel_temp;
	static unsigned int cur_state;
	unsigned int m_idx;
	unsigned int result;
	unsigned int new_state_abs, new_state_rel;
	unsigned int max_state;
	static int errmsg_count = 1;
	
	MODULE_TRACE_START();
	
	cur_state = tdev_sts.dev_sts[D_TEMPDD_OMAP_INDEX];
	abs_temp = omap_temp;
	rel_temp = omap_temp - env_temp;
	max_state = temperature_millor_param[temperature_millor_max - 1]->id;
	
	
	/*decide state by absolute temperature*/
	for(m_idx = 0; m_idx < temperature_millor_max; m_idx++){
		if(temperature_prev_state_abs
				== temperature_millor_param[ m_idx ]->id)
			break;
	}
	if(m_idx == temperature_millor_max){
		MODULE_TRACE_END();
		return D_TEMPDD_RET_NG;
	}

	if(temperature_prev_state_abs == D_TEMPDD_DEV_STS_NORMAL){
		/*don't decide omap state, when all threshold is unused*/
		if (temperature_millor_max == 1){
			if(errmsg_count == 1){
				printk(KERN_EMERG"unused temperature threshold\n");
				errmsg_count = 0;
			}
			MODULE_TRACE_END();
			return D_TEMPDD_DEV_STS_NORMAL;
		} else if(abs_temp >= temperature_millor_param
						[m_idx + 1]->abs_temp_th){
			new_state_abs = temperature_omap_disable( abs_temp,
						temperature_prev_state_abs,
						D_TEMPDD_ABSOLUTE );
		} else{
			new_state_abs = D_TEMPDD_DEV_STS_NORMAL;
			temperature_over_counter[D_TEMPDD_CNT_ABS_DIS] = 0;
			temperature_over_counter[D_TEMPDD_CNT_ABS_ENA] = 0;
		}
	} else if(temperature_prev_state_abs == max_state){
		if(abs_temp <= temperature_millor_param[m_idx]->abs_temp_th){
			new_state_abs = temperature_omap_enable(abs_temp,
						temperature_prev_state_abs,
						D_TEMPDD_ABSOLUTE);
		} else{
			new_state_abs = max_state;
			temperature_over_counter[D_TEMPDD_CNT_ABS_DIS] = 0;
			temperature_over_counter[D_TEMPDD_CNT_ABS_ENA] = 0;
		}
	} else{
		if(abs_temp >= temperature_millor_param
						[m_idx + 1]->abs_temp_th){
			new_state_abs = temperature_omap_disable(abs_temp,
						temperature_prev_state_abs,
						D_TEMPDD_ABSOLUTE);
		} else if(abs_temp <= temperature_millor_param
						[m_idx]->abs_temp_th){
			new_state_abs = temperature_omap_enable(abs_temp,
						temperature_prev_state_abs,
						D_TEMPDD_ABSOLUTE);
		} else{
			new_state_abs = temperature_millor_param[m_idx]->id;
			temperature_over_counter[D_TEMPDD_CNT_ABS_DIS] = 0;
			temperature_over_counter[D_TEMPDD_CNT_ABS_ENA] = 0;
		}
	}
	
	/*decide state only absolute temperature*/
    /* iD Power MOD start */
    /*                        */
	/* if(rel_temp < 0){ */
	{
    /* iD Power MOD end */
		/* printk(KERN_EMERG"decide omap state only "
				"absolute temperature\n"); */
		temperature_prev_state_abs = new_state_abs;
		temperature_over_counter[D_TEMPDD_CNT_REL_DIS] = 0;
		temperature_over_counter[D_TEMPDD_CNT_REL_ENA] = 0;
			
		MODULE_TRACE_END();
		return new_state_abs;
	}
	
	/*decide state by relative temperature*/
	for(m_idx = 0; m_idx < temperature_millor_max; m_idx++){
		if(temperature_prev_state_rel
				== temperature_millor_param[ m_idx ]->id)
			break;
	}
	if(m_idx == temperature_millor_max){
		MODULE_TRACE_END();
		return D_TEMPDD_RET_NG;
	}
	
	if(temperature_prev_state_rel == D_TEMPDD_DEV_STS_NORMAL){
		if(rel_temp >= temperature_millor_param
						[m_idx + 1]->rel_temp_th){
			new_state_rel = temperature_omap_disable( rel_temp,
						temperature_prev_state_rel,
						D_TEMPDD_RELATIVE );
		} else{
			new_state_rel = D_TEMPDD_DEV_STS_NORMAL;
			temperature_over_counter[D_TEMPDD_CNT_REL_DIS] = 0;
			temperature_over_counter[D_TEMPDD_CNT_REL_ENA] = 0;
		}
	} else if(temperature_prev_state_rel == max_state){
		if(rel_temp <= temperature_millor_param[m_idx]->rel_temp_th){
			new_state_rel = temperature_omap_enable(rel_temp,
						temperature_prev_state_rel,
						D_TEMPDD_RELATIVE);
		} else{
			new_state_rel = max_state;
			temperature_over_counter[D_TEMPDD_CNT_REL_DIS] = 0;
			temperature_over_counter[D_TEMPDD_CNT_REL_ENA] = 0;
		}
	} else{
		if(rel_temp >= temperature_millor_param
						[m_idx + 1]->rel_temp_th){
			new_state_rel = temperature_omap_disable(rel_temp,
						temperature_prev_state_rel,
						D_TEMPDD_RELATIVE);
		} else if(rel_temp <= temperature_millor_param
						[m_idx]->rel_temp_th){
			new_state_rel = temperature_omap_enable(rel_temp,
						temperature_prev_state_rel,
						D_TEMPDD_RELATIVE);
		} else{
			new_state_rel = temperature_millor_param[ m_idx ]->id;
			temperature_over_counter[D_TEMPDD_CNT_REL_DIS] = 0;
			temperature_over_counter[D_TEMPDD_CNT_REL_ENA] = 0;
		}
	}
	
	if(new_state_abs >= new_state_rel){
		result = new_state_abs;
	} else{
		result = new_state_rel;
	}
	
	temperature_prev_state_abs = new_state_abs;
	temperature_prev_state_rel = new_state_rel;
	MODULE_TRACE_END();
	
	return result; 
}

static signed int temperature_omap_disable(signed int temp,
					     unsigned int prev_state,
					     unsigned int type)
{
	signed long i;
	unsigned int tmp_th[D_TEMPDD_DEV_STS_NUM];
	unsigned int tmp_cnt[D_TEMPDD_DEV_STS_NUM];
	unsigned int tmp_id[D_TEMPDD_DEV_STS_NUM];
	unsigned int over_th;
	signed int result;
	unsigned int *millor_count[2];
	unsigned int idx;
	unsigned int ng_idx;
	
	MODULE_TRACE_START();
	
	over_th = 0;
	ng_idx  = 0;
	result = prev_state;
	for(idx = 0; idx < temperature_millor_max; idx++){
		if(temperature_millor_param[idx]->id == prev_state)
			break; 
	}
	
	/*printk("*** prev_state = %d, idx = %d\n", prev_state, idx);*/
	
	if( type == D_TEMPDD_ABSOLUTE ){
		for( i = 0; i < temperature_millor_max; i++ )
		{
			tmp_th[i] = temperature_millor_param[i]->abs_temp_th;
			tmp_cnt[i] = temperature_millor_param[i]->abs_cnt_th;
			tmp_id[i] = temperature_millor_param[i]->id;
		}
		millor_count[ 0 ] = &temperature_over_counter
							[D_TEMPDD_CNT_ABS_DIS];
		millor_count[ 1 ] = &temperature_over_counter
							[D_TEMPDD_CNT_ABS_ENA];
	} else if( type == D_TEMPDD_RELATIVE ){
		for( i = 0; i < temperature_millor_max; i++ )
		{
			tmp_th[i] = temperature_millor_param[i]->rel_temp_th;
			tmp_cnt[i] = temperature_millor_param[i]->rel_cnt_th;
			tmp_id[i] = temperature_millor_param[i]->id;
		}
		millor_count[ 0 ] = &temperature_over_counter
							[D_TEMPDD_CNT_REL_DIS];
		millor_count[ 1 ] = &temperature_over_counter
							[D_TEMPDD_CNT_REL_ENA];
	} else{
		/* iDPower ADD start */
		/* coverity 16561, 16560, 16559, 16558 */
		for( i = 0; i < temperature_millor_max; i++ )
		{
			tmp_th[i] = temperature_millor_param[i]->abs_temp_th;
			tmp_cnt[i] = temperature_millor_param[i]->abs_cnt_th;
			tmp_id[i] = temperature_millor_param[i]->id;
		}
		millor_count[ 0 ] = &temperature_over_counter
							[D_TEMPDD_CNT_ABS_DIS];
		millor_count[ 1 ] = &temperature_over_counter
							[D_TEMPDD_CNT_ABS_ENA];
		/* iDPower ADD end */
		;
	}
	
	/*compare temperature and threshold*/
	for( i = idx; i < temperature_millor_max - 1; i++)
	{
		if( tmp_cnt[i + 1] != 0xFF ){  /* skip? */
			if( temp >= tmp_th[i + 1] ){
				over_th++;
			} else{
				break;
			}
			if( over_th == 1){
				ng_idx = i;
			}
		}
	}
	
	if( over_th >= 1 ){
		(*millor_count[0])++;
		
		/*if((over_th >= 2) || ((*millor_count[0]) >= tmp_cnt[idx]))*/
		if((over_th >= 2) || ((*millor_count[0]) >= tmp_cnt[ng_idx + 1])){
			/*printk(KERN_EMERG"over_th = %u\n",over_th);*/
			result = tmp_id[ng_idx+1];
			*millor_count[ 0 ] = 0;
			*millor_count[ 1 ] = 0;
		} else{
			*millor_count[ 1 ] = 0;
		}
		
	} else{
		*millor_count[ 0 ] = 0;
		*millor_count[ 1 ] = 0;
	}
	
	/*printk("*** result(%d) = %d\n", type, result);*/
	
	MODULE_TRACE_END();
	return result;
}

static signed int temperature_omap_enable(signed int temp,
					     unsigned int prev_state,
					     unsigned int type)
{
	signed int i;
	unsigned int tmp_th[D_TEMPDD_DEV_STS_NUM];
	unsigned int tmp_cnt[D_TEMPDD_DEV_STS_NUM];
	unsigned int tmp_id[D_TEMPDD_DEV_STS_NUM];
	unsigned int over_th;
	signed int result;
	unsigned int *millor_count[2];
	unsigned int idx;
	unsigned int ng_idx;

	MODULE_TRACE_START();
	
	over_th = 0;
	ng_idx  = 0;
	result = prev_state;
	for(idx = 0; idx < temperature_millor_max; idx++){
		if(temperature_millor_param[idx]->id == prev_state)
			break; 
	}
	
	/*printk("*** prev_state = %d, idx = %d\n", prev_state, idx);*/
	
	if( type == D_TEMPDD_ABSOLUTE ){
		for( i = 0; i < temperature_millor_max; i++ )
		{
			tmp_th[i] = temperature_millor_param[i]->abs_temp_th;
			tmp_cnt[i] = temperature_millor_param[i]->abs_cnt_th;
			tmp_id[i] = temperature_millor_param[i]->id;
		}
		millor_count[ 0 ] = &temperature_over_counter
							[D_TEMPDD_CNT_ABS_ENA];
		millor_count[ 1 ] = &temperature_over_counter
							[D_TEMPDD_CNT_ABS_DIS];
	} else if( type == D_TEMPDD_RELATIVE ){
		for( i = 0; i < temperature_millor_max; i++ )
		{
			tmp_th[i] = temperature_millor_param[i]->rel_temp_th;
			tmp_cnt[i] = temperature_millor_param[i]->rel_cnt_th;
			tmp_id[i] = temperature_millor_param[i]->id;
		}
		millor_count[ 0 ] = &temperature_over_counter
							[D_TEMPDD_CNT_REL_ENA];
		millor_count[ 1 ] = &temperature_over_counter
							[D_TEMPDD_CNT_REL_DIS];
	} else{
		;
	}
	
	/*compare temperature and threshold*/
	for( i = idx; i > 0; i--)
	{
		if( tmp_cnt[i] != 0xFF ){
			if( temp < tmp_th[i] ){
				over_th++;
			} else{
				break;
			}
			if( over_th == 1){
				ng_idx = i;
			}
		}
	}
	
	if( over_th >= 1 ){
		(*millor_count[0])++;
		
		if((over_th >= 2) || ((*millor_count[0]) >= tmp_cnt[ng_idx])){
			result = tmp_id[ ng_idx - 1 ];
			*millor_count[ 0 ] = 0;
			*millor_count[ 1 ] = 0;
		} else{
			*millor_count[ 1 ] = 0;
		}
		
	} else{
		*millor_count[ 0 ] = 0;
		*millor_count[ 1 ] = 0;
	}
	
	/*printk("*** result(%d) = %d\n", type, result);*/
	
	MODULE_TRACE_END();
	return result;
}

