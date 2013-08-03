/*
 * @file felica_ctrl_local.h
 * @brief Local header file of FeliCa control driver
 *
 * @date 2010/12/17
 */

#ifndef __DV_FELICA_CTRL_LOCAL_H__
#define __DV_FELICA_CTRL_LOCAL_H__

/*
 * include extra header
 */
#include <linux/sched.h>            /* jiffies    */

#include <linux/cfgdrv.h>           /* cfgdrvier  */
// <iDPower_002> mod S
//#include <linux/gpio_ex.h>          /* gpio       */
#include <linux/gpio.h>             /* gpio       */
// <iDPower_002> mod E
// <iDPower_002> del S
//#include <linux/i2c/i2c-subpmic.h>  /* i2c submic */
// <iDPower_002> del E
#include "../../../arch/arm/mach-msm/board-8064.h"

// #define FELICA_SYS_HAVE_DETECT_RFS
#include <mach/irqs.h>
/*
 * BIT defined
 */
#define FELICA_POS_BIT_0          (0x00000001U)
#define FELICA_POS_BIT_1          (0x00000002U)
#define FELICA_POS_BIT_2          (0x00000004U)
#define FELICA_POS_BIT_3          (0x00000008U)
#define FELICA_POS_BIT_4          (0x00000010U)
#define FELICA_POS_BIT_5          (0x00000020U)
#define FELICA_POS_BIT_6          (0x00000040U)

/*
 * Identification information of FeliCa device drivers
 */
#define FELICA_DEV_NAME             "felica"            /* Device Name        */
#define FELICA_DEV_NAME_CTRL_PON    "felica_pon"        /* PON Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_CEN    "felica_cen"        /* CEN Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_RFS    "felica_rfs"        /* RFS Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_ITR    "felica_interrupt"  /* ITR Terminal Ctrl  */
#define FELICA_DEV_NAME_CTRL_DEBUG  "felica_ctrl_dbg"   /* Ctrl debug         */
#define FELICA_DEV_NAME_COMM        "felica"            /* Commnunication     */
#define FELICA_DEV_NAME_COMM_DEBUG  "felica_comm_dbg"   /* Comm deubg         */
#define FELICA_DEV_NAME_RWS         "felica_rws"        /* RWS                */
#define FELICA_DEV_NAME_CFG         "felica_cfg"        /* CFG                */
// <Combo_chip> add S
#define SNFC_DEV_NAME_CTRL_RFS      "snfc_rfs"        /* RFS Terminal Ctrl  */
#define SNFC_DEV_NAME_CTRL_INTU     "snfc_intu_poll"    /* INTU Terminal Ctrl */
#define SNFC_DEV_NAME_CTRL_CEN      "snfc_cen"          /* CEN Terminal Ctrl  */
#define SNFC_DEV_NAME_CTRL_AVAILABLE "snfc_available_poll"/* AVAILABLE Terminal Ctrl */
#define SNFC_DEV_NAME_CTRL_HSEL     "snfc_hsel"         /* HSEL Terminal Ctrl */
#define SNFC_DEV_NAME_CTRL_UARTCC   "snfc_uartcc"       /* UARTCC Terminal Ctrl  */
#define SNFC_DEV_NAME_CTRL_PON      "snfc_pon"          /* PON Terminal Ctrl  */
// <Combo_chip> add E

#define FELICA_DEV_NUM (1)
#define FELICA_DEV_ITR_MAX_OPEN_NUM (1)

/*
 * Major Minor number
 */
#define FELICA_CTRL_MAJOR_NO  (123)            /* FeliCa control device       */
#define FELICA_COMM_MAJOR_NO  (124)            /* FeliCa communication device */
#define FELICA_RWS_MAJOR_NO   (125)            /* FeliCa rws device           */
#define FELICA_CFG_MAJOR_NO   (126)            /* FeliCa cfg device           */

