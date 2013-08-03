/*
 *  drivers/mtdwrap/mtdwrap.c
 *
 *  Copyright (C) 2010 Panasonic
 *  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/stddef.h>

#include <linux/err.h>
#include <linux/errno.h>

#include <linux/mtd/mtd.h>
#include <linux/mtdwrap.h>


struct mtdwrapdrv_operations mtdwrap_drv_ops;

/**
 * mtdwrap_read_bbm - Read data from flash
 * @param partno    partition no
 * @param from      offset to read from
 * @param len       number of bytes to read
 * @param retlen    pointer to variable to store the number of read bytes
 * @param buf       the databuffer to put data
 *
 * Read with ecc
*/
int mtdwrap_read_bbm( int partno, loff_t from,
			size_t len, u_char *buf )
{
	struct mtd_info *mtd;
	int ret = 0;
	size_t retlen;
	size_t tmp_len;
	u_char *tmp_buf;
	unsigned long p_from = 0;

	if (!buf)
		return -EINVAL;

	tmp_buf = buf;
	
	if( mtdwrap_drv_ops.convert != NULL )
	{
		p_from = mtdwrap_drv_ops.convert( partno, (unsigned long)from );
		if( p_from == -1 )
		{
			printk("mtdwrap_read_bbm convert error!!\n");
			return -EFAULT;
		}
		from = (loff_t)p_from;
	}
	
	mtd = get_mtd_device(NULL, partno);
	if (IS_ERR(mtd))
		return -ENODEV;
	
	if (((from + len) > mtd->size) ||
	    ((from & (mtd->writesize - 1)) != 0) ||
	    ((len & (mtd->writesize - 1)) != 0) ||
	    (((from & (mtd->erasesize - 1)) + len) > mtd->erasesize)) {
		put_mtd_device(mtd);
		return -EINVAL;
	}
	
	while (len > 0) {
		if (len > mtd->writesize)
			tmp_len = mtd->writesize;
		else
			tmp_len = len;
		
		ret = mtd->read(mtd, from, tmp_len, &retlen, tmp_buf);
		if ((ret != 0) && (ret != -EUCLEAN)) {
			printk("mtdwrap_read_bbm mtd read error %d!!\n",ret);
			break;
		}
		len -= tmp_len;
		from += tmp_len;
		tmp_buf += tmp_len;
		ret = 0;
	}

	put_mtd_device(mtd);
	return ret;
}
EXPORT_SYMBOL_GPL(mtdwrap_read_bbm);
EXPORT_SYMBOL(mtdwrap_drv_ops);
