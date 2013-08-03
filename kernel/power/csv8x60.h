#ifndef __ARCH_ARM_MACH_MSM_CSV8x60_H
#define __ARCH_ARM_MACH_MSM_CSV8x60_H

/*----------------------------------------------------------------------------*/
/* Include                                                                    */
/*----------------------------------------------------------------------------*/
#include <asm-generic/int-ll64.h>
#include <linux/time.h>

/*----------------------------------------------------------------------------*/
/* Define                                                                     */
/*----------------------------------------------------------------------------*/
#define SAFETY_QUAD  /* Rupy add */

#define CLK_ON                     1
#define CLK_OFF                    0

#ifdef SAFETY_QUAD /* Rupy mod Start */
/* APQ8064 GPIO Port No */
#define APQ_GPIONO1   1
#define APQ_GPIONO2   2
#define APQ_GPIONO86  86
#else /* SAFETY_QUAD */
/* MSM8960 GPIO Port No */
#define MSM_GPIONO1			1
#define MSM_GPIONO15		15
#define MSM_GPIONO68		68
#endif /* SAFETY_QUAD *//* Rupy mod end */

/* PM8921 GPIO Port No */
#ifdef SAFETY_QUAD /* Rupy mod Start */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)  (pm_gpio - 1 + PM8921_GPIO_BASE)
#define PMIC_GPIONO1  		PM8921_GPIO_PM_TO_SYS(1)
#define PMIC_GPIONO16		PM8921_GPIO_PM_TO_SYS(16)
#define PMIC_GPIONO18		PM8921_GPIO_PM_TO_SYS(18)
#define PMIC_GPIONO20  		PM8921_GPIO_PM_TO_SYS(20)
#define PMIC_GPIONO37  		PM8921_GPIO_PM_TO_SYS(37)
#else /*SAFETY_QUAD*/
#if 1 /* npdc300020773 */
#define PMIC_GPIONOBASE		1
#define PMIC_GPIONO16		16 - PMIC_GPIONOBASE
#define PMIC_GPIONO18		18 - PMIC_GPIONOBASE
#define PMIC_GPIONO25		25 - PMIC_GPIONOBASE
#else
#define PMIC_GPIONOBASE		151
#define PMIC_GPIONO16		16 + PMIC_GPIONOBASE
#define PMIC_GPIONO18		18 + PMIC_GPIONOBASE
#define PMIC_GPIONO25		25 + PMIC_GPIONOBASE
#endif
#endif /*SAFETY_QUAD*//* Rupy mod end */

/* Device Chip Addr */
#define SPIDER3_CHIP_ADDR	0x08
#define TOUCHPANEL_CHIP_ADDR	0x20
#ifdef    CONFIG_MMTUNER_DRIVER   /* Jessie add start */
#define AN30183A_CHIP_ADDR	0x6E
#endif /* CONFIG_MMTUNER_DRIVER *//* Jessie add end */

/* Spider3 Reg Addr */
#define SPIDER3_CURRENT_SET3	0xF2

/* TouchPanel Reg Addr */
#define TOUCHPANEL_DEVICE_BASE		0xE5
#define TOUCHPANEL_DEVICE_OFFSET	0x00

/* PM8921 Reg Addr */
#if  0 /* Rupy Del start */
#define VIB_DRV			0x4A
#endif /* Rupy del end */
#define SSBI_REG_ADDR_LPG_BANK_EN	0x144
/* Rupy Add start */
//#define SSBI_REG_ADDR_VREG_L23_18_LCD		0xDA
//#define SSBI_REG_ADDR_VREG_L22_28_TP		0xD8
#define SSBI_REG_ADDR_VREG_LVS5_18			0x68
#define SSBI_REG_ADDR_VREG_L11_27			0xC2
#define SSBI_REG_ADDR_VREG_L16_27			0xCC
#define SSBI_REG_ADDR_VREG_L12_12			0xC4
/* Rupy Add end */

#ifdef    CONFIG_MMTUNER_DRIVER    /* Jessie add start */
/* AN30183A Reg Addr */
#define AN30183A_CNT		0x00
#endif /* CONFIG_MMTUNER_DRIVER */ /* Jessie add end */

/* I2C CH */
#define I2C_1                      0
#define I2C_2                      1
#define I2C_3                      2
#define I2C_4                      3
#define I2C_NUM_IF                 4

