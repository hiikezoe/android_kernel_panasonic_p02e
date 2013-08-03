/*
 * drivers/serial/msm_serial.c - driver for msm7k serial device and console
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Acknowledgements:
 * This file is based on msm_serial.c, originally
 * Written by Robert Love <rlove@google.com>  */


// <iDPower_002> del S
//#if defined(CONFIG_SERIAL_MSM_HSL_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
//#define SUPPORT_SYSRQ
//#endif
// <iDPower_002> del E

// <iDPower_002> add S

#ifdef DEBUG_SERIAL_MSM_HSL2
#define msm_hsl2_printk(fmt, ...)               printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_IF_FNC_ST(fmt, ...)      DEBUG_FD_LOG_OUT_FUNC("***IF ST***", fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_IF_FNC_ED(fmt, ...)      DEBUG_FD_LOG_OUT_FUNC("***IF ED***", fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_IF_FNC_PARAM(fmt, ...)   DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_IF_ASSERT(fmt, ...)      DEBUG_FD_LOG_OUT_ASSERT(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_IF_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_IF_FNC_ED(fmt, ...)
#define DEBUG_FD_LOG_IF_FNC_PARAM(fmt, ...)
#define DEBUG_FD_LOG_IF_ASSERT(fmt, ...)
#endif /* DEBUG_SERIAL_MSM_HSL2 */

/* OUTPUT format */
#define DEBUG_FD_LOG_OUT_FUNC(str, fmt, ...) ({                                \
    do{                                                                        \
        char dmsg[30];                                                         \
        struct timeval tv;                                                     \
        do_gettimeofday(&tv);                                                  \
        sprintf(dmsg, fmt, ##__VA_ARGS__);                                     \
        msm_hsl2_printk("[DVFD][%d][%s][%s]%s\n"                  \
                          , current->pid, str, __func__, dmsg);                              \
    }while(0); })                                                              \

#define DEBUG_FD_LOG_OUT_PARAM(fmt, ...) ({                                    \
    do{                                                                        \
        char dmsg[30];                                                         \
        sprintf(dmsg, fmt, ##__VA_ARGS__);                                     \
        msm_hsl2_printk("[DVFD][%s]%s\n", __func__, dmsg);                       \
    }while(0); })                                                              \

#define DEBUG_FD_LOG_OUT_ASSERT(fmt, ...) ({                                   \
    do{                                                                        \
        char dmsg[50];                                                         \
        sprintf(dmsg, fmt, ##__VA_ARGS__);                                     \
        msm_hsl2_printk("[DVFD][---ASSERT---][%s][%d]%s\n"                       \
                                     , __func__, __LINE__, dmsg);              \
    }while(0); })                                                              \
// <iDPower_002> add E

#include <linux/atomic.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/of.h>
#include <linux/of_device.h>
// <iDPower_009> add S
#include <linux/wakelock.h>
// <iDPower_009> add E
// <iDPower_002> add S
#include <linux/wait.h>
// <iDPower_002> add E
#include <mach/board.h>
#include <mach/msm_serial_hs_lite.h>
#include <asm/mach-types.h>
#include "msm_serial_hs_hwreg.h"
// <Combo_chip> add S
#include <linux/mutex.h>
#include <linux/snfc_combo.h>
/*
 * This is used to lock changes in serial line configuration.
 */
static DEFINE_MUTEX(uartcc_state_mutex);
// <Combo_chip> add E

struct msm_hsl_port {
	struct uart_port	uart;
	char			name[16];
	struct clk		*clk;
	struct clk		*pclk;
	struct dentry		*loopback_dir;
	unsigned int		imr;
	unsigned int		*uart_csr_code;
	unsigned int            *gsbi_mapbase;
	unsigned int            *mapped_gsbi;
	int			is_uartdm;
	unsigned int            old_snap_state;
	unsigned int		ver_id;
        int                     tx_timeout;
// <iDPower_002> add S
	wait_queue_head_t	fsync_timer;
	int					fsync_flag;
// <iDPower_002> add E
};

// <iDPower_009> add S
struct wake_lock felica_wake_lock;
// <iDPower_009> add E

// <iDPower_002> add S
#define FSYNC_TIMER_EXPIRE	((600 * HZ) / 1000 + 1)
// <iDPower_002> add E

#define UARTDM_VERSION_11_13	0
#define UARTDM_VERSION_14	1

#define UART_TO_MSM(uart_port)	((struct msm_hsl_port *) uart_port)
#define is_console(port)	((port)->cons && \
				(port)->cons->index == (port)->line)

// <Combo_chip> add S
static int Uartdrv_felica_open_set_uartcc_state(void);
static int Uartdrv_felica_close_set_uartcc_state(void);
/*
 * error param
 */
#define SNFC_UCC_NOERR          (0)
#define SNFC_UCC_BUSY_ERROR     (-1)                    /* Busy error         */
#define SNFC_UCC_ABNORMAL_ERROR (-2)                    /* abnormal error     */
#define SNFC_UCC_OPEN_ERROR     (-1)                    /* open error         */
#define SNFC_UCC_MULTIPLE_OPEN_ERROR (-2)               /* Multiplex open error */
#define SNFC_UCC_CLOSE_ERROR    (-3)                    /* close error        */
/*
 * Polling ON/OFF
 */
#define POLLING_OUT_L  (0)                              /* Polling_ON   */
#define POLLING_OUT_H  (1)                              /* Polling_OFF  */
/*
 * HSEL ON/OFF
 */
#define HSEL_OUT_L  (0)                                 /* HSEL_ON      */
#define HSEL_OUT_H  (1)                                 /* HSEL_OFF     */
/*
 * Retry times
 */
#define SNFC_CTRL_RETRY_COUNT  (10)                     /* リトライ回数           */
/*
 * Collision Control state
 */
static enum UARTCC_STATE s_uartcc_state;                /* UARTCC state           */
static int s_irq_requested_cnt;                         /* counter of IRQ request *//* npdc300056782 add */
/*
 * FeliCa open counter
 */
enum FELICA_OPENCLOSE {
    SNFC_FELICA_OPEN_1ST = 0,                           /* FeliCa open(1st)       */
    SNFC_FELICA_OPEN_2ND,                               /* FeliCa open(2nd)       */
    SNFC_FELICA_CLOSE_1ST,                              /* FeliCa close(1st)      */
    SNFC_FELICA_CLOSE_2ND                               /* FeliCa close(2nd)      */
};
static enum FELICA_OPENCLOSE s_felica_openclose;        /* FeliCa open Flag       */
static unsigned int s_felica_close_flg;                 /* FeliCa close Flag      */
/*
 * Prototype declaration
 */
static int msm_hsl2_open_combo(struct inode *inode, struct file *filp);
static int msm_hsl2_close_combo(struct inode *inode, struct file *filp);
// <Combo_chip> add E

static const unsigned int regmap[][UARTDM_LAST] = {
	[UARTDM_VERSION_11_13] = {
		[UARTDM_MR1] = UARTDM_MR1_ADDR,
		[UARTDM_MR2] = UARTDM_MR2_ADDR,
		[UARTDM_IMR] = UARTDM_IMR_ADDR,
		[UARTDM_SR] = UARTDM_SR_ADDR,
		[UARTDM_CR] = UARTDM_CR_ADDR,
		[UARTDM_CSR] = UARTDM_CSR_ADDR,
		[UARTDM_IPR] = UARTDM_IPR_ADDR,
		[UARTDM_ISR] = UARTDM_ISR_ADDR,
		[UARTDM_RX_TOTAL_SNAP] = UARTDM_RX_TOTAL_SNAP_ADDR,
		[UARTDM_TFWR] = UARTDM_TFWR_ADDR,
		[UARTDM_RFWR] = UARTDM_RFWR_ADDR,
		[UARTDM_RF] = UARTDM_RF_ADDR,
		[UARTDM_TF] = UARTDM_TF_ADDR,
		[UARTDM_MISR] = UARTDM_MISR_ADDR,
		[UARTDM_DMRX] = UARTDM_DMRX_ADDR,
		[UARTDM_NCF_TX] = UARTDM_NCF_TX_ADDR,
		[UARTDM_DMEN] = UARTDM_DMEN_ADDR,
		[UARTDM_TXFS] = UARTDM_TXFS_ADDR,
		[UARTDM_RXFS] = UARTDM_RXFS_ADDR,

	},
	[UARTDM_VERSION_14] = {
		[UARTDM_MR1] = 0x0,
		[UARTDM_MR2] = 0x4,
		[UARTDM_IMR] = 0xb0,
		[UARTDM_SR] = 0xa4,
		[UARTDM_CR] = 0xa8,
		[UARTDM_CSR] = 0xa0,
		[UARTDM_IPR] = 0x18,
		[UARTDM_ISR] = 0xb4,
		[UARTDM_RX_TOTAL_SNAP] = 0xbc,
		[UARTDM_TFWR] = 0x1c,
		[UARTDM_RFWR] = 0x20,
		[UARTDM_RF] = 0x140,
		[UARTDM_TF] = 0x100,
		[UARTDM_MISR] = 0xac,
		[UARTDM_DMRX] = 0x34,
		[UARTDM_NCF_TX] = 0x40,
		[UARTDM_DMEN] = 0x3c,
		[UARTDM_TXFS] = 0x4c,
		[UARTDM_RXFS] = 0x50,
	},
};

static struct of_device_id msm_hsl_match_table[] = {
	{	.compatible = "qcom,msm-lsuart-v14",
		.data = (void *)UARTDM_VERSION_14
	},
	{}
};

static int get_console_state(struct uart_port *port);
static struct dentry *debug_base;
static inline void wait_for_xmitr(struct uart_port *port);

static int tty_open_count = 0;
static int wrp_tty_open(struct inode *inode, struct file *filp)
{
    int retval;
    DEBUG_FD_LOG_IF_FNC_ST("start open_count = %d", tty_open_count);
    retval = tty_open(inode, filp);
    if (retval >= 0) {
        tty_open_count++;
    }
    DEBUG_FD_LOG_IF_FNC_ED("return=%d, open_count = %d", retval, tty_open_count);
    return retval;
}

static int wrp_tty_release(struct inode *inode, struct file *filp)
{
    int retval;
    DEBUG_FD_LOG_IF_FNC_ST("start open_count = %d", tty_open_count);
    retval = tty_release(inode, filp);
    if (retval >= 0) {
        tty_open_count--;
    }
    DEBUG_FD_LOG_IF_FNC_ED("return=%d, open_count = %d", retval, tty_open_count);
    return retval;
}

static inline void msm_hsl_write(struct uart_port *port,
				 unsigned int val, unsigned int off)
{
	iowrite32(val, port->membase + off);
}
static inline unsigned int msm_hsl_read(struct uart_port *port,
		     unsigned int off)
{
	return ioread32(port->membase + off);
}

static unsigned int msm_serial_hsl_has_gsbi(struct uart_port *port)
{
	return UART_TO_MSM(port)->is_uartdm;
}

