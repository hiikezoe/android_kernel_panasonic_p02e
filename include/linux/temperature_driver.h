/*
*    temperature.h
* 
* Usage:    temperature sensor driver by i2c for linux
*
*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html
 *
 */

#ifndef __TEMPERATURE_DRIVER__  /*temperature_driver.h*/
#define __TEMPERATURE_DRIVER__

#define D_TEMPDD_FLG_OFF             0
#define D_TEMPDD_FLG_ON              1

#define D_TEMPDD_CLEAR               0

#define D_TEMPDD_RET_OK              0
#define D_TEMPDD_RET_NG             -1
#define D_TEMPDD_RET_PARAM_NG       -2
#define D_TEMPDD_RET_SYSCALL_NG     -3



/*temperature driver measurement status */
#define D_TEMPDD_TM_STS_NORMAL       0 /*temperature measurement no start status   */
#define D_TEMPDD_TM_STS_WATCH        1 /*temperature measurement start status      */
#define D_TEMPDD_TM_STS_WAITTIMER    2 /*temperature measurement watch timer status*/

/*device status */
#define D_TEMPDD_DEV_STS_NORMAL      0 /*device temperature normal status     */
#define D_TEMPDD_DEV_STS_ABNORMAL    1 /*device temperature illegal status    */
#define D_TEMPDD_DEV_STS_ABNORMAL_L2 2 /*device temperature illegal status (middle)*/
#define D_TEMPDD_DEV_STS_ABNORMAL_L3 3 /*device temperature illegal status (high)*/
#define D_TEMPDD_DEV_STS_NUM		4 /*number of device status*/

/*sensor status*/
#define D_TEMPDD_NORMAL_SENSOR       0 /*sensor temperature normal status     */
#define D_TEMPDD_ABNORMAL_SENSOR     1 /*sensor temperatue illegal status     */
#define D_TEMPDD_NORMALTOAB_SENSOR   2 /*normal -> illegal changes in status  */
#define D_TEMPDD_ABNORMALTONO_SENSOR 3 /*illegal-> normal changes in status   */

/*device type*/
#define D_TEMPDD_DEVNUM              7 /*device num                           *//* <Quad> */
#define D_TEMPDD_DEVNUM_LOCAL       11 /*device num for local                 *//* <Quad> */
#define D_TEMPDD_BATTERY             0 /*BATTERY                              */
#define D_TEMPDD_WLAN                1 /*WLAN                                 */
#define D_TEMPDD_DTV_BKL             2 /*DTV_BKL                              */
#define D_TEMPDD_CAMERA              3 /*CAMERA                               */
#define D_TEMPDD_BATTERY_AMR         4 /*BATTERY_AMR                          */
#define D_TEMPDD_OMAP                5 /*OMAP                                 */
/* Quad ADD start */
#define D_TEMPDD_BATTERY_DTV         6 /*BATTERY_DTV                          *//* <Quad> */
/* Quad ADD end */

#define D_TEMPDD_BATTERY_INDEX       0 /*BATTERY Index                        */
#define D_TEMPDD_WLAN_INDEX          1 /*WLAN Index                           */
#define D_TEMPDD_DTV_BKL_INDEX       2 /*DTV_BKL Index                        */
#define D_TEMPDD_CAMERA_INDEX        3 /*CAMERA Index                         */
#define D_TEMPDD_BATTERY_AMR_INDEX   4 /*BATTERY_AMR Index                    */
#define D_TEMPDD_BATTERY_WLAN_INDEX  5 /*BATTERY(WLAN mode) Index             */
#define D_TEMPDD_BATTERY_CAMERA_INDEX 6 /*BATTERY(CAMERA) Index                */
#define D_TEMPDD_CAMERA_WLAN_INDEX   7 /*CAMERA(WLAN mode) Index              */
/* Quad MOD/ADD start */
#define D_TEMPDD_BATTERY_DTV_INDEX    8 /*BATTERY_DTV Index                 *//* <Quad> *//* npdc300050675 */
#define D_TEMPDD_CAMERA_CAMERA_INDEX  9 /*CAMERA(CAMERA) Index              *//* <Quad> *//* npdc300050675 */
#define D_TEMPDD_OMAP_INDEX          10 /*OMAP Index                           *//* <Quad> *//* npdc300050675 */
/* Quad MOD/ADD end */

