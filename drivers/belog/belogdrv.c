/*
 * drivers/misc/belogdrv.c
 *
 * A Logging Subsystem
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * Robert Love <rlove@google.com>
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

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include "belogdrv.h"

#include <asm/ioctls.h>

#include <linux/laputa-fixaddr.h>

#ifndef DMEM_BELOG_APL_HIST_START
#define DMEM_BELOG_APL_HIST_START 0x90340000
#endif

#define BELOG_BUFFER_SIZE        (0x3FFDC)  //256KB
//#define BELOG_BUFFER_SIZE        (0x1FFEE)  //128KB
//#define BELOG_BUFFER_SIZE        (0x7FF0)   // 32KB
#define BELOG_IOREMAP_BUFFER_SIZE  (BELOG_BUFFER_SIZE + sizeof(struct belogdrv_buffer))

#define BELOG_DRV_SIG              (0x43474244) /* DBGC */

#define BELOG_WAIT_EVENTS          (0x00)
#define BELOG_WAKE_UP_BUFFER_FULL  (0x01)
#define BELOG_WAKE_UP_SHUTDOWN     (0x02)

/*
 * struct belogdrv_log - represents a specific log, such as 'main' or 'radio'
 *
 * This structure lives from module insertion until module removal, so it does
 * not need additional reference counting. The structure is protected by the
 * mutex 'mutex'.
 */
struct belogdrv_log {
	unsigned char 		*buffer;/* the ring buffer itself */
	struct miscdevice	misc;	/* misc device representing the log */
	wait_queue_head_t	wq;	/* wait queue for readers */
	struct list_head	readers; /* this log's readers */
	struct mutex		mutex;	/* mutex protecting buffer */
	//size_t			w_off;	/* current write head offset */
	size_t			*w_off;
	//size_t			head;	/* new readers start here */
	size_t			*head;
	size_t			size;	/* size of the log */
};

/*
 * struct belogdrv_reader - a logging device open for reading
 *
 * This object lives from open to release, so we don't need additional
 * reference counting. The structure is protected by log->mutex.
 */
struct belogdrv_reader {
	struct belogdrv_log	*log;	/* associated log */
	struct list_head	list;	/* entry in belogdrv_log's list */
	size_t			r_off;	/* current read head offset */
//	size_t			*r_off;
};

wait_queue_head_t              belog_task_wq;
spinlock_t                     belog_kthread_spinlock;
static char                   *belog_copy_buf;
size_t                         belog_copy_w_off = 0;
size_t                         belog_copy_head  = 0;
volatile unsigned long         belog_kthread_flag = BELOG_WAIT_EVENTS;
static struct belogdrv_buffer *belogdrv_buffer;

/* belogdrv_offset - returns index 'n' into the log via (optimized) modulus */
#define belogdrv_offset(n)	((n) < (log->size) ? (n) : (n - log->size))

/*
 * file_get_belog - Given a file structure, return the associated log
 *
 * This isn't aesthetic. We have several goals:
 *
 * 	1) Need to quickly obtain the associated log during an I/O operation
 * 	2) Readers need to maintain state (belogdrv_reader)
 * 	3) Writers need to be very fast (open() should be a near no-op)
 *
 * In the reader case, we can trivially go file->belogdrv_reader->belogdrv_log.
 * For a writer, we don't want to maintain a belogdrv_reader, so we just go
 * file->belogdrv_log. Thus what file->private_data points at depends on whether
 * or not the file was opened for reading. This function hides that dirtiness.
 */
static inline struct belogdrv_log *file_get_belog(struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct belogdrv_reader *reader = file->private_data;
		return reader->log;
	} else
		return file->private_data;
}

/*
 * get_entry_len - Grabs the length of the payload of the next entry starting
 * from 'off'.
 *
 * Caller needs to hold log->mutex.
 */
