/*
 * drivers/fgc/pfgc_table.c
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

/*----------------------------------------------------------------------------*/
/* include                                                                    */
/*----------------------------------------------------------------------------*/
#include "inc/pfgc_local.h"                      /*         DD                */

/*----------------------------------------------------------------------------*/
/*                                                              */
/*----------------------------------------------------------------------------*/
T_FGC_CTL           gfgc_ctl;                   /* FGC-DD                     */
T_FGC_BATT_INFO     gfgc_batt_info;             /*                            */
spinlock_t          gfgc_spinlock_if;           /* IF                         */
spinlock_t          gfgc_spinlock_info;         /*                            */
spinlock_t          gfgc_spinlock_log;          /*                            */
spinlock_t          gfgc_spinlock_fgc;          /*<1905359>                   */
#ifdef    SYS_HAVE_IBP                          /*<QCIO017>                   */
spinlock_t          gfgc_spinlock_ibp;          /*<QCIO017> IBP               */
#endif /* SYS_HAVE_IBP */                       /*<QCIO017>                   */

struct semaphore    gfgc_sem_update;            /*<1905359>                   */
struct completion   gfgc_comp;

T_FGC_PATH_ERROR    gfgc_path_error;
T_FGC_PATH_DEBUG    gfgc_path_debug;

unsigned long       gfgc_fgic_mode;             /*<3130560>                   */
unsigned long       gfgc_int_flag;              /*<3210076>                   */

volatile unsigned long gfgc_kthread_flag;       /*<1905359>                   */
wait_queue_head_t   gfgc_task_wq;               /*<1905359>                   */

volatile atomic_t   gfgc_susres_flag;           /* npdc300035452 */
wait_queue_head_t   gfgc_susres_wq;             /* npdc300035452 */

T_FGC_ECURRENT      gfgc_ecurrent;              /*<1210001>                   */

#ifdef    SYS_HAVE_IBP                          /*<QCIO017>                   */
wait_queue_head_t   gfgc_ibp_wq;                /*<QCIO017> IBP               */
#endif /* SYS_HAVE_IBP */                       /*<QCIO017>                   */

//Jessie add start
unsigned char       gfgc_chgsts_thread;
//Jessie add end

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
unsigned char       gfgc_open_cnt[ ] = { 0,0,0,0,0,   /*         DD open           */
                                         0,0,0,0,0,
                                         0,0,0,0,0,
                                         0,0,0,0,0 };

