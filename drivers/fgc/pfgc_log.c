/*
 * drivers/fgc/pfgc_log.c
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

/*----------------------------------------------------------------------------*/
/* include                                                                    */
/*----------------------------------------------------------------------------*/
#include "inc/pfgc_local.h"                      /*         DD                */
#include <linux/jiffies.h>

#define DFGC_LOG_PROC_MAXBUF 64

typedef int (*pfgc_log_commandhandler)( void *arg1, void *arg2, void *arg3 );

struct t_fgc_log_command_info {
	char *strName;
	pfgc_log_commandhandler pFunc;
	char *strComment;
};

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
static void pfgc_log_dump_file( T_FGC_LOG *ptr );/*<4210014> DUMP             */
#if DFGC_SW_TEST_DEBUG
static void pfgc_sdram_dumplog_proc( char **ptr );
static void pfgc_log_capacity_proc( char **ptr );
static void pfgc_log_ocv_proc( char **ptr );
static void pfgc_log_charge_proc( char **ptr );
#endif /* DFGC_SW_TEST_DEBUG */

static void pfgc_log_proc_work_handler( struct work_struct *work );
static void pfgc_log_proc_timer_handler( unsigned long data );
static int pfgc_log_proc_write( struct file *file, const char *buffer, unsigned long count, void *data ); /* mrc24602 */
static int pfgc_log_proc_handle_help( void *arg1, void *arg2, void *arg3 );
static int pfgc_log_proc_handle_interval( void *arg1, void *arg2, void *arg3 );
static int pfgc_log_atoi( char *strValue );
static char *pfgc_log_skip_space( char *aryInputBuf );
static size_t pfgc_log_strlen_word( char *aryInputBuf );
static struct t_fgc_log_command_info *debug_search_command( char *aryInputBuf, struct t_fgc_log_command_info *pTable, int table_num );

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
static char		pfgc_log_procbuf[ DFGC_LOG_PROC_MAXBUF + 1 ];
static char		*pfgc_log_dispbuf;
struct timer_list 	pfgc_log_proc_timer;
struct workqueue_struct *pfgc_log_proc_wq;
struct work_struct 	pfgc_log_proc_work;
static int		pfgc_log_proc_interval_sec;
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*                pfgc_log_ocv_init                                           */
/*                SYS                                                         */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                T_FGC_SYSERR_DATA *info  SYSERR                             */
/*                                                                            */
/******************************************************************************/
void pfgc_log_syserr( T_FGC_SYSERR_DATA *info )
{

    MFGC_RDPROC_PATH( DFGC_LOG_SYSERR | 0x0000 );

    if( info == NULL )                           /*       NULL                */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_SYSERR | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_LOG_SYSERR | 0x0001 );
        return;
    }

    memcpy(( void * )&info->data[ 12 ],          /*  S                        */
           ( void * )&gfgc_ctl.mac_state,
           sizeof( gfgc_ctl.mac_state ));

    info->data[ 16 ] = gfgc_ctl.chg_sts;         /* UI                        */
    info->data[ 17 ] = gfgc_ctl.dev1_chgsts;     /*                           */
    info->data[ 18 ]                             /*                           */
        = ( unsigned char )(( gfgc_ctl.dev1_volt & 0xFF00 ) >> 8 );
    info->data[ 19 ]                             /*                           */
        = ( unsigned char )( gfgc_ctl.dev1_volt & 0x00FF );
    info->data[ 20 ] = gfgc_ctl.dev1_class;      /*            class          */
    info->data[ 21 ] = gfgc_ctl.capacity_old;    /*                           */
    info->data[ 22 ] = gfgc_ctl.health_old;      /*                           */
                                                 /*     23                    */
    info->data[ 24 ]                             /* Wakeup                    */
/*<PCIO033>        = ( unsigned char )(( gfgc_ctl.flag & 0xFF000000 ) >> 24 );*/
        = ( unsigned char )( gfgc_ctl.flag & 0x000000FF ); /*<PCIO033>        */
    info->data[ 25 ]                             /* Wakeup                    */
/*<PCIO033>        = ( unsigned char )(( gfgc_ctl.flag & 0x00FF0000 ) >> 16 );*/
        = ( unsigned char )(( gfgc_ctl.flag & 0x0000FF00 ) >> 8 ); /*<PCIO033>*/
    info->data[ 26 ]                             /* Wakeup                    */
/*<PCIO033>        = ( unsigned char )(( gfgc_ctl.flag & 0x0000FF00 ) >> 8 ); */
        = ( unsigned char )(( gfgc_ctl.flag & 0x00FF0000 ) >> 16 );/*<PCIO033>*/
    info->data[ 27 ]                             /* Wakeup                    */
/*<PCIO033>        = ( unsigned char )( gfgc_ctl.flag & 0x000000FF );*/
        = ( unsigned char )(( gfgc_ctl.flag & 0xFF000000 ) >> 24 );/*<PCIO033>*/
    info->data[ 28 ] = gfgc_ctl.chg_start;       /*                           */
    info->data[ 29 ] = gfgc_batt_info.capacity;  /*                           */
    info->data[ 30 ] = gfgc_batt_info.health;    /*                           */

    ms_alarm_proc(                               /*  V                        */
                info->kind,
                info->func_no,
                NULL,
                0,
                ( void * )info->data,
                CSYSERR_LOGINF_LEN );

    MFGC_RDPROC_PATH( DFGC_LOG_SYSERR | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_log_dump_init                                          */
/*                FGC-IC                                                      */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pfgc_log_dump_init( void )
{
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    MFGC_RDPROC_PATH( DFGC_LOG_DUMP_INIT | 0x0000 );

                                                /*                 WBB        */
    gfgc_ctl.remap_log_dump = ioremap_cache( DFGC_SDRAM_LOG, /*  C            */
                                             sizeof( T_FGC_LOG ),
                                             CACHE_WBB );

    if( gfgc_ctl.remap_log_dump == NULL )       /* ioremap                    */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_INIT | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_INIT | 0x0001 );
        MFGC_SYSERR_DATA_SET2(
                    CSYSERR_ALM_RANK_A,
                    DFGC_SYSERR_IOREMAP_ERR,
                    ( DFGC_LOG_DUMP_INIT | 0x0001 ),
                    DFGC_SDRAM_LOG,
                    sizeof( T_FGC_LOG ),
                    syserr_info );
        pfgc_log_syserr( &syserr_info );         /*  V SYSERR                 */
    }

                                                 /*                           */
    if( gfgc_ctl.remap_log_dump->fgc_log_data[ 0 ].log_wp
                        >= DFGC_LOG_MAX )
    {
        gfgc_ctl.remap_log_dump->fgc_log_data[ 0 ].log_wp = 0;
    }

                                                 /*<1130369>                  */
    pdctl_powfac_bat_stat_set( gfgc_ctl.remap_ocv->ocv_ctl_flag );/*<1130369> */

    /* log area initialize in every starting */
    {
        memset( gfgc_ctl.remap_log_dump,         /*  S                        */
                0,
                sizeof( T_FGC_LOG ));
    }

    MFGC_RDPROC_PATH( DFGC_LOG_DUMP_INIT | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_log_dump                                               */
/*                FG-IC                                                       */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                unsigend char type                                          */
/*                T_FGC_REGDUMP *regdump                                      */
/*                                                                            */
/******************************************************************************/
void pfgc_log_dump( unsigned char type, T_FGC_REGDUMP *regdump)
{
    unsigned long   flags;                       /*                           */
    unsigned short  wp;                          /*                           */
    T_FGC_LOG_DATA  *log_data;                   /*                           */
    T_FGC_CTL       *fgc_ctl;                    /* FGC-DD                    */
    T_FGC_BATT_INFO *batt_info;                  /*                           */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    T_FGC_SOH     *t_soh;                        /*                           *//*<Conan>*/
#endif /*<Conan>1111XX*/


    MFGC_RDPROC_PATH( DFGC_LOG_DUMP | 0x0000 );

    spin_lock_irqsave( &gfgc_spinlock_log, flags ); /*  V                     */

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    t_soh = gfgc_ctl.remap_soh;                  /*                           *//*<Conan>*/
#endif /*<Conan>1111XX*/

                                                 /*                           */
    wp = gfgc_ctl.remap_log_dump->fgc_log_data[ 0 ].log_wp;

                                                 /*                           */
    log_data =  &gfgc_ctl.remap_log_dump->fgc_log_data[ wp ];

    do_gettimeofday( &log_data->tv );             /*  V                        */

                                                 /*                          */
    fgc_ctl               = &gfgc_ctl;           /* FG-DD                     */
    log_data->dev1_chgsts = fgc_ctl->dev1_chgsts;/*         1                 */
/*<3230948>    log_data->dev1_volt   = fgc_ctl->dev1_chgsts;*//*         1                 */
    log_data->dev1_volt   = fgc_ctl->dev1_volt;  /*<3230948>         1                 */
    log_data->dev1_class  = fgc_ctl->dev1_class; /*         1  class          */
    log_data->type        = type;                /*                           */
//Jessie del    log_data->chg_sts     = fgc_ctl->chg_sts;    /* UI                        */
    log_data->chg_sts     = gfgc_chgsts_thread;  /* UI                        */ //Jessie add
    batt_info             = &gfgc_batt_info;     /*                     adr   */
    log_data->capacity    = batt_info->capacity; /* UI                        */
    log_data->health      = batt_info->health;   /* UI                        */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    log_data->soh_health             =           /*                           *//*<Conan>*/
        t_soh->soh_health;                                                      /*<Conan>*/
    log_data->soh_cycle_cnt          =           /*                           *//*<Conan>*/
        t_soh->cycle_cnt;                                                       /*<Conan>*/
    log_data->degra_batt_estimate_val =          /*                           *//*<Conan>*/
        t_soh->degra_batt_estimate_val;                                         /*<Conan>*/
    log_data->cycle_degra_batt_capa  =           /*                           *//*<Conan>*/
        t_soh->cycle_degra_batt_capa;                                           /*<Conan>*/
    log_data->soh_degra_batt_capa    =           /*                           *//*<Conan>*/
        t_soh->soh_degra_batt_capa;                                             /*<Conan>*/
#endif /*<Conan>1111XX*/
/*<1905359>    if(( type != D_FGC_CHG_INT_IND ) &&     *//*<3210076>                  */
/*<1905359>       ( type != D_FGC_RF_CLASS_IND ))      *//*<3210076>RFClass           */
    if((( type & D_FGC_FG_INIT_IND )             /*<1905359>                  */
        == D_FGC_FG_INIT_IND ) ||                /*<1905359>                  */
       (( type & D_FGC_FG_INT_IND )              /*<1905359> FGIC             */
        == D_FGC_FG_INT_IND ))                   /*<1905359>                  */
    {                                            /*<3210076>regdump           */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP | 0x0002 );/*<3210076>                */
    memcpy( ( void * )&log_data->fgc_regdump,    /*  V   S FG-IC                */
            ( void * )regdump,
            sizeof( log_data->fgc_regdump ));
    }                                            /*<3210076>                  */
    else                                         /*<3210076>                  */
    {                                            /*<3210076>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP | 0x0003 );/*<3210076>                */
        memset( ( void * )&log_data->fgc_regdump,/*<3210076>  S               */
                0xFF,                            /*<3210076>    0xFF          */
                sizeof( log_data->fgc_regdump ));/*<3210076>                  */
    }                                            /*<3210076>                  */

#if 1  // SlimID Add Start
    /*                                */
    memcpy((void *)log_data->charge_reg, (void *)fgc_ctl->charge_reg, DCTL_CHG_STSREG_MAX);
#endif // SlimID Add End

    log_data->fgc_regdump.mstate =
        ( unsigned short )( ( unsigned )((log_data->fgc_regdump.cntl_h) << 8)
                            | (log_data->fgc_regdump.cntl_l) );

    log_data->no = wp;                           /*     No.                   */
    wp++;                                        /*                           */
    if( wp >= DFGC_LOG_MAX )                     /*                           */
    {                                            /*                           */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP | 0x0001 );
        wp = DFGC_CLEAR;                         /*                           */
    }
                                                 /*                           */
    gfgc_ctl.remap_log_dump->fgc_log_data[ 0 ].log_wp = wp;

    spin_unlock_irqrestore( &gfgc_spinlock_log, flags ); /*  V                */

    /* DUMP                                    *//*<4210014>                  */
    if(( wp == DFGC_LOG_FILE_OUT1 ) ||           /*<4210014>                  */
       ( wp == DFGC_LOG_FILE_OUT2 ) ||           /*<4210014>                  */
       ( wp == DFGC_LOG_FILE_OUT3 ) ||           /*<4210014>                  */
       ( wp == DFGC_LOG_FILE_OUT4 ))             /*<4210014>                  */
    {                                            /*<4210014>                  */
        pfgc_log_dump_file(                      /*<4210014>  V               */
            gfgc_ctl.remap_log_dump );           /*<4210014>                  */
    }                                            /*<4210014>                  */

    MFGC_RDPROC_PATH( DFGC_LOG_DUMP | 0xFFFF );
}


