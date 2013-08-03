/******************************************************************************/
/*                  :                                                         */
/*                  :  laputa-fixaddr.h                                       */
/*                  :  2010.10.26                                             */
/*                  :                                                         */
/*                  :  SDRAM                                                  */
/*                                                                            */
/*         Unpublished Copyright(c)  2010          NTT      MSE               */
/******************************************************************************/
/****** Laputa               **************************************************/
/* SDRAM      -------                                     10.10.26  MSE       */
/*            -------  USB                                11.01.27  MSE       */
/*            -------                                     11.02.14  MSE       */
/*            -------      DD                             11.02.23  MSE       */
/******************************************************************************/
/****** Totoro               **************************************************/
/* SDRAM      -------          (Totoro1.1  i    )         11.05.17  MSE       */
/*            -------      NAND                           11.05.31  MSE       */
/*            -------  SMC                                11.06.08  MSE       */
/*            -------  kdump                              11.06.22  MSE       */
/*            -------  FOTA                               11.08.02  MSE       */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _LAPUTA_FIXADDR_H_                              /* _LAPUTA_FIXADDR_H_ */
#define _LAPUTA_FIXADDR_H_

/******************************************************************************/
/*  Size (dec)                                                                */
/******************************************************************************/
#define DMEM_SCHED_HISTORY_SIZE ( 0x200000 )
#define DMEM_SMC_BKEXTSTORAGE_SIZE ( 0x300000 )
#define DMEM_CFG_SIZE		( 0x40000 )
#define DMEM_DEV1_SIZE		( 0x8000  )
#define DMEM_DEV2_SIZE		( 0x4000  )
#define DMEM_DEV3_SIZE		( 0x4000  )
#define DMEM_RAM_CONSOLE_SIZE   ( 0x20000 )
#define DMEM_SDRAM_BBT_SIZE	( 0x3000  )
#define DMEM_CRASH_ELF32_HEADER_SIZE ( 0x2000 )
#define DMEM_UKDUMP_KEY_AREA_SIZE ( 0x20 )
#define DMEM_PANIC_FLAG_SIZE ( 0x20 )

/* ---                ------------------------------------------------------- */
/* ---                      ------------------------------------------------- */
#define DMEM_UBT_VER_SIZE                            (  4 )
#define DMEM_TOK_VALUE_SIZE                          (  4 )
#define DMEM_RESET_INFO_SIZE                         (  4 )
#define DMEM_ALLRST_FLG_SIZE                         (  4 )
#define DMEM_CSV_ERR_FLG_SIZE                        (  4 )
#define DMEM_DUMMY_POW_STATUS_SIZE                   (  4 )
#define DMEM_OCV_COMP_FLG_SIZE                       (  4 )
#define DMEM_FOTA_FLG_SIZE                           (  4 )
#define DMEM_LK_VERSION_SIZE                         (  8 )
#define DMEM_CHG_TOTAL_TIMER_SIZE                    (  4 )
#define DMEM_CHG_MAX_VOLTAGE_ADJUST_SIZE             (  4 )
#define DMEM_CHG_RESUME_VOLTAGE_SIZE                 (  4 )
#define DMEM_CHG_TERM_CURRENT_SIZE                   (  4 )
#define DMEM_CHG_COOL_TEMP_ADJUST_SIZE               (  4 )
#define DMEM_CHG_WARM_TEMP_ADJUST_SIZE               (  4 )
#define DMEM_CHG_TEMP_CHECK_PERIOD_SIZE              (  4 )
#define DMEM_CHG_MAX_BAT_CHG_CURRENT_SIZE            (  4 )
#define DMEM_CHG_COOL_BAT_CHG_CURRENT_SIZE           (  4 )
#define DMEM_CHG_WARM_BAT_CHG_CURRENT_SIZE           (  4 )
#define DMEM_CHG_COOL_BAT_CHG_VOLTAGE_SIZE           (  4 )
#define DMEM_CHG_WARM_BAT_CHG_VOLTAGE_SIZE           (  4 )
#define DMEM_CHG_COOL_TEMP_DELTA_SIZE                (  4 )
#define DMEM_CHG_WARM_TEMP_DELTA_SIZE                (  4 )
#define DMEM_CHG_READ_ERR_SIZE                       (  4 )
#define DMEM_RTC_TIME_ADJUST_SIZE                    (  4 )
#define DMEM_CHG_AD_PCB_VERSION_SIZE                 (  4 )
#define DMEM_NO_LVA_FLG_SIZE                         (  4 )
#define DMEM_HAP_RATED_VOL_SIZE                      (  4 )
#define DMEM_HAP_CLAMP_VOL_SIZE                      (  4 )
#define DMEM_HAP_LOSS_COMPENSATION_SIZE              (  4 )
#define DMEM_HAP_BACK_EMF_SIZE                       (  4 )
#define DMEM_HAP_FEEDBACK_CNT_SIZE                   (  4 )
#define DMEM_HAP_CONTROL_1_SIZE                      (  4 )
#define DMEM_HAP_CONTROL_2_SIZE                      (  4 )
#define DMEM_HAP_CONTROL_3_SIZE                      (  4 )
#define DMEM_COMMON_MMU_TBL_SIZE                     (16384)