/*sensor type*/
/* iD Power MOD start */
/* 周囲温度は使用しないため削除 */
#define D_TEMPDD_SENSOR_NUM          4 /*sensor num                         *//* <Quad> */
#define D_TEMPDD_SENSOR1             0 /*sensor1(PA_TEMP)                   */
#define D_TEMPDD_SENSOR2             1 /*sensor2(WL_TEMP)                   */
#define D_TEMPDD_SENSOR3             2 /*sensor3(MSM_TEMP)                  */
#define D_TEMPDD_SENSOR4             3 /*sensor4(CAM_TEMP)                  *//* <Quad> */
/* iD Power MOD end */

/*sensor channel*/ 
/* iD Power MOD start */
/* Spider3に合わせてチャネルを修正 */
/* Quad MOD start */
//#define D_TEMPDD_CNANNEL_S1           4 /*channel of sensor1(VTH4, PA)*/
//#define D_TEMPDD_CNANNEL_SET_S1     (1 << 4)
//#define D_TEMPDD_CNANNEL_S2           7 /*channel of sensor2(VTH7, WLAN)*/
//#define D_TEMPDD_CNANNEL_SET_S2     (1 << 7)
#define D_TEMPDD_CNANNEL_S1           7 /*channel of sensor1(VTH7, 3G-PA)*//* <Quad> */
#define D_TEMPDD_CNANNEL_SET_S1     (1 << 7) /* <Quad> */
#define D_TEMPDD_CNANNEL_S1_2         6 /*channel of sensor1-2(VTH6, 2G-PA)*//* <Quad> */
#define D_TEMPDD_CNANNEL_SET_S1_2   (1 << 6) /* <Quad> */
#define D_TEMPDD_CNANNEL_S2           4 /*channel of sensor2(VTH4, WLAN)*//* <Quad> */
#define D_TEMPDD_CNANNEL_SET_S2     (1 << 4) /* <Quad> */
/* Quad MOD end */
#define D_TEMPDD_CNANNEL_S3           1 /*channel of sensor3(VTH1, MSM)*/
#define D_TEMPDD_CNANNEL_SET_S3     (1 << 1)
/* iD Power MOD end */
/* iD Power ADD start */
#define D_TEMPDD_MIN_ADVAL          0x00D   /*   1degree */
#define D_TEMPDD_MAX_ADVAL          0x792   /* 120degree */
#define D_TEMPDD_TYPICAL_VAL        0x1F5   /* 補正用Typical値 */
/* iD Power ADD end */

/*AD -> ℃ transform define*/
#define D_TEMPDD_MINTEMP             1 /*min                                  */
#define D_TEMPDD_MAXTEMP           120 /*max                                  */

/*temperature type*/
#define D_TEMPDD_ABSOLUTE		0
#define D_TEMPDD_RELATIVE		1

/*temperature counter type*/
#define D_TEMPDD_CNT_ABS_DIS		0
#define D_TEMPDD_CNT_REL_DIS		1
#define D_TEMPDD_CNT_ABS_ENA		2
#define D_TEMPDD_CNT_REL_ENA		3

/*AD conversion type*/
#define D_TEMPDD_ADCONV_OMAP		1
#define D_TEMPDD_ADCONV_3G		2

#define D_TEMPDD_MAX_STR 100

/******************************************************************************/
/* io_ctl define                                                              */
/******************************************************************************/
#define TEMPERATURE_IOC_MAGIC       'x'        
#define D_TEMPDD_IOCTL_OMAP_ENABLE  _IO( TEMPERATURE_IOC_MAGIC,  0)
					   			/*start(OMAP) */
#define D_TEMPDD_IOCTL_OMAP_DISABLE _IO( TEMPERATURE_IOC_MAGIC,  1)
								/*stop(OMAP)  */
#define D_TEMPDD_IOCTL_3G_ENABLE  _IO( TEMPERATURE_IOC_MAGIC,  2)
								/*start(3G)   */
#define D_TEMPDD_IOCTL_3G_DISABLE _IO( TEMPERATURE_IOC_MAGIC,  3)
								/*stop(3G)    */
#define D_TEMPDD_IOCTL_3G_P_ENABLE  _IO( TEMPERATURE_IOC_MAGIC,  4)
								/*start(3G_P) */