/********************************************************************<4210014>*/
/*                pfgc_log_dump_file                                 <4210014>*/
/*                FG-IC                                              <4210014>*/
/*                2010.01.22                                         <4210014>*/
/*                NTTDMSE                                            <4210014>*/
/*                                                                   <4210014>*/
/*                                                                   <4210014>*/
/*                T_FGC_LOG   *ptr                                   <4210014>*/
/*                                                                   <4210014>*/
/********************************************************************<4210014>*/
static void pfgc_log_dump_file( T_FGC_LOG *ptr ) /*<4210014>                  */
{                                                /*<4210014>                  */
    const char * file[ DFGC_LOG_FILE_MAX + 1 ] = /*<4210014>                  */
    {                                            /*<4210014>      FILE  +1    */
        "/log/fgic/fgic01",                      /*                           */
        "/log/fgic/fgic02",
        "/log/fgic/fgic03",
        "/log/fgic/fgic04",
        "/log/fgic/fgic05",
        "/log/fgic/fgic06",
        "/log/fgic/fgic07",
        "/log/fgic/fgic08",
        "/log/fgic/fgic09",
        "/log/fgic/fgic10",
        "/log/fgic/fgic11",
        "/log/fgic/fgic12",
        "/log/fgic/fgic13",
        "/log/fgic/fgic14",
        "/log/fgic/fgic15",
        "/log/fgic/fgic16",
        "/log/fgic/fgic17",
        "/log/fgic/fgic18",
        "/log/fgic/fgic19",
        "/log/fgic/fgic20",
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/* conan Add-Start */
        "/log/fgic/fgic21",
        "/log/fgic/fgic22",
        "/log/fgic/fgic23",
        "/log/fgic/fgic24",
        "/log/fgic/fgic25",
        "/log/fgic/fgic26",
        "/log/fgic/fgic27",
        "/log/fgic/fgic28",
        "/log/fgic/fgic29",
        "/log/fgic/fgic30",
        "/log/fgic/fgic31",
        "/log/fgic/fgic32",
        "/log/fgic/fgic33",
        "/log/fgic/fgic34",
        "/log/fgic/fgic35",
        "/log/fgic/fgic36",
        "/log/fgic/fgic37",
        "/log/fgic/fgic38",
        "/log/fgic/fgic39",
        "/log/fgic/fgic40",
        "/log/fgic/fgic41",
        "/log/fgic/fgic42",
        "/log/fgic/fgic43",
        "/log/fgic/fgic44",
        "/log/fgic/fgic45",
        "/log/fgic/fgic46",
        "/log/fgic/fgic47",
        "/log/fgic/fgic48",
        "/log/fgic/fgic49",
        "/log/fgic/fgic50",
        "/log/fgic/fgic51",
        "/log/fgic/fgic52",
        "/log/fgic/fgic53",
        "/log/fgic/fgic54",
        "/log/fgic/fgic55",
        "/log/fgic/fgic56",
        "/log/fgic/fgic57",
        "/log/fgic/fgic58",
        "/log/fgic/fgic59",
        "/log/fgic/fgic60",
        "/log/fgic/fgic61",
        "/log/fgic/fgic62",
        "/log/fgic/fgic63",
        "/log/fgic/fgic64",
        "/log/fgic/fgic65",
        "/log/fgic/fgic66",
        "/log/fgic/fgic67",
        "/log/fgic/fgic68",
        "/log/fgic/fgic69",
        "/log/fgic/fgic70",
        "/log/fgic/fgic71",
        "/log/fgic/fgic72",
        "/log/fgic/fgic73",
        "/log/fgic/fgic74",
        "/log/fgic/fgic75",
        "/log/fgic/fgic76",
        "/log/fgic/fgic77",
        "/log/fgic/fgic78",
        "/log/fgic/fgic79",
        "/log/fgic/fgic80",
        "/log/fgic/fgic81",
        "/log/fgic/fgic82",
        "/log/fgic/fgic83",
        "/log/fgic/fgic84",
        "/log/fgic/fgic85",
        "/log/fgic/fgic86",
        "/log/fgic/fgic87",
        "/log/fgic/fgic88",
        "/log/fgic/fgic89",
        "/log/fgic/fgic90",
        "/log/fgic/fgic91",
        "/log/fgic/fgic92",
        "/log/fgic/fgic93",
        "/log/fgic/fgic94",
        "/log/fgic/fgic95",
        "/log/fgic/fgic96",
        "/log/fgic/fgic97",
        "/log/fgic/fgic98",
        "/log/fgic/fgic99",
/* conan Add-End */
#endif /*<Conan>1111XX*/
        "/log/fgic/count"                        /*                           */
    };                                           /*<4210014>                  */
    signed long          dir;                    /*<4210014>DIR               */
    signed long          manage;                 /*<4210014>              FD  */
    signed long          dat;                    /*<4210014>              FD  */
    signed long          os_ret;                 /*<4210014>                  */
    char                 buf[ 3 ];               /*<4210014>                  */
    unsigned int         log_num;                /*<4210014>                  */
    unsigned int         x, y;                   /*<4210014>                  */
    T_FGC_SYSERR_DATA    syserr;                 /*<4210014>SYS               */
                                                 /*<4210014>                  */
                                                 /*<4210014>                  */
    MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0000 );        /*<4210014>       */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    log_num  = 0;                                /*<4210014>                  */
    buf[ 0 ] = '0';                              /*<4210014>                  */
    buf[ 1 ] = '0';                              /*<4210014>                  */
    buf[ 2 ] = '\n';                             /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    dir = sys_mkdir( "/log/fgic",
                     0777);
    if(( dir < 0 ) && ( dir != -EEXIST ))        /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0001 );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x0001 );   /*<4210014>       */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x0001 ),     /*<4210014>                  */
            dir,                                 /*<4210014>                  */
            ( unsigned long )file,               /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    manage = sys_open(                           /*<4210014>  C               */
                 file[ DFGC_LOG_CNT_OFFSET ],    /*<4210014>                  */
                 ( O_RDWR | O_CREAT ),           /*<4210014>                  */
                 0777 );                         /*<4210014>R/W/X             */
    if( manage < 0 )                             /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0002 );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x0002 );   /*<4210014>       */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x0002 ),     /*<4210014>                  */
            manage,                              /*<4210014>                  */
            ( unsigned long )file,               /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_read( manage,                   /*<4210014>  C               */
                       buf,                      /*<4210014>                  */
                       2 );                      /*<4210014>                  */
    if( os_ret > 0 )                             /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0003 );    /*<4210014>       */
                                                 /*<4210014>                  */
        x = buf[ 0 ] - '0';                      /*<4210014>         (atoi)   */
        y = buf[ 1 ] - '0';                      /*<4210014>         (atoi)   */
        log_num = ( x * 10 ) + y;                /*<4210014>                  */
                                                 /*<4210014>                  */
        /*                                     *//*<4210014>                  */
        if( log_num >= DFGC_LOG_FILE_MAX )       /*<4210014>                  */
        {                                        /*<4210014>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0004 );/*<4210014>       */
                                                 /*<4210014>                  */
            log_num = 0;                         /*<4210014>                  */
        }                                        /*<4210014>                  */
    }                                            /*<4210014>                  */
    else if( os_ret == 0 )                       /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0005 );    /*<4210014>       */
                                                 /*<4210014>                  */
        log_num = 0;                             /*<4210014>                  */
    }                                            /*<4210014>                  */
    else                                         /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0006 );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x0006 );   /*<4210014>       */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x0006 ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            manage,                              /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    dat = sys_open(                              /*<4210014>  C               */
              file[ log_num ],                   /*<4210014>                  */
              ( O_RDWR | O_CREAT ),              /*<4210014>                  */
              0777 );                            /*<4210014>R/W/X             */
    if( dat < 0 )                                /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0007 );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x0007 );   /*<4210014>       */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x0007 ),     /*<4210014>                  */
            dat,                                 /*<4210014>                  */
            log_num,                             /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_lseek( dat, 0, SEEK_SET );      /*<4210014>  C               */
    if( os_ret < 0 )                             /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0008 );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x0008 );   /*<4210014>       */
        sys_close( dat );                        /*<4210014>  N               */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x0008 ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            dat,                                 /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_write(                          /*<4210014>  C               */
                 dat,                            /*<4210014>                  */
                 ( char * )ptr,                  /*<4210014>                  */
                 ( sizeof( T_FGC_LOG_DATA )      /*<4210014>                  */
                   * DFGC_LOG_MAX ));            /*<4210014>                  */
    if( os_ret !=                                /*<4210014>                  */
        ( sizeof( T_FGC_LOG_DATA )               /*<4210014>                  */
          * DFGC_LOG_MAX ))                      /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x0009 );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x0009 );   /*<4210014>       */
        sys_close( dat );                        /*<4210014>  N               */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x0009 ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            dat,                                 /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_lseek( manage, 0, SEEK_SET );   /*<4210014>  C               */
    if( os_ret < 0 )                             /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x000A );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x000A );   /*<4210014>       */
        sys_close( dat );                        /*<4210014>  N               */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x000A ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            manage,                              /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    log_num++;                                   /*<4210014>                  */
    if( log_num >= DFGC_LOG_FILE_MAX )           /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x000B );    /*<4210014>       */
                                                 /*<4210014>                  */
        log_num = 0;                             /*<4210014>                  */
    }                                            /*<4210014>                  */
    x = ( log_num / 10 ) + '0';                  /*<4210014>         (itoa)   */
    y = ( log_num % 10 ) + '0';                  /*<4210014>         (itoa)   */
    buf[ 0 ] = x;                                /*<4210014>                  */
    buf[ 1 ] = y;                                /*<4210014>                  */
    buf[ 2 ] ='\n';                              /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_write( manage,                  /*<4210014>  C               */
                        buf,                     /*<4210014>                  */
                        sizeof( buf ));          /*<4210014>                  */
    if( os_ret != sizeof( buf ))                 /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x000C );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x000C );   /*<4210014>       */
        sys_close( dat );                        /*<4210014>  N               */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x000C ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            manage,                              /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_close( dat );                   /*<4210014>  C               */
    if( os_ret < 0 )                             /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x000D );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x000D );   /*<4210014>       */
        sys_close( manage );                     /*<4210014>  N               */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x000D ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            dat,                                 /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    /*                                         *//*<4210014>                  */
    os_ret = sys_close( manage );                /*<4210014>                  */
    if( os_ret < 0 )                             /*<4210014>                  */
    {                                            /*<4210014>                  */
        MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0x000E );    /*<4210014>       */
        MFGC_RDPROC_ERROR( DFGC_LOG_DUMP_FILE | 0x000E );   /*<4210014>       */
        MFGC_SYSERR_DATA_SET2(                   /*<4210014>                  */
            CSYSERR_ALM_RANK_B,                  /*<4210014>                  */
            DFGC_SYSERR_KERNEL_ERR,              /*<4210014>                  */
            ( DFGC_LOG_DUMP_FILE | 0x000E ),     /*<4210014>                  */
            os_ret,                              /*<4210014>                  */
            manage,                              /*<4210014>                  */
            syserr );                            /*<4210014>                  */
        pfgc_log_syserr( &syserr );              /*<4210014>  V SYSERR        */
        return;                                  /*<4210014>                  */
    }                                            /*<4210014>                  */
                                                 /*<4210014>                  */
    MFGC_RDPROC_PATH( DFGC_LOG_DUMP_FILE | 0xFFFF );        /*<4210014>       */
                                                 /*<4210014>                  */
    return;                                      /*<4210014>                  */
}                                                /*<4210014>                  */

void pfgc_log_shutdown( void )
{
	FGC_FUNCIN("\n");

	if( gfgc_ctl.remap_log_dump != NULL )
	{
		FGC_DEBUG("Save log\n");
		pfgc_log_dump_file( gfgc_ctl.remap_log_dump );
		sys_sync();                              /*<npdc100324074>FGIC log 1112XX*/
	}

	FGC_FUNCOUT("\n");
}

/******************************************************************************/
/*                pfgc_log_ocv_init                                           */
/*                OCV                                                         */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pfgc_log_ocv_init( void )
{
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    MFGC_RDPROC_PATH( DFGC_LOG_OCV_INIT | 0x0000 );

                                                 /*                 WBB       */
    gfgc_ctl.remap_ocv = ioremap_cache( DFGC_SDRAM_OCV,   /*  C               */
                                        sizeof( T_FGC_OCV ),
                                        CACHE_WBB );

    if( gfgc_ctl.remap_ocv == NULL )            /* ioremap                    */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_OCV_INIT | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_LOG_OCV_INIT | 0x0001 );
        MFGC_SYSERR_DATA_SET2(
                    CSYSERR_ALM_RANK_A,
                    DFGC_SYSERR_IOREMAP_ERR,
                    ( DFGC_LOG_OCV_INIT | 0x0001 ),
                    DFGC_SDRAM_OCV,
                    sizeof( T_FGC_OCV ),
                    syserr_info );
        pfgc_log_syserr( &syserr_info );        /*  V SYSERR                  */
    }
                                                /*<PCIO033>                   */
    gfgc_ctl.ocv_okng[ 0 ] = DFGC_CLEAR;        /*<PCIO033> OCV               */
    gfgc_ctl.ocv_okng[ 1 ] = DFGC_CLEAR;        /*<PCIO033> OCV               */
    gfgc_ctl.ocv_okng[ 2 ] = DFGC_CLEAR;        /*<PCIO033> OCV               */

    gfgc_ctl.timestamp = pfgc_log_time();

    MFGC_RDPROC_PATH( DFGC_LOG_OCV_INIT | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_log_ocv                                                */
/*                OCV                                                         */
/*                2008.10.21                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
int pfgc_log_ocv( void )
{
    signed int      ret;                        /*                            */
    unsigned short  count;                      /* OCV                        */
    T_FGC_OCV_DATA* fgc_ocv_data;               /* OCV                        */
    unsigned short  save_cnt;                   /* OCV                        */
    unsigned long   index;                      /* OCV                  index */
    unsigned char   ocv_okng[ DFGC_OCV_SAVE_MAX ]; /* OCV                     */
    unsigned long   ocv_time[ DFGC_OCV_SAVE_MAX ]; /*                         */
    unsigned short  ocv_temp[ DFGC_OCV_SAVE_MAX ]; /* OCV        TEMP         */
    unsigned short  ocv_volt[ DFGC_OCV_SAVE_MAX ]; /* OCV        VOLT         */
    unsigned short  ocv_nac[ DFGC_OCV_SAVE_MAX ];  /* OCV        NAC          */
    unsigned short  ocv_fac[ DFGC_OCV_SAVE_MAX ];  /* OCV        FAC          */
    unsigned short  ocv_rm[ DFGC_OCV_SAVE_MAX ];   /* OCV        RM           */
    unsigned short  ocv_fcc[ DFGC_OCV_SAVE_MAX ];  /* OCV        FCC          */
    unsigned short  ocv_soc[ DFGC_OCV_SAVE_MAX ];  /* OCV        SOC          */
    T_FGC_SYSERR_DATA syserr_info;              /* SYS                        */

    MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0x0000 );

	DEBUG_FGIC(("pfgc_log_ocv start\n"));

                                                /*                            */
    save_cnt     = DFGC_CLEAR;
    fgc_ocv_data = gfgc_ctl.remap_ocv->fgc_ocv_data;
    ret          = DFGC_CLEAR;

                                                /*                            */
    ret |= Hcm_romdata_read_knl(                /*  C OCV                     */
                    D_HCM_FG_OCV_OKNG,
                    0,
                    &ocv_okng[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_TIMESTAMP,
                    0,
                    ( unsigned char* )&ocv_time[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_TEMP,
                    0,
                    ( unsigned char* )&ocv_temp[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_VOLT,
                    0,
                    ( unsigned char* )&ocv_volt[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_NAC,
                    0,
                    ( unsigned char* )&ocv_nac[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_FAC,
                    0,
                    ( unsigned char* )&ocv_fac[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_RM,
                    0,
                    ( unsigned char* )&ocv_rm[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                           */
                    D_HCM_FG_OCV_FCC,
                    0,
                    ( unsigned char* )&ocv_fcc[0] );

    ret |= Hcm_romdata_read_knl(                /*  C                         */
                    D_HCM_FG_OCV_SOC,
                    0,
                    ( unsigned char* )&ocv_soc[0] );

    ret |= Hcm_romdata_read_knl(                /*  C OCV                     */
                    D_HCM_FG_OCV_COUNT,
                    0,
                    ( unsigned char* )&count );

    if( ret < 0 )                               /*                            */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_LOG_OCV | 0x0001 );
        MFGC_SYSERR_DATA_SET(
                    CSYSERR_ALM_RANK_B,
                    DFGC_SYSERR_NVM_ERR,
                    ( DFGC_LOG_OCV | 0x0001 ),
                    ret,
                    syserr_info );
        pfgc_log_syserr( &syserr_info );        /*  V SYSERR                  */
        return ret;
    }

    if( count == DFGC_OCV_SAVE_CNT_MAX )        /* OCV                        */
    {
        index = 0;
    }
    else
    {
/*<3131345>        index = count + 1;                      *//*                            */
/*<3131345>        index = index % DFGC_OCV_SAVE_MAX;*/
        index = count % DFGC_OCV_SAVE_MAX;      /*<3131345>                   */
    }

                                                /* OCV                        */
    while( fgc_ocv_data[ save_cnt ].ocv != DFGC_OFF ) /*  S                   */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0x0002 );
        ocv_okng[ index ]                       /* OCV                        */
            = fgc_ocv_data[ save_cnt ].ocv;
        gfgc_ctl.ocv_okng[ save_cnt ]           /*<PCIO033> OCV               */
            = fgc_ocv_data[ save_cnt ].ocv;     /*<PCIO033>                   */
#if 1 /* IDPower npdc300019622 */
        ocv_time[ index ] = fgc_ocv_data[ save_cnt ].ocv_timestamp;
#else
        fgc_ocv_data[ save_cnt ].ocv_timestamp  /*                            */
            = gfgc_ctl.timestamp;
        ocv_time[ index ] = gfgc_ctl.timestamp; /*                            */
#endif
        ocv_temp[ index ]                       /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_temp;
        ocv_volt[ index ]                       /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_volt;
        ocv_nac[ index ]                        /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_nac;
        ocv_fac[ index ]                        /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_fac;
        ocv_rm[ index ]                         /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_rm;
        ocv_fcc[ index ]                        /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_fcc;
        ocv_soc[ index ]                        /*                            */
            = fgc_ocv_data[ save_cnt ].ocv_soc;

                                                /*                            */
        if( save_cnt >= ( DFGC_OCV_SAVE_MAX - 1 ))
        {
            MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0x0003 );
            break;
        }

        save_cnt++;
        index++;
        if( index >= DFGC_OCV_SAVE_MAX )        /*                            */
        {
            MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0x0004 );
            index = DFGC_CLEAR;
        }
    }

    ret |= Hcm_romdata_write_knl(               /*  C OCV                     */
                D_HCM_FG_OCV_OKNG,
                0,
                ( unsigned char* )&ocv_okng[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_TIMESTAMP,
                0, ( unsigned char* )&ocv_time[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_TEMP,
                0,
                ( unsigned char* )&ocv_temp[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_VOLT,
                0,
                ( unsigned char* )&ocv_volt[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_NAC,
                0,
                ( unsigned char* )&ocv_nac[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_FAC,
                0,
                ( unsigned char* )&ocv_fac[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_RM,
                0,
                ( unsigned char* )&ocv_rm[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                           */
                D_HCM_FG_OCV_FCC,
                0,
                ( unsigned char* )&ocv_fcc[0] );

    ret |= Hcm_romdata_write_knl(               /*  C                         */
                D_HCM_FG_OCV_SOC,
                0,
                ( unsigned char* )&ocv_soc[0] );

    if( count >=                                /*                            */
        ( unsigned short )(DFGC_OCV_SAVE_CNT_MAX - save_cnt ))
    {
        count = DFGC_OCV_SAVE_CNT_MAX;          /*           MAX              */
    }
    else
    {
        count += save_cnt;                      /*                            */
    }

    ret |= Hcm_romdata_write_knl(               /*  C OCV                     */
                D_HCM_FG_OCV_COUNT,
                0,
                ( unsigned char* )&count );

    if( ret < 0 )                               /*                            */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0x0005 );
        MFGC_RDPROC_ERROR( DFGC_LOG_OCV | 0x0005 );
        MFGC_SYSERR_DATA_SET(
                    CSYSERR_ALM_RANK_B,
                    DFGC_SYSERR_NVM_ERR,
                    ( DFGC_LOG_OCV | 0x0005 ),
                    ret,
                    syserr_info );
        pfgc_log_syserr( &syserr_info );        /*  V SYSERR                  */
        return ret;
    }

    MFGC_RDPROC_PATH( DFGC_LOG_OCV | 0xFFFF );
    return ret;
}

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/******************************************************************************/
/*                pfgc_log_health                                             */
/*                                                                            */
/*                2008.10.21                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*<Conan>*//*                unsigned char health                             */
/*                unsigned char force_flg                                     *//*<Conan>*/
/*                                                                            */
/******************************************************************************/
/*<Conan>void pfgc_log_health( unsigned char health )*/
void pfgc_log_health( unsigned char force_flg )                                 /*<Conan>*/
{
/*<Conan>#if DFGC_SW_DELETED_FUNC*/
/*<Conan>    unsigned long timestamp[ DFGC_WORSE_MAX ];*/  /*                            */
    signed long   ret;                          /*                            */
/*<Conan>    unsigned char index;*/                        /*                            */
    T_FGC_SOH     *t_soh;                       /*                            *//*<Conan>*/
    T_FGC_TIME_DIFF_INFO time_diff;             /*                            *//*<Conan>*/
    T_FGC_SYSERR_DATA syserr_info;              /* SYS                        */

    MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0000 );

    t_soh = gfgc_ctl.remap_soh;                 /*                            *//*<Conan>*/

/*<Conan>    index = DFGC_WORSE_MAX;*/                     /*               MAX          */

/*<Conan>   switch( health ) */                           /*                            */
/*<Conan>    {*/
/*<Conan>        case DFGC_HEALTH_MEASURE:*/               /*                            */
/*<Conan>        case DFGC_HEALTH_GOOD:*/                  /*                            */
/*<Conan>            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0001 );*/
/*<Conan>            break;*/
/*<3131266>            break;*/
/*<Conan>            return;*/                             /*<3131266>                   */

/*<Conan>        case DFGC_HEALTH_DOWN:*/                  /*                            */
/*<Conan>            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0002 );*/
/*<Conan>            if( gfgc_ctl.health_old             *//*                            */
/*<Conan>                        != DFGC_HEALTH_DOWN )*/
/*<Conan>            {*/
/*<Conan>                MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0003 );*/
/*<Conan>                index = DFGC_WORSE_DOWN;*/
/*<Conan>            }*/

/*<Conan>*/                                                /* break                      */
/*<Conan>            break;*/                              /*<3131266>index              */

/*<Conan>        case DFGC_HEALTH_BAD:*/                   /*                            */
/*<Conan>            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0004 );*/
/*<Conan>            if( gfgc_ctl.health_old*/             /*                            */
/*<3131266>                        != DFGC_WORSE_BAD )*/
/*<Conan>                        != DFGC_HEALTH_BAD )*/    /*<3131266>                   */
/*<Conan>            {*/
/*<Conan>                MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0005 );*/
/*<Conan>                index = DFGC_WORSE_BAD;*/
/*<Conan>            }*/
/*<Conan>            break;*/                              /*<3131266>index              */
                                                /*<3131266>        switch     */
/*<Conan>        default:*/                                /*<3131266>                   */
/*<Conan>            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0009 );*/ /*<3131266>         */
/*<Conan>            return;*/                             /*<3131266>                   */
/*<Conan>    }*/                                           /*<3131266>                   */

                                                /*                            *//*<Conan>*/
    
                                                /*                            *//*<Conan>*/
                                                /*                            *//*<Conan>*/

    time_diff = pfgc_log_time_diff( t_soh->soh_degra_save_monotonic_timestamp,  /*<Conan>*/
                                    t_soh->soh_monotonic_timestamp);            /*<Conan>*/
                                    
    if(( time_diff.min >= DFGC_FGC_SAVE_TIME ) ||/*               24H         *//*<Conan>*/
           ( force_flg == DFGC_ON ))             /*                           *//*<Conan>*/
    {                                            /*                           *//*<Conan>*/
        ret = Hcm_romdata_write_knl(            /*  C                         *//*<Conan>*/
                D_HCM_E_H24_SAVE,                                               /*<Conan>*/
                0,                                                              /*<Conan>*/
                ( unsigned char* )&t_soh->degra_batt_measure_val );             /*<Conan>*/
        if( ret < 0 )                           /*                            *//*<Conan>*/
        {                                                                       /*<Conan>*/
            MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x0009 );                      /*<Conan>*/
            MFGC_SYSERR_DATA_SET2(                                              /*<Conan>*/
                    CSYSERR_ALM_RANK_B,                                         /*<Conan>*/
                    DFGC_SYSERR_NVM_ERR,                                        /*<Conan>*/
                    ( DFGC_LOG_HEALTH | 0x0009 ),                               /*<Conan>*/
                    D_HCM_E_H24_SAVE,                                           /*<Conan>*/
                    ret,                                                        /*<Conan>*/
                    syserr_info );                                              /*<Conan>*/
            pfgc_log_syserr( &syserr_info );    /*  V SYSERR                  *//*<Conan>*/
        }                                                                       /*<Conan>*/

                                                /*                            *//*<Conan>*/
        ret = Hcm_romdata_write_knl(            /*  C                         *//*<Conan>*/
                D_HCM_E_H24_SAVE_TSTAMP,                                        /*<Conan>*/
                0,                                                              /*<Conan>*/
                ( unsigned char* )&t_soh->soh_system_timestamp );                 /*<Conan>*/
        if( ret < 0 )                           /*                            *//*<Conan>*/
        {                                                                       /*<Conan>*/
            MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x000A );                      /*<Conan>*/
            MFGC_SYSERR_DATA_SET2(                                              /*<Conan>*/
                    CSYSERR_ALM_RANK_B,                                         /*<Conan>*/
                    DFGC_SYSERR_NVM_ERR,                                        /*<Conan>*/
                    ( DFGC_LOG_HEALTH | 0x000A ),                               /*<Conan>*/
                    D_HCM_E_H24_SAVE_TSTAMP,                                    /*<Conan>*/
                    ret,                                                        /*<Conan>*/
                    syserr_info );                                              /*<Conan>*/
            pfgc_log_syserr( &syserr_info );        /*  V SYSERR              *//*<Conan>*/
        }                                                                       /*<Conan>*/
                                                /* SOC                        *//*<Conan>*/
        ret = Hcm_romdata_write_knl(            /*  C                         *//*<Conan>*/
                D_HCM_E_H24_SAVE_SOC,                                           /*<Conan>*/
                0,                                                              /*<Conan>*/
            ( void * )&gfgc_ctl.remap_soh->soc_total );                         /*<Conan>*/
        if( ret < 0 )                           /*                            *//*<Conan>*/
        {                                                                       /*<Conan>*/
            MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x000B );                      /*<Conan>*/
            MFGC_SYSERR_DATA_SET2(                                              /*<Conan>*/
                    CSYSERR_ALM_RANK_B,                                         /*<Conan>*/
                    DFGC_SYSERR_NVM_ERR,                                        /*<Conan>*/
                    ( DFGC_LOG_HEALTH | 0x000B ),                               /*<Conan>*/
                    D_HCM_E_H24_SAVE_SOC,                                       /*<Conan>*/
                    ret,                                                        /*<Conan>*/
                    syserr_info );                                              /*<Conan>*/
            pfgc_log_syserr( &syserr_info );        /*  V SYSERR              *//*<Conan>*/
        }                                                                       /*<Conan>*/
                                                                                /*<Conan>*/
        gfgc_ctl.save_soh_degra_timestamp =     /*                            *//*<Conan>*/
            t_soh->soh_system_timestamp;        /*                            *//*<Conan>*/

        t_soh->soh_degra_save_monotonic_timestamp =/*                           *//*<Conan>*/
            t_soh->soh_monotonic_timestamp;        /*                           *//*<Conan>*/

    }                                                                           /*<Conan>*/

                                                /*          50%               *//*<Conan>*/
                                                /*                            *//*<Conan>*/
    if(( t_soh->degra_batt_estimate_val < DFGC_FGC_SAVE_THRES ) &&              /*<Conan>*/
       ( t_soh->soh_thres_timestamp == DFGC_SOH_VAL_INIT))                      /*<Conan>*/
    {                                                                           /*<Conan>*/
                                                /*                            *//*<Conan>*/
        ret = Hcm_romdata_read_knl(          /*  C                            *//*<Conan>*/
                D_HCM_E_DEGRADA_BL50_SAVE_TSTAMP,                               /*<Conan>*/
                0,                                                              /*<Conan>*/
                ( void * )&gfgc_ctl.save_soh_thres_timestamp );                 /*<Conan>*/
        if( ret < 0 )                           /*                            *//*<Conan>*/
        {                                                                       /*<Conan>*/
            MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x000C );                      /*<Conan>*/
            MFGC_SYSERR_DATA_SET2(                                              /*<Conan>*/
                    CSYSERR_ALM_RANK_B,                                         /*<Conan>*/
                    DFGC_SYSERR_NVM_ERR,                                        /*<Conan>*/
                    ( DFGC_LOG_HEALTH | 0x000C ),                               /*<Conan>*/
                    D_HCM_E_DEGRADA_BL50_SAVE_TSTAMP,                           /*<Conan>*/
                    ret,                                                        /*<Conan>*/
                    syserr_info );                                              /*<Conan>*/
            pfgc_log_syserr( &syserr_info );    /*  V SYSERR                  *//*<Conan>*/
        }                                                                       /*<Conan>*/
        else                                                                    /*<Conan>*/
        {                                                                       /*<Conan>*/
            if( gfgc_ctl.save_soh_thres_timestamp !=                            /*<Conan>*/
                    DFGC_SOH_VAL_INIT )                                         /*<Conan>*/
            {                                                                   /*<Conan>*/
                t_soh->soh_thres_timestamp =    /*           50%              *//*<Conan>*/
                    gfgc_ctl.save_soh_thres_timestamp;  /*                    *//*<Conan>*/
                /*                            */                                /*<Conan>*/
                return;                                                         /*<Conan>*/
            }                                                                   /*<Conan>*/
                                                                                /*<Conan>*/
        }                                                                       /*<Conan>*/
                                                /*                            *//*<Conan>*/
        ret = Hcm_romdata_write_knl(            /*  C                         *//*<Conan>*/
                D_HCM_E_DEGRADA_BL50,                                           /*<Conan>*/
                0,                                                              /*<Conan>*/
                ( unsigned char* )&t_soh->degra_batt_measure_val );             /*<Conan>*/
        if( ret < 0 )                           /*                            *//*<Conan>*/
        {                                                                       /*<Conan>*/
            MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x000D );                      /*<Conan>*/
            MFGC_SYSERR_DATA_SET2(                                              /*<Conan>*/
                    CSYSERR_ALM_RANK_B,                                         /*<Conan>*/
                    DFGC_SYSERR_NVM_ERR,                                        /*<Conan>*/
                    ( DFGC_LOG_HEALTH | 0x000D ),                               /*<Conan>*/
                    D_HCM_E_DEGRADA_BL50,                                       /*<Conan>*/
                    ret,                                                        /*<Conan>*/
                    syserr_info );                                              /*<Conan>*/
            pfgc_log_syserr( &syserr_info );    /*  V SYSERR                  *//*<Conan>*/
        }                                                                       /*<Conan>*/
                                                /*                            *//*<Conan>*/
        ret = Hcm_romdata_write_knl(            /*  C                         *//*<Conan>*/
                D_HCM_E_DEGRADA_BL50_SAVE_TSTAMP,                               /*<Conan>*/
                0,                                                              /*<Conan>*/
                ( unsigned char* )&t_soh->soh_system_timestamp );                 /*<Conan>*/
        if( ret < 0 )                           /*                            *//*<Conan>*/
        {                                                                       /*<Conan>*/
            MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x000E );                      /*<Conan>*/
            MFGC_SYSERR_DATA_SET2(                                              /*<Conan>*/
                    CSYSERR_ALM_RANK_B,                                         /*<Conan>*/
                    DFGC_SYSERR_NVM_ERR,                                        /*<Conan>*/
                    ( DFGC_LOG_HEALTH | 0x000E ),                               /*<Conan>*/
                    D_HCM_E_DEGRADA_BL50_SAVE_TSTAMP,                           /*<Conan>*/
                    ret,                                                        /*<Conan>*/
                    syserr_info );                                              /*<Conan>*/
            pfgc_log_syserr( &syserr_info );    /*  V SYSERR                  *//*<Conan>*/
        }                                                                       /*<Conan>*/

        t_soh->soh_thres_timestamp =              /*           50%            *//*<Conan>*/
            t_soh->soh_system_timestamp;          /*                          *//*<Conan>*/

    }                                                                           /*<Conan>*/
