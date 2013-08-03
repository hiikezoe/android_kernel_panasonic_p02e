/*
 * drivers/fgc/pfgc_common.c
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

static void pfgc_work_func( struct work_struct *work );
static void pfgc_timer_handler( unsigned long data );

static struct workqueue_struct *pfgc_wait_wq;
static DEFINE_MUTEX(pfgc_mutex);

/*!
 execute retry function.
 @return  0  success
 -1 unsuccess
 @param[in] pinfo wait info
 */
int pfgc_exec_retry( T_FGC_WAIT_INFO *pinfo )
{
	int ret, func_ret;

	func_ret = 0;

	if (pinfo == NULL) {
		return -1;
	}

	if (pinfo->pfunc == NULL) {
		return -1;
	}

	ret = pinfo->pfunc();
	mutex_lock(&pfgc_mutex);
	if (ret != 0) {
		ret = pfgc_wait_info_push(pinfo);
		if (ret < 0) {
			func_ret = -1;
		} else if (ret == 0) {
			pfgc_add_timer();
		} else {
		}
	}
	mutex_unlock(&pfgc_mutex);
	return func_ret;
}

/*!
 push function to retry queue.
 @return  push position
 -1 unsuccess
 @param[in] pinfo wait info
 */
int pfgc_wait_info_push( T_FGC_WAIT_INFO *pinfo )
{
	int count;
	T_FGC_WAIT_INFO **pptarget;
	T_FGC_WAIT_INFO *pinfo_work;

	if (pinfo == NULL) {
		return -1;
	}

	pinfo_work = kmalloc(sizeof(T_FGC_CTL), GFP_KERNEL);
	if (pinfo_work == NULL) {
		return -1;
	}
	*pinfo_work = *pinfo;

	count = 0;

	pptarget = &gfgc_ctl.pwait_info_head;

	while (*pptarget != NULL) {
		count++;
		if (pinfo_work->pfunc == (*pptarget)->pfunc) {
			kfree(pinfo_work);
			return count;
		}
		pptarget = &((*pptarget)->pnext);
	}

	*pptarget = pinfo_work;

	return count;
}

/*!
 pop function from retry queue.
 @return  0  success
          -1 unsuccess
 @param[out] pinfo wait info
 */
int pfgc_wait_info_pop( T_FGC_WAIT_INFO *pinfo )
{
	T_FGC_WAIT_INFO *ptarget;

	if (gfgc_ctl.pwait_info_head == NULL) {
		return -1;
	}

	ptarget = gfgc_ctl.pwait_info_head;
	*pinfo = *ptarget;
	gfgc_ctl.pwait_info_head = ptarget->pnext;
	kfree(ptarget);
	return 0;
}

/*!
 push function to head of retry queue.
 @return  0  success
          -1 unsuccess
 @param[in] pinfo wait info
 */
int pfgc_wait_info_depop( T_FGC_WAIT_INFO *pinfo )
{
	T_FGC_WAIT_INFO *pinfo_work;

	pinfo_work = kmalloc(sizeof(T_FGC_CTL), GFP_KERNEL);
	if (pinfo_work == NULL) {
		return -1;
	}
	*pinfo_work = *pinfo;
	pinfo_work->pnext = gfgc_ctl.pwait_info_head;
	gfgc_ctl.pwait_info_head = pinfo_work;
	return 0;
}

/*!
 timeout handler.
 @return  Noting
 @param[in] data (unused)
 */
static void pfgc_timer_handler( unsigned long data )
{
	queue_work(pfgc_wait_wq, &gfgc_ctl.work_info);
}

/*!
 Work function.
 @return  Noting
 @param[out] work work info (unused)
 */
static void pfgc_work_func( struct work_struct *work )
{
	T_FGC_WAIT_INFO info;
	int ret;

	mutex_lock(&pfgc_mutex);
	ret = pfgc_wait_info_pop(&info);
	mutex_unlock(&pfgc_mutex);
	if (ret < 0) {
		return;
	}

	ret = info.pfunc();
	mutex_lock(&pfgc_mutex);
	if (ret < 0) {
		ret = pfgc_wait_info_depop(&info);
		if (ret < 0) {
		}
		pfgc_add_timer();
	} else {
		if (info.pnext != NULL) {
			pfgc_add_timer();
		}
	}
	mutex_unlock(&pfgc_mutex);
}

