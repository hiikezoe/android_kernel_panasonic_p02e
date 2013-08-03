/*
 * include/linux/rh-fgc.h
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

#ifndef _rh_fgc_h                                    /* _rh_fgc_h             */
#define _rh_fgc_h                                    /* _rh_fgc_h             */

#include<linux/laputa-fixaddr.h>

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*-----        ---------------------------------------------------------------*/

#define DFGC_OK                 0                 /*                          */
#define DFGC_NG                 1                 /*                          */


/*-----          -------------------------------------------------------------*/

#define DFGC_OFF                0                 /* OFF                      */
#define DFGC_ON                 1                 /* ON                       */

/*-----          -------------------------------------------------------------*/

#define DFGC_CLEAR              0                 /*                           */

/*----- ioctl request --------------------------------------------------------*/
#define FGIOC_BASE              0x4700            /*                          */
enum
{
    FGIOCFGINFO = FGIOC_BASE,                     /*                          */
    FGIOCDFGET,                                   /* FuelGauge-IC DF          */
    FGIOCDFSET,                                   /* FuelGauge-IC DF          */
    FGIOCDFSTUDYGET,                              /* FG-IC DF                 */
    FGIOC_MAX
};

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
/*-----                          ---------------------------------------------*/
#define DFGC_RSV_AREA  (0x20)
#define DFGC_SOH_AREA  (sizeof( T_FGC_SOH ))     /* SlimID Add                */
#define DFGC_OCV_AREA  (sizeof( T_FGC_OCV ))     /* SlimID Add                */
#define DFGC_SDRAM_LOG (DMEM_DEV1_START)         /*                           */
#define DFGC_SDRAM_RSV (DFGC_SDRAM_LOG + sizeof(T_FGC_LOG))   /*              */
#define DFGC_SDRAM_SOH (DFGC_SDRAM_RSV + DFGC_RSV_AREA)       /*         TBL  */
#define DFGC_SDRAM_OCV (DFGC_SDRAM_SOH + sizeof( T_FGC_SOH )) /* OCV    TBL   */

/*----- FGC-DD     -----------------------------------------------------------*/
#define DFGC_STATE_BIT          0x20             /* FGC-DD    bit             */

/*-----          -------------------------------------------------------------*/
// SlimID Mod #define DFGC_LOG_MAX            675              /*                           */
#define DFGC_LOG_AREA_SIZE      (DMEM_DEV1_SIZE - (DFGC_RSV_AREA + DFGC_SOH_AREA + DFGC_OCV_AREA)) /*              SlimID Add  */
#define DFGC_LOG_MAX            (DFGC_LOG_AREA_SIZE / sizeof(T_FGC_LOG_DATA))                      /*             (SlimID Mod) */

/*-----                          ---------------------------------------------*/
#define DFGC_SOH_DATA_MAX       0xFF             /* SOH                       */
#define DFGC_SOH_CNT_MAX        0xFFFFFFFF       /* SOH        MAX            */

/*----- OCV             ------------------------------------------------------*/
#define DFGC_OCV_REQUEST        0x80             /* OCV        bit            */
#define DFGC_OCV_BAT_SET        0x40             /*         OCV        bit    */
#define DFGC_OCV_CHGMASK        0x1F             /*<3230450>OCV               */


/*----------------------------------------------------------------------------*/
/*             (          define  rh-dctl.h    )                              */
/*----------------------------------------------------------------------------*/

/*---- UI               ------------------------------------------------------*/
                                                  /* I/F                      */
                                                  /*                          */
#define DFGC_HEALTH_MEASURE      0xFF             /*       (      )           */
#define DFGC_HEALTH_GOOD         0x64             /*                          */
#define DFGC_HEALTH_DOWN         0x32             /*                          */
#define DFGC_HEALTH_BAD          0x0A             /*                          */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC DF                                                            */
/*----------------------------------------------------------------------------*/

/*----          --------------------------------------------------------------*/

#define DFGC_DF_EXT             0                 /*                          */
#define DFGC_DF_INT             1                 /* FROM                     */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC Ver                                                           */
/*----------------------------------------------------------------------------*/