/*<Conan>            if( index != DFGC_WORSE_MAX )*/       /*               MAX          */
/*<Conan>             {*/
/*<Conan>                 MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0006 );*/
/*<Conan>                 ret = Hcm_romdata_read_knl(*/     /*  C                            */
/*<Conan>                         D_HCM_FG_WORSE_TIMESTAMP,*/
/*<Conan>                         0,*/
/*<Conan>                         ( unsigned char* )timestamp );*/
/*<Conan>                 if( ret < 0 )*/                   /*                            */
/*<Conan>                 {*/
/*<Conan>                     MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0007 );*/
/*<Conan>                     MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x0007 );*/
/*<Conan>                     MFGC_SYSERR_DATA_SET2(*/
/*<Conan>                             CSYSERR_ALM_RANK_B,*/
/*<Conan>                             DFGC_SYSERR_NVM_ERR,*/
/*<Conan>                             ( DFGC_LOG_HEALTH | 0x0007 ),*/
/*<Conan>                             D_HCM_FG_WORSE_TIMESTAMP,*/
/*<Conan>                             ret,*/
/*<Conan>                             syserr_info );*/
/*<Conan>                     pfgc_log_syserr( &syserr_info );*//*  V SYSERR              */
/*<Conan>                 }*/

/*<Conan>*/                                                 /*                            */
/*<Conan>                 timestamp[ index ] = pfgc_log_time();*/

/*<Conan>*/                                                 /*                            */
/*<Conan>                 ret = Hcm_romdata_write_knl(*/    /*  C                         */
/*<Conan>                         D_HCM_FG_WORSE_TIMESTAMP,*/
/*<Conan>                         0,*/
/*<Conan>                         ( unsigned char* )timestamp );*/
/*<Conan>                 if( ret < 0 )*/                   /*                            */
/*<Conan>                 {*/
/*<Conan>                     MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0008 );*/
/*<Conan>                     MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x0008 );*/
/*<Conan>                     MFGC_SYSERR_DATA_SET2(*/
/*<Conan>                             CSYSERR_ALM_RANK_B,*/
/*<Conan>                             DFGC_SYSERR_NVM_ERR,*/
/*<Conan>                             ( DFGC_LOG_HEALTH | 0x0008 ),*/
/*<Conan>                             D_HCM_FG_WORSE_TIMESTAMP,*/
/*<Conan>                             ret,*/
/*<Conan>                             syserr_info );*/
/*<Conan>                     pfgc_log_syserr( &syserr_info );*//*  V SYSERR              */
/*<Conan>                 }*/
/*<Conan>             }*/
/*<3131266>            break;*/

/*<3131266>        default:*/
/*<3131266>            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0009 );*/
/*<3131266>            break;*/
/*<3131266>    }*/

    MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0xFFFF );

/*<Conan>#endif *//* DFGC_SW_DELETED_FUNC */
}
#else /*<Conan>1111XX*/
/******************************************************************************/
/*                pfgc_log_health                                             */
/*                                                                            */
/*                2008.10.21                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                unsigned char health                                        */
/*                                                                            */
/******************************************************************************/
void pfgc_log_health( unsigned char health )
{
#if DFGC_SW_DELETED_FUNC
    unsigned long timestamp[ DFGC_WORSE_MAX ];  /*                            */
    signed long   ret;                          /*                            */
    unsigned char index;                        /*                            */
    T_FGC_SYSERR_DATA syserr_info;              /* SYS                        */

    MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0000 );

    index = DFGC_WORSE_MAX;                     /*               MAX          */

    switch( health )                            /*                            */
    {
        case DFGC_HEALTH_MEASURE:               /*                            */
        case DFGC_HEALTH_GOOD:                  /*                            */
            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0001 );
/*<3131266>            break;*/
            return;                             /*<3131266>                   */

        case DFGC_HEALTH_DOWN:                  /*                            */
            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0002 );
            if( gfgc_ctl.health_old             /*                            */
                        != DFGC_HEALTH_DOWN )
            {
                MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0003 );
                index = DFGC_WORSE_DOWN;
            }
                                                /* break                      */
            break;                              /*<3131266>index              */

        case DFGC_HEALTH_BAD:                   /*                            */
            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0004 );
            if( gfgc_ctl.health_old             /*                            */
/*<3131266>                        != DFGC_WORSE_BAD )*/
                        != DFGC_HEALTH_BAD )    /*<3131266>                   */
            {
                MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0005 );
                index = DFGC_WORSE_BAD;
            }
            break;                              /*<3131266>index              */
                                                /*<3131266>        switch     */
        default:                                /*<3131266>                   */
            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0009 ); /*<3131266>         */
            return;                             /*<3131266>                   */
    }                                           /*<3131266>                   */

            if( index != DFGC_WORSE_MAX )       /*               MAX          */
            {
                MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0006 );
                ret = Hcm_romdata_read_knl(     /*  C                            */
                        D_HCM_FG_WORSE_TIMESTAMP,
                        0,
                        ( unsigned char* )timestamp );
                if( ret < 0 )                   /*                            */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0007 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x0007 );
                    MFGC_SYSERR_DATA_SET2(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_HEALTH | 0x0007 ),
                            D_HCM_FG_WORSE_TIMESTAMP,
                            ret,
                            syserr_info );
                    pfgc_log_syserr( &syserr_info );/*  V SYSERR              */
                }

                                                /*                            */
                timestamp[ index ] = pfgc_log_time();

                                                /*                            */
                ret = Hcm_romdata_write_knl(    /*  C                         */
                        D_HCM_FG_WORSE_TIMESTAMP,
                        0,
                        ( unsigned char* )timestamp );
                if( ret < 0 )                   /*                            */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0008 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_HEALTH | 0x0008 );
                    MFGC_SYSERR_DATA_SET2(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_HEALTH | 0x0008 ),
                            D_HCM_FG_WORSE_TIMESTAMP,
                            ret,
                            syserr_info );
                    pfgc_log_syserr( &syserr_info );/*  V SYSERR              */
                }
            }
/*<3131266>            break;*/

/*<3131266>        default:*/
/*<3131266>            MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0x0009 );*/
/*<3131266>            break;*/
/*<3131266>    }*/

    MFGC_RDPROC_PATH( DFGC_LOG_HEALTH | 0xFFFF );
#endif /* DFGC_SW_DELETED_FUNC */
}
#endif /*<Conan>1111XX*/

/******************************************************************************/
/*                pfgc_log_charge                                             */
/*                                                                            */
/*                2008.10.23                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                unsigned char type                                          */
/*                T_FGC_REGDUMP *regdump                                      */
/*                                                                            */
/******************************************************************************/
void pfgc_log_charge( unsigned char type, T_FGC_REGDUMP *regdump )
{
    signed int     ret;                          /*                           */
    unsigned long  l_time;                       /*                           */
    unsigned short first_time;                   /*                           */
    unsigned short count;                        /*                           */
    unsigned long  index;                        /*                     index */
    unsigned long  chg_time[ DFGC_LOG_CHG_MAX ]; /*                           */
    unsigned short chg_fcc[ DFGC_LOG_CHG_MAX ];  /*           FCC             */
    unsigned short chg_soc[ DFGC_LOG_CHG_MAX ];  /*           SOC             */
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    FGC_FUNCIN("\n");

     MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0000 );

