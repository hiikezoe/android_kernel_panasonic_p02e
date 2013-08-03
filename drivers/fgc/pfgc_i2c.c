/*
 * drivers/fgc/pfgc_i2c.c
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
#include <linux/rtc.h>
#include <linux/cfgdrv.h>
#define DFGC_RST_MONTH_SHIFT        22  /* shift for month */
#define DFGC_RST_DAY_SHIFT          17  /* shift for day */
#define DFGC_RST_HOUR_SHIFT         12  /* shift for hour */
#define DFGC_RST_MIN_SHIFT          6   /* shift for min */

static int pfgc_i2c_master_send( struct i2c_client *client,const char *buf ,int count );
static int pfgc_i2c_master_recv( struct i2c_client *client, char *buf ,int count );

static struct t_fgc_device_info fgc_device_info;
static unsigned long fgc_i2c_speed;              /* I2C transfer rate        */
static bool fgc_i2c_change_speed; /* IDPower #4 */

static DEFINE_MUTEX(pfgc_i2c_mutex);

/*!
 Write data by i2c.
 @return  0     success
          other unsuccess
 @param[in] data write data
 */
unsigned long pfgc_i2c_write( TCOM_RW_DATA *data )
{
	unsigned long *offset_local;
	unsigned char *data_1b;
	unsigned char offset_1b[1];
	unsigned char send_data[DFGC_FGC_SEQ_WRITE_MAX + 1];
	unsigned char i;
	int byte_num;
	int ret;
	struct i2c_client *i2c;
	unsigned char sequential = DFGC_OFF;
	unsigned long wait_usec = DFGC_WAIT_WRITE_USEC;


	mutex_lock(&pfgc_i2c_mutex);

	FGC_FUNCIN("offset=0x%lX size=%ld number=%ld\n",*data->offset, data->size, data->number);
	if(data == NULL){
		mutex_unlock(&pfgc_i2c_mutex);
		FGC_ERR("data == NULL\n");
		return EINVAL;
	}

	if(data->option != NULL){
		if((*((unsigned long*)data->option) & DCOM_ACCESS_MODE_MSK) == DCOM_ACCESS_MODE2){
			/* SlaveAddress=FG-IC(ROM)   */
			i2c = fgc_device_info.i2c_rom_dev->client;
			wait_usec = DFGC_WAIT_ROM_USEC;
		}else{
			/* SlaveAddress=FG-IC(Normal) */
			i2c = fgc_device_info.i2c_dev->client;
		}
		if((*((unsigned long*)data->option) & DCOM_SEQUENTIAL_ADDR) == DCOM_SEQUENTIAL_ADDR){
			sequential = DFGC_ON;
		}
	}else{
		/* SlaveAddress=FG-IC(Normal) */
		i2c = fgc_device_info.i2c_dev->client;
	}

	offset_local = data->offset;
	data_1b = (unsigned char *)data->data;

	if(sequential){ /* SEQUENTIAL WRITE           */
		/* sequential write */
		if(data->number >  DFGC_FGC_SEQ_WRITE_MAX)
		{
			mutex_unlock(&pfgc_i2c_mutex);
			FGC_ERR("sequential write max size over %ld\n", data->number);
			return EINVAL;
		}
		offset_1b[0] = (unsigned char)(*offset_local & 0xFF);
		FGC_DEBUG("sequential mode adr=0x%X data[0]=0x%X... number=%ld\n", offset_1b[0], data_1b[0], data->number);
		send_data[0] = offset_1b[0];
		memcpy( &send_data[1], data_1b, data->number);
		byte_num = data->number + 1;
		ret = pfgc_i2c_master_send(i2c, send_data, byte_num);
		if(ret < 0){
			mutex_unlock(&pfgc_i2c_mutex);
			FGC_ERR("pfgc_i2c_master_send() failed. ret=%d\n",ret);
			pfgc_log_i2c_error(PFGC_I2C_WRITE + 0x01, 0);
			return (unsigned long)ret;
		}

		udelay(wait_usec);
	}else{
		FGC_DEBUG("normal mode\n");

		byte_num = 2;
		for(i = 0; i < (data->number); i++){
			/* write */
			offset_1b[0] = (unsigned char)(*offset_local & 0xFF);
			FGC_DEBUG("adr=0x%X data[0]=0x%X... size=%d\n", offset_1b[0], data_1b[0], byte_num);
			send_data[0] = offset_1b[0];
			send_data[1] = data_1b[0];
			ret = pfgc_i2c_master_send(i2c, send_data, byte_num);
			if(ret < 0){
				mutex_unlock(&pfgc_i2c_mutex);
				FGC_ERR("pfgc_i2c_master_send() failed. ret=%d\n",ret);
				pfgc_log_i2c_error(PFGC_I2C_WRITE + 0x02, 0);
				return (unsigned long)ret;
			}

			data_1b++;
			offset_local++;
			udelay(wait_usec);
		}
	}

	mutex_unlock(&pfgc_i2c_mutex);

	return 0;
}