/* --- USB         ---------------------------------------------------------- */
#define DMEM_USBID_SIZE                              ( 48 )
#define DMEM_USB45TRIM_SIZE                          (  1 )
#define DMEM_TLLC_DEVICE_SIZE                        ( 18 )
#define DMEM_USB_DEVICE_DESCRIPTOR_SIZE              ( 18 )
#define DMEM_TLLC_STRING1_SIZE                       ( 84 )
#define DMEM_TLLC_STRING2_SIZE                       ( 32 )
#define DMEM_MASS_INQ_VENDORID_SIZE                  (  8 )
#define DMEM_MASS_INQ_PRODUCTID_SIZE                 ( 16 )
#define DMEM_USB_TETHERING_DESCRIPTOR_SIZE           ( 18 )
#define DMEM_USB_TETHERING_DESCRIPTOR_WITH_ADB_SIZE  ( 18 )
#define DMEM_E_ID_OFFSET1_SIZE                       (  1 )
#define DMEM_E_ID_OFFSET2_SIZE                       (  1 )
#define DMEM_E_ID_OFFSET3_SIZE                       (  1 )

/* --- etc ------------------------------------------------- */
#define DMEM_FTM_MODE_SIZE                           (  4 )

/* --- QUADからの追加-端末ログ収集------------------------------------------- */
#define DMEM_LAST_LOGCAT_MAIN_SIZE               ( 262144 )
#define DMEM_LAST_LOGCAT_EVENTS_SIZE             ( 262144 )
#define DMEM_LAST_LOGCAT_RADIO_SIZE              ( 262144 )
#define DMEM_LAST_LOGCAT_SYSTEM_SIZE             ( 262144 )
#define DMEM_LAST_LOGCAT_PMC_SIZE                ( 262144 )
#define DMEM_BELOG_APL_HIST_SIZE                 ( 131072 )
#define DMEM_BELOG_DEV_HIST_SIZE                 ( 262144 )