static __u32 get_entry_len(struct belogdrv_log *log, size_t off)
{
#if 0
	__u16 val;

	switch (log->size - off) {
	case 1:
		memcpy(&val, log->buffer + off, 1);
		memcpy(((char *) &val) + 1, log->buffer, 1);
		break;
	default:
		memcpy(&val, log->buffer + off, 2);
	}

	return sizeof(struct belogdrv_entry) + val;
#endif
        return sizeof(struct belogdrv_entry) + sizeof(struct belog_body);
}

/*
 * do_read_log_to_user - reads exactly 'count' bytes from 'log' into the
 * user-space buffer 'buf'. Returns 'count' on success.
 *
 * Caller must hold log->mutex.
 */
static ssize_t do_read_log_to_user(struct belogdrv_log *log,
				   struct belogdrv_reader *reader,
				   char __user *buf,
				   size_t count)
{
	size_t len;

	/*
	 * We read from the log in two disjoint operations. First, we read from
	 * the current read head offset up to 'count' bytes or to the end of
	 * the log, whichever comes first.
	 */
	len = min(count, log->size - reader->r_off);
	if (copy_to_user(buf, log->buffer + reader->r_off, len))
		return -EFAULT;

	/*
	 * Second, we read any remaining bytes, starting back at the head of
	 * the log.
	 */
	if (count != len)
		if (copy_to_user(buf + len, log->buffer, count - len))
			return -EFAULT;

	reader->r_off = belogdrv_offset(reader->r_off + count);

	return count;
}

/*
 * belogdrv_read - our log's read() method
 *
 * Behavior:
 *
 * 	- O_NONBLOCK works
 * 	- If there are no log entries to read, blocks until log is written to
 * 	- Atomically reads exactly one log entry
 *
 * Optimal read size is BELOGDRV_ENTRY_MAX_LEN. Will set errno to EINVAL if read
 * buffer is insufficient to hold next entry.
 */
static ssize_t belogdrv_read(struct file *file, char __user *buf,
			   size_t count, loff_t *pos)
{
	struct belogdrv_reader *reader = file->private_data;
	struct belogdrv_log *log = reader->log;
	ssize_t ret;
	DEFINE_WAIT(wait);

start:
	while (1) {
		prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

		mutex_lock(&log->mutex);
		ret = (*(log->w_off) == reader->r_off);
		mutex_unlock(&log->mutex);
		if (!ret)
			break;

		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -EINTR;
			break;
		}

		schedule();
	}

	finish_wait(&log->wq, &wait);
	if (ret)
		return ret;

	mutex_lock(&log->mutex);

	/* is there still something to read or did we race? */
	if (unlikely(*(log->w_off) == reader->r_off)) {
		mutex_unlock(&log->mutex);
		goto start;
	}

	/* get the size of the next entry */
	ret = get_entry_len(log, reader->r_off);
	if (count < ret) {
		ret = -EINVAL;
		goto out;
	}

	/* get exactly one entry from the log */
	ret = do_read_log_to_user(log, reader, buf, ret);

out:
	mutex_unlock(&log->mutex);

	return ret;
}

/*
 * get_next_entry - return the offset of the first valid entry at least 'len'
 * bytes after 'off'.
 *
 * Caller must hold log->mutex.
 */
static size_t get_next_entry(struct belogdrv_log *log, size_t off, size_t len)
{
	size_t count = 0;

	do {
		size_t nr = get_entry_len(log, off);
		off = belogdrv_offset(off + nr);
		count += nr;
	} while (count < len);

	return off;
}

/*
 * clock_interval - is a < c < b in mod-space? Put another way, does the line
 * from a to b cross c?
 */
static inline int clock_interval(size_t a, size_t b, size_t c)
{
	if (b < a) {
		if (a < c || b >= c)
			return 1;
	} else {
		if (a < c && b >= c)
			return 1;
	}

	return 0;
}

/*
 * fix_up_readers - walk the list of all readers and "fix up" any who were
 * lapped by the writer; also do the same for the default "start head".
 * We do this by "pulling forward" the readers and start head to the first
 * entry after the new write head.
 *
 * The caller needs to hold log->mutex.
 */