static int get_line(struct platform_device *pdev)
{
	const struct msm_serial_hslite_platform_data *pdata =
					pdev->dev.platform_data;
	if (pdata)
		return pdata->line;

	return pdev->id;
}

static int clk_en(struct uart_port *port, int enable)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	int ret = 0;

	if (enable) {
		ret = clk_prepare_enable(msm_hsl_port->clk);
		if (ret)
			goto err;
		if (msm_hsl_port->pclk) {
// <Quad> mod 0918 S
//			ret = clk_enable(msm_hsl_port->pclk);
			ret = clk_prepare_enable(msm_hsl_port->pclk);
			if (ret) {
// <Quad> mod 0918 S
// 				clk_disable(msm_hsl_port->clk);
				clk_disable_unprepare(msm_hsl_port->clk);
				goto err;
			}
		}
	} else {
// <Quad> mod 0918 S
// 		clk_disable(msm_hsl_port->clk);
		clk_disable_unprepare(msm_hsl_port->clk);
		if (msm_hsl_port->pclk)
// <Quad> mod 0918 S
// 			clk_disable(msm_hsl_port->pclk);
			clk_disable_unprepare(msm_hsl_port->pclk);
	}
err:
	return ret;
}
static int msm_hsl_loopback_enable_set(void *data, u64 val)
{
	struct msm_hsl_port *msm_hsl_port = data;
	struct uart_port *port = &(msm_hsl_port->uart);
	unsigned int vid;
	unsigned long flags;
	int ret = 0;

	ret = clk_set_rate(msm_hsl_port->clk, 7372800);
	if (!ret)
		clk_en(port, 1);
	else {
		pr_err("%s(): Error: Setting the clock rate\n", __func__);
		return -EINVAL;
	}

	vid = msm_hsl_port->ver_id;
	if (val) {
		spin_lock_irqsave(&port->lock, flags);
		ret = msm_hsl_read(port, regmap[vid][UARTDM_MR2]);
		ret |= UARTDM_MR2_LOOP_MODE_BMSK;
		msm_hsl_write(port, ret, regmap[vid][UARTDM_MR2]);
		spin_unlock_irqrestore(&port->lock, flags);
	} else {
		spin_lock_irqsave(&port->lock, flags);
		ret = msm_hsl_read(port, regmap[vid][UARTDM_MR2]);
		ret &= ~UARTDM_MR2_LOOP_MODE_BMSK;
		msm_hsl_write(port, ret, regmap[vid][UARTDM_MR2]);
		spin_unlock_irqrestore(&port->lock, flags);
	}

	clk_en(port, 0);
	return 0;
}
static int msm_hsl_loopback_enable_get(void *data, u64 *val)
{
	struct msm_hsl_port *msm_hsl_port = data;
	struct uart_port *port = &(msm_hsl_port->uart);
	unsigned long flags;
	int ret = 0;

	ret = clk_set_rate(msm_hsl_port->clk, 7372800);
	if (!ret)
		clk_en(port, 1);
	else {
		pr_err("%s(): Error setting clk rate\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&port->lock, flags);
	ret = msm_hsl_read(port, regmap[msm_hsl_port->ver_id][UARTDM_MR2]);
	spin_unlock_irqrestore(&port->lock, flags);
	clk_en(port, 0);

	*val = (ret & UARTDM_MR2_LOOP_MODE_BMSK) ? 1 : 0;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(loopback_enable_fops, msm_hsl_loopback_enable_get,
			msm_hsl_loopback_enable_set, "%llu\n");
/*
 * msm_serial_hsl debugfs node: <debugfs_root>/msm_serial_hsl/loopback.<id>
 * writing 1 turns on internal loopback mode in HW. Useful for automation
 * test scripts.
 * writing 0 disables the internal loopback mode. Default is disabled.
 */
static void msm_hsl_debugfs_init(struct msm_hsl_port *msm_uport,
								int id)
{
	char node_name[15];

	snprintf(node_name, sizeof(node_name), "loopback.%d", id);
	msm_uport->loopback_dir = debugfs_create_file(node_name,
					S_IRUGO | S_IWUSR,
					debug_base,
					msm_uport,
					&loopback_enable_fops);

	if (IS_ERR_OR_NULL(msm_uport->loopback_dir))
		pr_err("%s(): Cannot create loopback.%d debug entry",
							__func__, id);
}
static void msm_hsl_stop_tx(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	msm_hsl_port->imr &= ~UARTDM_ISR_TXLEV_BMSK;
	msm_hsl_write(port, msm_hsl_port->imr,
		regmap[msm_hsl_port->ver_id][UARTDM_IMR]);

// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	msm_hsl_port->fsync_flag = 1;
	wake_up(&msm_hsl_port->fsync_timer);

	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void msm_hsl_start_tx(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	msm_hsl_port->imr |= UARTDM_ISR_TXLEV_BMSK;
	msm_hsl_write(port, msm_hsl_port->imr,
		regmap[msm_hsl_port->ver_id][UARTDM_IMR]);

// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void msm_hsl_stop_rx(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	msm_hsl_port->imr &= ~(UARTDM_ISR_RXLEV_BMSK |
			       UARTDM_ISR_RXSTALE_BMSK);
	msm_hsl_write(port, msm_hsl_port->imr,
		regmap[msm_hsl_port->ver_id][UARTDM_IMR]);

// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void msm_hsl_enable_ms(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	msm_hsl_port->imr |= UARTDM_ISR_DELTA_CTS_BMSK;
	msm_hsl_write(port, msm_hsl_port->imr,
		regmap[msm_hsl_port->ver_id][UARTDM_IMR]);

// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void handle_rx(struct uart_port *port, unsigned int misr)
{
	struct tty_struct *tty = port->state->port.tty;
	unsigned int vid;
	unsigned int sr;
	int count = 0;
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
#ifdef DEBUG_SERIAL_MSM_HSL2
	int print_count = 0;
#endif /* DEBUG_SERIAL_MSM_HSL2 */

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	vid = msm_hsl_port->ver_id;
	/*
	 * Handle overrun. My understanding of the hardware is that overrun
	 * is not tied to the RX buffer, so we handle the case out of band.
	 */
	if ((msm_hsl_read(port, regmap[vid][UARTDM_SR]) &
				UARTDM_SR_OVERRUN_BMSK)) {
		port->icount.overrun++;
		tty_insert_flip_char(tty, 0, TTY_OVERRUN);
		msm_hsl_write(port, RESET_ERROR_STATUS,
			regmap[vid][UARTDM_CR]);
	}

	if (misr & UARTDM_ISR_RXSTALE_BMSK) {
		count = msm_hsl_read(port,
			regmap[vid][UARTDM_RX_TOTAL_SNAP]) -
			msm_hsl_port->old_snap_state;
		msm_hsl_port->old_snap_state = 0;
	} else {
		count = 4 * (msm_hsl_read(port, regmap[vid][UARTDM_RFWR]));
		msm_hsl_port->old_snap_state += count;
	}

	/* and now the main RX loop */
	while (count > 0) {
		unsigned int c;
		char flag = TTY_NORMAL;

		sr = msm_hsl_read(port, regmap[vid][UARTDM_SR]);
		if ((sr & UARTDM_SR_RXRDY_BMSK) == 0) {
			msm_hsl_port->old_snap_state -= count;
			break;
		}
		c = msm_hsl_read(port, regmap[vid][UARTDM_RF]);
		if (sr & UARTDM_SR_RX_BREAK_BMSK) {
			port->icount.brk++;
			if (uart_handle_break(port))
				continue;
		} else if (sr & UARTDM_SR_PAR_FRAME_BMSK) {
			port->icount.frame++;
		} else {
			port->icount.rx++;
		}

		/* Mask conditions we're ignorning. */
		sr &= port->read_status_mask;
		if (sr & UARTDM_SR_RX_BREAK_BMSK)
			flag = TTY_BREAK;
		else if (sr & UARTDM_SR_PAR_FRAME_BMSK)
			flag = TTY_FRAME;

		/* TODO: handle sysrq */
		/* if (!uart_handle_sysrq_char(port, c)) */
#ifdef DEBUG_SERIAL_MSM_HSL2
		printk("%08X: ", c);
		print_count++;
		if ((print_count % 8) == 0) {
			printk("\n");
		}
#endif /* DEBUG_SERIAL_MSM_HSL2 */
		tty_insert_flip_string(tty, (char *) &c,
				       (count > 4) ? 4 : count);
		count -= 4;
	}

	tty_flip_buffer_push(tty);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void handle_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	int sent_tx;
	int tx_count;
	int x;
	unsigned int tf_pointer = 0;
	unsigned int vid;
#ifdef DEBUG_SERIAL_MSM_HSL2
	int print_count = 0;
#endif /* DEBUG_SERIAL_MSM_HSL2 */

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	vid = UART_TO_MSM(port)->ver_id;
	tx_count = uart_circ_chars_pending(xmit);
#ifdef DEBUG_SERIAL_MSM_HSL2
	printk("tx_count=%d, ", tx_count);
#endif /* DEBUG_SERIAL_MSM_HSL2 */

	if (tx_count > (UART_XMIT_SIZE - xmit->tail))
		tx_count = UART_XMIT_SIZE - xmit->tail;
	if (tx_count >= port->fifosize)
		tx_count = port->fifosize;
#ifdef DEBUG_SERIAL_MSM_HSL2
	printk("tx_count=%d\n", tx_count);
#endif /* DEBUG_SERIAL_MSM_HSL2 */

	/* Handle x_char */
	if (port->x_char) {
		wait_for_xmitr(port);
		msm_hsl_write(port, tx_count + 1, regmap[vid][UARTDM_NCF_TX]);
		msm_hsl_read(port, regmap[vid][UARTDM_NCF_TX]);
		msm_hsl_write(port, port->x_char, regmap[vid][UARTDM_TF]);
		port->icount.tx++;
		port->x_char = 0;
	} else if (tx_count) {
		wait_for_xmitr(port);
		msm_hsl_write(port, tx_count, regmap[vid][UARTDM_NCF_TX]);
		msm_hsl_read(port, regmap[vid][UARTDM_NCF_TX]);
	}
	if (!tx_count) {
		msm_hsl_stop_tx(port);
// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return !tx_count = false");
// <iDPower_002> add S
		return;
	}

	while (tf_pointer < tx_count)  {
		if (unlikely(!(msm_hsl_read(port, regmap[vid][UARTDM_SR]) &
			       UARTDM_SR_TXRDY_BMSK)))
			continue;
		switch (tx_count - tf_pointer) {
		case 1: {
			x = xmit->buf[xmit->tail];
			port->icount.tx++;
			break;
		}
		case 2: {
			x = xmit->buf[xmit->tail]
				| xmit->buf[xmit->tail+1] << 8;
			port->icount.tx += 2;
			break;
		}
		case 3: {
			x = xmit->buf[xmit->tail]
				| xmit->buf[xmit->tail+1] << 8
				| xmit->buf[xmit->tail + 2] << 16;
			port->icount.tx += 3;
			break;
		}
		default: {
			x = *((int *)&(xmit->buf[xmit->tail]));
			port->icount.tx += 4;
			break;
		}
		}
#ifdef DEBUG_SERIAL_MSM_HSL2
		printk("%08X: ", x);
		print_count++;
		if ((print_count % 8) == 0) {
			printk("\n");
		}
#endif /* DEBUG_SERIAL_MSM_HSL2 */
		msm_hsl_write(port, x, regmap[vid][UARTDM_TF]);
		xmit->tail = ((tx_count - tf_pointer < 4) ?
			      (tx_count - tf_pointer + xmit->tail) :
			      (xmit->tail + 4)) & (UART_XMIT_SIZE - 1);
		tf_pointer += 4;
		sent_tx = 1;
	}

	if (uart_circ_empty(xmit))
		msm_hsl_stop_tx(port);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void handle_delta_cts(struct uart_port *port)
{
	unsigned int vid = UART_TO_MSM(port)->ver_id;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	msm_hsl_write(port, RESET_CTS, regmap[vid][UARTDM_CR]);
	port->icount.cts++;
	wake_up_interruptible(&port->state->port.delta_msr_wait);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static irqreturn_t msm_hsl_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	unsigned int vid;
	unsigned int misr;
	unsigned long flags;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	spin_lock_irqsave(&port->lock, flags);
// <Quad> del 0918
// 	clk_en(port, 1);
	vid = msm_hsl_port->ver_id;
	misr = msm_hsl_read(port, regmap[vid][UARTDM_MISR]);
	/* disable interrupt */
	msm_hsl_write(port, 0, regmap[vid][UARTDM_IMR]);

	if (misr & (UARTDM_ISR_RXSTALE_BMSK | UARTDM_ISR_RXLEV_BMSK)) {
		handle_rx(port, misr);
		if (misr & (UARTDM_ISR_RXSTALE_BMSK))
			msm_hsl_write(port, RESET_STALE_INT,
					regmap[vid][UARTDM_CR]);
		msm_hsl_write(port, 6500, regmap[vid][UARTDM_DMRX]);
		msm_hsl_write(port, STALE_EVENT_ENABLE, regmap[vid][UARTDM_CR]);
	}
	if (misr & UARTDM_ISR_TXLEV_BMSK)
		handle_tx(port);

	if (misr & UARTDM_ISR_DELTA_CTS_BMSK)
		handle_delta_cts(port);

	/* restore interrupt */
	msm_hsl_write(port, msm_hsl_port->imr, regmap[vid][UARTDM_IMR]);
// <Quad> del 0918
// 	clk_en(port, 0);
	spin_unlock_irqrestore(&port->lock, flags);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return=%d", IRQ_HANDLED);
// <iDPower_002> add E
	return IRQ_HANDLED;
}

static unsigned int msm_hsl_tx_empty(struct uart_port *port)
{
	unsigned int vid = UART_TO_MSM(port)->ver_id;
	unsigned int ret;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);
	ret = (msm_hsl_read(port, regmap[vid][UARTDM_SR]) &
	       UARTDM_SR_TXEMT_BMSK) ? TIOCSER_TEMT : 0;
// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
// <iDPower_002> add E
	return ret;
}

static void msm_hsl_reset(struct uart_port *port)
{
	unsigned int vid = UART_TO_MSM(port)->ver_id;

	/* reset everything */
	msm_hsl_write(port, RESET_RX, regmap[vid][UARTDM_CR]);
	msm_hsl_write(port, RESET_TX, regmap[vid][UARTDM_CR]);
	msm_hsl_write(port, RESET_ERROR_STATUS, regmap[vid][UARTDM_CR]);
	msm_hsl_write(port, RESET_BREAK_INT, regmap[vid][UARTDM_CR]);
	msm_hsl_write(port, RESET_CTS, regmap[vid][UARTDM_CR]);
	msm_hsl_write(port, RFR_LOW, regmap[vid][UARTDM_CR]);
}

static unsigned int msm_hsl_get_mctrl(struct uart_port *port)
{
// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return=%d", TIOCM_CAR | TIOCM_CTS | TIOCM_DSR | TIOCM_RTS);
// <iDPower_002> add E
	return TIOCM_CAR | TIOCM_CTS | TIOCM_DSR | TIOCM_RTS;
}

static void msm_hsl_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	unsigned int vid = UART_TO_MSM(port)->ver_id;
	unsigned int mr;
	unsigned int loop_mode;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	mr = msm_hsl_read(port, regmap[vid][UARTDM_MR1]);

	if (!(mctrl & TIOCM_RTS)) {
		mr &= ~UARTDM_MR1_RX_RDY_CTL_BMSK;
		msm_hsl_write(port, mr, regmap[vid][UARTDM_MR1]);
		msm_hsl_write(port, RFR_HIGH, regmap[vid][UARTDM_CR]);
	} else {
		mr |= UARTDM_MR1_RX_RDY_CTL_BMSK;
		msm_hsl_write(port, mr, regmap[vid][UARTDM_MR1]);
	}

	loop_mode = TIOCM_LOOP & mctrl;
	if (loop_mode) {
		mr = msm_hsl_read(port, regmap[vid][UARTDM_MR2]);
		mr |= UARTDM_MR2_LOOP_MODE_BMSK;
		msm_hsl_write(port, mr, regmap[vid][UARTDM_MR2]);

		/* Reset TX */
		msm_hsl_reset(port);

		/* Turn on Uart Receiver & Transmitter*/
		msm_hsl_write(port, UARTDM_CR_RX_EN_BMSK
		      | UARTDM_CR_TX_EN_BMSK, regmap[vid][UARTDM_CR]);
	}

// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void msm_hsl_break_ctl(struct uart_port *port, int break_ctl)
{
	unsigned int vid = UART_TO_MSM(port)->ver_id;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	if (break_ctl)
		msm_hsl_write(port, START_BREAK, regmap[vid][UARTDM_CR]);
	else
		msm_hsl_write(port, STOP_BREAK, regmap[vid][UARTDM_CR]);

// <Quad> del 0918
// 	clk_en(port, 0);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void msm_hsl_set_baud_rate(struct uart_port *port, unsigned int baud)
{
	unsigned int baud_code, rxstale, watermark;
	unsigned int data;
	unsigned int vid;
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

	switch (baud) {
	case 300:
		baud_code = UARTDM_CSR_75;
		rxstale = 1;
		break;
	case 600:
		baud_code = UARTDM_CSR_150;
		rxstale = 1;
		break;
	case 1200:
		baud_code = UARTDM_CSR_300;
		rxstale = 1;
		break;
	case 2400:
		baud_code = UARTDM_CSR_600;
		rxstale = 1;
		break;
	case 4800:
		baud_code = UARTDM_CSR_1200;
		rxstale = 1;
		break;
	case 9600:
		baud_code = UARTDM_CSR_2400;
		rxstale = 2;
		break;
	case 14400:
		baud_code = UARTDM_CSR_3600;
		rxstale = 3;
		break;
	case 19200:
		baud_code = UARTDM_CSR_4800;
		rxstale = 4;
		break;
	case 28800:
		baud_code = UARTDM_CSR_7200;
		rxstale = 6;
		break;
	case 38400:
		baud_code = UARTDM_CSR_9600;
		rxstale = 8;
		break;
	case 57600:
		baud_code = UARTDM_CSR_14400;
		rxstale = 16;
		break;
	case 115200:
		baud_code = UARTDM_CSR_28800;
		rxstale = 31;
		break;
	case 230400:
		baud_code = UARTDM_CSR_57600;
		rxstale = 31;
		break;
	case 460800:
		baud_code = UARTDM_CSR_115200;
		rxstale = 31;
		break;
	default: /* 115200 baud rate */
		baud_code = UARTDM_CSR_28800;
		rxstale = 31;
		break;
	}

	/* Set timeout to be ~600x the character transmit time */
	msm_hsl_port->tx_timeout = (1000000000 / baud) * 6;

	vid = msm_hsl_port->ver_id;
	msm_hsl_write(port, baud_code, regmap[vid][UARTDM_CSR]);

	/* RX stale watermark */
	watermark = UARTDM_IPR_STALE_LSB_BMSK & rxstale;
	watermark |= UARTDM_IPR_STALE_TIMEOUT_MSB_BMSK & (rxstale << 2);
	msm_hsl_write(port, watermark, regmap[vid][UARTDM_IPR]);

	/* Set RX watermark
	 * Configure Rx Watermark as 3/4 size of Rx FIFO.
	 * RFWR register takes value in Words for UARTDM Core
	 * whereas it is consider to be in Bytes for UART Core.
	 * Hence configuring Rx Watermark as 12 Words.
	 */
	watermark = (port->fifosize * 3) / (4*4);
	msm_hsl_write(port, watermark, regmap[vid][UARTDM_RFWR]);

	/* set TX watermark */
	msm_hsl_write(port, 0, regmap[vid][UARTDM_TFWR]);

	msm_hsl_write(port, CR_PROTECTION_EN, regmap[vid][UARTDM_CR]);
	msm_hsl_reset(port);

	data = UARTDM_CR_TX_EN_BMSK;
	data |= UARTDM_CR_RX_EN_BMSK;
	/* enable TX & RX */
	msm_hsl_write(port, data, regmap[vid][UARTDM_CR]);

	msm_hsl_write(port, RESET_STALE_INT, regmap[vid][UARTDM_CR]);
	/* turn on RX and CTS interrupts */
	msm_hsl_port->imr = UARTDM_ISR_RXSTALE_BMSK
		| UARTDM_ISR_DELTA_CTS_BMSK | UARTDM_ISR_RXLEV_BMSK;
	msm_hsl_write(port, msm_hsl_port->imr, regmap[vid][UARTDM_IMR]);
	msm_hsl_write(port, 6500, regmap[vid][UARTDM_DMRX]);
	msm_hsl_write(port, STALE_EVENT_ENABLE, regmap[vid][UARTDM_CR]);
}

static void msm_hsl_init_clock(struct uart_port *port)
{
	clk_en(port, 1);
}

static void msm_hsl_deinit_clock(struct uart_port *port)
{
	clk_en(port, 0);
}

static int msm_hsl_startup(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	struct platform_device *pdev = to_platform_device(port->dev);
	const struct msm_serial_hslite_platform_data *pdata =
					pdev->dev.platform_data;
	unsigned int data, rfr_level;
	unsigned int vid;
	int ret;
	unsigned long flags;
// <iDPower_002> add S
	struct tty_struct *tty;
	int idx;

	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	snprintf(msm_hsl_port->name, sizeof(msm_hsl_port->name),
		 "msm_serial_hsl%d", port->line);

	if (!(is_console(port)) || (!port->cons) ||
		(port->cons && (!(port->cons->flags & CON_ENABLED)))) {

		if (msm_serial_hsl_has_gsbi(port))
			if ((ioread32(msm_hsl_port->mapped_gsbi +
				GSBI_CONTROL_ADDR) & GSBI_PROTOCOL_I2C_UART)
					!= GSBI_PROTOCOL_I2C_UART)
				iowrite32(GSBI_PROTOCOL_I2C_UART,
					msm_hsl_port->mapped_gsbi +
						GSBI_CONTROL_ADDR);

		if (pdata && pdata->config_gpio) {
			ret = gpio_request(pdata->uart_tx_gpio,
						"UART_TX_GPIO");
			if (unlikely(ret)) {
				pr_err("%s: gpio request failed for:%d\n",
						__func__, pdata->uart_tx_gpio);
				return ret;
			}

			ret = gpio_request(pdata->uart_rx_gpio, "UART_RX_GPIO");
			if (unlikely(ret)) {
				pr_err("%s: gpio request failed for:%d\n",
						__func__, pdata->uart_rx_gpio);
				gpio_free(pdata->uart_tx_gpio);
				return ret;
			}
		}
	}
#ifndef CONFIG_PM_RUNTIME
	msm_hsl_init_clock(port);
#endif
	pm_runtime_get_sync(port->dev);

	/* Set RFR Level as 3/4 of UARTDM FIFO Size */
	if (likely(port->fifosize > 48))
		rfr_level = port->fifosize - 16;
	else
		rfr_level = port->fifosize;

	/*
	 * Use rfr_level value in Words to program
	 * MR1 register for UARTDM Core.
	 */
	rfr_level = (rfr_level / 4);

	spin_lock_irqsave(&port->lock, flags);

	vid = msm_hsl_port->ver_id;
	/* set automatic RFR level */
	data = msm_hsl_read(port, regmap[vid][UARTDM_MR1]);
	data &= ~UARTDM_MR1_AUTO_RFR_LEVEL1_BMSK;
	data &= ~UARTDM_MR1_AUTO_RFR_LEVEL0_BMSK;
	data |= UARTDM_MR1_AUTO_RFR_LEVEL1_BMSK & (rfr_level << 2);
	data |= UARTDM_MR1_AUTO_RFR_LEVEL0_BMSK & rfr_level;
	msm_hsl_write(port, data, regmap[vid][UARTDM_MR1]);
	spin_unlock_irqrestore(&port->lock, flags);

        /* npdc300056782 mod start */
        if (s_irq_requested_cnt == 0){
            ret = request_irq(port->irq, msm_hsl_irq, IRQF_TRIGGER_HIGH,
                          msm_hsl_port->name, port);
            if (unlikely(ret)) {
                printk(KERN_ERR "%s: failed to request_irq\n", __func__);
                return ret;
            }
        }
        s_irq_requested_cnt++;
        /* npdc300056782 mod end */

// <iDPower_002> add S
	init_waitqueue_head(&msm_hsl_port->fsync_timer);

	tty = port->state->port.tty;
	idx = tty->index;

	mutex_lock(&tty->termios_mutex);

	if (tty->driver->termios[idx]) {
		tty->driver->termios[idx]->c_ispeed = 460800;
		tty->driver->termios[idx]->c_ospeed = 460800;
		tty->driver->termios[idx]->c_cflag |= B460800;
		tty->driver->termios[idx]->c_cflag &= ~CSIZE;
		tty->driver->termios[idx]->c_cflag |= (CS8 | CLOCAL | CREAD | CRTSCTS);
		tty->driver->termios[idx]->c_lflag = 0;
		tty->driver->termios[idx]->c_oflag = 0;
		tty->driver->termios[idx]->c_iflag = IGNPAR;
		tty->driver->termios[idx]->c_cc[VMIN] = 0;
		tty->driver->termios[idx]->c_cc[VTIME] = 0;
	}

	tty_init_termios(tty);
	tty->icanon = (L_ICANON(tty) != 0);
	DEBUG_FD_LOG_IF_FNC_PARAM("tty->icanon=%d", tty->icanon);

	mutex_unlock(&tty->termios_mutex);

	DEBUG_FD_LOG_IF_FNC_ED("return=0");
// <iDPower_002> add E
	return 0;
}

static void msm_hsl_shutdown(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	struct platform_device *pdev = to_platform_device(port->dev);
	const struct msm_serial_hslite_platform_data *pdata =
					pdev->dev.platform_data;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

// <Quad> del 0918
// 	clk_en(port, 1);

	msm_hsl_port->imr = 0;
	/* disable interrupts */
	msm_hsl_write(port, 0, regmap[msm_hsl_port->ver_id][UARTDM_IMR]);

// <Quad> del 0918
// 	clk_en(port, 0);

        /* npdc300056782 mod start */
        if (s_irq_requested_cnt <= 1){
            free_irq(port->irq, port);
            s_irq_requested_cnt = 0;
        } else {
            s_irq_requested_cnt--;
        }
        /* npdc300056782 mod end */

#ifndef CONFIG_PM_RUNTIME
	msm_hsl_deinit_clock(port);
#endif
	pm_runtime_put_sync(port->dev);
	if (!(is_console(port)) || (!port->cons) ||
		(port->cons && (!(port->cons->flags & CON_ENABLED)))) {
		if (pdata && pdata->config_gpio) {
			gpio_free(pdata->uart_tx_gpio);
			gpio_free(pdata->uart_rx_gpio);
		}
	}

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static void msm_hsl_set_termios(struct uart_port *port,
				struct ktermios *termios,
				struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, mr;
	unsigned int vid;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	spin_lock_irqsave(&port->lock, flags);
// <Quad> del 0918
// 	clk_en(port, 1);

	/* calculate and set baud rate */
	baud = uart_get_baud_rate(port, termios, old, 300, 460800);

	msm_hsl_set_baud_rate(port, baud);

	vid = UART_TO_MSM(port)->ver_id;
	/* calculate parity */
	mr = msm_hsl_read(port, regmap[vid][UARTDM_MR2]);
	mr &= ~UARTDM_MR2_PARITY_MODE_BMSK;
	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			mr |= ODD_PARITY;
		else if (termios->c_cflag & CMSPAR)
			mr |= SPACE_PARITY;
		else
			mr |= EVEN_PARITY;
	}

	/* calculate bits per char */
	mr &= ~UARTDM_MR2_BITS_PER_CHAR_BMSK;
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		mr |= FIVE_BPC;
		break;
	case CS6:
		mr |= SIX_BPC;
		break;
	case CS7:
		mr |= SEVEN_BPC;
		break;
	case CS8:
	default:
		mr |= EIGHT_BPC;
		break;
	}

	/* calculate stop bits */
	mr &= ~(STOP_BIT_ONE | STOP_BIT_TWO);
	if (termios->c_cflag & CSTOPB)
		mr |= STOP_BIT_TWO;
	else
		mr |= STOP_BIT_ONE;

	/* set parity, bits per char, and stop bit */
	msm_hsl_write(port, mr, regmap[vid][UARTDM_MR2]);

	/* calculate and set hardware flow control */
	mr = msm_hsl_read(port, regmap[vid][UARTDM_MR1]);
	mr &= ~(UARTDM_MR1_CTS_CTL_BMSK | UARTDM_MR1_RX_RDY_CTL_BMSK);
	if (termios->c_cflag & CRTSCTS) {
		mr |= UARTDM_MR1_CTS_CTL_BMSK;
		mr |= UARTDM_MR1_RX_RDY_CTL_BMSK;
	}
	msm_hsl_write(port, mr, regmap[vid][UARTDM_MR1]);

	/* Configure status bits to ignore based on termio flags. */
	port->read_status_mask = 0;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UARTDM_SR_PAR_FRAME_BMSK;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UARTDM_SR_RX_BREAK_BMSK;

	uart_update_timeout(port, termios->c_cflag, baud);

// <Quad> del 0918
// 	clk_en(port, 0);
	spin_unlock_irqrestore(&port->lock, flags);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static const char *msm_hsl_type(struct uart_port *port)
{
// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");

	DEBUG_FD_LOG_IF_FNC_ED("return=MSM");
// <iDPower_002> add E
	return "MSM";
}

static void msm_hsl_release_port(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *uart_resource;
	resource_size_t size;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	uart_resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						     "uartdm_resource");
	if (!uart_resource)
		uart_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!uart_resource))
		return;
	size = uart_resource->end - uart_resource->start + 1;

	release_mem_region(port->mapbase, size);
	iounmap(port->membase);
	port->membase = NULL;

	if (msm_serial_hsl_has_gsbi(port)) {
		iowrite32(GSBI_PROTOCOL_IDLE, msm_hsl_port->mapped_gsbi +
			  GSBI_CONTROL_ADDR);
		iounmap(msm_hsl_port->mapped_gsbi);
		msm_hsl_port->mapped_gsbi = NULL;
	}

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static int msm_hsl_request_port(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *uart_resource;
	struct resource *gsbi_resource;
	resource_size_t size;
    static int already_requested = 0;
// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E
    if (already_requested) {
        DEBUG_FD_LOG_IF_FNC_ED("already run");
        return 0;
    } else {
        already_requested = 1;
    }

	uart_resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						     "uartdm_resource");
	if (!uart_resource)
		uart_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!uart_resource)) {
		pr_err("%s: can't get uartdm resource\n", __func__);
		return -ENXIO;
	}
	size = uart_resource->end - uart_resource->start + 1;

	if (unlikely(!request_mem_region(port->mapbase, size,
					 "msm_serial_hsl"))) {
		pr_err("%s: can't get mem region for uartdm\n", __func__);
		return -EBUSY;
	}

	port->membase = ioremap(port->mapbase, size);
	if (!port->membase) {
		release_mem_region(port->mapbase, size);
		return -EBUSY;
	}

	if (msm_serial_hsl_has_gsbi(port)) {
		gsbi_resource = platform_get_resource_byname(pdev,
							     IORESOURCE_MEM,
							     "gsbi_resource");
		if (!gsbi_resource)
			gsbi_resource = platform_get_resource(pdev,
						IORESOURCE_MEM, 1);
		if (unlikely(!gsbi_resource)) {
			pr_err("%s: can't get gsbi resource\n", __func__);
			return -ENXIO;
		}

		size = gsbi_resource->end - gsbi_resource->start + 1;
		msm_hsl_port->mapped_gsbi = ioremap(gsbi_resource->start,
						    size);
		if (!msm_hsl_port->mapped_gsbi) {
			return -EBUSY;
		}
	}

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return=0");
// <iDPower_002> add E
	return 0;
}