/******************************************************************************/
/*  Address (hex)                                                             */
/******************************************************************************/
/* --- START         -------------------------------------------------------- */
#define DMEM_SCHED_HISTORY_START   (0x90400000)
#define DMEM_SMC_BKEXTSTORAGE      (0xBFC00000)
#define DMEM_RAM_CONSOLE_START     (0x90140000)
#define DMEM_SDRAM_BBT_START    (DMEM_RAM_CONSOLE_START + DMEM_RAM_CONSOLE_SIZE)
#define DMEM_CRASH_ELF32_HEADER    (DMEM_SDRAM_BBT_START + DMEM_SDRAM_BBT_SIZE)
#define DMEM_UKDUMP_KEY_AREA (DMEM_CRASH_ELF32_HEADER + DMEM_CRASH_ELF32_HEADER_SIZE)
#define DMEM_PANIC_FLAG    (DMEM_UKDUMP_KEY_AREA + DMEM_UKDUMP_KEY_AREA_SIZE)
#define DMEM_CFG_START     (0x90180000)
#define DMEM_DEV1_START    (DMEM_CFG_START  + DMEM_CFG_SIZE)
#define DMEM_DEV2_START    (DMEM_DEV1_START + DMEM_DEV1_SIZE)
#define DMEM_DEV3_START    (DMEM_DEV2_START + DMEM_DEV2_SIZE)
#define DMEM_DEV4_START    (0x90200000)
/* --- USB         ---------------------------------------------------------- */
#define DMEM_USBID_BASE             (DMEM_CFG_START + 16)                        /* USB ID                */
#define DMEM_USB45TRIM              (DMEM_USBID_BASE + DMEM_USBID_SIZE)          /* USB Impedance         */
#define DMEM_TLLC_DEVICE            (DMEM_USB45TRIM + DMEM_USB45TRIM_SIZE + 15)  /* TLLC                  */
#define DMEM_USB_DEVICE_DESCRIPTOR  (DMEM_TLLC_DEVICE + DMEM_TLLC_DEVICE_SIZE + 14) /* USB Device Descriptor */
#define DMEM_TLLC_STRING1           (DMEM_USB_DEVICE_DESCRIPTOR + DMEM_USB_DEVICE_DESCRIPTOR_SIZE + 14) /* TLLC String */
#define DMEM_TLLC_STRING2           (DMEM_TLLC_STRING1 + DMEM_TLLC_STRING1_SIZE + 12) /* TLLC String */
#define DMEM_MASS_INQ_VENDORID      (DMEM_TLLC_STRING2 + DMEM_TLLC_STRING2_SIZE) /* Vendor ID */
#define DMEM_MASS_INQ_PRODUCTID     (DMEM_MASS_INQ_VENDORID + DMEM_MASS_INQ_VENDORID_SIZE + 8) /* Product ID */
#define DMEM_USB_TETHERING_DESCRIPTOR           (DMEM_MASS_INQ_PRODUCTID + DMEM_MASS_INQ_PRODUCTID_SIZE)
#define DMEM_USB_TETHERING_DESCRIPTOR_WITH_ADB  (DMEM_USB_TETHERING_DESCRIPTOR + DMEM_USB_TETHERING_DESCRIPTOR_SIZE + 14)
#define DMEM_E_ID_OFFSET1                       (DMEM_USB_TETHERING_DESCRIPTOR_WITH_ADB + DMEM_USB_TETHERING_DESCRIPTOR_WITH_ADB_SIZE + 14)
#define DMEM_E_ID_OFFSET2                       (DMEM_E_ID_OFFSET1 + DMEM_E_ID_OFFSET1_SIZE )
#define DMEM_E_ID_OFFSET3                       (DMEM_E_ID_OFFSET2 + DMEM_E_ID_OFFSET2_SIZE )

/* --- etc ------------------------------------------------- */
#define DMEM_FTM_MODE_STATUS                    (0x90180180)

/* ---                      ------------------------------------------------- */
#define DMEM_UBT_VER                (DMEM_DEV2_START)                            /* u-Boot Version        */
#define DMEM_TOK_VALUE              (DMEM_UBT_VER + DMEM_UBT_VER_SIZE)           /* TOK                   */
#define DMEM_RESET_INFO             (DMEM_TOK_VALUE + DMEM_TOK_VALUE_SIZE)       /* RESET_INFO            */
#define DMEM_ALLRST_FLG             (DMEM_RESET_INFO + DMEM_RESET_INFO_SIZE)     /* RESET_INFO            */
#define DMEM_CSV_ERR_FLG            (DMEM_ALLRST_FLG + DMEM_ALLRST_FLG_SIZE)     /* CSV_ERR               */
#define DMEM_DUMMY_POW_STATUS       (DMEM_CSV_ERR_FLG + DMEM_CSV_ERR_FLG_SIZE)   /* DUMMY_POWEROFF_STATUS */
                                                                                 /* OCV_measurement_Complete_Flag */ 
