/*
 * drivers/fgc/inc/pfgc_local.h
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

#ifndef _pfgc_local_h                                /* _pfgc_local_h         */
#define _pfgc_local_h                                /* _pfgc_local_h         */

#define DFGC_SW_DELETED_FUNC       (0)               /* delete SW             */
#define DFGC_SW_TEST_DEBUG         (0)               /* Debug Log for Test SW */

/*----------------------------------------------------------------------------*/
/* include                                                                    */
/*----------------------------------------------------------------------------*/

#include <asm/io.h>
/*<QAIO029>#include <linux/config.h>                                          */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include "linux/rh-fgc.h"                        /*                           */
#include <linux/spinlock.h>                      /* spinlock                  */
#include <linux/interrupt.h>                     /*<6130964>                  */
#include <linux/vmalloc.h>                       /*<QAIO027> vmalloc          */

#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
#include <linux/timer.h>                         /*<QCIO017>                  */
#include <asm/irq.h>                             /*<QCIO017> IRQ              */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */
#include <linux/syscalls.h>                      /*<4210014>                  */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mtdwrap.h>
#include <linux/kthread.h>
#include <linux/cfgdrv.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/charger.h>         /* IDPower:npdc300020757,npdc300029449 */
#include <linux/completion.h>
#include <linux/platform_device.h> /* IDPower */
#include <asm/atomic.h>            /* npdc300035452 */

#include "pfgc_adrd_local.h"

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/*                MFGC_SYSERR_PARAM_SET                                       */
/*                SYS                                                         */
/*                2008.09.30                                                  */
/*                MSE                                                         */
/*                                                                            */
/*                                                                            */
/*                unsigned char        kind                                   */
/*                unsigned long        fno                                    */
/*                unsigned long        path                                   */
/*                4byte                data1                                  */
/*                T_FGC_SYSERR_DATA    err_data  SYS                          */
/******************************************************************************/
#define MFGC_SYSERR_DATA_SET( k, fno, path, data1, err_data ) /*             */\
{                                                /*                          */\
    unsigned long __path;                        /*                          */\
    unsigned long __data1;                       /*                          */\
                                                 /*                          */\
    __path  = ( path );                          /*                          */\
    __data1 = ( data1 );                         /*                          */\
                                                 /*                          */\
    ( err_data ).kind = ( k );                   /*                          */\
    ( err_data ).func_no = ( fno );              /*                          */\
                                                 /*                          */\
    memset(( void* )&( err_data ).data[ 0 ],     /*   S   V                  */\
           0,                                    /*                          */\
           CSYSERR_SYSINF_LEN ) ;                /*                          */\
                                                 /*                          */\
    memcpy(( void* )&( err_data ).data[ 0 ],     /*   S   V                  */\
           ( void* )&__path,                     /*                          */\
           sizeof( __path ));                    /*                          */\
                                                 /*                          */\
    memcpy(( void* )&( err_data ).data[ 4 ],     /*   S   V                  */\
           ( void* )&__data1,                    /*                          */\
           sizeof( __data1 ));                   /*                          */\
                                                 /*                          */\
    FGC_ERR( "SYSERR : kind = 0x%02x, func_no = 0x%08x, path = 0x%08lx, data1 = 0x%08lx\n",\
            ( k ), ( fno ), __path, __data1 );                                 \
}                                                /*                          */

/******************************************************************************/
/*                MFGC_SYSERR_PARAM_SET2                                      */
/*                SYS                                                         */
/*                2008.09.30                                                  */
/*                MSE                                                         */
/*                                                                            */
/*                                                                            */
/*                unsigned char        kind                                   */
/*                unsigned long        fno                                    */
/*                unsigned long        path                                   */
/*                4byte                data1                                  */
/*                4byte                data2                                  */
/*                T_FGC_SYSERR_DATA    err_data  SYS                          */
/******************************************************************************/
#define MFGC_SYSERR_DATA_SET2( k, fno, path, data1, data2, err_data ) /*     */\
{                                                /*                          */\
    unsigned long __path;                        /*                          */\
    unsigned long __data1;                       /*                          */\
    unsigned long __data2;                       /*                          */\
                                                 /*                          */\
    __path  = ( path );                          /*                          */\
    __data1 = ( data1 );                         /*                          */\
    __data2 = ( data2 );                         /*                          */\
                                                 /*                          */\
    ( err_data ).kind  = ( k );                  /*                          */\
    ( err_data ).func_no  = ( fno );             /*                          */\
                                                 /*                          */\
    memset(( void* )( &( err_data ).data[ 0 ] ), /*   S   V                  */\
           0,                                    /*                          */\
           CSYSERR_SYSINF_LEN );                 /*                          */\
                                                 /*                          */\
    memcpy(( void* )&( err_data ).data[ 0 ],     /*   S   V                  */\
           ( void* )&__path,                     /*                          */\
           sizeof( __path ));                    /*                          */\
                                                 /*                          */\
    memcpy(( void* )( &( err_data ).data[ 4 ] ), /*   S   V                  */\
           ( void* )&__data1,                    /*                          */\
           sizeof( __data1 ));                   /*                          */\
                                                 /*                          */\
    memcpy(( void* )( &( err_data ).data[ 8 ] ), /*   S   V                  */\
           ( void* )&__data2,                    /*                          */\
           sizeof( __data2 ));                   /*                          */\
                                                                               \
    FGC_ERR( "SYSERR : kind = 0x%02x, func_no = 0x%08x, path = 0x%08lx, data1 = 0x%08lx, data2 = 0x%08lx\n",\
            ( k ), ( fno ), __path, __data1, __data2 );                        \
}                                                /*                          */

#ifdef CONFIG_PROC_FS_FGC_PATH                   /*                           */
#define MFGC_RDPROC_PATH( data )                 /*                          */\
{                                                /*                          */\
    if( gfgc_path_debug.path_cnt >= DFGC_RDPROC_DPATH_MAX ) /*               */\
    {                                            /*                          */\
        gfgc_path_debug.path_cnt = DFGC_CLEAR;   /*                          */\
    }                                            /*                          */\
                                                 /*                          */\
    gfgc_path_debug.path[ gfgc_path_debug.path_cnt ] = /*                    */\
                     ( unsigned long )( data ); /*                          */\
                                                 /*                          */\
    ( gfgc_path_debug.path_cnt++ );              /*                          */\
}                                                /*                           */
#else /* CONFIG_PROC_FS_FGC_PATH */              /*                           */
#define MFGC_RDPROC_PATH( data )                 /* proc                      */
#endif /* CONFIG_PROC_FS_FGC_PATH */             /*         read proc         */

#ifdef CONFIG_PROC_FS_FGC                        /*                           */
#define MFGC_RDPROC_ERROR( data )                /*                          */\
{                                                /*                          */\
    if( gfgc_path_error.path_cnt >= DFGC_RDPROC_EPATH_MAX ) /*               */\
    {                                            /*                          */\
        gfgc_path_error.path_cnt = DFGC_CLEAR;   /*                          */\
    }                                            /*                          */\
                                                 /*                          */\
    gfgc_path_error.path[ gfgc_path_error.path_cnt ] = /*                    */\
                     ( unsigned long )( data ); /*                          */\
                                                 /*                          */\
    ( gfgc_path_error.path_cnt++ );              /*                          */\
}                                                /*                           */
#else /* CONFIG_PROC_FS_FGC */                   /*                           */
#define MFGC_RDPROC_ERROR( data )                /* proc                      */
#endif /* CONFIG_PROC_FS_FGC */                  /*         read proc         */

