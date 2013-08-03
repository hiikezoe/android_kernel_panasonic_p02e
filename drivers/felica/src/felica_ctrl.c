/*
 * @file felica.c
 * @brief FeliCa code
 *
 * @date 2010/12/17
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*
 * include files
 */
#include <linux/err.h>              /* PTR_ERR/IS_ERR           */
#include <linux/string.h>           /* memset                   */
#include <linux/errno.h>            /* errno                    */
#include <linux/module.h>           /* Module parameters        */
#include <linux/init.h>             /* module_init, module_exit */
#include <linux/fs.h>               /* struct file_operations   */
#include <linux/kernel.h>           /* kernel                   */
#include <linux/mm.h>               /* kmallo, GFP_KERNEL       */
#include <linux/slab.h>             /* kmalloc, kfree           */
#include <linux/timer.h>            /* init_timer               */
#include <linux/param.h>            /* HZ const                 */
#include <linux/poll.h>             /* poll                     */
#include <linux/delay.h>            /* udelay, mdelay           */
#include <linux/wait.h>             /* wait_queue_head_t        */
#include <linux/vmalloc.h>          /* vmalloc()                */
#include <linux/device.h>           /* class_create             */
#include <linux/interrupt.h>        /* request_irq              */
#include <linux/fcntl.h>            /* O_RDONLY,O_WRONLY        */
#include <linux/cdev.h>             /* cdev_init add del        */
// <iDPower_002> add S
#include <linux/i2c.h>              /* i2c                      */
// <iDPower_002> add E

#include <linux/cfgdrv.h>           /* cfgdrvier  */
// <iDPower_002> mod S
//#include <linux/gpio_ex.h>          /* gpio       */
#include <linux/gpio.h>             /* gpio       */
// <iDPower_002> mod E
// <iDPower_002> del S
//#include <linux/i2c/i2c-subpmic.h>  /* i2c submic */
// <iDPower_002> del E

// <iDPower_009> add S
#include <linux/wakelock.h>
// <iDPower_009> add E

//
#include <linux/irq.h>
//
#include <asm/uaccess.h>    /* copy_from_user, copy_to_user */
#include <asm/ioctls.h>     /* FIONREAD                     */

#include "felica_ctrl.h"         /* public header file  */
#include "felica_ctrl_local.h"   /* private header file */
#include "felica_ctrl_debug.h"   /* debug file          */
// <Combo_chip> add S
#include <linux/snfc_combo.h>
// <Combo_chip> add E

/*
 * Function prottype defined
 */
/* External if */
FELICA_MT_WEAK static int __init FeliCaCtrl_init(void);
FELICA_MT_WEAK static void __exit FeliCaCtrl_exit(void);
FELICA_MT_WEAK static int FeliCaCtrl_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static unsigned int FeliCaCtrl_poll(struct file *pfile,  struct poll_table_struct *pwait);
FELICA_MT_WEAK static ssize_t FeliCaCtrl_write(struct file *pfile,  const char *buff, size_t count, loff_t *poff);
FELICA_MT_WEAK static ssize_t FeliCaCtrl_read(struct file *pfile, char *buff, size_t count, loff_t *poff);
// <iDPower_002> add S
FELICA_MT_WEAK static int FeliCaCtrl_cen_probe(struct i2c_client *client, const struct i2c_device_id *devid);
// <iDPower_002> add E

/* Interrupt handler */
FELICA_MT_WEAK static irqreturn_t FeliCaCtrl_interrupt_INT(int i_req, void *pv_devid);
FELICA_MT_WEAK static void FeliCaCtrl_timer_hnd_general(unsigned long p);
#ifdef FELICA_SYS_HAVE_DETECT_RFS
FELICA_MT_WEAK static irqreturn_t FeliCaCtrl_interrupt_RFS(int i_req, void *pv_devid);
FELICA_MT_WEAK static void FeliCaCtrl_timer_hnd_rfs_chata_low(unsigned long p);
FELICA_MT_WEAK static void FeliCaCtrl_timer_hnd_rfs_chata_high(unsigned long p);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
// <Combo_chip> add S
FELICA_MT_WEAK static irqreturn_t SnfcCtrl_interrupt_INTU(int i_req, void *pv_devid);
// <Combo_chip> add E

/* Internal function */
FELICA_MT_WEAK static int FeliCaCtrl_priv_ctrl_enable(struct FELICA_TIMER *pTimer);
FELICA_MT_WEAK static int FeliCaCtrl_priv_ctrl_unenable(struct FELICA_TIMER* pPonTimer, struct FELICA_TIMER* pCenTimer);
FELICA_MT_WEAK static int FeliCaCtrl_priv_ctrl_online(struct FELICA_TIMER *pTimer, int need_wait);  /* npdc300054413 add */
FELICA_MT_WEAK static int FeliCaCtrl_priv_ctrl_offline(void);
// <Combo_chip> add S
FELICA_MT_WEAK static int SnfcCtrl_priv_ctrl_tofelica(void);
FELICA_MT_WEAK static int SnfcCtrl_priv_ctrl_tosnfc(void);
// <Combo_chip> add E
FELICA_MT_WEAK static int FeliCaCtrl_priv_terminal_cen_read(unsigned char *cen_data);
FELICA_MT_WEAK static int FeliCaCtrl_priv_terminal_rfs_read(unsigned char *rfs_data);
FELICA_MT_WEAK static int FeliCaCtrl_priv_terminal_int_read(unsigned char *int_data);
// <Combo_chip> add S
FELICA_MT_WEAK static int SnfcCtrl_priv_terminal_hsel_read(unsigned int *hsel_data);
FELICA_MT_WEAK static int SnfcCtrl_priv_terminal_available_read(unsigned int *available_data, struct FELICA_TIMER *pTimer);
FELICA_MT_WEAK static int SnfcCtrl_priv_terminal_intu_read(unsigned int *int_data);
// <Combo_chip> add E
FELICA_MT_WEAK static int FeliCaCtrl_priv_terminal_cen_write(unsigned char cen_data);
FELICA_MT_WEAK static int FeliCaCtrl_priv_terminal_pon_write(unsigned char pon_data);
// <Combo_chip> add S
FELICA_MT_WEAK static int SnfcCtrl_priv_terminal_hsel_write(unsigned int hsel_data);
// <Combo_chip> add E
FELICA_MT_WEAK static int FeliCaCtrl_priv_interrupt_ctrl_init(unsigned char itr_type);
FELICA_MT_WEAK static void FeliCaCtrl_priv_interrupt_ctrl_exit(unsigned char itr_type);
FELICA_MT_WEAK static int FeliCaCtrl_priv_interrupt_ctrl_regist(unsigned char itr_type);
FELICA_MT_WEAK static void FeliCaCtrl_priv_interrupt_ctrl_unregist(unsigned char itr_type);
// <iDPower_002> add S
FELICA_MT_WEAK static int FeliCaCtrl_priv_acknowledge_polling(void);
// <iDPower_002> add E
#ifdef FELICA_SYS_HAVE_DETECT_RFS
FELICA_MT_WEAK static void FeliCaCtrl_priv_interrupt_ctrl_enable(unsigned char itr_type);
FELICA_MT_WEAK static void FeliCaCtrl_priv_interrupt_ctrl_disable(unsigned char itr_type);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
FELICA_MT_WEAK static void FeliCaCtrl_priv_drv_timer_data_init(struct FELICA_TIMER *pTimer);
#ifdef FELICA_SYS_HAVE_DETECT_RFS
FELICA_MT_WEAK static void FeliCaCtrl_priv_dev_rfs_timer_start(unsigned long expires, void (*function)(unsigned long));
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
FELICA_MT_WEAK static void FeliCaCtrl_priv_drv_init_cleanup(int cleanup);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_pon_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_cen_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_rfs_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_itr_open(struct inode *pinode, struct file *pfile);
// <Combo_chip> add S
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_intu_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_hsel_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_poll_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_available_open(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_hsel_write(const char *buff, size_t count);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_uartcc_state_write(struct file  *pfile, const char *buff, size_t count);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_hsel_read(char *buff, size_t count);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_intu_read(char *buff, size_t count);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_uartcc_state_read(char *buff, size_t count);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_available_read(char *buff, size_t count);
// <Combo_chip> add E
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_pon_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_cen_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_rfs_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int FeliCaCtrl_priv_dev_itr_close(struct inode *pinode, struct file *pfile);
// <Combo_chip> add S
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_intu_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_hsel_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_poll_close(struct inode *pinode, struct file *pfile);
FELICA_MT_WEAK static int SnfcCtrl_priv_dev_available_close(struct inode *pinode, struct file *pfile);
// <Combo_chip> add E
FELICA_MT_WEAK static void FeliCaCtrl_priv_alarm(unsigned long err_id);

/*
 * static paramater
 */
static struct class *felica_class; /* device class information */

static struct FELICA_DEV_INFO gPonCtrl; /* device control info(PON) */
static struct FELICA_DEV_INFO gCenCtrl; /* device control info(CEN) */
static struct FELICA_DEV_INFO gRfsCtrl; /* device control info(RFS) */
static struct FELICA_DEV_INFO gItrCtrl; /* device control info(ITR) */
// <Combo_chip> add S
static struct FELICA_DEV_INFO gIntuCtrl; /* device control info(INTU) */
static struct FELICA_DEV_INFO gHselCtrl; /* device control info(HSEL) */
static struct FELICA_DEV_INFO gPollCtrl; /* device control info(POLL) */
static struct FELICA_DEV_INFO gAvailableCtrl; /* device control info(AVAILABLE) */
// <Combo_chip> add E

static spinlock_t itr_lock;         /* spin_lock param */
static unsigned long itr_lock_flag; /* spin_lock flag  */

struct cdev g_cdev_fpon;   /* charactor device of PON */
struct cdev g_cdev_fcen;   /* charactor device of CEN */
struct cdev g_cdev_frfs;   /* charactor device of RFS */
struct cdev g_cdev_fitr;   /* charactor device of ITR */

//<iDPower_009> add S
struct wake_lock felicactrl_wake_lock;
//<iDPower_009> add E
// <Combo_chip> add S
struct cdev g_cdev_nintu;  /* charactor device of INTU*/
struct cdev g_cdev_nhsel;  /* charactor device of HSEL*/
struct cdev g_cdev_nautopoll;/* charactor device of AUTOPOLL*/
struct cdev g_cdev_navailable;/* charactor device of AVAILABLE*/
struct cdev g_cdev_nrfs;   /* charactor device of RFS */
struct cdev g_cdev_ncen;   /* charactor device of CEN */
struct cdev g_cdev_npon;   /* charactor device of PON */
// <Combo_chip> add E

/* Interrupt Control Informaion */
static struct FELICA_ITR_INFO gFeliCaItrInfo[DFD_ITR_TYPE_NUM] = {
	{
		.irq=     0,
#ifdef FELICA_SYS_HAVE_DETECT_RFS
		.handler= FeliCaCtrl_interrupt_RFS,
#else
		.handler= NULL,
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
		.flags=   IRQF_DISABLED|IRQF_TRIGGER_FALLING,
		.name=    FELICA_CTRL_ITR_STR_RFS,
		.dev=     NULL
	},
	{
		.irq=     0,
		.handler= FeliCaCtrl_interrupt_INT,
		.flags=   IRQF_DISABLED|IRQF_TRIGGER_FALLING,
		.name=    FELICA_CTRL_ITR_STR_INT,
		.dev=     NULL
// <Combo_chip> add S
	},
	{
		.irq=     0,
		.handler= SnfcCtrl_interrupt_INTU,
		.flags=   IRQF_DISABLED|(IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING),
		.name=    SNFC_CTRL_ITR_STR_INTU,
		.dev=     NULL
	}
// <Combo_chip> add E
};

/*
 * sturut of reginsting External if
 */
static struct file_operations felica_fops = {
	.owner=   THIS_MODULE,
	.poll=    FeliCaCtrl_poll,
	.read=    FeliCaCtrl_read,
	.write=   FeliCaCtrl_write,
#ifdef FELIACA_RAM_LOG_ON
	.ioctl=   FeliCaCtrl_ioctl,
#endif /* FELIACA_RAM_LOG_ON */
	.open=    FeliCaCtrl_open,
	.release= FeliCaCtrl_close
};

// <iDPower_002> add S
static struct i2c_client *cen_client;

static const struct i2c_device_id CEN_ID[] = {
	{ FELICA_DEV_NAME_CTRL_CEN, 0 },
	{ }
};

static struct i2c_driver cen_i2c_fops = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= FELICA_DEV_NAME_CTRL_CEN,
	},
	.class		= I2C_CLASS_HWMON,
	.probe		= FeliCaCtrl_cen_probe,
	.id_table	= CEN_ID,
};
// <iDPower_002> add E

/*
 * Functions
 */

/**
 *   @brief Initialize module function
 *
 *   @par   Outline:\n
 *          Register FeliCa device driver, \n
 *          and relate system call and the I/F function
 *
 *   @param none
 *
 *   @retval 0   Normal end
 *   @retval <0  Error no of register device
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int __init FeliCaCtrl_init(void)
{
	int ret = 0;
	int cleanup = DFD_CLUP_NONE;
	struct device *devt;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	memset(&gPonCtrl, 0x00, sizeof(gPonCtrl));
	memset(&gCenCtrl, 0x00, sizeof(gCenCtrl));
	memset(&gRfsCtrl, 0x00, sizeof(gRfsCtrl));
	memset(&gItrCtrl, 0x00, sizeof(gItrCtrl));
// <Combo_chip> add S
	memset(&gIntuCtrl, 0x00, sizeof(gIntuCtrl));
    memset(&gHselCtrl, 0x00, sizeof(gHselCtrl));
    memset(&gPollCtrl, 0x00, sizeof(gPollCtrl));
    memset(&gAvailableCtrl, 0x00, sizeof(gAvailableCtrl));
// <Combo_chip> add E

#ifdef FELICA_SYS_HAVE_DETECT_RFS
	ret = FeliCaCtrl_priv_interrupt_ctrl_init(DFD_ITR_TYPE_RFS);
	if (ret < 0) {
		DEBUG_FD_LOG_IF_ASSERT("ctrl_init() ret=%d", ret);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
		return ret;
	}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
	ret = FeliCaCtrl_priv_interrupt_ctrl_init(DFD_ITR_TYPE_INT);
	if (ret < 0) {
#ifdef FELICA_SYS_HAVE_DETECT_RFS
		FeliCaCtrl_priv_interrupt_ctrl_exit(DFD_ITR_TYPE_RFS);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
		DEBUG_FD_LOG_IF_ASSERT("ctrl_init() ret=%d", ret);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
		return ret;
	}
// <Combo_chip> add S
	ret = FeliCaCtrl_priv_interrupt_ctrl_init(DFD_ITR_TYPE_INTU);
	if (ret < 0) {
		DEBUG_FD_LOG_IF_ASSERT("ctrl_init() ret=%d", ret);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
		return ret;
	}
// <Combo_chip> add E

	do {
		/* register_chrdev_region() for felica_ctrl */
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_PON),
					FELICA_DEV_NUM,
					FELICA_DEV_NAME_CTRL_PON);
		if (ret < 0) {
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(PON) failed.");
			break;
		}
		cdev_init(&g_cdev_fpon, &felica_fops);
		g_cdev_fpon.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fpon,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_PON),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_PON;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(PON) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_CEN),
					FELICA_DEV_NUM,
					FELICA_DEV_NAME_CTRL_CEN);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_PON;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(CEN) failed.");
			break;
		}
		cdev_init(&g_cdev_fcen, &felica_fops);
		g_cdev_fcen.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fcen,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_CEN),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_CEN;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(CEN) failed.");
			break;
		}