#define FELICA_CTRL_MINOR_NO_PON   (0)                  /* PON Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_CEN   (1)                  /* CEN Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_RFS   (2)                  /* RFS Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_ITR   (3)                  /* ITR Terminal Ctrl  */
#define FELICA_CTRL_MINOR_NO_DEBUG (4)                  /* Ctrl debug         */
// <Combo_chip> add S
#define SNFC_CTRL_MINOR_NO_RFS     (5)                  /* RFS Terminal Ctrl  */
#define SNFC_CTRL_MINOR_NO_INTU    (6)                  /* INTU Terminal Ctrl */
#define SNFC_CTRL_MINOR_NO_CEN     (7)                  /* CEN Terminal Ctrl  */
#define SNFC_CTRL_MINOR_NO_AVAILABLE (8)            /* AVAILABLE Terminal Ctrl*/
#define SNFC_CTRL_MINOR_NO_HSEL    (9)                  /* HSEL Terminal Ctrl */
#define SNFC_CTRL_MINOR_NO_AUTOPOLL (10)                /* POLL Terminal Ctrl */
#define SNFC_CTRL_MINOR_NO_PON     (11)                 /* PON Terminal Ctrl  */
// <Combo_chip> add E

#define FELICA_COMM_MINOR_NO_COMM  (0)                  /* Commnunication     */
#define FELICA_COMM_MINOR_NO_DEBUG (1)                  /* Comm deubg         */

#define FELICA_RWS_MINOR_NO_RWS    (0)                  /* RWS                */

#define FELICA_CFG_MINOR_NO_CFG    (0)                  /* CFG                */
/*
 * Interrupts device name
 */
#define FELICA_CTRL_ITR_STR_RFS  "felica_ctrl_rfs"           /* RFS interrupt */
#define FELICA_CTRL_ITR_STR_INT  "felica_ctrl_int"           /* INT interrupt */
// <Combo_chip> add S
#define SNFC_CTRL_ITR_STR_INTU   "snfc_ctrl_intu"            /* INTU interrupt*/
// <Combo_chip> add E

/*
 * GPIO paramater
 */
// <iDPower_002> mod S
//#define FELICA_GPIO_PORT_RFS   GPIO_PORT_OMAP_178     /* RFS GPIO port number */
//#define FELICA_GPIO_PORT_INT   GPIO_PORT_OMAP_177     /* INT GPIO port number */
//#define FELICA_GPIO_PORT_PON   GPO_PORT_SUBPMIC_2     /* PON GPIO port number */

#define FELICA_GPIO_PORT_RFS   (38)// 0919 (43)                   /* RFS GPIO port number */
#define FELICA_GPIO_PORT_INT   (36)// 0919 (106)                  /* INT GPIO port number */
#define FELICA_GPIO_PORT_PON   (PM8921_GPIO_PM_TO_SYS(1)) // 0919 (68)                   /* PON GPIO port number */

//#define FELICA_GPIO_PORT_RFS   (43)                   /* RFS GPIO port number */
//#define FELICA_GPIO_PORT_INT   (106)                  /* INT GPIO port number */
//#define FELICA_GPIO_PORT_PON   (68)                   /* PON GPIO port number */
// <iDPower_002> mod E
// <Combo_chip> add S
#define SNFC_GPIO_PORT_HSEL  (PM8921_GPIO_PM_TO_SYS(7))/* INTU GPIO port number*/
#define SNFC_GPIO_PORT_INTU  (29)                    /* INTU GPIO port number */
// <Combo_chip> add E

#define FELICA_GPIO_STR_RFS    "felica_rfs"                  /* RFS labe name */
#define FELICA_GPIO_STR_INT    "felica_int"                  /* INT labe name */
#define FELICA_GPIO_STR_PON    "felica_pon"                  /* PON labe name */
// <Combo_chip> add S
#define SNFC_GPIO_STR_HSEL     "snfc_hsel"                   /* HSEL label name*/
#define SNFC_GPIO_STR_INTU     "snfc_intu"                   /* INTU label name*/
// <Combo_chip> add E

/*
 * SPI submic paramater
 */
// <iDPower_002> del S
//#define FELICA_SUBPMIC_PORT_CEN   SUBPMIC_FCCEN        /* CEN address         */
// <iDPower_002> del E
#define FELICA_SUBPMIC_FCCEN      FELICA_POS_BIT_0     /* Register bit of CEN */

// <iDPower_002> add S
/*
 * I2C parameter
 */