/*!
 Initialize retry work and timer.
 @return  Noting
 @param   Noting
 */
void pfgc_init_timer( void )
{
	INIT_WORK(&gfgc_ctl.work_info, pfgc_work_func);
	init_timer(&gfgc_ctl.timer_info);
	gfgc_ctl.timer_info.data = (unsigned long) &gfgc_ctl;
	pfgc_wait_wq = create_singlethread_workqueue("pfgc_wait_wq");
}

/*!
 Set timer.
 @return  Noting
 @param   Noting
 */
void pfgc_add_timer( void )
{
	int timer_msecvalue;
	int ret;

	ret = del_timer(&gfgc_ctl.timer_info);

	timer_msecvalue = 1000;
	gfgc_ctl.timer_info.function = pfgc_timer_handler;
	gfgc_ctl.timer_info.expires = jiffies + msecs_to_jiffies(
			timer_msecvalue);
	add_timer(&gfgc_ctl.timer_info);
}

/*!
 Write data method.
 @return  0  success
          1  unsuccess
 @param[in] id  open id(unused)
 @param[in] dev device (unused)
 @param[in] data write data
 */
TCOM_FUNC   pcom_acc_write( TCOM_ID        id,
                            unsigned long  dev,
                            TCOM_RW_DATA  *data )
{
	unsigned long ret_write;

	ret_write = pfgc_i2c_write(data);
	if( ret_write < 0 ){
		pfgc_log_i2c_error(PCOM_ACC_WRITE + 0x01, 0);
	}

	return ret_write;
}

/*!
 Read data method.
 @return  0  success
          1  unsuccess
 @param[in] id  open id(unused)
 @param[in] dev device (unused)
 @param[in/out] data read data
 */
TCOM_FUNC pcom_acc_read( TCOM_ID        id,
                         unsigned long  dev,
                         TCOM_RW_DATA  *data )
{
	unsigned long ret_read;

	ret_read = pfgc_i2c_read(data);
        if( ret_read < 0 ){
                pfgc_log_i2c_error(PCOM_ACC_READ + 0x01, 0);
        }

	return ret_read;
}

/*!
 Open method.
 @return  0  success
 @param[in] dev device (unused)
 @param[out] id  open id(unused)
 @param[in] dev device (unused)
 */
TCOM_FUNC pcom_acc_open( unsigned long     dev,
                         TCOM_ID          *id,
                         TCOM_OPEN_OPTION *option )
{
	return 0;
}

/*!
 Close method.
 @return  0  success
 @param[in] id  open id(unused)
 */
TCOM_FUNC pcom_acc_close( TCOM_ID id )
{
	return 0;
}

/*!
 I/O Control method.
 @return  0  success
 @param[in] id  open id(unused)
 @param[in] dev device (unused)
 @param[in] req ioctl request kind
 @param[in/out] data ioctl data
 */
TCOM_FUNC pcom_acc_ioctl( TCOM_ID        id,
                          unsigned long  dev,
                          unsigned long  req,
                          void          *data )
{
	unsigned long rate;

	switch(req) {
		case DCOM_IOCTL_RATE:
			rate = *((unsigned long *)data);
			pfgc_i2c_change_rate(rate);
			break;
		default:
			break;
	}
	return 0;
}

/*!
 EEPROM read method.
 @return  0     success
          other unsuccess
 @param[in] info_id EEPROM Area Access ID
 @param[in] info_no (unused)
 @param[out] data_adr read data
 */
int Hcm_romdata_read_knl( unsigned short  info_id,
                          unsigned short  info_no,
                          unsigned char  *data_adr )
{
	int ret, cnt;
	unsigned short size = 0x00;

	/* serch data size from eeprom table */
	for(cnt = 0; cnt < (sizeof(EEPDATA)/sizeof(EEPDATA[0])); cnt++){
		if(info_id == EEPDATA[ cnt ].type){
			size = EEPDATA[ cnt ].data_size;
			break;
		}
	}

	if( size == 0x00 ){
		ret = -1;
		return ret;
	}

	ret = cfgdrv_read(info_id, size, data_adr);

	return ret;
}