#define DMEM_OCV_COMP_FLG           (DMEM_DUMMY_POW_STATUS + DMEM_DUMMY_POW_STATUS_SIZE)
#define DMEM_FOTA_FLG               (DMEM_OCV_COMP_FLG + DMEM_OCV_COMP_FLG_SIZE) /* FOTA_FLG              */
#define DMEM_LK_VERSION             (DMEM_FOTA_FLG + DMEM_FOTA_FLG_SIZE)         /* LK_VERSION            */
#define DMEM_CHG_TOTAL_TIMER           (DMEM_LK_VERSION + DMEM_LK_VERSION_SIZE)
#define DMEM_CHG_MAX_VOLTAGE_ADJUST    (DMEM_CHG_TOTAL_TIMER + DMEM_CHG_TOTAL_TIMER_SIZE)
#define DMEM_CHG_RESUME_VOLTAGE        (DMEM_CHG_MAX_VOLTAGE_ADJUST + DMEM_CHG_MAX_VOLTAGE_ADJUST_SIZE) 
#define DMEM_CHG_TERM_CURRENT          (DMEM_CHG_RESUME_VOLTAGE + DMEM_CHG_RESUME_VOLTAGE_SIZE)
#define DMEM_CHG_COOL_TEMP_ADJUST      (DMEM_CHG_TERM_CURRENT + DMEM_CHG_TERM_CURRENT_SIZE)
#define DMEM_CHG_WARM_TEMP_ADJUST      (DMEM_CHG_COOL_TEMP_ADJUST + DMEM_CHG_COOL_TEMP_ADJUST_SIZE)
#define DMEM_CHG_TEMP_CHECK_PERIOD     (DMEM_CHG_WARM_TEMP_ADJUST + DMEM_CHG_WARM_TEMP_ADJUST_SIZE)
#define DMEM_CHG_MAX_BAT_CHG_CURRENT   (DMEM_CHG_TEMP_CHECK_PERIOD + DMEM_CHG_TEMP_CHECK_PERIOD_SIZE)
#define DMEM_CHG_COOL_BAT_CHG_CURRENT  (DMEM_CHG_MAX_BAT_CHG_CURRENT + DMEM_CHG_MAX_BAT_CHG_CURRENT_SIZE)
#define DMEM_CHG_WARM_BAT_CHG_CURRENT  (DMEM_CHG_COOL_BAT_CHG_CURRENT + DMEM_CHG_COOL_BAT_CHG_CURRENT_SIZE)
#define DMEM_CHG_COOL_BAT_CHG_VOLTAGE  (DMEM_CHG_WARM_BAT_CHG_CURRENT + DMEM_CHG_WARM_BAT_CHG_CURRENT_SIZE)
#define DMEM_CHG_WARM_BAT_CHG_VOLTAGE  (DMEM_CHG_COOL_BAT_CHG_VOLTAGE + DMEM_CHG_COOL_BAT_CHG_VOLTAGE_SIZE)
#define DMEM_CHG_COOL_TEMP_DELTA       (DMEM_CHG_WARM_BAT_CHG_VOLTAGE + DMEM_CHG_WARM_BAT_CHG_VOLTAGE_SIZE)
#define DMEM_CHG_WARM_TEMP_DELTA       (DMEM_CHG_COOL_TEMP_DELTA + DMEM_CHG_COOL_TEMP_DELTA_SIZE)
#define DMEM_CHG_READ_ERR              (DMEM_CHG_WARM_TEMP_DELTA + DMEM_CHG_COOL_TEMP_DELTA_SIZE)
#define DMEM_RTC_TIME_ADJUST           (DMEM_CHG_READ_ERR + DMEM_CHG_READ_ERR_SIZE)
#define DMEM_CHG_AD_PCB_VERSION        (DMEM_RTC_TIME_ADJUST + DMEM_RTC_TIME_ADJUST_SIZE)
#define DMEM_NO_LVA_FLG                (DMEM_CHG_AD_PCB_VERSION + DMEM_CHG_AD_PCB_VERSION_SIZE)
#define DMEM_HAP_RATED_VOL             (DMEM_NO_LVA_FLG + DMEM_NO_LVA_FLG_SIZE)
#define DMEM_HAP_CLAMP_VOL             (DMEM_HAP_RATED_VOL + DMEM_HAP_RATED_VOL_SIZE)
#define DMEM_HAP_LOSS_COMPENSATION     (DMEM_HAP_CLAMP_VOL + DMEM_HAP_CLAMP_VOL_SIZE)
#define DMEM_HAP_BACK_EMF              (DMEM_HAP_LOSS_COMPENSATION + DMEM_HAP_LOSS_COMPENSATION_SIZE)
#define DMEM_HAP_FEEDBACK_CNT          (DMEM_HAP_BACK_EMF + DMEM_HAP_BACK_EMF_SIZE)
#define DMEM_HAP_CONTROL_1             (DMEM_HAP_FEEDBACK_CNT + DMEM_HAP_FEEDBACK_CNT_SIZE)
#define DMEM_HAP_CONTROL_2             (DMEM_HAP_CONTROL_1 + DMEM_HAP_CONTROL_1_SIZE)
#define DMEM_HAP_CONTROL_3             (DMEM_HAP_CONTROL_2 + DMEM_HAP_CONTROL_2_SIZE)
#define DMEM_COMMON_MMU_TBL         (DMEM_DEV3_START)                            /* COMMON_MMU_TBL        */