static void fix_up_readers(struct belogdrv_log *log, size_t len)
{
	size_t old = *(log->w_off);
	size_t new = belogdrv_offset(old + len);
	struct belogdrv_reader *reader;

	if (clock_interval(old, new, *(log->head)))
		*(log->head) = get_next_entry(log, *(log->head), len);

	list_for_each_entry(reader, &log->readers, list)
		if (clock_interval(old, new, reader->r_off))
			reader->r_off = get_next_entry(log, reader->r_off, len);
}

/*
 * do_write_log - writes 'len' bytes from 'buf' to 'log'
 *
 * The caller needs to hold log->mutex.
 */
static void do_write_log(struct belogdrv_log *log, const void *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - *(log->w_off));
	memcpy(log->buffer + *(log->w_off), buf, len);

	if (count != len)
		memcpy(log->buffer, buf + len, count - len);

	*(log->w_off) = belogdrv_offset(*(log->w_off) + count);

}

/*
 * do_write_log_user - writes 'len' bytes from the user-space buffer 'buf' to
 * the log 'log'
 *
 * The caller needs to hold log->mutex.
 *
 * Returns 'count' on success, negative error code on failure.
 */
static ssize_t do_write_log_from_user(struct belogdrv_log *log,
				      const void __user *buf, size_t count)
{
	size_t len;
	unsigned long flags;

	len = min(count, log->size - *(log->w_off));
	if (len && copy_from_user(log->buffer + *(log->w_off), buf, len))
		return -EFAULT;

	if (count != len) {
		if (copy_from_user(log->buffer, buf + len, count - len))
			return -EFAULT;
	}

	if (count == log->size - *(log->w_off)) {
		memcpy(belog_copy_buf, log->buffer, BELOG_BUFFER_SIZE);
		belog_copy_w_off = *(log->w_off);
		belog_copy_head  = *(log->head);
		printk(KERN_ERR "belog: wake_up start\n" );
		spin_lock_irqsave( &belog_kthread_spinlock, flags );
		belog_kthread_flag = BELOG_WAKE_UP_BUFFER_FULL;
		spin_unlock_irqrestore( &belog_kthread_spinlock, flags );
		wake_up( &belog_task_wq );
		printk(KERN_ERR "belog: wake_up end\n" );
	}

	*(log->w_off) = belogdrv_offset(*(log->w_off) + count);

	return count;
}

/*
 * belogdrv_aio_write - our write method, implementing support for write(),
 * writev(), and aio_write(). Writes are our fast path, and we try to optimize
 * them above all else.
 */
ssize_t belogdrv_aio_write(struct kiocb *iocb, const struct iovec *iov,
			 unsigned long nr_segs, loff_t ppos)
{
	struct belogdrv_log *belog = file_get_belog(iocb->ki_filp);
	size_t orig = *(belog->w_off);
	struct belogdrv_entry header;
	struct timespec now;
	ssize_t ret = 0;
        unsigned short hlen;

	now = current_kernel_time();

	header.sec = now.tv_sec;
	header.nsec = now.tv_nsec;
        hlen = min_t(size_t, iocb->ki_left, BELOGDRV_ENTRY_MAX_PAYLOAD);

	mutex_lock(&belog->mutex);

	/*
	 * Fix up any readers, pulling them forward to the first readable
	 * entry after (what will be) the new write offset. We do this now
	 * because if we partially fail, we can end up with clobbered log
	 * entries that encroach on readable buffer.
	 */
	fix_up_readers(belog, sizeof(struct belogdrv_entry) + hlen);

	do_write_log(belog, &header, sizeof(struct belogdrv_entry));

	while (nr_segs-- > 0) {
		size_t len;
		ssize_t nr;

		/* figure out how much of this vector we can keep */
		len = min_t(size_t, iov->iov_len, hlen - ret);

		/* write out this segment's payload */
		nr = do_write_log_from_user(belog, iov->iov_base, len);
		if (unlikely(nr < 0)) {
			*(belog->w_off) = orig;
			mutex_unlock(&belog->mutex);
			return nr;
		}

		iov++;
		ret += nr;
	}

	mutex_unlock(&belog->mutex);

	/* wake up any blocked readers */
	wake_up_interruptible(&belog->wq);

	return ret;
}

