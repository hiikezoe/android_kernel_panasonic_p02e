/*
 * drivers/fgc/pfgc_sysfs.c
 *
 * Copyright(C) 2010 Panasonic Mobile Communications Co., Ltd.
 * Author: Naoto Suzuki <suzuki.naoto@kk.jp.panasonic.com>
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

#include "inc/pfgc_local.h"
#define	CONFIG_FGIC_MEASUREMENT			/*  */
#ifdef	CONFIG_FGIC_MEASUREMENT
#include "linux/rtc.h"	
#include "linux/time.h"	
#include "linux/kobject.h"
#include "linux/fs.h"
#include "linux/sysfs.h"
#include "linux/stat.h"
#endif /* CONFIG_FGIC_MEASUREMENT */

static struct platform_device *bat_pdev;

static enum power_supply_property fgc_props[] = {
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
// Conan        POWER_SUPPLY_PROP_CAPACITY
        POWER_SUPPLY_PROP_CAPACITY,                      // Conan
        POWER_SUPPLY_PROP_DEGRADATION_STATUS,            // Conan
        POWER_SUPPLY_PROP_DEGRADATION_DECISION,          // Conan
	    POWER_SUPPLY_PROP_DEGRADATION_MEASUREMENT,       // Conan 1201XX
        POWER_SUPPLY_PROP_DEGRADATION_ESTIMATION,        // Conan
        POWER_SUPPLY_PROP_DEGRADATION_CYCLE,             // Conan
        POWER_SUPPLY_PROP_DEGRADATION_STORE,             // Conan
        POWER_SUPPLY_PROP_DEGRADATION_TIMESTAMP_HIGHBIT, // Conan
        POWER_SUPPLY_PROP_DEGRADATION_TIMESTAMP_LOWBIT   // Conan
#else /*<Conan>1111XX*/
        POWER_SUPPLY_PROP_CAPACITY
#endif /*<Conan>1111XX*/
};

#ifdef	CONFIG_FGIC_MEASUREMENT
typedef struct	{
	__kernel_time_t	 	tSec;
	__kernel_suseconds_t	tUsec;
	int			mAmpere;
}	PfgcLogItem;
#define	PFGC_LOCAL_TIME_SEC	(9 * 60 * 60)
#define	PFGC_LOG_MAX		30000
static	PfgcLogItem	pfgcLogArea[PFGC_LOG_MAX];
static	int		pfgcLogPos;
static DEFINE_SPINLOCK(pfgcLogLock);
#endif /* CONFIG_FGIC_MEASUREMENT */


/*!
 Get capacity value.
 @return  0       success
          -EINVAL param error
 @param[in] psy power supply info (unused)
 @param[in] psp property kind
 @param[out] val power supply property value
 */
int pfgc_get_property( struct power_supply *psy,				/* mrc24602 */
                       enum power_supply_property psp,
                       union power_supply_propval *val )
{
	int ret = 0;
/* IDPower	dev_dbg(&bat_pdev->dev, "%s: psp=%d\n", __func__, psp);			*//* mrc23802 *//* mrc24004 *//* mrc31501 *//* mrc50801 */

	if (val == NULL) {
		return -EINVAL;
	}