/*----                        ------------------------------------------------*/

#define DFGC_VER_UP             0                 /*                          */
#define DFGC_VER_DOWN           1                 /*                          */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC F/W                                                           */
/*----------------------------------------------------------------------------*/

/*----          --------------------------------------------------------------*/

#define DFGC_FW_EXT             0                 /*                          */
#define DFGC_FW_INT             1                 /* FROM                     */
#define DFGC_FW_INT_NEW         2                 /*                          */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC DF                                                            */
/*----------------------------------------------------------------------------*/

/*---- DF             --------------------------------------------------------*/

#define DFGC_DFS_ON             0                 /* DF        ON             */
#define DFGC_DFS_OFF            1                 /* DF        OFF            */

/*----------------------------------------------------------------------------*/
/*         DD    R/W    (kernel                      )                        */
/*----------------------------------------------------------------------------*/

/*----                      --------------------------------------------------*/
/*<3040001>#define D_FGC_MSTATE_IND_BASE   0x00000000        *//*                          */
/*<3040001>enum                                              *//*                          */
/*<3040001>{                                                 *//*                          */
/*<3040001>    D_FGC_FG_INIT_IND = D_FGC_MSTATE_IND_BASE,    *//* FGC-DD                   */
/*<3040001>    D_FGC_FG_INT_IND,                             *//* FG_INT                   */
/*<3040001>    D_FGC_CHG_INT_IND,                            *//*                          */
/*<3040001>    D_FGC_RF_CLASS_IND,                           *//* RF Class                 */
/*<3040001>    D_FGC_MSTATE_IND,                             *//*                          */
/*<3040001>    D_FGC_MSTATE_IND_MAX                          */
/*<3040001>};                                                */
#define D_FGC_FG_INIT_IND       0x00000001        /*<3040001>FGC-DD           */
#define D_FGC_FG_INT_IND        0x00000002        /*<3040001>FG_INT           */
#define D_FGC_CHG_INT_IND       0x00000004        /*<3040001>                 */
#define D_FGC_RF_CLASS_IND      0x00000008        /*<3040001>RF Class         */
#define D_FGC_MSTATE_IND        0x00000010        /*<3040001>                  */
#define D_FGC_ECO_IND           0x00000020        /*<QEIO022>      ECO        */
#define D_FGC_SHUTDOWN_IND      0x00000040        /* Shutdown                 */
#define D_FGC_EXIT_IND          0x00000080        /*<1100232>                 */

#define DCTL_CHG_NO_CHARGE            (0x00)      /*                           */
#define DCTL_CHG_CONST_VOLT           (0x01)      /*                           */
#define DCTL_CHG_ACADP_OVER           (0x03)      /* ACADP                     */
#define DCTL_CHG_CHARGE               (0x05)      /*                           */
#define DCTL_CHG_VOLT_ERR             (0x06)      /*                           */
#define DCTL_CHG_TEMP_ERR             (0x08)      /*                           */
#define DCTL_CHG_CHARGE_FIN           (0x13)      /*                           */
#define DCTL_CHG_EXTPWR               (0x14)      /*                           */
// SlimID Add start
#define DCTL_CHG_TIMER_TIMEOUT        (0x04)      /* Timer timeout             */
#define DCTL_CHG_SUSPEND              (0x07)      /* Suspend                   */
#define DCTL_CHG_TEMP_SOFTLIMIT       (0x09)      /* Temperature Soft Limit    */
#define DCTL_CHG_OTHER                (0xff)

#define DCTL_CHG_STSREG_MAX           12          /* Status Register Count     */
// SlimID Add end
// Jessie add start
#define DCTL_CHG_WIRELESS_CHARGE      (0x0a)      /* 無接点充電中              */
#define DCTL_CHG_WIRELESS_CHG_FIN     (0x0b)      /* 無接点充電完了            */
// Jessie add end