static struct belogdrv_log *get_log_from_minor(int);

/*
 * belogdrv_open - the log's open() file operation
 *
 * Note how near a no-op this is in the write-only case. Keep it that way!
 */
static int belogdrv_open(struct inode *inode, struct file *file)
{
	struct belogdrv_log *log;
	int ret;

	ret = nonseekable_open(inode, file);
	if (ret)
		return ret;

	log = get_log_from_minor(MINOR(inode->i_rdev));
	if (!log)
		return -ENODEV;

	if (file->f_mode & FMODE_READ) {
		struct belogdrv_reader *reader;

		reader = kmalloc(sizeof(struct belogdrv_reader), GFP_KERNEL);
		if (!reader)
			return -ENOMEM;

		reader->log = log;
		INIT_LIST_HEAD(&reader->list);

		mutex_lock(&log->mutex);
		reader->r_off = *(log->head);
		list_add_tail(&reader->list, &log->readers);
		mutex_unlock(&log->mutex);

		file->private_data = reader;
	} else
		file->private_data = log;

	return 0;
}

/*
 * belogdrv_release - the log's release file operation
 *
 * Note this is a total no-op in the write-only case. Keep it that way!
 */
static int belogdrv_release(struct inode *ignored, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct belogdrv_reader *reader = file->private_data;
		struct belogdrv_log *log = reader->log;

		mutex_lock(&log->mutex);
		list_del(&reader->list);
		mutex_unlock(&log->mutex);

		kfree(reader);
	}

	return 0;
}

/*
 * belogdrv_poll - the log's poll file operation, for poll/select/epoll
 *
 * Note we always return POLLOUT, because you can always write() to the log.
 * Note also that, strictly speaking, a return value of POLLIN does not
 * guarantee that the log is readable without blocking, as there is a small
 * chance that the writer can lap the reader in the interim between poll()
 * returning and the read() request.
 */
static unsigned int belogdrv_poll(struct file *file, poll_table *wait)
{
	struct belogdrv_reader *reader;
	struct belogdrv_log *log;
	unsigned int ret = POLLOUT | POLLWRNORM;

	if (!(file->f_mode & FMODE_READ))
		return ret;

	reader = file->private_data;
	log = reader->log;

	poll_wait(file, &log->wq, wait);

	mutex_lock(&log->mutex);
	if (*(log->w_off) != reader->r_off)
		ret |= POLLIN | POLLRDNORM;
	mutex_unlock(&log->mutex);

	return ret;
}

static long belogdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct belogdrv_log *belog = file_get_belog(file);
	struct belogdrv_reader *reader;
	long ret = -ENOTTY;

	mutex_lock(&belog->mutex);

	switch (cmd) {
	case BELOGDRV_GET_LOG_BUF_SIZE:
		ret = belog->size;
		break;
	case BELOGDRV_GET_LOG_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (*(belog->w_off) >= reader->r_off)
			ret = *(belog->w_off) - reader->r_off;
		else
			ret = (belog->size - reader->r_off) + *(belog->w_off);
		break;
	case BELOGDRV_GET_NEXT_ENTRY_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (*(belog->w_off) != reader->r_off)
			ret = get_entry_len(belog, reader->r_off);
		else
			ret = 0;
		break;
	case BELOGDRV_FLUSH_LOG:
		if (!(file->f_mode & FMODE_WRITE)) {
			ret = -EBADF;
			break;
		}
		list_for_each_entry(reader, &belog->readers, list)
			reader->r_off = *(belog->w_off);
		*(belog->head) = *(belog->w_off);
		ret = 0;
		break;
	}

	mutex_unlock(&belog->mutex);

	return ret;
}