/*!
 Read data by i2c.
 @return  0     success
          other unsuccess
 @param[in/out] data read data
 */
unsigned long pfgc_i2c_read( TCOM_RW_DATA *data )
{
	unsigned long *offset_local;
	unsigned char *data_1b;
	unsigned char offset_1b[1];
	unsigned char i;
	int byte_num;
	int ret;
	struct i2c_client *i2c;
	unsigned char sequential = DFGC_OFF;
	unsigned long wait_usec = DFGC_WAIT_STP_STT_USEC;


	mutex_lock(&pfgc_i2c_mutex);

	FGC_FUNCIN("offset=0x%lX size=%ld number=%ld\n",*data->offset, data->size, data->number);

	if(data == NULL){
		mutex_unlock(&pfgc_i2c_mutex);
		FGC_ERR("data == NULL\n");
		return EINVAL;
	}

	if(data->option != NULL){
		if((*((unsigned long*)(data->option)) & DCOM_ACCESS_MODE_MSK) == DCOM_ACCESS_MODE2){
			/* SlaveAddress=FG-IC(ROM)   */
			i2c = fgc_device_info.i2c_rom_dev->client;
			wait_usec = DFGC_WAIT_ROM_USEC;
		}else{
			/* SlaveAddress=FG-IC(Normal) */
			i2c = fgc_device_info.i2c_dev->client;
		}

		if((*((unsigned long*)(data->option)) & DCOM_SEQUENTIAL_ADDR) == DCOM_SEQUENTIAL_ADDR){
			sequential = DFGC_ON;
		}
	}else{
		/* SlaveAddress=FG-IC(Normal) */
		i2c = fgc_device_info.i2c_dev->client;
	}

	if(data->size == 1){ /* 1byte only                 */
		offset_local = data->offset;
		data_1b = (unsigned char *)data->data;
		byte_num = 1;

		if(sequential){ /* SEQUENTIAL READ            */
			/* sequential read */

			FGC_DEBUG("sequential mode adr=0x%X data[0]=0x%X... number=%ld\n", offset_1b[0], data_1b[0], data->number);

			offset_1b[0] = (unsigned char)(*offset_local & 0xFF);
			ret = pfgc_i2c_master_send(i2c, offset_1b, byte_num);

			if(ret < 0){
				mutex_unlock(&pfgc_i2c_mutex);
				FGC_ERR("pfgc_i2c_master_send() failed. ret=%d\n",ret);
				pfgc_log_i2c_error(PFGC_I2C_READ + 0x01, 0);
				return (unsigned long)ret;
			}
			udelay(wait_usec);

			byte_num = data->number;
			ret = pfgc_i2c_master_recv(i2c, data_1b, byte_num);
			if(ret < 0){
				mutex_unlock(&pfgc_i2c_mutex);
				FGC_ERR("pfgc_i2c_master_recv() failed. ret=%d\n",ret);
				pfgc_log_i2c_error(PFGC_I2C_READ + 0x03, 0);
				return (unsigned long)ret;
			}

			udelay(wait_usec);
		}else{
			FGC_DEBUG("normal mode\n");

			for(i = 0; i < ( data->number ); i++){
				/* read */
				offset_1b[0] = (unsigned char)(*offset_local & 0xFF);

				FGC_DEBUG("adr=0x%X data[0]=0x%X... size=%d\n", offset_1b[0], data_1b[0], byte_num);

				ret = pfgc_i2c_master_send(i2c, offset_1b, byte_num);
				if(ret < 0){
					mutex_unlock(&pfgc_i2c_mutex);
					FGC_ERR("pfgc_i2c_master_send() failed. ret=%d\n",ret);
					pfgc_log_i2c_error(PFGC_I2C_READ + 0x02, 0);
					return (unsigned long)ret;
				}
				udelay(wait_usec);

				ret = pfgc_i2c_master_recv(i2c, data_1b, byte_num);
				if(ret < 0){
					mutex_unlock(&pfgc_i2c_mutex);
					FGC_ERR("pfgc_i2c_master_recv() failed. ret=%d\n",ret);
					pfgc_log_i2c_error(PFGC_I2C_READ + 0x04, 0);
					return (unsigned long)ret;
				}
				FGC_LOG("offset=%#02lx data=%#02x\n",*offset_local, *data_1b);
				data_1b++;
				offset_local++;
				udelay(wait_usec);
			}
		}
	}

	mutex_unlock(&pfgc_i2c_mutex);

	return 0;
}