/* ---------------------------------------------- */
#define DMEM_LAST_LOGCAT_MAIN_START         ( DMEM_DEV4_START               )                                    /* last_logcat main バッファ              */
#define DMEM_LAST_LOGCAT_EVENTS_START       ( DMEM_LAST_LOGCAT_MAIN_START   ) + ( DMEM_LAST_LOGCAT_MAIN_SIZE   ) /* last_logcat events バッファ            */
#define DMEM_LAST_LOGCAT_RADIO_START        ( DMEM_LAST_LOGCAT_EVENTS_START ) + ( DMEM_LAST_LOGCAT_EVENTS_SIZE ) /* last_logcat radio バッファ             */
#define DMEM_LAST_LOGCAT_SYSTEM_START       ( DMEM_LAST_LOGCAT_RADIO_START  ) + ( DMEM_LAST_LOGCAT_RADIO_SIZE  ) /* last_logcat system バッファ            */
#define DMEM_LAST_LOGCAT_PMC_START          ( DMEM_LAST_LOGCAT_SYSTEM_START ) + ( DMEM_LAST_LOGCAT_SYSTEM_SIZE ) /* last_logcat pmc バッファ               */
#define DMEM_BELOG_APL_HIST_START           ( DMEM_LAST_LOGCAT_PMC_START    ) + ( DMEM_LAST_LOGCAT_PMC_SIZE    ) /* 操作ログ アプリ起動履歴用バッファ      */
#define DMEM_BELOG_DEV_HIST_START           ( DMEM_BELOG_APL_HIST_START     ) + ( DMEM_BELOG_APL_HIST_SIZE     ) /* 操作ログ タッチ,キー操作履歴用バッファ */


#define DMEM_UBT_VER_OFFSET                 ( ( DMEM_UBT_VER - DMEM_DEV2_START ) / 4 )
#define DMEM_TOK_VALUE_OFFSET               ( ( DMEM_TOK_VALUE - DMEM_DEV2_START ) / 4 )
#define DMEM_RESET_INFO_OFFSET              ( ( DMEM_RESET_INFO - DMEM_DEV2_START ) / 4 )
#define DMEM_ALLRST_FLG_OFFSET              ( ( DMEM_ALLRST_FLG - DMEM_DEV2_START ) / 4 )
#define DMEM_CSV_ERR_FLG_OFFSET             ( ( DMEM_CSV_ERR_FLG - DMEM_DEV2_START ) / 4 )
#define DMEM_DUMMY_POW_STATUS_OFFSET        ( ( DMEM_DUMMY_POW_STATUS - DMEM_DEV2_START ) / 4 )
#define DMEM_OCV_COMP_FLG_OFFSET            ( ( DMEM_OCV_COMP_FLG - DMEM_DEV2_START ) / 4 )
#define DMEM_FOTA_FLG_OFFSET                ( ( DMEM_FOTA_FLG - DMEM_DEV2_START ) / 4 )