/*!
 EEPROM write method.
 @return  0     success
          other unsuccess
 @param[in] info_id EEPROM Area Access ID
 @param[in] info_no (unused)
 @param[in] data_adr write data
 */
int Hcm_romdata_write_knl( unsigned short  info_id,
                           unsigned short  info_no,
                           unsigned char  *data_adr )
{
	int ret, cnt;
	unsigned short size = 0x00;

	/* serch data size from eeprom table */
	for(cnt = 0; cnt < (sizeof(EEPDATA)/sizeof(EEPDATA[0])); cnt++){
		if(info_id == EEPDATA[ cnt ].type){
			size = EEPDATA[ cnt ].data_size;
			break;
		}
	}

	if( size == 0x00 ){
		ret = -1;
		return ret;
	}

	ret = cfgdrv_write(info_id, size, data_adr);

	return ret;
}

/*!
 Get battery info (dummy).
 @return  0     success
 @param[out] dctl_batinfo battery info
 */
int pdctl_mstate_batget( t_dctl_batinfo *dctl_batinfo )
{
	memset(dctl_batinfo, 0x00, sizeof(t_dctl_batinfo));

	return DCTL_OK;
}

/*!
 Set battery status (dummy).
 @return  Nothing
 @param[out] stat battery status
 */
void pdctl_powfac_bat_stat_set( unsigned char stat )
{
}

/*!
 Notify system error (dummy).
 @return  Nothing
 @param[in] err_rank system error rank (unused)
 @param[in] err_no function no (unused)
 @param[in] sys_data system data (unused)
 @param[in] err_sys_size system data size (unused)
 @param[in] log_data log data (unused)
 @param[in] err_log_size log data size (unused)
 */
void ms_alarm_proc( unsigned char  err_rank,
                    unsigned long  err_no,
                    void          *sys_data,
                    size_t         err_sys_size,
                    void          *log_data,
                    size_t         err_log_size )
{
}

/*!
 Get timestamp.
 @return  Nothing
 @param[out] year  year
 @param[out] month month
 @param[out] day   day
 @param[out] hour  hour
 @param[out] min   minute
 @param[out] sec   second
 */
void syserr_TimeStamp( unsigned long *year,
                       unsigned long *month,
                       time_t        *day,
                       time_t        *hour,
                       time_t        *min,
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>                              time_t        *sec );*/
                       time_t        *sec,   /*<Conan>*/
                       time_t        *usec ) /*<Conan>*/
#else /*<Conan>1111XX*/
                       time_t        *sec )