const unsigned long error_timestamp_tbl[] = {
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP1,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP2,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP3,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP4,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP5,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP6,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP7,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP8,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP9,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP10,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP11,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP12,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP13,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP14,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP15,
        D_HCM_E_FGIC_I2C_ERROR_TIMESTAMP16
};
const unsigned long error_code_tbl[] = {
        D_HCM_E_FGIC_I2C_ERROR_CODE1,
        D_HCM_E_FGIC_I2C_ERROR_CODE2,
        D_HCM_E_FGIC_I2C_ERROR_CODE3,
        D_HCM_E_FGIC_I2C_ERROR_CODE4,
        D_HCM_E_FGIC_I2C_ERROR_CODE5,
        D_HCM_E_FGIC_I2C_ERROR_CODE6,
        D_HCM_E_FGIC_I2C_ERROR_CODE7,
        D_HCM_E_FGIC_I2C_ERROR_CODE8,
        D_HCM_E_FGIC_I2C_ERROR_CODE9,
        D_HCM_E_FGIC_I2C_ERROR_CODE10,
        D_HCM_E_FGIC_I2C_ERROR_CODE11,
        D_HCM_E_FGIC_I2C_ERROR_CODE12,
        D_HCM_E_FGIC_I2C_ERROR_CODE13,
        D_HCM_E_FGIC_I2C_ERROR_CODE14,
        D_HCM_E_FGIC_I2C_ERROR_CODE15,
        D_HCM_E_FGIC_I2C_ERROR_CODE16
};
const unsigned long error_path_tbl[] = {
        D_HCM_E_FGIC_I2C_ERROR_PATH1,
        D_HCM_E_FGIC_I2C_ERROR_PATH2,
        D_HCM_E_FGIC_I2C_ERROR_PATH3,
        D_HCM_E_FGIC_I2C_ERROR_PATH4,
        D_HCM_E_FGIC_I2C_ERROR_PATH5,
        D_HCM_E_FGIC_I2C_ERROR_PATH6,
        D_HCM_E_FGIC_I2C_ERROR_PATH7,
        D_HCM_E_FGIC_I2C_ERROR_PATH8,
        D_HCM_E_FGIC_I2C_ERROR_PATH9,
        D_HCM_E_FGIC_I2C_ERROR_PATH10,
        D_HCM_E_FGIC_I2C_ERROR_PATH11,
        D_HCM_E_FGIC_I2C_ERROR_PATH12,
        D_HCM_E_FGIC_I2C_ERROR_PATH13,
        D_HCM_E_FGIC_I2C_ERROR_PATH14,
        D_HCM_E_FGIC_I2C_ERROR_PATH15,
        D_HCM_E_FGIC_I2C_ERROR_PATH16
};

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
const unsigned long gfgc_fgic_offset[ DFGC_REGGET_MAX ] = /* FuelGauge-IC Offset tbl*/
{
    DFGC_IC_CNTL_L,
    DFGC_IC_CNTL_H,
    DFGC_IC_TEMP_L,
    DFGC_IC_TEMP_H,
    DFGC_IC_VOLT_L,
    DFGC_IC_VOLT_H,
    DFGC_IC_FLAGS_L,
    DFGC_IC_FLAGS_H,
    DFGC_IC_NAC_L,
    DFGC_IC_NAC_H,
    DFGC_IC_FAC_L,
    DFGC_IC_FAC_H,
    DFGC_IC_RM_L,
    DFGC_IC_RM_H,
    DFGC_IC_FCC_L,
    DFGC_IC_FCC_H,
    DFGC_IC_AI_L,
    DFGC_IC_AI_H,
    DFGC_IC_TTE_L,
    DFGC_IC_TTE_H,
    DFGC_IC_TTF_L,
    DFGC_IC_TTF_H,
    DFGC_IC_SOH,
    DFGC_IC_SOH_STT,
    DFGC_IC_CNTL_L,       /* CC                                               */
    DFGC_IC_CNTL_H,       /* CC                                               */
    DFGC_IC_SOC_L,
    DFGC_IC_SOC_H
};

DECLARE_WAIT_QUEUE_HEAD( gfgc_fginfo_wait );     /*  V FuelGauge-IC               */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
unsigned char       gfgc_verup_type;
/* Control()                            */
const unsigned long CtrlRegAdr[]   = { 0x00, 0x01 }; /* Control() Reg Adr     */

const unsigned char CtrlStsCmd[]   = { 0x00, 0x00 }; /*                       */
const unsigned char FgicResetCmd[] = { 0x41, 0x00 };
const unsigned char ItEnableCmd[]  = { 0x21, 0x00 };
const unsigned char SealCmd[]      = { 0x20, 0x00 };
const unsigned char FwVersionCmd[] = { 0x02, 0x00 };
const unsigned char BootRomMode[]  = { 0x00, 0x0F };
const unsigned char UnsealCmd1[]   = { 0x14, 0x04 };
const unsigned char UnsealCmd0[]   = { 0x72, 0x36 };
const unsigned char FullAccsCmd1[] = { 0xFF, 0xFF };
const unsigned char FullAccsCmd0[] = { 0xFF, 0xFF };

const unsigned long ROMCmdAdr[]    = { 0x00, 0x64, 0x65 };
const unsigned char NormalMode[]   = { 0x0F, 0x0F, 0x00 };
const unsigned char OnePageErs[]   = { 0x03, 0x03, 0x00 };

const unsigned long MassErsAdr[]   = { 0x00, 0x04, 0x05,0x64, 0x65 };
const unsigned char MassErsCmd[]   = { 0x04, 0x83, 0xDE,0x65, 0x01 };

const unsigned long ROMRwSts[]     = { 0x66 };

const unsigned long DFErsAdr[]     = { 0x00, 0x04, 0x05,0x64, 0x65 };
const unsigned char DFErsCmd[]     = { 0x0C, 0x83, 0xDE,0x6D, 0x01 };