/*<1905359>    switch( type )                                                 */
    if(( type & D_FGC_CHG_INT_IND )                    /*<1905359>            */
       == D_FGC_CHG_INT_IND )                          /*<1905359>            */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x000A );  /*<1905359>            */
/*<1905359>        case D_FGC_CHG_INT_IND:     *//*                           */
// Jessie del            if( gfgc_ctl.chg_sts                 /*                             */
            if( gfgc_chgsts_thread               /*                             */ // Jessie add
                        == DCTL_CHG_CHARGE_FIN )
            {
        	MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0001 );

                l_time = pfgc_log_time();        /*  C                        */

                                                 /*                           */
                ret = Hcm_romdata_read_knl(      /*  C                        */
                        D_HCM_CHARGE_FIN_1STTIME,
                        0,
                        ( unsigned char* )&first_time );
                if( ret < 0 )                    /*                           */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0002 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0002 );
                    MFGC_SYSERR_DATA_SET2(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_CHARGE | 0x0002 ),
                            D_HCM_CHARGE_FIN_1STTIME,
                            ret,
                            syserr_info );
                    pfgc_log_syserr( &syserr_info );/*  V SYSERR              */
/*<1905359>                    break;                                         */
                    return;                         /*<1905359>               */
                }
                else if( first_time == DFGC_CLEAR ) /*                        */
                {

                                                 /*                           */
                    first_time = ( unsigned short )( l_time >> DFGC_DAY_SHIFT );
                    ret = Hcm_romdata_write_knl(     /*  C                    */
                            D_HCM_CHARGE_FIN_1STTIME,
                            0,
                            ( unsigned char* )&first_time );
                    if( ret < 0 )                /*                           */
                    {
                        MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0004 );
                        MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0004 );
                        MFGC_SYSERR_DATA_SET2(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_CHARGE | 0x0004 ),
                            D_HCM_CHARGE_FIN_1STTIME,
                            ret,
                            syserr_info );
                        pfgc_log_syserr( &syserr_info );/*  V SYSERR          */
                    }
                }
                else                             /*                           */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0005 );
                }

                                                 /*                           */
                ret  = Hcm_romdata_read_knl(     /*  C                        */
                            D_HCM_CHARGE_FIN_TIMESTAMP,
                            0,
                            ( unsigned char* )&chg_time[ 0 ] );
                ret |= Hcm_romdata_read_knl(     /*  C                        */
                            D_HCM_CHARGE_FIN_FCC,
                            0,
                            ( unsigned char* )&chg_fcc[ 0 ] );
                ret |= Hcm_romdata_read_knl(     /*  C                        */
                            D_HCM_CHARGE_FIN_SOC,
                            0,
                            ( unsigned char* )&chg_soc[ 0 ] );
                ret |= Hcm_romdata_read_knl(     /*  C                        */
                            D_HCM_CHARGE_FIN_COUNT,
                            0,
                            ( unsigned char* )&count );
                if( ret < 0 )                    /*                           */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0006 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0006 );
                    MFGC_SYSERR_DATA_SET(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_CHARGE | 0x0006 ),
                            ret,
                            syserr_info );
                    pfgc_log_syserr( &syserr_info );/*  V SYSERR              */
/*<1905359>                    break;                                         */
                    return;                         /*<1905359>               */
                }


                                                 /*                           */
                index = ( unsigned long )( count % DFGC_LOG_CHG_MAX );

                chg_time[ index ] = l_time;      /*                           */
                chg_fcc[ index ]                 /*                           */
/*<3210076>                            = ( unsigned short )regdump->fcc_l;    */
                            = gfgc_batt_info.fcc;/*<3210076> regdump          */
                chg_soc[ index ]                 /*                           */
/*<3210076>                            = ( unsigned short )regdump->soc_l;    */
                            = gfgc_batt_info.soc;/*<3210076> regdump          */

                ret  = Hcm_romdata_write_knl(    /*  C                        */
                            D_HCM_CHARGE_FIN_TIMESTAMP,
                            0,
                            ( unsigned char* )&chg_time[ 0 ] );
                ret |= Hcm_romdata_write_knl(    /*  C                        */
                            D_HCM_CHARGE_FIN_FCC,
                            0,
                            ( unsigned char* )&chg_fcc[ 0 ]);
                ret |= Hcm_romdata_write_knl(    /*  C                        */
                            D_HCM_CHARGE_FIN_SOC,
                            0,
                            ( unsigned char* )&chg_soc[ 0 ] );

                if( count >= DFGC_LOG_CHG_CNT_MAX )   /*                      */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0007 );
                    count = DFGC_CLEAR;          /*                           */
                }
                else
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0008 );
                    count++;                     /*                           */
                }

                ret |= Hcm_romdata_write_knl(      /*  C                  */
                            D_HCM_CHARGE_FIN_COUNT,
                            0,
                            ( unsigned char* )&count );
                if( ret < 0 )                /*                           */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0009 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0009 );
                    MFGC_SYSERR_DATA_SET(
                                CSYSERR_ALM_RANK_B,
                                DFGC_SYSERR_NVM_ERR,
                                ( DFGC_LOG_CHARGE | 0x0009 ),
                                ret,
                                syserr_info );
                    pfgc_log_syserr( &syserr_info );    /*  V SYSERR      */
/*<1905359>                    break;                                         */
                    return;                         /*<1905359>               */
                }
            }
// Jessie add start
            else if( gfgc_chgsts_thread               /*  */
                        == DCTL_CHG_WIRELESS_CHG_FIN )
            {
                MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0010 );

                l_time = pfgc_log_time();        /*           */

                                                 /*         */
                ret = Hcm_romdata_read_knl(      /*     */
                        D_HCM_WIRELESSCHARGE_FIN_1STTIME,
                        0,
                        ( unsigned char* )&first_time );

                if( ret < 0 )                    /*         */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0020 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0020 );
                    MFGC_SYSERR_DATA_SET2(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_CHARGE | 0x0020 ),
                            D_HCM_WIRELESSCHARGE_FIN_1STTIME,
                            ret,
                            syserr_info );
                    pfgc_log_syserr( &syserr_info );/*          */
                    return;
                }
                else if( first_time == DFGC_CLEAR ) /*          */
                {

                                                 /*           */
                    first_time = ( unsigned short )( l_time >> DFGC_DAY_SHIFT );
                    ret = Hcm_romdata_write_knl(     /*     */
                            D_HCM_WIRELESSCHARGE_FIN_1STTIME,
                            0,
                            ( unsigned char* )&first_time );
                    if( ret < 0 )                /*         */
                    {
                        MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0030 );
                        MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0030 );
                        MFGC_SYSERR_DATA_SET2(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_CHARGE | 0x0030 ),
                            D_HCM_WIRELESSCHARGE_FIN_1STTIME,
                            ret,
                            syserr_info );
                        pfgc_log_syserr( &syserr_info );/*     */
                    }
                }
                else                             /*     */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0040 );
                }

                                                 /*       */
                ret  = Hcm_romdata_read_knl(     /*        */
                            D_HCM_WIRELESSCHARGE_FIN_TIMESTAMP,
                            0,
                            ( unsigned char* )&chg_time[ 0 ] );
                ret |= Hcm_romdata_read_knl(     /* */
                            D_HCM_WIRELESSCHARGE_FIN_FCC,
                            0,
                            ( unsigned char* )&chg_fcc[ 0 ] );
                ret |= Hcm_romdata_read_knl(     /*              */
                            D_HCM_WIRELESSCHARGE_FIN_SOC,
                            0,
                            ( unsigned char* )&chg_soc[ 0 ] );
                ret |= Hcm_romdata_read_knl(     /*           */
                            D_HCM_WIRELESSCHARGE_FIN_COUNT,
                            0,
                            ( unsigned char* )&count );
                if( ret < 0 )                    /*         */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0050 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0050 );
                    MFGC_SYSERR_DATA_SET(
                            CSYSERR_ALM_RANK_B,
                            DFGC_SYSERR_NVM_ERR,
                            ( DFGC_LOG_CHARGE | 0x0050 ),
                            ret,
                            syserr_info );
                    pfgc_log_syserr( &syserr_info );/*          */
                    return;
                }

                                                 /*         */
                index = ( unsigned long )( count % DFGC_LOG_CHG_MAX );

                chg_time[ index ] = l_time;      /*                       */
                chg_fcc[ index ]                 /*     */
                            = gfgc_batt_info.fcc;/*<3210076> regdump  */
                chg_soc[ index ]                 /*                   */
                            = gfgc_batt_info.soc;/*<3210076> regdump  */

                ret  = Hcm_romdata_write_knl(    /*         */
                            D_HCM_WIRELESSCHARGE_FIN_TIMESTAMP,
                            0,
                            ( unsigned char* )&chg_time[ 0 ] );
                ret |= Hcm_romdata_write_knl(    /* */
                            D_HCM_WIRELESSCHARGE_FIN_FCC,
                            0,
                            ( unsigned char* )&chg_fcc[ 0 ]);
                ret |= Hcm_romdata_write_knl(    /*               */
                            D_HCM_WIRELESSCHARGE_FIN_SOC,
                            0,
                            ( unsigned char* )&chg_soc[ 0 ] );

                if( count >= DFGC_LOG_CHG_CNT_MAX )   /*  */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0060 );
                    count = DFGC_CLEAR;          /*             */
                }
                else
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0070 );
                    count++;                     /*           */
                }

                ret |= Hcm_romdata_write_knl(      /*       */
                            D_HCM_WIRELESSCHARGE_FIN_COUNT,
                            0,
                            ( unsigned char* )&count );
                if( ret < 0 )                /*         */
                {
                    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x0080 );
                    MFGC_RDPROC_ERROR( DFGC_LOG_CHARGE | 0x0080 );
                    MFGC_SYSERR_DATA_SET(
                                CSYSERR_ALM_RANK_B,
                                DFGC_SYSERR_NVM_ERR,
                                ( DFGC_LOG_CHARGE | 0x0080 ),
                                ret,
                                syserr_info );
                    pfgc_log_syserr( &syserr_info );    /*  */
                    return;
                }
            }
// Jessie add end
/*<1905359>            break;                                                 */

/*<1905359>        default:                                                   */
/*<1905359>            MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0x000A );          */
/*<1905359>            break;                                                 */
    }

    MFGC_RDPROC_PATH( DFGC_LOG_CHARGE | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_log_capacity                                           */
/*                                                                            */
/*                2008.10.29                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                unsigned char type                                          */
/*                                                                            */
/******************************************************************************/
void pfgc_log_capacity( unsigned char type )
{
    signed int ret;                              /*                           */
/*<3131028>    T_FGC_SOH_LOG nvm;                           *//*                           */
    T_FGC_SOC_LOG nvm;                           /*<3131028>                    */
    unsigned short count;                        /*                           */
    unsigned long index;                         /*         index             */
    unsigned long l_time;                        /*                           */
    unsigned char capacity;                      /*<3131028>                  */
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    FGC_FUNCIN("\n");

    MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0000 );

    ret = Hcm_romdata_read_knl(                  /*  C                        */
                D_HCM_FG_CONSUMPTION_TIMESTAMP,
                0,
                ( unsigned char* )&nvm );

    ret |= Hcm_romdata_read_knl(                 /*  C                        */
                D_HCM_FG_CONSUMPTION_COUNT,
                0,
                ( unsigned char* )&count );

    if( ret < 0 )                                /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_LOG_CAPACITY | 0x0001 );
        count = 0;                               /*                           */
        MFGC_SYSERR_DATA_SET2(
                CSYSERR_ALM_RANK_B,
                DFGC_SYSERR_NVM_ERR,
                ( DFGC_LOG_CAPACITY | 0x0001 ),
                D_HCM_FG_CONSUMPTION_COUNT,
                ret,
                syserr_info );
            pfgc_log_syserr( &syserr_info );     /*  V SYSERR                 */
        return;                                  /*<3131028>                  */
    }

/*<3131028>    if( count >= DFGC_LOG_SOC_MAX )              *//*                           */
/*<3131028>    {*/
/*<3131028>        MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0002 );*/
/*<3131028>        count = DFGC_CLEAR;                      *//*                           */
/*<3131028>    }*/
/*<3131028>    else*/
/*<3131028>    {*/
/*<3131028>        MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0003 );*/
/*<3131028>        count++;                                 *//*                           */
/*<3131028>    }*/

/*<3131028>                                                 *//*                           */
/*<3131028>    index = ( unsigned long )( count % DFGC_LOG_SOC_MAX );*/

    if( gfgc_ctl.chg_start == DFGC_ON )          /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0004 );

        if( count >= DFGC_LOG_SOC_CNT_MAX )      /*<3131028>                  */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0002 ); /*<3131028>       */
            count = DFGC_CLEAR;                  /*<3131028>                  */
        }                                        /*<3131028>                  */
        else                                     /*<3131028>                  */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0003 ); /*<3131028>       */
            count++;                             /*<3131028>                  */
        }                                        /*<3131028>                  */
                                                 /*<3131028>                  */
        index = ( unsigned long )( count % DFGC_LOG_SOC_MAX ); /*<3131028>    */

        memset( &nvm.soh_t[ index ],             /*  S                        */
                DFGC_CLEAR,
/*<3131028>                sizeof( T_FGC_SOH_TIME )); */
                sizeof( T_FGC_SOC_TIME ));       /*<3131028>                  */

        ret = Hcm_romdata_write_knl(             /*  C                        */
                    D_HCM_FG_CONSUMPTION_COUNT,
                    0,
                    ( unsigned char* )&count );

        ret |= Hcm_romdata_write_knl(            /*<3131028>  C               */
                    D_HCM_FG_CONSUMPTION_TIMESTAMP, /*<3131028>               */
                    0,                           /*<3131028>                  */
                    ( unsigned char* )&nvm );    /*<3131028>                  */

        gfgc_ctl.chg_start = DFGC_OFF;           /*               OFF         */

        if( ret < 0 )                            /*                           */
        {
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0005 );
            MFGC_RDPROC_ERROR( DFGC_LOG_CAPACITY | 0x0005 );
            MFGC_SYSERR_DATA_SET2(
                CSYSERR_ALM_RANK_B,
                DFGC_SYSERR_NVM_ERR,
                ( DFGC_LOG_CAPACITY | 0x0005 ),
                D_HCM_FG_CONSUMPTION_COUNT,
                ret,
                syserr_info );
            pfgc_log_syserr( &syserr_info );     /*  V SYSERR                 */
            return;
        }

    }
// Jessie del start
//    else if(( gfgc_ctl.chg_sts                   /*                           */
//                     != DCTL_CHG_CHARGE )
// Jessie del end
// Jessie add start
    else if(( gfgc_chgsts_thread                /*                           */
                     != DCTL_CHG_CHARGE          &&
              gfgc_chgsts_thread
                     != DCTL_CHG_WIRELESS_CHARGE)
// Jessie add end
            && (( gfgc_ctl.mac_state             /*                   OFF     */
                        != DCTL_STATE_TEST )     ||
                ( gfgc_ctl.mac_state
                        != DCTL_STATE_TEST2 )    ||
                ( gfgc_ctl.mac_state
                        != DCTL_STATE_TEST_2G )  ||
                ( gfgc_ctl.mac_state
                        != DCTL_STATE_TEST2_2G ) ||
                ( gfgc_ctl.mac_state
                        != DCTL_STATE_POWOFF )))
    {
        MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0006 );
/*<3131028>        ret = Hcm_romdata_read_knl(              *//*  C                        */
/*<3131028>                D_HCM_FG_CONSUMPTION_TIMESTAMP,*/
/*<3131028>                0,*/
/*<3131028>                ( unsigned char* )&nvm );*/

/*<3131028>        if( ret < 0 )                            *//*                           */
/*<3131028>        {*/
/*<3131028>            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0007 );*/
/*<3131028>            MFGC_RDPROC_ERROR( DFGC_LOG_CAPACITY | 0x0007 );*/
/*<3131028>            MFGC_SYSERR_DATA_SET2(*/
/*<3131028>                CSYSERR_ALM_RANK_B,*/
/*<3131028>                DFGC_SYSERR_NVM_ERR,*/
/*<3131028>                ( DFGC_LOG_CAPACITY | 0x0007 ),*/
/*<3131028>                D_HCM_FG_CONSUMPTION_TIMESTAMP,*/
/*<3131028>                ret,*/
/*<3131028>                syserr_info );*/
/*<3131028>            pfgc_log_syserr( &syserr_info );     *//*  V SYSERR                 */
/*<3131028>            return;*/
/*<3131028>        }*/
        index = ( unsigned long )( count % DFGC_LOG_SOC_MAX ); /*<3131028>    */

        l_time = pfgc_log_time();                /*  C                        */

