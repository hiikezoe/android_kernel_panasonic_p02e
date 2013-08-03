/* 
 * Battery Safety IC driver
 * Copyright(C) 2011 NTT-Data MSE Co., Ltd.
 * Written by M.M.
 * Based on spider2.h
 */

#ifndef _SAFETYIC_H
#define _SAFETYIC_H


#define SPIDER_EXOV     0x00	/* Overvoltage  */
#define SPIDER_NORMAL    0x80	/* normal */
#define SPIDER_OTHER   0x01	/* except overvoltage */
#define SPIDER_TEMP    0x02	/* Temp Error */

#define SPIDER_ERR_REG_NUM  6	/* number of error detection register */
#define SPIDER_TIMESAVE_MAX 8	/* number of saved error detection time */
#define SPIDER_REGSAVE_MAX          /* number of saved error detection register*/\
        ( SPIDER_TIMESAVE_MAX * SPIDER_ERR_REG_NUM )
#define SPIDER_COUNT_MAX 255	/* max number of saved error detection */

#define SPIDER_ERR1_OTHER  0x7C	/* other error */
#define SPIDER_ERR1_TEMP   0x03	/* VTH6 Temp error*/
#define SPIDER_ERR2_EXT    0x30	/* External over voltage */
#define SPIDER_ERR2_OTHER  0x4F	/* Other error  */

#define CONFIG_PROC_FS_DCTL

/* safety-ic int factor */
#define DSAFETY_FCT_VEXT          0x40  /* EXT voltage error      */ /* Quad add <npdc300053027> */
#define DSAFETY_FCT_THR           0x20  /* THR VB error           */
#define DSAFETY_FCT_TSOFT         0x10  /* temperature soft error */
#define DSAFETY_FCT_THARD         0x08  /* temperature hard error */
#define DSAFETY_FCT_CSOFT1        0x04  /* current soft error1    */
#define DSAFETY_FCT_CSOFT2        0x02  /* current soft error2    */
#define DSAFETY_FCT_CHARD         0x01  /* current hard error     */

/* safety-ic func state */
#define DSAFETY_FUNC_INTFACTOR    0x01  /* safety_ic int      */
#define DSAFETY_FUNC_FCTGET       0x02  /* fctget timer t.o   */
#define DSAFETY_FUNC_SHUTDOWN     0x04  /* shutdown timer t.o */

/* safety-ic register address */
#define DSAFETY_CURRENT_FACTOR1  0xF5
#define DSAFETY_CURRENT_FACTOR2  0xF6
#define DSAFETY_TEMP_FACTOR1     0xF7
#define DSAFETY_TEMP_FACTOR2     0xF8
#define DSAFETY_ETC_FACTOR       0xF9
#define DSAFETY_INT_FACTOR       0xFA
#define DSAFETY_CPUMODE          0xFB
#define DSAFETY_PROTECT          0xFC
#define DSAFETY_SHUTDOWN         0xFD

/* IDPower start */
#define DSAFETY_CURRENT_SET1     0xF0
#define DSAFETY_CURRENT_SET2     0xF1
#define DSAFETY_CURRENT_SET3     0xF2
#define DSAFETY_TEMP_SET1        0xF3
#define DSAFETY_TEMP_SET2        0xF4

#define DSAFETY_CURRENT_OUT        0
#define DSAFETY_TEMP_OUT           1
#define DSAFETY_ADC_MAX_CHANNELS   8

#define DSAFETY_CHANNEL_STA        0
#define DSAFETY_CHANNEL_SET_STA  (1 << 0)
#define DSAFETY_CHANNEL_S1         1
#define DSAFETY_CHANNEL_SET_S1   (1 << 1)
#define DSAFETY_CHANNEL_S2         2
#define DSAFETY_CHANNEL_SET_S2   (1 << 2)
#define DSAFETY_CHANNEL_S3         3
#define DSAFETY_CHANNEL_SET_S3   (1 << 3)
#define DSAFETY_CHANNEL_S4         4
#define DSAFETY_CHANNEL_SET_S4   (1 << 4)
#define DSAFETY_CHANNEL_S5         5
#define DSAFETY_CHANNEL_SET_S5   (1 << 5)
#define DSAFETY_CHANNEL_S6         6
#define DSAFETY_CHANNEL_SET_S6   (1 << 6)
#define DSAFETY_CHANNEL_S7         7
#define DSAFETY_CHANNEL_SET_S7   (1 << 7)
/* IDPower end */

extern int safety_ic_current_temp_adc(unsigned int type, unsigned int channel, int *buf); /* IDPower */

/* IDPower void set_charger_psy(struct power_supply *psy); */
struct SPIDER2_REG {
	u8 status_1;		/* 0xFC */
	u8 status_2;		/* 0xFD */
};

#define GPIO_SPIDER2 (24)

/* Jessie add start */
/* charge-status */
extern unsigned char safety_ic_charge_sts;

#define DSAFETY_CHG_NORMAL       0
#define DSAFETY_CHG_FORCE_STOP   1
/* Jessie add end */

#endif /* _SAFETYIC_H */