#define D_TEMPDD_IOCTL_3G_P_DISABLE _IO( TEMPERATURE_IOC_MAGIC,  5)
								/*stop(3G_P)  */
#define D_TEMPDD_IOCTL_WLAN_ENABLE  _IO( TEMPERATURE_IOC_MAGIC,  6)
								/*start(WLAN) */
#define D_TEMPDD_IOCTL_WLAN_DISABLE _IO( TEMPERATURE_IOC_MAGIC,  7)
								/*stop(WLAN)  */
#define D_TEMPDD_IOCTL_CAMERA_ENABLE  _IO( TEMPERATURE_IOC_MAGIC,  8)
								/*start(CAMERA)*/
#define D_TEMPDD_IOCTL_CAMERA_DISABLE _IO( TEMPERATURE_IOC_MAGIC,  9)
								/*stop(CAMERA)*/
/* Quad ADD start */
#define D_TEMPDD_IOCTL_DTV_ENABLE  _IO( TEMPERATURE_IOC_MAGIC, 10) /* <Quad> */
								/*start (DTV)*/                    /* <Quad> */
#define D_TEMPDD_IOCTL_DTV_DISABLE _IO( TEMPERATURE_IOC_MAGIC, 11) /* <Quad> */
								/*stop (DTV)*/                     /* <Quad> */
/* Quad ADD end */
/* Quad MOD start */
#define D_TEMPDD_IOCTL_GET     _IO( TEMPERATURE_IOC_MAGIC,  12) /*status get  */
/* Quad MOD end */

/******************************************************************************/
/* bit mask                                                                   */
/******************************************************************************/

#define M_BATTERY (1<<0)               /*battey mask                          */
#define M_WLAN    (1<<1)               /*wlan   mask                          */
#define M_DTV_BKL (1<<2)               /*DTV and BackLight mask               */
#define M_CAMERA  (1<<3)               /*camera mask                          */
/* Onji Add Start */
#define M_BATTERY_AMR  (1<<4)          /*battey_arm mask                      */
#define M_BATTERY_WLAN (1<<5)          /*battey(WLAN) mask                    */
#define M_BATTERY_CAMERA (1<<6)        /*battery(CAMERA) mask                      */
#define M_CAMERA_WLAN  (1<<7)          /*camera(WLAN) mask                    */
/* Quad MOD/ADD start */
#define M_BATTERY_DTV (1<<8)           /*battery(DTV) mask                    *//* <Quad> *//* npdc300050675 */
#define M_CAMERA_CAMERA   (1<<9)      /*camera(CAMERA) mask                  *//* <Quad> *//* npdc300050675 */
#define M_OMAP    (1<<10)               /*omap   mask                          *//* <Quad> *//* npdc300050675 */
/* Quad MOD/ADD end */

/* Quad MOD start */
#define M_3G_MODE   (1<<11)            /*3G mode mask                        *//* <Quad> */
#define M_3G_P_MODE (1<<12)            /*3G-packet mode mask                 *//* <Quad> */
#define M_WLAN_MODE (1<<13)            /*WLAN mode mask                      *//* <Quad> */
#define M_CAMERA_MODE  (1<<14)         /*camera mode mask                    *//* <Quad> */
/* Quad MOD End */
/* Quad ADD start */
#define M_DTV_MODE  (1<<15)            /*dtv mode mask                       *//* <Quad> */
/* Quad ADD end */

#define D_M_3G   (M_BATTERY + M_WLAN + M_DTV_BKL + M_CAMERA + M_BATTERY_AMR + M_3G_MODE) /* 3G mode     */
/* npdc300035875 start */
#define D_M_3G_P (M_BATTERY + M_WLAN + M_DTV_BKL + M_CAMERA + M_BATTERY_AMR + M_3G_P_MODE) /* 3G-packet mode */
//#define D_M_3G_P (M_BATTERY + M_WLAN + M_DTV_BKL + M_CAMERA + M_3G_P_MODE)               /* 3G-packet mode */
/* npdc300035875 end */
/* Quad MOD start */
#define D_M_WLAN (M_BATTERY_WLAN + M_CAMERA_WLAN + M_BATTERY_AMR + M_WLAN_MODE)         /* WLAN mode *//* <Quad> */
#define D_M_CAMERA (M_BATTERY_CAMERA + M_CAMERA_CAMERA + M_CAMERA_MODE)          /* camera mode *//* <Quad> */
/* Quad MOD End */
/* Quad ADD start */
#define D_M_DTV (M_BATTERY_DTV + M_DTV_MODE) /* DTV mode *//* <Quad> */
/* Quad ADD end */
/* Onji Add End */
/******************************************************************************/
/* table                                                                      */
/******************************************************************************/
#define D_TEMPDD_OMAP_MINOR	0	/*OMAP                                */
#define D_TEMPDD_3G_MINOR	1	/*3G                                  */
#define D_TEMPDD_MINOR_NUM_MAX	2	/*Max number of MINOR                 */

