/*
 *  Copyright (C) 2011 Panasonic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/fs.h>

#include <linux/err.h>
#include <linux/errno.h>

#define MAX_PATHSIZE	32
#define PATHNAME_BASE	"/dev/block/mmcblk0p"

#define DEBUG_PRINT
#ifdef DEBUG_PRINT
#define debug_print(fmt,args...)  printk (fmt ,##args)
#else
#define debug_print(fmt,args...)
#endif /* DEBUG_PRINT */

int mtdwrap_read_bbm( int partno, loff_t from, size_t len, u_char *buf )
{
	int ret;
	size_t retlen;
	char pathname[MAX_PATHSIZE];
	struct file *fp;
	struct inode *inode;
	loff_t size;

	debug_print("mtdwrap_read_bbm() enter with %d, %lld, %u, %p\n", partno, from, len, buf);

	if (!buf)
		return -EINVAL;

	ret = snprintf(pathname, MAX_PATHSIZE, "%s%d", PATHNAME_BASE, partno);
	if ( ret < 0 ) {
		return -EINVAL;
	}
	debug_print("pathname: %s\n", pathname);

	fp = filp_open(pathname, O_RDONLY | O_LARGEFILE, 0);
	if (IS_ERR(fp)) {
		debug_print( "Failed to open\n" );
		return PTR_ERR(fp);
	}

	inode = fp->f_path.dentry->d_inode;
	if (!inode) {
		filp_close(fp, NULL);
		debug_print( "Failed to get inode\n" );
		return -EINVAL;
	}

	size = i_size_read(inode->i_mapping->host);
	if (size < 0) {
		filp_close(fp, NULL);
		debug_print( "Failed to get directory size\n" );
		return -EINVAL;
	}

	if(( from + len ) > size) {
		filp_close(fp, NULL);
		debug_print( "Too large. size=%lld\n", size );
		return -EINVAL;
    }

	retlen = kernel_read(fp, from, buf, len);
	if (retlen != len) {
		debug_print( "kernel_read() returned %d\n", retlen );
		ret = retlen < 0 ? retlen : -EIO;
	} else {
		ret = 0;
	}

	filp_close(fp, NULL);
	debug_print("mtdwrap_read_bbm() leave with %d\n",ret);
	return ret;
}
EXPORT_SYMBOL_GPL(mtdwrap_read_bbm);