#define FELICA_I2C_ENABLE_WRITE  (0x0002)
#define FELICA_I2C_DISABLE_WRITE (0x0003)
#define FELICA_I2C_CHANGE_STATUS (0x0004)
#define FELICA_I2C_DATA_READ     (0x0005)
#define FELICA_I2C_DATA_WRITE    (0x0006)
#define FELICA_I2C_READ_MODE     (0x0000)
#define FELICA_I2C_WRITE_MODE    (0x0001)
// <iDPower_002> add E

/*
 * Timer
 */
#define FELICA_TIMER_CHATA             (1)
#define FELICA_TIMER_CHATA_DELAY       ((50 * HZ) / 1000 + 1)
#define FELICA_TIMER_CEN_TERMINAL_WAIT ((20 * HZ) / 1000 + 1)
#define FELICA_TIMER_PON_TERMINAL_WAIT ((30 * HZ) / 1000 + 1)
// <Combo_chip> add S
#define FELICA_TIMER_AVAILABLE_TERMINAL_WAIT ((500 * HZ) / 1000 + 1)
// <Combo_chip> add E

/*
 * The FeliCa terminal condition to acquire and to set for Internal.
 */
#define DFD_OUT_L    (0x00)                                    /* condition L */
#define DFD_OUT_H    (0x01)                                    /* condition H */

/*
 * Interrupt kinds
 */
enum{
	DFD_ITR_TYPE_RFS = 0,                               /* RFS Interrupt      */
	DFD_ITR_TYPE_INT,                                   /* INT Interrupt      */
// <Combo_chip> add S
	DFD_ITR_TYPE_INTU,                                  /* INTU Interrupt     */
// <Combo_chip> add E
	DFD_ITR_TYPE_NUM                                    /* Interrupt type num */
};

/*
 * Cleanup parameters
 */
enum{
	DFD_CLUP_NONE = 0,
	DFD_CLUP_UNREG_CDEV_PON,
	DFD_CLUP_CDEV_DEL_PON,
	DFD_CLUP_UNREG_CDEV_CEN,
	DFD_CLUP_CDEV_DEL_CEN,
	DFD_CLUP_UNREG_CDEV_RFS,
	DFD_CLUP_CDEV_DEL_RFS,
	DFD_CLUP_UNREG_CDEV_ITR,
	DFD_CLUP_CDEV_DEL_ITR,
// <Combo_chip> add S
    DFD_CLUP_UNREG_CDEV_INTU,
    DFD_CLUP_CDEV_DEL_INTU,
    DFD_CLUP_UNREG_CDEV_HSEL,
    DFD_CLUP_CDEV_DEL_HSEL,
    DFD_CLUP_UNREG_CDEV_POLL,
    DFD_CLUP_CDEV_DEL_POLL,
    DFD_CLUP_UNREG_CDEV_AVAILABLE,
    DFD_CLUP_CDEV_DEL_AVAILABLE,
    DFD_CLUP_UNREG_CDEV_SNFC_CEN,
    DFD_CLUP_CDEV_DEL_SNFC_CEN,
    DFD_CLUP_UNREG_CDEV_SNFC_RFS,
    DFD_CLUP_CDEV_DEL_SNFC_RFS,
    DFD_CLUP_UNREG_CDEV_SNFC_PON,
    DFD_CLUP_CDEV_DEL_SNFC_PON,
// <Combo_chip> add E
	DFD_CLUP_CLASS_DESTORY,
	DFD_CLUP_DEV_DESTORY_PON,
	DFD_CLUP_DEV_DESTORY_CEN,
	DFD_CLUP_DEV_DESTORY_RFS,
	DFD_CLUP_DEV_DESTORY_ITR,
	DFD_CLUP_DEV_DESTORY_COMM,
	DFD_CLUP_DEV_DESTORY_RWS,
	DFD_CLUP_DEV_DESTORY_CFG,
// <Combo_chip> add S
    DFD_CLUP_DEV_DESTORY_SNFC_PON,
    DFD_CLUP_DEV_DESTORY_SNFC_RFS,
    DFD_CLUP_DEV_DESTORY_SNFC_CEN,
    DFD_CLUP_DEV_DESTORY_HSEL,
    DFD_CLUP_DEV_DESTORY_POLL,
    DFD_CLUP_DEV_DESTORY_INTU,
    DFD_CLUP_DEV_DESTORY_AVAILABLE,
// <Combo_chip> add E
	DFD_CLUP_ALL
};