/*----                      --------------------------------------------------*/
/*<3040001>#define D_FGC_MSTATE_REQ        0x00000080        *//*                          */
#define D_FGC_CAPACITY_REQ      0x00000100        /* Battery Capacity         */
#define D_FGC_CURRENT_REQ       0x00000200        /* Average Current          */
#define D_FGC_FCC_REQ           0x00000400        /* FCC                      *//* IDPower */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----- FuelGauge-IC                   ---------------------------------------*/
enum                                             /* FuelGauge-IC Reg. Offset  */
{
    DFGC_IC_CNTL_L,                              /* [0x00] Contol             */
    DFGC_IC_CNTL_H,                              /* [0x01]                    */
    DFGC_IC_AR_L,                                /* [0x02] AtRate             */
    DFGC_IC_AR_H,                                /* [0x03]                    */
    DFGC_IC_ARTTE_L,                             /* [0x04] AtRateTimeToEmpty  */
    DFGC_IC_ARTTE_H,                             /* [0x05]                    */
    DFGC_IC_TEMP_L,                              /* [0x06] Temperature        */
    DFGC_IC_TEMP_H,                              /* [0x07]                    */
    DFGC_IC_VOLT_L,                              /* [0x08] Voltage            */
    DFGC_IC_VOLT_H,                              /* [0x09]                    */
    DFGC_IC_FLAGS_L,                             /* [0x0A] Flags              */
    DFGC_IC_FLAGS_H,                             /* [0x0B]                    */
    DFGC_IC_NAC_L,                               /* [0x0C] NormainalAvailableCapacity*/
    DFGC_IC_NAC_H,                               /* [0x0D]                    */
    DFGC_IC_FAC_L,                               /* [0x0E] FullAvailableCapacity*/
    DFGC_IC_FAC_H,                               /* [0x0F]                    */
    DFGC_IC_RM_L,                                /* [0x10] RemainingCapacity  */
    DFGC_IC_RM_H,                                /* [0x11]                    */
    DFGC_IC_FCC_L,                               /* [0x12] FullChargeCapacity */
    DFGC_IC_FCC_H,                               /* [0x13]                    */
    DFGC_IC_AI_L,                                /* [0x14] AverageCurrent     */
    DFGC_IC_AI_H,                                /* [0x15]                    */
    DFGC_IC_TTE_L,                               /* [0x16] TimeToEmpty        */
    DFGC_IC_TTE_H,                               /* [0x17]                    */
    DFGC_IC_TTF_L,                               /* [0x18] TimeToFull         */
    DFGC_IC_TTF_H,                               /* [0x19]                    */
    DFGC_IC_SI_L,                                /* [0x1A] StandbyCurrent     */
    DFGC_IC_SI_H,                                /* [0x1B]                    */
    DFGC_IC_STTE_L,                              /* [0x1C] StandbyTimeToEmpty */
    DFGC_IC_STTE_H,                              /* [0x1D]                    */
    DFGC_IC_MLI_L,                               /* [0x1E] MaxLoadCurrent     */
    DFGC_IC_MLI_H,                               /* [0x1F]                    */
    DFGC_IC_MLTTE_L,                             /* [0x20] MaxLoadTimeToEmpty */
    DFGC_IC_MLTTE_H,                             /* [0x21]                    */
    DFGC_IC_AE_L,                                /* [0x22] AvailableEnergy    */
    DFGC_IC_AE_H,                                /* [0x23]                    */
    DFGC_IC_AP_L,                                /* [0x24] AveragePower       */
    DFGC_IC_AP_H,                                /* [0x25]                    */
    DFGC_IC_TTECP_L,                             /* [0x26] TimeToEmptyAtConstantPower*/
    DFGC_IC_TTECP_H,                             /* [0x27]                    */
    DFGC_IC_SOH,                                 /* [0x28] StateOfHelth       */
    DFGC_IC_SOH_STT,                             /* [0x29] StateOfHelth Status*/
    DFGC_IC_CC_L,                                /* [0x2A] CycleCount         */
    DFGC_IC_CC_H,                                /* [0x2B]                    */
    DFGC_IC_SOC_L,                               /* [0x2C] StateOfCharge      */
    DFGC_IC_SOC_H,                               /* [0x2D]                    */
                                                 /*                           */
    DFGC_IC_MAX                                  /* [0x2E] FuelGauge-IC              */
};