static void msm_hsl_config_port(struct uart_port *port, int flags)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_MSM;
		if (msm_hsl_request_port(port))
			return;
	}
	if (msm_serial_hsl_has_gsbi(port)) {
		if (msm_hsl_port->pclk)
// <Quad> mod 0918
// 			clk_enable(msm_hsl_port->pclk);
			clk_prepare_enable(msm_hsl_port->pclk);
		if ((ioread32(msm_hsl_port->mapped_gsbi + GSBI_CONTROL_ADDR) &
			GSBI_PROTOCOL_I2C_UART) != GSBI_PROTOCOL_I2C_UART)
			iowrite32(GSBI_PROTOCOL_I2C_UART,
				msm_hsl_port->mapped_gsbi + GSBI_CONTROL_ADDR);
		if (msm_hsl_port->pclk)
// <Quad> mod 0918
// 			clk_disable(msm_hsl_port->pclk);
			clk_disable_unprepare(msm_hsl_port->pclk);
	}

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static int msm_hsl_verify_port(struct uart_port *port,
			       struct serial_struct *ser)
{
// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_MSM))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return=0");
// <iDPower_002> add E
	return 0;
}

static void msm_hsl_power(struct uart_port *port, unsigned int state,
			  unsigned int oldstate)
{
	int ret;
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	switch (state) {
	case 0:
		ret = clk_set_rate(msm_hsl_port->clk, 7372800);
		if (ret)
			pr_err("%s(): Error setting UART clock rate\n",
								__func__);
		clk_en(port, 1);
		break;
	case 3:
		clk_en(port, 0);
		ret = clk_set_rate(msm_hsl_port->clk, 0);
		if (ret)
			pr_err("%s(): Error setting UART clock rate to zero.\n",
								__func__);
		break;
	default:
		pr_err("%s(): msm_serial_hsl: Unknown PM state %d\n",
							__func__, state);
	}

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

static struct uart_ops msm_hsl_uart_pops = {
	.tx_empty = msm_hsl_tx_empty,
	.set_mctrl = msm_hsl_set_mctrl,
	.get_mctrl = msm_hsl_get_mctrl,
	.stop_tx = msm_hsl_stop_tx,
	.start_tx = msm_hsl_start_tx,
	.stop_rx = msm_hsl_stop_rx,
	.enable_ms = msm_hsl_enable_ms,
	.break_ctl = msm_hsl_break_ctl,
	.startup = msm_hsl_startup,
	.shutdown = msm_hsl_shutdown,
	.set_termios = msm_hsl_set_termios,
	.type = msm_hsl_type,
	.release_port = msm_hsl_release_port,
	.request_port = msm_hsl_request_port,
	.config_port = msm_hsl_config_port,
	.verify_port = msm_hsl_verify_port,
	.pm = msm_hsl_power,
};

static struct msm_hsl_port msm_hsl_uart_ports[] = {
	{
		.uart = {
			.iotype = UPIO_MEM,
			.ops = &msm_hsl_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
			.fifosize = 64,
			.line = 0,
		},
	},
// <iDPower_002> del S
//	{
//		.uart = {
//			.iotype = UPIO_MEM,
//			.ops = &msm_hsl_uart_pops,
//			.flags = UPF_BOOT_AUTOCONF,
//			.fifosize = 64,
//			.line = 1,
//		},
//	},
//	{
//		.uart = {
//			.iotype = UPIO_MEM,
//			.ops = &msm_hsl_uart_pops,
//			.flags = UPF_BOOT_AUTOCONF,
//			.fifosize = 64,
//			.line = 2,
//		},
//	},
// <iDPower_002> del E
};

#define UART_NR	ARRAY_SIZE(msm_hsl_uart_ports)

static inline struct uart_port *get_port_from_line(unsigned int line)
{
	return &msm_hsl_uart_ports[line].uart;
}

static unsigned int msm_hsl_console_state[8];

static void dump_hsl_regs(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	unsigned int vid = msm_hsl_port->ver_id;
	unsigned int sr, isr, mr1, mr2, ncf, txfs, rxfs, con_state;

	sr = msm_hsl_read(port, regmap[vid][UARTDM_SR]);
	isr = msm_hsl_read(port, regmap[vid][UARTDM_ISR]);
	mr1 = msm_hsl_read(port, regmap[vid][UARTDM_MR1]);
	mr2 = msm_hsl_read(port, regmap[vid][UARTDM_MR2]);
	ncf = msm_hsl_read(port, regmap[vid][UARTDM_NCF_TX]);
	txfs = msm_hsl_read(port, regmap[vid][UARTDM_TXFS]);
	rxfs = msm_hsl_read(port, regmap[vid][UARTDM_RXFS]);
	con_state = get_console_state(port);

	msm_hsl_console_state[0] = sr;
	msm_hsl_console_state[1] = isr;
	msm_hsl_console_state[2] = mr1;
	msm_hsl_console_state[3] = mr2;
	msm_hsl_console_state[4] = ncf;
	msm_hsl_console_state[5] = txfs;
	msm_hsl_console_state[6] = rxfs;
	msm_hsl_console_state[7] = con_state;

	pr_info("%s(): Timeout: %d uS\n", __func__, msm_hsl_port->tx_timeout);
	pr_info("%s(): SR:  %08x\n", __func__, sr);
	pr_info("%s(): ISR: %08x\n", __func__, isr);
	pr_info("%s(): MR1: %08x\n", __func__, mr1);
	pr_info("%s(): MR2: %08x\n", __func__, mr2);
	pr_info("%s(): NCF: %08x\n", __func__, ncf);
	pr_info("%s(): TXFS: %08x\n", __func__, txfs);
	pr_info("%s(): RXFS: %08x\n", __func__, rxfs);
	pr_info("%s(): Console state: %d\n", __func__, con_state);
}

/*
 *  Wait for transmitter & holding register to empty
 *  Derived from wait_for_xmitr in 8250 serial driver by Russell King  */
static void wait_for_xmitr(struct uart_port *port)
{
	struct msm_hsl_port *msm_hsl_port = UART_TO_MSM(port);
	unsigned int vid = msm_hsl_port->ver_id;
	int count = 0;

	if (!(msm_hsl_read(port, regmap[vid][UARTDM_SR]) &
			UARTDM_SR_TXEMT_BMSK)) {
		while (!(msm_hsl_read(port, regmap[vid][UARTDM_ISR]) &
			UARTDM_ISR_TX_READY_BMSK) &&
		       !(msm_hsl_read(port, regmap[vid][UARTDM_SR]) &
			UARTDM_SR_TXEMT_BMSK)) {
			udelay(1);
			touch_nmi_watchdog();
			cpu_relax();
			if (++count == msm_hsl_port->tx_timeout) {
				dump_hsl_regs(port);
				panic("MSM HSL wait_for_xmitr is stuck!");
			}
		}
		msm_hsl_write(port, CLEAR_TX_READY, regmap[vid][UARTDM_CR]);
	}
}

// <iDPower_002> del S
//#ifdef CONFIG_SERIAL_MSM_HSL_CONSOLE
//static void msm_hsl_console_putchar(struct uart_port *port, int ch)
//{
//	unsigned int vid = UART_TO_MSM(port)->ver_id;
//
//	wait_for_xmitr(port, UARTDM_ISR_TX_READY_BMSK);
//	msm_hsl_write(port, 1, regmap[vid][UARTDM_NCF_TX]);
//
//	while (!(msm_hsl_read(port, regmap[vid][UARTDM_SR]) &
//				UARTDM_SR_TXRDY_BMSK)) {
//		udelay(1);
//		touch_nmi_watchdog();
//	}
//
//	msm_hsl_write(port, ch, regmap[vid][UARTDM_TF]);
//}
//
//static void msm_hsl_console_write(struct console *co, const char *s,
//				  unsigned int count)
//{
//	struct uart_port *port;
//	struct msm_hsl_port *msm_hsl_port;
//	unsigned int vid;
//	int locked;
//
//	BUG_ON(co->index < 0 || co->index >= UART_NR);
//
//	port = get_port_from_line(co->index);
//	msm_hsl_port = UART_TO_MSM(port);
//	vid = msm_hsl_port->ver_id;
//
//	/* not pretty, but we can end up here via various convoluted paths */
//	if (port->sysrq || oops_in_progress)
//		locked = spin_trylock(&port->lock);
//	else {
//		locked = 1;
//		spin_lock(&port->lock);
//	}
//	msm_hsl_write(port, 0, regmap[vid][UARTDM_IMR]);
//	uart_console_write(port, s, count, msm_hsl_console_putchar);
//	msm_hsl_write(port, msm_hsl_port->imr, regmap[vid][UARTDM_IMR]);
//	if (locked == 1)
//		spin_unlock(&port->lock);
//}
//
//static int msm_hsl_console_setup(struct console *co, char *options)
//{
//	struct uart_port *port;
//	unsigned int vid;
//	int baud = 0, flow, bits, parity;
//	int ret;
//
//	if (unlikely(co->index >= UART_NR || co->index < 0))
//		return -ENXIO;
//
//	port = get_port_from_line(co->index);
//	vid = UART_TO_MSM(port)->ver_id;
//
//	if (unlikely(!port->membase))
//		return -ENXIO;
//
//	port->cons = co;
//
//	pm_runtime_get_noresume(port->dev);
//
//#ifndef CONFIG_PM_RUNTIME
//	msm_hsl_init_clock(port);
//#endif
//	pm_runtime_resume(port->dev);
//
//	if (options)
//		uart_parse_options(options, &baud, &parity, &bits, &flow);
//
//	bits = 8;
//	parity = 'n';
//	flow = 'n';
//	msm_hsl_write(port, UARTDM_MR2_BITS_PER_CHAR_8 | STOP_BIT_ONE,
//		      regmap[vid][UARTDM_MR2]);	/* 8N1 */
//
//	if (baud < 300 || baud > 115200)
//		baud = 115200;
//	msm_hsl_set_baud_rate(port, baud);
//
//	ret = uart_set_options(port, co, baud, parity, bits, flow);
//	msm_hsl_reset(port);
//	/* Enable transmitter */
//	msm_hsl_write(port, CR_PROTECTION_EN, regmap[vid][UARTDM_CR]);
//	msm_hsl_write(port, UARTDM_CR_TX_EN_BMSK, regmap[vid][UARTDM_CR]);
//
//	printk(KERN_INFO "msm_serial_hsl: console setup on port #%d\n",
//	       port->line);
//
//	return ret;
//}
//
//static struct uart_driver msm_hsl_uart_driver;
//
//static struct console msm_hsl_console = {
//	.name = "ttyHSL",
//	.write = msm_hsl_console_write,
//	.device = uart_console_device,
//	.setup = msm_hsl_console_setup,
//	.flags = CON_PRINTBUFFER,
//	.index = -1,
//	.data = &msm_hsl_uart_driver,
//};
//
//#define MSM_HSL_CONSOLE	(&msm_hsl_console)
/*
 * get_console_state - check the per-port serial console state.
 * @port: uart_port structure describing the port
 *
 * Return the state of serial console availability on port.
 * return 1: If serial console is enabled on particular UART port.
 * return 0: If serial console is disabled on particular UART port.
 */
static int get_console_state(struct uart_port *port)
{
	if (is_console(port) && (port->cons->flags & CON_ENABLED))
		return 1;
	else
		return 0;
}

///* show_msm_console - provide per-port serial console state. */
//static ssize_t show_msm_console(struct device *dev,
//				struct device_attribute *attr, char *buf)
//{
//	int enable;
//	struct uart_port *port;
//
//	struct platform_device *pdev = to_platform_device(dev);
//	port = get_port_from_line(pdev->id);
//
//	enable = get_console_state(port);
//
//	return snprintf(buf, sizeof(enable), "%d\n", enable);
//}
//
///*
// * set_msm_console - allow to enable/disable serial console on port.
// *
// * writing 1 enables serial console on UART port.
// * writing 0 disables serial console on UART port.
// */
//static ssize_t set_msm_console(struct device *dev,
//				struct device_attribute *attr,
//				const char *buf, size_t count)
//{
//	int enable, cur_state;
//	struct uart_port *port;
//
//	struct platform_device *pdev = to_platform_device(dev);
//	port = get_port_from_line(pdev->id);
//
//	cur_state = get_console_state(port);
//	enable = buf[0] - '0';
//
//	if (enable == cur_state)
//		return count;
//
//	switch (enable) {
//	case 0:
//		pr_debug("%s(): Calling stop_console\n", __func__);
//		console_stop(port->cons);
//		pr_debug("%s(): Calling unregister_console\n", __func__);
//		unregister_console(port->cons);
//		pm_runtime_put_sync(&pdev->dev);
//		pm_runtime_disable(&pdev->dev);
//		/*
//		 * Disable UART Core clk
//		 * 3 - to disable the UART clock
//		 * Thid parameter is not used here, but used in serial core.
//		 */
//		msm_hsl_power(port, 3, 1);
//		break;
//	case 1:
//		pr_debug("%s(): Calling register_console\n", __func__);
//		/*
//		 * Disable UART Core clk
//		 * 0 - to enable the UART clock
//		 * Thid parameter is not used here, but used in serial core.
//		 */
//		msm_hsl_power(port, 0, 1);
//		pm_runtime_enable(&pdev->dev);
//		register_console(port->cons);
//		break;
//	default:
//		return -EINVAL;
//	}
//
//	return count;
//}
//static DEVICE_ATTR(console, S_IWUSR | S_IRUGO, show_msm_console,
//						set_msm_console);
//#else
//#define MSM_HSL_CONSOLE	NULL
//#endif
// <iDPower_002> del E

static struct uart_driver msm_hsl_uart_driver = {
// <iDPower_002> mod S
//	.owner = THIS_MODULE,
//	.driver_name = "msm_serial_hsl",
//	.dev_name = "ttyHSL",
//	.nr = UART_NR,
//	.cons = MSM_HSL_CONSOLE,
	.owner = THIS_MODULE,
	.driver_name = "msm_serial_hsl2",
	.dev_name = "felica",
	.nr = UART_NR,
	.major = 124,
	.minor = 0,
// <iDPower_002> mod E
};
// <Combo_chip> add S
static struct uart_driver msm_hsl_uart_driver_nfc = {
	.owner = THIS_MODULE,
	.driver_name = "msm_serial_hsl_nfc",
	.dev_name = "snfc_uart",
	.nr = UART_NR,
	.major = 124,
	.minor = 1
};
// <Combo_chip> add E

static atomic_t msm_serial_hsl_next_id = ATOMIC_INIT(0);

static int __devinit msm_serial_hsl_probe(struct platform_device *pdev)
{
	struct msm_hsl_port *msm_hsl_port;
	struct resource *uart_resource;
	struct resource *gsbi_resource;
	struct uart_port *port;
	const struct of_device_id *match;
	int ret;

	if (pdev->id == -1)
		pdev->id = atomic_inc_return(&msm_serial_hsl_next_id) - 1;

	if (unlikely(get_line(pdev) < 0 || get_line(pdev) >= UART_NR))
		return -ENXIO;

	printk(KERN_INFO "msm_serial_hsl: detected port #%d\n", pdev->id);

	port = get_port_from_line(get_line(pdev));
	port->dev = &pdev->dev;
	msm_hsl_port = UART_TO_MSM(port);

	match = of_match_device(msm_hsl_match_table, &pdev->dev);
	if (!match)
		msm_hsl_port->ver_id = UARTDM_VERSION_11_13;
	else
		msm_hsl_port->ver_id = (unsigned int)match->data;

	gsbi_resource =	platform_get_resource_byname(pdev,
						     IORESOURCE_MEM,
						     "gsbi_resource");
	if (!gsbi_resource)
		gsbi_resource = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	msm_hsl_port->clk = clk_get(&pdev->dev, "core_clk");
	if (gsbi_resource) {
		msm_hsl_port->is_uartdm = 1;
		msm_hsl_port->pclk = clk_get(&pdev->dev, "iface_clk");
	} else {
		msm_hsl_port->is_uartdm = 0;
		msm_hsl_port->pclk = NULL;
	}

	if (unlikely(IS_ERR(msm_hsl_port->clk))) {
		printk(KERN_ERR "%s: Error getting clk\n", __func__);
		return PTR_ERR(msm_hsl_port->clk);
	}
	if (unlikely(IS_ERR(msm_hsl_port->pclk))) {
		printk(KERN_ERR "%s: Error getting pclk\n", __func__);
		return PTR_ERR(msm_hsl_port->pclk);
	}

	uart_resource = platform_get_resource_byname(pdev,
						     IORESOURCE_MEM,
						     "uartdm_resource");
	if (!uart_resource)
		uart_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!uart_resource)) {
		printk(KERN_ERR "getting uartdm_resource failed\n");
		return -ENXIO;
	}
	port->mapbase = uart_resource->start;

	port->irq = platform_get_irq(pdev, 0);
	if (unlikely((int)port->irq < 0)) {
		printk(KERN_ERR "%s: getting irq failed\n", __func__);
		return -ENXIO;
	}

	device_set_wakeup_capable(&pdev->dev, 1);
	platform_set_drvdata(pdev, port);
	pm_runtime_enable(port->dev);
