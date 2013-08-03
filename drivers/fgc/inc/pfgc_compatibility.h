/*
 * drivers/fgc/inc/pfgc_compatibility.h
 *
 * Copyright(C) 2010 Panasonic Mobile Communications Co., Ltd.
 * Author: Naoto Suzuki <suzuki.naoto@kk.jp.panasonic.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _pfgc_compatibility_h                    /* _pfgc_compatibility_h     */
#define _pfgc_compatibility_h                    /* _pfgc_compatibility_h     */

/** define **/
/* Device node */
#define DNODES_FGC            (217)              /*                           */
#define DNODES_FGC_FGCINFO    (0)                /* /dev/fg/info              */
#define DNODES_FGC_FGCDF      (1)                /* /dev/fg/df                */
#define DNODES_FGC_FGCVER     (2)                /* /dev/fg/ver               */
#define DNODES_FGC_FGCFW      (3)                /* /dev/fg/fw                */
#define DNODES_FGC_FGCDFS     (4)                /* /dev/fg/dfs               */

/* Device Control */
#define DCTL_OK               (0)                /*                           */
#define DCTL_BAT_PAR_FULL     (0x00)             /* 100%                      */
#define DCTL_BAT_MIN_FULL     (0x0000)           /* 1000min                   */
#define DCTL_OFF              (0)                /* OFF                       */
#define DCTL_ON               (0)                /* ON                        */
#define DCTL_BLVL_ALM         (0)                /*                           */
#define DCTL_BLVL_MIN         (0)                /*       1                   */
#define DCTL_BLVL_LVL2        (0)                /*       2                   */
#define DCTL_BLVL_LVL3        (0)                /*       3                   */
#define DCTL_STATE_CALL_2G    (0x00000000)       /* 2G                        */
#define DCTL_STATE_CALL_IRAT  (0x00000001)       /* 2G  3GHandover            */
#define DCTL_STATE_TEST_2G    (0x00000002)       /* 2G                        */
#define DCTL_STATE_CALL       (0x00000003)       /*                           */
#define DCTL_STATE_TEST2_2G   (0x00000004)       /* 2G                        */
#define DCTL_STATE_TV_CALL    (0x00000005)       /*                           */
#define DCTL_STATE_TEST2      (0x00000006)       /*                           */
#define DCTL_STATE_INTMIT_2G  (0x00000007)       /* 2G                        */
#define DCTL_STATE_TEST       (0x00000008)       /*                           */
#define DCTL_STATE_POWON      (0x00000009)       /*     ON                    */
#define DCTL_STATE_INTMIT     (0x00000010)       /*                           */
#define DCTL_STATE_POWOFF     (0x00000011)       /*     OFF                   */

/* Battery Status */
#define BAT_NO_CHARGE         (0x00)             /* normal battery operation  */
#define BAT_CHARGEFIN2        (0x13)             /* Charge complete 2         */
#define BAT_ST_INT            (0x00)             /*                           */
#define BAT_ST_EXTWB          (0x01)             /*         (        )        */

/* I2C Access */
#define DCOM_SEQUENTIAL_ADDR  (0x00000001)       /* I2C                       */
#define DCOM_ACCESS_MODE1     (0x00000010)       /*               1           */
#define DCOM_ACCESS_MODE2     (0x00000020)       /*               2           */
#define DCOM_ACCESS_MODE_MSK  (0x00000030)       /*               MSK         */
#define DCOM_ERRLOG_NOWRITE   (0x00000000)       /*                           */
#define DCOM_OK               (0)                /*                           */
#define DCOM_RATE400K         (400)              /* I2C          400kbps      */
#define DCOM_RATE100K         (100)              /* I2C          100kbps      */
#define DCOM_DEV_DEVICE4      (0)                /* FG-IC                     */
#define DCOM_IOCTL_RATE       (1)                /* I2C                       */
#define DCOM_I2C_NAI_NG       (-EREMOTEIO)       /*                           */
#define DCOM_ERR_READ5        (DCOM_I2C_NAI_NG)  /* READ ERR5                 */

/* System error */
#define CSYSERR_SYSINF_LEN    (32)               /*                           */
#define CSYSERR_LOGINF_LEN    (32)               /*                           */
#define CSYSERR_ALM_RANK_A    (0x01)             /*               A           */
#define CSYSERR_ALM_RANK_B    (0x02)             /*               B           */
#define	MSD_FGC               (0x0000)