/*<3131028>        if(( gfgc_batt_info.capacity             *//*           10%             */
/*<3131028>                    <= DFGC_CAPACITY_10 )*/
/*<3131028>            && ( nvm.soh_t[ index ].time_10      *//*                           */
/*<3131028>                    == DFGC_CLEAR ))*/
/*<3131028>        {*/
/*<3131028>            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0008 );*/
/*<3131028>            nvm.soh_t[ index ].time_10 = l_time; *//*                           */
/*<3131028>        }*/
/*<3131028>        else if(( gfgc_batt_info.capacity        *//*           20%             */
/*<3131028>                    <= DFGC_CAPACITY_20 )*/
/*<3131028>            && ( nvm.soh_t[ index ].time_20      *//*                           */
/*<3131028>                    == DFGC_CLEAR ))*/
/*<3131028>        {*/
/*<3131028>            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0009 );*/
/*<3131028>            nvm.soh_t[ index ].time_20 = l_time; *//*                           */
/*<3131028>        }*/
/*<3131028>        else if(( gfgc_batt_info.capacity        *//*           40%             */
/*<3131028>                    <= DFGC_CAPACITY_40 )*/
/*<3131028>            && ( nvm.soh_t[ index ].time_40      *//*                           */
/*<3131028>                    == DFGC_CLEAR ))*/
/*<3131028>        {*/
/*<3131028>            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000A );*/
/*<3131028>            nvm.soh_t[ index ].time_40 = l_time; *//*                           */
/*<3131028>        }*/
/*<3131028>        else if(( gfgc_batt_info.capacity        *//*           60%             */
/*<3131028>                    <= DFGC_CAPACITY_60 )*/
/*<3131028>            && ( nvm.soh_t[ index ].time_60      *//*                           */
/*<3131028>                    == DFGC_CLEAR ))*/
/*<3131028>        {*/
/*<3131028>            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000B );*/
/*<3131028>            nvm.soh_t[ index ].time_60 = l_time; *//*                           */
/*<3131028>        }*/
/*<3131028>        else if(( gfgc_batt_info.capacity        *//*           80%             */
/*<3131028>                    <= DFGC_CAPACITY_80 )*/
/*<3131028>            && ( nvm.soh_t[ index ].time_80      *//*                           */
/*<3131028>                    == DFGC_CLEAR ))*/
/*<3131028>        {*/
/*<3131028>            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000C );*/
/*<3131028>            nvm.soh_t[ index ].time_80 = l_time; *//*                           */
/*<3131028>        }*/
        capacity = gfgc_batt_info.capacity;      /*<3131028>                  */
        if( capacity <= DFGC_CAPACITY_10 )       /*<3131028>          10%     */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0008 ); /*<3131028>       */
            if( nvm.soh_t[ index ].time_10       /*<3131028>                  */
                     == DFGC_CLEAR )             /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0010 ); /*<3131028>   */
                nvm.soh_t[ index ].time_10 = l_time; /*<3131028>              */
            }                                    /*<3131028>                  */
            else                                 /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0011 ); /*<3131028>   */
                return;                          /*<3131028>                  */
            }                                    /*<3131028>                  */
        }                                        /*<3131028>                  */
        else if( capacity <= DFGC_CAPACITY_20 )  /*<3131028>          20%     */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0009 ); /*<3131028>       */
            if( nvm.soh_t[ index ].time_20       /*<3131028>                  */
                     == DFGC_CLEAR )             /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0012 ); /*<3131028>   */
                nvm.soh_t[ index ].time_20 = l_time; /*<3131028>              */
            }                                    /*<3131028>                  */
            else                                 /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0013 ); /*<3131028>   */
                return;                          /*<3131028>                  */
            }                                    /*<3131028>                  */
        }                                        /*<3131028>                  */
        else if( capacity <= DFGC_CAPACITY_40 )  /*<3131028>          40%     */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000A ); /*<3131028>       */
            if( nvm.soh_t[ index ].time_40       /*<3131028>                  */
                     == DFGC_CLEAR )             /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0014 ); /*<3131028>   */
                nvm.soh_t[ index ].time_40 = l_time; /*<3131028>              */
            }                                    /*<3131028>                  */
            else                                 /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0015 ); /*<3131028>   */
                return;                          /*<3131028>                  */
            }                                    /*<3131028>                  */
        }                                        /*<3131028>                  */
        else if( capacity <= DFGC_CAPACITY_60 )  /*<3131028>          60%     */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000B ); /*<3131028>       */
            if( nvm.soh_t[ index ].time_60       /*<3131028>                  */
                     == DFGC_CLEAR )             /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0016 ); /*<3131028>   */
                nvm.soh_t[ index ].time_60 = l_time; /*<3131028>              */
            }                                    /*<3131028>                  */
            else                                 /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0017 ); /*<3131028>   */
                return;                          /*<3131028>                  */
            }                                    /*<3131028>                  */
        }                                        /*<3131028>                  */
        else if( capacity <= DFGC_CAPACITY_80 )  /*<3131028>          80%     */
        {                                        /*<3131028>                  */
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000C ); /*<3131028>       */
            if( nvm.soh_t[ index ].time_80       /*<3131028>                  */
                     == DFGC_CLEAR )             /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0018 ); /*<3131028>   */
                nvm.soh_t[ index ].time_80 = l_time; /*<3131028>              */
            }                                    /*<3131028>                  */
            else                                 /*<3131028>                  */
            {                                    /*<3131028>                  */
                MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x0019 ); /*<3131028>   */
                return;                          /*<3131028>                  */
            }                                    /*<3131028>                  */
        }                                        /*<3131028>                  */
        else
        {
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000D );
            return;
        }

        ret = Hcm_romdata_write_knl(             /*  C                        */
                D_HCM_FG_CONSUMPTION_TIMESTAMP,
                0,
                ( unsigned char* )&nvm );
        if( ret < 0 )                            /*                           */
        {
            MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000E );
            MFGC_RDPROC_ERROR( DFGC_LOG_CAPACITY | 0x000E );
            MFGC_SYSERR_DATA_SET2(
                CSYSERR_ALM_RANK_B,
                DFGC_SYSERR_NVM_ERR,
                ( DFGC_LOG_CAPACITY | 0x000E ),
                D_HCM_FG_CONSUMPTION_TIMESTAMP,
                ret,
                syserr_info );
            pfgc_log_syserr( &syserr_info );     /*  V SYSERR                 */
        }
    }
    else
    {
        MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0x000F );
    }

    MFGC_RDPROC_PATH( DFGC_LOG_CAPACITY | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_log_time                                               */
/*                                                                            */
/*                2008.10.22                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                unsigned long time                                          */
/*                                    000000mmmmDDDDHHHHMMMMMMSSSSSS          */
/*                                    mmmm   :   (1-12) 4bit                  */
/*                                    DDDDD  :   (1-31) 5bit                  */
/*                                    HHHHH  :   (0-23) 5bit                  */
/*                                    MMMMMM :   (0-59) 6bit                  */
/*                                    SSSSSS :   (0-59) 6bit                  */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
unsigned long  pfgc_log_time( void )
{
    unsigned long year;                         /*                            */
    unsigned long month;                        /*                            */
    time_t        day;                          /*                            */
    time_t        hour;                         /*                            */
    time_t        min;                          /*                            */
    time_t        sec;                          /*                            */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    time_t        usec;                         /*                            */
#endif /*<Conan>1111XX*/

    unsigned long timestamp;                    /*                            */

    MFGC_RDPROC_PATH( DFGC_LOG_TIME | 0x0000 );

    syserr_TimeStamp( &year,                    /*  V                         */
                      &month,
                      &day,
                      &hour,
                      &min,
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>                      &sec );*/
                      &sec, /*<Conan>*/
                      &usec );
#else /*<Conan>1111XX*/
                      &sec );
#endif /*<Conan>1111XX*/

                                                /*                            */
    timestamp = ((((( month << DFGC_MONTH_SHIFT )
               | (( unsigned long )day   << DFGC_DAY_SHIFT   ))
               | (( unsigned long )hour  << DFGC_HOUR_SHIFT  ))
               | (( unsigned long )min   << DFGC_MIN_SHIFT   ))
               | (( unsigned long )sec                       ));

    MFGC_RDPROC_PATH( DFGC_LOG_TIME | 0xFFFF );

    return timestamp;
}

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/******************************************************************************//*<Conan>*/
/*                pfgc_log_time_ex(    )                                      *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*              unsigned long long sys_time                                   *//*<Conan>*/
/*              000000yyyyyyyyyyyymmmmDDDDHHHHMMMMMMSSSSSSuuuuuuuuuuuuuuuuuuuu*//*<Conan>*/
/*                              yyyyyyyyyyyy :   (0-4095) 12bit               *//*<Conan>*/
/*                                    mmmm   :   (1-12) 4bit                  *//*<Conan>*/
/*                                    DDDDD  :   (1-31) 5bit                  *//*<Conan>*/
/*                                    HHHHH  :   (0-23) 5bit                  *//*<Conan>*/
/*                                    MMMMMM :   (0-59) 6bit                  *//*<Conan>*/
/*                                    SSSSSS :   (0-59) 6bit                  *//*<Conan>*/
/*                      uuuuuuuuuuuuuuuuuuuu :           (0-999999) 20bit     *//*<Conan>*/
/*              unsigned long long mono_time                                  *//*<Conan>*/
/*              00000DDDDDDDDDDDDDDDDDDDDDHHHHMMMMMMSSSSSSuuuuuuuuuuuuuuuuuuuu*//*<Conan>*/
/*                                    DDDDD  :   (1-2097151) 21bit            *//*<Conan>*/
/*                                    HHHHH  :   (0-23) 5bit                  *//*<Conan>*/
/*                                    MMMMMM :   (0-59) 6bit                  *//*<Conan>*/
/*                                    SSSSSS :   (0-59) 6bit                  *//*<Conan>*/
/*                      uuuuuuuuuuuuuuuuuuuu :           (0-999999) 20bit     *//*<Conan>*/
/******************************************************************************//*<Conan>*/
void pfgc_log_time_ex( unsigned long long * sys_time,
                       unsigned long long * mono_time )
{
    unsigned long year;                         /*                            */
    unsigned long month;                        /*                            */
    time_t        day;                          /*                            */
    time_t        hour;                         /*                            */
    time_t        min;                          /*                            */
    time_t        sec;                          /*                            */
    time_t        usec;                         /*                            */
    unsigned long mono_day;                     /*                            */


    unsigned long long timestamp;               /*                            */


    syserr_TimeStamp( &year,                    /*  V                         */
                      &month,
                      &day,
                      &hour,
                      &min,
                      &sec,
                      &usec );

                                                /*                            */
    timestamp  = ( unsigned long long )year << ( DFGC_YEAR_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )month << ( DFGC_MONTH_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )day   << ( DFGC_DAY_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )hour  << ( DFGC_HOUR_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )min   << ( DFGC_MIN_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )sec   << ( DFGC_SEC_SHIFT );
    timestamp |= ( unsigned long long )usec;

    *sys_time = timestamp;
    
    monotonic_TimeStamp( &mono_day,             /*  V                         */
                         &hour,
                         &min,
                         &sec,
                         &usec );

                                                /*                            */
    timestamp  = ( unsigned long long )mono_day   << ( DFGC_DAY_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )hour  << ( DFGC_HOUR_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )min   << ( DFGC_MIN_SHIFT + DFGC_TIME_OFFSET );
    timestamp |= ( unsigned long long )sec   << ( DFGC_SEC_SHIFT );
    timestamp |= ( unsigned long long )usec;

    *mono_time = timestamp;

//    return timestamp;/*<Conan time add>*/
    return;
}

/******************************************************************************//*<Conan>*/
/*                pfgc_log_time_diff                                          *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                T_FGC_TIME_INFO                                             *//*<Conan>*/
/*                unsigned long long src_time,dst_time                        *//*<Conan>*/
/*                                      (1-2097151) 21bit                     *//*<Conan>*/
/*                                      (0-23) 5bit                           *//*<Conan>*/
/*                                      (0-59) 6bit                           *//*<Conan>*/
/*                                      (0-59) 6bit                           *//*<Conan>*/
/*                                              (0-999999) 20bit              *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/******************************************************************************//*<Conan>*/
T_FGC_TIME_DIFF_INFO pfgc_log_time_diff( unsigned long long src_time, 
                                         unsigned long long dst_time )
{
    T_FGC_TIME_DIFF_INFO diff_time;             /*                            */
    T_FGC_TIME_INFO tmp_src_time;               /*                            */
    T_FGC_TIME_INFO tmp_dst_time;               /*                            */
    unsigned long long tmp_src_sec  = 0;        /*                            */
    unsigned long long tmp_dst_sec  = 0;        /*                            */
    unsigned long long calc_sec1;               /*                            */
    unsigned long long calc_sec2;               /*                            */
    unsigned long long tmp_src;                 /*                            */
    unsigned long long tmp_dst;                 /*                            */
    
    
    tmp_src = src_time;                         /*                            */
    tmp_dst = dst_time;                         /*                            */
    
                                                /*                            */
    tmp_dst_time.day   = (tmp_dst >> ( DFGC_DAY_SHIFT + DFGC_TIME_OFFSET ))   & DFGC_DAY_BIT  ;   /*    */
    tmp_dst_time.hour  = (tmp_dst >> ( DFGC_HOUR_SHIFT + DFGC_TIME_OFFSET ))  & DFGC_HOUR_BIT ;   /*    */
    tmp_dst_time.min   = (tmp_dst >> ( DFGC_MIN_SHIFT + DFGC_TIME_OFFSET ))   & DFGC_MIN_BIT  ;   /*    */
    tmp_dst_time.sec   = (tmp_dst >> ( DFGC_SEC_SHIFT ))                      & DFGC_SEC_BIT  ;   /*    */
    tmp_dst_time.usec  = tmp_dst                                              & DFGC_USEC_BIT ;   /*            */

    tmp_src_time.day   = (tmp_src >> ( DFGC_DAY_SHIFT + DFGC_TIME_OFFSET ))   & DFGC_DAY_BIT  ;   /*    */
    tmp_src_time.hour  = (tmp_src >> ( DFGC_HOUR_SHIFT + DFGC_TIME_OFFSET ))  & DFGC_HOUR_BIT ;   /*    */
    tmp_src_time.min   = (tmp_src >> ( DFGC_MIN_SHIFT + DFGC_TIME_OFFSET ))   & DFGC_MIN_BIT  ;   /*    */
    tmp_src_time.sec   = (tmp_src >> ( DFGC_SEC_SHIFT ))                      & DFGC_SEC_BIT  ;   /*    */
    tmp_src_time.usec  = tmp_src                                              & DFGC_USEC_BIT ;   /*            */

    if((tmp_dst_time.day > DFGC_MAX_DAY) ||
       (tmp_src_time.day > DFGC_MAX_DAY))
    {
        diff_time.min  = DFGC_SOH_VAL_INIT;
        diff_time.sec  = DFGC_SOH_VAL_INIT;
        diff_time.usec = DFGC_SOH_VAL_INIT;
        return diff_time;                       /*           0                */
    }

    /*                    */
    /*                       ->         */
    tmp_dst_sec = ((((((unsigned long long)tmp_dst_time.day * 24 ) + tmp_dst_time.hour ) * 60 ) +
                   tmp_dst_time.min ) * 60 ) +
                   tmp_dst_time.sec;
    
    /*                    */
    /*                       ->         */
    tmp_src_sec = ((((((unsigned long long)tmp_src_time.day * 24 ) + tmp_src_time.hour ) * 60 ) +
                   tmp_src_time.min ) * 60 ) +
                   tmp_src_time.sec;
    
    /*                */
    if( tmp_dst_time.usec >= tmp_src_time.usec )
    {
        diff_time.usec = tmp_dst_time.usec - tmp_src_time.usec;
    }
    else
    {
        if(( tmp_src_sec >= tmp_dst_sec ) ||
           ( tmp_dst_sec <= 0 ))                /*                            */
        {
            diff_time.min  = DFGC_SOH_VAL_INIT;
            diff_time.sec  = DFGC_SOH_VAL_INIT;
            diff_time.usec = DFGC_SOH_VAL_INIT;
            return diff_time;                   /*           0                */
        }
        
        diff_time.usec = ( 1000000 - tmp_src_time.usec ) + tmp_dst_time.usec;
        
        tmp_dst_sec -= 1;                       /* 1                          */
    }
    
    if( tmp_src_sec > tmp_dst_sec )             /*                            */
    {
        diff_time.min  = DFGC_SOH_VAL_INIT;
        diff_time.sec  = DFGC_SOH_VAL_INIT;
        diff_time.usec = DFGC_SOH_VAL_INIT;
        return diff_time;                       /*           0                */
    }
    
    calc_sec1 = tmp_dst_sec - tmp_src_sec;      /*                            */
    
    calc_sec2 = do_div(calc_sec1, 0x3C);        /*                            */
    diff_time.sec = (unsigned long)calc_sec2;   /*                            */
    diff_time.min = (unsigned long)calc_sec1;   /*                            */

    return diff_time;
}
#endif /*<Conan>1111XX*/