	switch (psp) {
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = gfgc_ctl.get_capacity();
			gfgc_ctl.capacity_old = val->intval;
			FGC_BATTERY("intval=%d\n",val->intval);
			break;
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
		// Conan Add Start
		case POWER_SUPPLY_PROP_DEGRADATION_STATUS:
		case POWER_SUPPLY_PROP_DEGRADATION_DECISION:
		case POWER_SUPPLY_PROP_DEGRADATION_MEASUREMENT: // 1201XX
		case POWER_SUPPLY_PROP_DEGRADATION_ESTIMATION:
		case POWER_SUPPLY_PROP_DEGRADATION_CYCLE:
		case POWER_SUPPLY_PROP_DEGRADATION_STORE:
		case POWER_SUPPLY_PROP_DEGRADATION_TIMESTAMP_HIGHBIT:
		case POWER_SUPPLY_PROP_DEGRADATION_TIMESTAMP_LOWBIT:
			val->intval = gfgc_ctl.get_degradation_info( psp );
			FGC_BATTERY("deg_info=%d\n",val->intval);
			break;
		// Conan Add end
#endif /*<Conan>1111XX*/
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static struct power_supply power_supply_bat = {					/* mrc22102 *//* mrc24301 */
        .properties = fgc_props,
	.num_properties = ARRAY_SIZE(fgc_props),				/* mrc70301 *//* mrc10121 */
	.get_property = pfgc_get_property,
/* IDPower #13       .name = "battery", */
	.name = "fgcbattery", /* IDPower #13 */
	.type = POWER_SUPPLY_TYPE_BATTERY,
};

#ifdef	CONFIG_FGIC_MEASUREMENT
static void	pfgc_log_init(void)
{
	unsigned long		flags;

        spin_lock_irqsave(&pfgcLogLock, flags);
	memset((void*)&pfgcLogArea[0], 0, sizeof(PfgcLogItem) * PFGC_LOG_MAX);
	pfgcLogPos = 0;
        spin_unlock_irqrestore(&pfgcLogLock, flags);
}

static void	pfgc_log_add(void)
{
	int			nextPos;
	signed short		ampare;		/* for nemo */
	unsigned long		flags;
	struct timeval		tv;
	int			rc;

	do_gettimeofday(&tv);
	rc = pfgc_fgc_mstate(D_FGC_CURRENT_REQ, &ampare);
	if (rc != 0)			return;
        spin_lock_irqsave(&pfgcLogLock, flags);
	if (pfgcLogArea[pfgcLogPos].tSec == 0) {
	    nextPos = pfgcLogPos;
	} else {
//	    if (pfgcLogArea[pfgcLogPos].tSec == tv.tv_sec) {
//		spin_unlock_irqrestore(&pfgcLogLock, flags);
//		return;
//	    }
	    nextPos = (pfgcLogPos >= (PFGC_LOG_MAX - 1)) ? 0 : pfgcLogPos + 1 ; 
	}
	pfgcLogArea[nextPos].tSec    = tv.tv_sec;
	pfgcLogArea[nextPos].tUsec   = tv.tv_usec;
	pfgcLogArea[nextPos].mAmpere = -ampare;
	pfgcLogPos = nextPos;
        spin_unlock_irqrestore(&pfgcLogLock, flags);
}
int	lastPos;

static void *	pfgc_log_getArea(
int	base,
loff_t	pos
) {
	int	curPos;

	if ((pos >= PFGC_LOG_MAX) || (pos < -PFGC_LOG_MAX))		return NULL;
	curPos = base + pos;
	if (curPos < 0) {
	    curPos += PFGC_LOG_MAX;
	} else if (curPos >= PFGC_LOG_MAX) {
	    curPos -= PFGC_LOG_MAX;
	}
	lastPos = curPos;

	return((void*)&pfgcLogArea[curPos]);
}

static void *	pfgc_pic_info_start(struct seq_file *m, loff_t *pos)
{
	PfgcLogItem *	pW;
	unsigned long	flags;

        spin_lock_irqsave(&pfgcLogLock, flags);
        if ((pfgcLogPos >= 0) && (pfgcLogPos < (PFGC_LOG_MAX - 1))) {
            m->private = (void*)(pfgcLogPos + 1);
	    if (pfgcLogArea[(int)(m->private)].tSec == 0) {
		m->private = 0;
	    }
        } else if (pfgcLogPos == (PFGC_LOG_MAX - 1)) {
            m->private = 0;
	} else {
            printk("DEBUGLOG : illegal position[%d] \n", pfgcLogPos);
            m->private = 0;
        }
        spin_unlock_irqrestore(&pfgcLogLock, flags);
        pW =  (PfgcLogItem*)(pfgc_log_getArea((int)(m->private), *pos));
        return (pW);
}


static void *   pfgc_pic_info_next(struct seq_file *m, void *arg, loff_t *pos)
{
PfgcLogItem *  pW;

	do {
	    (*pos)++;
	    pW = pfgc_log_getArea((int)(m->private), *pos);
	} while((pW != NULL) && (pW->tSec == 0));
	return(pW);
}

static int pfgc_pic_show(struct seq_file *m, void *arg)
{
	PfgcLogItem *  pItem = (PfgcLogItem*)arg;
	struct rtc_time		tm;
	unsigned int		msec;

	if (pItem->tSec == 0) {
//	    seq_printf(m, "\n");
	} else {
	    rtc_time_to_tm(pItem->tSec + PFGC_LOCAL_TIME_SEC, &tm);
	    msec = pItem->tUsec / 1000;
	    if (msec > 1000)		msec = 0;
            seq_printf(m, "%04d%02d%02d%02d%02d%02d%03d, %d\n",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, msec, 
		pItem->mAmpere);
	}
        return 0;
}

static void pfgc_pic_stop(struct seq_file *m, void *arg)
{
        m->private = NULL;
}

static const struct seq_operations pfgc_pic_info_op = {
	.start  = pfgc_pic_info_start,
	.next   = pfgc_pic_info_next,
	.stop   = pfgc_pic_stop,
	.show   = pfgc_pic_show,
};

static int pfgc_pic_info_open(struct inode *inode, struct file *file)
{
        return seq_open(file, &pfgc_pic_info_op);
}

static ssize_t pfgc_pic_write(
	struct file *           file,
	const char __user *     buffer,
	size_t                  count,
	loff_t *                pos
) {
	char    from[256];

	if (count >= 256) {
	    printk("%s[%d] pos[%d]\n", buffer, (int)count, (int)(*pos));
        } else {
            memcpy(&from[0], buffer, (int)count);
	    from[count] = '\0';
	    if (!strcmp(&from[0], "clean")) {
		pfgc_log_init();
		printk("pfgc_pic_write clean info table of ampere log\n");
	    } else {
		printk("pfgc_pic_write(%08x, %s, %d, %08x[%d]) is not supported\n", (int)file, buffer, count, (int)pos, (int)*pos);
	    }
	}
	return(count);
}


static const struct file_operations proc_pfgc_pic_info_file_operations = {
        .open		= pfgc_pic_info_open,
        .read		= seq_read,
        .llseek		= seq_lseek,
        .release	= seq_release,
        .write		= pfgc_pic_write,
};

static void *	pfgc_pic_list_start(struct seq_file *m, loff_t *pos)
{
	PfgcLogItem *	pW;

        if ((pfgcLogPos >= 0) && (pfgcLogPos < PFGC_LOG_MAX)) {
            m->private = (void*)pfgcLogPos;
        } else {
            printk("DEBUGLOG : illegal position[%d] \n", pfgcLogPos);
            m->private = 0;
        }
        pW =  (PfgcLogItem*)(pfgc_log_getArea((int)(m->private), *pos));
        return (pW);
}

static void *   pfgc_pic_list_next(struct seq_file *m, void *arg, loff_t *pos)
{
PfgcLogItem *  pW;

	do {
	    (*pos)++;
	    pW = pfgc_log_getArea((int)(m->private), 1 - *pos);
	} while((pW != NULL) && (pW->tSec == 0));
	return(pW);
}

static const struct seq_operations pfgc_pic_list_op = {
	.start  = pfgc_pic_list_start,
	.next   = pfgc_pic_list_next,
	.stop   = pfgc_pic_stop,
	.show   = pfgc_pic_show,
};

static int pfgc_pic_list_open(struct inode *inode, struct file *file)
{
        return seq_open(file, &pfgc_pic_list_op);
}

static const struct file_operations proc_pfgc_pic_list_file_operations = {
        .open           = pfgc_pic_list_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = seq_release,
};

static ssize_t pfgc_pic_show_current(
    struct kobject *		pKobj,
    struct kobj_attribute *	pAttr,
    char *			pBuf
) {
	struct rtc_time		tm;
	struct timeval		tv;
	signed short		ampare;		/* for nemo */
	int			rc;

	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec + PFGC_LOCAL_TIME_SEC, &tm);
	rc = pfgc_fgc_mstate(D_FGC_CURRENT_REQ, &ampare);

	if (rc == 0) {
            return sprintf(pBuf, "%04d/%02d/%02d %02d:%02d:%02d.%06d, %d\n",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec, -ampare);
	}
	return 0;
}

static ssize_t pfgc_pic_show_info(
    struct kobject *		pKobj,
    struct kobj_attribute *	pAttr,
    char *			pBuf
) {
	struct rtc_time		tm;
	struct timeval		tv;
	signed short		ampare;		/* for nemo */
	int			msec;
	int			rc;

	do_gettimeofday(&tv);
	rc = pfgc_fgc_mstate(D_FGC_CURRENT_REQ, &ampare);

	if (rc == 0) {
	    rtc_time_to_tm(tv.tv_sec + PFGC_LOCAL_TIME_SEC, &tm);
	    msec = tv.tv_usec / 1000;
	    if (msec > 1000)		msec = 0;
            return sprintf(pBuf, "%04d%02d%02d%02d%02d%02d%03d, %d\n",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, msec, -ampare);
	}
	return 0;
}

static ssize_t pfgc_pic_show_touch(
    struct kobject *		pKobj,
    struct kobj_attribute *	pAttr,
    char *			pBuf
) {
	
	pfgc_log_add();
	return pfgc_pic_show_current(pKobj, pAttr, pBuf);
}

static ssize_t pfgc_pic_show_capacity(
    struct kobject *		pKobj,
    struct kobj_attribute *	pAttr,
    char *			pBuf
) {
	int	capacity;
	int	rc;
	
	rc = pfgc_fgc_mstate(D_FGC_CAPACITY_REQ, &capacity);

	return sprintf(pBuf, "capacity = %d\n", capacity);
}

static struct kobj_attribute	pfgc_pic_prop_attr_current	= __ATTR(curr, S_IRUGO, pfgc_pic_show_current, NULL);
static struct kobj_attribute	pfgc_pic_prop_attr_info		= __ATTR(info1,S_IRUGO, pfgc_pic_show_info, NULL);
static struct kobj_attribute	pfgc_pic_prop_attr_touch	= __ATTR(touch,S_IRUGO, pfgc_pic_show_touch, NULL);
static struct kobj_attribute	pfgc_pic_prop_attr_capacity	= __ATTR(capa ,S_IRUGO, pfgc_pic_show_capacity, NULL);
static struct attribute *	pfgc_pic_prop_attrs[] = {
	&pfgc_pic_prop_attr_info.attr,
	&pfgc_pic_prop_attr_current.attr,
	&pfgc_pic_prop_attr_touch.attr,
	&pfgc_pic_prop_attr_capacity.attr,
	NULL
};

static struct attribute_group	pfgc_pic_prop_attr_group = {
        .attrs = pfgc_pic_prop_attrs,
};

void	pfgc_pic_add(void)
{
	struct kobject *	pKobj_pic;
	struct proc_dir_entry *	pDir_pic;
	int			err;

	pKobj_pic = kobject_create_and_add("pic", NULL);
	if (pKobj_pic) {
	    err = sysfs_create_group(pKobj_pic, &pfgc_pic_prop_attr_group);
	    if (err != 0)
		printk("sysfs_create_group(%p, %p) is error[%d]\n", pKobj_pic, &pfgc_pic_prop_attr_group, err);
	}
        pDir_pic = proc_mkdir("pic", NULL);
	if (pDir_pic) {
	    proc_create("info2", S_IRUGO , pDir_pic, &proc_pfgc_pic_info_file_operations);
	    proc_create("list", S_IRUGO , pDir_pic, &proc_pfgc_pic_list_file_operations);
	}
}
#endif /* CONFIG_FGIC_MEASUREMENT */


/*!
 Register sysfs device.
 @return  0      success
          other  unsuccess
 @param   Nothing
 */
int pfgc_class_register( void )
{
	int ret = 0;

	bat_pdev = platform_device_register_simple(DFGC_DEV, 0, NULL, 0);
	if (IS_ERR(bat_pdev)) {
		FGC_ERR("platform_device_register_simple() failed.\n");
		return PTR_ERR(bat_pdev);
	}

	ret = power_supply_register(&bat_pdev->dev, &power_supply_bat);
	if (ret != 0) {
		FGC_ERR("power_supply_register() failed. ret=%d\n", ret);
    		platform_device_unregister(bat_pdev);
		return ret;
	}
#ifdef	CONFIG_FGIC_MEASUREMENT
	pfgc_log_init();
	pfgc_pic_add();
#endif /* CONFIG_FGIC_MEASUREMENT */

	return ret;
}

/*!
 Unregister sysfs device.
 @return  Nothing
 @param   Nothing
 */
void pfgc_class_unregister( void )
{
	power_supply_unregister(&power_supply_bat);
	platform_device_unregister(bat_pdev);
}

/*!
 notify capacity changed.
 @return  Nothing
 @param   Nothing
 */
void pfgc_uevent_send( void )
{
	power_supply_changed(&power_supply_bat);
}