#define DMEM_CHG_TOTAL_TIMER_OFFSET                ( ( DMEM_CHG_TOTAL_TIMER - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_MAX_VOLTAGE_ADJUST_OFFSET         ( ( DMEM_CHG_MAX_VOLTAGE_ADJUST - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_RESUME_VOLTAGE_OFFSET             ( ( DMEM_CHG_RESUME_VOLTAGE - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_TERM_CURRENT_OFFSET               ( ( DMEM_CHG_TERM_CURRENT - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_COOL_TEMP_ADJUST_OFFSET           ( ( DMEM_CHG_COOL_TEMP_ADJUST - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_WARM_TEMP_ADJUST_OFFSET           ( ( DMEM_CHG_WARM_TEMP_ADJUST - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_TEMP_CHECK_PERIOD_OFFSET          ( ( DMEM_CHG_TEMP_CHECK_PERIOD - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_MAX_BAT_CHG_CURRENT_OFFSET        ( ( DMEM_CHG_MAX_BAT_CHG_CURRENT - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_COOL_BAT_CHG_CURRENT_OFFSET       ( ( DMEM_CHG_COOL_BAT_CHG_CURRENT - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_WARM_BAT_CHG_CURRENT_OFFSET       ( ( DMEM_CHG_WARM_BAT_CHG_CURRENT - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_COOL_BAT_CHG_VOLTAGE_OFFSET       ( ( DMEM_CHG_COOL_BAT_CHG_VOLTAGE - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_WARM_BAT_CHG_VOLTAGE_OFFSET       ( ( DMEM_CHG_WARM_BAT_CHG_VOLTAGE - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_COOL_TEMP_DELTA_OFFSET            ( ( DMEM_CHG_COOL_TEMP_DELTA - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_WARM_TEMP_DELTA_OFFSET            ( ( DMEM_CHG_WARM_TEMP_DELTA - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_READ_ERR_OFFSET                   ( ( DMEM_CHG_READ_ERR - DMEM_DEV2_START ) / 4 )
#define DMEM_RTC_TIME_ADJUST_OFFSET                ( ( DMEM_RTC_TIME_ADJUST - DMEM_DEV2_START ) / 4 )
#define DMEM_CHG_AD_PCB_VERSION_OFFSET             ( ( DMEM_CHG_AD_PCB_VERSION - DMEM_DEV2_START ) / 4 )
#define DMEM_NO_LVA_FLG_OFFSET                     ( ( DMEM_NO_LVA_FLG - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_RATED_VOL_OFFSET                  ( ( DMEM_HAP_RATED_VOL - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_CLAMP_VOL_OFFSET                  ( ( DMEM_HAP_CLAMP_VOL - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_LOSS_COMPENSATION_OFFSET          ( ( DMEM_HAP_LOSS_COMPENSATION - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_BACK_EMF_OFFSET                   ( ( DMEM_HAP_BACK_EMF - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_FEEDBACK_CNT_OFFSET               ( ( DMEM_HAP_FEEDBACK_CNT - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_CONTROL_1_OFFSET                  ( ( DMEM_HAP_CONTROL_1 - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_CONTROL_2_OFFSET                  ( ( DMEM_HAP_CONTROL_2 - DMEM_DEV2_START ) / 4 )
#define DMEM_HAP_CONTROL_3_OFFSET                  ( ( DMEM_HAP_CONTROL_3 - DMEM_DEV2_START ) / 4 )
#endif                                               /* _LAPUTA_FIXADDR_H_    */
/******************************************************************************/
/*         Unpublished Copyright(c)  2010          NTT      MSE               */
/******************************************************************************/