/********************************************************************<QEIO022>*/
/*                pfgc_log_eco                                       <QEIO022>*/
/*                      ECO                                          <QEIO022>*/
/*                2009.10.26                                         <QEIO022>*/
/*                NTTDMSE                                            <QEIO022>*/
/*                                                                   <QEIO022>*/
/*                                                                   <QEIO022>*/
/*                unsigned long *mode                                <QEIO022>*/
/*                                                                   <QEIO022>*/
/********************************************************************<QEIO022>*/
void pfgc_log_eco( unsigned long *mode )                           /*<QEIO022>*/
{                                                                  /*<QEIO022>*/
#if DFGC_SW_DELETED_FUNC
    signed int                mod_ret;           /*<QEIO022>                  */
    unsigned int              cnt;               /*<QEIO022>                  */
    unsigned long             timestamp;         /*<QEIO022>                  */
    unsigned short            eco_count;         /*<QEIO022>       ECO        */
    T_FGC_ECO_SOC_LOG         eco_soc_log;       /*<QEIO022> SOC              */
    T_FGC_ECO_TIME_LOG        eco_time_log;      /*<QEIO022>                  */
    T_FGC_SYSERR_DATA         syserr_info;       /*<QEIO022> SYS              */
                                                 /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
    MFGC_RDPROC_PATH( DFGC_LOG_ECO | 0x0000 );   /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
    if( mode == NULL )                           /*<QEIO022>                  */
    {                                            /*<QEIO022>                  */
        MFGC_RDPROC_PATH(                        /*<QEIO022>                  */
            DFGC_LOG_ECO | 0x0001 );             /*<QEIO022>                  */
        MFGC_RDPROC_ERROR(                       /*<QEIO022>                  */
            DFGC_LOG_ECO | 0x0001 );             /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
        MFGC_SYSERR_DATA_SET(                    /*<QEIO022>                  */
            CSYSERR_ALM_RANK_B,                  /*<QEIO022>                  */
            DFGC_SYSERR_PARAM_ERR,               /*<QEIO022>                  */
            ( DFGC_LOG_ECO | 0x0001 ),           /*<QEIO022>                  */
            ( unsigned long )mode,               /*<QEIO022>                  */
            syserr_info );                       /*<QEIO022>                  */
        pfgc_log_syserr( &syserr_info );         /*<QEIO022>   V SYSERR       */
                                                 /*<QEIO022>                  */
        return;                                  /*<QEIO022>                  */
    }                                            /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
    if( *mode == DCTL_PWR_SAVE_ECO_ON )          /*<QEIO022>                  */
    {                                            /*<QEIO022>                  */
        /*                                     *//*<QEIO022>                  */
        mod_ret = Hcm_romdata_read_knl(          /*<QEIO022>   C              */
                      D_HCM_E_ECOMODE_LOG_CNT,   /*<QEIO022>                  */
                      0,                         /*<QEIO022>                  */
                      ( unsigned char * )        /*<QEIO022>                  */
                          &eco_count );          /*<QEIO022>                  */
        if( mod_ret < 0 )                        /*<QEIO022>                  */
        {                                        /*<QEIO022>                  */
            MFGC_RDPROC_PATH(                    /*<QEIO022>                  */
                DFGC_LOG_ECO | 0x0002 );         /*<QEIO022>                  */
            MFGC_RDPROC_ERROR(                   /*<QEIO022>                  */
                DFGC_LOG_ECO | 0x0002 );         /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
            MFGC_SYSERR_DATA_SET(                /*<QEIO022>                  */
                CSYSERR_ALM_RANK_B,              /*<QEIO022>                  */
                DFGC_SYSERR_NVM_ERR,             /*<QEIO022>                  */
                ( DFGC_LOG_ECO | 0x0002 ),       /*<QEIO022>                  */
                mod_ret,                         /*<QEIO022>                  */
                syserr_info );                   /*<QEIO022>                  */
            pfgc_log_syserr( &syserr_info );     /*<QEIO022>   V SYSERR       */
        }                                        /*<QEIO022>                  */
        /*                                     *//*<QEIO022>                  */
        else if( eco_count                       /*<QEIO022>                  */
                 < DFGC_LOG_ECO_LOG_MAX )        /*<QEIO022>                  */
        {                                        /*<QEIO022>                  */
            MFGC_RDPROC_PATH(                    /*<QEIO022>                  */
                DFGC_LOG_ECO | 0x0003 );         /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
            /*                                 *//*<QEIO022>                  */
            timestamp = pfgc_log_time();         /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
            /*                                 *//*<QEIO022>                  */
            mod_ret = Hcm_romdata_read_knl(      /*<QEIO022>   C              */
                D_HCM_E_ECOMODE_LOG_SOC,         /*<QEIO022>                  */
                0,                               /*<QEIO022>                  */
                ( unsigned char * )&eco_soc_log );  /*<QEIO022>               */
            mod_ret |= Hcm_romdata_read_knl(     /*<QEIO022>   C              */
                D_HCM_E_ECOMODE_LOG_TIMESTAMP,   /*<QEIO022>                  */
                0,                               /*<QEIO022>                  */
                ( unsigned char * )&eco_time_log );  /*<QEIO022>              */
            if( mod_ret < 0 )                               /*<QEIO022>       */
            {                                               /*<QEIO022>       */
                MFGC_RDPROC_PATH( DFGC_LOG_ECO | 0x0004 );  /*<QEIO022>       */
                MFGC_RDPROC_ERROR( DFGC_LOG_ECO | 0x0004 ); /*<QEIO022>       */
                                                            /*<QEIO022>       */
                MFGC_SYSERR_DATA_SET(                       /*<QEIO022>       */
                    CSYSERR_ALM_RANK_B,                     /*<QEIO022>       */
                    DFGC_SYSERR_NVM_ERR,                    /*<QEIO022>       */
                    ( DFGC_LOG_ECO | 0x0004 ),              /*<QEIO022>       */
                    mod_ret,                                /*<QEIO022>       */
                    syserr_info );                          /*<QEIO022>       */
                pfgc_log_syserr( &syserr_info );            /*<QEIO022>   V   */
            }                                               /*<QEIO022>       */
            else                                            /*<QEIO022>       */
            {                                               /*<QEIO022>       */
                MFGC_RDPROC_PATH( DFGC_LOG_ECO | 0x0005 );  /*<QEIO022>       */
                                                            /*<QEIO022>       */
                /*                                        *//*<QEIO022>       */
                cnt = eco_count % DFGC_LOG_ECO_MAX;         /*<QEIO022>       */
                eco_soc_log.soc[ cnt ] = gfgc_batt_info.soc;/*<QEIO022>       */
                eco_time_log.time[ cnt ] = timestamp;       /*<QEIO022>       */
                                                            /*<QEIO022>       */
                /*                                        *//*<QEIO022>       */
                eco_count++;                                /*<QEIO022>       */
                                                            /*<QEIO022>       */
                /*                                        *//*<QEIO022>       */
                mod_ret = Hcm_romdata_write_knl(            /*<QEIO022>   C   */
                            D_HCM_E_ECOMODE_LOG_CNT,        /*<QEIO022>       */
                            0,                              /*<QEIO022>       */
                            ( unsigned char * )&eco_count );    /*<QEIO022>   */
                mod_ret |= Hcm_romdata_write_knl(           /*<QEIO022>   C   */
                            D_HCM_E_ECOMODE_LOG_SOC,        /*<QEIO022>       */
                            0,                              /*<QEIO022>       */
                            ( unsigned char * )&eco_soc_log );  /*<QEIO022>   */
                mod_ret |= Hcm_romdata_write_knl(           /*<QEIO022>   C   */
                            D_HCM_E_ECOMODE_LOG_TIMESTAMP,  /*<QEIO022>       */
                            0,                              /*<QEIO022>       */
                            ( unsigned char * )&eco_time_log );  /*<QEIO022>  */
                if( mod_ret < 0 )                           /*<QEIO022>       */
                {                                           /*<QEIO022>       */
                    MFGC_RDPROC_PATH(                       /*<QEIO022>       */
                        DFGC_LOG_ECO | 0x0006 );            /*<QEIO022>       */
                    MFGC_RDPROC_ERROR(                      /*<QEIO022>       */
                        DFGC_LOG_ECO | 0x0006 );            /*<QEIO022>       */
                                                            /*<QEIO022>       */
                    MFGC_SYSERR_DATA_SET(                   /*<QEIO022>       */
                        CSYSERR_ALM_RANK_B,                 /*<QEIO022>       */
                        DFGC_SYSERR_NVM_ERR,                /*<QEIO022>       */
                        ( DFGC_LOG_ECO | 0x0006 ),          /*<QEIO022>       */
                        mod_ret,                            /*<QEIO022>       */
                        syserr_info );                      /*<QEIO022>       */
                    pfgc_log_syserr( &syserr_info );        /*<QEIO022>   V   */
                }                                           /*<QEIO022>       */
            }                                               /*<QEIO022>       */
        }                                        /*<QEIO022>                  */
        /*                                     *//*<QEIO022>                  */
        else                                     /*<QEIO022>                  */
        {                                        /*<QEIO022>                  */
            MFGC_RDPROC_PATH(                    /*<QEIO022>                  */
                DFGC_LOG_ECO | 0x0007 );         /*<QEIO022>                  */
        }                                        /*<QEIO022>                  */
    }                                            /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
    MFGC_RDPROC_PATH( DFGC_LOG_ECO | 0xFFFF );   /*<QEIO022>                  */
#endif /* DFGC_SW_DELETED_FUNC */
    return;                                      /*<QEIO022>                  */
}                                                /*<QEIO022>                  */

void pfgc_log_print( char  **lk_p, int mode )
{
	unsigned char count_x;                        /*                          */
	unsigned char count_y;                        /*                          */

	FGC_FUNCIN("\n");

        /*--------------------------------------------------------------------*/
        /*                                                                    */
        /*--------------------------------------------------------------------*/
        *lk_p += sprintf( *lk_p, "***** 電池残量DD *****\n" );

        *lk_p += sprintf( *lk_p, "エラー表示\n");
        *lk_p += sprintf( *lk_p, "  エラーポインタ : = %03d\n",
              ( unsigned char )gfgc_path_error.path_cnt );
        *lk_p += sprintf( *lk_p, "   (最新パスは一つ前)\n");
        *lk_p += sprintf( *lk_p,                    /* 1                        */
                         "          00       01       02       " );
        *lk_p += sprintf( *lk_p,
                         "03       04       05       06       " );
        *lk_p += sprintf( *lk_p,
                         "07       08       09\n" );

        for( count_y = 0;                         /*   R                      */
             count_y < ( DFGC_RDPROC_EPATH_MAX / 10 );
             count_y++ )                          /*                          */
        {                                         /*                          */
            *lk_p += sprintf( *lk_p, "%03d ",       /* 10                       */
                             ( count_y * 10 ));   /*                          */
                                                  /*                          */
            for( count_x = 0;                     /*   R                      */
                 count_x < 10;                    /*                          */
                 count_x++ )                      /*                          */
            {                                     /*                          */
                *lk_p += sprintf( *lk_p, "%08lx ",  /*                          */
                                 gfgc_path_error.path[ /*                     */
                                  ( count_y * 10 ) +  count_x ] ); /*         */
            }                                     /*                          */
                                                  /*                          */
            *lk_p += sprintf( *lk_p, "\n" );        /*                          */
        }                                         /*                          */

        *lk_p += sprintf( *lk_p, "\n");

        *lk_p += sprintf( *lk_p, "パス表示\n");
        *lk_p += sprintf( *lk_p, "   パスポインタ : = %03ld\n",
                               gfgc_path_debug.path_cnt );
        *lk_p += sprintf( *lk_p, "   (最新パスは一つ前)\n");
        *lk_p += sprintf( *lk_p,                    /* 1                        */
                         "         00       01       02       " );
        *lk_p += sprintf( *lk_p,
                         "03       04       05       06       " );
        *lk_p += sprintf( *lk_p,
                         "07       08       09\n" );

        for( count_y = 0;                         /*   R                      */
             count_y < ( MFGC_RDPROC_PATH_MAX / 10 );
             count_y++ )                          /*                          */
        {                                         /*                          */
            *lk_p += sprintf( *lk_p, "%03d ",       /* 10                       */
                             ( count_y * 10 ));   /*                          */
                                                  /*                          */
            for( count_x = 0;                     /*   R                      */
                 count_x < 10;                    /*                          */
                 count_x++ )                      /*                          */
            {                                     /*                          */
                *lk_p += sprintf( *lk_p, "%08lx ",  /*                          */
                                 gfgc_path_debug.path[ /*                     */
                                  ( count_y * 10 ) +  count_x ] ); /*         */
            }                                     /*                          */
                                                  /*                          */
            *lk_p += sprintf( *lk_p, "\n" );        /*                          */
        }                                         /*                          */

        *lk_p += sprintf( *lk_p, "\n" );
        *lk_p += sprintf( *lk_p, "FGC-DDデータ表示\n" );
        *lk_p += sprintf( *lk_p, "\t電池レベル2閾値  0x%02x\n",
                                          gfgc_ctl.lvl_2 );
        *lk_p += sprintf( *lk_p, "\t電池レベル1閾値  0x%02x\n",
                                          gfgc_ctl.lvl_1 );
        *lk_p += sprintf( *lk_p, "\tLVA閾値(3G)      0x%02x\n",
                                          gfgc_ctl.lva_3g );
        *lk_p += sprintf( *lk_p, "\tLVA閾値(2G)      0x%02x\n",
                                          gfgc_ctl.lva_2g );
        *lk_p += sprintf( *lk_p, "\tALARM閾値(3G)    0x%02x\n",  /*<3210049>*/
                                          gfgc_ctl.alarm_3g ); /*<3210049>*/
        *lk_p += sprintf( *lk_p, "\tALARM閾値(2G)    0x%02x\n",  /*<3210049>*/
                                          gfgc_ctl.alarm_2g ); /*<3210049>*/
        *lk_p += sprintf( *lk_p, "\tDUMP処理有効     0x%02x\n",
                                          gfgc_ctl.dump_enable );
        *lk_p += sprintf( *lk_p, "\t充電タイマ       0x%02x\n",
                                          gfgc_ctl.charge_timer );
        *lk_p += sprintf( *lk_p, "\t劣化平均回数     0x%02x\n",
                                          gfgc_ctl.soh_n );
        *lk_p += sprintf( *lk_p, "\t劣化平均初期値   0x%02x\n",
                                          gfgc_ctl.soh_init );
        *lk_p += sprintf( *lk_p, "\t劣化精度指定     0x%02x\n",
                                          gfgc_ctl.soh_level );
        *lk_p += sprintf( *lk_p, "\t劣化気味閾値     0x%02x\n",
                                          gfgc_ctl.soh_down );
        *lk_p += sprintf( *lk_p, "\t劣化閾値         0x%02x\n",
                                          gfgc_ctl.soh_bad );
        *lk_p += sprintf( *lk_p, "\t電池容量UI固定   0x%02x\n",
                                          gfgc_ctl.capacity_fix );
        *lk_p += sprintf( *lk_p, "\t移動機状態       0x%08lx\n",
                                          gfgc_ctl.mac_state );
        *lk_p += sprintf( *lk_p, "\tUI上の充電状態   0x%02x\n",
                                          gfgc_ctl.chg_sts );
        *lk_p += sprintf( *lk_p, "\tDEV1 充電状態    0x%02x\n",
                                          gfgc_ctl.dev1_chgsts );
        *lk_p += sprintf( *lk_p, "\tDEV1 電池電圧    0x%04x\n",
                                          gfgc_ctl.dev1_volt );
        *lk_p += sprintf( *lk_p, "\tDEV1 class情報   0x%02x\n",
                                          gfgc_ctl.dev1_class );
        *lk_p += sprintf( *lk_p, "\t通知済UI電池容量 0x%02x\n",
                                          gfgc_ctl.capacity_old );
        *lk_p += sprintf( *lk_p, "\t電池劣化状態     0x%02x\n",
                                          gfgc_ctl.health_old );
        *lk_p += sprintf( *lk_p, "\twakeup要因       0x%08lx\n",
                                          gfgc_ctl.flag );
        *lk_p += sprintf( *lk_p, "\t充電開始フラグ   0x%02x\n",
                                          gfgc_ctl.chg_start );
        *lk_p += sprintf( *lk_p, "\tOCV1結果 1:成功  0x%02x\n",       /*<PCIO033>*/
                                          gfgc_ctl.ocv_okng[ 0 ] ); /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tOCV2結果 2:失敗  0x%02x\n",       /*<PCIO033>*/
                                          gfgc_ctl.ocv_okng[ 1 ] ); /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tOCV3結果 0:未実施0x%02x\n",       /*<PCIO033>*/
                                          gfgc_ctl.ocv_okng[ 2 ] ); /*<PCIO033>*/

        *lk_p += sprintf( *lk_p, "\n" );
        *lk_p += sprintf( *lk_p, "電池情報テーブル\n" );
        *lk_p += sprintf( *lk_p, "\tUI電池容量       0x%02x\n",
                                          gfgc_batt_info.capacity );
        *lk_p += sprintf( *lk_p, "\tUI電池劣化状態   0x%02x\n",
                                          gfgc_batt_info.health );
        *lk_p += sprintf( *lk_p, "\t電池レベル       0x%02x\n",
                                          gfgc_batt_info.level );
        *lk_p += sprintf( *lk_p, "\t電池状態         0x%02x\n",
                                          gfgc_batt_info.batt_status );
        *lk_p += sprintf( *lk_p, "\tMTF電池残量      0x%02x\n",
                                          gfgc_batt_info.rest_batt );
        *lk_p += sprintf( *lk_p, "\tMTF通話可能時間  0x%04x\n",
                                          gfgc_batt_info.rest_time );
        *lk_p += sprintf( *lk_p, "\t電池温度         0x%04x\n",
                                          gfgc_batt_info.temp );
        *lk_p += sprintf( *lk_p, "\t電池電圧VOLT(mV) 0x%04x\n",    /*<PCIO033>*/
                                          gfgc_batt_info.volt ); /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tNAC              0x%04x\n",
                                          gfgc_batt_info.nac );
        *lk_p += sprintf( *lk_p, "\tFAC              0x%04x\n",
                                          gfgc_batt_info.fac );
        *lk_p += sprintf( *lk_p, "\tRM               0x%04x\n",
                                          gfgc_batt_info.rm );
        *lk_p += sprintf( *lk_p, "\tFCC              0x%04x\n",
                                          gfgc_batt_info.fcc );
        if( mode == DFGC_LOG_AI_HEX ){
		*lk_p += sprintf( *lk_p, "\t平均電流         0x%04x\n",
					gfgc_batt_info.ai );
        }
        else{
		*lk_p += sprintf( *lk_p, "\t平均電流 %s(mA)%6d\n",
					(gfgc_batt_info.ai & 0x8000 ? "放電" : "充電"), (signed short)gfgc_batt_info.ai);
        }
        *lk_p += sprintf( *lk_p, "\t残利用時間(分)   0x%04x\n",
                                          gfgc_batt_info.tte );
        *lk_p += sprintf( *lk_p, "\t残充電時間(分)   0x%04x\n",
                                          gfgc_batt_info.ttf );
        *lk_p += sprintf( *lk_p, "\t電池劣化度(％)   0x%04x\n",
                                          gfgc_batt_info.soh );
        *lk_p += sprintf( *lk_p, "\tCNTL             0x%04x\n",
                                          gfgc_batt_info.cc );
        *lk_p += sprintf( *lk_p, "\t電池容量SOC      0x%04x\n",
                                          gfgc_batt_info.soc );

        *lk_p += sprintf( *lk_p, "\n" );                             /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "SDRAM固定領域\n" );                /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tLOG_WP           0x%04x\n",      /*<PCIO033>*/
                         gfgc_ctl.remap_log_dump->fgc_log_data[ 0 ].log_wp );   /*<PCIO033>*/