/* NG Code */
#define CSV_INIT_ERR               -1
#define CSV_CLK_ERR                -2
#ifdef SAFETY_QUAD /* Rupy mod start */
#define LCD_DCDC_EN_NG						(1 << 0)
//#define LCD_VREG_L23_18_LCD_NG				(1 << 1)
#define BL_DCDC_EN_NG						(1 << 2)
//#define TOUCHPANEL_V28TP_NG					(1 << 3)
#define HAPTICS_EN_NG						(1 << 4)
#define LED_RED_NG							(1 << 5)
#define LED_GREEN_NG						(1 << 6)
#define LED_BLUE_NG							(1 << 7)
#define MICROSD_VMMC1_NG					(1 << 8)
#define CAMERA_V12CAM_EN_NG					(1 << 9)
#define CAMERA_VREG_LVS5_18_NG				(1 << 10)
#define CAMERA_VREG_L11_27_NG				(1 << 11)
#define CAMERA_V11CAM_EN_NG					(1 << 12)
#define CAMERA_VREG_L16_27_NG				(1 << 13)
#define CAMERA_VREG_L12_12_NG				(1 << 14)
#define CAMERA_nCAMISP_RST_NG				(1 << 15)
#define AUDIO_GPOOUT_NG						(1 << 16)
#define MMTUNER_V18DTV_NG					(1 << 17)
#define MMTUNER_V28DTV_NG					(1 << 18)
#define MMTUNER_V12DTV_NG					(1 << 19)
#define WLAN_GPIO_DATAOUT_NG				(1 << 20)
#define FELICA_GPOOUT_NG					(1 << 21)
#endif /* SAFETY_QUAD *//* Rupy mod end */
#ifdef SAFETY_JESSIE /* Rupy mod start */
#if  0  /* Jessie mod start */
#define LCD_VSP_NG				   (1 << 0)
#define LCD_VSN_NG       		   (1 << 1)
#define LCD_V28LCD_NG  			   (1 << 2)
#define LCD_V18LCD_NG          	   (1 << 3)
#define LCD_BL_ANODE_NG            (1 << 4)
#define TOUCHPANEL_V28TP_NG        (1 << 5)
#define VIBRATION_VIB_DRV_NG       (1 << 6)
#define LED_RED_NG     			   (1 << 7)
#define LED_GREEN_NG               (1 << 8)
#define MICROSD_VMMC1_NG           (1 << 9)
#define CAMERA_V12CAM_NG		   (1 << 10)
#define CAMERA_V18CAM_NG		   (1 << 11)
#define CAMERA_V28CAM_NG           (1 << 12)
#define CAMERA_V28CAMAF_NG         (1 << 13)
#define AUDIO_GPOOUT_NG     	   (1 << 14)
#define WLAN_GPIO_DATAOUT_NG       (1 << 15)
#define LIGHTPROX_COMMANDL_NG      (1 << 16)
#define FELICA_GPOOUT_NG           (1 << 17)
#define CHARGING_STATUS_NG         (1 << 18)
#else
#define LCD_VSP_NG                 (1 << 0)
#define LCD_VSN_NG                 (1 << 1)
#define LCD_BL_ANODE_NG            (1 << 2)
#define TOUCHPANEL_V28TP_NG        (1 << 3)
#define VIBRATION_VIB_DRV_NG       (1 << 4)
#define LED_RED_NG                 (1 << 5)
#define LED_GREEN_NG               (1 << 6)
#define LED_BLUE_NG                (1 << 7)
#define MICROSD_VMMC1_NG           (1 << 8)
#define CAMERA_V28CAM_NG           (1 << 9)
#define AUDIO_GPOOUT_NG            (1 << 10)
#define MMTUNER_V18DTV_NG          (1 << 11)
#define MMTUNER_V28DTV_NG          (1 << 12)
#define MMTUNER_V12DTV_NG          (1 << 13)
#define WLAN_GPIO_DATAOUT_NG       (1 << 14)
#define LIGHTPROX_COMMANDL_NG      (1 << 15)
#define FELICA_GPOOUT_NG           (1 << 16)
#define CHARGING_STATUS_NG         (1 << 17)
#endif /* Jessie mod end */
#endif /* SAFETY_JESSIE *//* Rupy mod start */
#define CSV_I2C_NG                 (1 << 31)