// <iDPower_002> del S
//#ifdef CONFIG_SERIAL_MSM_HSL_CONSOLE
//	ret = device_create_file(&pdev->dev, &dev_attr_console);
//	if (unlikely(ret))
//		pr_err("%s():Can't create console attribute\n", __func__);
//#endif
// <iDPower_002> del E
	msm_hsl_debugfs_init(msm_hsl_port, pdev->id);

//<iDPower_009> add S
    wake_lock_init(&felica_wake_lock, WAKE_LOCK_SUSPEND, "felica_wake_lock");
//<iDPower_009> add E

	/* Temporarily increase the refcount on the GSBI clock to avoid a race
	 * condition with the earlyprintk handover mechanism.
	 */
	if (msm_hsl_port->pclk)
		clk_prepare_enable(msm_hsl_port->pclk);
	ret = uart_add_one_port(&msm_hsl_uart_driver, port);
	ret = uart_add_one_port(&msm_hsl_uart_driver_nfc, port);
	if (msm_hsl_port->pclk)
		clk_disable_unprepare(msm_hsl_port->pclk);
	return ret;
}

static int __devexit msm_serial_hsl_remove(struct platform_device *pdev)
{
	struct msm_hsl_port *msm_hsl_port = platform_get_drvdata(pdev);
	struct uart_port *port;

	port = get_port_from_line(get_line(pdev));
// <iDPower_002> del S
//#ifdef CONFIG_SERIAL_MSM_HSL_CONSOLE
//	device_remove_file(&pdev->dev, &dev_attr_console);
//#endif
// <iDPower_002> del E
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	device_set_wakeup_capable(&pdev->dev, 0);
	platform_set_drvdata(pdev, NULL);
	uart_remove_one_port(&msm_hsl_uart_driver, port);

	clk_put(msm_hsl_port->pclk);
	clk_put(msm_hsl_port->clk);
	debugfs_remove(msm_hsl_port->loopback_dir);

//<iDPower_009> add S
    wake_lock_destroy(&felica_wake_lock);
//<iDPower_009> add S

	return 0;
}