static const struct file_operations belogdrv_fops = {
	.owner          = THIS_MODULE        ,
	.read           = belogdrv_read      ,
	.aio_write      = belogdrv_aio_write ,
	.poll           = belogdrv_poll      ,
	.unlocked_ioctl = belogdrv_ioctl     ,
	.compat_ioctl   = belogdrv_ioctl     ,
	.open           = belogdrv_open      ,
	.release        = belogdrv_release   ,
};

/*
 * Defines a log structure with name 'NAME' and a size of 'SIZE' bytes, which
 * must be a power of two, greater than BELOGDRV_ENTRY_MAX_LEN, and less than
 * LONG_MAX minus BELOGDRV_ENTRY_MAX_LEN.
 */
#define DEFINE_BELOGDRV_DEVICE(VAR, NAME, SIZE) \
static struct belogdrv_log VAR = { \
	.buffer = NULL, \
	.misc = { \
		.minor = MISC_DYNAMIC_MINOR, \
		.name = NAME, \
		.fops = &belogdrv_fops, \
		.parent = NULL, \
	}, \
	.wq = __WAIT_QUEUE_HEAD_INITIALIZER(VAR .wq), \
	.readers = LIST_HEAD_INIT(VAR .readers), \
	.mutex = __MUTEX_INITIALIZER(VAR .mutex), \
	.size = SIZE, \
};

DEFINE_BELOGDRV_DEVICE(belog_main, BELOGDRV_LOG, BELOG_BUFFER_SIZE)

static struct belogdrv_log *get_log_from_minor(int minor)
{
	if (belog_main.misc.minor == minor)
		return &belog_main;
	return NULL;
}

static int __init init_belog(struct belogdrv_log *belog, unsigned long offset, unsigned long size)
{
	int ret;
	int n,m;

	belogdrv_buffer = ioremap(offset, size);
	if (belogdrv_buffer == NULL) {
		printk(KERN_ERR "belog : failed to map memory\n");
		return -ENOMEM;
	}

        ret = misc_register(&belog->misc);
        if (unlikely(ret)) {
                printk(KERN_ERR "belogdrv: failed to register misc "
                       "device for log '%s'!\n", belog->misc.name);
                return ret;
        }

	if (belogdrv_buffer->sig == BELOG_DRV_SIG) {
		if ((belogdrv_buffer->w_off >= BELOG_BUFFER_SIZE) ||
			(belogdrv_buffer->head >= BELOG_BUFFER_SIZE)){
			printk(KERN_INFO "belogdrv: found existing invalid "
			       "buffer, w_off %d\n",belogdrv_buffer->w_off);
			belogdrv_buffer->w_off = 0;
			belogdrv_buffer->head  = 0;
			memset(belogdrv_buffer, 0x00, BELOG_IOREMAP_BUFFER_SIZE);
		} else {
                        printk(KERN_INFO "belogdrv: found existing buffer, "
			       "w_off %d\n",belogdrv_buffer->w_off);
		}
	} else {
		printk(KERN_INFO "belogdrv: no valid data in buffer "
		       "(sig = 0x%08x)\n", belogdrv_buffer->sig);
		belogdrv_buffer->w_off = 0;
		belogdrv_buffer->head  = 0;
		memset(belogdrv_buffer, 0x00, BELOG_IOREMAP_BUFFER_SIZE);
        }

	n = belogdrv_buffer->w_off;
	m = sizeof(struct belogdrv_entry) + sizeof(struct belog_body);
	if((n % m) != 0) belogdrv_buffer->w_off = (n / m) * m;
	printk(KERN_INFO "belogdrv: w_off=%d n=%d\n",belogdrv_buffer->w_off,n);

	belog->w_off  = &belogdrv_buffer->w_off;
	belog->head   = &belogdrv_buffer->head;

	belog->buffer = belogdrv_buffer->data;
	belogdrv_buffer->sig   = BELOG_DRV_SIG;

	printk(KERN_INFO "belogdrv: created %luK log '%s'\n",
	       (unsigned long) belog->size >> 10, belog->misc.name);

	return 0;
}

#define DBELOG_LOG_FILE_MAX      28
#define DBELOG_LOG_CNT_OFFSET    DBELOG_LOG_FILE_MAX