/*                                                                            */
#define MFGC_FGRW_DTSET( mfgc_comacc_rwdt, mfgc_offset, mfgc_data )             \
{                                                                               \
    ( mfgc_comacc_rwdt.offset ) = (( unsigned long * )mfgc_offset ); /*                  */\
    ( mfgc_comacc_rwdt.data )  = (( void * )mfgc_data ); /*                   */\
    ( mfgc_comacc_rwdt.number ) = ( sizeof( mfgc_data )); /*                  */\
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*----- bit mask  ------------------------------------------------------------*/
#define DFGC_BITMASK_0         0x00000001        /*<QCIO017>                  */
#define DFGC_BITMASK_1         0x00000002        /*<QCIO017>                  */
#define DFGC_BITMASK_2         0x00000004        /*<QCIO017>                  */
#define DFGC_BITMASK_3         0x00000008        /*<QCIO017>                  */
#define DFGC_BITMASK_4         0x00000010        /*<QCIO017>                  */
#define DFGC_BITMASK_5         0x00000020        /*<QCIO017>                  */
#define DFGC_BITMASK_6         0x00000040        /*<QCIO017>                  */
#define DFGC_BITMASK_7         0x00000080        /*<QCIO017>                  */
#define DFGC_BITMASK_8         0x00000100        /*<QCIO017>                  */
#define DFGC_BITMASK_9         0x00000200        /*<QCIO017>                  */
#define DFGC_BITMASK_A         0x00000400        /*<QCIO017>                  */
#define DFGC_BITMASK_B         0x00000800        /*<QCIO017>                  */
#define DFGC_BITMASK_C         0x00001000        /*<QCIO017>                  */
#define DFGC_BITMASK_D         0x00002000        /*<QCIO017>                  */
#define DFGC_BITMASK_E         0x00004000        /*<QCIO017>                  */
#define DFGC_BITMASK_F         0x00008000        /*<QCIO017>                  */
#define DFGC_BITMASK_1BIT      0x00000001        /*<QCIO017>                  */
#define DFGC_BITMASK_4BIT      0x0000000F        /*<QCIO017>                  */
#define DFGC_BITMASK_8BIT      0x000000FF        /*<QCIO017>                  */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----- major number  --------------------------------------------------------*/

#define DFGC_MAJOR           DNODES_FGC           /*         DD               */
#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
#define DFGC_IBP_MAJOR       DNODES_IBP           /*<QCIO017> IBP             */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */

/*----- minor number  --------------------------------------------------------*/

#define DFGC_MINOR_NUM_MAX   5                    /*                          */

#define DFGC_INFO_MINOR      DNODES_FGC_FGCINFO   /* /dev/fg/info             */
#define DFGC_DF_MINOR        DNODES_FGC_FGCDF     /* /dev/fg/df               */
#define DFGC_VER_MINOR       DNODES_FGC_FGCVER    /* /dev/fg/ver              */
#define DFGC_FW_MINOR        DNODES_FGC_FGCFW     /* /dev/fg/fw               */
#define DFGC_DFS_MINOR       DNODES_FGC_FGCDFS    /* /dev/fg/dfs              */

#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
#define DFGC_IBP_MINOR_MAX    2                   /*<QCIO017> IBPマイナー最大 */
#define DFGC_IBP_MINOR_NORMAL DNODES_IBP_CHECK1   /*<QCIO017> /dev/ibp/check1 */
#define DFGC_IBP_MINOR_TEST   DNODES_IBP_CHECK2   /*<QCIO017> /dev/ibp/check2 */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */

/*----- logical device name --------------------------------------------------*/

#define DFGC_DEV             "fgc-dd"            /* device control for MAW    */
#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
#define DIBP_DEV             "ibp-dd"            /*<QCIO017>                  */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----- FG-IC                             ------------------------------------*/
#define DFGC_IC_FLAGS_SYSDOWN  0x02              /* SYSDOWN                   */

/*----- FGC-DD           -----------------------------------------------------*/
#define DFGC_LVL_2_INIT        0x14              /*                           */
#define DFGC_LVL_1_INIT        0x0A              /*                           */
#define DFGC_LVA_3G_INIT       0x01              /* LVA      3G               */
#define DFGC_LVA_2G_INIT       0x03              /* LVA      2G               */
#define DFGC_DUMP_ENABLE_INIT  0x00              /*                           */
#define DFGC_DUMP_ENABLE_CLASS 0x01              /* RF Class                  */
#define DFGC_CHARGE_TIMER_INIT 0x02              /* FG-IC                     */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#define DFGC_SOH_N_INIT        0x01              *//*                           */
#define DFGC_SOH_N_INIT        0x05              /*                           *//*<Conan>*/
#else /*<Conan>1111XX*/
#define DFGC_SOH_N_INIT        0x01              /*                           */
#endif /*<Conan>1111XX*/
#define DFGC_SOH_INIT_INIT     0x64              /*                           */
#define DFGC_SOH_LEVEL_INIT    0x03              /*                           */
#define DFGC_SOH_LEVEL_MAX     0x03              /*                     MAX   */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#define DFGC_SOH_DOWN_INIT     0x46              *//*                           */
#define DFGC_SOH_DOWN_INIT     0x50              /*                           *//*<Conan>*/
/*<Conan>#define DFGC_SOH_BAD_INIT      0x2D              *//*                           */
#define DFGC_SOH_BAD_INIT      0x41              /*                           *//*<Conan>*/
#else /*<Conan>1111XX*/
#define DFGC_SOH_DOWN_INIT     0x46              /*                           */
#define DFGC_SOH_BAD_INIT      0x2D              /*                           */
#endif /*<Conan>1111XX*/
#define DFGC_SOH_TTE_INIT      65535             /*                           */
#define DFGC_SOH_TTF_INIT      65535             /*                           */
#define DFGC_SOH_CAPACITY_INIT 0x0320            /*               (mA)        */
#define DFGC_SOH_VOLT_INIT     0x101F            /*                4127mV     */
#define DFGC_SOH_TEMP_INIT     0x0BA1            /*            24.55          */
#define DFGC_AI_INIT           0xFFD1            /* 1s              -51mA     */
#define DFGC_CAPACITY_FIX_DIS  0x00              /*         UI                */
#define DFGC_CAPACITY_FIX_ENA  0x01              /*         UI                */
#define DFGC_CAPACITY_MAX      100               /*                           */
#define DFGC_CAPACITY_80       80                /*         80%               */
#define DFGC_CAPACITY_60       60                /*         60%               */
#define DFGC_CAPACITY_40       40                /*         40%               */
#define DFGC_CAPACITY_20       20                /*         20%               */
#define DFGC_CAPACITY_10       10                /*         10%               */
#define DFGC_CAPACITY_LVA      (15)              /* LVA Threshold             */
#define DFGC_CAPACITY_MIN      0                 /*                           */
#define DFGC_DEV1_VOLT_INIT    0x0350            /*                           */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#define DFGC_SOH_BAD_INIT      0x2D             *//*                           */
#define DFGC_BATT_CAPA_DESIGN_INIT 0x47E         /*                  1150mAh  *//*<Conan>1112XX*/
#define DFGC_SOH_CYCLE_VAL_INIT 0x1031           /*                  4145(x10^-4mAH)    *//*<Conan>1112XX*/
#define DFGC_SOC_CYCLE_VAL_INIT 0x46             /*                           SOC       *//*<Conan>*/
#define DFGC_SOH_VAL_INIT      0                 /*                           *//*<Conan>*/
#define DFGC_SOC_MAX_CYCLE     0xFFFF            /*                           *//*<Conan>*/
#else /*<Conan>1111XX*/
#define DFGC_SOH_BAD_INIT      0x2D             /*                           */
#endif /*<Conan>1111XX*/
#define DFGC_INIT_COMPLETE     0x80              /* Control(Low bit7:Init_Complete) *//*<npdc300054685>*/

#define DFGC_FGC_COM_WAIT_CNT  100               /*                 openMAX   */
#define DFGC_FGC_COM_WAIT_TIMEOUT ( 100*HZ/1000 )/* WAIT    (100msec)         */

#define DFGC_FGC_SEQ_WRITE_MAX  DFGC_ROW_SIZE_INST /* Sequential Write MaxSize*/

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
#define DFGC_FGC_10_TO_7_POWER  10000000          /* 10^7                      *//*<Conan>*/
#define DFGC_FGC_10_TO_6_POWER   1000000          /* 10^6                      *//*<Conan>*/
#define DFGC_FGC_10_TO_3_POWER      1000          /* 10^3                      *//*<Conan>1112XX*/
#define DFGC_FGC_10_TO_1_POWER        10          /* 10^1                      *//*<Conan>*/
#define DFGC_FGC_CNV_PERCENT         100          /* %                         *//*<Conan>*/
#define DFGC_FGC_MAX_PERCENT    (DFGC_FGC_CNV_PERCENT * DFGC_FGC_10_TO_7_POWER) /*              *//*<Conan>*/
#define DFGC_FGC_ALLOWABLE_ERROR 10               /*                     (10%) *//*<Conan>*/
#define DFGC_FGC_SAVE_THRES ( 50 * DFGC_FGC_10_TO_7_POWER )/*                       (50(%)10^7  )*//*<Conan>*/
#define DFGC_FGC_ROMDATA_WRITE_1 0x1              /*                                  *//*<Conan>*/
#define DFGC_FGC_ROMDATA_WRITE_2 0x2              /*                                  *//*<Conan>*/
#define DFGC_FGC_SAVE_TIME       1440             /*                 ((  )24          *//*<Conan>*/
#define DFGC_FGC_SOC_MAX_PERCENT     100          /* SOC      (%)                     *//*<Conan>*/
#define DFGC_SOH_DEGRA_VAL_TBL00 0x0035 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL01 0x0055 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL02 0x0079 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL10 0x0058 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL11 0x0088 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL12 0x00B7 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL20 0x00C0 /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL21 0x00EC /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL22 0x011B /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL30 0x014E /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL31 0x01DC /*                      *//*<Conan>1112XX*/
#define DFGC_SOH_DEGRA_VAL_TBL32 0x026A /*                      *//*<Conan>1112XX*/
#endif /*<Conan>1111XX*/


/*-----            -----------------------------------------------------------*/
#define DFGC_TYPE_INIT         0x00              /*                           */
#define DFGC_TYPE_FG_INT       0x01              /* FG-IC                     */
#define DFGC_TYPE_CHARGE       0x02              /*                           */
#define DFGC_TYPE_RF_CLASS     0x03              /* RF Class                  */

/*-----          -------------------------------------------------------------*/
#define DFGC_RDPROC_DPATH_MAX  100               /*           MAX             */
#define DFGC_RDPROC_EPATH_MAX  10                /*               MAX         */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
#define DFGC_MIN_SHIFT         6                 /*             bit           */
#define DFGC_HOUR_SHIFT        12                /*             bit           */
#define DFGC_DAY_SHIFT         17                /*             bit           */
#define DFGC_MONTH_SHIFT       22                /*             bit           */
#define DFGC_YEAR_SHIFT        26                /*             bit           */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
#define DFGC_SEC_SHIFT         20                /*             bit           *//*<Conan>*/
#define DFGC_TIME_OFFSET       DFGC_SEC_SHIFT    /* usec                      *//*<Conan>*/

#define DFGC_USEC_BIT          0x000FFFFF        /* usec                bit   *//*<Conan>*/
#define DFGC_SEC_BIT           0x0000003F        /*                 bit       *//*<Conan>*/
#define DFGC_MIN_BIT           0x0000003F        /*                 bit       *//*<Conan>*/
#define DFGC_HOUR_BIT          0x0000001F        /*                 bit       *//*<Conan>*/
#define DFGC_DAY_BIT           0x001FFFFF        /*                 bit       *//*<Conan>*/

#define DFGC_MAX_DAY           0x16E00           /*                           *//*<Conan>*/
#endif /*<Conan>1111XX*/

#define DFGC_WORSE_DOWN        0                 /* Helth            Index    */
#define DFGC_WORSE_BAD         1                 /* Helth        Index        */
#define DFGC_WORSE_MAX         2                 /* Helth    index            */

#define DFGC_OCV_SAVE_MAX      3                 /* OCV                       */
#define DFGC_OCV_SAVE_CNT_MAX  0xFFFF            /* OCV            MAX        */

#define DFGC_LOG_CHG_MAX       3                 /*                     MAX   */
#define DFGC_LOG_CHG_CNT_MAX   0xFFFF            /*               MAX         */

/*<QAIO028>#define DFGC_LOG_SOC_MAX       3    *//*                     MAX   */
#define DFGC_LOG_SOC_MAX       7                 /*<QAIO028>                  */
#define DFGC_LOG_SOC_CNT_MAX   0xFFFF            /*                     MAX   */

/*<3131028>#define DFGC_LOG_SOH_MAX       3                 *//*                     MAX   */

#define DFGC_LOG_AI_DEC        (1)
#define DFGC_LOG_AI_HEX        (2)

#define PFGC_I2C_MASTER_SEND	0x0100
#define PFGC_I2C_MASTER_RECV	0x0200
#define PFGC_I2C_WRITE		0x0300
#define PFGC_I2C_READ		0x0400
#define PFGC_I2C_SLEEP_SET	0x0500
#define PFGC_I2C_GET_CONTROLREG	0x0600
#define PCOM_ACC_WRITE		0x0700
#define PCOM_ACC_READ		0x0800
#define PFGC_I2C_SUSPEND	0x0900
#define PFGC_I2C_RESUME		0x0A00
#define PFGC_FGC_AIGET		0x0B00
#define PFGC_FGC_REGGET         0x0C00
#define PFGC_FGIC_COMWRITE      0x0D00
#define PFGC_FGIC_COMREAD       0x0E00
#define PFGC_INFO_UPDATE        0x0F00
#define PFGC_FGC_FCCGET		0x1000 /* IDPower */

/*-----                           -----------------<4210014>------------------*/
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#define DFGC_LOG_FILE_MAX      20*/                /*<4210014>                  */
#define DFGC_LOG_FILE_MAX      99                /*<4210014>                  *//*<Conan>*/
#else /*<Conan>1111XX*/
#define DFGC_LOG_FILE_MAX      20                /*<4210014>                  */
#endif /*<Conan>1111XX*/
#define DFGC_LOG_CNT_OFFSET    DFGC_LOG_FILE_MAX /*<4210014>     FILE  OFFSET */
#define DFGC_LOG_FILE_OUT1   ( 150 + 1 )         /*<4210014> 150    (WP      )*/
#define DFGC_LOG_FILE_OUT2   ( 300 + 1 )         /*<4210014> 300    (WP      )*/
#define DFGC_LOG_FILE_OUT3   ( 450 + 1 )         /*<4210014> 450    (WP      )*/
#define DFGC_LOG_FILE_OUT4   ( 0 )               /*<4210014> 674    (WP      )*/

/*-----       ECO     -----------------------------<QEIO022>------------------*/
#define DFGC_LOG_ECO_MAX       8                 /*<QEIO022>                  */
#define DFGC_LOG_ECO_LOG_MAX   0xFFFF            /*<QEIO022>                  */

/*----- SYS           --------------------------------------------------------*/
#define DFGC_SYSERR_MSTATE_NULL       (( MSD_FGC << 16 ) | 0x0001 ) /* mstate                    */
#define DFGC_SYSERR_COM_OPEN_ERR      (( MSD_FGC << 16 ) | 0x0002 ) /*                 open      */
#define DFGC_SYSERR_COM_IOCTL_ERR     (( MSD_FGC << 16 ) | 0x0003 ) /*                 read      */
#define DFGC_SYSERR_COM_READ_ERR      (( MSD_FGC << 16 ) | 0x0004 ) /*                 read      */
#define DFGC_SYSERR_IOREMAP_ERR       (( MSD_FGC << 16 ) | 0x0005 ) /* LOG    ioremap            */
#define DFGC_SYSERR_BATGET_ERR        (( MSD_FGC << 16 ) | 0x0006 ) /* pdctl_mstate_batget             */
#define DFGC_SYSERR_BATTLEVEL_ERR     (( MSD_FGC << 16 ) | 0x0007 ) /*                           */
#define DFGC_SYSERR_NVM_ERR           (( MSD_FGC << 16 ) | 0x0008 ) /*                           */
#define DFGC_SYSERR_PARAM_ERR         (( MSD_FGC << 16 ) | 0x0009 ) /*                 <PCIO034> */
#define DFGC_SYSERR_INTERNAL_ERR      (( MSD_FGC << 16 ) | 0x000A ) /*                 <PCIO034> */
#define DFGC_SYSERR_CLOCK_ERR         (( MSD_FGC << 16 ) | 0x000B ) /*             ERR <QCIO017> */
#define DFGC_SYSERR_KERNEL_ERR        (( MSD_FGC << 16 ) | 0x000C ) /*             ERR <QCIO017> */
#define DFGC_SYSERR_UART_ERR          (( MSD_FGC << 16 ) | 0x000D ) /* UART            <QCIO017> */

#define MFGC_RDPROC_PATH_MAX   100
#define DFGC_FGIC_DUMP_MAX     1000              /* FuelGauge-ICダンプ数      */

#define DFGC_INIT              0x00110000
#define DFGC_OPEN              0x00120000
#define DFGC_IOCTL             0x00130000
#define DFGC_READ              0x00140000
#define DFGC_WRITE             0x00150000
#define DFGC_POLL              0x00160000
#define DFGC_RELEASE           0x00170000
#define DFGC_EXIT              0x00180000
#define DFGC_READ_PROC         0x00190000
#define DFGC_DF_GET            0x01110000
#define DFGC_DF_SET            0x01120000
#define DFGC_DF_STUDY_CHECK    0x01130000
#define DFGC_FGIC_DFREAD       0x01140000
#define DFGC_FGIC_DFWRITE      0x01150000
#define DFGC_FW_WRITE          0x01210000
#define DFGC_FW_UPDATE         0x01220000
#define DFGC_FW_FLACCSMODE_SET 0x01230000
#define DFGC_FW_FLACCSMODE_REV 0x01240000
#define DFGC_FGIC_MDCHK        0x01250000
#define DFGC_FGIC_MDCHG        0x01260000
#define DFGC_FW_ROWWRITE       0x01270000
#define DFGC_FGIC_COMREAD      0x01280000
#define DFGC_FGIC_COMWRITE     0x01290000
#define DFGC_FW_ROWREAD        0x012A0000
#define DFGC_FGIC_COMOPEN      0x012B0000        /*<PCIO034>                  */
#define DFGC_DFS_READ          0x01310000
#define DFGC_DFS_WRITE         0x01320000
#define DFGC_VER_INIT          0x01410000
#define DFGC_VER_READ          0x01420000
#define DFGC_INFO_INIT         0x01510000
#define DFGC_INFO_UPDATE       0x01520000
#define DFGC_INFO_HEALTH_INIT  0x01530000
#define DFGC_INFO_HEALTH_CHECK 0x01540000
#define DFGC_INFO_BATT_LEVEL   0x01550000
#define DFGC_FGC_MSTATE        0x01610000
#define DFGC_FGC_REGGET        0x01620000
#define DFGC_FGC_NVM_GET       0x01630000
#define DFGC_FGC_INT           0x01640000
#define DFGC_FGC_OCV_REQUEST   0x01650000
#define DFGC_FGC_KTHREAD       0x01660000        /*<1905359>                  */
#define DFGC_LOG_SYSERR        0x01710000
#define DFGC_LOG_DUMP_INIT     0x01720000
#define DFGC_LOG_DUMP          0x01730000
#define DFGC_LOG_OCV_INIT      0x01740000
#define DFGC_LOG_OCV           0x01750000
#define DFGC_LOG_HEALTH        0x01760000
#define DFGC_LOG_CHARGE        0x01770000
#define DFGC_LOG_CAPACITY      0x01780000
#define DFGC_LOG_TIME          0x01790000
#define DFGC_LOG_ECO           0x017A0000        /*<QEIO022>                  */
#define DFGC_LOG_DUMP_FILE     0x017B0000        /*<4210014>                  */
#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
#define DFGC_IBP_INIT          0x02110000        /*<QCIO017>                  */
#define DFGC_IBP_SET_TIMER     0x02120000        /*<QCIO017>                  */
#define DFGC_IBP_UART_INIT     0x02130000        /*<QCIO017>                  */
#define DFGC_IBP_UART_EXIT     0x02140000        /*<QCIO017>                  */
#define DFGC_IBP_SEND_CMD      0x02150000        /*<QCIO017>                  */
#define DFGC_IBP_INTERRUPT     0x02160000        /*<QCIO017>                  */
#define DFGC_IBP_RECV_INTERRUPT    0x02170000    /*<QCIO017>                  */
#define DFGC_IBP_PLL_TIMEOUT   0x02180000        /*<QCIO017>                  */
#define DFGC_IBP_RECV_TIMEOUT  0x02190000        /*<QCIO017>                  */
#define DFGC_IBP_TIMEOUT       0x021A0000        /*<QCIO017>                  */
#define DFGC_IBP_CHECK_READ    0x021B0000        /*<QCIO017>                  */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
                                                 /*       (TEMP*0.1) - 273.15 */
                                                 /* TEMP  (      273.15) * 10 */
                                                 /* 5            TEMP  35     */
/*<3200086>#define DFGC_HELTE_TEMP_LOW    0x0ADE                              *//*                     5.05  */
/*<3200086>#define DFGC_HELTE_TEMP_HIGH   0x0C09                              *//*                    34.95  */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
#define DFGC_HELTE_TEMP_LOW    0x0ADE            /*                     5.05  *//*<Conan>*/
#define DFGC_HELTE_TEMP_HIGH   0x0C09            /*                    34.95  *//*<Conan>*/
#endif /*<Conan>1111XX*/
#define DFGC_MASK_GPIOB4_DINT  0x0010            /*<3210076> GPIOBDINT bit4   */
#define DFGC_MASK_GPIOB4_CTRL  0x0400            /*<3210076> GPIOB4CNT bit10  */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
#define DFGC_HELTE_DEGRA_TBL_VOL_MAX  3          /*              TBL      (V)   *//*<Conan>*/
#define DFGC_HELTE_DEGRA_TBL_TEMP_MAX 4          /*              TBL      (TEMP)*//*<Conan>*/
#define DFGC_HELTE_TEMP_LEVEL_0     0x0B0f       /*                     9.95  *//*<Conan>*/
#define DFGC_HELTE_TEMP_LEVEL_1     0x0BD7       /*                    29.95  *//*<Conan>*/
#define DFGC_HELTE_TEMP_LEVEL_2     0x0C9F       /*                    49.95  *//*<Conan>*/
#define DFGC_HELTE_VOLT_LEVEL_0     0x0EB0       /*                     3760mV*//*<Conan>*/
#define DFGC_HELTE_VOLT_LEVEL_1     0x0FA0       /*                     4000mV*//*<Conan>*/
#define DFGC_HELTE_VOLT_LEVEL_2     0x10FE       /*                     4350mV*//*<Conan>1112XX*/
#define DFGC_HELTE_TBL_LEVEL_0        0          /*                         0 *//*<Conan>*/
#define DFGC_HELTE_TBL_LEVEL_1        1          /*                         1 *//*<Conan>*/
#define DFGC_HELTE_TBL_LEVEL_2        2          /*                         2 *//*<Conan>*/
#define DFGC_HELTE_TBL_LEVEL_3        3          /*                         3 *//*<Conan>*/
#endif /*<Conan>1111XX*/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
#define DFGC_VERUP_DF          0x05               /*             (DF    )     */
#define DFGC_VERUP_INSTDF      0x55               /*             (INST+DF)    */
#define DFGC_VERUP_NONE        0x00               /*             (INST+DF)    */
#define DFGC_MASK_DFVER_MODEL  0xFFFF0000         /*             (INST+DF)    */
#define DFGC_MASK_DFVER_VER    0x0000FFFF         /*             (INST+DF)    */

/*----------------------------------------------------------------------------*/
/* DF  Data Flash                                                             */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* DFS  Data Flash                                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* F/W                                                                        */
/*----------------------------------------------------------------------------*/
#define DFGC_UNSEAL_RTRYCNT        2              /* Seal                     */
#define DFGC_FLACCS_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_INSTERS_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_DFERS_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_ROWWRT_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_ERSWHOLE_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_ERSDF_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_ERSONEP_RTRYCNT        2              /* FullAccess            cnt*/
#define DFGC_I2CSPDSET_RTRYCNT      2             /*<3210046>I2C              */
#define DFGC_I2CSPDSET_RTRYCNT2     2             /*<3210046>I2C              */
#define DFGC_WHOLE_RTRYCNT          4             /*<3210043>Inst             */
#define DFGC_DFWHOLE_RTRYCNT        2             /*<3210043>DF               */
#define DFGC_2ROWWRT_RTRYCNT        2             /*<3210043>2ROW             */

#define DFGC_ROW_SIZE_INST     96
#define DFGC_INST_ROW_MAX      512
#define DFGC_ROW_SIZE_DF       32
#define DFGC_DF_ROW_MAX        32
#define DFGC_INTG_POS          15

#define DFGC_UPDSTS_NML        0x11               /* Update Status "Normal"   */
#define DFGC_UPDSTS_UPD        0xAA               /* Update Status "Update"   */

#define DFGC_WRTREQ_DF         0x0A               /*             (DF)         */
#define DFGC_WRTREQ_INST       0x02               /*             (INST)       */

#define DFGC_ROWWRTCMD_DF      0x0A               /* ROW                (DF)  */
#define DFGC_ROWWRTCMD_INST    0x02               /* ROW                (INST)*/

#define DFGC_WAIT_ERS_INST     400                /* Instruction Erase Wait   */
#define DFGC_WAIT_ERS_ONEP     20                 /* OnePage Erase Wait       */
#define DFGC_WAIT_ERS_DF       200                /* DataFlash Erase Wait     */
#define DFGC_WAIT_MODE_ROM     20                 /* ROM            Wait      */
#define DFGC_WAIT_MODE_NML     2000               /* Normal            Wait   */

#define DFGC_WAIT_RESET_CMD    100                /*<3210044> RESETCmd  Wait  */
#define DFGC_WAIT_ITENA_CMD    2                  /*<3210044> ITEnable  Wait  */

#define DFGC_ROWWRITE_WAIT_INST 2                /* Instruction RowWrite Wait*/
#define DFGC_ROWWRITE_WAIT_DF   2                /* DataFlash RowWrite Wait  */
#define DFGC_ROWWRITE_WAIT_INST_2ROW 20          /*<3210044>Top2RowWrite Wait */

#define DFGC_PEKBTCMD_WAIT     20                 /*<3210046> PeekByte  Wait  */
#define DFGC_POKBTCMD_WAIT     20                 /*<3210046> PokeByte  Wait  */

#define DFGC_WAIT_SEALED       (2)                /* Seal Cmd Wait            */

#define DFGC_WAIT_WRITE_USEC   (500)              /* Normal I2C Write wait    */
#define DFGC_WAIT_STP_STT_USEC (100)              /* Normal I2C Stop-Start wait */
#define DFGC_WAIT_ROM_USEC     (0)                /* Rom I2C Access No wait   */

#define DFGC_MASK_SEALSTS      0x20
#define DFGC_MASK_FLACCSSTS    0x40
#define DFGC_MASK_ITENABLE     0x01

#define DFGC_I2C_SPEED_400K    0x0B              /*<3210046>I2C        reg    */

#define D_FGC_NAND_BLOCK       DMEM_NAND_FGC_FGICFW_BLOCK                       /*<QAIO027>              */
#define D_FGC_NAND_SECTOR_NUM  ( DMEM_NAND_FGC_FGICFW_SIZE * SECTOR_PER_BLOCK ) /*<QAIO027> NAND         */

#define DFGC_THREAD_PRIORITY  0                   /*<1905359>                 */
#define DFGC_THREAD_NICE     -3                   /*<1905359>         NICE    */
#define DFGC_THREAD_POLICY    SCHED_NORMAL        /*<1905359>                 */
#define DFGC_THREAD_NAME      "FGC_thread"        /*<1905359>                 */
                                                  /*<1905359> (    15    +\0) */

/*----------------------------------------------------------------------------*/
/* IBP                                                                        */
/*----------------------------------------------------------------------------*/
#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */

#define DFGC_IBP_CHECK_MAX         3             /*<QCIO017>                  */
#define DFGC_IBP_CHECK_RECV_MAX    5             /*<QCIO017>                  */
#define DFGC_IBP_SEND_MAX         15             /*<QCIO017> IBP              */
#define DFGC_IBP_RECV_MAX        108             /*<QCIO017> IBP              */
                                                 /*<QCIO017>                  */
/*                                             *//*<QCIO017>                  */
#define DFGC_IBP_DATA_NONE      0x00             /*<QCIO017>                  */
#define DFGC_IBP_SEND_INT       0x2              /*<QCIO017>                  */
#define DFGC_IBP_RECV_INT       0x4              /*<QCIO017>                  */
#define DFGC_IBP_INT_MASK       0x00000000       /*<QCIO017>                  */
#define DFGC_IBP_INT_SEND       0x00000002       /*<QCIO017>                  */
#define DFGC_IBP_INT_RECV       0x00000005       /*<QCIO017>                  */
                                                 /*<QCIO017> (+            )  */
#define DFGC_IBP_FIFO_RESET     0x00000006       /*<QCIO017>       FIFO RESET */
#define DFGC_IBP_LINE_CONTROL   0x00000B00       /*<QCIO017> UA3_LCR          */
#define DFGC_IBP_FIFO_CONTROL   0x00000007       /*<QCIO017> UA3_CHAR         */
#define DFGC_IBP_9600BPS        0x00000060       /*<QCIO017> 9600bps          */
#define DFGC_IBP_UA3LSR_TDRE    0x00000020       /*<QCIO017> UA3-LSR bit5     */
#define DFGC_IBP_GPIOI7_PLOFF_OUT  0x00001000    /*<QCIO017> Pullup-OFF,      */
#define DFGC_IBP_GPIOI6_PLOFF_DIS  0x00000002    /*<QCIO017> Pullup-OFF,      */
#define DFGC_IBP_GPIOI7_PLOFF_DIS  0x00000002    /*<QCIO017> Pullup-OFF,      */
#define DFGC_IBP_GPIOI6_PLON_IN    0x00001016    /*<QCIO017> Pullup-ON,       */
#define DFGC_IBP_GPIOI7_PLON_DIS   0x00000016    /*<QCIO017> Pullup-ON,       */
#define DFGC_IBP_GPIOI6_PLON_DIS   0x00000016    /*<QCIO017> Pullup-ON,       */
                                                 /*<QCIO017>                  */
/* IRQ                                         *//*<QCIO017>                  */
#ifdef DFGC_LINUX_24                             /*<QCIO017>                  */
#define DFGC_IRQ_TYPE           void             /*<QCIO017> irq              */
#define DFGC_IRQ_RET            ;                /*<QCIO017> irq              */
#else /* DFGC_LINUX_24 */                        /*<QCIO017>                  */
#define DFGC_IRQ_TYPE           irqreturn_t      /*<QCIO017> irq              */
#define DFGC_IRQ_RET            IRQ_HANDLED      /*<QCIO017> irq              */
#endif /* DFGC_LINUX_24 */                       /*<QCIO017>                  */
                                                 /*<QCIO017>                  */
/*                                             *//*<QCIO017>                  */
#define DFGC_IBP_PLL_WAIT     17                 /*<QCIO017> PLL        [ms]  */
#define DFGC_IBP_DONE_WAIT   220                 /*<QCIO017>             [ms] */
#define DFGC_IBP_RECV_WAIT     2                 /*<QCIO017>             [ms] */
                                                 /*<QCIO017>                  */
/*                                             *//*<QCIO017>                  */
#define DFGC_IBP_INIT_MODE     0                 /*<QCIO017>                  */
#define DFGC_IBP_SEND_MODE     1                 /*<QCIO017>                  */
#define DFGC_IBP_RECV_MODE     2                 /*<QCIO017>                  */
                                                 /*<QCIO017>                  */
/*                                             *//*<QCIO017>                  */
#define DFGC_IBP_NORMAL_END    0x00000001        /*<QCIO017> IBP              */
#define DFGC_IBP_ERROR_END     0x00000002        /*<QCIO017> IBP              */
#define DFGC_IBP_CRITICAL_ERR  0x00000003        /*<QCIO017> IBP              */
                                                 /*<QCIO017>                  */
/*                                             *//*<QCIO017>                  */
#define DFGC_IBP_CMD_ACTIVE    0x00000000        /*<QCIO017> Active           */
#define DFGC_IBP_CMD_TEMP      0x00000001        /*<QCIO017>                  */
#define DFGC_IBP_CMD_VOLT      0x00000002        /*<QCIO017>                  */
#define DFGC_IBP_CMD_CUR_I     0x00000003        /*<QCIO017>                  */
#define DFGC_IBP_CMD_AVE_I     0x00000004        /*<QCIO017>                  */
#define DFGC_IBP_CMD_REL_STS   0x00000005        /*<QCIO017>                  */
#define DFGC_IBP_CMD_ABS_STS   0x00000006        /*<QCIO017>                  */
#define DFGC_IBP_CMD_REMAIN    0x00000007        /*<QCIO017>                  */
#define DFGC_IBP_CMD_FULL      0x00000008        /*<QCIO017>                  */
#define DFGC_IBP_CMD_RUN_TIME  0x00000009        /*<QCIO017>                  */
#define DFGC_IBP_CMD_AVE_EMP   0x0000000A        /*<QCIO017>         (    )   */
#define DFGC_IBP_CMD_AVE_FULL  0x0000000B        /*<QCIO017>                  */
#define DFGC_IBP_CMD_STATUS    0x0000000C        /*<QCIO017>                  */
#define DFGC_IBP_CMD_CYCLE     0x0000000D        /*<QCIO017>                  */
#define DFGC_IBP_CMD_DESIGN_C  0x0000000E        /*<QCIO017>                  */
#define DFGC_IBP_CMD_DESIGN_V  0x0000000F        /*<QCIO017>                  */
                                                 /*<QCIO017>                  */
#define DFGC_IBP_CMD_PACK_INFO 0x00000010        /*<QCIO017>                  */
#define DFGC_IBP_CMD_BP_INFO   0x00000011        /*<QCIO017>                  */
#define DFGC_IBP_CMD_ADD_INFO  0x00000012        /*<QCIO017>                  */
#define DFGC_IBP_CMD_NAME      0x00000013        /*<QCIO017>                  */
#define DFGC_IBP_CMD_ID        0x00000016        /*<QCIO017> ID               */
#define DFGC_IBP_CMD_CS_ALL    0x00000017        /*<QCIO017> CS    (    )     */
#define DFGC_IBP_CMD_STOP      0x00000018        /*<QCIO017>                  */
#define DFGC_IBP_CMD_LOWPOW    0x00000019        /*<QCIO017>                  */
#define DFGC_IBP_CMD_TEST      0x0000001A        /*<QCIO017>                  */
#define DFGC_IBP_CMD_ERR       0x0000001E        /*<QCIO017>                  */
#define DFGC_IBP_CMD_REV       0x0000001F        /*<QCIO017>                  */
                                                 /*<QCIO017>                  */
#define DFGC_IBP_CMD_RATE_EMP  0x00000020        /*<QCIO017>                  */
#define DFGC_IBP_CMD_RATE_FULL 0x00000021        /*<QCIO017>                  */
#define DFGC_IBP_CMD_CS        0x00000025        /*<QCIO017>     CS           */
#define DFGC_IBP_CMD_THRES     0x00000027        /*<QCIO017>                  */
#define DFGC_IBP_CMD_SET_PARAM 0x00000028        /*<QCIO017>                  */
#define DFGC_IBP_CMD_SET_ID    0x0000002A        /*<QCIO017> IMEI             */
#define DFGC_IBP_CMD_SET_CAPA  0x0000002C        /*<QCIO017>                  */
#define DFGC_IBP_CMD_SET_ADDR  0x0000002D        /*<QCIO017> EEPROM           */
#define DFGC_IBP_CMD_SET_CCODE 0x0000002E        /*<QCIO017>                  */
#define DFGC_IBP_CMD_RESET     0x0000002F        /*<QCIO017>                  */
                                                 /*<QCIO017>                  */
#define DFGC_IBP_CMD_MAX       0x00000030        /*<QCIO017>                  */
#define DFGC_IBP_CMD_INIT      0x000000FF        /*<QCIO017>                  */
                                                 /*<QCIO017>                  */
/*                                             *//*<QCIO017>                  */
#define DFGC_IBP_CHECK_LOG_DATA    3             /*<QCIO017>                  */
#define DFGC_IBP_CHECK_LOG_MAX  0xFF             /*<QCIO017>                  */

#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */

#define OMAP4_GPIO6_BASE		0x4805D000
#define OMAP4_GPIO_AS_LEN		4096
#define OMAP4_GPIO_IRQSTATUS0		0x002c
#define OMAP4_GPIO_IRQSTATUSSET0	0x0034
#define OMAP4_GPIO_IRQSTATUSCLR0	0x003c

/* for IRQ Control */
#define DFGC_DRIVER_NAME                     "fuelgauge_drv"
/* IDPower #define DFGC_PORT_NO                         (176) */
/* Quad #define DFGC_PORT_NO                         (58) *//* IDPower */
#define DFGC_PORT_NO                         (81) /* Quad */
#define DFGC_MASK_GPIO_FGC_PORT              (0x01 << (DFGC_PORT_NO % 32))

/* for debug */
#define OCV_LOG_PRINT( lineno, function )                           \
{                                                                   \
    printk( "### FG ### %s line = %d \n", (function), (lineno) );   \
}

#define OCV_ROMDATA_LOG(function, ret, id_name, id, data)  \
{                                                          \
    printk("### FG ### %s , ret = %d, id_name = %s , id = %d, data = [0x%02X] \n", (function), (ret), (id_name), (id), (data)); \
}

#define LOG_CORE( kind, fmt, ... )      \
{                                       \
    struct timeval tp;                  \
    do_gettimeofday( &tp );             \
                                        \
    printk( KERN_INFO "###%s### %ld:%ld %04d %s "fmt, kind, tp.tv_sec, tp.tv_usec, __LINE__, __FUNCTION__, ##__VA_ARGS__ ); \
}

#define FGC_DEBUG( fmt, ... )        //LOG_CORE( "FGC", fmt, ##__VA_ARGS__ )
#define FGC_ERR( fmt, ... )          LOG_CORE( "FGC ERR", fmt, ##__VA_ARGS__ )
#define FGC_IRQ( fmt, ... )          //LOG_CORE( "FGC IRQ", fmt, ##__VA_ARGS__ )
#define FGC_BATTERY( fmt, ... )      //LOG_CORE( "FGC BAT", fmt, ##__VA_ARGS__ )
#define FGC_LOG( fmt, ... )          //LOG_CORE( "FGC LOG", fmt, ##__VA_ARGS__ )
#define FGC_FUNCIN( fmt, ... )       //LOG_CORE( "FGC IN  ", fmt, ##__VA_ARGS__ )
#define FGC_FUNCOUT( fmt, ... )      //LOG_CORE( "FGC OUT ", fmt, ##__VA_ARGS__ )
#define FGC_FWUP( fmt, ... )         //LOG_CORE( "FGC FWUP", fmt, ##__VA_ARGS__ )
#define FGC_CHG( fmt, ... )          //LOG_CORE( "FGC CHG", fmt, ##__VA_ARGS__ )

/* IDPower start */
//#define FGIC_DEBUG
#ifdef FGIC_DEBUG
#define DEBUG_FGIC(X)            printk X
#else /* FGIC_DEBUG */
#define DEBUG_FGIC(X)
#endif /* FGIC_DEBUG */
/* IDPower end */

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_wait_info{
	int (*pfunc)(void);
	struct t_fgc_wait_info *pnext;
}T_FGC_WAIT_INFO;

/*----- FGC-DD           -----------------------------------------------------*/
typedef struct t_fgc_ctl                         /* FGC-DD                    */
{
                                                 /*<3210049>                  */
    unsigned char            lvl_2;              /*                           */
    unsigned char            lvl_1;              /*                           */
    unsigned char            lva_3g;             /* LVA      3G               */
    unsigned char            lva_2g;             /* LVA      2G               */
    unsigned char            alarm_3g;           /*<3210049>              3G  */
    unsigned char            alarm_2g;           /*<3210049>              2G  */
    unsigned char            dump_enable;        /*                           */
    unsigned char            charge_timer;       /* FG-IC                     */
    unsigned char            soh_n;              /*                           */
    unsigned char            soh_init;           /*                           */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>    unsigned char            soh_level;          *//*                           */
#else /*<Conan>1111XX*/
    unsigned char            soh_level;          /*                           */
#endif /*<Conan>1111XX*/
    unsigned char            soh_down;           /*                           */
    unsigned char            soh_bad;            /*                           */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    unsigned char            soh_level;          /*                           *//*<Conan>*/
    unsigned char            reserved;           /*                           *//*<Conan>*/
    unsigned short           batt_capa_design;   /*             (mAh)         *//*<Conan>*/
    unsigned char            soc_cycle_val;      /*                           SOC   *//*<Conan>*/
    unsigned char            align3[ 3 ];        /* 4byte                     *//*<Conan>*/
    unsigned short           soh_degra_val_tbl[DFGC_HELTE_DEGRA_TBL_TEMP_MAX] /*                        *//*<Conan>*/
                                              [DFGC_HELTE_DEGRA_TBL_VOL_MAX]; /*                        *//*<Conan>*/
    unsigned short           soh_cycle_val;      /*                     ((mAh)10^-1) *//*<Conan>*/
    unsigned char            save_degra_batt_measure_val;/*                 (%)                *//*<Conan>*/
    unsigned char            save_soh_health;            /*                                    *//*<Conan>*/
    unsigned long            save_degra_batt_estimate_va;/*               ((%) 10^7  )         *//*<Conan>*/
    unsigned long            save_cycle_degra_batt_capa; /*                       ((%) 10^7  ) *//*<Conan>*/
    unsigned long            save_soh_degra_batt_capa;   /*                       ((%) 10^7  ) *//*<Conan>*/
    unsigned short           save_soh_cycle_cnt;         /*                                    *//*<Conan>*/
    unsigned long long       save_soh_degra_timestamp;   /*                                    *//*<Conan>*/
    unsigned long long       save_soh_thres_timestamp;   /*                                                  *//*<Conan>*/
    unsigned short           soh_degra_volt[DFGC_HELTE_DEGRA_TBL_VOL_MAX];    /*                              *//*<Conan>*/
    unsigned char            save_soc_total;             /* SOC                                *//*<Conan>*/
#endif /*<Conan>1111XX*/
    unsigned char            ocv_enable;         /*<3131029> OCV              */
    unsigned char            capacity_fix;       /*         UI                */
/*<3131029>    unsigned char            align2[ 2 ];        *//*<3210049>4byte             */
    unsigned char            no_lva;             /* LVA Non-notification      */
    int                      (*get_capacity)(void);/* Get Capacity            */
                                                 /* SDRAM                     */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    int                      (*get_degradation_info)(int);/* Get Degradation  *//* Conan */
#endif /*<Conan>1111XX*/
    T_FGC_LOG *              remap_log_dump;     /*                           */
    T_FGC_SOH *              remap_soh;          /*         TBL               */
    T_FGC_OCV *              remap_ocv;          /* OCV    TBL                */
                                                 /*                           */
    unsigned long            mac_state;          /*                           */
    unsigned char            charge_reg[DCTL_CHG_STSREG_MAX];/*status register*/ // SlimID Add
    unsigned char            chg_sts;            /* UI                        */
    unsigned char            dev1_chgsts;        /*                           */
    unsigned short           dev1_volt;          /*                           */
    unsigned char            dev1_class;         /*            class          */
    unsigned char            capacity_old;       /*                           */
    unsigned char            health_old;         /*                           */
    unsigned char            update_enable;      /* info update enable flag   */
    unsigned long            flag;               /* wakeup                    */
    unsigned char            chg_start;          /*                           */
/*<PCIO033>    unsigned char            align1[ 3 ];        *//* 3byte                     */
    unsigned char            ocv_okng[ DFGC_OCV_SAVE_MAX ]; /*<PCIO033>OCV    */
#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
    unsigned char            ibp_check;          /*<QCIO017>                  */
    unsigned char            ibp_uart3;          /*<QCIO017> UART3            */
    unsigned char            ibp_mode;           /*<QCIO017> IBP              */
    unsigned char            ibp_cmd;            /*<QCIO017> IBP              */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */
    struct timer_list        timer_info;
    struct work_struct       work_info;
    T_FGC_WAIT_INFO          *pwait_info_head;
    unsigned long            timestamp;          /*                            */
} T_FGC_CTL;

                                                 /*<1210001>                  */
typedef struct t_fgc_ecurrent                    /*<1210001> ( D_STATE_LOG )  */
{                                                /*<1210001>                  */
    unsigned char            one_timer;          /*<1210001> 1                */
    unsigned char            three_timer1;       /*<1210001> 3      1         */
    unsigned char            three_timer2;       /*<1210001> 3      2         */
    unsigned char            four_timer;         /*<1210001> 4                */
    unsigned char            five_timer;         /*<1210001> 5                */
    unsigned char            six_timer;          /*<1210001> 6                */
    unsigned char            debug_port;         /*<1210001> DEBUG            */
    unsigned char            idle3;              /*<1210001> idle3            */
    unsigned char            idle4;              /*<1210001> idle4            */
    unsigned char            idle5;              /*<1210001> idle5            */
    unsigned char            cpu;                /*<1210001>     CPU          */
    unsigned char            ram_off;            /*<1210001> RAM-OFF          */
    unsigned char            arm_fake;           /*<1210001> ARM FAKE         */
    unsigned char            hard_debug;         /*<1210001>                  */
    unsigned char            detection;          /*<1210001>                  */
    unsigned char            st_flag;            /*<1210001> ST               */
                                                 /*<1210001>                  */
    unsigned char            signal_output;      /*<1210001>                  */
    unsigned char            select_byte0;       /*<1210001>     Byte0        */
    unsigned char            select_byte1;       /*<1210001>     Byte1        */
    unsigned char            select_byte2;       /*<1210001>     Byte2        */
    unsigned char            select_byte3;       /*<1210001>     Byte3        */
    unsigned char            reserved1;          /*<1210001>                  */
    unsigned char            reserved2;          /*<1210001>                  */
    unsigned char            reserved3;          /*<1210001>                  */
    unsigned char            ccpu;               /*<1210001> CCPU ICE Mode    */
    unsigned char            brb;                /*<1210001> BRB/             */
    unsigned char            fake_power_down;    /*<1210001> Fake Power Down  */
    unsigned char            sebug_sel;          /*<1210001> DEBUG_SEL1/SEL2  */
    unsigned char            prbgr1;             /*<1210001> PRBGR1           */
    unsigned char            prbgr2;             /*<1210001> PRBGR2           */
    unsigned char            reserved4;          /*<1210001>                  */
    unsigned char            reserved5;          /*<1210001>                  */
}T_FGC_ECURRENT;                                 /*<1210001>                  */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----            ------------------------------------------------------------*/
typedef struct t_fgc_path_debug{                 /*                           */
    unsigned long   path_cnt;                    /*                           */
    unsigned long  path[ DFGC_RDPROC_DPATH_MAX ];/*                           */
}T_FGC_PATH_DEBUG;

/*----                --------------------------------------------------------*/
typedef struct t_fgc_path_error{                 /*                           */
    unsigned long   path_cnt;                    /*                           */
    unsigned long  path[ DFGC_RDPROC_EPATH_MAX ];/*                           */
}T_FGC_PATH_ERROR;

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*---- SYS             -------------------------------------------------------*/
typedef struct t_fgc_syserr_data                 /* SYS                       */
{                                                /*                           */
    unsigned char   kind;                        /*                           */
    unsigned char   align1[ 3 ];                 /* 4byte align               */
    unsigned long   func_no;                     /*                           */
    unsigned char   data[ CSYSERR_SYSINF_LEN ];  /*                           */
} T_FGC_SYSERR_DATA;

/*---- D_HCM_FG_CONSUMPTION_TIMESTAMP             ----------------------------*/
/*<3131028>typedef struct t_fgc_soh_time                    *//*                           */
typedef struct t_fgc_soc_time                    /*<3131028>                          */
{
    unsigned long   time_80;                     /* 80%                       */
    unsigned long   time_60;                     /* 60%                       */
    unsigned long   time_40;                     /* 40%                       */
    unsigned long   time_20;                     /* 20%                       */
    unsigned long   time_10;                     /* 10%                       */
/*<3131028>} T_FGC_SOH_TIME;                   */
} T_FGC_SOC_TIME;                                /*<3131028>                  */

/*<3131028>typedef struct t_fgc_soh_log                     *//*                           */
/*<3131028>{                                                */
/*<3131028>    T_FGC_SOH_TIME  soh_t[ DFGC_LOG_SOH_MAX ];   *//*                           */
/*<3131028>} T_FGC_SOH_LOG;                                 */
typedef struct t_fgc_soc_log                     /*<3131028>                    */
{                                                /*<3131028>                  */
    T_FGC_SOC_TIME  soh_t[ DFGC_LOG_SOC_MAX ];   /*<3131028>                  */
} T_FGC_SOC_LOG;                                 /*<3131028>                  */

/*-----       ECO     -----------------------------<QEIO022>------------------*/
typedef struct t_fgc_eco_soc_log                 /*<QEIO022>       ECO    SOC */
{                                                /*<QEIO022>                  */
    unsigned short soc[ DFGC_LOG_ECO_MAX ];      /*<QEIO022>                  */
} T_FGC_ECO_SOC_LOG;                             /*<QEIO022>                  */
typedef struct t_fgc_eco_time_log                /*<QEIO022>       ECO        */
{                                                /*<QEIO022>                  */
    unsigned long  time[ DFGC_LOG_ECO_MAX ];     /*<QEIO022>                  */
} T_FGC_ECO_TIME_LOG;                            /*<QEIO022>                  */

/*----------------------------------------------------------------------------*/
/* /dev/fg/info                                                               */
/*----------------------------------------------------------------------------*/
#define DFGC_REGGET_MAX       28                 /* FG-IC Offset tbl MAX      */
#define DFGC_REGGET_OFFSET_AI (16)               /* FG-IC Offset tbl AI       */
#define DFGC_REGGET_OFFSET_FCC (14)               /* FG-IC Offset tbl FCC     *//* IDPower */

/*----------------------------------------------------------------------------*/
/* /dev/fg/df                                                                 */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_df_subclass                 /* DF                        */
{
    unsigned char    subclass;                    /* Sub Class ID             */
    unsigned char    block;                       /* Block Number             */
    unsigned char    block_data;                  /* Block Data Number        */
} T_FGC_DF_SUBCLASS;

/*----------------------------------------------------------------------------*/
/* /dev/fg/fw                                                                 */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_fw_rom_memmap               /* ROM    FGIC-FW            */
{                                                /*                           */
    unsigned short      rom_inst_ver;            /* ROM    Instraction Ver    */
    unsigned short      rom_align1;              /* 4byte                     */
    unsigned long       rom_df_ver;              /* ROM    DataFlash Ver      */
    unsigned char       rom_align2[8];           /* 16byte                    */
    unsigned char       rom_Pack0_Ra[32];        /* ROM    Pack0              */
    unsigned char       rom_align3[976];         /* 1kbyte                    */
    unsigned char       rom_DataFlash[1024];     /* ROM    DataFlash          */
    unsigned char       rom_InstFlash[49152];    /* ROM    InstractionFlash   */
    unsigned char       rom_align4[79872];       /* 128kbyte                  */
} T_FGC_FW_COM_MEMMAP;                           /*                           */

/*----------------------------------------------------------------------------*/
/* IBP                                                                        */
/*----------------------------------------------------------------------------*/
#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
typedef struct t_fgc_ibp_check_log               /*<QCIO017>                  */
{                                                /*<QCIO017>                  */
    unsigned long  time_stamp[ DFGC_IBP_CHECK_LOG_DATA ];   /*<QCIO017>       */
} T_FGC_IBP_CHECK_LOG;                           /*<QCIO017>                  */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*----------------------------------------------------------------------------*//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*----------------------------------------------------------------------------*//*<Conan>*/
typedef struct t_fgc_time_info                   /*                           *//*<Conan>*/
{                                                /*                           *//*<Conan>*/
    unsigned long       day;                     /*                           *//*<Conan>*/
    unsigned char       hour;                    /*                           *//*<Conan>*/
    unsigned char       min;                     /*                           *//*<Conan>*/
    unsigned char       sec;                     /*                           *//*<Conan>*/
    unsigned long       usec;                    /*                           *//*<Conan>*/
} T_FGC_TIME_INFO;                               /*                           *//*<Conan>*/

typedef struct t_fgc_time_diff_info              /*                           *//*<Conan>*/
{                                                /*                           *//*<Conan>*/
    unsigned long       min;                     /*                           *//*<Conan>*/
    unsigned char       sec;                     /*                           *//*<Conan>*/
    unsigned long       usec;                    /*                           *//*<Conan>*/
} T_FGC_TIME_DIFF_INFO;                          /*                           *//*<Conan>*/
#endif /*<Conan>1111XX*/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
extern spinlock_t          gfgc_spinlock_if;    /* IF                         */
extern spinlock_t          gfgc_spinlock_info;  /*                            */
extern spinlock_t          gfgc_spinlock_log;   /*                            */
extern spinlock_t          gfgc_spinlock_fgc;   /*<1905359>                   */
#ifdef    SYS_HAVE_IBP                          /*<QCIO017>                   */
extern spinlock_t          gfgc_spinlock_ibp;   /*<QCIO017> IBP               */
#endif /* SYS_HAVE_IBP */                       /*<QCIO017>                   */
extern struct semaphore    gfgc_sem_update;     /*<1905359>                   */
extern struct completion   gfgc_comp;
extern unsigned long       gfgc_int_flag;       /*<3210076>                   */
extern wait_queue_head_t   gfgc_task_wq;        /*<1905359>                   */
extern T_FGC_ECURRENT      gfgc_ecurrent;       /*<1210001>                   */
#ifdef    SYS_HAVE_IBP                          /*<QCIO017>                   */
extern wait_queue_head_t   gfgc_ibp_wq;         /*<QCIO017> IBP               */
#endif /* SYS_HAVE_IBP */                       /*<QCIO017>                   */

extern wait_queue_head_t   gfgc_susres_wq;      /* npdc300035452 */
extern volatile atomic_t   gfgc_susres_flag;    /* npdc300035452 */

//Jessie add start
extern unsigned char       gfgc_chgsts_thread;
//Jessie add end

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
extern unsigned char       gfgc_open_cnt[ ];      /*         DD open          */

extern const unsigned long error_timestamp_tbl[];
extern const unsigned long error_code_tbl[];
extern const unsigned long error_path_tbl[];

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
extern T_FGC_CTL           gfgc_ctl;             /* FGC-DD                     */
extern T_FGC_BATT_INFO     gfgc_batt_info;       /*                            */
extern const unsigned long gfgc_fgic_offset[ DFGC_REGGET_MAX ]; /* FuelGauge-IC Offset tbl*/

extern T_FGC_PATH_ERROR    gfgc_path_error;
extern T_FGC_PATH_DEBUG    gfgc_path_debug;

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
extern unsigned char       gfgc_verup_type;

extern unsigned long       gfgc_fgic_mode;      /*<3130560>                   */
/* Control()                            */
extern const unsigned long CtrlRegAdr[2];

extern const unsigned char CtrlStsCmd[2];
extern const unsigned char FgicResetCmd[2];
extern const unsigned char ItEnableCmd[2];
extern const unsigned char SealCmd[2];
extern const unsigned char FwVersionCmd[2];
extern const unsigned char BootRomMode[2];
extern const unsigned char UnsealCmd1[2];
extern const unsigned char UnsealCmd0[2];
extern const unsigned char FullAccsCmd1[2];
extern const unsigned char FullAccsCmd0[2];

extern const unsigned long ROMCmdAdr[3];
extern const unsigned char NormalMode[3];
extern const unsigned char OnePageErs[3];

extern const unsigned long MassErsAdr[5];
extern const unsigned char MassErsCmd[5];

extern const unsigned long ROMRwSts[1];

extern const unsigned long DFErsAdr[5];
extern const unsigned char DFErsCmd[5];

extern const unsigned long ROMWrCmdAdr[3];
extern const unsigned long ROMWrDatAdr[1];
extern const unsigned long ROMWrSumAdr[2];

extern const unsigned long PkBtCmdWtAdr[7];       /*<3210046>                 */
extern const unsigned char I2C400KSet[7];         /*<3210046>                 */
extern const unsigned long PkBtCmdRdAdr[6];       /*<3210046>                 */
extern const unsigned char I2CSpdGet[6];          /*<3210046>                 */
extern const unsigned long PkBtCmdRdReg[1];       /*<3210046>                 */

extern const T_FGC_DF_SUBCLASS DF_SC_Qmax0;
extern const unsigned long Qmax_readtbl[2];       /*<3131157>                 */
extern const T_FGC_DF_SUBCLASS DF_SC_Qmax1;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra1Sts;
extern const T_FGC_DF_SUBCLASS DF_SC_Rax0Sts;
extern const T_FGC_DF_SUBCLASS DF_SC_Rax1Sts;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0Sts;
extern const unsigned long Ra0_readtbl[19];       /*<3131157>                 */
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0Flg;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0Btr_L;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0Btr_H;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0Gain;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_1;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_2;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_3;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_4;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_5;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_6;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_7;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_8;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_9;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_10;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_11;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_12;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_13;
extern const T_FGC_DF_SUBCLASS DF_SC_Ra0_14;
extern const T_FGC_DF_SUBCLASS DF_SC_DFVer0;
extern const T_FGC_DF_SUBCLASS DF_SC_DFVer1;
extern const T_FGC_DF_SUBCLASS DF_SC_DFVer2;
extern const T_FGC_DF_SUBCLASS DF_SC_DFVer3;
extern const unsigned long DFBlkCtrlAdr[3];
extern const unsigned long DFBlkSumAdr[1];
extern const unsigned long DFVerReadAdr[3];
extern const unsigned char DFVerReadCmd[3];
extern const unsigned long DFVerAdr[4];

extern const unsigned long UDStsCmdAdr[3];
extern const unsigned char UDSts_toUPD[3];
extern const unsigned char UDSts_toNML[3];
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
extern const T_FGC_DF_SUBCLASS DF_SC_Cyclecount0;/*<Conan>*/
extern const T_FGC_DF_SUBCLASS DF_SC_Cyclecount1;/*<Conan>*/
extern const unsigned long Cyclecount0_readtbl[2];      /*<Conan>*/
extern const unsigned long Cyclecount1_readtbl[2];      /*<Conan>*/
#endif /*<Conan>1111XX*/


extern struct file_operations gfgc_fops;

extern wait_queue_head_t      gfgc_fginfo_wait;  /* FuelGaugeIC               */

extern volatile unsigned long gfgc_kthread_flag; /*<1905359>                  */

/*----------------------------------------------------------------------------*/
/* IBP                                                                        */
/*----------------------------------------------------------------------------*/
#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
extern unsigned int           gfgc_ibp_data_num; /*<QCIO017>                  */
extern unsigned char                             /*<QCIO017>                  */
    gfgc_ibp_data[ DFGC_IBP_RECV_MAX + 1 ];      /*<QCIO017>                  */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
extern unsigned int pfgc_fw_update( unsigned char type );
extern unsigned int pfgc_fw_fullaccessmode_set( unsigned int *old_status );
extern unsigned int pfgc_fw_fullaccessmode_rev( unsigned int *old_status );
extern unsigned int pfgc_fgic_modecheck( void );
extern unsigned int pfgc_fgic_modechange( unsigned char mode );
extern unsigned int pfgc_fw_rowwrite( TCOM_ID com_acc_id, unsigned char type,
                       unsigned short row_no , unsigned char * rw_data );
extern unsigned int pfgc_fw_rowread( TCOM_ID com_acc_id,
                       unsigned short row_no , unsigned char * rw_data );
extern unsigned int pfgc_fw_rowread_df( TCOM_ID com_acc_id,
                       unsigned short row_no , unsigned char * rw_data );
extern unsigned int pfgc_fgic_dfread( const T_FGC_DF_SUBCLASS * df_subclass , unsigned char * rw_data );
extern unsigned int pfgc_fgic_dfread_num( const T_FGC_DF_SUBCLASS * df_subclass ,     /*<3131157>*/
                                           unsigned char * rw_data,                   /*<3131157>*/
                                   const unsigned long * rd_tbl, unsigned char num ); /*<3131157>*/
extern unsigned int pfgc_fgic_dfwrite( const T_FGC_DF_SUBCLASS * df_subclass , unsigned char * rw_data );
extern TCOM_FUNC pfgc_fgic_comread( TCOM_ID comwrite_id, TCOM_RW_DATA * comwrite_rwdata );
extern TCOM_FUNC pfgc_fgic_comwrite( TCOM_ID comwrite_id, TCOM_RW_DATA * comwrite_rwdata );
extern TCOM_FUNC pfgc_fgic_comopen( TCOM_ID * comopen_id );      /* <PCIO034> */
extern unsigned int pfgc_fw_write( T_FGC_FW * fw_data );
extern unsigned int pfgc_df_get( T_FGC_RW_DF * df_data, unsigned char * df_rd_data  );
extern unsigned int pfgc_df_set( T_FGC_RW_DF * df_data, unsigned char * df_wr_data  );
extern unsigned int pfgc_df_study_check( T_FGC_DF_STUDY * df_study_data );
extern unsigned int pfgc_dfs_read( unsigned short * dfs_rd_data );
extern unsigned int pfgc_dfs_write( unsigned long * dfs_wr_data );
extern unsigned char pfgc_ver_init( void );
extern unsigned int pfgc_ver_read( T_FGC_FW_VER * fw_ver_data );
extern unsigned char pfgc_info_init( void );     /*                           */
extern unsigned long pfgc_info_update(           /*                           */
                     unsigned char type );
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#if DFGC_SW_DELETED_FUNC*/
extern void pfgc_info_health_init( void );       /*                           */
extern unsigned char pfgc_info_health_check(     /*                           */
                     T_FGC_REGDUMP *regdump );
extern void pfgc_log_time_ex( unsigned long long * sys_time, /*                 (    ) *//*<Conan>*/
                              unsigned long long * mono_time);/*<Conan>*/

extern T_FGC_TIME_DIFF_INFO pfgc_log_time_diff( unsigned long long src_time,  /*                      *//*<Conan>*/
                                                unsigned long long dst_time); /*<Conan>*/

extern unsigned char pfgc_info_soh_degra_check( T_FGC_REGDUMP *regdump ,unsigned long long* degra_val); /*                        *//*<Conan>*/
/*<Conan>#endif*/ /* DFGC_SW_DELETED_FUNC */
#else /*<Conan>1111XX*/
#if DFGC_SW_DELETED_FUNC
extern void pfgc_info_health_init( void );       /*                           */
extern unsigned char pfgc_info_health_check(     /*                           */
                     T_FGC_REGDUMP *regdump );
#endif /* DFGC_SW_DELETED_FUNC */
#endif /*<Conan>1111XX*/
extern unsigned char pfgc_info_batt_level(       /*                           */
                     unsigned char soc );
extern int pfgc_fgc_nvm_get( void );             /*                           */
extern int pfgc_fgc_regget( unsigned char type,  /* FG-IC                     */
                      TCOM_RW_DATA  *com_acc_rwdata );
extern irqreturn_t pfgc_fgc_int( int irq, void *dev_id ); /* FG-IC            */
extern void pfgc_fgc_ocv_request( void );        /* OCV                       */
extern int  pfgc_fgc_kthread( void* arg );       /*                           */
extern void pfgc_log_syserr( T_FGC_SYSERR_DATA *info ); /* SYS                */
extern void pfgc_log_dump_init( void );          /* FG-IC                     */
extern void pfgc_log_dump( unsigned char type,   /* FG-IC                     */
                           T_FGC_REGDUMP *regdump);
extern void pfgc_log_ocv_init( void );           /* OCV                       */
extern int pfgc_log_ocv( void );                 /* OCV                       */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>extern void pfgc_log_health( unsigned char health );*//*                        */
extern void pfgc_log_health( unsigned char force_flg );        /*                      *//*<Conan>*/
#else /*<Conan>1111XX*/
extern void pfgc_log_health( unsigned char health );/*                        */
#endif /*<Conan>1111XX*/
extern void pfgc_log_charge( unsigned char type, /*                           */
                      T_FGC_REGDUMP *regdump );
extern void pfgc_log_capacity( unsigned char type ); /*                       */
extern unsigned long  pfgc_log_time( void );     /*                           */
extern void pfgc_log_eco( unsigned long *mode ); /*<QEIO022>       ECO        */
extern int pfgc_log_rdproc( char  *pk_page,      /*         DD proc           */
                            char  **pk_start,
                            off_t pk_off,
                            int   pk_count,
                            int   *pk_eof,
                            void  *pk_data );
extern void pfgc_log_proc_init( void );
extern void pfgc_log_proc_suspend( void );
extern void pfgc_log_proc_resume( void );
extern void pfgc_log_shutdown( void );

extern int pfgc_wait_info_push( T_FGC_WAIT_INFO *pinfo );
extern int pfgc_wait_info_pop( T_FGC_WAIT_INFO *pinfo );
extern int pfgc_wait_info_depop( T_FGC_WAIT_INFO *pinfo );
extern int pfgc_exec_retry( T_FGC_WAIT_INFO *pinfo );
extern void pfgc_init_timer( void );
extern void pfgc_add_timer( void );
extern void pfgc_uevent_send( void );

#ifdef    SYS_HAVE_IBP                           /*<QCIO017>                  */
extern int pfgc_log_rdproc_ibp( char  *pk_page,  /*<2040002>                  */
                                char  **pk_start,/*<2040002>                  */
                                off_t pk_off,    /*<2040002>                  */
                                int   pk_count,  /*<2040002>                  */
                                int   *pk_eof,   /*<2040002>                  */
                                void  *pk_data );/*<2040002>                  */
extern unsigned char pfgc_ibp_init( void );      /*<QCIO017>                  */
extern signed int    pfgc_ibp_check_read(        /*<QCIO017>                  */
                         char          *buf,     /*<QCIO017>                  */
                         size_t        count,    /*<QCIO017>                  */
                         unsigned char type );   /*<QCIO017>                  */
#endif /* SYS_HAVE_IBP */                        /*<QCIO017>                  */

#if DFGC_SW_TEST_DEBUG
extern int pfgc_log_rdproc_sdram( char  *pk_page, /* readproc of sdram        */
                                  char  **pk_start,
                                  off_t pk_off,
                                  int   pk_count,
                                  int   *pk_eof,
                                  void  *pk_data );
extern int pfgc_log_rdproc_nand1( char  *pk_page,   /* rerdproc of            */
                                  char  **pk_start, /*          nand capacity */
                                  off_t pk_off,
                                  int   pk_count,
                                  int   *pk_eof,
                                  void  *pk_data );
extern int pfgc_log_rdproc_nand2( char  *pk_page,    /* rerdproc of           */
                                  char  **pk_start,  /*   nand ocv and charge */
                                  off_t pk_off,
                                  int   pk_count,
                                  int   *pk_eof,
                                  void  *pk_data );
#endif /* DFGC_SW_TEST_DEBUG */
extern void pfgc_log_i2c_error( unsigned short path, unsigned short info );

#endif                                           /* _pfgc_local_h             */