#endif /*<Conan>1111XX*/
{
    time_t         work;
    unsigned short i,tmp_month=1;
    unsigned long  tmp_year = 1970;
/*<8602253>    unsigned char  *day_of_month;                                  */
    unsigned char  day_of_month[ MONTH_OF_YEAR ];/*<8602253>                  */
    struct timeval tv;

/*<8602253>#define MONTH_OF_YEAR 12                                           */

    /*                          */
/*<8602253>    const static unsigned char normalmonth[ MONTH_OF_YEAR ] = {    */
/*<8602253>        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31             */
/*<8602253>    };                                                             */

    /*                  */
/*<8602253>    const static unsigned char leapmonth[ MONTH_OF_YEAR ] = {      */
/*<8602253>        31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31             */
/*<8602253>    };                                                             */

    day_of_month[ 0 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 1 ]  = 28;                     /*<8602253>                  */
    day_of_month[ 2 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 3 ]  = 30;                     /*<8602253>                  */
    day_of_month[ 4 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 5 ]  = 30;                     /*<8602253>                  */
    day_of_month[ 6 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 7 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 8 ]  = 30;                     /*<8602253>                  */
    day_of_month[ 9 ]  = 31;                     /*<8602253>                  */
    day_of_month[ 10 ] = 30;                     /*<8602253>                  */
    day_of_month[ 11 ] = 31;                     /*<8602253>                  */

    /* NULL         */
    if(( year == NULL ) ||
       ( month == NULL )||
       ( day == NULL )  ||
       ( hour == NULL ) ||
       ( min == NULL )  ||
       ( sec  == NULL ))
    {
        /* ERR_MSG("### [syserr_TimeStamp]:Arg is NULL Pointer ###\n"); */
        return;
    }

    /*                    */
    do_gettimeofday(&tv);

    work = tv.tv_sec;
    /*    work = xtime.tv_sec; */
    /*    localtime(&work); */

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    *usec = tv.tv_usec;/*<Conan>*/
#endif /*<Conan>1111XX*/

    /*                     ? */
    if (work < 0)
    {
        /* 2004/01/01 00:00:00 */
        *year = 2004;
        *month = 1;
        *day = 1;
        *hour = 0;
        *min = 0;
        *sec = 0;
    }

    /*                + 9                     */
    work += 9 * 60 * 60;

    /*          */
    *sec = work % 60;

    /*          */
    work -= *sec;
    work /= 60;
    *min = work % 60;

    /*            */
    work -= *min;
    work /= 60;
    *hour = work % 24;

    work -= *hour;           /*          */
    *day = work / 24;        /* 1970                            */

/*<D1IO092>while( *day >= 366 )      *//* 1970                            */
    while( *day >= 366 )   /* 1970                            *//*<D1IO092>  M*/
    {
        /*       ? */
        if((( tmp_year % 4 ) == 0 ) &&
           ((( tmp_year % 100 ) != 0 ) || (( tmp_year % 400 ) == 0 )))
        {
            *day -= 366;    /*         366       */
        }
        else
        {
            *day -= 365;        /*                 365        */
        }

        tmp_year++;
    }

    *day += 1;                  /* 1    1      0        */


    if((( tmp_year % 4 ) == 0 ) &&
       ((( tmp_year % 100 ) != 0 ) || (( tmp_year % 400 ) == 0 )))
    {
        /*                              */
/*<8602253> day_of_month = (unsigned char *)&leapmonth[0];                    */
        day_of_month[ FEBRUARY ] = LEAPMONTH;    /*<8602253>                  */
    }
    else
    {
        /*                                    */
/*<8602253> day_of_month = (unsigned char *)&normalmonth[0];                  */
        day_of_month[ FEBRUARY ] = NORMALMONTH;  /*<8602253>                  */
    }

    /* day                              */
/*<D1IO092>for( i = 0; i < MONTH_OF_YEAR; i++ )                               */
    for( i = 0; i < MONTH_OF_YEAR; i++ )          /*<D1IO092>  D              */
    {
        if(*day <= (time_t)*( day_of_month + ( tmp_month - 1 )))
        {
            break;
        }
        else
        {
            *day -= *( day_of_month + ( tmp_month - 1 ));
            tmp_month++;
        }
    }

    /* 1  1       */
    if( tmp_month == 13 )
    {
        tmp_month = 1;
        tmp_year += 1;
    }
#if 0
    if( tmp_year < 2004 )
    {
        /* 2004/01/01 00:00:00 */
        *year = 2004;
        *month = 1;
        *day = 1;
        *hour = 0;
        *min = 0;
        *sec = 0;

        return;
    }
#endif

    *year = tmp_year;                                              /* pgr0541 */
    *month = tmp_month;                                            /* pgr0541 */
}

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
void monotonic_TimeStamp( unsigned long *day,                                   /*<Conan time add>*/
                          time_t        *hour,
                          time_t        *min,
                          time_t        *sec,
                          time_t        *usec )
{
    time_t         work;
    unsigned long  usec_tmp;
    struct timespec tp;

    /*                    */
    tp.tv_sec = tp.tv_nsec = 0;
    do_posix_clock_monotonic_gettime(&tp);
    monotonic_to_bootbased(&tp);

    work = tp.tv_sec;

    /* nsec       */
    usec_tmp = (unsigned long)(tp.tv_nsec / 1000);
    *usec = usec_tmp;        /* nsec           */

    /*          */
    *sec = work % 60;

    /*          */
    work -= *sec;
    work /= 60;
    *min = work % 60;

    /*            */
    work -= *min;
    work /= 60;
    *hour = work % 24;

    work -= *hour;           /*          */
    *day = work / 24;        /*                              */

}
#endif /*<Conan>1111XX*/