#if DFGC_SW_DELETED_FUNC
        *lk_p += sprintf( *lk_p, "\tSOH_WP           0x%02x\n",      /*<PCIO033>*/
                         gfgc_ctl.remap_soh->soh_wp );             /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tSOH_SUM          0x%04x\n",      /*<PCIO033>*/
                         gfgc_ctl.remap_soh->soh_sum );            /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tSOH_OLD          0x%02x\n",      /*<PCIO033>*/
                         gfgc_ctl.remap_soh->soh_old );            /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tSOH_AVE          0x%02x\n",      /*<PCIO033>*/
                         gfgc_ctl.remap_soh->soh_ave );            /*<PCIO033>*/
        *lk_p += sprintf( *lk_p, "\tSOH_CNT          0x%08lx\n",     /*<PCIO033>*/
                         gfgc_ctl.remap_soh->soh_cnt );            /*<PCIO033>*/
#endif /* DFGC_SW_DELETED_FUNC */
        *lk_p += sprintf( *lk_p, "\tOCV_CTL          0x%04x\n",
                                          gfgc_ctl.remap_ocv->ocv_ctl_flag );

        *lk_p += sprintf( *lk_p, "\n" );                             /*<3210076>*/
        *lk_p += sprintf( *lk_p, "\tFGC割り込み回数  0x%08lx\n",     /*<3210076>*/
                                          gfgc_int_flag );         /*<3210076>*/
}

/******************************************************************************/
/*                pfgc_log_rdproc                                             */
/*                        DD proc                                             */
/*                2008.10.22                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                lk_len                                 byte                 */
/*                pk_page                                                     */
/*                pk_start                                                    */
/*                pk_off                         (0          )                */
/*                pk_count                                                    */
/*                pk_eof             EOF        (          )                  */
/*                pk_data                                                     */
/*                                                                            */
/******************************************************************************/
#ifdef CONFIG_PROC_FS_FGC                         /*         DD proc          */
int pfgc_log_rdproc( char  *pk_page,
                     char  **pk_start,
                     off_t pk_off,
                     int   pk_count,
                     int   *pk_eof,
                     void  *pk_data )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    int   lk_len;                                 /*       byte               */
    char  *lk_p;                                  /*         Buffer           */
#if DFGC_SW_TEST_DEBUG
    int   ret;
    unsigned short ctrl_data;
#endif /* DFGC_SW_TEST_DEBUG */
    FGC_FUNCIN("\n");

    if(( NULL == pk_page )  ||                    /*       NULL               */
       ( NULL == pk_start ) ||
       ( NULL == pk_eof ))
    {
        lk_len = 0;                               /*       byte    0          */
    }
    else                                          /*       NULL               */
    {
        lk_p = pk_page;                           /* Buffer                   */


	pfgc_log_print(&lk_p, DFGC_LOG_AI_HEX);
	FGC_LOG("%spk_page=%p lk_p=%p\n", __FUNCTION__, pk_page, lk_p);
/*<2040002>#ifdef    SYS_HAVE_IBP                                             *//*<QCIO017>*/
/*<2040002>        lk_p += sprintf( lk_p, "\n" );                             *//*<QCIO017>*/
/*<2040002>        lk_p += sprintf( lk_p, "\tIBP              0x%08x\n",      *//*<QCIO017>*/
/*<2040002>                                          gfgc_ibp_data_num );     *//*<QCIO017>*/
/*<2040002>        lk_p += sprintf( lk_p, "\tIBP          \n\t");             *//*<QCIO017>*/
/*<2040002>        for( count_x = 0;                                   *//*  R*//*<QCIO017>*/
/*<2040002>             count_x < gfgc_ibp_data_num;                          *//*<QCIO017>*/
/*<2040002>             count_x++ )                                           *//*<QCIO017>*/
/*<2040002>        {                                                          *//*<QCIO017>*/
/*<2040002>            lk_p += sprintf( lk_p, "%02x ",gfgc_ibp_data[count_x]);*//*<QCIO017>*/
/*<2040002>        }                                                          *//*<QCIO017>*/
/*<2040002>        lk_p += sprintf( lk_p, "\n" );                             *//*<QCIO017>*/
/*<2040002>#endif *//* SYS_HAVE_IBP */                                          /*<QCIO017>*/
#if DFGC_SW_TEST_DEBUG
        lk_p += sprintf( lk_p,"***** 最新 CONTROL 表示 *****\n" );
        ret = pfgc_i2c_get_controlreg( &ctrl_data );
        if ( ret != DFGC_OK )
        {
            lk_p += sprintf( lk_p, "\tCNTL             I2Cエラー\n" );
        }
        else
        {
            lk_p += sprintf( lk_p, "\tCNTL             0x%04x\n", ctrl_data );
        }
#endif /* DFGC_SW_TEST_DEBUG */

        lk_len = ( lk_p - pk_page );              /*       byte               */

        if( lk_len <= ( pk_off + pk_count ))      /*       byte               */
        {
            *pk_eof = 1;                          /* 1                        */
        }

        if( lk_len > pk_count )                   /*       byte               */
        {                                         /*                          */
            lk_len = pk_count;                    /*       byte               */
                                                  /*                          */
        }

        if( lk_len < 0 )                          /*       byte               */
        {
            lk_len = 0;                           /*       byte    0          */
        }

    }

    return lk_len;                                /*       byte               */
}

/******************************************************************************/
/*                pfgc_log_rdproc_ibp                                <2040002>*/
/*                        DD IBP     proc                                     */
/*                2009.06.24                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                lk_len                                 byte                 */
/*                pk_page                                                     */
/*                pk_start                                                    */
/*                pk_off                         (0          )                */
/*                pk_count                                                    */
/*                pk_eof             EOF        (          )                  */
/*                pk_data                                                     */
/*                                                                            */
/******************************************************************************/
#ifdef    SYS_HAVE_IBP                            /*<2040002>                 */
int pfgc_log_rdproc_ibp( char  *pk_page,          /*<2040002>                 */
                         char  **pk_start,        /*<2040002>                 */
                         off_t pk_off,            /*<2040002>                 */
                         int   pk_count,          /*<2040002>                 */
                         int   *pk_eof,           /*<2040002>                 */
                         void  *pk_data )         /*<2040002>                 */
{                                                 /*<2040002>                 */
    int                  lk_len;                  /*<2040002>                 */
    char                 *lk_p;                   /*<2040002>                 */
    unsigned char        count_x;                 /*<2040002>                 */
                                                  /*<2040002>                 */
                                                  /*<2040002>                 */
    /*                                          *//*<2040002>                 */
    if(( NULL == pk_page )  ||                    /*<2040002>                 */
       ( NULL == pk_start ) ||                    /*<2040002>                 */
       ( NULL == pk_eof ))                        /*<2040002>                 */
    {                                             /*<2040002>                 */
        lk_len = 0;                               /*<2040002>                 */
    }                                             /*<2040002>                 */
    else                                          /*<2040002>                 */
    {                                             /*<2040002>                 */
        /*                                      *//*<2040002>                 */
        lk_p = pk_page;                           /*<2040002>                 */
                                                  /*<2040002>                 */
        lk_p += sprintf( lk_p,                              /*<2040002>       */
                         "***** 電池残量DD (IBP) *****\n" );/*<2040002>       */
        lk_p += sprintf( lk_p, "\n" );                      /*<2040002>       */
                                                            /*<2040002>       */
        lk_p += sprintf( lk_p,                              /*<2040002>       */
                         "\tIBP受信サイズ    0x%08x\n",     /*<2040002>       */
                         gfgc_ibp_data_num );               /*<2040002>       */
        lk_p += sprintf( lk_p, "\tIBP受信データ");          /*<2040002>       */
        for( count_x = 0;                                   /*<2040002>   R   */
             count_x < gfgc_ibp_data_num;                   /*<2040002>       */
             count_x++ )                                    /*<2040002>       */
        {                                                   /*<2040002>       */
            if(( count_x % 16 ) == 0 )                      /*<2040002>       */
            {                                               /*<2040002>       */
                lk_p += sprintf( lk_p, "\n\t");             /*<2040002>       */
            }                                               /*<2040002>       */
            lk_p += sprintf( lk_p,                          /*<2040002>       */
                             "%02x ",                       /*<2040002>       */
                             gfgc_ibp_data[ count_x ]);     /*<2040002>       */
        }                                                   /*<2040002>       */
        lk_p += sprintf( lk_p, "\n" );                      /*<2040002>       */
                                                  /*<2040002>                 */
        /*       byte                           *//*<2040002>                 */
        lk_len = ( lk_p - pk_page );              /*<2040002>                 */
                                                  /*<2040002>                 */
        /*       byte                           *//*<2040002>                 */
        if( lk_len <= ( pk_off + pk_count ))      /*<2040002>                 */
        {                                         /*<2040002>                 */
            *pk_eof = 1;                          /*<2040002>       EOF       */
        }                                         /*<2040002>                 */
                                                  /*<2040002>                 */
        /*       byte                           *//*<2040002>                 */
        /* (                          )         *//*<2040002>                 */
        if( lk_len > pk_count )                   /*<2040002>                 */
        {                                         /*<2040002>                 */
            lk_len = pk_count;                    /*<2040002>                 */
        }                                         /*<2040002>                 */
                                                  /*<2040002>                 */
        /*                                      *//*<2040002>                 */
        if( lk_len < 0 )                          /*<2040002>                 */
        {                                         /*<2040002>                 */
            lk_len = 0;                           /*<2040002>         0       */
        }                                         /*<2040002>                 */
    }                                             /*<2040002>                 */
                                                  /*<2040002>                 */
    return lk_len;                                /*<2040002>                 */
}                                                 /*<2040002>                 */
#endif /* SYS_HAVE_IBP */                         /*<2040002>                 */

#if DFGC_SW_TEST_DEBUG
/* SDRAM       readproc     */
int pfgc_log_rdproc_sdram( char  *pk_page,
                           char  **pk_start,
                           off_t pk_off,
                           int   pk_count,
                           int   *pk_eof,
                           void  *pk_data )
{
	/*--------------------------------------------------------------------*/
	/*                                                                    */
	/*--------------------------------------------------------------------*/
	int   lk_len;                             /*       byte               */
	char  *lk_p;                              /*         Buffer           */

	if(( NULL == pk_page )  ||                /*       NULL               */
	   ( NULL == pk_start ) ||
	   ( NULL == pk_eof ))
	{
		lk_len = 0;                       /*       byte    0          */
	}
	else                                      /*       NULL               */
	{
		lk_p = pk_page;                   /* Buffer                   */

		/*------------------------------------------------------------*/
		/*                                                            */
		/*------------------------------------------------------------*/
		lk_p += sprintf( lk_p, "***** 電池残量DD SDRAM情報 *****\n" );

		pfgc_sdram_dumplog_proc( &lk_p );

		lk_len = ( lk_p - pk_page );      /*       byte               */

		if( lk_len <= ( pk_off + pk_count )) /*       byte               */
		{
			*pk_eof = 1;              /* 1                        */
		}

		if( lk_len > pk_count )           /*       byte               */
		{                                 /*                          */
			lk_len = pk_count;        /*       byte               */
                                                  /*                          */
		}

		if( lk_len < 0 )                  /*       byte               */
		{
			lk_len = 0;               /*       byte    0          */
		}

	}
	return lk_len;                            /*       byte               */
}

/* NAND-                   readproc     */
int pfgc_log_rdproc_nand1( char  *pk_page,
                           char  **pk_start,
                           off_t pk_off,
                           int   pk_count,
                           int   *pk_eof,
                           void  *pk_data )
{
	/*--------------------------------------------------------------------*/
	/*                                                                    */
	/*--------------------------------------------------------------------*/
	int   lk_len;                             /*       byte               */
	char  *lk_p;                              /*         Buffer           */

	if(( NULL == pk_page )  ||                /*       NULL               */
	   ( NULL == pk_start ) ||
	   ( NULL == pk_eof ))
	{
		lk_len = 0;                       /*       byte    0          */
	}
	else                                      /*       NULL               */
	{
		lk_p = pk_page;                   /* Buffer                   */

		/*------------------------------------------------------------*/
		/*                                                            */
		/*------------------------------------------------------------*/
		lk_p += sprintf( lk_p, "***** 電池残量DD NAND情報1 *****\n" );

		pfgc_log_capacity_proc( &lk_p );;

		lk_len = ( lk_p - pk_page );      /*       byte               */

		if( lk_len <= ( pk_off + pk_count )) /*       byte               */
		{
			*pk_eof = 1;              /* 1                        */
		}

		if( lk_len > pk_count )           /*       byte               */
		{                                 /*                          */
			lk_len = pk_count;        /*       byte               */
                                                  /*                          */
		}

		if( lk_len < 0 )                  /*       byte               */
		{
			lk_len = 0;               /*       byte    0          */
		}

	}
	return lk_len;                            /*       byte               */
}

/* NAND-OCV    /               readproc     */
int pfgc_log_rdproc_nand2( char  *pk_page,
                           char  **pk_start,
                           off_t pk_off,
                           int   pk_count,
                           int   *pk_eof,
                           void  *pk_data )
{
	/*--------------------------------------------------------------------*/
	/*                                                                    */
	/*--------------------------------------------------------------------*/
	int   lk_len;                             /*       byte               */
	char  *lk_p;                              /*         Buffer           */
	unsigned char data;                       /*                          */
	int           ret;                        /*                          */

	if(( NULL == pk_page )  ||                /*       NULL               */
	   ( NULL == pk_start ) ||
	   ( NULL == pk_eof ))
	{
		lk_len = 0;                       /*       byte    0          */
	}
	else                                      /*       NULL               */
	{
		lk_p = pk_page;                   /* Buffer                   */

		/*------------------------------------------------------------*/
		/*                                                            */
		/*------------------------------------------------------------*/
		lk_p += sprintf( lk_p, "***** 電池残量DD NAND情報2 *****\n" );

		lk_p += sprintf( lk_p, "\n" );
		ret = cfgdrv_read(D_HCM_E_NO_LVA_FLG, 1, &data);
		if(ret < 0)
		{
			lk_p += sprintf( lk_p, "\t不揮発読み出しエラー\n" );
		}
		else
		{
			lk_p += sprintf( lk_p, "\tLVA非通知フラグ 0x%02x\n", data );
		}

		pfgc_log_ocv_proc( &lk_p );
		pfgc_log_charge_proc( &lk_p );

		lk_len = ( lk_p - pk_page );      /*       byte               */

		if( lk_len <= ( pk_off + pk_count )) /*       byte               */
		{
			*pk_eof = 1;              /* 1                        */
		}

		if( lk_len > pk_count )           /*       byte               */
		{                                 /*                          */
			lk_len = pk_count;        /*       byte               */
			                          /*                          */
		}

		if( lk_len < 0 )                  /*       byte               */
		{
			lk_len = 0;               /*       byte    0          */
		}

	}

	return lk_len;                            /*       byte               */
}