static void belog_log_create( int size , unsigned char type )
{
	const char * file[ DBELOG_LOG_FILE_MAX + 1 ] =
	{
		"/log4/belog01",
		"/log4/belog02",
		"/log4/belog03",
		"/log4/belog04",
		"/log4/belog05",
		"/log4/belog06",
		"/log4/belog07",
		"/log4/belog08",
		"/log4/belog09",
		"/log4/belog10",
		"/log4/belog11",
		"/log4/belog12",
		"/log4/belog13",
		"/log4/belog14",
		"/log4/belog15",
		"/log4/belog16",
		"/log4/belog17",
		"/log4/belog18",
		"/log4/belog19",
		"/log4/belog20",
		"/log4/belog21",
		"/log4/belog22",
		"/log4/belog23",
		"/log4/belog24",
		"/log4/belog25",
		"/log4/belog26",
		"/log4/belog27",
		"/log4/belog28",
		"/log4/count"
	};

	const char * backupfile[ 3 ] =
	{
		"/log4/belog.backup",
		"/log4/head",
		"/log4/w_off"
	};

	signed long          manage;
	signed long          dat;
	signed long          os_ret;
	char                 buf[ 3 ];
	char                 back[ 7 ];
	unsigned int         log_num;
	unsigned int         x, y;

	printk(KERN_ERR "belog : belog_log_create start!\n");

	/* 内部変数の初期化 */
	log_num  =  0;
	buf[ 0 ] = '0';
	buf[ 1 ] = '0';
	buf[ 2 ] = '\n';

	back[ 0 ] = '0';
	back[ 1 ] = '0';
	back[ 2 ] = '0';
	back[ 3 ] = '0';
	back[ 4 ] = '0';
	back[ 5 ] = '0';
	back[ 6 ] = '\n';

	if( type == BELOG_WAKE_UP_BUFFER_FULL ) { // バッファがフルサイズとなった
		/* 管理ファイルのオープン */
		manage = sys_open( file[ DBELOG_LOG_CNT_OFFSET ], ( O_RDWR | O_CREAT ), 0777 );
		if( manage < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_open %s\n",file[ DBELOG_LOG_CNT_OFFSET ]);
			return;
		}

		/* 管理ファイルのリード */
		os_ret = sys_read( manage, buf, 2 );
		if( os_ret > 0 ) {
			x = buf[ 0 ] - '0';
			y = buf[ 1 ] - '0';
			log_num = ( x * 10 ) + y;

			/* 値のチェック */
			if( log_num >= DBELOG_LOG_FILE_MAX )
			{
				log_num = 0;
			}
		} else if( os_ret == 0 ) {
			log_num = 0;
		} else {
			printk(KERN_ERR "belog : belog_log_create error sys_read %s\n",file[ DBELOG_LOG_CNT_OFFSET ]);
			sys_close( manage );
			return;
		}

		/* データファイルのオープン */
		dat = sys_open( file[ log_num ], ( O_RDWR | O_CREAT ), 0777 );
		if( dat < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_open %s\n",file[ log_num ]);
			sys_close( manage );
			return;
		}

		/* データファイル先頭へシーク */
		os_ret = sys_lseek( dat, 0, SEEK_SET );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_lseek %s\n",file[ log_num ]);
			sys_close( dat );
			sys_close( manage );
			return;
		}

		/* データファイルへライト */
		os_ret = sys_write( dat, ( char * )belog_copy_buf, size );
		if( os_ret != size ) {
			printk(KERN_ERR "belog : belog_log_create error sys_write %s\n",file[ log_num ]);
			sys_close( dat );
			sys_close( manage );
			return;
		}

		/* 管理ファイル先頭へシーク */
		os_ret = sys_lseek( manage, 0, SEEK_SET );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_lseek %s\n",file[ DBELOG_LOG_CNT_OFFSET ]);
			sys_close( dat );
			sys_close( manage );
			return;
		}

		/* ファイル番号のカウントアップ */
		log_num++;
		if( log_num >= DBELOG_LOG_FILE_MAX ) {
			log_num = 0;
		}
		x = ( log_num / 10 ) + '0';
		y = ( log_num % 10 ) + '0';
		buf[ 0 ] = x;
		buf[ 1 ] = y;
		buf[ 2 ] ='\n';

		/* 管理ファイルへライト */
		os_ret = sys_write( manage, buf, sizeof( buf ));
		if( os_ret != sizeof( buf )) {
			printk(KERN_ERR "belog : belog_log_create error sys_write %s\n",file[ DBELOG_LOG_CNT_OFFSET ]);
			sys_close( dat );
			sys_close( manage );
			return;
		}

		/* データファイルのクローズ */
		os_ret = sys_close( dat );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_close data file.\n");
			sys_close( manage );
			return;
		}

		/* 管理ファイルのクローズ */
		os_ret = sys_close( manage );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_close %s\n",file[ DBELOG_LOG_CNT_OFFSET ]);
			return;
		}

	} else if( type == BELOG_WAKE_UP_SHUTDOWN ) {
		/* データバックアップファイルのオープン */
		dat = sys_open( backupfile[ log_num ], ( O_RDWR | O_CREAT ), 0777 );
		if( dat < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_open %s\n",backupfile[ log_num ]);
			return;
		}

		/* データバックアップファイル先頭へシーク */
		os_ret = sys_lseek( dat, 0, SEEK_SET );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_lseek %s\n",backupfile[ log_num ]);
			sys_close( dat );
			return;
		}

		/* データバックアップファイルへライト */
		os_ret = sys_write( dat, ( char * )belog_copy_buf, size );
		if( os_ret != size ) {
			printk(KERN_ERR "belog : belog_log_create error sys_write %s\n",backupfile[ log_num ]);
			sys_close( dat );
			return;
		}

		/* データバックアップファイルのクローズ */
		os_ret = sys_close( dat );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_close %s\n",backupfile[ log_num ]);
			return;
		}

		/* head バックアップファイルのオープン */
		log_num++;
		dat = sys_open( backupfile[ log_num ], ( O_RDWR | O_CREAT ), 0777 );
		if( dat < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_open %s\n",backupfile[ log_num ]);
			return;
		}

		/* head バックアップファイル先頭へシーク */
		os_ret = sys_lseek( dat, 0, SEEK_SET );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_lseek %s\n",backupfile[ log_num ]);
			sys_close( dat );
			return;
		}

		/* head を文字列変換 */
		printk(KERN_ERR "belog : belog_log_create head=%d\n",belog_copy_head);
		if( belog_copy_head >= BELOG_BUFFER_SIZE ) {
			belog_copy_head = 0;
		}
		os_ret = snprintf(back, sizeof( back ),"%06d\n",belog_copy_head);
		back[ 6 ] ='\n';
		if( os_ret != sizeof( back )) {
			printk(KERN_ERR "belog : belog_log_create error snprintf %s\n",backupfile[ log_num ]);
			sys_close( dat );
			return;
		}

		/* head データバックアップファイルへライト */
		os_ret = sys_write( dat, ( char * )back, sizeof( back ) );
		if( os_ret !=  sizeof( back )) {
			printk(KERN_ERR "belog : belog_log_create error sys_write %s %s\n",backupfile[ log_num ],back);
			sys_close( dat );
			return;
		}
		/* head データバックアップファイルのクローズ */
		os_ret = sys_close( dat );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_close %s\n",backupfile[ log_num ]);
			return;
		}

		/* w_off バックアップファイルのオープン */
		log_num++;
		dat = sys_open( backupfile[ log_num ], ( O_RDWR | O_CREAT ), 0777 );
		if( dat < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_open %s\n",backupfile[ log_num ]);
			return;
		}

		/* w_off バックアップファイル先頭へシーク */
		os_ret = sys_lseek( dat, 0, SEEK_SET );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_lseek %s\n",backupfile[ log_num ]);
			sys_close( dat );
			return;
		}

		/* w_off を文字列変換 */
		printk(KERN_ERR "belog : belog_log_create w_off=%d\n",belog_copy_w_off);
		if( belog_copy_w_off >= BELOG_BUFFER_SIZE ) {
			belog_copy_w_off = 0;
		}
		os_ret = snprintf(back, sizeof( back ),"%06d\n",belog_copy_w_off);
		back[ 6 ] ='\n';
		if( os_ret != sizeof( back )) {
			printk(KERN_ERR "belog : belog_log_create error snprintf %s\n",backupfile[ log_num ]);
			sys_close( dat );
			return;
		}

		/* w_off データバックアップファイルへライト */
		os_ret = sys_write( dat, ( char * )back, sizeof( back ) );
		if( os_ret != sizeof( back )) {
			printk(KERN_ERR "belog : belog_log_create error sys_write %s %s\n",backupfile[ log_num ],back);
			sys_close( dat );
			return;
		}
		/* w_off データバックアップファイルのクローズ */
		os_ret = sys_close( dat );
		if( os_ret < 0 ) {
			printk(KERN_ERR "belog : belog_log_create error sys_close %s\n",backupfile[ log_num ]);
			return;
		}

	}

	printk(KERN_ERR "belog : belog_log_create end!\n");

	return;
}

