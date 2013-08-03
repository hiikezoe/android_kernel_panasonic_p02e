/*
 *  include/linux/mtdwrap.h
 *
 *  Copyright (C) 2010 Panasonic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MTDWRAP_H__
#define __MTDWRAP_H__

#ifdef __KERNEL__
#include <linux/types.h>
#endif /* __KERNEL__ */

#define MMC_MODEM_PART            ( 1)
#define MMC_MODEM_DEV_NAME        ("/dev/block/mmcblk0p1")

#define MMC_SBL1_PART             ( 2)
#define MMC_SBL1_DEV_NAME         ("/dev/block/mmcblk0p2")

#define MMC_SBL2_PART             ( 3)
#define MMC_SBL2_DEV_NAME         ("/dev/block/mmcblk0p3")

#define MMC_SBL3_PART             ( 4)
#define MMC_SBL3_DEV_NAME         ("/dev/block/mmcblk0p4")

#define MMC_ABOOT_PART            ( 5)
#define MMC_ABOOT_DEV_NAME        ("/dev/block/mmcblk0p5")

#define MMC_RPM_PART              ( 6)
#define MMC_RPM_DEV_NAME          ("/dev/block/mmcblk0p6")

#define MMC_BOOT_PART             ( 7)
#define MMC_BOOT_DEV_NAME         ("/dev/block/mmcblk0p7")

#define MMC_TZ_PART               ( 8)
#define MMC_TZ_DEV_NAME           ("/dev/block/mmcblk0p8")

#define MMC_PAD_PART              ( 9)
#define MMC_PAD_DEV_NAME          ("/dev/block/mmcblk0p9")

#define MMC_MODEMST1_PART         (10)
#define MMC_MODEMST1_DEV_NAME     ("/dev/block/mmcblk0p10")

#define MMC_MODEMST2_PART         (11)
#define MMC_MODEMST2_DEV_NAME     ("/dev/block/mmcblk0p11")

#define MMC_SYSTEM_PART           (12)
#define MMC_SYSTEM_DEV_NAME       ("/dev/block/mmcblk0p12")

#define MMC_USERDATA_PART         (13)
#define MMC_USERDATA_DEV_NAME     ("/dev/block/mmcblk0p13")

#define MMC_PERSIST_PART          (14)
#define MMC_PERSIST_DEV_NAME      ("/dev/block/mmcblk0p14")

#define MMC_CACHE_PART            (15)
#define MMC_CACHE_DEV_NAME        ("/dev/block/mmcblk0p15")

#define MMC_TOMBSTONES_PART       (16)
#define MMC_TOMBSTONES_DEV_NAME   ("/dev/block/mmcblk0p16")

#define MMC_MISC_PART             (17)
#define MMC_MISC_DEV_NAME         ("/dev/block/mmcblk0p17")

#define MMC_RECOVERY_PART         (18)
#define MMC_RECOVERY_DEV_NAME     ("/dev/block/mmcblk0p18")

#define MMC_M9KEFS1_PART          (19)
#define MMC_M9KEFS1_DEV_NAME      ("/dev/block/mmcblk0p19")

#define MMC_M9KEFS2_PART          (20)
#define MMC_M9KEFS2_DEV_NAME      ("/dev/block/mmcblk0p20")

#define MMC_M9KEFS3_PART          (21)
#define MMC_M9KEFS3_DEV_NAME      ("/dev/block/mmcblk0p21")

#define MMC_DDR_PART              (22)
#define MMC_DDR_DEV_NAME          ("/dev/block/mmcblk0p22")

#define MMC_M9KEFSC_PART          (23)
#define MMC_M9KEFSC_DEV_NAME      ("/dev/block/mmcblk0p23")

#define MMC_GROW_PART             (24)
#define MMC_GROW_DEV_NAME         ("/dev/block/mmcblk0p24")

#define MMC_SSD_PART              (25)
#define MMC_SSD_DEV_NAME          ("/dev/block/mmcblk0p25")

#define MMC_FOTADELTA_PART        (26)
#define MMC_FOTADELTA_DEV_NAME    ("/dev/block/mmcblk0p26")

#define MMC_PRIVATE1_PART         (27)
#define MMC_PRIVATE1_DEV_NAME     ("/dev/block/mmcblk0p27")

#define MMC_PRIVATE2_PART         (28)
#define MMC_PRIVATE2_DEV_NAME     ("/dev/block/mmcblk0p28")

#define MMC_LOG_PART              (29)
#define MMC_LOG_DEV_NAME          ("/dev/block/mmcblk0p29")

#define MMC_LOG3_PART             (30)
#define MMC_LOG3_DEV_NAME         ("/dev/block/mmcblk0p30")

#define MMC_RECOVERY_B_PART       (30)
#define MMC_RECOVERY_B_DEV_NAME   ("/dev/block/mmcblk0p30")

#define MMC_LOG4_PART             (31)
#define MMC_LOG4_DEV_NAME         ("/dev/block/mmcblk0p31")

#define MMC_FOTA_M_PART           (32)
#define MMC_FOTA_M_DEV_NAME       ("/dev/block/mmcblk0p32")

#define MMC_FOTA_B_PART           (33)
#define MMC_FOTA_B_DEV_NAME       ("/dev/block/mmcblk0p33")

#define MMC_CFG_TBL_PART          (34)
#define MMC_CFG_TBL_DEV_NAME      ("/dev/block/mmcblk0p34")

#define MMC_FACTORIES_PART        (35)
#define MMC_FACTORIES_DEV_NAME    ("/dev/block/mmcblk0p35")

#define MMC_RECOVERY_K_B_PART     (36)
#define MMC_RECOVERY_K_B_DEV_NAME ("/dev/block/mmcblk0p36")

#define MMC_BLOCK1_PART           (37)
#define MMC_BLOCK1_DEV_NAME       ("/dev/block/mmcblk0p37")

#define MMC_CFGDATA_PART          (38)
#define MMC_CFGDATA_DEV_NAME      ("/dev/block/mmcblk0p38")

#define MMC_CFG_TBL_OFFSET        (0x00000000)
#define MMC_SI_VER_OFFSET         (0x00040000)
#define MMC_FGIC_FW_OFFSET        (0x00080000)
#define MMC_CAM_PARM_OFFSET       (0x000C0000)
#define MMC_WLCHG_FW_OFFSET       (0x002C0000)
#define MMC_TP_PARM_OFFSET        (0x00300000)

#ifdef __KERNEL__
int mtdwrap_read_bbm( int partno, loff_t from, 
			size_t len, u_char *buf );

/* operations of generic driver for NAND */
struct mtdwrapdrv_operations
{
    unsigned long (*convert)(int, unsigned long);
};

extern struct mtdwrapdrv_operations mtdwrap_drv_ops;
#endif /* __KERNEL__ */

#endif /* __MTDWRAP_H__ */