/*
 * Chataring count
 */
#define DFD_LOW_RFS_CHATA_CNT_MAX  (3)
#define DFD_LOW_RFS_CHATA_CNT_WIN  (2)
#define DFD_HIGH_RFS_CHATA_CNT_MAX (5)
#define DFD_HIGH_RFS_CHATA_CNT_WIN (3)

/*
 * Alarm type
 */
#define DFD_ALARM_BASE              ( 0x00000000 )
#define DFD_ALARM_SUBPMIC_READ      ( DFD_ALARM_BASE | 0x0200 )
#define DFD_ALARM_SUBPMIC_WRITE     ( DFD_ALARM_BASE | 0x0201 )
#define DFD_ALARM_INTERRUPT_REQUEST ( DFD_ALARM_BASE | 0x0300 )

/* npdc300054413 add start */
/*
 * boolean type
 */
#define FELICA_TRUE 1
#define FELICA_FALSE 0
/* npdc300054413 add end */

/*
 * Wait Timer
 */
struct FELICA_TIMER {
	struct timer_list Timer;
	wait_queue_head_t wait;
};

/*
 * Device control data structure(PON terminal)
 */
struct FELICA_CTRLDEV_PON_INFO {
	/* Timeout control information */
	struct FELICA_TIMER PON_Timer;              /* PON terminal control timer */
};

/*
 * Device control data structure(CEN terminal)
 */
struct FELICA_CTRLDEV_CEN_INFO {
	/* Timeout control information */
	struct FELICA_TIMER PON_Timer;              /* PON terminal control timer */
	struct FELICA_TIMER CEN_Timer;              /* CEN terminal control timer */
};

// <Combo_chip> add S
/*
 * Device control data structure(wait timer 500ms for FeliCa finished)
 */
struct FELICA_CTRLDEV_AVAILABLE_INFO {
	/* Timeout control information */
	struct FELICA_TIMER AVAILABLE_Timer;        /* AVAILABLE control timer    */
};
// <Combo_chip> add E

/*
 * Device control data structure(ITR terminal)
 */
#ifdef FELICA_SYS_HAVE_DETECT_RFS
struct FELICA_CTRLDEV_ITR_INFO {
	/* RFS chataring counter */
	unsigned int      RFS_L_cnt;                             /* L level count */
	unsigned int      RFS_H_cnt;                             /* H level count */

	/* ITR result information */
	unsigned char     INT_Flag;                             /* INT interrupts */
	unsigned char     RFS_Flag;                             /* RFS interrupts */
// <Combo_chip> mod S
	unsigned char     INTU_Flag;                            /* INTU interrupts*/
//	unsigned char     reserve1[2];                          /* reserved       */
	unsigned char     reserve1[1];                          /* reserved       */
// <Combo_chip> mod E

	/* wait queue */
	wait_queue_head_t RcvWaitQ;

	/* Timeout control information */
	struct FELICA_TIMER RFS_Timer;                     /* RFS chataring timer */
};
#else
struct FELICA_CTRLDEV_ITR_INFO {
	/* ITR result information */
	unsigned char     INT_Flag;                             /* INT interrupts */
	unsigned char     RFS_Flag;                             /* RFS interrupts */
// <Combo_chip> mod S
	unsigned char     INTU_Flag;                            /* INTU interrupts*/
//	unsigned char     reserve1[2];                          /* reserved       */
	unsigned char     reserve1[1];                          /* reserved       */
// <Combo_chip> mod E

	/* wait queue */
	wait_queue_head_t RcvWaitQ;
};
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

/*
 * Device control data structure
 */
struct FELICA_DEV_INFO {
	unsigned int    w_cnt;                             /* write open counter  */
	unsigned int    r_cnt;                             /* read open counter   */
	void * Info;                                       /* interal saving data */
};


struct FELICA_ITR_INFO {
	unsigned int irq;
	irq_handler_t handler;
	unsigned long flags;
	const char *name;
	void *dev;
};

#endif /* __DV_FELICA_CTRL_LOCAL_H__ */