#ifdef CONFIG_PM
static int msm_serial_hsl_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_port *port;
	port = get_port_from_line(get_line(pdev));

	if (port) {

		if (is_console(port))
			msm_hsl_deinit_clock(port);

		uart_suspend_port(&msm_hsl_uart_driver, port);
		if (device_may_wakeup(dev))
			enable_irq_wake(port->irq);
	}

	return 0;
}

static int msm_serial_hsl_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_port *port;
	port = get_port_from_line(get_line(pdev));

	if (port) {

		uart_resume_port(&msm_hsl_uart_driver, port);
		if (device_may_wakeup(dev))
			disable_irq_wake(port->irq);

		if (is_console(port))
			msm_hsl_init_clock(port);
	}

	return 0;
}
#else
#define msm_serial_hsl_suspend NULL
#define msm_serial_hsl_resume NULL
#endif

static int msm_hsl_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_port *port;
	port = get_port_from_line(get_line(pdev));

	dev_dbg(dev, "pm_runtime: suspending\n");
	msm_hsl_deinit_clock(port);
	return 0;
}

static int msm_hsl_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_port *port;
	port = get_port_from_line(get_line(pdev));

	dev_dbg(dev, "pm_runtime: resuming\n");
	msm_hsl_init_clock(port);
	return 0;
}