void create_belogfile( void ){
	unsigned long flags;

	memcpy(belog_copy_buf, belogdrv_buffer->data, BELOG_BUFFER_SIZE);
	belog_copy_w_off = belogdrv_buffer->w_off;
	belog_copy_head  = belogdrv_buffer->head;

        spin_lock_irqsave( &belog_kthread_spinlock, flags );
        belog_kthread_flag = BELOG_WAKE_UP_SHUTDOWN;
        spin_unlock_irqrestore( &belog_kthread_spinlock, flags );

        wake_up( &belog_task_wq );
}

int belog_kthread_main( void* arg )
{
	mm_segment_t           oldfs;
	struct sched_param     param;
	unsigned long          flags;
	unsigned char          type;
	int                    size;

	printk(KERN_ERR "kanpand : belog_kthread_main start\n");

	param.sched_priority = 0;
	type                 = 0;
	size                 = BELOG_BUFFER_SIZE;

        daemonize( "kanpand" );
	oldfs = get_fs();
	set_fs( KERNEL_DS );
	sched_setscheduler( current,
                        SCHED_NORMAL,
                        &param );
	set_user_nice( current, -3 );
	set_fs( oldfs );

	while( 1 )
	{
		wait_event( belog_task_wq, ( belog_kthread_flag != 0 ));

                spin_lock_irqsave( &belog_kthread_spinlock, flags );
                type = ( unsigned char )belog_kthread_flag;
		belog_kthread_flag = BELOG_WAIT_EVENTS;
                spin_unlock_irqrestore( &belog_kthread_spinlock, flags );

		printk(KERN_ERR "kanpand : belog_log_create type=0x%d\n",type);
		belog_log_create( size , type );
	}

	printk(KERN_ERR "kanpand : belog_kthread_main end\n");
	return 0;
}
static int __init belogdrv_init(void)
{
	int ret;
	struct task_struct *belog_kthread_id;

	ret = init_belog(&belog_main, DMEM_BELOG_APL_HIST_START, BELOG_IOREMAP_BUFFER_SIZE);

	init_waitqueue_head( &belog_task_wq );
	belog_kthread_id = kthread_create( belog_kthread_main, NULL, "anpan_kthread" );
	if( IS_ERR(belog_kthread_id) )
	{
		printk(KERN_ERR "belog : kthread create error!!\n" );
                return -EIO;
        }
	wake_up_process( belog_kthread_id );

	belog_copy_buf = kmalloc(BELOG_BUFFER_SIZE, GFP_KERNEL);
	if (belog_copy_buf == NULL) {
		printk(KERN_ERR "belog : failed to allocate buffer!!\n");
		return -1;
	}

	if (unlikely(ret))
		goto out;

out:
	return ret;
}
device_initcall(belogdrv_init);