// <iDPower_002> add S
		ret = i2c_add_driver(&cen_i2c_fops);
		if (ret < 0 ) {
			cleanup = DFD_CLUP_UNREG_CDEV_CEN;
			DEBUG_FD_LOG_IF_ASSERT("i2c_add_driver(CEN) failed.");
			break;
		}
// <iDPower_002> add E
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_RFS),
					FELICA_DEV_NUM,
					FELICA_DEV_NAME_CTRL_RFS);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_CEN;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(RFS) failed.");
			break;
		}
		cdev_init(&g_cdev_frfs, &felica_fops);
		g_cdev_frfs.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_frfs,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_RFS),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_RFS;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(RFS) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_ITR),
					FELICA_DEV_NUM,
					FELICA_DEV_NAME_CTRL_ITR);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_RFS;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(ITR) failed.");
			break;
		}
		cdev_init(&g_cdev_fitr, &felica_fops);
		g_cdev_fitr.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fitr,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_ITR),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_ITR;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(ITR) failed.");
			break;
		}
#ifdef FELIACA_RAM_LOG_ON
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_DEBUG),
					FELICA_DEV_NUM,
					FELICA_DEV_NAME_CTRL_DEBUG);
		if (ret < 0) {
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(DEBUG) failed.");
			break;
		}
		cdev_init(&g_cdev_fdebug, &felica_fops);
		g_cdev_fdebug.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_fdebug,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_DEBUG),
				FELICA_DEV_NUM);
#endif /* FELIACA_RAM_LOG_ON */
// <Combo_chip> add S
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_PON),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_PON);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_ITR;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(PON) failed.");
			break;
		}
		cdev_init(&g_cdev_npon, &felica_fops);
		g_cdev_npon.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_npon,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_PON),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_SNFC_PON;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(PON) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_RFS),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_RFS);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_SNFC_PON;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(INTU) failed.");
			break;
		}
		cdev_init(&g_cdev_nrfs, &felica_fops);
		g_cdev_nrfs.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_nrfs,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_RFS),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_SNFC_RFS;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(RFS) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_CEN),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_CEN);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_SNFC_RFS;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(INTU) failed.");
			break;
		}
		cdev_init(&g_cdev_ncen, &felica_fops);
		g_cdev_ncen.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_ncen,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_CEN),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_SNFC_CEN;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(RFS) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_INTU),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_INTU);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_SNFC_CEN;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(INTU) failed.");
			break;
		}
		cdev_init(&g_cdev_nintu, &felica_fops);
		g_cdev_nintu.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_nintu,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_INTU),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_INTU;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(INTU) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_HSEL),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_HSEL);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_INTU;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(HSEL) failed.");
			break;
		}
		cdev_init(&g_cdev_nhsel, &felica_fops);
		g_cdev_nhsel.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_nhsel,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_HSEL),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_HSEL;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(HSEL) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AUTOPOLL),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_UARTCC);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_HSEL;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(AUTOPOLL) failed.");
			break;
		}
		cdev_init(&g_cdev_nautopoll, &felica_fops);
		g_cdev_nautopoll.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_nautopoll,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AUTOPOLL),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_POLL;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(POLL) failed.");
			break;
		}
		ret = register_chrdev_region(
					MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AVAILABLE),
					FELICA_DEV_NUM,
					SNFC_DEV_NAME_CTRL_AVAILABLE);
		if (ret < 0) {
			cleanup = DFD_CLUP_CDEV_DEL_POLL;
			DEBUG_FD_LOG_IF_ASSERT("register_chrdev_region(AVAILABLE) failed.");
			break;
		}
		cdev_init(&g_cdev_navailable, &felica_fops);
		g_cdev_navailable.owner = THIS_MODULE;
		ret = cdev_add(
				&g_cdev_navailable,
				MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AVAILABLE),
				FELICA_DEV_NUM);
		if (ret < 0) {
			cleanup = DFD_CLUP_UNREG_CDEV_AVAILABLE;
			DEBUG_FD_LOG_IF_ASSERT("cdev_init(AVAILABLE) failed.");
			break;
		}
// <Combo_chip> add E

		/* class_create() for felica_ctrl, felica */
		felica_class = class_create(THIS_MODULE, FELICA_DEV_NAME);
		if (IS_ERR(felica_class)) {
			ret = PTR_ERR(felica_class);
// <Combo_chip> mod S
//			cleanup = DFD_CLUP_CDEV_DEL_ITR;
			cleanup = DFD_CLUP_CDEV_DEL_AVAILABLE;
// <Combo_chip> mod E
			DEBUG_FD_LOG_IF_ASSERT("class_create failed.");
			break;
		}

		/* device_create() for felica_ctrl */
		devt = device_create(
			felica_class,
			NULL,
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_PON),
			NULL,
			FELICA_DEV_NAME_CTRL_PON);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_CLASS_DESTORY;
			DEBUG_FD_LOG_IF_ASSERT("device_create(PON) failed.");
			break;
		}
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_CEN),
				NULL,
				FELICA_DEV_NAME_CTRL_CEN);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_PON;
			DEBUG_FD_LOG_IF_ASSERT("device_create(CEN) failed.");
			break;
		}
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_RFS),
				NULL,
				FELICA_DEV_NAME_CTRL_RFS);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_CEN;
			DEBUG_FD_LOG_IF_ASSERT("device_create(RFS) failed.");
			break;
		}
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_ITR),
				NULL,
				FELICA_DEV_NAME_CTRL_ITR);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_RFS;
			DEBUG_FD_LOG_IF_ASSERT("device_create(ITR) failed.");
			break;
		}
#ifdef FELIACA_RAM_LOG_ON
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_DEBUG),
				NULL,
				FELICA_DEV_NAME_CTRL_DEBUG);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			DEBUG_FD_LOG_IF_ASSERT("device_create(DEBUG1) failed.");
			break;
		}
#endif /* FELIACA_RAM_LOG_ON */
// <iDPower_002> del S
//		/* device_create() for felica */
//		devt = device_create(
//				felica_class,
//				NULL,
//				MKDEV(FELICA_COMM_MAJOR_NO, FELICA_COMM_MINOR_NO_COMM),
//				NULL,
//				FELICA_DEV_NAME_COMM);
//		if (IS_ERR(devt)) {
//			ret = PTR_ERR(devt);
//			cleanup = DFD_CLUP_DEV_DESTORY_ITR;
//			DEBUG_FD_LOG_IF_ASSERT("device_create(COMM) failed.");
//			break;
//		}
// <iDPower_002> del E
#ifdef FELIACA_RAM_LOG_ON
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_COMM_MAJOR_NO, FELICA_COMM_MINOR_NO_DEBUG),
				NULL,
				FELICA_DEV_NAME_COMM_DEBUG);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			DEBUG_FD_LOG_IF_ASSERT("device_create(DEBUG2) failed.");
			break;
		}
#endif /* FELIACA_RAM_LOG_ON */
		/* device_create() for felica_rws */
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_RWS_MAJOR_NO, FELICA_RWS_MINOR_NO_RWS),
				NULL,
				FELICA_DEV_NAME_RWS);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_COMM;
			DEBUG_FD_LOG_IF_ASSERT("device_create(RWS) failed.");
			break;
		}
		/* device_create() for felica_cfg */
		devt = device_create(
				felica_class,
				NULL,
				MKDEV(FELICA_CFG_MAJOR_NO, FELICA_CFG_MINOR_NO_CFG),
				NULL,
				FELICA_DEV_NAME_CFG);
		if (IS_ERR(devt)) {
			ret = PTR_ERR(devt);
			cleanup = DFD_CLUP_DEV_DESTORY_COMM;
			DEBUG_FD_LOG_IF_ASSERT("device_create(CFG) failed.");
			break;
		}
// <Combo_chip> add S
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_PON),
                NULL,
                SNFC_DEV_NAME_CTRL_PON);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_CFG;
            DEBUG_FD_LOG_IF_ASSERT("device_create(PON) failed.");
            break;
        }
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_RFS),
                NULL,
                SNFC_DEV_NAME_CTRL_RFS);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_SNFC_PON;
            DEBUG_FD_LOG_IF_ASSERT("device_create(INTU) failed.");
            break;
        }
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_CEN),
                NULL,
                SNFC_DEV_NAME_CTRL_CEN);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_SNFC_RFS;
            DEBUG_FD_LOG_IF_ASSERT("device_create(INTU) failed.");
            break;
        }
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_INTU),
                NULL,
                SNFC_DEV_NAME_CTRL_INTU);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_CEN;
            DEBUG_FD_LOG_IF_ASSERT("device_create(INTU) failed.");
            break;
        }
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_HSEL),
                NULL,
                SNFC_DEV_NAME_CTRL_HSEL);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_INTU;
            DEBUG_FD_LOG_IF_ASSERT("device_create(HSEL) failed.");
            break;
        }
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AUTOPOLL),
                NULL,
                SNFC_DEV_NAME_CTRL_UARTCC);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_HSEL;
            DEBUG_FD_LOG_IF_ASSERT("device_create(POLL) failed.");
            break;
        }
        devt = device_create(
                felica_class,
                NULL,
                MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AVAILABLE),
                NULL,
                SNFC_DEV_NAME_CTRL_AVAILABLE);
        if (IS_ERR(devt)) {
            ret = PTR_ERR(devt);
            cleanup = DFD_CLUP_DEV_DESTORY_POLL;
            DEBUG_FD_LOG_IF_ASSERT("device_create(HSEL) failed.");
            break;
        }
// <Combo_chip> add E
	} while(0);

	/* cleanup device driver when failing in the initialization f*/
	FeliCaCtrl_priv_drv_init_cleanup(cleanup);

    //<iDPower_009> add S
    wake_lock_init(&felicactrl_wake_lock, WAKE_LOCK_SUSPEND, "felicactrl_wake_lock");
    //<iDPower_009> add S

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}
module_init(FeliCaCtrl_init);

