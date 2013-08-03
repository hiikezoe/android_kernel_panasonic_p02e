#ifndef __F_VENDOR_IOCTL_H_
#define __F_VENDOR_IOCTL_H_

#include <linux/ioctl.h>

#define SCSI_COMMAND_SIZE 		16

struct csw_status
{
	unsigned int  residue;
	unsigned long status;
};

union sense_data{
	unsigned long sd;
	struct
	{
		unsigned char ascq;
		unsigned char asc;
		unsigned char sense_key;
		unsigned char reserved;
	}sc;
};


struct receive_command
{
	unsigned long signature;
	unsigned long tag;
	unsigned long transfer_length;
	unsigned char flags;
	unsigned char lun;
	unsigned char length;
	unsigned char cmd[SCSI_COMMAND_SIZE];
};

struct get_inbuff_status
{
	char*			buff;
	int			size;
	unsigned char		access;
};

struct set_outbuff_status
{
	int			recv_size;
	unsigned int		action;
	unsigned char		access;
	struct csw_status	csw;
	union sense_data	sense;
};

struct send_data
{
	char*			buff;
	int			size;
	unsigned int		action;
	struct csw_status	csw;
	union sense_data	sense;
};

struct receive_data
{
	char*			buff;
	int			size;
};

struct no_data
{
	struct csw_status	csw;
	union sense_data	sense;
};

union mass_ioctl
{
	struct receive_command		recv_cmd;
	struct get_inbuff_status  	get_ibuf;
	struct set_outbuff_status 	set_obuf;
	struct send_data          	send_dat;
	struct receive_data       	recv_dat;
	struct no_data            	no_dat;
};

struct init_info
{
	unsigned char		inquiry[12];
	int			buff_size;
	int			actual_size;
};


#define MASSDRV_IOC_MAGIC      'M'

#define MASSDRV_IOCTL_RECEIVE_COMMAND              _IO(MASSDRV_IOC_MAGIC, 1)
#define MASSDRV_IOCTL_GET_INBUFF_STATUS            _IO(MASSDRV_IOC_MAGIC, 2)
#define MASSDRV_IOCTL_SET_OUTBUFF_STATUS           _IO(MASSDRV_IOC_MAGIC, 3)
#define MASSDRV_IOCTL_SEND_DATA                    _IO(MASSDRV_IOC_MAGIC, 4)
#define MASSDRV_IOCTL_RECEIVE_DATA                 _IO(MASSDRV_IOC_MAGIC, 5)
#define MASSDRV_IOCTL_NO_DATA                      _IO(MASSDRV_IOC_MAGIC, 6)
#define MASSDRV_IOCTL_UNKNOWN_COMMAND              _IO(MASSDRV_IOC_MAGIC, 7)
#define MASSDRV_IOCTL_BUFFER_SYNC                  _IO(MASSDRV_IOC_MAGIC, 8)
#define MASSDRV_IOCTL_SET_INITIALIZE_INFO          _IO(MASSDRV_IOC_MAGIC, 10)
#define MASSDRV_IOCTL_GET_MEDIA_PATH               _IO(MASSDRV_IOC_MAGIC, 11)

#endif /* __F_VENDOR_IOCTL_H_ */