/*----- FuelGauge-IC               -------------------------------------------*/
#define D_FGC_SLEEP_ENABLE          0x13         /* Sleep                     */
#define D_FGC_SLEEP_DISABLE         0x14         /* Sleep                     */

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_batt_info                   /*                           */
{
    unsigned char    capacity;                   /* UI          (%)           */
    unsigned char    health;                     /* UI                        */
    unsigned char    level;                      /*           (DBG    )       */
    unsigned char    batt_status;                /*               (DBG    )   */
    unsigned char    rest_batt;                  /*               (%)(DBG    )*/
    unsigned char    align1;                     /* 2byte                     */
    unsigned short   rest_time;                  /*             (DBG    )     */
    unsigned short   temp;                       /*         (DBG    )         */
    unsigned short   volt;                       /*         (DBG    )         */
    unsigned short   nac;                      /*                   (DBG    ) */
    unsigned short   fac;                /*                         (DBG    ) */
    unsigned short   rm;                       /*                   (DBG    ) */
    unsigned short   fcc;                /*                         (DBG    ) */
    unsigned short   ai;                         /*         (DBG    )         */
    unsigned short   tte;                        /*           (  )(DBG    )   */
    unsigned short   ttf;                        /*           (  )(DBG    )   */
    unsigned short   soh;                        /*           (%)(DBG    )    */
    unsigned short   cc;                         /*         (DBG    )         */
    unsigned short   soc;                        /*         (DBG    )         */
} T_FGC_BATT_INFO;

// SlimID Add start
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_chgsts_tbl                  /*                           */
{
    unsigned char    chgsts;                     /*                           */
    unsigned char    chargereg[DCTL_CHG_STSREG_MAX];/*    Status              */
} T_FGC_CHGSTS_TBL;
// SlimID Add end

/*----------------------------------------------------------------------------*/
/* DF                                                                         */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_rw_df                       /* DF                        */
{
    unsigned char    type;                       /*                           */
    unsigned char    id;                         /* Sub Class ID              */
    unsigned char    block;                      /* Block Number              */
    unsigned char    block_data;                 /* Block Data Number         */
    unsigned char *  data;                       /*                           */
} T_FGC_RW_DF;

/*----------------------------------------------------------------------------*/
/* F/W Version                                                                */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_fw_ver                      /* F/W Version               */
{
    unsigned short   inst_ver_fgic;              /* FG-IC Inst. Flash Ver.    */
    unsigned short   inst_ver_int;               /*           Inst. Flash Ver.*/
    unsigned long    df_ver_fgic;                /* FG-IC DF Ver.             */
    unsigned long    df_ver_int;                 /*           DF Ver.         */
    unsigned long    ver_up;                     /*                           */
} T_FGC_FW_VER;

/*----------------------------------------------------------------------------*/
/* F/W                                                                        */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_fw                          /* F/W                       */
{
    unsigned char    type;                       /*                           */
    unsigned char    align1[ 3 ];                /* 4byte                     */
    unsigned long    source;                     /*                           */
} T_FGC_FW;