/**
 *   @brief Exit module function
 *
 *   @par   Outline:\n
 *          Erases subscription of device driver
 *
 *   @param none
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void __exit FeliCaCtrl_exit(void)
{

	DEBUG_FD_LOG_IF_FNC_ST("start");

	FeliCaCtrl_priv_drv_init_cleanup(DFD_CLUP_ALL);
// <iDPower_002> add S
	i2c_del_driver(&cen_i2c_fops);
// <iDPower_002> add E

#ifdef FELICA_SYS_HAVE_DETECT_RFS
	FeliCaCtrl_priv_interrupt_ctrl_exit(DFD_ITR_TYPE_RFS);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
	FeliCaCtrl_priv_interrupt_ctrl_exit(DFD_ITR_TYPE_INT);

    //<iDPower_009> add S
    wake_lock_destroy(&felicactrl_wake_lock);
    //<iDPower_009> add E

// <Combo_chip> add S
	FeliCaCtrl_priv_interrupt_ctrl_exit(DFD_ITR_TYPE_INTU);
// <Combo_chip> add E

	DEBUG_FD_LOG_IF_FNC_ED("return");
	return;
}
module_exit(FeliCaCtrl_exit);

/**
 *   @brief Open felica drivers function
 *
 *   @par   Outline:\n
 *          Acquires information on the FeliCa device control, and initializes
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_open(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	unsigned char minor = 0;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if ((pinode == NULL) || (pfile == NULL)) {
		DEBUG_FD_LOG_IF_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);
	DEBUG_FD_LOG_MINOR_TO_STR(minor);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_PON:
		ret = FeliCaCtrl_priv_dev_pon_open(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_CEN:
		ret = FeliCaCtrl_priv_dev_cen_open(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_RFS:
		ret = FeliCaCtrl_priv_dev_rfs_open(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_ITR:
		ret = FeliCaCtrl_priv_dev_itr_open(pinode, pfile);
		break;
#ifdef FELIACA_RAM_LOG_ON
	case FELICA_CTRL_MINOR_NO_DEBUG:
		ret = 0;
		break;
#endif /* FELIACA_RAM_LOG_ON */
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_PON:
		ret = FeliCaCtrl_priv_dev_pon_open(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_RFS:
		ret = FeliCaCtrl_priv_dev_rfs_open(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_INTU:
		ret = SnfcCtrl_priv_dev_intu_open(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_CEN:
		ret = FeliCaCtrl_priv_dev_cen_open(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_HSEL:
		ret = SnfcCtrl_priv_dev_hsel_open(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_AUTOPOLL:
		ret = SnfcCtrl_priv_dev_poll_open(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_AVAILABLE:
		ret = SnfcCtrl_priv_dev_available_open(pinode, pfile);
		break;
// <Combo_chip> add E
	default:
		ret = -ENODEV;
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close felica drivers function
 *
 *   @par   Outline:\n
 *          Releases information on the FeliCa driver control
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_close(struct inode *pinode, struct file *pfile)
{
	int ret = 0;
	unsigned char minor = 0;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if ((pinode == NULL) || (pfile == NULL)) {
		DEBUG_FD_LOG_IF_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	minor = MINOR(pinode->i_rdev);
	DEBUG_FD_LOG_MINOR_TO_STR(minor);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_PON:
		ret = FeliCaCtrl_priv_dev_pon_close(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_CEN:
		ret = FeliCaCtrl_priv_dev_cen_close(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_RFS:
		ret = FeliCaCtrl_priv_dev_rfs_close(pinode, pfile);
		break;
	case FELICA_CTRL_MINOR_NO_ITR:
		ret = FeliCaCtrl_priv_dev_itr_close(pinode, pfile);
		break;
#ifdef FELIACA_RAM_LOG_ON
	case FELICA_CTRL_MINOR_NO_DEBUG:
		ret = 0;
		break;
#endif /* FELIACA_RAM_LOG_ON */
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_PON:
		ret = FeliCaCtrl_priv_dev_pon_close(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_RFS:
		ret = FeliCaCtrl_priv_dev_rfs_close(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_INTU:
		ret = SnfcCtrl_priv_dev_intu_close(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_CEN:
		ret = FeliCaCtrl_priv_dev_cen_close(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_HSEL:
		ret = SnfcCtrl_priv_dev_hsel_close(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_AUTOPOLL:
		ret = SnfcCtrl_priv_dev_poll_close(pinode, pfile);
		break;
	case SNFC_CTRL_MINOR_NO_AVAILABLE:
		ret = SnfcCtrl_priv_dev_available_close(pinode, pfile);
		break;
// <Combo_chip> add E
	default:
		ret = -ENODEV;
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}


/**
 *   @brief poll function
 *
 *   @par   Outline:\n
 *          Acquires whether or not it is writable to the device,\n
 *          and whether or not it is readable to it
 *
 *   @param[in]     *pfile  pointer of file infomation struct
 *   @param[in/out] *pwait  polling table structure address
 *
 *   @retval Success\n
 *             Returns in the OR value of the reading, the writing in\n
 *               case reading\n
 *                 0                    block's there being\n
 *                 POLLIN | POLLRDNORM  not in the block\n
 *               case writing\n
 *                 0                    block's there being\n
 *                 POLLOUT | POLLWRNORM not in the block\n
 *           Failure\n
 *                 POLLHUP | POLLERR    not in the block of reading and writing
 *
 *   @par Special note
 *     - As for the writing in, because it doesn't use, \n
 *       it always specifies block prosecution.
 *
 **/
FELICA_MT_WEAK
static unsigned int FeliCaCtrl_poll(
							struct file *pfile,
							struct poll_table_struct *pwait)
{
	unsigned int ret = 0;
	unsigned char minor = 0;
    struct FELICA_CTRLDEV_ITR_INFO *pItr = NULL;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if (pfile == NULL) {
		ret = (POLLHUP | POLLERR);
		DEBUG_FD_LOG_IF_ASSERT("pfile=%p", pfile);
		DEBUG_FD_LOG_IF_FNC_ED("return=%#x", ret);
		return ret;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);
	DEBUG_FD_LOG_MINOR_TO_STR(minor);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_ITR:
		pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
		if (NULL == pItr) {
			DEBUG_FD_LOG_IF_ASSERT("pItr=NULL");
			ret = (POLLHUP | POLLERR);
			break;
		}
		poll_wait(pfile, &pItr->RcvWaitQ, pwait);

		/* Readable check */
		if (FELICA_EDGE_NON != pItr->INT_Flag) {
			ret |= (POLLIN  | POLLRDNORM);
		}
		if (FELICA_EDGE_NON != pItr->RFS_Flag) {
			ret |= (POLLIN  | POLLRDNORM);
		}

		/* Writeable is allways not block */
		ret |= (POLLOUT | POLLWRNORM);
		break;
	default:
		ret = (POLLHUP | POLLERR);
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%#x", ret);
	return ret;
}

/**
 *   @brief Writes felica drivers function
 *
 *   @par   Outline:\n
 *          Writes data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file infomation struct
 *   @param[in] *buff   user buffer address which maintains the data to write
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0       Wrote data size
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EIO     I/O error
 *   @retval -EFAULT  Bad address
 *
 *   @par Special note
 *     - none
 *
 **/
FELICA_MT_WEAK
static ssize_t FeliCaCtrl_write(
							struct file  *pfile,
							const char   *buff,
							size_t        count,
							loff_t       *poff)
{
	ssize_t ret = 0;
	unsigned char minor = 0;
	unsigned long ret_cpy = 0;
	int ret_sub = 0;
	struct FELICA_CTRLDEV_PON_INFO *pPonCtrl = NULL;
	struct FELICA_CTRLDEV_CEN_INFO *pCenCtrl = NULL;

	unsigned char local_buff[FELICA_OUT_SIZE];

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if ((pfile == NULL) || (buff == NULL)) {
		DEBUG_FD_LOG_IF_ASSERT("pfile=%p buff=%p", pfile, buff);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);
	DEBUG_FD_LOG_MINOR_TO_STR(minor);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_PON:
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_PON:
// <Combo_chip> add E
		pPonCtrl = (struct FELICA_CTRLDEV_PON_INFO*)gPonCtrl.Info;
		if (NULL == pPonCtrl) {
			DEBUG_FD_LOG_IF_ASSERT("gPonCtrl.Info = NULL");
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		if (count != FELICA_OUT_SIZE) {
			DEBUG_FD_LOG_IF_ASSERT("count=%d", count);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		ret_cpy = copy_from_user(local_buff, buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			DEBUG_FD_LOG_IF_ASSERT("copy_from_user() ret=%ld",ret_cpy);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EFAULT;
		}

		DEBUG_FD_LOG_TERMINAL_IF("[PON] write() data=%d", local_buff[0]);
		if (FELICA_OUT_L == local_buff[0]) {
			ret_sub = FeliCaCtrl_priv_ctrl_offline();
		}
		else if (FELICA_OUT_H == local_buff[0]) {
			ret_sub = FeliCaCtrl_priv_ctrl_online(&(pPonCtrl->PON_Timer), FELICA_TRUE);    /* npdc300054413 add */
		}
		/* npdc300054413 add start */
		else if (FELICA_OUT_H_NOWAIT == local_buff[0]) {
			ret_sub = FeliCaCtrl_priv_ctrl_online(&(pPonCtrl->PON_Timer), FELICA_FALSE);
		}
		/* npdc300054413 add end */
		else {
			ret_sub = -EINVAL;
			DEBUG_FD_LOG_IF_ASSERT("local_buff[0]=%d", local_buff[0]);
		}

		if (ret_sub == 0) {
			ret = FELICA_OUT_SIZE;
		}
		else{
			ret = ret_sub;
		}
		break;

	case FELICA_CTRL_MINOR_NO_CEN:
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_CEN:
// <Combo_chip> add E
		pCenCtrl = (struct FELICA_CTRLDEV_CEN_INFO*)gCenCtrl.Info;
		if (NULL == pCenCtrl) {
			DEBUG_FD_LOG_IF_ASSERT("gCenCtrl.Info = NULL");
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		if (count != FELICA_OUT_SIZE) {
			DEBUG_FD_LOG_IF_ASSERT("count=%d", count);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		ret_cpy = copy_from_user(local_buff, buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			DEBUG_FD_LOG_IF_ASSERT("copy_from_user() ret=%ld",ret_cpy);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EFAULT;
		}

		DEBUG_FD_LOG_TERMINAL_IF("[CEN] write() data=%d", local_buff[0]);
		if (FELICA_OUT_L == local_buff[0]) {
			ret_sub = FeliCaCtrl_priv_ctrl_unenable(
						  &pCenCtrl->PON_Timer
						, &pCenCtrl->CEN_Timer);
		}
		else if (FELICA_OUT_H == local_buff[0]) {
			ret_sub = FeliCaCtrl_priv_ctrl_enable(
						  &pCenCtrl->CEN_Timer);
		}
		else {
			ret_sub = -EINVAL;
			DEBUG_FD_LOG_IF_ASSERT("local_buff[0]=%d", local_buff[0]);
		}

		if (ret_sub == 0) {
			ret = FELICA_OUT_SIZE;
		}
		else {
			ret = ret_sub;
		}
		break;
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_HSEL:
		ret = SnfcCtrl_priv_dev_hsel_write(buff,count);
		break;
	case SNFC_CTRL_MINOR_NO_AUTOPOLL:
		ret = SnfcCtrl_priv_dev_uartcc_state_write(pfile, buff, count);
		break;
// <Combo_chip> add E

	default:
		ret = -ENODEV;
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Read felica drivers function
 *
 *   @par   Outline:\n
 *          Read data from the device to the user space
 *
 *   @param[in] *pfile  pointer of file infomation struct
 *   @param[in] *buff   user buffer address which maintains the data to read
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0        Read data size
 *   @retval -EINVAL   Invalid argument
 *   @retval -EFAULT   Bad address
 *   @retval -EBADF    Bad file number
 *   @retval -ENODEV   No such device
 *
 *   @par Special note
 *     - none
 *
 **/
FELICA_MT_WEAK
static ssize_t FeliCaCtrl_read(
							struct file  *pfile,
							char         *buff,
							size_t        count,
							loff_t       *poff)
{
	ssize_t 		ret = 0;
	unsigned char	local_buff[max(FELICA_OUT_SIZE, FELICA_EDGE_OUT_SIZE)];
	unsigned char	minor = 0;
	int 			ret_sub = 0;
	unsigned char	w_rdata;
	unsigned long	ret_cpy = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pItr;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if ((pfile == NULL) || (buff == NULL)) {
		DEBUG_FD_LOG_IF_ASSERT("pfile=%p buff=%p", pfile, buff);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	minor = MINOR(pfile->f_dentry->d_inode->i_rdev);
	DEBUG_FD_LOG_MINOR_TO_STR(minor);

	switch (minor) {
	case FELICA_CTRL_MINOR_NO_CEN:
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_CEN:
// <Combo_chip> add E
		if (count != FELICA_OUT_SIZE) {
			DEBUG_FD_LOG_IF_ASSERT("count=%d", count);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		ret_sub = FeliCaCtrl_priv_terminal_cen_read(&w_rdata);
		if (ret_sub != 0) {
			DEBUG_FD_LOG_IF_ASSERT("cen_read() ret=%d", ret_sub);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret_sub);
			return ret_sub;
		}

		switch (w_rdata) {
		case DFD_OUT_H:
			local_buff[0] = FELICA_OUT_H;
			break;
		case DFD_OUT_L:
		default:
			local_buff[0] = FELICA_OUT_L;
			break;
		}
		DEBUG_FD_LOG_TERMINAL_IF("[CEN] read() data=%d", local_buff[0]);

		ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			DEBUG_FD_LOG_IF_ASSERT("copy_to_user() ret=%ld",ret_cpy);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EFAULT);
			return -EFAULT;
		}
		else {
			ret = FELICA_OUT_SIZE;
		}
		break;

	case FELICA_CTRL_MINOR_NO_RFS:
// <Combo_chip> add S
	case SNFC_CTRL_MINOR_NO_RFS:
// <Combo_chip> add E
		if (count != FELICA_OUT_SIZE) {
			DEBUG_FD_LOG_IF_ASSERT("count=%d", count);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		ret_sub = FeliCaCtrl_priv_terminal_rfs_read(&w_rdata);
		if (ret_sub != 0) {
			DEBUG_FD_LOG_IF_ASSERT("rfs_read() ret=%d",ret_sub);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret_sub);
			return ret_sub;
		}

		switch (w_rdata) {
		case DFD_OUT_L:
			local_buff[0] = FELICA_OUT_RFS_L;
			break;
		case DFD_OUT_H:
		default:
			local_buff[0] = FELICA_OUT_RFS_H;
			break;
		}
		DEBUG_FD_LOG_TERMINAL_IF("[RFS] read() data=%d", local_buff[0]);

		ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
		if (ret_cpy != 0) {
			DEBUG_FD_LOG_IF_ASSERT("copy_to_user() ret=%ld",ret_cpy);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EFAULT);
			return -EFAULT;
		}
		else {
			ret = FELICA_OUT_SIZE;
		}
		break;

	case FELICA_CTRL_MINOR_NO_ITR:
		pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
		if (NULL == pItr) {
			DEBUG_FD_LOG_IF_ASSERT("gItrCtrl.Info = NULL");
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		if (count != FELICA_EDGE_OUT_SIZE) {
			DEBUG_FD_LOG_IF_ASSERT("count=%d", count);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
			return -EINVAL;
		}

		/* Begin disabling interrupts for protecting RFS and INT Flag */
		spin_lock_irqsave(&itr_lock, itr_lock_flag);

		local_buff[0] = pItr->RFS_Flag;
		local_buff[1] = pItr->INT_Flag;
		pItr->RFS_Flag = FELICA_EDGE_NON;
		pItr->INT_Flag = FELICA_EDGE_NON;

		/* End disabling interrupts for protecting RFS and INT Flag */
		spin_unlock_irqrestore(&itr_lock, itr_lock_flag);

		DEBUG_FD_LOG_TERMINAL_IF("[ITR] read() RFS=%d INT=%d", local_buff[0], local_buff[1]);

		ret_cpy = copy_to_user(buff, local_buff, FELICA_EDGE_OUT_SIZE);
		if (ret_cpy != 0) {
			DEBUG_FD_LOG_IF_ASSERT("copy_to_user() ret=%ld",ret_cpy);
			DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EFAULT);
			return -EFAULT;
		}
		else {
			ret = FELICA_EDGE_OUT_SIZE;
		}
		break;

#ifdef FELIACA_RAM_LOG_ON
	case FELICA_CTRL_MINOR_NO_DEBUG:
		ret = felicadd_debug_read_ram(
				  pfile
				, buff
				, count
				, poff
			  );
		break;
#endif /* FELIACA_RAM_LOG_ON */
// <Combo_chip> add S
    case SNFC_CTRL_MINOR_NO_HSEL:
        ret = SnfcCtrl_priv_dev_hsel_read(buff, count);
        break;
    case SNFC_CTRL_MINOR_NO_INTU:
        ret = SnfcCtrl_priv_dev_intu_read(buff, count);
        break;
    case SNFC_CTRL_MINOR_NO_AUTOPOLL:
        ret = SnfcCtrl_priv_dev_uartcc_state_read(buff, count);
        break;
    case SNFC_CTRL_MINOR_NO_AVAILABLE:
        ret = SnfcCtrl_priv_dev_available_read(buff, count);
        break;
// <Combo_chip> add E

	default:
		ret = -ENODEV;
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}
	
	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief INT interrupts handler
 *
 *   @par   Outline:\n
 *          Called when INT interrups from FeliCa device
 *
 *   @param[in]  i_req     IRQ number
 *   @param[in] *pv_devid  All-round pointer\n
 *                         (The argument which was set at the function "request_irq()")
 *   @retval IRQ_HANDLED  Interrupt was handled by this device
 *
 *   @par Special note
 *     - none
 *
 **/
FELICA_MT_WEAK
static irqreturn_t FeliCaCtrl_interrupt_INT(int i_req, void *pv_devid)
{
	struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
	int ret_sub = 0;
	unsigned char rdata = DFD_OUT_H;

	DEBUG_FD_LOG_ITR_FNC_ST("start");

	//<iDPower_009> add S
	wake_lock_timeout(&felicactrl_wake_lock, HZ);
	//<iDPower_009> add S
	
	if (NULL == pItr) {
		DEBUG_FD_LOG_ITR_ASSERT("pItr=NULL");
		DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
		return IRQ_HANDLED;
	}
	
	ret_sub = FeliCaCtrl_priv_terminal_int_read(&rdata);

	if ((ret_sub == 0) && (rdata == DFD_OUT_L)) {
		pItr->INT_Flag = FELICA_EDGE_L;
		wake_up_interruptible(&(pItr->RcvWaitQ));
		DEBUG_FD_LOG_TERMINAL_ITR("INT FELICA_EDGE_L");
	}

	DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
	return IRQ_HANDLED;
}

/**
 *   @brief RFS interrupts handler
 *
 *   @par   Outline:\n
 *          Called when RFS interrups from FeliCa device
 *
 *   @param[in]  i_req     IRQ number
 *   @param[in] *pv_devid  All-round pointer\n
 *                         (The argument which was set at the function "request_irq()")
 *   @retval IRQ_HANDLED  Interrupt was handled by this device
 *
 *   @par Special note
 *     - none
 *
 **/
#ifdef FELICA_SYS_HAVE_DETECT_RFS
FELICA_MT_WEAK
static irqreturn_t FeliCaCtrl_interrupt_RFS(int i_req, void *pv_devid)
{
	struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
	int ret_sub = 0;
	unsigned char rdata = DFD_OUT_H;

	DEBUG_FD_LOG_ITR_FNC_ST("start");
	
	if (NULL == pItr) {
		DEBUG_FD_LOG_ITR_ASSERT("pItr=NULL");
		DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
		return IRQ_HANDLED;
	}

	FeliCaCtrl_priv_interrupt_ctrl_disable(DFD_ITR_TYPE_RFS);

	/* Initialize chataring counter */
	pItr->RFS_L_cnt = 0;
	pItr->RFS_H_cnt = 0;

	ret_sub = FeliCaCtrl_priv_terminal_rfs_read(&rdata);
	if (ret_sub != 0) {
		FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		DEBUG_FD_LOG_ITR_ASSERT("rfs_read() ret=%d", ret_sub);
		DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
		return IRQ_HANDLED;
	}

	if (rdata == DFD_OUT_L) {
		pItr->RFS_L_cnt++;
	}
	else{
		pItr->RFS_H_cnt++;
	}

	/* start chataring timer */
	FeliCaCtrl_priv_dev_rfs_timer_start(jiffies + FELICA_TIMER_CHATA,
									FeliCaCtrl_timer_hnd_rfs_chata_low);

	DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
	return IRQ_HANDLED;
}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

// <Combo_chip> add S
/**
 *   @brief INTU interrupts handler
 *
 *   @par   Outline:\n
 *          Called when INTU interrups from SNFC device
 *
 *   @param[in]  i_req     IRQ number
 *   @param[in] *pv_devid  All-round pointer\n
 *                         (The argument which was set at the function "request_irq()")
 *   @retval IRQ_HANDLED  Interrupt was handled by this device
 *
 *   @par Special note
 *     - none
 *
 **/
FELICA_MT_WEAK
static irqreturn_t SnfcCtrl_interrupt_INTU(int i_req, void *pv_devid)
{
    struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gIntuCtrl.Info;
    unsigned int ret_sub = 0;
    unsigned int rdata = DFD_OUT_H;

    DEBUG_FD_LOG_ITR_FNC_ST("start");
    
    if (NULL == pItr) {
        DEBUG_FD_LOG_ITR_ASSERT("pItr=NULL");
        DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
        return IRQ_HANDLED;
    }
    
    ret_sub = SnfcCtrl_priv_terminal_intu_read(&rdata);

    if ((ret_sub == 0) && (rdata == FELICA_EDGE_L)) {
        pItr->INTU_Flag = FELICA_EDGE_L;
        DEBUG_FD_LOG_TERMINAL_ITR("INTU FELICA_EDGE_L");
    }
    else if ((ret_sub == 0) && (rdata == FELICA_EDGE_H)) {
        pItr->INTU_Flag = FELICA_EDGE_H;
        DEBUG_FD_LOG_TERMINAL_ITR("INTU FELICA_EDGE_H");
    }
    else {}

    wake_up_interruptible(&(pItr->RcvWaitQ));

    DEBUG_FD_LOG_ITR_FNC_ED("return=%d", IRQ_HANDLED);
    return IRQ_HANDLED;
}
// <Combo_chip> add E

/**
 *   @brief Generic timer handler
 *
 *   @par   Outline:\n
 *          Called when timeout for generic
 *
 *   @param[in] p  Address of the timer structure
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 *
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_timer_hnd_general(unsigned long p)
{
	struct FELICA_TIMER *pTimer;

	DEBUG_FD_LOG_ITR_FNC_ST("start");

	pTimer = (struct FELICA_TIMER *)p;
	if (pTimer != NULL) {
		wake_up_interruptible(&pTimer->wait);
	}

	DEBUG_FD_LOG_ITR_FNC_ED("return");
	return;
}

/**
 *   @brief RFS chataring timer handler(for low edge)
 *
 *   @par   Outline:\n
 *          Called when timeout for RFS chataring
 *
 *   @param[in] p  Address of the timer structure
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 *
 **/
#ifdef FELICA_SYS_HAVE_DETECT_RFS
FELICA_MT_WEAK
static void FeliCaCtrl_timer_hnd_rfs_chata_low(unsigned long p)
{
	struct FELICA_TIMER *pTimer = (struct FELICA_TIMER *)p;
	struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
	int ret_sub = 0;
	unsigned char rdata = DFD_OUT_H;

	DEBUG_FD_LOG_ITR_FNC_ST("start");

	if ((NULL == pItr) || (pTimer == NULL)) {
		FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		DEBUG_FD_LOG_ITR_ASSERT("pItr=%p pTimer=%p",pItr,pTimer);
		DEBUG_FD_LOG_ITR_FNC_ED("return");
		return;
	}

	ret_sub = FeliCaCtrl_priv_terminal_rfs_read(&rdata);
	if (ret_sub != 0) {
		FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		DEBUG_FD_LOG_ITR_ASSERT("rfs_read() ret=%d", ret_sub);
		DEBUG_FD_LOG_ITR_FNC_ED("return");
		return;
	}

	if (rdata == DFD_OUT_L) {
		pItr->RFS_L_cnt++;
	}
	else{
		pItr->RFS_H_cnt++;
	}

	if ((pItr->RFS_L_cnt + pItr->RFS_H_cnt) >= DFD_LOW_RFS_CHATA_CNT_MAX) {
		/* When reaching the specified number of times */
		if (pItr->RFS_L_cnt >= DFD_LOW_RFS_CHATA_CNT_WIN) {
			/* Chata result "EDGE_L" */
			pItr->RFS_Flag = FELICA_EDGE_L;
			wake_up_interruptible(&(pItr->RcvWaitQ));

			DEBUG_FD_LOG_TERMINAL_ITR("RFS FELICA_EDGE_L");

			/* Initialize chataring counter */
			pItr->RFS_L_cnt = 0;
			pItr->RFS_H_cnt = 0;

			/* start chataring timer */
			FeliCaCtrl_priv_dev_rfs_timer_start(
									jiffies + FELICA_TIMER_CHATA_DELAY,
									FeliCaCtrl_timer_hnd_rfs_chata_high);
		}
		else{
			/* Initialize chataring counter */
			pItr->RFS_L_cnt = 0;
			pItr->RFS_H_cnt = 0;

			/* Chata result "EDGE_H" */
			FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		}
	}
	else {
		/* When not reaching the specified number of times */
		/* restart chataring timer */
		FeliCaCtrl_priv_dev_rfs_timer_start(jiffies + FELICA_TIMER_CHATA,
										FeliCaCtrl_timer_hnd_rfs_chata_low);
	}

	DEBUG_FD_LOG_ITR_FNC_ED("return");
	return;
}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

#ifdef FELICA_SYS_HAVE_DETECT_RFS
/**
 *   @brief RFS chataring timer handler(for high edge)
 *
 *   @par   Outline:\n
 *          Called when timeout for RFS chataring
 *
 *   @param[in] p  Address of the timer structure
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 *
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_timer_hnd_rfs_chata_high(unsigned long p)
{
	struct FELICA_TIMER *pTimer = (struct FELICA_TIMER *)p;
	struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;
	int ret_sub = 0;
	unsigned char rdata = DFD_OUT_H;

	DEBUG_FD_LOG_ITR_FNC_ST("start");

	if ((NULL == pItr) || (pTimer == NULL)) {
		FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		DEBUG_FD_LOG_ITR_ASSERT("pItr=%p pTimer=%p",pItr,pTimer);
		DEBUG_FD_LOG_ITR_FNC_ED("return");
		return;
	}

	ret_sub = FeliCaCtrl_priv_terminal_rfs_read(&rdata);
	if (ret_sub != 0) {
		FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		DEBUG_FD_LOG_ITR_ASSERT("rfs_read() ret=%d", ret_sub);
		DEBUG_FD_LOG_ITR_FNC_ED("return");
		return;
	}

	if (rdata == DFD_OUT_L) {
		pItr->RFS_L_cnt++;
	}
	else{
		pItr->RFS_H_cnt++;
	}

	if ((pItr->RFS_L_cnt + pItr->RFS_H_cnt) >= DFD_HIGH_RFS_CHATA_CNT_MAX) {
		/* When reaching the specified number of times */
		if (pItr->RFS_H_cnt >= DFD_HIGH_RFS_CHATA_CNT_WIN) {
			/* Chata result "EDGE_H" */
			pItr->RFS_Flag = FELICA_EDGE_H;
			wake_up_interruptible(&(pItr->RcvWaitQ));

			DEBUG_FD_LOG_TERMINAL_ITR("RFS FELICA_EDGE_H");

			/* Initialize chataring counter */
			pItr->RFS_L_cnt = 0;
			pItr->RFS_H_cnt = 0;

			FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
		}
		else{
			/* Initialize chataring counter */
			pItr->RFS_L_cnt = 0;
			pItr->RFS_H_cnt = 0;

			/* Chata result "EDGE_L" */
			FeliCaCtrl_priv_dev_rfs_timer_start(
									jiffies + FELICA_TIMER_CHATA_DELAY,
									FeliCaCtrl_timer_hnd_rfs_chata_high);
		}
	}
	else {
		/* When not reaching the specified number of times */
		/* restart chataring timer */
		FeliCaCtrl_priv_dev_rfs_timer_start(jiffies + FELICA_TIMER_CHATA,
										FeliCaCtrl_timer_hnd_rfs_chata_high);
	}

	DEBUG_FD_LOG_ITR_FNC_ED("return");
	return;
}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

/**
 *   @brief Enable FeliCa control
 *
 *   @param[in] *pTimer  pointer of timer data(CEN terminal control timer)
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EBUSY   Device or resource busy
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_ctrl_enable(struct FELICA_TIMER *pTimer)
{
	int ret = 0;
	int ret_sub = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if (NULL == pTimer) {
		DEBUG_FD_LOG_PRIV_ASSERT("pTimer==NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	ret_sub = timer_pending(&pTimer->Timer);

	if (ret_sub != 0) {
		DEBUG_FD_LOG_PRIV_ASSERT("timer_pending() ret=%d", ret_sub);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EBUSY);
		return -EBUSY;
	}

	ret = FeliCaCtrl_priv_terminal_cen_write(DFD_OUT_H);

	if (ret >= 0) {
		/* wait cen timer */
		pTimer->Timer.function = FeliCaCtrl_timer_hnd_general;
		pTimer->Timer.data	   = (unsigned long)pTimer;
		mod_timer(&pTimer->Timer, jiffies + FELICA_TIMER_CEN_TERMINAL_WAIT);

		wait_event_interruptible(
			pTimer->wait,
			(timer_pending(&pTimer->Timer) == 0));

		ret_sub = timer_pending(&pTimer->Timer);
		if (ret_sub == 1) {
			del_timer(&pTimer->Timer);
		}
#ifdef FELICA_SYS_HAVE_DETECT_RFS
		FeliCaCtrl_priv_interrupt_ctrl_enable(DFD_ITR_TYPE_RFS);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Disable FeliCa control
 *
 *   @param[in] *pPonTimer  pointer of timer data(PON terminal control timer)
 *   @param[in] *pCenTimer  pointer of timer data(PON terminal control timer)
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EBUSY   Device or resource busy
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_ctrl_unenable(
							struct FELICA_TIMER* pPonTimer,
							struct FELICA_TIMER* pCenTimer)
{
	int ret = 0;
	int ret_sub = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pPonTimer) || (NULL == pCenTimer)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pPonTimer=%p pCenTimer=%p", pPonTimer, pCenTimer);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	(void)FeliCaCtrl_priv_ctrl_offline();

	/* wait pon timer */
	pPonTimer->Timer.function = FeliCaCtrl_timer_hnd_general;
	pPonTimer->Timer.data	   = (unsigned long)pPonTimer;
	mod_timer(&pPonTimer->Timer, jiffies + FELICA_TIMER_PON_TERMINAL_WAIT);

	wait_event_interruptible(
		pPonTimer->wait,
		(timer_pending(&pPonTimer->Timer) == 0));

	ret_sub = timer_pending(&pPonTimer->Timer);
	if (ret_sub == 1) {
		del_timer(&pPonTimer->Timer);
	}

#ifdef FELICA_SYS_HAVE_DETECT_RFS
	FeliCaCtrl_priv_interrupt_ctrl_disable(DFD_ITR_TYPE_RFS);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

	ret = FeliCaCtrl_priv_terminal_cen_write(DFD_OUT_L);

	if (ret == 0) {
		pCenTimer->Timer.function = FeliCaCtrl_timer_hnd_general;
		pCenTimer->Timer.data	   = (unsigned long)pCenTimer;
		mod_timer(&pCenTimer->Timer, jiffies + FELICA_TIMER_CEN_TERMINAL_WAIT);

		wait_event_interruptible(
			pCenTimer->wait,
			(timer_pending(&pCenTimer->Timer) == 0));

		ret_sub = timer_pending(&pCenTimer->Timer);
		if (ret_sub == 1) {
			del_timer(&pCenTimer->Timer);
		}
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Enable FeliCa IC function
 *   In case of P2P, waiting after PON=H causes timeout error by peer, so
 *   `need_wait' parameter is needed.
 *
 *   @param[in] *pTimer  pointer of timer data(PON terminal control timer)
 *   @param[in] need_wait non-zero waits after PON=H
 *
 *   @retval 0     Normal end
 *   @retval -EIO  I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_ctrl_online(struct FELICA_TIMER *pTimer, int need_wait)    /* npdc300054413 mod */
{
	int ret = 0;
	int ret_sub = 0;
	unsigned char rdata;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if (NULL == pTimer) {
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	ret_sub = FeliCaCtrl_priv_terminal_cen_read(&rdata);
	if ((ret_sub != 0) || (rdata == DFD_OUT_L)) {
		ret = -EIO;
		DEBUG_FD_LOG_PRIV_ASSERT("cen_read() ret=%d rdata=%d", ret_sub, rdata);
	}

	if (ret == 0) {
		ret_sub = FeliCaCtrl_priv_terminal_pon_write(DFD_OUT_H);

		if (ret_sub != 0) {
			ret = ret_sub;
			DEBUG_FD_LOG_PRIV_ASSERT("pon_write() ret=%d", ret_sub);
		}
	}

	if ((ret == 0) && need_wait) {                                           /* npdc300054413 add */

		pTimer->Timer.function = FeliCaCtrl_timer_hnd_general;
		pTimer->Timer.data     = (unsigned long)pTimer;
		mod_timer(&pTimer->Timer, jiffies + FELICA_TIMER_PON_TERMINAL_WAIT);

		wait_event_interruptible(
			pTimer->wait,
			(timer_pending(&pTimer->Timer) == 0));

		ret_sub = timer_pending(&pTimer->Timer);
		if (ret_sub == 1) {
			del_timer(&pTimer->Timer);
		}
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Disable FeliCa IC function
 *
 *   @param  none
 *
 *   @retval 0     Normal end
 *   @retval -EIO  I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_ctrl_offline(void)
{
	int ret = 0;
	int ret_sub = 0;
	unsigned char rdata;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	ret_sub = FeliCaCtrl_priv_terminal_cen_read(&rdata);
	if ((ret_sub != 0) || (rdata == DFD_OUT_L)) {
		ret = -EIO;
		DEBUG_FD_LOG_PRIV_ASSERT("cen_read() ret=%d rdata=%d", ret_sub, rdata);
	}

	if (ret == 0) {
		ret_sub = FeliCaCtrl_priv_terminal_pon_write(DFD_OUT_L);

		if (ret_sub != 0) {
			ret = ret_sub;
			DEBUG_FD_LOG_PRIV_ASSERT("pon_write() ret=%d", ret_sub);
		}
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief SNFC HSEL setting function
 *
 *   @param[in] 
 *
 *   @retval 0     Normal end
 *   @retval -EIO  I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_ctrl_tofelica(void)
{
    int ret = 0;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    ret = SnfcCtrl_priv_terminal_hsel_write(DFD_OUT_L);

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief SNFC HSEL setting function
 *
 *   @param[in]
 *
 *   @retval 0     Normal end
 *   @retval -EIO  I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_ctrl_tosnfc(void)
{
    int ret = 0;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    ret = SnfcCtrl_priv_terminal_hsel_write(DFD_OUT_H);

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}
// <Combo_chip> add E

/**
 *   @brief Read CEN Terminal settings
 *
 *   @param[out] cen_data  Status of the CEN terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_terminal_cen_read(unsigned char *cen_data)
{
	int ret = 0;
	u8 w_rdata;
// <iDPower_002> add S
	int ret_sub = 0;
	u8 buff_for_data_read[] = {FELICA_I2C_DATA_READ};
	struct i2c_msg msg[2];
// <iDPower_002> add E

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

// <iDPower_002> add S
	if (NULL == cen_client) {
		DEBUG_FD_LOG_PRIV_ASSERT("cen_client=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
// <iDPower_002> add E

	if (NULL == cen_data) {
		DEBUG_FD_LOG_PRIV_ASSERT("cen_data=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

// <iDPower_002> mod S
//	ret = subpmic_i2c_read_u8(FELICA_SUBPMIC_PORT_CEN, &w_rdata);
//	if (ret != 0) {
//		DEBUG_FD_LOG_PRIV_ASSERT("subpmic_i2c_read_u8 ret=%d", ret);
//		FeliCaCtrl_priv_alarm(DFD_ALARM_SUBPMIC_READ);
//		ret = -EIO;
//	}
	msg[0].addr	= cen_client->addr;
	msg[0].len	= 1;
	msg[0].flags	= 0;
	msg[0].buf	= buff_for_data_read;
	msg[1].addr	= cen_client->addr;
	msg[1].len	= 1;
	msg[1].flags	= I2C_M_RD;
	msg[1].buf	= &w_rdata;
	ret_sub = i2c_transfer(cen_client->adapter, msg, 2);
	if (ret_sub < 0) {
		DEBUG_FD_LOG_PRIV_ASSERT("i2c_transfer ret=%d", ret_sub);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EIO);
		return -EIO;
	}
// <iDPower_002> mod E
	else {
		DEBUG_FD_LOG_TERMINAL_DEV("[CEN] get=%d", (w_rdata & FELICA_SUBPMIC_FCCEN));
		if ((w_rdata & FELICA_SUBPMIC_FCCEN) == FELICA_SUBPMIC_FCCEN) {
			*cen_data = DFD_OUT_H;
		}
		else {
			*cen_data = DFD_OUT_L;
		}
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Read RFS Terminal settings
 *
 *   @param[out] rfs_data  Status of the RFS terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_terminal_rfs_read(unsigned char *rfs_data)
{
	int ret = 0;
	int w_rdata;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if (NULL == rfs_data) {
		DEBUG_FD_LOG_PRIV_ASSERT("rfs_data=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	w_rdata = gpio_get_value(FELICA_GPIO_PORT_RFS);
	DEBUG_FD_LOG_TERMINAL_DEV("[RFS] get=%d", w_rdata);

	if (w_rdata == 0) {
		*rfs_data = DFD_OUT_L;
	}
	else {
		*rfs_data = DFD_OUT_H;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Read INT Terminal settings
 *
 *   @param[out] int_data  Status of the INT terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_terminal_int_read(unsigned char *int_data)
{
	int ret = 0;
	int w_rdata;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if (NULL == int_data) {
		DEBUG_FD_LOG_PRIV_ASSERT("int_data=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	w_rdata = gpio_get_value(FELICA_GPIO_PORT_INT);
	DEBUG_FD_LOG_TERMINAL_DEV("[INT] get=%d", w_rdata);

	if (w_rdata == 0) {
		*int_data = DFD_OUT_L;
	}
	else {
		*int_data = DFD_OUT_H;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief Read HSEL Terminal settings
 *
 *   @param[out] hsel_data  Status of the HSEL terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_terminal_hsel_read(unsigned int *hsel_data)
{
    unsigned int ret = 0;
    unsigned int w_rdata;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (NULL == hsel_data) {
        DEBUG_FD_LOG_PRIV_ASSERT("hsel_data=NULL");
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    w_rdata = gpio_get_value(SNFC_GPIO_PORT_HSEL);
    DEBUG_FD_LOG_TERMINAL_DEV("[HSEL] get=%d", w_rdata);

    if (w_rdata == 0) {
        *hsel_data = DFD_OUT_L;
    }
    else {
        *hsel_data = DFD_OUT_H;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief Read HSEL Terminal settings
 *
 *   @param[out] hsel_data  Status of the HSEL terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
int SnfcCtrl_terminal_hsel_read_dd(unsigned int *hsel_data)
{
    DEBUG_FD_LOG_IF_FNC_ST("start");
    
    if (NULL == hsel_data) {
        DEBUG_FD_LOG_IF_ASSERT("hsel_data=NULL");
        DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    *hsel_data = gpio_get_value(SNFC_GPIO_PORT_HSEL);
    
    DEBUG_FD_LOG_IF_FNC_ED("[HSEL] get=%d", *hsel_data);

    return 0;
}

/**
 *   @brief Write UARTCC state Terminal settings
 *
 *   @param[in] poll_data  Value of the UARTCC state terminal to set
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_terminal_available_read(unsigned int *available_data, struct FELICA_TIMER *pTimer)
{
    int ret_sub = 0;
    unsigned char rfs_data = 0;                    /* RFS data              */
    unsigned char cen_data = 0;                    /* CEN data              */
    unsigned int restart_val = 0;                  /* RFS=H,CEN=H,FeliCa=No Use*/
    unsigned int uartcc_state = 0;                 /* uartcc_state          */

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (NULL == pTimer) {
        DEBUG_FD_LOG_PRIV_ASSERT("pTimer==NULL");
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    if (NULL == available_data) {
        DEBUG_FD_LOG_PRIV_ASSERT("available_data=NULL");
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }
    
    ret_sub = timer_pending(&pTimer->Timer);
    if (ret_sub != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("timer_pending() ret=%d", ret_sub);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EBUSY);
        return -EBUSY;
    }

    while (1) {
        ret_sub = FeliCaCtrl_priv_terminal_rfs_read(&rfs_data);
        if (ret_sub < 0) {
            DEBUG_FD_LOG_PRIV_ASSERT("rfs_read() ret=%d",ret_sub);
            DEBUG_FD_LOG_PRIV_FNC_ED("return");
            return ret_sub;
        }
        
        ret_sub = FeliCaCtrl_priv_terminal_cen_read(&cen_data);
        if (ret_sub < 0) {
            DEBUG_FD_LOG_PRIV_ASSERT("cen_read() ret=%d",ret_sub);
            DEBUG_FD_LOG_PRIV_FNC_ED("return");
            return ret_sub;
        }
        
        Uartdrv_uartcc_state_read_dd(&uartcc_state);

        switch (uartcc_state) {
            case SNFC_STATE_NFC_POLLING:
            case SNFC_STATE_IDLE:
                if((rfs_data == DFD_OUT_H) && (cen_data == DFD_OUT_H)) {
                    restart_val = 1;
                }
                break;
            case SNFC_STATE_WAITING_POLL_END:
            case SNFC_STATE_FELICA_ACTIVE:
            default:
                break;
        }
        
        if (restart_val == 1) {
            *available_data = restart_val;
            break;
        }
        else {
            /* wait timer 500ms for FeliCa finished */
            pTimer->Timer.function = FeliCaCtrl_timer_hnd_general;
            pTimer->Timer.data	   = (unsigned long)pTimer;
            mod_timer(&pTimer->Timer, jiffies + FELICA_TIMER_AVAILABLE_TERMINAL_WAIT);
            
            wait_event_interruptible(
                pTimer->wait,
                (timer_pending(&pTimer->Timer) == 0));
                
            ret_sub = timer_pending(&pTimer->Timer);
            if (ret_sub == 1) {
                del_timer(&pTimer->Timer);
            }
        }
    }/* while(1) */
    
    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret_sub);
    return ret_sub;
}

/**
 *   @brief Read INT Terminal settings
 *
 *   @param[out] int_data  Status of the INT terminal
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_terminal_intu_read(unsigned int *intu_data)
{
    unsigned int ret = 0;
    unsigned int w_rdata;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (NULL == intu_data) {
        DEBUG_FD_LOG_PRIV_ASSERT("intu_data=NULL");
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    w_rdata = gpio_get_value(SNFC_GPIO_PORT_INTU);
    DEBUG_FD_LOG_TERMINAL_DEV("[INTU] get=%d", w_rdata);

    if (w_rdata == 0) {
        *intu_data = DFD_OUT_L;
    }
    else {
        *intu_data = DFD_OUT_H;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}
// <Combo_chip> add E

/**
 *   @brief Write CEN Terminal settings
 *
 *   @param[in] cen_data  Value of the CEN terminal to set
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_terminal_cen_write(unsigned char cen_data)
{
	int ret = 0;
	u8 w_wdata = 0;
// <iDPower_002> add S
	int ret_sub = 0;
	u8 buff_for_to_write_mode[] = {FELICA_I2C_CHANGE_STATUS, FELICA_I2C_WRITE_MODE};
	u8 buff_for_enable_write[]  = {FELICA_I2C_ENABLE_WRITE};
	u8 buff_for_data_write[2]   = {FELICA_I2C_DATA_WRITE, w_wdata};
	u8 buff_for_disable_write[] = {FELICA_I2C_DISABLE_WRITE};
	u8 buff_for_to_read_mode[]  = {FELICA_I2C_CHANGE_STATUS, FELICA_I2C_READ_MODE};

	struct i2c_msg msg_for_to_write_mode[1];
	struct i2c_msg msg_for_enable_write[1];
	struct i2c_msg msg_for_data_write[1];
	struct i2c_msg msg_for_disable_write[1];
	struct i2c_msg msg_for_to_read_mode[1];
// <iDPower_002> add E

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

// <iDPower_002> add S
	if (NULL == cen_client) {
		DEBUG_FD_LOG_PRIV_ASSERT("cen_client=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
// <iDPower_002> add E

	DEBUG_FD_LOG_PRIV_FNC_PARAM("cen_data=%d", cen_data);

	switch (cen_data) {
	case DFD_OUT_H:
		w_wdata = FELICA_SUBPMIC_FCCEN;
		break;
	case DFD_OUT_L:
		w_wdata = 0;
		break;
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("cen_data=%d", cen_data);
		ret = -EINVAL;
		break;
	}

// <iDPower_002> mod S
//	if (ret == 0) {
//		DEBUG_FD_LOG_TERMINAL_DEV("[CEN] set=%d", w_wdata);
//		ret = subpmic_i2c_write_u8(FELICA_SUBPMIC_PORT_CEN, w_wdata);
//		if (ret != 0) {
//			DEBUG_FD_LOG_PRIV_ASSERT("subpmic_i2c_write_u8 ret=%d",ret);
//			FeliCaCtrl_priv_alarm(DFD_ALARM_SUBPMIC_WRITE);
//			ret = -EIO;
//		}
//	}
	if (ret == 0) {
		DEBUG_FD_LOG_TERMINAL_DEV("i2c_transfer for to write mode.");
		msg_for_to_write_mode[0].addr	= cen_client->addr;
		msg_for_to_write_mode[0].len	= 2;
		msg_for_to_write_mode[0].flags	= 0;
		msg_for_to_write_mode[0].buf	= buff_for_to_write_mode;
		ret_sub = i2c_transfer(cen_client->adapter, msg_for_to_write_mode, 1);
		if (ret_sub < 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("i2c_transfer ret=%d",ret_sub);
			ret = -EIO;
		}
	}

	if (ret >= 0) {
		DEBUG_FD_LOG_TERMINAL_DEV("i2c_transfer for enable write.");
		msg_for_enable_write[0].addr	= cen_client->addr;
		msg_for_enable_write[0].len		= 1;
		msg_for_enable_write[0].flags	= 0;
		msg_for_enable_write[0].buf		= buff_for_enable_write;
		ret_sub = i2c_transfer(cen_client->adapter, msg_for_enable_write, 1);
		if (ret_sub < 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("i2c_transfer ret=%d",ret_sub);
			ret = -EIO;
		}
	}

	if (ret >= 0) {
		DEBUG_FD_LOG_TERMINAL_DEV("i2c_transfer for data write.");
		DEBUG_FD_LOG_TERMINAL_DEV("[CEN] set=%d", w_wdata);
		msg_for_data_write[0].addr	= cen_client->addr;
		msg_for_data_write[0].len	= 2;
		msg_for_data_write[0].flags	= 0;
		msg_for_data_write[0].buf	= buff_for_data_write;
		buff_for_data_write[1] 		= w_wdata;
		ret_sub = i2c_transfer(cen_client->adapter, msg_for_data_write, 1);
		if (ret_sub < 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("i2c_transfer ret=%d",ret_sub);
			ret = -EIO;
		}
	}

	if (ret >= 0) {
		ret_sub = FeliCaCtrl_priv_acknowledge_polling();
		if (ret_sub < 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("FeliCaCtrl_priv_acknowledge_polling ret=%d",ret_sub);
			ret = -EIO;
		}
	}

	if (ret == 0) {
		DEBUG_FD_LOG_TERMINAL_DEV("i2c_transfer for disable write.");
		msg_for_disable_write[0].addr	= cen_client->addr;
		msg_for_disable_write[0].len	= 1;
		msg_for_disable_write[0].flags	= 0;
		msg_for_disable_write[0].buf	= buff_for_disable_write;
		ret_sub = i2c_transfer(cen_client->adapter, msg_for_disable_write, 1);
		if (ret_sub < 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("i2c_transfer ret=%d",ret_sub);
			ret = -EIO;
		}
	}

	if (ret >= 0) {
		DEBUG_FD_LOG_TERMINAL_DEV("i2c_transfer for read mode.");
		msg_for_to_read_mode[0].addr	= cen_client->addr;
		msg_for_to_read_mode[0].len		= 2;
		msg_for_to_read_mode[0].flags	= 0;
		msg_for_to_read_mode[0].buf		= buff_for_to_read_mode;
		ret_sub = i2c_transfer(cen_client->adapter, msg_for_to_read_mode, 1);
		if (ret_sub < 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("i2c_transfer ret=%d",ret_sub);
			ret = -EIO;
		}
	}
// <iDPower_002> mod E

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Write PON Terminal settings
 *
 *   @param[in] pon_data  Value of the PON terminal to set
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_terminal_pon_write(unsigned char pon_data)
{
	int ret = 0;
	int ret_sub = 0;
	u8 w_wdata = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("pon_data=%d", pon_data);

	switch (pon_data) {
	case DFD_OUT_L:
		w_wdata = 0;
		break;
	case DFD_OUT_H:
		w_wdata = 1;
		break;
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("pon_data=%d", pon_data);
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		ret_sub = gpio_request(FELICA_GPIO_PORT_PON, FELICA_GPIO_STR_PON);
		if (ret_sub != 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("gpio_request() ret=%d", ret_sub);
			ret = -EIO;
		}
		else {
			DEBUG_FD_LOG_TERMINAL_DEV("[PON] set=%d", w_wdata);
			ret_sub = gpio_direction_output(FELICA_GPIO_PORT_PON, w_wdata);
			if (ret_sub != 0) {
				DEBUG_FD_LOG_PRIV_ASSERT("gpio_direction_output() ret=%d", ret_sub);
				ret = -EIO;
			}
			gpio_free(FELICA_GPIO_PORT_PON);
		}
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief Write HSEL Terminal settings
 *
 *   @param[in] pon_data  Value of the HSEL terminal to set
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_terminal_hsel_write(unsigned int hsel_data)
{
    unsigned int ret = 0;
    unsigned int ret_sub = 0;
    u8 w_wdata = 0;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");
    DEBUG_FD_LOG_PRIV_FNC_PARAM("hsel_data=%d", hsel_data);

    switch (hsel_data) {
    case DFD_OUT_L:
        w_wdata = 0;
        break;
    case DFD_OUT_H:
        w_wdata = 1;
        break;
    default:
        DEBUG_FD_LOG_PRIV_ASSERT("hsel_data=%d", hsel_data);
        ret = -EINVAL;
        break;
    }

    if (ret == 0) {
        ret_sub = gpio_request(SNFC_GPIO_PORT_HSEL, SNFC_GPIO_STR_HSEL);
        if (ret_sub != 0) {
            DEBUG_FD_LOG_PRIV_ASSERT("gpio_request() ret=%d", ret_sub);
            ret = -EIO;
        }
        else {
            DEBUG_FD_LOG_TERMINAL_DEV("[HSEL] set=%d", w_wdata);
            ret_sub = gpio_direction_output(SNFC_GPIO_PORT_HSEL, w_wdata);
            if (ret_sub != 0) {
                DEBUG_FD_LOG_PRIV_ASSERT("gpio_direction_output() ret=%d", ret_sub);
                ret = -EIO;
            }
            gpio_free(SNFC_GPIO_PORT_HSEL);
        }
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}
// <Combo_chip> add E

/**
 *   @brief Initialize interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_interrupt_ctrl_init(unsigned char itr_type)
{
	int ret = 0;
	int ret_sub = 0;
	unsigned gpio_type = 0;
	const char *gpio_label = NULL;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("itr_type=%d", itr_type);

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
		gpio_type = FELICA_GPIO_PORT_RFS;
		gpio_label = FELICA_GPIO_STR_RFS;
		break;
	case DFD_ITR_TYPE_INT:
		gpio_type = FELICA_GPIO_PORT_INT;
		gpio_label = FELICA_GPIO_STR_INT;
		break;
// <Combo_chip> add S
	case DFD_ITR_TYPE_INTU:
		gpio_type = SNFC_GPIO_PORT_INTU;
		gpio_label = SNFC_GPIO_STR_INTU;
		break;
// <Combo_chip> add E
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		ret_sub = gpio_request(gpio_type, gpio_label);
		if (ret_sub != 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("gpio_request() ret=%d", ret_sub);
			ret = -EIO;
		}
		else {
			ret_sub = gpio_direction_input(gpio_type);
			if (ret_sub != 0) {
				DEBUG_FD_LOG_PRIV_ASSERT("gpio_direction_input() ret=%d", ret_sub);
				ret = -EIO;
			}
			else {
				ret_sub = gpio_to_irq(gpio_type);
				if (ret_sub < 0) {
					DEBUG_FD_LOG_PRIV_ASSERT("gpio_to_irq() ret=%d", ret_sub);
					ret = -EIO;
				}
				else {
					gFeliCaItrInfo[itr_type].irq = ret_sub;
				}
			}
			if (ret != 0) {
				gpio_free(gpio_type);
			}
		}
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Exit interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_interrupt_ctrl_exit(unsigned char itr_type)
{
	int ret = 0;
	unsigned gpio_type = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("itr_type=%d", itr_type);

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
		gpio_type = FELICA_GPIO_PORT_RFS;
		break;
	case DFD_ITR_TYPE_INT:
		gpio_type = FELICA_GPIO_PORT_INT;
		break;
// <Combo_chip> add S
	case DFD_ITR_TYPE_INTU:
		gpio_type = SNFC_GPIO_PORT_INTU;
		break;
// <Combo_chip> add E
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("itr_type=%d", itr_type);
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		gpio_free(gpio_type);
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return;
}

/**
 *   @brief Register interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_interrupt_ctrl_regist(unsigned char itr_type)
{
	int ret = 0;
	int ret_sub = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("itr_type=%d", itr_type);

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
	case DFD_ITR_TYPE_INT:
// <Combo_chip> add S
	case DFD_ITR_TYPE_INTU:
// <Combo_chip> add E
		break;
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("itr_type=%d", itr_type);
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
//
////		irq_set_irq_type(
////						gFeliCaItrInfo[itr_type].irq,
////						gFeliCaItrInfo[itr_type].flags);
//
		ret_sub = request_irq(
						gFeliCaItrInfo[itr_type].irq,
						gFeliCaItrInfo[itr_type].handler,
						gFeliCaItrInfo[itr_type].flags,
						gFeliCaItrInfo[itr_type].name,
						gFeliCaItrInfo[itr_type].dev);
		if (ret_sub != 0)
		{
			DEBUG_FD_LOG_PRIV_ASSERT("request_irq() ret=%d", ret_sub);
			FeliCaCtrl_priv_alarm(DFD_ALARM_INTERRUPT_REQUEST);
			ret = -EIO;
		}

		enable_irq_wake(gFeliCaItrInfo[itr_type].irq);
//////		enable_irq(gFeliCaItrInfo[itr_type].irq);
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Unregister interrupt control
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_interrupt_ctrl_unregist(unsigned char itr_type)
{
	int ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("itr_type=%d", itr_type);

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
	case DFD_ITR_TYPE_INT:
        case DFD_ITR_TYPE_INTU:
		break;
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("itr_type=%d", itr_type);
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		free_irq(
				gFeliCaItrInfo[itr_type].irq,
				gFeliCaItrInfo[itr_type].dev);
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return;
}

// <iDPower_002> add S
/**
 *   @brief Acknowledge polling
 *
 *   @param none
 *
 *   @retval 0        Normal end
 *   @retval <0       error
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_acknowledge_polling()
{
	int ret = 0;
	int i;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if (NULL == cen_client) {
		DEBUG_FD_LOG_PRIV_ASSERT("cen_client=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	for (i = 0; i < 5; i++) {
		udelay(1000);
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}
// <iDPower_002> add E

#ifdef FELICA_SYS_HAVE_DETECT_RFS
/**
 *   @brief enable interrupt
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_interrupt_ctrl_enable(unsigned char itr_type)
{
	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("itr_type=%d", itr_type);

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
	case DFD_ITR_TYPE_INT:
		enable_irq(gFeliCaItrInfo[itr_type].irq);
		break;
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("itr_type=%d", itr_type);
		break;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return;
}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

#ifdef FELICA_SYS_HAVE_DETECT_RFS
/**
 *   @brief disable interrupt
 *
 *   @param[in] itr_type  interrupt type
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_interrupt_ctrl_disable(unsigned char itr_type)
{
	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("itr_type=%d", itr_type);

	switch (itr_type) {
	case DFD_ITR_TYPE_RFS:
	case DFD_ITR_TYPE_INT:
		disable_irq_nosync(gFeliCaItrInfo[itr_type].irq);
		break;
	default:
		DEBUG_FD_LOG_PRIV_ASSERT("itr_type=%d", itr_type);
		break;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return;
}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

/**
 *   @brief Initialize timer data
 *
 *   @param[in] pTimer  timer data
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_drv_timer_data_init(struct FELICA_TIMER *pTimer)
{
	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if (pTimer != NULL) {
		init_timer(&pTimer->Timer);
		init_waitqueue_head(&pTimer->wait);
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return;
}

#ifdef FELICA_SYS_HAVE_DETECT_RFS
/**
 *   @brief Start RFS chataring timer
 *
 *   @param[in] expires   expires of setting timer struct
 *   @param[in] function  function of setting timer struct
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_dev_rfs_timer_start(
						unsigned long expires,
						void (*function)(unsigned long))
{
	struct FELICA_CTRLDEV_ITR_INFO *pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == function) || (NULL == pItr)) {
		DEBUG_FD_LOG_PRIV_ASSERT("function=%p pItr=%p", function, pItr);
		DEBUG_FD_LOG_PRIV_FNC_ED("return");
		return;
	}
	DEBUG_FD_LOG_PRIV_FNC_PARAM("expires=%ld jiffies=%ld", expires, jiffies);

	pItr->RFS_Timer.Timer.function = function;
	pItr->RFS_Timer.Timer.data = (unsigned long)&pItr->RFS_Timer;

	mod_timer(&pItr->RFS_Timer.Timer, expires);

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return;
}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

/**
 *   @brief cleanup driver setup
 *
 *   @par   Outline:\n
 *          For cleaning device driver setup info
 *
 *   @param[in] cleanup  cleanup point
 *
 *   @retval none
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_drv_init_cleanup(int cleanup)
{
	DEBUG_FD_LOG_PRIV_FNC_ST("start");
	DEBUG_FD_LOG_PRIV_FNC_PARAM("cleanup=%d",cleanup);

	/* cleanup */
	switch (cleanup) {
	case DFD_CLUP_ALL:
// <Combo_chip> add S
	case DFD_CLUP_DEV_DESTORY_AVAILABLE:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_AVAILABLE));
	case DFD_CLUP_DEV_DESTORY_POLL:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_AUTOPOLL));
	case DFD_CLUP_DEV_DESTORY_HSEL:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_HSEL));
	case DFD_CLUP_DEV_DESTORY_INTU:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_INTU));
	case DFD_CLUP_DEV_DESTORY_SNFC_CEN:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_CEN));
	case DFD_CLUP_DEV_DESTORY_SNFC_RFS:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_RFS));
	case DFD_CLUP_DEV_DESTORY_SNFC_PON:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, SNFC_CTRL_MINOR_NO_PON));
// <Combo_chip> add E
	case DFD_CLUP_DEV_DESTORY_CFG:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CFG_MAJOR_NO, FELICA_CFG_MINOR_NO_CFG));
	case DFD_CLUP_DEV_DESTORY_RWS:
		device_destroy(
			felica_class,
			MKDEV(FELICA_RWS_MAJOR_NO, FELICA_RWS_MINOR_NO_RWS));
#ifdef FELIACA_RAM_LOG_ON
		device_destroy(
			felica_class,
			MKDEV(FELICA_COMM_MAJOR_NO, FELICA_COMM_MINOR_NO_DEBUG));
#endif /* FELIACA_RAM_LOG_ON */
	case DFD_CLUP_DEV_DESTORY_COMM:
		device_destroy(
			felica_class,
			MKDEV(FELICA_COMM_MAJOR_NO, FELICA_COMM_MINOR_NO_COMM));
	case DFD_CLUP_DEV_DESTORY_ITR:
#ifdef FELIACA_RAM_LOG_ON
		device_destroy(
			felica_class,
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_DEBUG));
#endif /* FELIACA_RAM_LOG_ON */
		device_destroy(
			felica_class,
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_ITR));
	case DFD_CLUP_DEV_DESTORY_RFS:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_RFS));
	case DFD_CLUP_DEV_DESTORY_CEN:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_CEN));
	case DFD_CLUP_DEV_DESTORY_PON:
		device_destroy(
			felica_class,
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_PON));
	case DFD_CLUP_CLASS_DESTORY:
		class_destroy(felica_class);
// <Combo_chip> add S
	case DFD_CLUP_CDEV_DEL_SNFC_PON:
		cdev_del(&g_cdev_npon);
	case DFD_CLUP_UNREG_CDEV_SNFC_PON:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_PON),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_SNFC_RFS:
		cdev_del(&g_cdev_nrfs);
	case DFD_CLUP_UNREG_CDEV_SNFC_RFS:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_RFS),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_SNFC_CEN:
		cdev_del(&g_cdev_ncen);
	case DFD_CLUP_UNREG_CDEV_SNFC_CEN:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_CEN),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_AVAILABLE:
		cdev_del(&g_cdev_navailable);
	case DFD_CLUP_UNREG_CDEV_AVAILABLE:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AVAILABLE),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_POLL:
		cdev_del(&g_cdev_nautopoll);
	case DFD_CLUP_UNREG_CDEV_POLL:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_AUTOPOLL),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_HSEL:
		cdev_del(&g_cdev_nhsel);
	case DFD_CLUP_UNREG_CDEV_HSEL:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_HSEL),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_INTU:
		cdev_del(&g_cdev_nintu);
	case DFD_CLUP_UNREG_CDEV_INTU:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, SNFC_CTRL_MINOR_NO_INTU),
			FELICA_DEV_NUM);
// <Combo_chip> add E
	    case DFD_CLUP_CDEV_DEL_ITR:
#ifdef FELIACA_RAM_LOG_ON
		cdev_del(&g_cdev_fdebug);
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_DEBUG),
			FELICA_DEV_NUM);
#endif /* FELIACA_RAM_LOG_ON */
		cdev_del(&g_cdev_fitr);
	case DFD_CLUP_UNREG_CDEV_ITR:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_ITR),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_RFS:
		cdev_del(&g_cdev_frfs);
	case DFD_CLUP_UNREG_CDEV_RFS:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_RFS),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_CEN:
		cdev_del(&g_cdev_fcen);
	case DFD_CLUP_UNREG_CDEV_CEN:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_CEN),
			FELICA_DEV_NUM);
	case DFD_CLUP_CDEV_DEL_PON:
		cdev_del(&g_cdev_fpon);
	case DFD_CLUP_UNREG_CDEV_PON:
		unregister_chrdev_region(
			MKDEV(FELICA_CTRL_MAJOR_NO, FELICA_CTRL_MINOR_NO_PON),
			FELICA_DEV_NUM);
	case DFD_CLUP_NONE:
	default:
		break;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return;
}

/**
 *   @brief Open PON device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_pon_open(struct inode *pinode,
														struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_PON_INFO *pCtrl = NULL;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_RDONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(=O_RDONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EACCES);
		return -EACCES;
	}
	else if ((pfile->f_flags & O_ACCMODE) != O_WRONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_WRONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
	else{}

	if ((gPonCtrl.w_cnt == 0) && (gPonCtrl.Info != NULL)) {
		kfree(gPonCtrl.Info);
		gPonCtrl.Info = NULL;
	}

	pCtrl = gPonCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			DEBUG_FD_LOG_PRIV_ASSERT("kmalloc() ret=NULL");
			DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -ENOMEM);
			return -ENOMEM;
		}
		gPonCtrl.Info = pCtrl;
		FeliCaCtrl_priv_drv_timer_data_init(&pCtrl->PON_Timer);

		gPonCtrl.w_cnt = 0;
	}

	gPonCtrl.w_cnt++;

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Open CEN device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_cen_open(struct inode *pinode,
														struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_CEN_INFO *pCtrl = NULL;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) != O_RDONLY)
		&& ((pfile->f_flags & O_ACCMODE) != O_WRONLY)) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY !=O_WRONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((gCenCtrl.w_cnt == 0)
				&& (gCenCtrl.r_cnt == 0)
				&& (NULL != gCenCtrl.Info)) {
		kfree(gCenCtrl.Info);
		gCenCtrl.Info = NULL;
	}

	pCtrl = gCenCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			DEBUG_FD_LOG_PRIV_ASSERT("kmalloc() ret=NULL");
			DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -ENOMEM);
			return -ENOMEM;
		}
		gCenCtrl.Info = pCtrl;
		FeliCaCtrl_priv_drv_timer_data_init(&pCtrl->PON_Timer);
		FeliCaCtrl_priv_drv_timer_data_init(&pCtrl->CEN_Timer);

		gCenCtrl.r_cnt = 0;
		gCenCtrl.w_cnt = 0;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_RDONLY) {
		gCenCtrl.r_cnt++;
	}
	else {
		gCenCtrl.w_cnt++;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Open RFS device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_rfs_open(struct inode *pinode,
														struct file *pfile)
{
	int ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_WRONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(=O_WRONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EACCES);
		return -EACCES;
	}
	else if ((pfile->f_flags & O_ACCMODE) != O_RDONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
	else{}

	gRfsCtrl.r_cnt++;

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Open ITR device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Initialize internal infomation, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_itr_open(struct inode *pinode,
														struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pCtrl = NULL;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_WRONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(=O_WRONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EACCES);
		return -EACCES;
	}
	else if ((pfile->f_flags & O_ACCMODE) != O_RDONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
	else if (gItrCtrl.r_cnt >= FELICA_DEV_ITR_MAX_OPEN_NUM) {
		DEBUG_FD_LOG_PRIV_ASSERT("r_cnt=%d", gItrCtrl.r_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
	else{}

	if ((gItrCtrl.r_cnt == 0) && (gItrCtrl.Info != NULL)) {
		kfree(gItrCtrl.Info);
		gItrCtrl.Info = NULL;
	}

	pCtrl = gItrCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			DEBUG_FD_LOG_PRIV_ASSERT("kmalloc() ret=NULL");
			DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -ENOMEM);
			return -ENOMEM;
		}
		gItrCtrl.Info = pCtrl;
		init_waitqueue_head(&pCtrl->RcvWaitQ);
		spin_lock_init(&itr_lock);
		gItrCtrl.r_cnt = 0;
		gItrCtrl.w_cnt = 0;
		pCtrl->INT_Flag = FELICA_EDGE_NON;
		pCtrl->RFS_Flag = FELICA_EDGE_NON;
		gFeliCaItrInfo[DFD_ITR_TYPE_INT].dev = pCtrl;
#ifdef FELICA_SYS_HAVE_DETECT_RFS
		gFeliCaItrInfo[DFD_ITR_TYPE_RFS].dev = pCtrl;
		FeliCaCtrl_priv_drv_timer_data_init(&pCtrl->RFS_Timer);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
	}

	if (gItrCtrl.r_cnt == 0) {
		do {
			ret = FeliCaCtrl_priv_interrupt_ctrl_regist(DFD_ITR_TYPE_INT);
			if (ret != 0) {
				DEBUG_FD_LOG_PRIV_ASSERT("ctrl_regist() INT ret=%d", ret);
				break;
			}
#ifdef FELICA_SYS_HAVE_DETECT_RFS
			ret = FeliCaCtrl_priv_interrupt_ctrl_regist(DFD_ITR_TYPE_RFS);
			if (ret != 0) {
				FeliCaCtrl_priv_interrupt_ctrl_unregist(DFD_ITR_TYPE_INT);
				DEBUG_FD_LOG_PRIV_ASSERT("ctrl_regist() RFS ret=%d", ret);
				break;
			}
			ret = enable_irq_wake(gFeliCaItrInfo[DFD_ITR_TYPE_RFS].irq);
////			enable_irq(gFeliCaItrInfo[DFD_ITR_TYPE_RFS].irq);
			if (ret != 0) {
				FeliCaCtrl_priv_interrupt_ctrl_unregist(DFD_ITR_TYPE_RFS);
				FeliCaCtrl_priv_interrupt_ctrl_unregist(DFD_ITR_TYPE_INT);
				DEBUG_FD_LOG_PRIV_ASSERT("enable_irq_wake() RFS ret=%d", ret);
				break;
			}
			spin_lock_irqsave(&itr_lock, itr_lock_flag);
			if (timer_pending(&pCtrl->RFS_Timer.Timer) == 0) {
				(void)FeliCaCtrl_interrupt_RFS(
						gFeliCaItrInfo[DFD_ITR_TYPE_RFS].irq,
						gFeliCaItrInfo[DFD_ITR_TYPE_RFS].dev);
			}
			spin_unlock_irqrestore(&itr_lock, itr_lock_flag);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */
		} while (0);
	}

	if (ret == 0) {
		gItrCtrl.r_cnt++;
	}
	else if (gItrCtrl.r_cnt == 0) {
		kfree(gItrCtrl.Info);
		gItrCtrl.Info = NULL;
	}
	else{}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief Open ITR device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Initialize internal information, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_intu_open(struct inode *pinode, struct file *pfile)
{
    unsigned int    ret = 0;
    struct FELICA_CTRLDEV_ITR_INFO *pCtrl = NULL;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) != O_RDONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((gIntuCtrl.r_cnt == 0) && (gIntuCtrl.Info != NULL)) {
		kfree(gIntuCtrl.Info);
		gIntuCtrl.Info = NULL;
	}

	pCtrl = gIntuCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			DEBUG_FD_LOG_PRIV_ASSERT("kmalloc() ret=NULL");
			DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -ENOMEM);
			return -ENOMEM;
		}
		gIntuCtrl.Info = pCtrl;
		init_waitqueue_head(&pCtrl->RcvWaitQ);
		spin_lock_init(&itr_lock);
		gIntuCtrl.r_cnt = 0;
		gIntuCtrl.w_cnt = 0;
		pCtrl->INTU_Flag = FELICA_EDGE_NON;
		gFeliCaItrInfo[DFD_ITR_TYPE_INTU].dev = pCtrl;
	}

	if (gIntuCtrl.r_cnt == 0) {
		ret = FeliCaCtrl_priv_interrupt_ctrl_regist(DFD_ITR_TYPE_INTU);
		if (ret != 0) {
			DEBUG_FD_LOG_PRIV_ASSERT("ctrl_regist() INTU ret=%d", ret);
		}
	}

	if (ret == 0) {
		gIntuCtrl.r_cnt++;
	}
	else if (gIntuCtrl.r_cnt == 0) {
		kfree(gIntuCtrl.Info);
		gIntuCtrl.Info = NULL;
	}
	else{}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Open HSEL device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Initialize internal information, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *   @retval -ENOMEM  Out of memory
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_hsel_open(struct inode *pinode, struct file *pfile)
{
	unsigned int    ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) != O_RDONLY)
		&& ((pfile->f_flags & O_ACCMODE) != O_WRONLY)) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY !=O_WRONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
    }

	if ((pfile->f_flags & O_ACCMODE) == O_RDONLY) {
		gHselCtrl.r_cnt++;
	}
	else {
		gHselCtrl.w_cnt++;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Open AUTOPOLL device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Initialize internal information, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_poll_open(struct inode *pinode, struct file *pfile)
{
	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) != O_RDWR) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY !=O_WRONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) == O_RDONLY) {
		gPollCtrl.r_cnt++;
	}
	else {
		gPollCtrl.w_cnt++;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return");
	return 0;
}

/**
 *   @brief Open AVAILABLE device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Initialize internal information, and count up open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EACCES  Permission denied
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_available_open(struct inode *pinode, struct file *pfile)
{
	unsigned int    ret = 0;
	struct FELICA_CTRLDEV_AVAILABLE_INFO *pCtrl = NULL;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if ((pfile->f_flags & O_ACCMODE) != O_RDONLY) {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x(!=O_RDONLY)", pfile->f_flags);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
	if ((gAvailableCtrl.r_cnt == 0) && (gAvailableCtrl.Info != NULL)) {
		kfree(gAvailableCtrl.Info);
		gAvailableCtrl.Info = NULL;
	}

	pCtrl = gAvailableCtrl.Info;

	if (NULL == pCtrl) {
		pCtrl = kmalloc(sizeof(*pCtrl), GFP_KERNEL);
		if (NULL == pCtrl) {
			DEBUG_FD_LOG_PRIV_ASSERT("kmalloc() ret=NULL");
			DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -ENOMEM);
			return -ENOMEM;
		}
		gAvailableCtrl.Info = pCtrl;
		FeliCaCtrl_priv_drv_timer_data_init(&pCtrl->AVAILABLE_Timer);

		gAvailableCtrl.r_cnt = 0;
	}

	gAvailableCtrl.r_cnt++;

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Writes SNFC drivers function
 *
 *   @par   Outline:\n
 *          Writes data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file information struct
 *   @param[in] *buff   user buffer address which maintains the data to write
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0       Wrote data size
 *   @retval -EINVAL  Invalid argument
 *   @retval -EFAULT  Bad address
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_hsel_write(const char *buff, size_t count)
{
    unsigned int    ret = 0;
    unsigned char   local_buff[FELICA_OUT_SIZE];
    unsigned long   ret_cpy = 0;
    unsigned int    ret_sub = 0;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (count != FELICA_OUT_SIZE) {
        DEBUG_FD_LOG_PRIV_ASSERT("count=%d", count);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    ret_cpy = copy_from_user(local_buff, buff, FELICA_OUT_SIZE);
    if (ret_cpy != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("copy_from_user() ret=%ld",ret_cpy);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EFAULT;
    }

    DEBUG_FD_LOG_TERMINAL_IF("[HSEL] write() data=%d", local_buff[0]);
    if (FELICA_OUT_L == local_buff[0]) {
        ret_sub = SnfcCtrl_priv_ctrl_tofelica();
    }
    else if (FELICA_OUT_H == local_buff[0]) {
        ret_sub = SnfcCtrl_priv_ctrl_tosnfc();
    }
    else {
        ret_sub = -EINVAL;
        DEBUG_FD_LOG_PRIV_ASSERT("local_buff[0]=%d", local_buff[0]);
    }

    if (ret_sub == 0) {
        ret = FELICA_OUT_SIZE;
    }
    else{
        ret = ret_sub;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief Writes SNFC drivers function
 *
 *   @par   Outline:\n
 *          Writes data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file information struct
 *   @param[in] *buff   user buffer address which maintains the data to write
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0       Wrote data size
 *   @retval -EINVAL  Invalid argument
 *   @retval -ENODEV  No such device
 *   @retval -EIO     I/O error
 *   @retval -EFAULT  Bad address
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_uartcc_state_write(struct file  *pfile, const char *buff, size_t count)
{
    unsigned int    ret = 0;
    unsigned char   local_buff[FELICA_OUT_SIZE];
    unsigned long   ret_cpy = 0;
    unsigned int    ret_sub = 0;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (count != FELICA_OUT_SIZE) {
        DEBUG_FD_LOG_PRIV_ASSERT("count=%d", count);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    ret_cpy = copy_from_user(local_buff, buff, FELICA_OUT_SIZE);
    if (ret_cpy != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("copy_from_user() ret=%ld",ret_cpy);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EFAULT;
    }

    DEBUG_FD_LOG_TERMINAL_IF("[uartcc_state] write() data=%d", local_buff[0]);
    if (FELICA_OUT_SNFC_ENDPRC == local_buff[0]) {
        ret_sub = Uartdrv_pollend_set_uartcc_state();
    }
    else {
        ret_sub = Uartdrv_pollstart_set_uartcc_state(local_buff[0]);
    }

    if (ret_sub == 0) {
        ret = FELICA_OUT_SIZE;
    }
    else{
        ret = ret_sub;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief Read SNFC drivers function
 *
 *   @par   Outline:\n
 *          Read data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file information struct
 *   @param[in] *buff   user buffer address which maintains the data to read
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0        Read data size
 *   @retval -EINVAL   Invalid argument
 *   @retval -EFAULT   Bad address
 *   @retval -EBADF    Bad file number
 *   @retval -ENODEV   No such device
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_hsel_read(char *buff, size_t count)
{
    unsigned int    ret = 0;
    unsigned int    local_buff[FELICA_OUT_SIZE];
    unsigned long   ret_cpy = 0;
    unsigned int    ret_sub = 0;
    unsigned int    w_rdata;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (count != FELICA_OUT_SIZE) {
        DEBUG_FD_LOG_PRIV_ASSERT("count=%d", count);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    ret_sub = SnfcCtrl_priv_terminal_hsel_read(&w_rdata);
    if (ret_sub != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("hsel_read() ret=%d",ret_sub);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret_sub);
        return ret_sub;
    }

    local_buff[0] = w_rdata;

    DEBUG_FD_LOG_TERMINAL_IF("[HSEL] read() data=%d", local_buff[0]);

    ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
    if (ret_cpy != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("copy_to_user() ret=%ld",ret_cpy);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EFAULT);
        return -EFAULT;
    }
    else {
        ret = FELICA_OUT_SIZE;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief Read SNFC drivers function
 *
 *   @par   Outline:\n
 *          Read data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file information struct
 *   @param[in] *buff   user buffer address which maintains the data to read
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0        Read data size
 *   @retval -EINVAL   Invalid argument
 *   @retval -EFAULT   Bad address
 *   @retval -EBADF    Bad file number
 *   @retval -ENODEV   No such device
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_intu_read(char *buff, size_t count)
{
    unsigned int    ret = 0;
    unsigned int    local_buff[FELICA_OUT_SIZE];
    unsigned long   ret_cpy = 0;
    unsigned long   intu_lock_flag; /* spin_lock flag  */
    struct FELICA_CTRLDEV_ITR_INFO *pItr;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    pItr = (struct FELICA_CTRLDEV_ITR_INFO*)gIntuCtrl.Info;
    if (NULL == pItr) {
        DEBUG_FD_LOG_PRIV_ASSERT("gItrCtrl.Info = NULL");
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    if (count != FELICA_OUT_SIZE) {
        DEBUG_FD_LOG_PRIV_ASSERT("count=%d", count);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }
    
    wait_event_interruptible(pItr->RcvWaitQ, (pItr->INTU_Flag != FELICA_EDGE_NON));

    /* Begin disabling interrupts for protecting INTU Flag */
    spin_lock_irqsave(&itr_lock, intu_lock_flag);

    local_buff[0] = pItr->INTU_Flag;
    pItr->INTU_Flag = FELICA_EDGE_NON;

    /* End disabling interrupts for protecting INTU Flag */
    spin_unlock_irqrestore(&itr_lock, intu_lock_flag);

    DEBUG_FD_LOG_TERMINAL_IF("[INTU] read() INTU=%d", local_buff[0]);

    ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
    if (ret_cpy != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("copy_to_user() ret=%ld",ret_cpy);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EFAULT);
        return -EFAULT;
    }
    else {
        ret = FELICA_OUT_SIZE;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief Read SNFC drivers function
 *
 *   @par   Outline:\n
 *          Read data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file information struct
 *   @param[in] *buff   user buffer address which maintains the data to read
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0        Read data size
 *   @retval -EINVAL   Invalid argument
 *   @retval -EFAULT   Bad address
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_uartcc_state_read(char *buff, size_t count)
{
    unsigned int    ret = 0;
    unsigned int    local_buff[FELICA_OUT_SIZE];
    unsigned long   ret_cpy = 0;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    if (count != FELICA_OUT_SIZE) {
        DEBUG_FD_LOG_PRIV_ASSERT("count=%d", count);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    Uartdrv_uartcc_state_read_dd(local_buff);
    
    DEBUG_FD_LOG_TERMINAL_IF("[uartcc_state] read() state=%d", local_buff[0]);

    ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
    if (ret_cpy != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("copy_to_user() ret=%ld",ret_cpy);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EFAULT);
        return -EFAULT;
    }
    else {
        ret = FELICA_OUT_SIZE;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}

/**
 *   @brief Read SNFC drivers function
 *
 *   @par   Outline:\n
 *          Read data in the device from the user space
 *
 *   @param[in] *pfile  pointer of file information struct
 *   @param[in] *buff   user buffer address which maintains the data to read
 *   @param[in]  count  The request data transfer size
 *   @param[in] *poff   The file position of the access by the user
 *
 *   @retval >0        Read data size
 *   @retval -EINVAL   Invalid argument
 *   @retval -EFAULT   Bad address
 *   @retval -EBADF    Bad file number
 *   @retval -ENODEV   No such device
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_available_read(char *buff, size_t count)
{
    unsigned int    ret = 0;
    unsigned int    local_buff[FELICA_OUT_SIZE];
    unsigned long   ret_cpy = 0;
    unsigned int    ret_sub = 0;
    unsigned int    w_rdata;
    struct FELICA_CTRLDEV_AVAILABLE_INFO *pAvailableCtrl = NULL;

    DEBUG_FD_LOG_PRIV_FNC_ST("start");

    pAvailableCtrl = (struct FELICA_CTRLDEV_AVAILABLE_INFO*)gAvailableCtrl.Info;
    if (NULL == pAvailableCtrl) {
        DEBUG_FD_LOG_IF_ASSERT("gAvailableCtrl.Info = NULL");
        DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }
    
    if (count != FELICA_OUT_SIZE) {
        DEBUG_FD_LOG_PRIV_ASSERT("count=%d", count);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
        return -EINVAL;
    }

    ret_sub = SnfcCtrl_priv_terminal_available_read(&w_rdata, &pAvailableCtrl->AVAILABLE_Timer);
    if (ret_sub != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("hsel_read() ret=%d",ret_sub);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret_sub);
        return ret_sub;
    }

    local_buff[0] = w_rdata;
    DEBUG_FD_LOG_TERMINAL_IF("[AVAILABLE] read() data=%d", local_buff[0]);

    ret_cpy = copy_to_user(buff, local_buff, FELICA_OUT_SIZE);
    if (ret_cpy != 0) {
        DEBUG_FD_LOG_PRIV_ASSERT("copy_to_user() ret=%ld",ret_cpy);
        DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EFAULT);
        return -EFAULT;
    }
    else {
        ret = FELICA_OUT_SIZE;
    }

    DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
    return ret;
}
// <Combo_chip> add E

/**
 *   @brief Close PON device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_pon_close(struct inode *pinode, struct file *pfile)
{
	int    ret = 0;
	struct FELICA_CTRLDEV_PON_INFO *pCtrl =
				 (struct FELICA_CTRLDEV_PON_INFO*)gPonCtrl.Info;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		DEBUG_FD_LOG_PRIV_ASSERT("pCtrl=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_WRONLY)
		 					&& (gPonCtrl.w_cnt > 0)) {
		gPonCtrl.w_cnt--;
	}
	else {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x w_cnt=%d", pfile->f_flags, gPonCtrl.w_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (gPonCtrl.w_cnt == 0) {
		if (timer_pending(&pCtrl->PON_Timer.Timer) == 1) {
			del_timer(&pCtrl->PON_Timer.Timer);
			wake_up_interruptible(&pCtrl->PON_Timer.wait);
		}

		(void)FeliCaCtrl_priv_terminal_pon_write(DFD_OUT_L);

		kfree(gPonCtrl.Info);
		gPonCtrl.Info = NULL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close CEN device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_cen_close(struct inode *pinode, struct file *pfile)
{
	int    ret = 0;
	struct FELICA_CTRLDEV_CEN_INFO *pCtrl =
				(struct FELICA_CTRLDEV_CEN_INFO*)gCenCtrl.Info;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");
#if 0
	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		DEBUG_FD_LOG_PRIV_ASSERT("pCtrl=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
							&& (gCenCtrl.r_cnt > 0)) {
		gCenCtrl.r_cnt--;
	}
	else if (((pfile->f_flags & O_ACCMODE) == O_WRONLY)
								&& (gCenCtrl.w_cnt > 0)) {
		gCenCtrl.w_cnt--;
	}
	else
	{
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d w_cnt=%d", pfile->f_flags, gCenCtrl.r_cnt, gCenCtrl.w_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
#endif
	if ((gCenCtrl.r_cnt == 0) && (gCenCtrl.w_cnt == 0)) {
		if (timer_pending(&pCtrl->PON_Timer.Timer) == 1) {
			del_timer(&pCtrl->PON_Timer.Timer);
			wake_up_interruptible(&pCtrl->PON_Timer.wait);
		}

		if (timer_pending(&pCtrl->CEN_Timer.Timer) == 1) {
			del_timer(&pCtrl->CEN_Timer.Timer);
			wake_up_interruptible(&pCtrl->CEN_Timer.wait);
		}

		kfree(gCenCtrl.Info);
		gCenCtrl.Info = NULL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close RFS device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_rfs_close(
								struct inode *pinode,
								struct file *pfile)
{
	int ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
							&& (gRfsCtrl.r_cnt > 0)) {
		gRfsCtrl.r_cnt--;
	}
	else {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d", pfile->f_flags, gRfsCtrl.r_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close ITR device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Free internal infomation, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode infomation struct
 *   @param[in] *pfile   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int FeliCaCtrl_priv_dev_itr_close(
									struct inode *pinode,
									struct file *pfile)
{
	int ret = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pCtrl =
				(struct FELICA_CTRLDEV_ITR_INFO*)gItrCtrl.Info;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		DEBUG_FD_LOG_PRIV_ASSERT("pCtrl=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
								&& (gItrCtrl.r_cnt > 0)
								&& (gItrCtrl.r_cnt <= FELICA_DEV_ITR_MAX_OPEN_NUM)) {
		gItrCtrl.r_cnt--;
	}
	else {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d", pfile->f_flags, gItrCtrl.r_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	wake_up_interruptible(&pCtrl->RcvWaitQ);

	if (gItrCtrl.r_cnt == 0) {
#ifdef FELICA_SYS_HAVE_DETECT_RFS
		if (timer_pending(&pCtrl->RFS_Timer.Timer) == 1) {
			del_timer(&pCtrl->RFS_Timer.Timer);
			wake_up_interruptible(&pCtrl->RFS_Timer.wait);
		}
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

		FeliCaCtrl_priv_interrupt_ctrl_unregist(DFD_ITR_TYPE_INT);
#ifdef FELICA_SYS_HAVE_DETECT_RFS
		FeliCaCtrl_priv_interrupt_ctrl_unregist(DFD_ITR_TYPE_RFS);
#endif /* FELICA_SYS_HAVE_DETECT_RFS */

		kfree(gItrCtrl.Info);
		gItrCtrl.Info = NULL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief Close INTU device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Free internal information, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_intu_close(struct inode *pinode, struct file *pfile)
{
	unsigned int ret = 0;
	struct FELICA_CTRLDEV_ITR_INFO *pCtrl =
				(struct FELICA_CTRLDEV_ITR_INFO*)gIntuCtrl.Info;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		DEBUG_FD_LOG_PRIV_ASSERT("pCtrl=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
								&& (gIntuCtrl.r_cnt > 0)
								&& (gIntuCtrl.r_cnt <= FELICA_DEV_ITR_MAX_OPEN_NUM)) {
		gIntuCtrl.r_cnt--;
	}
	else {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d", pfile->f_flags, gIntuCtrl.r_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	wake_up_interruptible(&pCtrl->RcvWaitQ);

	if (gIntuCtrl.r_cnt == 0) {
		FeliCaCtrl_priv_interrupt_ctrl_unregist(DFD_ITR_TYPE_INTU);
		kfree(gIntuCtrl.Info);
		gIntuCtrl.Info = NULL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close HSEL device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Free internal information, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_hsel_close(struct inode *pinode, struct file *pfile)
{
	unsigned int    ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
							&& (gHselCtrl.r_cnt > 0)) {
		gHselCtrl.r_cnt--;
	}
	else if (((pfile->f_flags & O_ACCMODE) == O_WRONLY)
								&& (gHselCtrl.w_cnt > 0)) {
		gHselCtrl.w_cnt--;
	}
	else
	{
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d w_cnt=%d", pfile->f_flags, gHselCtrl.r_cnt, gCenCtrl.w_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close POLL device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Free internal information, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_poll_close(struct inode *pinode, struct file *pfile)
{
	unsigned int ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
								&& (gPollCtrl.r_cnt > 0)) {
		gPollCtrl.r_cnt--;
	}
	else if (((pfile->f_flags & O_ACCMODE) == O_WRONLY)
								&& (gPollCtrl.w_cnt > 0)) {
		gPollCtrl.w_cnt--;
	}
	else if (((pfile->f_flags & O_ACCMODE) == O_RDWR)
								&& (gPollCtrl.w_cnt > 0)) {
		gPollCtrl.w_cnt--;
	}
	else
	{
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d w_cnt=%d", pfile->f_flags, gPollCtrl.r_cnt, gCenCtrl.w_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}

/**
 *   @brief Close AVAILABLE device contorl driver of SNFC
 *
 *   @par   Outline:\n
 *          Free internal information, and count down open counter
 *
 *   @param[in] *pinode  pointer of inode information struct
 *   @param[in] *pfile   pointer of file information struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *
 *   @par Special note
 *     - none
 **/
FELICA_MT_WEAK
static int SnfcCtrl_priv_dev_available_close(struct inode *pinode, struct file *pfile)
{
	unsigned int ret = 0;
	struct FELICA_CTRLDEV_AVAILABLE_INFO *pCtrl =
				 (struct FELICA_CTRLDEV_AVAILABLE_INFO*)gAvailableCtrl.Info;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ((NULL == pfile) || (NULL == pinode)) {
		DEBUG_FD_LOG_PRIV_ASSERT("pfile=%p pinode=%p", pfile, pinode);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (NULL == pCtrl) {
		DEBUG_FD_LOG_PRIV_ASSERT("pCtrl=NULL");
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (((pfile->f_flags & O_ACCMODE) == O_RDONLY)
							&& (gAvailableCtrl.r_cnt > 0)) {
		gAvailableCtrl.r_cnt--;
	}
	else {
		DEBUG_FD_LOG_PRIV_ASSERT("f_flags=%x r_cnt=%d", pfile->f_flags, gAvailableCtrl.r_cnt);
		DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	if (gAvailableCtrl.r_cnt == 0) {
		if (timer_pending(&pCtrl->AVAILABLE_Timer.Timer) == 1) {
			del_timer(&pCtrl->AVAILABLE_Timer.Timer);
			wake_up_interruptible(&pCtrl->AVAILABLE_Timer.wait);
		}
		kfree(gAvailableCtrl.Info);
		gAvailableCtrl.Info = NULL;
	}

	DEBUG_FD_LOG_PRIV_FNC_ED("return=%d", ret);
	return ret;
}
// <Combo_chip> add E

// <iDPower_002> add S

/**
 *   @brief Probe CEN device contorl driver of FeliCa
 *
 *   @par   Outline:\n
 *          Register FeliCa cen device driver.
 *
 *   @param[in] *client  i2c client struct
 *   @param[in] *devid   i2c device id struct
 *
 *   @retval 0         Normal end
 *   @retval -ENOTSUPP I2C not supported.
 *
 *   @par Special note
 *     - none
 **/

FELICA_MT_WEAK
static int FeliCaCtrl_cen_probe(
									struct i2c_client *client,
									const struct i2c_device_id *devid)
{
	int ret = 0;

	DEBUG_FD_LOG_PRIV_FNC_ST("start");

	if ( i2c_check_functionality(client->adapter, I2C_FUNC_I2C) ) {
		cen_client = client;
	} else {
		DEBUG_FD_LOG_PRIV_ASSERT("i2c_check_functionality() false");
		ret = -ENOTSUPP;
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}
// <iDPower_002> add E

/**
 *   @brief Save alarm infomation function
 *
 *   @par   Outline:\n
 *
 *
 *   @param[in]   err_id    errno
 *
 *   @retval none
 *
 *   @par Special note
 *     - T.B.D.
 **/
FELICA_MT_WEAK
static void FeliCaCtrl_priv_alarm(unsigned long err_id)
{
	return;
}

EXPORT_SYMBOL(SnfcCtrl_terminal_hsel_read_dd);
MODULE_AUTHOR("Panasonic Mobile Communications Co., Ltd.");
MODULE_DESCRIPTION("FeliCa Control Driver");
MODULE_LICENSE("GPL");
