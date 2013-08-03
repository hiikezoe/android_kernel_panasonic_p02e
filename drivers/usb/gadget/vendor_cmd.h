#ifndef __F_VENDOR_CMD_H_
#define __F_VENDOR_CMD_H_

#include "vendor_ioctl.h"

#define MASS_ACTN_START			0
#define MASS_ACTN_CONTINUE		1
#define MASS_ACTN_COMPLETE		2
#define MASS_ACTN_ABORT			3

struct fsg_common;
struct fsg_dev;

enum vendor_phase {
	VDR_PHASE_IDLE = 0,
	VDR_PHASE_COMMAND,
	VDR_PHASE_NO_DATA,
	VDR_PHASE_DATA_IN,
	VDR_PHASE_DATA_OUT,
	VDR_PHASE_DATA_WAIT,
	VDR_PHASE_STATUS,
	VDR_PHASE_UNKNOWN,
	VDR_PHASE_ERROR,
	VDR_PHASE_MAX
};

struct fsg_common_vendor {
	char                media_path[256];
	u8                  vendor_specific[12];
	unsigned char       vdr_dat_complete;
	enum vendor_phase   vdr_phase;
	int                 vdr_total_recv_size;
	u32                 usr_buf_size;
};

static int do_vendor_cmd(struct fsg_common *common);
static int do_ioctl_unknown_command( struct fsg_common* common );
static int do_ioctl_no_data( struct fsg_common* common , struct no_data* nodata );
static int do_ioctl_receive_data( struct fsg_common* common , struct receive_data* recv_dat );
static int do_ioctl_send_data( struct fsg_common* common , struct send_data* send_dat );
static int do_ioctl_set_outbuff_status( struct fsg_common* common , struct set_outbuff_status* outbuf );
static int do_ioctl_get_infbuff_status( struct fsg_common* common , struct get_inbuff_status* inbuf );
static int do_ioctl_receive_command( struct fsg_common* common , struct receive_command* cmd );
static void do_ioctl_err( struct fsg_common* common );

static long massdev_cmd_ioctl( struct file* filp, unsigned int cmd, unsigned long arg );
static long massdev_info_ioctl( struct file* filp, unsigned int cmd, unsigned long arg );

static void notify_change_path(char* buf);

#endif /* __F_VENDOR_CMD_H_ */
