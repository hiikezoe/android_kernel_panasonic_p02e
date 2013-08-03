/* include/linux/belogdrv.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Author: Robert Love <rlove@android.com>
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

#ifndef _LINUX_BELOGDRV_H
#define _LINUX_BELOGDRV_H

#include <linux/types.h>
#include <linux/ioctl.h>

struct belogdrv_entry {
	__s32		sec;	/* seconds since Epoch */
	__s32		nsec;	/* nanoseconds */
	char		msg[0];	/* the entry's payload */
};

struct belog_body {
	char		msg[32];
	char		type;
	char		state;
	short		d1;
	short		d2;
};

struct belogdrv_buffer {
	uint32_t	sig;
	size_t		head;   // head
	size_t		w_off;  // w_off
	uint8_t		data[0];
};

#define BELOGDRV_LOG			"belog"

#define BELOGDRV_ENTRY_MAX_LEN            (4*1024)
#define BELOGDRV_ENTRY_MAX_PAYLOAD	\
	(BELOGDRV_ENTRY_MAX_LEN - sizeof(struct belogdrv_entry))

#define __BELOGDRVIO	0xAE

#define BELOGDRV_GET_LOG_BUF_SIZE	_IO(__BELOGDRVIO, 1) /* size of log */
#define BELOGDRV_GET_LOG_LEN		_IO(__BELOGDRVIO, 2) /* used log len */
#define BELOGDRV_GET_NEXT_ENTRY_LEN	_IO(__BELOGDRVIO, 3) /* next entry len */
#define BELOGDRV_FLUSH_LOG		_IO(__BELOGDRVIO, 4) /* flush log */

#endif /* _LINUX_BELOGDRV_H */