/**
 * pfgc_i2c_master_send - issue a single I2C message in master transmit mode
 * @client: Handle to slave device
 * @buf: Data that will be written to the slave
 * @count: How many bytes to write
 *
 * Returns negative errno, or else the number of bytes written.
 */
static int pfgc_i2c_master_send( struct i2c_client *client,const char *buf ,int count )
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
#if 1 /* IDPower #4 */
	if((fgc_i2c_change_speed == true) && (fgc_i2c_speed == DCOM_RATE100K)){
		msg.flags |= I2C_M_FORCE_SS;
		fgc_i2c_change_speed =false;
	}
	else if((fgc_i2c_change_speed == true) && (fgc_i2c_speed == DCOM_RATE400K)){
		msg.flags |= I2C_M_FORCE_FS;
		fgc_i2c_change_speed =false;
	}
	else{
	}
#else /* IDPower #4 */
	if(fgc_i2c_speed == DCOM_RATE100K){
		msg.flags |= I2C_M_FORCE_SS;
	}
#endif /* IDPower #4 */
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);
	if( ret < 0 ){
		pfgc_log_i2c_error(PFGC_I2C_MASTER_SEND +0x01, (unsigned short)(-1 * ret));
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

/**
 * pfgc_i2c_master_recv - issue a single I2C message in master receive mode
 * @client: Handle to slave device
 * @buf: Where to store data read from slave
 * @count: How many bytes to read
 *
 * Returns negative errno, or else the number of bytes read.
 */
static int pfgc_i2c_master_recv( struct i2c_client *client, char *buf ,int count )
{
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
#if 1 /* IDPower #4 */
	if((fgc_i2c_change_speed == true) && (fgc_i2c_speed == DCOM_RATE100K)){
		msg.flags |= I2C_M_FORCE_SS;
		fgc_i2c_change_speed =false;
	}
	else if((fgc_i2c_change_speed == true) && (fgc_i2c_speed == DCOM_RATE400K)){
		msg.flags |= I2C_M_FORCE_FS;
		fgc_i2c_change_speed =false;
	}
	else{
	}
#else /* IDPower #4 */
	if(fgc_i2c_speed == DCOM_RATE100K){
		msg.flags |= I2C_M_FORCE_SS;
	}
#endif /* IDPower #4 */
	msg.len = count;
	msg.buf = buf;

//	DEBUG_FGIC(("adap->dev->p->driver_data->->pdata->clk_freq = %d\n", adap->dev->p->driver_data->pdata->clk_freq));
	ret = i2c_transfer(adap, &msg, 1);
	if( ret < 0 ){
		pfgc_log_i2c_error(PFGC_I2C_MASTER_RECV + 0x01, (unsigned short)(-1 * ret));
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

/*!
 Set enable/disable sleep+.
 @return  0  success
          -1 unsuccess
 @param[in] type enable/disable command
 */
int pfgc_i2c_sleep_set( unsigned short type )
{
	TCOM_RW_DATA rw_data;
	unsigned char setdata[2];
	unsigned long option;
	unsigned long i2c_ret;
	int ret;

	FGC_FUNCIN("\n");
	ret = DFGC_OK;

	setdata[0] = (unsigned char)(type & 0xFF);
	setdata[1] = (unsigned char)((unsigned)(type & 0xFF00) >> 8);

	option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
	rw_data.option = &option;
	rw_data.size = 1;

	MFGC_FGRW_DTSET(rw_data, CtrlRegAdr, setdata);

	i2c_ret = pfgc_i2c_write(&rw_data);
	if(i2c_ret != 0){
		pfgc_log_i2c_error(PFGC_I2C_SLEEP_SET + 0x01 , 0);
		ret = -DFGC_NG;
	}

	return ret;
}

/*!
 Read Control() register.
 @return  0  success
          -1 unsuccess
 @param[out] pdata control register value
 */
int pfgc_i2c_get_controlreg( unsigned short *pdata )
{
	TCOM_RW_DATA rw_data;
	unsigned long option;
	unsigned long i2c_ret;
	int ret;

	FGC_FUNCIN("\n");
	ret = DFGC_OK;

	option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
	rw_data.offset = (unsigned long *)&CtrlRegAdr[0];
	rw_data.data = (void *)&CtrlStsCmd[0];
	rw_data.option = &option;
	rw_data.size = 1;
	rw_data.number = 2;

	i2c_ret = pfgc_i2c_write(&rw_data);
	if(i2c_ret != 0){
		pfgc_log_i2c_error(PFGC_I2C_GET_CONTROLREG + 0x01 , 0);
		FGC_ERR("pfgc_i2c_write() failed.");
		ret = -DFGC_NG;
		return ret;
	}

	rw_data.data = pdata;
	i2c_ret = pfgc_i2c_read(&rw_data);
	if(i2c_ret != 0){
		pfgc_log_i2c_error(PFGC_I2C_GET_CONTROLREG + 0x02 , 0);
		FGC_ERR("pfgc_i2c_read() failed.");
		ret = -DFGC_NG;
	}

	return ret;
}

/*!
 I2C probe method.
 @return  0  success
          other unsuccess
 @param[in] i2c i2c client
 @param[in] devid i2c device id (unused)
 */
int pfgc_i2c_probe( struct i2c_client *i2c, const struct i2c_device_id *devid )
{
	struct t_fgc_i2c_dev_info *di;
	int retval = 0;
	
	FGC_FUNCIN("\n");

	if(i2c == NULL){
		retval = -EINVAL;
		return retval;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if(!di){
		dev_err( &i2c->dev, "failed to allocate device info data\n" );
		retval = -ENOMEM;
		return retval;
	}

	i2c_set_clientdata(i2c, di);
	di->dev = &i2c->dev;
	di->client = i2c;

	fgc_device_info.i2c_dev = di;

	FGC_FUNCOUT("\n");
	return retval;
}

/*!
 I2C probe method of ROM mode.
 @return  0  success
          other unsuccess
 @param[in] i2c i2c client
 @param[in] devid i2c device id (unused)
 */
int pfgc_i2c_rom_probe( struct i2c_client *i2c,
	                const struct i2c_device_id *devid )
{
	struct t_fgc_i2c_dev_info *di;
	int retval = 0;

	if(i2c == NULL){
		retval = -EINVAL;
		return retval;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if(!di){
		dev_err(&i2c->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		return retval;
	}

	i2c_set_clientdata(i2c, di);
	di->dev = &i2c->dev;
	di->client = i2c;

	fgc_device_info.i2c_rom_dev = di;

	return retval;
}

/*!
 I2C remove method.
 @return  0  success
          other unsuccess
 @param[in] i2c i2c client (unused)
 */
int pfgc_i2c_remove( struct i2c_client *i2c )
{
	kfree(fgc_device_info.i2c_dev);

	return 0;
}

/*!
 I2C remove method of ROM mode.
 @return  0  success
          other unsuccess
 @param[in] i2c i2c client (unused)
 */
int pfgc_i2c_rom_remove( struct i2c_client *i2c )
{
	kfree(fgc_device_info.i2c_rom_dev);

	return 0;
}

/*!
 I2C suspend method.
 @return  0  success
          other unsuccess
 @param[in] i2c i2c client (unused)
 @param[in] mesg message (unused)
 */
int pfgc_i2c_suspend( struct i2c_client *i2c, pm_message_t mesg )
{
	int ret;
	int i;
	unsigned long flags;
	unsigned long fg_flag;

	FGC_FUNCIN("\n");

	spin_lock_irqsave( &gfgc_spinlock_if, flags );
	fg_flag = gfgc_kthread_flag;
/*<npdc100388275>	spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); */

	if((fg_flag & D_FGC_FG_INT_IND ) == D_FGC_FG_INT_IND)
	{
		printk("**** FGC-INT SUSPEND-RETRY \n");
		spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); /*<npdc100388275>*/
		return -1;
	}
	spin_unlock_irqrestore( &gfgc_spinlock_if, flags );     /*<npdc100388275>*/

	pfgc_log_proc_suspend();
	for(i = 0; i < 3; i++)
	{
		ret = pfgc_i2c_sleep_set(D_FGC_SLEEP_DISABLE);
		if (ret == DFGC_OK)
		{
			atomic_set(&gfgc_susres_flag, 0);               /* npdc300035452 */
			break;
		}
		pfgc_log_i2c_error(PFGC_I2C_SUSPEND + 0x01 , 0);
	}

	return ret;
}

/*!
 I2C resume method.
 @return  0  success
          other unsuccess
 @param[in] i2c i2c client (unused)
 */
int pfgc_i2c_resume( struct i2c_client *i2c )
{
	int ret;
	int i;
	FGC_FUNCIN("\n");

	pfgc_log_proc_resume();
	for(i = 0; i < 3; i++)
	{
		ret = pfgc_i2c_sleep_set(D_FGC_SLEEP_ENABLE);
		if (ret == DFGC_OK)
		{
			atomic_set(&gfgc_susres_flag, 1);               /* npdc300035452 */
			wake_up(&gfgc_susres_wq);                       /* npdc300035452 */
			break;
		}
		pfgc_log_i2c_error(PFGC_I2C_RESUME + 0x01 , 0);
	}

	return ret;
}

/*!
 I2C shutdown method.
 @return  Nothing
 @param[in] client i2c client (unused)
 */
void pfgc_i2c_shutdown(struct i2c_client *client)
{
	unsigned long flags;

	FGC_FUNCIN("\n");

	spin_lock_irqsave( &gfgc_spinlock_if, flags );
	gfgc_kthread_flag |= D_FGC_SHUTDOWN_IND;
	spin_unlock_irqrestore( &gfgc_spinlock_if, flags );
	wake_up( &gfgc_task_wq );

	/* wait for kernel thread */
	wait_for_completion_timeout(&gfgc_comp, msecs_to_jiffies(500));
	FGC_FUNCOUT("\n");
}

/*!
 Change I2C transfer rate.
 @return  Nothing
 @param[in] rate i2c transfer rate
 */
void pfgc_i2c_change_rate( unsigned long rate )
{
	FGC_FUNCIN("rate=%ld\n", rate);

	fgc_i2c_speed = rate;
	fgc_i2c_change_speed = true; /* IDPower #4 */

	return;
}

static const struct i2c_device_id
        fuelgauge_id_table[] = {{"fuelgauge", 0}, {}};

static const struct i2c_device_id
        fuelgauge_id_table_rom[] = {{"fuelgauge_rom", 0}, {}};

static struct i2c_driver
	i2c_fuelgauge = {
	                 .probe = pfgc_i2c_probe,
	                 .remove = pfgc_i2c_remove,
	                 .shutdown = pfgc_i2c_shutdown,
	                 .suspend = pfgc_i2c_suspend,
	                 .resume = pfgc_i2c_resume,
	                 .driver =
	                  {
	                   .name = "fuelgauge",
	                   .owner = THIS_MODULE,
	                  },
	                 .id_table = fuelgauge_id_table, };

static struct i2c_driver
	i2c_fuelgauge_rom = {
	                     .probe = pfgc_i2c_rom_probe,
	                     .remove = pfgc_i2c_rom_remove,
	                     .driver =
	                      {
	                       .name = "fuelgauge_rom",
	                       .owner = THIS_MODULE,
	                      },
	                      .id_table = fuelgauge_id_table_rom, };
/*!
 Initialize I2C.
 @return  0 success
          other unsuccess
 @param   Nothing
 */
int pfgc_i2c_init( void )
{
	int ret;

	fgc_i2c_speed = DCOM_RATE400K;
	fgc_i2c_change_speed = false; /* IDPower #4 */

	ret = i2c_add_driver(&i2c_fuelgauge);

	if (ret == 0) {
		ret = i2c_add_driver(&i2c_fuelgauge_rom);
		if (ret != 0) {
			i2c_del_driver(&i2c_fuelgauge);
		}
	}

	return ret;
}

/*!
 Terminate I2C.
 @return  Nothing
 @param   Nothing
 */
void pfgc_i2c_exit( void )
{
	i2c_del_driver(&i2c_fuelgauge);
	i2c_del_driver(&i2c_fuelgauge_rom);
}

void pfgc_log_i2c_error( unsigned short path, unsigned short info )
{
        unsigned char count;
        unsigned long index;
        unsigned long path_info;
        unsigned long code_info;
        unsigned long timestamp;
        signed int ret;
        struct rtc_time tm;
        struct timeval tv;
        ret = 0;
        count = 0;
        tm.tm_mon = 0;
        tm.tm_mday = 0;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        // get count
        ret = cfgdrv_read(D_HCM_E_FGIC_I2C_ERROR_COUNT, sizeof(count), &count);

        if (ret < 0) {
                count = 0;
        }

        if (count < 255) {

                index = count % 16;

                // path
                path_info = path;
                // info
                code_info = info;

                // write error_code & error_path
                ret =  cfgdrv_write(error_code_tbl[index], sizeof(code_info), (unsigned char *)&code_info);
                if (ret < 0) {
                        printk("cannnot write error code : ret %d \n", ret);
                }

                ret =  cfgdrv_write(error_path_tbl[index], sizeof(path_info), (unsigned char *)&path_info);
                if (ret < 0) {
                        printk("cannnot write error path : ret %d \n", ret);
                }

                /*
                   syserr_TimeStamp is a temporary function. It should be replaced with a function which uses RTC.
                   syserr_TimeStamp( &year, &month, &day, &hour, &min, &second );
                 */
                /* which should be used jiffies or get_timeofday? */
                /*        rtc_time_to_tm(jiffies, &tm); */
                do_gettimeofday(&tv);
                rtc_time_to_tm(tv.tv_sec, &tm);

                timestamp = ((((((tm.tm_mon +1) << DFGC_MONTH_SHIFT)
                                  | ((unsigned long)tm.tm_mday << DFGC_DAY_SHIFT))
                                  | ((unsigned long)tm.tm_hour << DFGC_HOUR_SHIFT))
                                  | ((unsigned long)tm.tm_min << DFGC_MIN_SHIFT))
                                  | (unsigned long)tm.tm_sec);

                ret = cfgdrv_write(error_timestamp_tbl[index], sizeof(timestamp), (unsigned char *)&timestamp);
                if (ret < 0) {
                        printk("cannnot write timestamp : ret %d \n", ret);
                }

                // count up & write
                count++;
                ret = cfgdrv_write(D_HCM_E_FGIC_I2C_ERROR_COUNT, sizeof(count), &count);

                if (ret < 0) {
                        printk("cannnot write error count : ret %d \n", ret);
                }
        }
}