#define BIT_0                      (1 << 0)
#define BIT_1                      (1 << 1)
#define BIT_2                      (1 << 2)
#define BIT_3                      (1 << 3)
#define BIT_4                      (1 << 4)
#define BIT_5                      (1 << 5)
#define BIT_6                      (1 << 6)
#define BIT_7                      (1 << 7)
#define BIT_8                      (1 << 8)
#define BIT_19                     (1 << 19)
#define BIT_22                     (1 << 22)
#define BIT_27                     (1 << 27)
#define BIT_29                     (1 << 29)
#define ALL_BIT_CLR                0x00

/* IDPower */
#define CHK_VIBRATION_VIB_DRV_REG_BIT       (BIT_4 | BIT_5 | BIT_6 | BIT_7)
#define CHK_LED_RED_REG_BIT                 (BIT_5)
#define CHK_LED_GREEN_REG_BIT               (BIT_4)
/* Jessie add start */
#define CHK_LED_BLUE_REG_BIT                (BIT_6)
#define CHK_MMTUNER_V18DTV_REG_BIT          (BIT_0)
#define CHK_MMTUNER_V28DTV_REG_BIT          (BIT_3)
#define CHK_MMTUNER_V12DTV_REG_BIT          (BIT_4)
/* Jessie add end */
#define CHK_PM8921_REG_ENABLE_BIT           (BIT_7)	// Rupy add
/* Jessie mod start */
#ifdef    DEBUG_PT
#define CSV_LOG_FUNCIN(fmt, ...) \
	pr_info( "### CSV ### IN  %05d %s " fmt, __LINE__, __FUNCTION__, \
			  ##__VA_ARGS__ );
#define CSV_LOG_FUNCOUT(fmt, ...) \
	pr_info( "### CSV ### OUT %05d %s " fmt, __LINE__, __FUNCTION__, \
			  ##__VA_ARGS__);
#define DEBUG_CSV(X) pr_info X
#else
#define CSV_LOG_FUNCIN(fmt, ...)
#define CSV_LOG_FUNCOUT(fmt, ...)
#define DEBUG_CSV(X) pr_debug X
#endif /* DEBUG_PT */
/* Jessie mod end */
/*----------------------------------------------------------------------------*/
/* Struct                                                                     */
/*----------------------------------------------------------------------------*/
struct current_chk_dev_list
{    
#ifdef SAFETY_QUAD /* Rupy mod start */
		u8  lcd_dcdc_en;
//		u8  lcd_v18lcd;
		u8  bl_dcdc_en;
//		u8  touchpanel_v28tp;
		u8  haptics_en;
		u8  led;
		u8  microSD_vmmc1;
		u8  v12cam_en;
		u8  vreg_lvs5_18;
		u8  vreg_l11_27;
		u8  v11cam_en;
		u8  vreg_l16_27;
		u8  vreg_l12_12;
		u8  ncamisp_rst;
		u8  audio_gpoout;
#ifdef    CONFIG_MMTUNER_DRIVER
		u8  mmtuner_reg;
#endif /* CONFIG_MMTUNER_DRIVER */
		u8  wlan_vreg;
		u8  felica_gpoout;
#endif /* SAFETY_QUAD *//* Rupy mod end */
#ifdef SAFETY_JESSIE /* Rupy mod start */
        u8  lcd_vsp;
	    u8  lcd_vsn;
		u8  lcd_v28lcd;
		u8  lcd_v18lcd;
		u8  lcd_bl_anode;
		u8  touchpanel_v28tp;
        u8  vibration_vib_drv;
#if 0  /* Jessie mod start */
		u8  led_red;
		u8  led_green;
#else
        u8  led;
#endif /* Jessie mod end */
        u8  microSD_vmmc1;
#if  0 /* Jessie mod start */
        u8  camera_v12cam;
        u8  camera_v18cam;
        u8  camera_v28cam;
        u8  camera_v28camaf;
#else
        u8  camera_v28cam;
#endif	/* Jessie mod end */
#ifdef    CONFIG_MMTUNER_DRIVER   /* Jessie add start */
        u8  mmtuner_reg;
#endif /* CONFIG_MMTUNER_DRIVER *//* Jessie add end */
        u8  audio_gpoout;
        u8  wlan_vreg;
        u8  lightprox_commandl;
        u8  felica_gpoout;
        u8  charging_status;
#endif /* SAFETY_JESSIE *//* Rupy mod end */
};

/*----------------------------------------------------------------------------*/
/* Function Prototype                                                         */
/*----------------------------------------------------------------------------*/
int csv_enable_soft_mode2(void);
int csv_disable_soft_mode2(void);
#endif
