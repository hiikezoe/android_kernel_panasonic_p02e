#include <linux/poll.h>

#include "massdev.h"
#include "vendor_ioctl.h"
#include "vendor_cmd.h"
#include "debug_log.h"


static struct mass_device mass_dev_info[] =
{
	{
		.minor  = DEV_MINOR_COMMAND,
		.rwmode = RWMODE_NONE,
	},
	{
		.minor  = DEV_MINOR_INFO,
		.rwmode = RWMODE_NONE,
	}
};

static long          massdev_cmd_ioctl( struct file* filp, unsigned int cmd, unsigned long arg );
static unsigned int massdev_cmd_poll( struct file* filp, poll_table *wait );
static int          massdev_cmd_open(struct inode *ip, struct file *fp);
static int          massdev_cmd_release(struct inode *ip, struct file *fp);
static long          massdev_info_ioctl( struct file* filp, unsigned int cmd, unsigned long arg );
static unsigned int massdev_info_poll( struct file* filp, poll_table *wait );
static int          massdev_info_open(struct inode *ip, struct file *fp);
static int          massdev_info_release(struct inode *ip, struct file *fp);

static struct file_operations massdev_cmd_fops = {
    .owner   = THIS_MODULE,
    .unlocked_ioctl   = massdev_cmd_ioctl,
    .poll    = massdev_cmd_poll,
    .open    = massdev_cmd_open,
    .release = massdev_cmd_release,
};

static struct file_operations massdev_info_fops = {
    .owner   = THIS_MODULE,
    .unlocked_ioctl   = massdev_info_ioctl,
    .poll    = massdev_info_poll,
    .open    = massdev_info_open,
    .release = massdev_info_release,
};

static struct miscdevice massdev_cmd_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "mass_cmd",
    .fops  = &massdev_cmd_fops,
};

static struct miscdevice massdev_info_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "mass_info",
    .fops  = &massdev_info_fops,
};


static unsigned int massdev_cmd_poll( struct file* filp, poll_table *wait )
{

	unsigned int        mask = 0;
	struct mass_device* dev = (struct mass_device*)filp->private_data;
	unsigned long       flags;
	
	FUNC_IN();
	CPRM_LOGOUT("filp=%p, dev=%p, &dev->read_wait=%p, &dev->write_wait=%p, wait=%p\n",filp, dev, &dev->read_wait,&dev->write_wait, wait);
	poll_wait(filp, &dev->read_wait, wait);
	poll_wait(filp, &dev->write_wait, wait);

	spin_lock_irqsave(&dev->lock, flags);

	do {
		if( dev->rwmode & RWMODE_ERROR ) {
			mask = POLLERR;
			break;
		}
		if( dev->rwmode & RWMODE_READ ) {
			mask |= POLLIN | POLLRDNORM;
		}
		if( dev->rwmode & RWMODE_WRITE ) {
			mask |= POLLOUT | POLLWRNORM;
		}
	} while(0);

	dev->rwmode = RWMODE_NONE;
	spin_unlock_irqrestore(&dev->lock, flags);
	
	FUNC_OUT();
	return mask;
}

static unsigned int massdev_info_poll( struct file* filp, poll_table *wait )
{
	unsigned int        mask = 0;
	struct mass_device* dev = (struct mass_device*)filp->private_data;
	unsigned long       flags;

	FUNC_IN();
	poll_wait(filp, &dev->read_wait, wait);
	poll_wait(filp, &dev->write_wait, wait);

	spin_lock_irqsave(&dev->lock, flags);

	do {
		if( dev->rwmode & RWMODE_TERM ) {
			mask = 0x1234;
			break;
		}
		if( dev->rwmode & RWMODE_ERROR ) {
			mask = POLLERR;
			break;
		}
		if( dev->rwmode & RWMODE_READ ) {
			mask |= POLLIN | POLLRDNORM;
		}
		if( dev->rwmode & RWMODE_WRITE ) {
			mask |= POLLOUT | POLLWRNORM;
		}
	} while(0);

	dev->rwmode = RWMODE_NONE;
	spin_unlock_irqrestore(&dev->lock, flags);

	FUNC_OUT();
	return mask;
}

static int massdev_cmd_open(struct inode *ip, struct file *fp)
{
	FUNC_IN();
	fp->private_data = (void*)&mass_dev_info[DEV_MINOR_COMMAND];
	FUNC_OUT();
	return 0;
}

static int massdev_info_open(struct inode *ip, struct file *fp)
{
	FUNC_IN();
	fp->private_data = (void*)&mass_dev_info[DEV_MINOR_INFO];
	FUNC_OUT();
	return 0;
}

static int massdev_cmd_release(struct inode *ip, struct file *fp)
{
	return 0;
}

static int massdev_info_release(struct inode *ip, struct file *fp)
{
	return 0;
}