/*AD*/
struct adsensor_t { 
	int	ad_sensor1;		/*sensor1                     */
	int	ad_sensor2;		/*sensor2                     */
	int	ad_sensor3;		/*sensor3                     */
	int	ad_sensor4;		/*sensor4                     */
};


/*temperature*/
struct tempsensor_t {
	int	t_sensor1;		/*sensor1                           */
	int	t_sensor2;		/*sensor2                           */
	int	t_sensor3;		/*sensor3                           */
	int	t_sensor4;		/*sensor4                           */
};                                  

/*device status table */
struct dev_state_local_t {
                                        /*device status backup table          */
                        /* 0:battery 1:wlan 2:omap 3:camera 4:battery_arm */ 
                        /* 5:battery(wlan) 6:omap(wlan) 7:camera(wlan)     */
    char                      dev_sts[D_TEMPDD_DEVNUM_LOCAL];
    struct tempsensor_t      temperature_val;/*sensor temperature             */
    struct adsensor_t         ad_val;        /*sensor AD                      */
};

struct dev_state_t {
                                        /*device status backup table          */
                        /* 0:battery 1:wlan 2:omap 3:camera 4:battery_arm */ 
    char                      dev_sts[D_TEMPDD_DEVNUM];
    struct tempsensor_t      temperature_val;/*sensor temperature             */
    struct adsensor_t         ad_val;        /*sensor AD                      */
};

/* sensor status backup table */
struct sensor_state_t {             /*  0:battery 1:wlan 2:omap 3:camera     */
    char    sensor_sts_old[D_TEMPDD_DEVNUM_LOCAL];   
                                     /* previous sensor status                */
    char    sensor_sts_new[D_TEMPDD_DEVNUM_LOCAL]; 
                                     /* new sensor status                     */
    char    changing_num[D_TEMPDD_DEVNUM_LOCAL]; 
                                     /* status change counter                 */
};

/*threshold table     */
struct hcmsensor_param{
    int disable_temp[D_TEMPDD_SENSOR_NUM]; /*sensor 1-3 disable temperature   */
    int disable_time[D_TEMPDD_SENSOR_NUM]; /*sensor 1-3 disable temperature   */
    int enable_temp[D_TEMPDD_SENSOR_NUM];  /*sensor 1-3 enable  temperature   */
    int enable_time[D_TEMPDD_SENSOR_NUM];  /*sensor 1-3 enable  temperature   */
};    
struct hcmparam {
    int vp_heat_suppress;
    int vp_period;
    int enable_time_3g;
    int enable_time_3g_p;
    int enable_time_wlan;
    int enable_time_camera;
    int enable_time_dtv; /* <Quad> */
    struct hcmsensor_param dev_hcmparam[D_TEMPDD_DEVNUM_LOCAL];
    unsigned int factory_adj_val;   /* iD Power ADD */
};    

struct hcmsensor_omap_param {
	unsigned int id;			/* table id                   */
	unsigned int timer;	/* period timer */
	signed short abs_temp_th;
				/* threshold for absolute temperature */
	signed short rel_temp_th;
				/* threshold for relation temperature */
	unsigned char abs_cnt_th;
				/* detection count for absolute temperature */
	unsigned char rel_cnt_th;
				/* detection count for relation temperature */
	unsigned short reserve; /*reserved member*/
};

typedef struct{
    unsigned int ad_val;
    int temp;
}_D_TEMPDD_ADCHANGE_TABLE;

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	int cpu;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};

/******************************************************************************/
/* external define                                                            */
/******************************************************************************/
int temperature_getdevsts(int device,int * devsts);
int temperature_gpadcsuspend(int state);

#endif /*temperature_driver.h*/
