#ifndef _DTVD_TUNER_COM_H_
#define _DTVD_TUNER_COM_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/errno.h>
#include <linux/wait.h>          /* Linux2.6対応 */

#include <linux/semaphore.h>
#include <asm/atomic.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>
#ifdef TUNER_MCBSP_ENABLE
#include <plat/mcbsp.h>
#endif /* TUNER_MCBSP_ENABLE */
#ifdef TUNER_TSIF_ENABLE
#include <linux/tsif_api.h>
#endif /* TUNER_TSIF_ENABLE */
#include <linux/i2c.h>

#include "dtvtuner.h"             /* 外部向けヘッダ */
#include "dtvd_tuner_def.h"       /* 定数宣言ヘッダ */
#include "dtvd_tuner_tag.h"       /* 構造体定義ヘッダ */
#include "dtvd_tuner_pro.h"       /* プロトタイプ宣言ヘッダ */
#include "dtvd_tuner_hw.h"        /* ハード関連定義ヘッダ */
#include "dtvd_tuner_tsprcv.h"    /* TSパケット受信部関連ヘッダ */

#endif /* _DTVD_TUNER_COM_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