static struct dev_pm_ops msm_hsl_dev_pm_ops = {
	.suspend = msm_serial_hsl_suspend,
	.resume = msm_serial_hsl_resume,
	.runtime_suspend = msm_hsl_runtime_suspend,
	.runtime_resume = msm_hsl_runtime_resume,
};

static struct platform_driver msm_hsl_platform_driver = {
	.probe = msm_serial_hsl_probe,
	.remove = __devexit_p(msm_serial_hsl_remove),
	.driver = {
// <iDPower_002> mod S
//		.name = "msm_serial_hsl",
		.name = "msm_serial_hsl2",
// <iDPower_002> mod E
		.owner = THIS_MODULE,
		.pm = &msm_hsl_dev_pm_ops,
		.of_match_table = msm_hsl_match_table,
	},
};

// <iDPower_002> add S
/* for file operation override */
static struct file_operations msm_hsl2_fops;

static inline struct tty_struct *file_tty(struct file *file)
{
        return ((struct tty_file_private *)file->private_data)->tty;
}

static ssize_t buffer_size_check(size_t buf_size, size_t count)
{
	int ret = 0;

	DEBUG_FD_LOG_IF_FNC_PARAM("count=%d, buf_size=%d", count, buf_size);
	if (count >= N_TTY_BUF_SIZE) {
		DEBUG_FD_LOG_IF_ASSERT("count is larger than buffer size.");
		ret = -EINVAL;
	}

	DEBUG_FD_LOG_IF_FNC_PARAM("ret=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief AUTOPOLL UARTCC state setting
 *
 *   @par   Outline:\n
 *          State judging by polling state and uartcc_state setting.
 *
 *   @param[in] polling_data  POLLING_OUT_H:polling start
 *                            POLLING_OUT_L:polling not start
 *
 *   @retval 0        Normal end
 *   @retval -EIO  I/O error
 *   @retval -SNFC_UCC_BUSY_ERROR  UART busy
 *   @retval -SNFC_UCC_ABNORMAL_ERROR  abnormal error
 *
 *   @par Special note
 *     - none
 **/
int Uartdrv_pollstart_set_uartcc_state(unsigned int polling_data)
{
	int ret = SNFC_UCC_NOERR;

	DEBUG_FD_LOG_IF_FNC_ST("start [s_uartcc_state=%d]", s_uartcc_state);

	mutex_lock(&uartcc_state_mutex);

	switch (s_uartcc_state) {
	case SNFC_STATE_WAITING_POLL_END:
		ret = SNFC_UCC_ABNORMAL_ERROR;
		break;
	case SNFC_STATE_NFC_POLLING:
		ret = SNFC_UCC_ABNORMAL_ERROR;
		break;
	case SNFC_STATE_FELICA_ACTIVE:
		ret = SNFC_UCC_BUSY_ERROR;
		break;
	case SNFC_STATE_IDLE:
		if (polling_data == POLLING_OUT_H) {
			s_uartcc_state = SNFC_STATE_NFC_POLLING;
		}
		else if (polling_data == POLLING_OUT_L) {
			/* No Changes */
		}
		else {}
		break;
	default:
		ret = -EIO;
		DEBUG_FD_LOG_IF_ASSERT("polling_data=%d", polling_data);
		break;
	}

	mutex_unlock(&uartcc_state_mutex);

	DEBUG_FD_LOG_IF_FNC_ED("return=%d [s_uartcc_state=%d]", ret, s_uartcc_state);

	return ret;
}

/**
 *   @brief AUTOPOLL UARTCC state setting
 *
 *   @par   Outline:\n
 *          State judging by The end of Autopolling and uartcc_state setting.
 *
 *   @param
 *
 *   @retval 0        Normal end
 *   @retval -EIO  I/O error
 *   @retval -SNFC_UCC_ABNORMAL_ERROR  abnormal error
 *
 *   @par Special note
 *     - none
 **/
int Uartdrv_pollend_set_uartcc_state(void)
{
	int ret = SNFC_UCC_NOERR;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	mutex_lock(&uartcc_state_mutex);

	switch (s_uartcc_state) {
	case SNFC_STATE_WAITING_POLL_END:
		s_uartcc_state = SNFC_STATE_FELICA_ACTIVE;
		break;
	case SNFC_STATE_NFC_POLLING:
		s_uartcc_state = SNFC_STATE_IDLE;
		break;
	case SNFC_STATE_FELICA_ACTIVE:
		ret = SNFC_UCC_ABNORMAL_ERROR;
		break;
	case SNFC_STATE_IDLE:
			/* No Changes */
		break;
	default:
		ret = -EIO;
		DEBUG_FD_LOG_IF_ASSERT("return=%d", ret);
		break;
	}

	mutex_unlock(&uartcc_state_mutex);

    DEBUG_FD_LOG_IF_FNC_ED("return=%d [s_uartcc_state=%d]", ret, s_uartcc_state);
	return ret;
}

/**
 *   @brief AUTOPOLL UARTCC state setting
 *
 *   @par   Outline:\n
 *          State judging by FeliCa open and uartcc_state setting.
 *
 *   @param
 *
 *   @retval 0        Normal end
 *   @retval -EIO  I/O error
 *   @retval -SNFC_UCC_OPEN_ERROR  open error
 *   @retval -SNFC_UCC_MULTIPLE_OPEN_ERROR  Double open error
 *
 *   @par Special note
 *     - none
 **/
static int Uartdrv_felica_open_set_uartcc_state(void)
{
	int ret = SNFC_UCC_NOERR;
	int retry_cnt;
	unsigned int   hsel_data;

	DEBUG_FD_LOG_IF_FNC_ST("start  [s_uartcc_state=%d]", s_uartcc_state);

	mutex_lock(&uartcc_state_mutex);

	switch (s_uartcc_state) {
	case SNFC_STATE_WAITING_POLL_END:
		ret = SNFC_UCC_MULTIPLE_OPEN_ERROR;
		break;
	case SNFC_STATE_NFC_POLLING:
		s_uartcc_state = SNFC_STATE_WAITING_POLL_END;
		for (retry_cnt = 0; retry_cnt < SNFC_CTRL_RETRY_COUNT; retry_cnt++) {
			ret = SnfcCtrl_terminal_hsel_read_dd(&hsel_data);
			if ((hsel_data == HSEL_OUT_L)&&(ret == 0)) {
			    /* HSEL=L(FeliCa enable) */
				break;
			}
			msleep(30);/*30ms Waiting */
		}/* for */
		break;
	case SNFC_STATE_FELICA_ACTIVE:
	        ret = SNFC_UCC_MULTIPLE_OPEN_ERROR;
		break;
	case SNFC_STATE_IDLE:
		ret = SnfcCtrl_terminal_hsel_read_dd(&hsel_data);
		if ((hsel_data == HSEL_OUT_L)&&(ret == 0)) {
			s_uartcc_state = SNFC_STATE_FELICA_ACTIVE;
		}
		else {
			ret = SNFC_UCC_OPEN_ERROR;
		}
		break;
	default:
		ret = -EIO;
		DEBUG_FD_LOG_IF_ASSERT("return =%d", ret);
		break;
	}

	mutex_unlock(&uartcc_state_mutex);

	DEBUG_FD_LOG_IF_FNC_ED("return=%d [s_uartcc_state=%d]", ret, s_uartcc_state);
	return ret;
}

/**
 *   @brief AUTOPOLL UARTCC state setting
 *
 *   @par   Outline:\n
 *          State judging by FeliCa close and uartcc_state setting.
 *
 *   @param
 *
 *   @retval 0        Normal end
 *   @retval -EIO  I/O error
 *   @retval -SNFC_UCC_CLOSE_ERROR  close error
 *
 *   @par Special note
 *     - none
 **/
static int Uartdrv_felica_close_set_uartcc_state(void)
{
	int ret = SNFC_UCC_NOERR;

	DEBUG_FD_LOG_IF_FNC_ST("start  [s_uartcc_state=%d]", s_uartcc_state);

	mutex_lock(&uartcc_state_mutex);

	switch (s_uartcc_state) {
	case SNFC_STATE_WAITING_POLL_END:
		ret = SNFC_UCC_CLOSE_ERROR;
		break;
	case SNFC_STATE_NFC_POLLING:
		ret = SNFC_UCC_CLOSE_ERROR;
		break;
	case SNFC_STATE_FELICA_ACTIVE:
		s_uartcc_state = SNFC_STATE_IDLE;
		break;
	case SNFC_STATE_IDLE:
	        ret = SNFC_UCC_CLOSE_ERROR;
		break;
	default:
		ret = -EIO;
		DEBUG_FD_LOG_IF_ASSERT("return=%d", ret);
		break;
	}

	mutex_unlock(&uartcc_state_mutex);

    DEBUG_FD_LOG_IF_FNC_ED("return=%d [s_uartcc_state=%d]", ret, s_uartcc_state);
    
	return ret;
}

/**
 *   @brief AUTOPOLL UARTCC state get(External reference)
 *
 *   @par   Outline:\n
 *          uartcc state get.
 *
 *   @param[in] *uartcc_state  uartcc state is acquired. 
 *
 *   @retval
 *
 *   @par Special note
 *     - none
 **/
int Uartdrv_uartcc_state_read_dd(unsigned int *uartcc_state)
{
	DEBUG_FD_LOG_IF_FNC_ST("start");

	if (uartcc_state == NULL) {
		DEBUG_FD_LOG_IF_ASSERT("intu_data=NULL");
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}
    
	*uartcc_state = s_uartcc_state;

	DEBUG_FD_LOG_IF_FNC_ED("return");
    
	return 1;
}
// <Combo_chip> add E


// <Combo_chip> add S
/**
 *   @brief Open judging for NFC or FeliCa
 *
 *   @par   Outline:\n
 *          Open judgment processing
 *
 *   @param[in] *inode  pointer of inode infomation struct
 *   @param[in] *filp   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EIO  I/Oerror
 *   @retval -SNFC_UCC_OPEN_ERROR  open error
 *   @retval -SNFC_UCC_MULTIPLE_OPEN_ERROR  Double open error
 *
 *   @par Special note
 *     - none
 **/
static int msm_hsl2_open_combo(struct inode *inode, struct file *filp)
{
	int ret = 0;
	unsigned int minor = 0;
    
	DEBUG_FD_LOG_IF_FNC_ST("start %d %d",s_felica_openclose,s_uartcc_state);
	
	minor = iminor(inode);
	switch (minor) {
	case 0:/* FeliCa */
	    if (s_felica_openclose != SNFC_FELICA_OPEN_2ND) {
		ret = Uartdrv_felica_open_set_uartcc_state();
		if (ret != 0) {
			DEBUG_FD_LOG_IF_ASSERT("return =%d", ret);
			DEBUG_FD_LOG_IF_FNC_ED("return =%d", ret);
    	        s_felica_openclose = SNFC_FELICA_OPEN_1ST;
			return ret;
		}
	        /* FeliCa UART open(1st)*/
    		ret = wrp_tty_open(inode, filp);
	        s_felica_openclose = SNFC_FELICA_OPEN_2ND;
    		break;
	    }
	    else {
	        /* FeliCa UART open(2nd)*/
		ret = wrp_tty_open(inode, filp);
	        s_felica_openclose = SNFC_FELICA_CLOSE_1ST;
    		break;
	    }
	case 1:/* NFC */
		/* NFC UART open*/
		ret = wrp_tty_open(inode, filp);
		break;
	default:
		ret = -EIO;
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}
    
	DEBUG_FD_LOG_IF_FNC_ED("return=%d  %d %d",ret,s_felica_openclose,s_uartcc_state);
	return ret;
}
// <Combo_chip> add E

static int msm_hsl2_close(struct inode *inode, struct file *filp)
{
	struct tty_struct *tty = file_tty(filp);
	struct uart_state *state;
	struct uart_port *port;
	struct msm_hsl_port *msm_hsl_port;
	int ret;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if (tty_paranoia_check(tty, inode, "msm_hsl2_close")) {
		return -EIO;
	}

	state = tty->driver_data;
	if (!state) {
		DEBUG_FD_LOG_IF_ASSERT("tty->driver_data=%p", state);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	port = state->uart_port;
	if (!port) {
		DEBUG_FD_LOG_IF_ASSERT("state->uart_port=%p", port);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	msm_hsl_port = UART_TO_MSM(port);
	if (!msm_hsl_port) {
		DEBUG_FD_LOG_IF_ASSERT("msm_hsl_port=%p", msm_hsl_port);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	msm_hsl_port->fsync_flag = 1;
	wake_up(&msm_hsl_port->fsync_timer);

	ret = wrp_tty_release(inode, filp);
// <iDPower_009> add S
    wake_unlock(&felica_wake_lock);
// <iDPower_009> add E
	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}

// <Combo_chip> add S
/**
 *   @brief Close judging for NFC or FeliCa
 *
 *   @par   Outline:\n
 *          Close judgment processing
 *
 *   @param[in] *inode  pointer of inode infomation struct
 *   @param[in] *filp   pointer of file infomation struct
 *
 *   @retval 0        Normal end
 *   @retval -EINVAL  Invalid argument
 *   @retval -EIO     I/O error
 *   @retval -SNFC_UCC_CLOSE_ERROR  close error
 *
 *   @par Special note
 *     - none
 **/
static int msm_hsl2_close_combo(struct inode *inode, struct file *filp)
{
	int ret = 0;
	unsigned int minor = 0;
    
	DEBUG_FD_LOG_IF_FNC_ST("start %d %d",s_felica_openclose,s_uartcc_state);
	
	minor = iminor(inode);
	switch (minor) {
	case 0:/* FeliCa */
	    if (s_felica_openclose == SNFC_FELICA_CLOSE_1ST) {
	        /* FeliCa UART close(1st) */
    		ret = msm_hsl2_close(inode, filp);
	        s_felica_openclose = SNFC_FELICA_CLOSE_2ND;
	    }
	    else {
		ret = Uartdrv_felica_close_set_uartcc_state();
		if (ret != 0) {
			DEBUG_FD_LOG_IF_ASSERT("return =%d", ret);
			DEBUG_FD_LOG_IF_FNC_ED("return =%d", ret);
    	        s_felica_openclose = SNFC_FELICA_OPEN_1ST;
			return ret;
		}
	        /* FeliCa UART close(2nd) */
		ret = msm_hsl2_close(inode, filp);
	        s_felica_openclose = SNFC_FELICA_OPEN_1ST;
	        s_felica_close_flg = 1;
	    }
		break;
	case 1:/* NFC */
                //npdc300061351
		//if(s_uartcc_state == SNFC_STATE_FELICA_ACTIVE) {
                //    ret = SNFC_UCC_CLOSE_ERROR;
                //    break;
		//}
        
		/* NFC UART close */
		ret = msm_hsl2_close(inode, filp);
		break;
	default:
		ret = -EIO;
		DEBUG_FD_LOG_IF_ASSERT("minor=%d", minor);
		break;
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d  %d %d",ret,s_felica_openclose,s_uartcc_state);
	return ret;
}
// <Combo_chip> add E

static ssize_t msm_hsl2_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	ssize_t ret;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	ret = buffer_size_check(sizeof(buf), count);
	if (ret == 0) {
// <Combo_chip> add S
	    if (s_felica_close_flg == 1) {
	        ret = wrp_tty_release(file->f_path.dentry->d_inode,file);
            if (ret < 0) {
                DEBUG_FD_LOG_IF_FNC_ED("return=%d [tty_release]", ret);
                s_felica_close_flg = 0;
                return -EINVAL;
            }
	        ret = wrp_tty_open(file->f_path.dentry->d_inode,file);
	        if(ret<0){
	            DEBUG_FD_LOG_IF_FNC_ED("return=%d [tty_open(read)]", ret);
	            s_felica_close_flg = 0;
	            return -EINVAL;
	        }
	        s_felica_close_flg = 0;
	    }
// <Combo_chip> add E
		ret = tty_read(file, buf, count, ppos);
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}

static ssize_t msm_hsl2_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	ssize_t ret;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	ret = buffer_size_check(sizeof(buf), count);
	if (ret == 0) {
// <Combo_chip> add S
	    if (s_felica_close_flg == 1) {
	        ret = wrp_tty_release(file->f_path.dentry->d_inode,file);
            if (ret < 0) {
                DEBUG_FD_LOG_IF_FNC_ED("return=%d [tty_release]", ret);
                s_felica_close_flg = 0;
                return -EINVAL;
            }
	        ret = wrp_tty_open(file->f_path.dentry->d_inode,file);
	        if(ret<0){
	            DEBUG_FD_LOG_IF_FNC_ED("return=%d [tty_open(write)]", ret);
	            s_felica_close_flg = 0;
	            return -EINVAL;
	        }
	        s_felica_close_flg = 0;
	    }
// <Combo_chip> add E
		ret = tty_write(file, buf, count, ppos);
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;
}

#ifdef DEBUG_SERIAL_MSM_HSL2
static long msm_hsl2_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	ssize_t ret;

	DEBUG_FD_LOG_IF_FNC_ST("start");
	
	ret = tty_ioctl(file, cmd, arg);

	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
	return ret;

}
#endif /* DEBUG_SERIAL_MSM_HSL2 */

/* <Quad> mod 0912 */
/*static int msm_hsl2_fsync(struct file *file, int datasync) */
static int msm_hsl2_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	struct tty_struct *tty = file_tty(file);
	struct uart_state *state;
	struct uart_port *port;
	struct msm_hsl_port *msm_hsl_port;
	struct circ_buf *circ;
	int ret;

	DEBUG_FD_LOG_IF_FNC_ST("start");

	if (tty_paranoia_check(tty, inode, "msm_hsl2_fsync")) {
		return -EIO;
	}

	state = tty->driver_data;
	if (!state) {
		DEBUG_FD_LOG_IF_ASSERT("tty->driver_data=%p", state);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	port = state->uart_port;
	if (!port) {
		DEBUG_FD_LOG_IF_ASSERT("state->uart_port=%p", port);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	msm_hsl_port = UART_TO_MSM(port);
	if (!msm_hsl_port) {
		DEBUG_FD_LOG_IF_ASSERT("msm_hsl_port=%p", msm_hsl_port);
		DEBUG_FD_LOG_IF_FNC_ED("return=%d", -EINVAL);
		return -EINVAL;
	}

	circ = &port->state->xmit;
	msm_hsl_port->fsync_flag = 0;
	if (!uart_circ_empty(circ)) {
		ret = wait_event_timeout(
			msm_hsl_port->fsync_timer,
			msm_hsl_port->fsync_flag,
			FSYNC_TIMER_EXPIRE
		);
		if (ret == 0) {
			DEBUG_FD_LOG_IF_ASSERT("fsync timeout.");
		}
	}

	DEBUG_FD_LOG_IF_FNC_ED("return=0");
	return 0;
}
// <iDPower_002> add E

static int __init msm_serial_hsl_init(void)
{
	int ret;
// <iDPower_002> add S
	struct uart_driver *uart_drv = &msm_hsl_uart_driver;
	struct tty_driver  *tty_drv;
// <Combo_chip> add S
	struct uart_driver *uart_drv_nfc = &msm_hsl_uart_driver_nfc;
	struct tty_driver  *tty_drv_nfc;
// <Combo_chip> add E

	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E
    
// <Combo_chip> add S
	s_uartcc_state = SNFC_STATE_IDLE;
        s_irq_requested_cnt = 0;                             /* npdc300056782 add */
	s_felica_openclose = SNFC_FELICA_OPEN_1ST;
    s_felica_close_flg = 0;
// <Combo_chip> add E

	ret = uart_register_driver(&msm_hsl_uart_driver);
	if (unlikely(ret))
		return ret;
// <Combo_chip> add S
	ret = uart_register_driver(&msm_hsl_uart_driver_nfc);
	if (unlikely(ret)) {
		return ret;
	}
// <Combo_chip> add E

// <iDPower_002> mod S
//	debug_base = debugfs_create_dir("msm_serial_hsl", NULL);
	debug_base = debugfs_create_dir("msm_serial_hsl2", NULL);
// <iDPower_002> mod E
	if (IS_ERR_OR_NULL(debug_base))
		pr_err("%s():Cannot create debugfs dir\n", __func__);

	ret = platform_driver_register(&msm_hsl_platform_driver);
    if (unlikely(ret)) {
		uart_unregister_driver(&msm_hsl_uart_driver);
// <Combo_chip> add S
		uart_unregister_driver(&msm_hsl_uart_driver_nfc);
// <Combo_chip> add E
    }

// <iDPower_002> add S
	/* file operation override */
	tty_default_fops(&msm_hsl2_fops);
// <Combo_chip> mod S
//#ifdef DEBUG_SERIAL_MSM_HSL2
//	msm_hsl2_fops.open           = msm_hsl2_open;
	msm_hsl2_fops.open           = msm_hsl2_open_combo;
//#endif /* DEBUG_SERIAL_MSM_HSL2 */
//	msm_hsl2_fops.release        = msm_hsl2_close;
	msm_hsl2_fops.release        = msm_hsl2_close_combo;
// <Combo_chip> mod E
	msm_hsl2_fops.read           = msm_hsl2_read;
	msm_hsl2_fops.write          = msm_hsl2_write;
#ifdef DEBUG_SERIAL_MSM_HSL2
	msm_hsl2_fops.unlocked_ioctl = msm_hsl2_ioctl;
#endif /* DEBUG_SERIAL_MSM_HSL2 */
	msm_hsl2_fops.fsync          = msm_hsl2_fsync;
	tty_drv = uart_drv->tty_driver;
	tty_drv->cdev.ops = &msm_hsl2_fops;
// <Combo_chip> add S
	tty_drv_nfc = uart_drv_nfc->tty_driver;
	tty_drv_nfc->cdev.ops = &msm_hsl2_fops;
// <Combo_chip> add E
// <iDPower_002> add E

	printk(KERN_INFO "msm_serial_hsl: driver initialized\n");

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return=%d", ret);
// <iDPower_002> add E
	return ret;
}

static void __exit msm_serial_hsl_exit(void)
{
// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ST("start");
// <iDPower_002> add E

	debugfs_remove_recursive(debug_base);
// <iDPower_002> del S
//#ifdef CONFIG_SERIAL_MSM_HSL_CONSOLE
//	unregister_console(&msm_hsl_console);
//#endif
// <iDPower_002> del E
	platform_driver_unregister(&msm_hsl_platform_driver);
	uart_unregister_driver(&msm_hsl_uart_driver);
// <Combo_chip> add S
	uart_unregister_driver(&msm_hsl_uart_driver_nfc);
// <Combo_chip> add E

// <iDPower_002> add S
	DEBUG_FD_LOG_IF_FNC_ED("return");
// <iDPower_002> add E
}

EXPORT_SYMBOL(Uartdrv_uartcc_state_read_dd);
EXPORT_SYMBOL(Uartdrv_pollstart_set_uartcc_state);
EXPORT_SYMBOL(Uartdrv_pollend_set_uartcc_state);
module_init(msm_serial_hsl_init);
module_exit(msm_serial_hsl_exit);

MODULE_DESCRIPTION("Driver for msm HSUART serial device");
MODULE_LICENSE("GPL v2");
