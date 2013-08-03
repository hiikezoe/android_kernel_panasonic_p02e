#ifndef __F_MASSDEV_H_
#define __F_MASSDEV_H_

#include <linux/ioctl.h>

/* device RWmode type */
#define RWMODE_NONE    0
#define RWMODE_READ    1
#define RWMODE_WRITE   2
#define RWMODE_ERROR   4
#define RWMODE_TERM    8

enum mass_dev_minor {
    DEV_MINOR_COMMAND,
    DEV_MINOR_INFO,
    DEV_MINOR_MAX
};

struct mass_device {
	int                 minor;
	int                 rwmode;
	wait_queue_head_t   read_wait;
	wait_queue_head_t   write_wait;
	spinlock_t          lock;
};

#endif /* __F_MASSDEV_H_ */