/* SDRAM fgc_log_data      */
static void pfgc_sdram_dumplog_proc( char **ptr )
{
	char  *lk_p;                              /*         Buffer           */
	unsigned short count_log;                 /*                          */
	unsigned short wp;                        /*                          */

	lk_p = *ptr;                              /* Buffer                   */
	wp = gfgc_ctl.remap_log_dump->fgc_log_data[ 0 ].log_wp;

	/* SDRAM : fgc_log_data */
	lk_p += sprintf( lk_p, "\n" );
	lk_p += sprintf( lk_p, "SDRAMデータ表示\n" );
	lk_p += sprintf( lk_p, "\tライトポインタ  0x%04x\n", wp );
	for(count_log = 0; count_log < DFGC_LOG_MAX; count_log++ ) {
		/*       3                                 */
		if((count_log <= 2)
			|| ((wp != 0) && (count_log == wp - 1))
			|| (count_log == DFGC_LOG_MAX - 1)) {
				lk_p += sprintf( lk_p, "\n" );
				lk_p += sprintf( lk_p, "\tLOG_DATA[0x%04x]\n", count_log );
				lk_p += sprintf( lk_p, "\t\tタイムスタンプ  0x%08lx\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].tv.tv_sec );
				lk_p += sprintf( lk_p, "\t\tダンプトリガ    0x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].type );
				lk_p += sprintf( lk_p, "\t\tUI上の充電状態  0x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].chg_sts );
				lk_p += sprintf( lk_p, "\t\tUI上の電池容量  0x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].capacity );
				lk_p += sprintf( lk_p, "\t\t通番            0x%04x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].no );
				lk_p += sprintf( lk_p, "\t\tライトポインタ  0x%04x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].log_wp );
				lk_p += sprintf( lk_p, "\t  ----- レジスタダンプ -----\n" );
				lk_p += sprintf( lk_p, "\t\tCNTL            0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.cntl_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.cntl_l );
				lk_p += sprintf( lk_p, "\t\tTEMP            0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.temp_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.temp_l );
				lk_p += sprintf( lk_p, "\t\tVOLT            0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.volt_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.volt_l );
				lk_p += sprintf( lk_p, "\t\tFLAGS           0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.temp_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.temp_l );
				lk_p += sprintf( lk_p, "\t\tNAC             0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.nac_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.nac_l );
				lk_p += sprintf( lk_p, "\t\tFAC             0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.fac_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.fac_l );
				lk_p += sprintf( lk_p, "\t\tRM              0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.rm_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.rm_l );
				lk_p += sprintf( lk_p, "\t\tFCC             0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.fcc_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.fcc_l );
				lk_p += sprintf( lk_p, "\t\tAI              0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.ai_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.ai_l );
				lk_p += sprintf( lk_p, "\t\tTTE             0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.tte_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.tte_l );
				lk_p += sprintf( lk_p, "\t\tTTF             0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.ttf_h,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.ttf_l );
				lk_p += sprintf( lk_p, "\t\tSOH             0x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.soh );
				lk_p += sprintf( lk_p, "\t\tSOH_STT         0x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.soh_stt );
				lk_p += sprintf( lk_p, "\t\tSOC             0x%02x%02x\n",
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.soc_l,
							gfgc_ctl.remap_log_dump->fgc_log_data[count_log].fgc_regdump.soc_h );
			}
	}
	*ptr = lk_p;                              /* Buffer                   */
	return;
}


/* NAND OCV                */
static void pfgc_log_ocv_proc( char **ptr )
{
	char  *lk_p;                              /*         Buffer           */
	signed int      ret;                      /*                          */
	unsigned short  count;                    /* OCV                      */
	unsigned long   index;                    /* OCV                  index */
	unsigned char   ocv_okng[ DFGC_OCV_SAVE_MAX ]; /* OCV                 */
	unsigned long   ocv_time[ DFGC_OCV_SAVE_MAX ]; /*                     */
	unsigned short  ocv_temp[ DFGC_OCV_SAVE_MAX ]; /* OCV        TEMP     */
	unsigned short  ocv_volt[ DFGC_OCV_SAVE_MAX ]; /* OCV        VOLT     */
	unsigned short  ocv_nac[ DFGC_OCV_SAVE_MAX ];  /* OCV        NAC      */
	unsigned short  ocv_fac[ DFGC_OCV_SAVE_MAX ];  /* OCV        FAC      */
	unsigned short  ocv_rm[ DFGC_OCV_SAVE_MAX ];   /* OCV        RM       */
	unsigned short  ocv_fcc[ DFGC_OCV_SAVE_MAX ];  /* OCV        FCC      */
	unsigned short  ocv_soc[ DFGC_OCV_SAVE_MAX ];  /* OCV        SOC      */

	lk_p = *ptr;                              /* Buffer                   */

	lk_p += sprintf( lk_p, "\n" );
	lk_p += sprintf( lk_p, "NAND OCV測定結果データ表示\n" );

						/*                            */
	ret          = DFGC_CLEAR;
                                                /*                            */
	ret |= Hcm_romdata_read_knl(            /*  C OCV                     */
	                D_HCM_FG_OCV_OKNG,
	                0,
	                &ocv_okng[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_TIMESTAMP,
	                0,
	                ( unsigned char* )&ocv_time[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_TEMP,
	                0,
	                ( unsigned char* )&ocv_temp[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_VOLT,
	                0,
	                ( unsigned char* )&ocv_volt[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_NAC,
	                0,
	                ( unsigned char* )&ocv_nac[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_FAC,
	                0,
	                ( unsigned char* )&ocv_fac[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_RM,
	                0,
	                ( unsigned char* )&ocv_rm[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                           */
	                D_HCM_FG_OCV_FCC,
	                0,
	                ( unsigned char* )&ocv_fcc[0] );

	ret |= Hcm_romdata_read_knl(            /*  C                         */
	                D_HCM_FG_OCV_SOC,
	                0,
	                ( unsigned char* )&ocv_soc[0] );

	ret |= Hcm_romdata_read_knl(            /*  C OCV                     */
	                D_HCM_FG_OCV_COUNT,
	                0,
	                ( unsigned char* )&count );

	if( ret < 0 )                           /*                            */
	{
		lk_p += sprintf( lk_p, "\t不揮発読み出しエラー\n" );
		*ptr = lk_p;
		return;
	}

	lk_p += sprintf( lk_p, "\tOCV実行回数     0x%04x\n", count );
	/*               */
	for(index = 0; index < DFGC_OCV_SAVE_MAX; index++)
	{
		lk_p += sprintf( lk_p, "\n" );
		lk_p += sprintf( lk_p, "\tOCV_DATA[%ld]\n", index );
		lk_p += sprintf( lk_p, "\t\tOCV実行結果            0x%02x\n",
							ocv_okng[ index ] );
		lk_p += sprintf( lk_p, "\t\t時刻                   0x%08lx\n",
							ocv_time[ index ] );
		lk_p += sprintf( lk_p, "\t\t温度                   0x%04x\n",
							ocv_temp[ index ] );
		lk_p += sprintf( lk_p, "\t\t電圧                   0x%04x\n",
							ocv_volt[ index ] );
		lk_p += sprintf( lk_p, "\t\t未補正電池残容量       0x%04x\n",
							ocv_nac[ index ] );
		lk_p += sprintf( lk_p, "\t\t未補正満充電容量       0x%04x\n",
							ocv_fac[ index ] );
		lk_p += sprintf( lk_p, "\t\t補正後の残容量         0x%04x\n",
							ocv_rm[ index ] );
		lk_p += sprintf( lk_p, "\t\t補正後の満充電電池容量 0x%04x\n",
							ocv_fcc[ index ] );
		lk_p += sprintf( lk_p, "\t\t残容量率               0x%04x\n",
							ocv_soc[ index ] );
	}
	*ptr = lk_p;                              /* Buffer                   */
	return;
}

/* NAND capacity      */
static void pfgc_log_capacity_proc( char **ptr )
{
	char  *lk_p;                              /*         Buffer           */
	signed int ret;                           /*                          */
	T_FGC_SOC_LOG nvm;                        /*                          */
	unsigned short count;                     /*                          */
	unsigned long index;                      /*         index            */

	lk_p = *ptr;                              /* Buffer                   */

	lk_p += sprintf( lk_p, "\n" );
	lk_p += sprintf( lk_p, "電池消費履歴表示\n" );

	ret = Hcm_romdata_read_knl(               /*  C                       */
			D_HCM_FG_CONSUMPTION_TIMESTAMP,
			0,
			( unsigned char* )&nvm );

	ret |= Hcm_romdata_read_knl(              /*  C                       */
			D_HCM_FG_CONSUMPTION_COUNT,
			0,
			( unsigned char* )&count );

	if( ret < 0 )                             /*                          */
	{
		lk_p += sprintf( lk_p, "\t不揮発読み出しエラー\n" );
		*ptr = lk_p;
		return;
	}

	lk_p += sprintf( lk_p, "\t記録回数      0x%04x\n", count );
	/*              */
	for(index = 0; index < DFGC_LOG_SOC_MAX; index++)
	{
		lk_p += sprintf( lk_p, "\n" );
		lk_p += sprintf( lk_p, "\tCAPACITY_DATA[Area%ld]\n", index + 1 );
		lk_p += sprintf( lk_p, "\t\t80％時のタイムスタンプ   0x%08lx\n",
							nvm.soh_t[ index ].time_80 );
		lk_p += sprintf( lk_p, "\t\t60％時のタイムスタンプ   0x%08lx\n",
							nvm.soh_t[ index ].time_60 );
		lk_p += sprintf( lk_p, "\t\t40％時のタイムスタンプ   0x%08lx\n",
							nvm.soh_t[ index ].time_40 );
		lk_p += sprintf( lk_p, "\t\t20％時のタイムスタンプ   0x%08lx\n",
							nvm.soh_t[ index ].time_20 );
		lk_p += sprintf( lk_p, "\t\t10％時のタイムスタンプ   0x%08lx\n",
							nvm.soh_t[ index ].time_10 );
	}

	*ptr = lk_p;                              /* Buffer                   */
	return;
}

/* NAND charge        */
static void pfgc_log_charge_proc( char **ptr )
{
	char  *lk_p;                              /*         Buffer           */
	signed int     ret;                       /*                          */
	unsigned short first_time;                /*                          */
	unsigned short count;                     /*                          */
	unsigned long  index;                     /*                     index */
	unsigned long  chg_time[ DFGC_LOG_CHG_MAX ]; /*                       */
	unsigned short chg_fcc[ DFGC_LOG_CHG_MAX ];  /*           FCC         */
	unsigned short chg_soc[ DFGC_LOG_CHG_MAX ];  /*           SOC         */

	lk_p = *ptr;                              /* Buffer                   */

	lk_p += sprintf( lk_p, "\n" );
	lk_p += sprintf( lk_p, "NAND 満充電履歴表示\n" );

	/*                           */
	ret = Hcm_romdata_read_knl(               /*  C                       */
			D_HCM_CHARGE_FIN_1STTIME,
			0,
			( unsigned char* )&first_time );
	/*                           */
	ret  = Hcm_romdata_read_knl(              /*  C                       */
			D_HCM_CHARGE_FIN_TIMESTAMP,
			0,
			( unsigned char* )&chg_time[ 0 ] );
	ret |= Hcm_romdata_read_knl(             /*  C                        */
			D_HCM_CHARGE_FIN_FCC,
			0,
			( unsigned char* )&chg_fcc[ 0 ] );
	ret |= Hcm_romdata_read_knl(             /*  C                        */
			D_HCM_CHARGE_FIN_SOC,
			0,
			( unsigned char* )&chg_soc[ 0 ] );
	ret |= Hcm_romdata_read_knl(             /*  C                        */
			D_HCM_CHARGE_FIN_COUNT,
			0,
			( unsigned char* )&count );
	if( ret < 0 )                            /*                           */
	{
		lk_p += sprintf( lk_p, "\t不揮発読み出しエラー\n" );
		*ptr = lk_p;
		return;
	}

	lk_p += sprintf( lk_p, "\t満充電回数回数     0x%02x\n", count );
	/*               */
	lk_p += sprintf( lk_p, "\t初回満充電月日取得 0x%08x\n", first_time );
	for(index = 0; index < DFGC_OCV_SAVE_MAX; index++)
	{
		lk_p += sprintf( lk_p, "\n" );
		lk_p += sprintf( lk_p, "\tCHARGE_DATA[%ld]\n", index );
		lk_p += sprintf( lk_p, "\t\t満充電月日             0x%08lx\n",
							chg_time[ index ] );
		lk_p += sprintf( lk_p, "\t\t補正後の満充電電池容量 0x%04x\n",
							chg_fcc[ index ] );
		lk_p += sprintf( lk_p, "\t\t残容量率               0x%04x\n",
							chg_soc[ index ] );
	}
	*ptr = lk_p;                              /* Buffer                   */
	return;
}
#endif /* DFGC_SW_TEST_DEBUG */

#endif /* CONFIG_PROC_FS_FGC */                   /*         DD proc          */

void pfgc_log_proc_init( void )
{
	struct proc_dir_entry *ent;

	FGC_FUNCIN("\n");

	pfgc_log_dispbuf = NULL;
	pfgc_log_proc_interval_sec = 0;
	init_timer(&pfgc_log_proc_timer);
	pfgc_log_proc_timer.function = pfgc_log_proc_timer_handler;

	INIT_WORK(&pfgc_log_proc_work, pfgc_log_proc_work_handler);
	pfgc_log_proc_wq = create_singlethread_workqueue("fgc_proc");
	if (!pfgc_log_proc_wq){
		FGC_LOG("Error! %s create_singlethread_workqueue return NULL\n", __FUNCTION__);
	}
	ent = create_proc_entry( "driver/fgc", S_IRUSR | S_IWUSR, NULL ); /* mrc20801 */
	if ( !ent ) {
		FGC_LOG("Error! %s create_proc_entry return NULL\n", __FUNCTION__);
	}
	else {
		ent->write_proc = pfgc_log_proc_write;
		ent->read_proc = pfgc_log_rdproc;
	}
}

void pfgc_log_proc_suspend( void )
{
	int ret;

	FGC_FUNCIN("\n");

	ret = del_timer(&pfgc_log_proc_timer);
	ret = cancel_work_sync(&pfgc_log_proc_work);
	ret = flush_work(&pfgc_log_proc_work);

	return;
}

void pfgc_log_proc_resume( void )
{
	int ret;

	FGC_FUNCIN("\n");

	if(pfgc_log_proc_interval_sec > 0){
		ret = del_timer(&pfgc_log_proc_timer);
		pfgc_log_proc_timer.expires = jiffies + msecs_to_jiffies(pfgc_log_proc_interval_sec * 1000);
		add_timer(&pfgc_log_proc_timer);
	}

	return;
}

static void pfgc_log_proc_work_handler( struct work_struct *work )
{
	char *pStr;
	char strBuf[257];
	int countMax;
	int count;
	unsigned long wakeup;

	FGC_FUNCIN("\n");

	if(pfgc_log_dispbuf == NULL){
		return;
	}

	wakeup = pfgc_info_update( D_FGC_FG_INIT_IND );

	pStr = pfgc_log_dispbuf;
	pfgc_log_print(&pStr, DFGC_LOG_AI_DEC);

	countMax = ((strlen(pfgc_log_dispbuf)/256));
	if(strlen(pfgc_log_dispbuf)%256){
		countMax++;
	}
	strBuf[256] = '\0';
	for( count = 0; count < countMax; count++){
		memcpy( strBuf, &pfgc_log_dispbuf[256 * count], 256 );
		printk("%s", strBuf);
	}
}


static struct t_fgc_log_command_info command_table[] = {
	{ "help"		, pfgc_log_proc_handle_help,	"This help." },
	{ "interval"		, pfgc_log_proc_handle_interval,"interval <sec> 0 is stop." },
};

static int pfgc_log_proc_handle_help( void *arg1, void *arg2, void *arg3 )
{
	int table_num;
	int count;
	struct t_fgc_log_command_info *pTable;

	FGC_FUNCIN("\n");

	table_num = sizeof(command_table) / sizeof(command_table[0]);
	for(count=0; count<table_num; count++){
		pTable = &command_table[count];
		printk("    %d:%s %s\n", count, pTable->strName, pTable->strComment);
	}

	return 0;
}

static int pfgc_log_proc_handle_interval( void *arg1, void *arg2, void *arg3 )
{
	char *strSec;
	int ret;

	strSec = (char *)arg1;

	FGC_LOG("%s %s(%dsecs)\n", __FUNCTION__, strSec, pfgc_log_atoi(strSec));

	strSec = pfgc_log_skip_space(strSec);

	pfgc_log_proc_interval_sec = pfgc_log_atoi(strSec);
	ret = del_timer(&pfgc_log_proc_timer);
	if(pfgc_log_proc_interval_sec > 0){
		pfgc_log_dispbuf = (char *)kmalloc(1024*2,GFP_KERNEL);
		pfgc_log_proc_timer.expires = jiffies + msecs_to_jiffies(pfgc_log_proc_interval_sec * 1000);
		add_timer(&pfgc_log_proc_timer);
	}
	else{
		kfree(pfgc_log_dispbuf);
		pfgc_log_dispbuf = NULL;
	}

	return 0;
}

static void pfgc_log_proc_timer_handler( unsigned long data )
{
	int ret;

	queue_work(pfgc_log_proc_wq, &pfgc_log_proc_work);

	ret = del_timer(&pfgc_log_proc_timer);
	pfgc_log_proc_timer.expires = jiffies + msecs_to_jiffies(pfgc_log_proc_interval_sec * 1000);
	add_timer(&pfgc_log_proc_timer);
}

static char *pfgc_log_skip_space( char *aryInputBuf )
{
	char *pStr;
	pStr = aryInputBuf;
	while( *pStr == ' ' ){
		pStr++;
	}
	return pStr;
}

static int pfgc_log_atoi( char *strValue )
{
	int value;

	value = 0;

	while((*strValue >= '0') && (*strValue <= '9')){
		value = ( value * 10 ) + (*strValue - '0');
		strValue++;
	}

	return value;
}

static size_t pfgc_log_strlen_word( char *aryInputBuf )
{
	char *pStr;

	pStr = aryInputBuf;
	while(*pStr != ' '){
		if(*pStr == '\0'){
			break;
		}
		pStr++;
	}
	return (size_t)(pStr - aryInputBuf);
}

static struct t_fgc_log_command_info *debug_search_command( char *aryInputBuf, struct t_fgc_log_command_info *pTable, int table_num )
{
	int count;
	struct t_fgc_log_command_info *pInfo;
	int commandNameSize;
	int inputNameSize;

	inputNameSize = pfgc_log_strlen_word(aryInputBuf);

	for(count=0; count<table_num; count++){
		pInfo = &pTable[count];

		commandNameSize = strlen(pInfo->strName);

		if(commandNameSize != inputNameSize){
			FGC_LOG("%d : %d %d\n", count, commandNameSize, inputNameSize);
			continue;
		}
		if(!strncmp(pInfo->strName, aryInputBuf,commandNameSize)){
			return pInfo;
		}
		else{
			FGC_LOG("%d : [%s]<-comp->[%s] size=%d\n", count, pInfo->strName, aryInputBuf, commandNameSize);
		}
	}
	return NULL;
}


static int pfgc_log_proc_write( struct file *file, const char *buffer, unsigned long count, void *data ) /* mrc24602 */
{
	int copy_len = 0;

	struct t_fgc_log_command_info *pInfo;
	int ret;

	if(buffer == NULL){
		FGC_LOG( KERN_INFO "%s : buffer is NULL\n", __FUNCTION__ );
		return -EFAULT;
	}

	if(count > ( DFGC_LOG_PROC_MAXBUF )){
		copy_len = DFGC_LOG_PROC_MAXBUF;
	}
	else{
		copy_len = count;
	}

	pfgc_log_procbuf[ copy_len ]='\0';

	memcpy(pfgc_log_procbuf, buffer, copy_len);

	if(pfgc_log_procbuf[copy_len - 1] == '\n'){
		pfgc_log_procbuf[copy_len - 1] = '\0';
	}

	pInfo = debug_search_command(pfgc_log_procbuf, command_table, sizeof(command_table) / sizeof(command_table[0]));
	if(pInfo == NULL){
		pInfo = &command_table[0];
	}

	if(pInfo->pFunc){
		ret = pInfo->pFunc(pfgc_log_procbuf + strlen(pInfo->strName) + 1, NULL, NULL);
	}
	else{
		printk("%s command have no function.\n", pInfo->strName);
	}

	return copy_len;
}