/*----------------------------------------------------------------------------*/
/* DF                                                                         */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_df_study                    /* DF                        */
{
    unsigned char    df_study_state;             /* DF                        */
    unsigned char    align1[ 3 ];                /* 4byte                     */
} T_FGC_DF_STUDY;

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_regdump                     /* FG-IC                     */
{                                                /*                           */
    unsigned char            cntl_l;             /* Control                   */
    unsigned char            cntl_h;             /*                           */
    unsigned char            temp_l;             /* Temperature               */
    unsigned char            temp_h;             /*                           */
    unsigned char            volt_l;             /* Voltage                   */
    unsigned char            volt_h;             /*                           */
    unsigned char            flags_l;            /* Flags                     */
    unsigned char            flags_h;            /*                           */
    unsigned char            nac_l;              /* NormainalAvailableCapacity*/
    unsigned char            nac_h;              /*                           */
    unsigned char            fac_l;              /* FullAvailableCapacity     */
    unsigned char            fac_h;              /*                           */
    unsigned char            rm_l;               /* RemainingCapacity         */
    unsigned char            rm_h;               /*                           */
    unsigned char            fcc_l;              /* FullChargeCapacity        */
    unsigned char            fcc_h;              /*                           */
    unsigned char            ai_l;               /* AverageCurrent            */
    unsigned char            ai_h;               /*                           */
    unsigned char            tte_l;              /* TimeToEmpty               */
    unsigned char            tte_h;              /*                           */
    unsigned char            ttf_l;              /* TimeToFull                */
    unsigned char            ttf_h;              /*                           */
    unsigned char            soh;                /* StateOfHealth             */
    unsigned char            soh_stt;            /* StateOfHealth States      */
/*<QAIO028>    unsigned char            cc_l;               *//* CycleCount                */
/*<QAIO028>    unsigned char            cc_h;               *//*                           */
    unsigned short           mstate;             /*<QAIO028>                  */
    unsigned char            soc_l;              /* StateOfCharge             */
    unsigned char            soc_h;              /*                           */
} T_FGC_REGDUMP;                                 /*                           */

/*<QGIO012>#if defined(DIPL_ROM_IPLF) | defined(DIPL_PRISM) | defined(DIPL_RAM_IPLF)       */
#if defined(DIPL_ROM_IPLF) | defined(DIPL_PRISM) | defined(DIPL_RAM_IPLF) | defined(DIPL_LKCD) /*<QGIO012>*/
struct timeval {
        unsigned long   tv_sec;                  /* seconds */
        unsigned long   tv_usec;                 /* microseconds */
};
/*<QGIO012>#endif       *//* DIPL_ROM_IPLF *//* DIPL_PRISM *//* DIPL_RAM_IPLF */
#endif /* DIPL_ROM_IPLF *//* DIPL_PRISM *//* DIPL_RAM_IPLF *//* DIPL_LKCD */    /*<QGIO012>*/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_soh                         /*                           */
{                                                /*                           */
    unsigned char    soh_wp;                     /*                           */
    unsigned char    soh[ DFGC_SOH_DATA_MAX ];   /* SOH                       */
    unsigned short   soh_sum;                    /* SOH                       */
    unsigned char    soh_old;                    /*                           */
    unsigned char    soh_ave;                    /*                           */
    unsigned long    soh_cnt;                    /* SOH                       */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    unsigned char    sho_rsv[2];                 /*                                              *//*<Conan>*/
    unsigned char    degra_batt_measure_val;     /*                 (%)                          *//*<Conan>*/
    unsigned char    soh_health;                 /*                                              *//*<Conan>*/
    unsigned long    degra_batt_estimate_val;    /*               ((%) 10^7  )                   *//*<Conan>*/
    unsigned long    cycle_degra_batt_capa;      /*                       ((%) 10^7  )           *//*<Conan>*/
    unsigned long    soh_degra_batt_capa;        /*                       ((%) 10^7  )           *//*<Conan>*/
    unsigned short   cycle_cnt;                  /*                                              *//*<Conan>*/
    unsigned long long soh_system_timestamp;     /*                                              *//*<Conan>*/
    unsigned long long soh_degra_save_timestamp; /*                                                  *//*<Conan>*/
    unsigned long long soh_thres_timestamp;      /*                                              *//*<Conan>*/
    unsigned long long soh_monotonic_timestamp;  /*                                              *//*<Conan>*//*<Conan time add>*/
    unsigned long long soh_degra_save_monotonic_timestamp;  /*                                                  *//*<Conan>*//*<Conan time add>*/
    unsigned long long soh_degra_batt_total_capa;/*                                              *//*<Conan>*/
    unsigned short   soc_total;                  /* SOC                                          *//*<Conan>*/
    unsigned short   soc_old;                    /*       SOC                                    *//*<Conan>*/
#endif /*<Conan>1111XX*/
} T_FGC_SOH;                                     /*                           */