/* TimeStamp */
#define MONTH_OF_YEAR         (12)
#define LEAPMONTH             (29)
#define NORMALMONTH           (28)
#define FEBRUARY              (1)

/* Memory Access */
#define ioremap_cache(cookie,size,cache_type) \
        ioremap((cookie),(size))

/** typedef **/
typedef unsigned long TCOM_FUNC;                 /*                           */
typedef unsigned long TCOM_ID;                   /*                 ID        */
typedef void          TCOM_OPEN_OPTION;          /*                           */

/** structure **/
typedef struct T_dctl_batinfo                    /*                           */
{
    unsigned short  adcnv_var;                   /*                           */
    unsigned short  rest_time;                   /* MTF                       */
    unsigned char   rfbatt_class_info;           /* RF                        */
    unsigned char   rest_batt;                   /* MTF              (%)      */
    unsigned char   level;                       /*                           */
    unsigned char   det1;                        /*     DET1                  */
    unsigned char   capacity;                    /*         (%)               */
    unsigned char   dummy[ 3 ];                  /* 4                         */
} t_dctl_batinfo;

typedef struct tcom_rw_data                      /*                           */
{
    unsigned long  *offset;                      /*                           */
    void           *data;                        /*                           */
    unsigned long   number;                      /*                           */
    unsigned long   size;                        /*                           */
    void           *option;                      /*                           */
}TCOM_RW_DATA;

struct t_fgc_eeprom_tbl {
	unsigned short type;                     /* Access ID                 */
	unsigned short data_size;                /* dataSize                  */
};

/** enum **/
enum ioremap_mode {
     IOR_MODE_UNCACHED,
     IOR_MODE_CACHED,
     CACHE_WBB
};

/** table **/
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>extern struct t_fgc_eeprom_tbl const EEPDATA[18];*/
extern struct t_fgc_eeprom_tbl const EEPDATA[27];/*<Conan>*/
#else /*<Conan>1111XX*/
/*Jessie del extern struct t_fgc_eeprom_tbl const EEPDATA[18]; */
extern struct t_fgc_eeprom_tbl const EEPDATA[23];/* Jessie add */
#endif /*<Conan>1111XX*/

/** function declaration **/
extern TCOM_FUNC   pcom_acc_write( TCOM_ID        id,
                                   unsigned long  dev,
                                   TCOM_RW_DATA  *data );

extern TCOM_FUNC   pcom_acc_read(  TCOM_ID        id,
                                   unsigned long  dev,
                                   TCOM_RW_DATA  *data );

extern TCOM_FUNC   pcom_acc_open( unsigned long     dev,
                                  TCOM_ID          *id,
                                  TCOM_OPEN_OPTION *option );

extern TCOM_FUNC   pcom_acc_close( TCOM_ID id );

extern TCOM_FUNC   pcom_acc_ioctl( TCOM_ID        id,
                                   unsigned long  dev,
                                   unsigned long  req,
                                   void          *data );

extern int Hcm_romdata_read_knl( unsigned short  info_id,
                                 unsigned short  info_no,
                                 unsigned char  *data_adr );

extern int Hcm_romdata_write_knl( unsigned short  info_id,
                                  unsigned short  info_no,
                                  unsigned char  *data_adr );

extern int pdctl_mstate_batget( t_dctl_batinfo *dctl_batinfo );

extern void pdctl_powfac_bat_stat_set( unsigned char stat );

extern void ms_alarm_proc( unsigned char  err_rank,
                           unsigned long  err_no,
                           void          *sys_data,
                           size_t         err_sys_size,
                           void          *log_data,
                           size_t         err_log_size );

extern void syserr_TimeStamp( unsigned long *year,
                              unsigned long *month,
                              time_t        *day,
                              time_t        *hour,
                              time_t        *min,
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>                              time_t        *sec );*/
                              time_t        *sec,   /*<Conan>*/
                              time_t        *usec );/*<Conan>*/
extern void monotonic_TimeStamp( unsigned long *day,/*<Conan>*/
                              time_t        *hour,  /*<Conan>*/
                              time_t        *min,   /*<Conan>*/
                              time_t        *sec,   /*<Conan>*/   
                              time_t        *usec );/*<Conan>*/
#else /*<Conan>1111XX*/
                              time_t        *sec );
#endif /*<Conan>1111XX*/

#endif                                           /* _pfgc_compatibility_h     */