const unsigned long ROMWrCmdAdr[] = { 0x00, 0x01, 0x02 };
const unsigned long ROMWrDatAdr[] = { 0x04 };
const unsigned long ROMWrSumAdr[] = { 0x64, 0x65 };

const unsigned long PkBtCmdWtAdr[] = { 0x00, 0x01, 0x02, 0x04, 0x05, 0x64, 0x65 }; /*<3210046>*/
const unsigned char I2C400KSet[]   = { 0x06, 0x50, 0x80, 0x01, 0x0B, 0xE2, 0x00 }; /*<3210046>*/
const unsigned long PkBtCmdRdAdr[] = { 0x00, 0x01, 0x02, 0x04, 0x64, 0x65 };       /*<3210046>*/
const unsigned char I2CSpdGet[]    = { 0x07, 0x50, 0x80, 0x01, 0xD8, 0x00 };       /*<3210046>*/
const unsigned long PkBtCmdRdReg[] = { 0x05 };                                     /*<3210046>*/

const T_FGC_DF_SUBCLASS DF_SC_Qmax0     = { 0x52, 0x00, 0x42 };
const unsigned long Qmax_readtbl[] = { 0x42 , 0x47 };                           /*<3131157>*/
const T_FGC_DF_SUBCLASS DF_SC_Qmax1     = { 0x52, 0x00, 0x47 };
const T_FGC_DF_SUBCLASS DF_SC_Ra1Sts    = { 0x5C, 0x00, 0x40 };
const T_FGC_DF_SUBCLASS DF_SC_Rax0Sts   = { 0x5D, 0x00, 0x40 };
const T_FGC_DF_SUBCLASS DF_SC_Rax1Sts   = { 0x5E, 0x00, 0x40 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0Sts    = { 0x5B, 0x00, 0x40 };
const unsigned long Ra0_readtbl[] = { 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /*<3131157>*/
                                      0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, /*<3131157>*/
                                      0x50, 0x51, 0x52 };                             /*<3131157>*/
const T_FGC_DF_SUBCLASS DF_SC_Ra0Flg    = { 0x5B, 0x00, 0x41 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0Btr_L  = { 0x5B, 0x00, 0x42 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0Btr_H  = { 0x5B, 0x00, 0x43 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0Gain   = { 0x5B, 0x00, 0x44 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_1     = { 0x5B, 0x00, 0x45 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_2     = { 0x5B, 0x00, 0x46 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_3     = { 0x5B, 0x00, 0x47 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_4     = { 0x5B, 0x00, 0x48 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_5     = { 0x5B, 0x00, 0x49 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_6     = { 0x5B, 0x00, 0x4A };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_7     = { 0x5B, 0x00, 0x4B };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_8     = { 0x5B, 0x00, 0x4C };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_9     = { 0x5B, 0x00, 0x4D };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_10    = { 0x5B, 0x00, 0x4E };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_11    = { 0x5B, 0x00, 0x4F };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_12    = { 0x5B, 0x00, 0x50 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_13    = { 0x5B, 0x00, 0x51 };
const T_FGC_DF_SUBCLASS DF_SC_Ra0_14    = { 0x5B, 0x00, 0x52 };
const T_FGC_DF_SUBCLASS DF_SC_DFVer0    = { 0x39, 0x01, 0x41 };
const T_FGC_DF_SUBCLASS DF_SC_DFVer1    = { 0x39, 0x01, 0x42 };
const T_FGC_DF_SUBCLASS DF_SC_DFVer2    = { 0x39, 0x01, 0x43 };
const T_FGC_DF_SUBCLASS DF_SC_DFVer3    = { 0x39, 0x01, 0x44 };
const unsigned long DFBlkCtrlAdr[]    = { 0x61, 0x3E, 0x3F };
const unsigned long DFBlkSumAdr[]     = { 0x60 };
const unsigned long DFVerReadAdr[]    = { 0x61, 0x3E, 0x3F };
const unsigned char DFVerReadCmd[]    = { 0x01, 0x39, 0x01 };
const unsigned long DFVerAdr[]        = { 0x41, 0x42, 0x43, 0x44 };

const unsigned long UDStsCmdAdr[]    = { 0x61, 0x3E, 0x40 };
const unsigned char UDSts_toUPD[]    = { 0x00, 0x3A, 0xAA };
const unsigned char UDSts_toNML[]    = { 0x00, 0x3A, 0x11 };
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
const T_FGC_DF_SUBCLASS DF_SC_Cyclecount0     = { 0x52, 0x00, 0x44 };           /*<Conan>*/
const unsigned long Cyclecount0_readtbl[] = { 0x44, 0x45 };                     /*<Conan>*/
const T_FGC_DF_SUBCLASS DF_SC_Cyclecount1     = { 0x52, 0x00, 0x49 };           /*<Conan>*/
const unsigned long Cyclecount1_readtbl[] = { 0x49, 0x4A };                     /*<Conan>*/
#endif /*<Conan>1111XX*/


/* EEPROM Access Table */
struct t_fgc_eeprom_tbl const EEPDATA[] = {
    { D_HCM_CHARGE_FIN_COUNT,         0x0002 },
    { D_HCM_CHARGE_FIN_1STTIME,       0x0002 },
    { D_HCM_CHARGE_FIN_TIMESTAMP,     0x000C },
    { D_HCM_CHARGE_FIN_SOC,           0x0006 },
    { D_HCM_CHARGE_FIN_FCC,           0x0006 },
    { D_HCM_FG_CONSUMPTION_TIMESTAMP, 0x008C },
    { D_HCM_FG_CONSUMPTION_COUNT,     0x0002 },
    { D_HCM_FG_OCV_OKNG,              0x0003 },
    { D_HCM_FG_OCV_TIMESTAMP,         0x000C },
    { D_HCM_FG_OCV_TEMP,              0x0006 },
    { D_HCM_FG_OCV_VOLT,              0x0006 },
    { D_HCM_FG_OCV_NAC,               0x0006 },
    { D_HCM_FG_OCV_FAC,               0x0006 },
    { D_HCM_FG_OCV_RM,                0x0006 },
    { D_HCM_FG_OCV_FCC,               0x0006 },
    { D_HCM_FG_OCV_SOC,               0x0006 },
    { D_HCM_FG_OCV_COUNT,             0x0002 },
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>    { D_HCM_FG_OCV_FLAG,              0x0001 }*/
    { D_HCM_FG_OCV_FLAG,              0x0001 },                                 /*<Conan>*/
    { D_HCM_FG_PARAM2,                0x0009 },                                 /*<Conan>*/
    { D_HCM_E_SDETERIO_FTEMP,         0x0018 },                                 /*<Conan>*/
    { D_HCM_E_DEGRADA_CONST_CYCLE,    0x0002 },                                 /*<Conan>*/
    { D_HCM_E_SDETERIO_FVOL,          0x0006 },                                 /*<Conan>*/
    { D_HCM_E_DEGRADA_BL50,           0x0010 },                                 /*<Conan>*/
    { D_HCM_E_DEGRADA_BL50_SAVE_TSTAMP, 0x0008 },                               /*<Conan>*/
    { D_HCM_E_H24_SAVE,               0x0010 },                                 /*<Conan>*/
    { D_HCM_E_H24_SAVE_TSTAMP,        0x0008 },                                 /*<Conan>*/
    { D_HCM_E_H24_SAVE_SOC,           0x0001 }                                  /*<Conan>*/
#else /*<Conan>1111XX*/
    { D_HCM_FG_OCV_FLAG,              0x0001 }
#endif /*<Conan>1111XX*/
/* Jessie add start */
    ,
    { D_HCM_WIRELESSCHARGE_FIN_COUNT,         0x0002 },
    { D_HCM_WIRELESSCHARGE_FIN_1STTIME,       0x0002 },
    { D_HCM_WIRELESSCHARGE_FIN_TIMESTAMP,     0x000C },
    { D_HCM_WIRELESSCHARGE_FIN_SOC,           0x0006 },
    { D_HCM_WIRELESSCHARGE_FIN_FCC,           0x0006 }
/* Jessie add end */
};