/*----------------------------------------------------------------------------*/
/* OCV                                                                        */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_ocv_data                    /* OCV                       */
{                                                /*                           */
    unsigned long            ocv_timestamp;      /*                           */
    unsigned char            ocv;                /* OCV                       */
    unsigned char            reserved;           /*                           */
    unsigned short           ocv_temp;           /* OCV        TEMP           */
    unsigned short           ocv_volt;           /* OCV        VOLT           */
    unsigned short           ocv_nac;            /* OCV        NAC            */
    unsigned short           ocv_fac;            /* OCV        FAC            */
    unsigned short           ocv_rm;             /* OCV        RM             */
    unsigned short           ocv_fcc;            /* OCV        FCC            */
    unsigned short           ocv_soc;            /* OCV        SOC            */
} T_FGC_OCV_DATA;                                /*                           */

/*----------------------------------------------------------------------------*/
/* OCV                                                                        */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_ocv                         /* OCV                       */
{                                                /*                           */
    unsigned short           ocv_ctrl[ 3 ];      /* OCV        CTRL           */
    unsigned char            align1[ 2 ];        /* 4byte                     */
    T_FGC_OCV_DATA           fgc_ocv_data[ 3 ];  /* OCV                       */
    unsigned char            ocv_ctl_flag;       /* OCV                       */
    unsigned char            align2[ 3 ];        /*                           */
} T_FGC_OCV;                                     /*                           */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_log_data                    /* FG-IC                     */
{                                                /*                           */
    struct timeval           tv;                 /*                           */
    unsigned char            health;             /* UI                        */
    unsigned char            dev1_chgsts;        /*                           */
    unsigned short           dev1_volt;          /*                           */
    unsigned char            dev1_class;         /*            class          */
    unsigned char            type;               /*                           */
    unsigned char            chg_sts;            /* UI                        */
    unsigned char            capacity;           /* UI                        */
    unsigned short           no;                 /*                           */
    unsigned short           log_wp;             /*                           */
    T_FGC_REGDUMP            fgc_regdump;        /* FuelGauge-IC              */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    unsigned char            soh_health;         /*                           *//*<Conan>*/
    unsigned char            reserv;             /*                           *//*<Conan>*/
    unsigned short           soh_cycle_cnt;      /*                           *//*<Conan>*/
    unsigned long            degra_batt_estimate_val;/*                       *//*<Conan>*/
    unsigned long            cycle_degra_batt_capa;  /*                       *//*<Conan>*/
    unsigned long            soh_degra_batt_capa  ;  /*                       *//*<Conan>*/
#endif /*<Conan>1111XX*/
    unsigned char            charge_reg[DCTL_CHG_STSREG_MAX];/*status register*/ // SlimID Add
    unsigned char            reserved[4];        /*                           */ // SlimID Add
} T_FGC_LOG_DATA;                                /*                           */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_log                         /* FG-IC                     */
{                                                /*                           */
    T_FGC_LOG_DATA   fgc_log_data[ DFGC_LOG_MAX ];/*                          */
} T_FGC_LOG;                                     /*                           */

/*----------------------------------------------------------------------------*/
/* FuelGauge-IC                                                               */
/*----------------------------------------------------------------------------*/
typedef struct t_fgc_ctrl_tbl                    /* FuelGauge-IC              */
{
    T_FGC_LOG                fgc_log;            /*                           */
    unsigned char            fgc_rsv[32];        /*                           */
    T_FGC_SOH                fgc_soh;            /*                           */
    T_FGC_OCV                fgc_ocv;            /* OCV                       */
} T_FGC_CTRL_TBL;

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
extern int pfgc_fgc_mstate( unsigned long type , void * data );
extern unsigned short pfgc_fgc_volt_send(void);
/*<3130560> extern unsigned char gfgc_fgic_mode; */
                                                 /*         DD    R/W         */

#endif                                           /* _rh_fgc_h                 */

