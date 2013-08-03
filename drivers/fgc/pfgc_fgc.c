/*
 * drivers/fgc/pfgc_fgc.c
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

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
static int pfgc_fgc_aiget( signed short *ai_data );
static int pfgc_fgc_fccget( signed int *fcc_data ); /* IDPower */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
const unsigned char sfgc_charger_tbl[ 27 ] =     /*                           */
{
#if 0  // SlimID Mod Start
    DCTL_CHG_NO_CHARGE,                          /* [ 0]                      */
    DCTL_CHG_CONST_VOLT,                         /* [ 1]                      */
    DCTL_CHG_CHARGE,                             /* [ 2]                      */
    DCTL_CHG_ACADP_OVER,                         /* [ 3] ACADP                */
    DCTL_CHG_CHARGE,                             /* [ 4]                      */
    DCTL_CHG_CHARGE,                             /* [ 5]                      */
    DCTL_CHG_VOLT_ERR,                           /* [ 6]                      */
    DCTL_CHG_CHARGE,                             /* [ 7]                      */
    DCTL_CHG_TEMP_ERR,                           /* [ 8]                      */
    DCTL_CHG_TEMP_ERR,                           /* [ 9]                      */
    DCTL_CHG_CHARGE,                             /* [10]                      */
    DCTL_CHG_TEMP_ERR,                           /* [11]                      */
    DCTL_CHG_TEMP_ERR,                           /* [12]                      */
    DCTL_CHG_CHARGE,                             /* [13]                      */
    DCTL_CHG_TEMP_ERR,                           /* [14]                      */
    DCTL_CHG_TEMP_ERR,                           /* [15]                      */
    DCTL_CHG_CHARGE,                             /* [16]                      */
    DCTL_CHG_CHARGE,                             /* [17]                      */
    DCTL_CHG_CHARGE,                             /* [18]                      */
    DCTL_CHG_CHARGE_FIN,                         /* [19]                      */
    DCTL_CHG_EXTPWR,                             /* [20]                      */
    DCTL_CHG_CHARGE,                             /* [21]                      */
    DCTL_CHG_CHARGE,                             /* [22]                      */
    DCTL_CHG_CHARGE,                             /* [23]                      */
    DCTL_CHG_CHARGE_FIN,                         /* [24]                      */
    DCTL_CHG_NO_CHARGE,                          /* [25]                      */
    DCTL_CHG_CONST_VOLT                          /* [26]                      */
#else  // SlimID Mod
    DCTL_CHG_NO_CHARGE,                          /* [ 0]                      */
    DCTL_CHG_CONST_VOLT,                         /* [ 1]                      */
    DCTL_CHG_CHARGE,                             /* [ 2]                      */
    DCTL_CHG_ACADP_OVER,                         /* [ 3] ACADP                */
    DCTL_CHG_NO_CHARGE,                          /* [ 4]                      */
    DCTL_CHG_CHARGE,                             /* [ 5]                      */
    DCTL_CHG_VOLT_ERR,                           /* [ 6]                      */
    DCTL_CHG_NO_CHARGE,                          /* [ 7]                      */
    DCTL_CHG_TEMP_ERR,                           /* [ 8]                      */
    DCTL_CHG_TEMP_ERR,                           /* [ 9]                      */
// Jessie del    DCTL_CHG_CHARGE,                             /* [10]                      */
// Jessie del    DCTL_CHG_TEMP_ERR,                           /* [11]                      */
    DCTL_CHG_WIRELESS_CHARGE,                    /* [10]                      */// Jessie add
    DCTL_CHG_WIRELESS_CHG_FIN,                   /* [11]                      */// Jessie add
    DCTL_CHG_TEMP_ERR,                           /* [12]                      */
    DCTL_CHG_CHARGE,                             /* [13]                      */
    DCTL_CHG_TEMP_ERR,                           /* [14]                      */
    DCTL_CHG_TEMP_ERR,                           /* [15]                      */
    DCTL_CHG_CHARGE,                             /* [16]                      */
    DCTL_CHG_CHARGE,                             /* [17]                      */
    DCTL_CHG_CHARGE,                             /* [18]                      */
    DCTL_CHG_CHARGE_FIN,                         /* [19]                      */
    DCTL_CHG_EXTPWR,                             /* [20]                      */
    DCTL_CHG_CHARGE,                             /* [21]                      */
    DCTL_CHG_CHARGE,                             /* [22]                      */
    DCTL_CHG_CHARGE,                             /* [23]                      */
    DCTL_CHG_CHARGE_FIN,                         /* [24]                      */
    DCTL_CHG_NO_CHARGE,                          /* [25]                      */
    DCTL_CHG_CONST_VOLT                          /* [26]                      */
#endif // SlimID Mod End
};

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*                pfgc_fgc_mstate                                             */
/*                        DD    R/W                                           */
/*                2008.10.21                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                unsigend long type                                          */
/*                void  *data                                                 */
/*                                                                            */
/******************************************************************************/
int pfgc_fgc_mstate( unsigned long type, void *data )
{
/*<1905359>    unsigned long wakeup;           *//*                           */
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */
/* IDPower    T_FGC_CHGSTS_TBL *chgsts_info;              *//* <SlimID>                  */
    unsigned char chgsts;                        /*                           */
#if DFGC_SW_DELETED_FUNC
    unsigned char ocv_chgsts;                    /*<3230450>OCV               */
#endif /* DFGC_SW_DELETED_FUNC */
    unsigned long flags;                         /*<1905359>                  */
    int*  valp;
    int   ret;
	signed short ch_ai; /* IDPower */

	DEBUG_FGIC(("pfgc_fgc_mstate start, type = %lx\n", type));

    MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0000 );

    if( data == NULL )                           /*         NULL              */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_FGC_MSTATE | 0x0001 );
        MFGC_SYSERR_DATA_SET(
                    CSYSERR_ALM_RANK_B,
                    DFGC_SYSERR_MSTATE_NULL,
                    ( DFGC_FGC_MSTATE | 0x0001 ),
                    type,
                    syserr_info );
        pfgc_log_syserr( &syserr_info );         /*  V SYSERR                 */
        return -1;
    }

    switch( type )
    {
        case D_FGC_CHG_INT_IND:                  /*                           */
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0002 );
/* IDPower #if 1   *//* SlimID */
#if 0 /* IDPower */
            chgsts_info = (T_FGC_CHGSTS_TBL* )data;
            gfgc_ctl.dev1_chgsts                 /*                           */
                = chgsts_info->chgsts;
            chgsts                               /*                           */
                = sfgc_charger_tbl[ chgsts_info->chgsts ];
            /*                              */
            memcpy((void *)gfgc_ctl.charge_reg,
                   (void *)chgsts_info->chargereg,
                   DCTL_CHG_STSREG_MAX);
#else
            gfgc_ctl.dev1_chgsts                 /*                           */
                = *(( unsigned char * )data );
            chgsts                               /*                           */
                = sfgc_charger_tbl[ *(( unsigned char *)data) ];
#endif   /* SlimID */
                                                 /*                           */
            FGC_CHG("type = 0x%08lx, data = 0x%02x\n", type, gfgc_ctl.dev1_chgsts);
// Jessie del start
//            if(( gfgc_ctl.chg_sts                /*                           */
//                        != DCTL_CHG_CHARGE )
/*<3131028>                || ( chgsts*/
//                && ( chgsts                      /*<3131028>                  */
//                            == DCTL_CHG_CHARGE ))
// Jessie del end
// Jessie add start
            if((( gfgc_ctl.chg_sts                /*     */
                        != DCTL_CHG_CHARGE )      &&
                ( gfgc_ctl.chg_sts
                        != DCTL_CHG_WIRELESS_CHARGE )
                )
                &&(
                          ( chgsts                      /*<3131028>                  */
                            == DCTL_CHG_CHARGE )  ||
                          ( chgsts
                            == DCTL_CHG_WIRELESS_CHARGE )
                )
// Jessie add end
            )
            {
                MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0003 );
                gfgc_ctl.chg_start = DFGC_ON;    /*                           */
            }
#if DFGC_SW_DELETED_FUNC
            /*<3230450>-------------------------------------------------------*/
            /*<3230450>MAW                                    OCV             */
            /*<3230450>                                                       */
            /*<3230450>MAW                                  MAW               */
            /*<3230450>-------------------------------------------------------*/
            ocv_chgsts =                         /*<3230450>OCV               */
                ( gfgc_ctl.remap_ocv->ocv_ctl_flag /*<3230450>OCV             */
                  & DFGC_OCV_CHGMASK );          /*<3230450>                  */
            ocv_chgsts =                         /*<3230450>                  */
                sfgc_charger_tbl[ ocv_chgsts ];  /*<3230450>                  */
            if(( ocv_chgsts                      /*<3230450>                  */
                        == DCTL_CHG_CONST_VOLT ) /*<3230450>                  */
                && ( chgsts                      /*<3230450>                  */
                        != DCTL_CHG_CONST_VOLT ))/*<3230450>                  */
            {                                    /*<3230450>                  */
                MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0010 ); /*<3230450>     */
                pfgc_fgc_ocv_request();          /*<3230450>OCV               */
                system_reset( 1 );               /*<3230450>                  */
            }                                    /*<3230450>                  */
/*<3230450>            if(( gfgc_ctl.chg_sts                *//*<3040003>OCV               */
            else if(( gfgc_ctl.chg_sts           /*<3230450>MAW               */
                        == DCTL_CHG_CONST_VOLT ) /*<3040003>                  */
                && ( chgsts                      /*<3040003>                  */
                        != DCTL_CHG_CONST_VOLT ))/*<3040003>                  */
            {                                    /*<3040003>                  */
                MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0004 );
                pfgc_fgc_ocv_request();          /*<3040003>OCV               */
            }                                    /*<3040003>                  */
#endif /* DFGC_SW_DELETED_FUNC */
            gfgc_ctl.chg_sts = chgsts;           /*                           */

// Jessie del start
//            if(( gfgc_ctl.chg_sts                /*                           */
//                            != DCTL_CHG_CHARGE )
// Jessie del end
// Jessie add start
            if((( gfgc_ctl.chg_sts                /*                           */
                            != DCTL_CHG_CHARGE)        &&
               ( gfgc_ctl.chg_sts
                            != DCTL_CHG_WIRELESS_CHARGE))
// Jessie add end
                || ( gfgc_ctl.dev1_chgsts        /*                           */
                            == BAT_CHARGEFIN2 ))
            {
                MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0005 );
                gfgc_batt_info.batt_status       /*                           */
                               = BAT_ST_INT;
            }
            else
            {
                MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0006 );
                gfgc_batt_info.batt_status       /*                           */
                               = BAT_ST_EXTWB;
            }
            break;

        case D_FGC_RF_CLASS_IND:                 /* RF class                  */
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0007 );
            gfgc_ctl.dev1_class                  /* RF class                  */
                = *(( unsigned char * )data );
            if(( gfgc_ctl.dump_enable            /*                           */
                    & DFGC_DUMP_ENABLE_CLASS )
                != DFGC_DUMP_ENABLE_CLASS )
            {
                MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0008 );
                return -1;
            }
            break;

        case D_FGC_MSTATE_IND:                   /*                           */
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0009 );
            switch( *(( unsigned long * )data ))
            {
                case DCTL_STATE_POWON:           /*     ON                    */
                case DCTL_STATE_INTMIT:          /*                           */
                case DCTL_STATE_CALL:            /*                           */
                case DCTL_STATE_TV_CALL:         /* TV                        */
                case DCTL_STATE_TEST:            /*                           */
                case DCTL_STATE_TEST2:           /*             2             */
                case DCTL_STATE_POWOFF:          /*     OFF                   */
                case DCTL_STATE_INTMIT_2G:       /* 2G                        */
                case DCTL_STATE_CALL_2G:         /* 2G                        */
                case DCTL_STATE_CALL_IRAT:       /*                           */
                case DCTL_STATE_TEST_2G:         /* 2G                        */
                case DCTL_STATE_TEST2_2G:        /* 2G            2           */
                    MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x000A );
                                                 /*                           */
                    gfgc_ctl.mac_state = *(( unsigned long *)data);
                    break;

                default:                         /*                           */
                    MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x000B );
                    break;
            }
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x000C );
            break;

        case D_FGC_ECO_IND:                      /*<QEIO022>      ECO         */
            MFGC_RDPROC_PATH(                    /*<QEIO022>                  */
                DFGC_FGC_MSTATE | 0x000F );      /*<QEIO022>                  */
                                                 /*<QEIO022>                  */
            pfgc_log_eco((unsigned long *)data );/*<QEIO022>  V               */
                                                 /*<QEIO022>                  */
            MFGC_RDPROC_PATH(                    /*<QEIO022>                  */
                DFGC_FGC_MSTATE | 0xFFFF );      /*<QEIO022>                  */
            return 0;                            /*         Thread            */

        case D_FGC_CAPACITY_REQ:
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0011 );
            valp  = (int*)data;
            *valp = gfgc_ctl.get_capacity();
	        DEBUG_FGIC(("### FGC ### soc = %d\n", *valp));
            return 0;

        case D_FGC_CURRENT_REQ:
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0012 );

/* IDPower            ret = pfgc_fgc_aiget((signed short *)data ); */
/* IDPower start */
			ret = pfgc_fgc_aiget(&ch_ai);
			if(ch_ai < 0){
				*(signed int*)data = ( signed int )(0xFFFF0000 | ch_ai);
			}
			else{
				*(signed int*)data = ( signed int )(0xFFFF & ch_ai);
			}
	        DEBUG_FGIC(("### FGC ### ret = %d, ai = %d(0x%08x)\n", ret, *(signed int *)data, *(signed int *)data));
/* IDPower end */
            FGC_CHG("type = 0x%08lx, data = %d(0x%04x)\n", type, *(signed short *)data, *(unsigned short *)data);
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0xFFFF );
            return ret;
/* IDPower start */
        case D_FGC_FCC_REQ:
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x0013 );

            ret = pfgc_fgc_fccget((signed int *)data );

            DEBUG_FGIC(("### FGC ### ret = %d, fcc = %d(0x%08x)\n", ret, *(signed int *)data, *(signed int *)data));
            FGC_CHG("type = 0x%08lx, data = %d(0x%08x)\n", type, *(signed int *)data, *(unsigned int *)data);
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0xFFFF );
            return ret;
/* IDPower end */

        default:
            MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x000D );
            break;
    }

                                                 /*                           */
/*<1905359>    wakeup = pfgc_info_update(( unsigned char )type );             *//*  C                  */

/*<1905359>    if( wakeup == DFGC_ON )                                        *//*                           */
    if( gfgc_ctl.update_enable == DFGC_ON )
    {
         MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0x000E );
/*<1905359>        gfgc_ctl.flag |= type;                                     */
/*<1905359>        wake_up_interruptible( &gfgc_fginfo_wait );                *//*  V                      */
        spin_lock_irqsave( &gfgc_spinlock_if, flags );             /*<1905359>*/
        gfgc_kthread_flag |= type;                                 /*<1905359>*/
        spin_unlock_irqrestore( &gfgc_spinlock_if, flags );        /*<1905359>*/

        wake_up( &gfgc_task_wq );                           /*  V*//*<1905359>*/
    }

    MFGC_RDPROC_PATH( DFGC_FGC_MSTATE | 0xFFFF );

	DEBUG_FGIC(("pfgc_fgc_mstate end\n"));
    return 0;
}

/******************************************************************************/
/**
 * @fn     pfgc_fgc_volt_send ( void )
 * @brief  return value of voltage
 * @date   2012.10.17
 * @author Noriyuki.Takahashi
 * @retval Vaule of voltage.
 * @note   Note<br>
 *         create new.<br>
*/
/******************************************************************************/
unsigned short pfgc_fgc_volt_send(void)
{
    return gfgc_batt_info.volt;
}

/******************************************************************************/
/*                pfgc_fgc_regget                                             */
/*                FG-IC                                                       */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                unsigend char type                                          */
/*                TCOM_RW_DATA  *com_acc_rwdata                               */
/*                                                                            */
/******************************************************************************/
int pfgc_fgc_regget( unsigned char type,
                      TCOM_RW_DATA  *com_acc_rwdata )
{
    TCOM_ID           com_acc_id;
    TCOM_FUNC         com_acc_ret;
    T_FGC_SYSERR_DATA syserr_info;
    unsigned long     data;
    unsigned long     cnt;
    signed long       timeout;                   /*                           */
    TCOM_RW_DATA      com_acc_write;             /*<4210025>     WRITE        */
    unsigned long     accs_option;               /*<4210025> I2C              */
    int               ret;

    FGC_FUNCIN("\n");

    MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0000 );

    ret = 0;

    for( cnt = 0; cnt < DFGC_FGC_COM_WAIT_CNT; cnt++ ) /*  D                  */
    {
        com_acc_ret = pcom_acc_open(             /*  C                 open   */
                        DCOM_DEV_DEVICE4,
                        &com_acc_id,
                        NULL );

        if( com_acc_ret == DCOM_OK )             /*       OK                  */
        {
            MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0001 );
#ifndef DVDV_FGC_I2C_RATE_CONST                  /*                           */
            data = DCOM_RATE400K;                /* 400k                      */
            com_acc_ret = pcom_acc_ioctl(        /*  C                        */
                                com_acc_id,
                                DCOM_DEV_DEVICE4,
                                DCOM_IOCTL_RATE,
                                &data );
            if( com_acc_ret != DCOM_OK )         /*       OK                  */
            {
                                                 /*                           */
                                                 /*                           */
                MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0006 );
                MFGC_RDPROC_ERROR( DFGC_FGC_REGGET | 0x0006 );
                MFGC_SYSERR_DATA_SET2(
                        CSYSERR_ALM_RANK_B,
                        DFGC_SYSERR_COM_IOCTL_ERR,
                        ( DFGC_FGC_REGGET | 0x0006 ),
                        DCOM_DEV_DEVICE4,
                        com_acc_ret,
                        syserr_info );
                pfgc_log_syserr( &syserr_info ); /*  V SYSERR             */
                ret = -1;
            }
#endif /* DVDV_FGC_I2C_RATE_CONST */

            accs_option = DCOM_ACCESS_MODE1 |    /* write Control() register  */
                          DCOM_SEQUENTIAL_ADDR;
            com_acc_write.option = &accs_option; /*<4210025> Normal           */
            com_acc_write.size   = 1;            /*<4210025> 1byte            */
            MFGC_FGRW_DTSET( com_acc_write,      /*<4210025> CTRL_STATUS Cmd  */
                             CtrlRegAdr,         /*<4210025>                  */
                             CtrlStsCmd );       /*<4210025>                  */
                                                 /*<4210025>                  */
            com_acc_ret = pcom_acc_write(        /*<4210025>  C     ACC WRITE */
                            com_acc_id,          /*<4210025>                  */
                            DCOM_DEV_DEVICE4,    /*<4210025>                  */
                            &com_acc_write );    /*<4210025>                  */
            if( com_acc_ret != DCOM_OK )         /*<4210025> NG               */
            {                                    /*<4210025>                  */
                pfgc_log_i2c_error(PFGC_FGC_REGGET + 0x01, 0);
                MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0007 );   /*<4210025>   */
                MFGC_RDPROC_ERROR( DFGC_FGC_REGGET | 0x0007 );  /*<4210025>   */
                MFGC_SYSERR_DATA_SET2(           /*<4210025>                  */
                        CSYSERR_ALM_RANK_A,      /*<4210025>                  */
                        DFGC_SYSERR_COM_READ_ERR,/*<4210025>                  */
                        ( DFGC_FGC_REGGET | 0x0007 ),           /*<4210025>   */
                        DCOM_DEV_DEVICE4,        /*<4210025>                  */
                        com_acc_ret,             /*<4210025>                  */
                        syserr_info );           /*<4210025>                  */
                pfgc_log_syserr( &syserr_info ); /*<4210025>  V SYSERR        */
                ret = -1;
            }                                    /*<4210025>                  */

            com_acc_ret = pcom_acc_read(         /*  C                 read   */
                            com_acc_id,
                            DCOM_DEV_DEVICE4,
                            com_acc_rwdata );
            if( com_acc_ret != DCOM_OK )         /*       OK                  */
            {
                pfgc_log_i2c_error(PFGC_FGC_REGGET + 0x02, 0);
                MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0002 );
                MFGC_RDPROC_ERROR( DFGC_FGC_REGGET | 0x0002 );
                MFGC_SYSERR_DATA_SET2(
                        CSYSERR_ALM_RANK_A,
                        DFGC_SYSERR_COM_READ_ERR,
                        ( DFGC_FGC_REGGET | 0x0002 ),
                        DCOM_DEV_DEVICE4,
                        com_acc_ret,
                        syserr_info );
                pfgc_log_syserr( &syserr_info ); /*  V SYSERR                 */
                ret = -1;
            }

            com_acc_ret = pcom_acc_close(        /*  C                 close  */
                            com_acc_id );
            if( com_acc_ret != DCOM_OK )         /*       OK                  */
            {
                MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0003 );
            }

            break;
        }
        else                                     /*       OK                  */
        {
            MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0004 );
            set_current_state( TASK_INTERRUPTIBLE ); /*  V                    */
                                                 /*          WAIT             */
            timeout = schedule_timeout( DFGC_FGC_COM_WAIT_TIMEOUT ); /*  C    */

        }
    }

/*<3100735>    if( cnt > DFGC_FGC_COM_WAIT_CNT )            *//* 100ms * 100      open      */
    if( cnt >= DFGC_FGC_COM_WAIT_CNT )           /*<3100735>         *       open      */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0x0005 );
        MFGC_RDPROC_ERROR( DFGC_FGC_REGGET | 0x0005 );
        MFGC_SYSERR_DATA_SET2(                   /* SYSERR                    */
                    CSYSERR_ALM_RANK_A,
                    DFGC_SYSERR_COM_OPEN_ERR,
                    ( DFGC_FGC_REGGET | 0x0005 ),
                    DCOM_DEV_DEVICE4,
                    com_acc_ret,
                    syserr_info );
        pfgc_log_syserr( &syserr_info );         /*  V SYSERR                 */
        ret = -1;
    }

    MFGC_RDPROC_PATH( DFGC_FGC_REGGET | 0xFFFF );
    return ret;
}

/******************************************************************************/
/*                pfgc_fgc_nvm_get                                            */
/*                                                                            */
/*                2008.10.17                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
int pfgc_fgc_nvm_get( void )
{
    int ret;

    MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0000 );
#if DFGC_SW_DELETED_FUNC
    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_MTF_REST_OF_BATT_1,
            0,
            ( void * )&gfgc_ctl.lvl_2 );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0001 );
        gfgc_ctl.lvl_2 = DFGC_LVL_2_INIT;        /* MTF_REST_OF_BATT1(3G)     */
    }

    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_MTF_REST_OF_BATT_2,
            0,
            ( void * )&gfgc_ctl.lvl_1 );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0002 );
        gfgc_ctl.lvl_1 = DFGC_LVL_1_INIT;        /* MTF_REST_OF_BATT2(3G)     */
    }

    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_FG_PARAM1,
            0,
            ( void * )&gfgc_ctl.lva_3g );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0003 );
        gfgc_ctl.lva_3g = DFGC_LVA_3G_INIT;      /* LVA      3G               */
        gfgc_ctl.lva_2g = DFGC_LVA_2G_INIT;      /* LVA      2G               */
    }
                                                 /*<3210049>                  */
    ret = Hcm_romdata_read_knl(                  /*<3210049>  C               */
            D_HCM_PID_BATT_ALARM,                /*<3210049>              3G  */
            0,                                   /*<3210049>                  */
            ( void * )&gfgc_ctl.alarm_3g );      /*<3210049>                  */
    if( ret != 0 )                               /*<3210049>                  */
    {                                            /*<3210049>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0006 ); /*<3210049>            */
        gfgc_ctl.alarm_3g = DFGC_LVA_3G_INIT;    /*<3210049>              3G  */
    }                                            /*<3210049>                  */
                                                 /*<3210049>                  */
    ret = Hcm_romdata_read_knl(                  /*<3210049>  C               */
            D_HCM_PID_BATT_ALARM5,               /*<3210049>              2G  */
            0,                                   /*<3210049>                  */
            ( void * )&gfgc_ctl.alarm_2g );      /*<3210049>                  */
    if( ret != 0 )                               /*<3210049>                  */
    {                                            /*<3210049>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0007 ); /*<3210049>            */
        gfgc_ctl.alarm_2g = DFGC_LVA_2G_INIT;    /*<3210049>              2G  */
    }                                            /*<3210049>                  */
#endif /* DFGC_SW_DELETED_FUNC *//*<Conan>*/
                                                 /*<3131029>                  */
    ret = Hcm_romdata_read_knl(                  /*<3131029>  C               */
            D_HCM_FG_OCV_FLAG,                   /*<3131029>OCV               */
            0,                                   /*<3131029>                  */
            ( void * )&gfgc_ctl.ocv_enable );    /*<3131029>                  */
    if( ret != 0 )                               /*<3131029>                  */
    {                                            /*<3131029>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0008 ); /*<3131029>            */
        gfgc_ctl.ocv_enable = DFGC_ON;           /*<3131029>        ON        */
        return ret;
    }                                            /*<3131029>                  */

    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_FG_PARAM2,
            0,
/*<Conan>            ( void * )&gfgc_ctl.dump_enable );*/
            ( void * )&gfgc_ctl.soh_n );                                        /*<Conan>*/

    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0004 );
/*<Conan>        gfgc_ctl.dump_enable*/                     /* RF Class                  */
/*<Conan>            = DFGC_DUMP_ENABLE_INIT;*/             /*       ON                  */
/*<Conan>        gfgc_ctl.charge_timer*/                    /* FG-IC                     */
/*<Conan>            = DFGC_CHARGE_TIMER_INIT;*/
        gfgc_ctl.soh_n = DFGC_SOH_N_INIT;        /*                           */
        gfgc_ctl.soh_init = DFGC_SOH_INIT_INIT;  /*                           */
        gfgc_ctl.soh_level = DFGC_SOH_LEVEL_INIT;/*                 (        )*//* -> #define DFGC_SOH_LEVEL_INIT 0x03                      */
        gfgc_ctl.soh_down = DFGC_SOH_DOWN_INIT;  /*                           *//* -> #define DFGC_SOH_DOWN_INIT  0x50                           *//*<Conan>*/
        gfgc_ctl.soh_bad = DFGC_SOH_BAD_INIT;	 /* #define DFGC_SOH_BAD_INIT 0x41                        *//*<Conan>*/
        gfgc_ctl.batt_capa_design = DFGC_BATT_CAPA_DESIGN_INIT; /*              *//*<Conan>*/
        gfgc_ctl.soc_cycle_val = DFGC_SOC_CYCLE_VAL_INIT;       /*                           SOC   *//*<Conan>*/
    }

/*<Conan>#endif *//* DFGC_SW_DELETED_FUNC */
    ret = Hcm_romdata_read_knl(                  /*  C                        *//*<Conan>*/
            D_HCM_E_SDETERIO_FTEMP,                                             /*<Conan>*/
            0,                                                                  /*<Conan>*/
            ( void * )&gfgc_ctl.soh_degra_val_tbl );                            /*<Conan>*/

    if( ret != 0 )                               /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
                                                 /*                           *//*<Conan>*/
        gfgc_ctl.soh_degra_val_tbl[0][0] = DFGC_SOH_DEGRA_VAL_TBL00;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[0][1] = DFGC_SOH_DEGRA_VAL_TBL01;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[0][2] = DFGC_SOH_DEGRA_VAL_TBL02;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[1][0] = DFGC_SOH_DEGRA_VAL_TBL10;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[1][1] = DFGC_SOH_DEGRA_VAL_TBL11;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[1][2] = DFGC_SOH_DEGRA_VAL_TBL12;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[2][0] = DFGC_SOH_DEGRA_VAL_TBL20;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[2][1] = DFGC_SOH_DEGRA_VAL_TBL21;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[2][2] = DFGC_SOH_DEGRA_VAL_TBL22;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[3][0] = DFGC_SOH_DEGRA_VAL_TBL30;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[3][1] = DFGC_SOH_DEGRA_VAL_TBL31;            /*<Conan>1112XX*/
        gfgc_ctl.soh_degra_val_tbl[3][2] = DFGC_SOH_DEGRA_VAL_TBL32;            /*<Conan>1112XX*/

    }                                                                           /*<Conan>*/

    ret = Hcm_romdata_read_knl(                  /*  C                        *//*<Conan>*/
            D_HCM_E_DEGRADA_CONST_CYCLE,                                        /*<Conan>*/
            0,                                                                  /*<Conan>*/
            ( void * )&gfgc_ctl.soh_cycle_val );                                /*<Conan>*/

    if( ret != 0 )                               /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
        gfgc_ctl.soh_cycle_val = DFGC_SOH_CYCLE_VAL_INIT; /*                  *//*<Conan>*/
    }                                                                           /*<Conan>*/

    ret = Hcm_romdata_read_knl(                  /*  C                        *//*<Conan>*/
            D_HCM_E_SDETERIO_FVOL,                                              /*<Conan>*/
            0,                                                                  /*<Conan>*/
            ( void * )&gfgc_ctl.soh_degra_volt );                               /*<Conan>*/

    if( ret != 0 )                               /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
        gfgc_ctl.soh_degra_volt[0] = DFGC_HELTE_VOLT_LEVEL_0;     /*                     A     *//*<Conan>*/
        gfgc_ctl.soh_degra_volt[1] = DFGC_HELTE_VOLT_LEVEL_1;     /*                     B     *//*<Conan>*/
        gfgc_ctl.soh_degra_volt[2] = DFGC_HELTE_VOLT_LEVEL_2;     /*                     C     *//*<Conan>*/
    }                                                                           /*<Conan>*/

    ret = Hcm_romdata_read_knl(                  /*  C                        *//*<Conan>*/
            D_HCM_E_H24_SAVE,                                                   /*<Conan>*/
            0,                                                                  /*<Conan>*/
            ( void * )&gfgc_ctl.save_degra_batt_measure_val );                  /*<Conan>*/

    if( ret != 0 )                               /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
        gfgc_ctl.save_degra_batt_measure_val =   /*                           *//*<Conan>*/
            DFGC_SOH_VAL_INIT;                                                  /*<Conan>*/
        gfgc_ctl.save_soh_health =               /*                           *//*<Conan>*/
            DFGC_SOH_VAL_INIT;                                                  /*<Conan>*/
        gfgc_ctl.save_degra_batt_estimate_va =   /*                           *//*<Conan>*/
            DFGC_FGC_MAX_PERCENT;                                               /*<Conan>*/
        gfgc_ctl.save_cycle_degra_batt_capa =    /*                           *//*<Conan>*/
            DFGC_FGC_MAX_PERCENT;                                               /*<Conan>*/
        gfgc_ctl.save_soh_degra_batt_capa =      /*                           *//*<Conan>*/
            DFGC_FGC_MAX_PERCENT;                                               /*<Conan>*/
        gfgc_ctl.save_soh_cycle_cnt =            /*                           *//*<Conan>*/
            DFGC_SOH_VAL_INIT;                                                  /*<Conan>*/
    }                                                                           /*<Conan>*/

    gfgc_ctl.remap_soh->cycle_cnt =              /*                           *//*<Conan>*/
        gfgc_ctl.save_soh_cycle_cnt;                                            /*<Conan>*/

    ret = Hcm_romdata_read_knl(                  /*  C                        *//*<Conan>*/
            D_HCM_E_H24_SAVE_TSTAMP,                                            /*<Conan>*/
            0,                                                                  /*<Conan>*/
            ( void * )&gfgc_ctl.save_soh_degra_timestamp );                     /*<Conan>*/

    if( ret != 0 )                               /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
        gfgc_ctl.save_soh_degra_timestamp =      /*                           *//*<Conan>*/
            DFGC_SOH_VAL_INIT;                                                  /*<Conan>*/
    }                                                                           /*<Conan>*/

    ret = Hcm_romdata_read_knl(                  /*  C                        *//*<Conan>*/
            D_HCM_E_H24_SAVE_SOC,                                               /*<Conan>*/
            0,                                                                  /*<Conan>*/
            ( void * )&gfgc_ctl.save_soc_total );                               /*<Conan>*/
    if( ret != 0 )                               /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
        gfgc_ctl.save_soc_total =                /* SOC                       *//*<Conan>*/
            DFGC_SOH_VAL_INIT;                                                  /*<Conan>*/
    }                                                                           /*<Conan>*/

    gfgc_ctl.remap_soh->soc_total =              /* SOC                       *//*<Conan>*/
        gfgc_ctl.save_soc_total;                                                /*<Conan>*/

/*<Conan>*/                                                 /*<3131029>                  */
/*<Conan>    ret = Hcm_romdata_read_knl(*/                  /*<3131029>  C               */
/*<Conan>            D_HCM_FG_OCV_FLAG,*/                   /*<3131029>OCV               */
/*<Conan>            0,*/                                   /*<3131029>                  */
/*<Conan>            ( void * )&gfgc_ctl.ocv_enable );*/    /*<3131029>                  */
/*<Conan>    if( ret != 0 )*/                               /*<3131029>                  */
/*<Conan>    {*/                                            /*<3131029>                  */
/*<Conan>        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0008 );*/ /*<3131029>            */
/*<Conan>        gfgc_ctl.ocv_enable = DFGC_ON;*/           /*<3131029>        ON        */
/*<Conan>        return ret;*/
/*<Conan>    }*/                                            /*<3131029>                  */
#if DFGC_SW_DELETED_FUNC
    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_SDDTVPOP_SEL,
            0,
            ( void * )&gfgc_ctl.capacity_fix );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0005 );
        gfgc_ctl.capacity_fix                    /*           2  1            */
            = DFGC_CAPACITY_FIX_DIS;             /* UI                        */
    }

    ret = Hcm_romdata_read_knl(                  /*<1210001>  C               */
            D_STATE_LOG,                         /*<1210001>                  */
            0,                                   /*<1210001>                  */
            ( void * )&gfgc_ecurrent );          /*<1210001>                  */
    if( ret != 0 )                               /*<1210001>                  */
    {                                            /*<1210001>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0009 );      /*<1210001>       */
        gfgc_ecurrent.detection = 0;             /*<1210001>                  */
    }                                            /*<1210001>                  */
                                                 /*<1210001>                  */
#endif /* DFGC_SW_DELETED_FUNC */

    MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0xFFFF );

    return ret;
}
#else /*<Conan>1111XX*/
int pfgc_fgc_nvm_get( void )
{
    int ret;

    MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0000 );
#if DFGC_SW_DELETED_FUNC
    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_MTF_REST_OF_BATT_1,
            0,
            ( void * )&gfgc_ctl.lvl_2 );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0001 );
        gfgc_ctl.lvl_2 = DFGC_LVL_2_INIT;        /* MTF_REST_OF_BATT1(3G)     */
    }

    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_MTF_REST_OF_BATT_2,
            0,
            ( void * )&gfgc_ctl.lvl_1 );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0002 );
        gfgc_ctl.lvl_1 = DFGC_LVL_1_INIT;        /* MTF_REST_OF_BATT2(3G)     */
    }

    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_FG_PARAM1,
            0,
            ( void * )&gfgc_ctl.lva_3g );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0003 );
        gfgc_ctl.lva_3g = DFGC_LVA_3G_INIT;      /* LVA      3G               */
        gfgc_ctl.lva_2g = DFGC_LVA_2G_INIT;      /* LVA      2G               */
    }
                                                 /*<3210049>                  */
    ret = Hcm_romdata_read_knl(                  /*<3210049>  C               */
            D_HCM_PID_BATT_ALARM,                /*<3210049>              3G  */
            0,                                   /*<3210049>                  */
            ( void * )&gfgc_ctl.alarm_3g );      /*<3210049>                  */
    if( ret != 0 )                               /*<3210049>                  */
    {                                            /*<3210049>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0006 ); /*<3210049>            */
        gfgc_ctl.alarm_3g = DFGC_LVA_3G_INIT;    /*<3210049>              3G  */
    }                                            /*<3210049>                  */
                                                 /*<3210049>                  */
    ret = Hcm_romdata_read_knl(                  /*<3210049>  C               */
            D_HCM_PID_BATT_ALARM5,               /*<3210049>              2G  */
            0,                                   /*<3210049>                  */
            ( void * )&gfgc_ctl.alarm_2g );      /*<3210049>                  */
    if( ret != 0 )                               /*<3210049>                  */
    {                                            /*<3210049>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0007 ); /*<3210049>            */
        gfgc_ctl.alarm_2g = DFGC_LVA_2G_INIT;    /*<3210049>              2G  */
    }                                            /*<3210049>                  */

    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_FG_PARAM2,
            0,
            ( void * )&gfgc_ctl.dump_enable );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0004 );
        gfgc_ctl.dump_enable                     /* RF Class                  */
            = DFGC_DUMP_ENABLE_INIT;             /*       ON                  */
        gfgc_ctl.charge_timer                    /* FG-IC                     */
            = DFGC_CHARGE_TIMER_INIT;
        gfgc_ctl.soh_n = DFGC_SOH_N_INIT;        /*                           */
        gfgc_ctl.soh_init = DFGC_SOH_INIT_INIT;  /*                           */
        gfgc_ctl.soh_level = DFGC_SOH_LEVEL_INIT;/*                 (        )*/
        gfgc_ctl.soh_down = DFGC_SOH_DOWN_INIT;  /*                           */
        gfgc_ctl.soh_bad = DFGC_SOH_BAD_INIT;
    }
#endif /* DFGC_SW_DELETED_FUNC */
                                                 /*<3131029>                  */
    ret = Hcm_romdata_read_knl(                  /*<3131029>  C               */
            D_HCM_FG_OCV_FLAG,                   /*<3131029>OCV               */
            0,                                   /*<3131029>                  */
            ( void * )&gfgc_ctl.ocv_enable );    /*<3131029>                  */
    if( ret != 0 )                               /*<3131029>                  */
    {                                            /*<3131029>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0008 ); /*<3131029>            */
        gfgc_ctl.ocv_enable = DFGC_ON;           /*<3131029>        ON        */
        return ret;
    }                                            /*<3131029>                  */

#if DFGC_SW_DELETED_FUNC
    ret = Hcm_romdata_read_knl(                  /*  C                        */
            D_HCM_SDDTVPOP_SEL,
            0,
            ( void * )&gfgc_ctl.capacity_fix );
    if( ret != 0 )                               /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0005 );
        gfgc_ctl.capacity_fix                    /*           2  1            */
            = DFGC_CAPACITY_FIX_DIS;             /* UI                        */
    }

    ret = Hcm_romdata_read_knl(                  /*<1210001>  C               */
            D_STATE_LOG,                         /*<1210001>                  */
            0,                                   /*<1210001>                  */
            ( void * )&gfgc_ecurrent );          /*<1210001>                  */
    if( ret != 0 )                               /*<1210001>                  */
    {                                            /*<1210001>                  */
        MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0x0009 );      /*<1210001>       */
        gfgc_ecurrent.detection = 0;             /*<1210001>                  */
    }                                            /*<1210001>                  */
                                                 /*<1210001>                  */
#endif /* DFGC_SW_DELETED_FUNC */

    MFGC_RDPROC_PATH( DFGC_FGC_NVM_GET | 0xFFFF );
    return ret;
}
#endif /*<Conan>1111XX*/

/******************************************************************************/
/*                pfgc_fgc_int                                                */
/*                FG-IC                                                       */
/*                2008.10.21                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
irqreturn_t pfgc_fgc_int(int irq, void *dev_id)
{
/* IDPower #12   volatile void __iomem *reg_addr;              *//*                          */

    MFGC_RDPROC_PATH( DFGC_FGC_INT | 0x0000 );

    FGC_IRQ("\n");
    DEBUG_FGIC(("########## pfgc_fgc_int start ##########\n"));

#if 0 /* IDPower */
    /*                                                        *//*<3210076>   */
    /* ( pint_disable                                         *//*<3210076>   */
    /*                                                      ) *//*<3210076>   */
    reg_addr = pfgc_get_reg_addr(OMAP4_GPIO_IRQSTATUSCLR0);
    *(( volatile unsigned int * ) reg_addr )
        = DFGC_MASK_GPIO_FGC_PORT;
#endif
                                                                /*<3210076>   */
    /*                                                        *//*<3210076>   */
    gfgc_int_flag++;                                            /*<3210076>   */

/*<1905359>    gfgc_ctl.flag |= D_FGC_FG_INT_IND;           *//*                           */

/*<1905359>    wake_up_interruptible( &gfgc_fginfo_wait );  *//*  V WakeUp                 */

    spin_lock( &gfgc_spinlock_if );               /*<npdc100388275>              */
    gfgc_kthread_flag |= D_FGC_FG_INT_IND;        /*<1905359>                 */
    spin_unlock( &gfgc_spinlock_if );             /*<npdc100388275>*/
    wake_up( &gfgc_task_wq );                     /*<1905359>  V WakeUp Thread*/

    //printk("%s end\n", __FUNCTION__); //kawai

    MFGC_RDPROC_PATH( DFGC_FGC_INT | 0xFFFF );

    return IRQ_HANDLED;
}

/******************************************************************************/
/*                pfgc_fgc_ocv_request                                        */
/*                OCV                                                         */
/*                2008.10.21                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pfgc_fgc_ocv_request( void )
{
    MFGC_RDPROC_PATH( DFGC_FGC_OCV_REQUEST | 0x0000 );

    gfgc_ctl.remap_ocv->ocv_ctl_flag |= DFGC_OCV_REQUEST;

    MFGC_RDPROC_PATH( DFGC_FGC_OCV_REQUEST | 0xFFFF );
}

/********************************************************************<1905359>*/
/*                pfgc_fgc_kthread                                   <1905359>*/
/*                                                                   <1905359>*/
/*                2009.04.01                                         <1905359>*/
/*                NTTDMSE                                            <1905359>*/
/*                                                                   <1905359>*/
/*                                                                   <1905359>*/
/*                                                                   <1905359>*/
/*                                                                   <1905359>*/
/********************************************************************<1905359>*/
int pfgc_fgc_kthread( void* arg )                /*                 <1905359>*/
{                                                 /*                 <1905359>*/
    unsigned char          type;                  /*                 <1905359>*/
    unsigned long          flags;                 /*                 <1905359>*/
    mm_segment_t           oldfs;                 /*                 <1905359>*/
    struct sched_param     param;                 /*                 <1905359>*/
    unsigned long          wakeup;                /*                 <1905359>*/
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    T_FGC_SOH              *t_soh;                /*                          *//*<Conan>*/
#endif /*<Conan>1111XX*/

    DEBUG_FGIC(("pfgc_fgc_kthread start\n"));
                                                  /*                 <1905359>*/
                                                  /*                 <1905359>*/
    MFGC_RDPROC_PATH( DFGC_FGC_KTHREAD | 0x0000 );/*                 <1905359>*/
                                                  /*                 <1905359>*/
    /*                                          *//*                 <1905359>*/
    param.sched_priority = DFGC_THREAD_PRIORITY;  /*                 <1905359>*/
    type                 = 0;
                                                  /*                 <1905359>*/
    /*                                          *//*                 <1905359>*/
    daemonize( DFGC_THREAD_NAME );                /*  V              <1905359>*/
    oldfs = get_fs();                             /*  N thread_info  <1905359>*/
    set_fs( KERNEL_DS );                          /*                 <1905359>*/
    sched_setscheduler( current,                  /*  N              <1905359>*/
                        DFGC_THREAD_POLICY,       /*                 <1905359>*/
                        &param );                 /*                 <1905359>*/
    set_user_nice( current, DFGC_THREAD_NICE );   /*  V nice         <1905359>*/
    set_fs( oldfs );                              /*         (    )  <1905359>*/
                                                  /*                 <1905359>*/
    /*                                          *//*                 <1905359>*/
    while( 1 )                                    /*  R              <1905359>*/
    {                                             /*                 <1905359>*/
        MFGC_RDPROC_PATH( DFGC_FGC_KTHREAD | 0x0001 );/*             <1905359>*/

        if(( type & D_FGC_SHUTDOWN_IND )
            == D_FGC_SHUTDOWN_IND )
        {
            /* file flush and notify complete */
            FGC_DEBUG("complete_and_exit\n");
            complete_and_exit(&gfgc_comp, 0);
            break;
        }
                                                      /*             <1905359>*/
        wait_event( gfgc_task_wq,                     /*             <1905359>*/
                    ( gfgc_kthread_flag != 0 ));      /*             <1905359>*/
                                                      /*             <1905359>*/
		/* IDP npdc300035452 : i2c  resume        thread                 */
		if (atomic_read(&gfgc_susres_flag) == 0){
			FGC_IRQ("*** wait for resume\n");
			wait_event_timeout(gfgc_susres_wq, (atomic_read(&gfgc_susres_flag) != 0), 200 * HZ / 1000 );
			FGC_IRQ("*** complete resume\n");
		}

        FGC_IRQ("wake_up trigger = 0x%08lx\n", gfgc_kthread_flag);
/*<npdc100388275>        spin_lock_irqsave( &gfgc_spinlock_fgc,        *//*             <1905359>*/
/*<npdc100388275>                           flags );                   *//*             <1905359>*/
        spin_lock_irqsave( &gfgc_spinlock_if,         /*             <npdc100388275>*/
                           flags );                   /*             <npdc100388275>*/
                                                      /*             <1905359>*/
        type = ( unsigned char )gfgc_kthread_flag;    /*             <1905359>*/
        gfgc_kthread_flag = 0;                        /*             <1905359>*/
                                                      /*             <1905359>*/
//Jessie add start
        if(( type & D_FGC_CHG_INT_IND ) == D_FGC_CHG_INT_IND )
        {
            gfgc_chgsts_thread = gfgc_ctl.chg_sts;
        }
//Jessie add end

/*<npdc100388275>        spin_unlock_irqrestore( &gfgc_spinlock_fgc,   *//*             <1905359>*/
/*<npdc100388275>                                flags );              *//*             <1905359>*/
        spin_unlock_irqrestore( &gfgc_spinlock_if,    /*             <npdc100388275>*/
                                flags );              /*             <npdc100388275>*/
                                                      /*             <1100232>*/
        if(( type & D_FGC_EXIT_IND )                  /*             <1100232>*/
           == D_FGC_EXIT_IND )                        /*             <1100232>*/
        {                                             /*             <1100232>*/
            MFGC_RDPROC_PATH( DFGC_FGC_KTHREAD | 0x0003 ); /*        <1100232>*/
            break;                                    /*             <1100232>*/
        }                                             /*             <1100232>*/
        else if(( type & D_FGC_SHUTDOWN_IND )
            == D_FGC_SHUTDOWN_IND )
        {
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
            t_soh = gfgc_ctl.remap_soh;          /*                           *//*<Conan>*/
                                                 /*                           *//*<Conan>*/
            pfgc_log_time_ex( &t_soh->soh_system_timestamp,                     /*<Conan>*/
                              &t_soh->soh_monotonic_timestamp);                 /*<Conan>*/
                          
            pfgc_log_health(DFGC_ON);                 /*                      *//*<Conan>*/
#endif /*<Conan>1111XX*/
            /* Save log to NAND from SDRAM */
            pfgc_log_shutdown();
            if(( type & ~D_FGC_SHUTDOWN_IND ) == 0 )
            {
                /* factor is shutdown only */
                continue;
            }
        }
        else
        {
            /* DO NOTHING */
        }
                                                      /*             <1905359>*/
        wakeup = pfgc_info_update( type );            /*  C          <1905359>*/
        if( wakeup == DFGC_ON )                       /*             <1905359>*/
        {                                             /*             <1905359>*/
            MFGC_RDPROC_PATH( DFGC_FGC_KTHREAD | 0x0002 ); /*        <1905359>*/
                                                      /*             <1905359>*/
            spin_lock_irqsave(                        /*             <1905359>*/
                &gfgc_spinlock_fgc, flags );          /*             <1905359>*/
                                                      /*             <1905359>*/
            gfgc_ctl.flag |= type;                    /*             <1905359>*/
                                                      /*             <1905359>*/
            spin_unlock_irqrestore(                   /*             <1905359>*/
                &gfgc_spinlock_fgc, flags );          /*             <1905359>*/
                                                      /*             <1905359>*/
            wake_up_interruptible( &gfgc_fginfo_wait );/*            <1905359>*/
        }                                             /*             <1905359>*/
    }                                             /*                 <1905359>*/
                                                  /*                 <1905359>*/
    MFGC_RDPROC_PATH( DFGC_FGC_KTHREAD | 0xFFFF );/*                 <1905359>*/

    DEBUG_FGIC(("pfgc_fgc_kthread end\n"));

    return 0;
}                                                 /*                 <1905359>*/

/* Get Average Current Method */
static int pfgc_fgc_aiget( signed short *ai_data )
{
	int ret, mod_ret;
	TCOM_RW_DATA com_acc_data;
	unsigned char regdata[2];
	unsigned long mode;


	FGC_FUNCIN("\n");
	ret = 0;
	*ai_data = 0;

	mode = gfgc_fgic_mode;
	if(mode == DCOM_ACCESS_MODE1)	/* Normal mode */
	{
		mod_ret = pfgc_i2c_sleep_set(D_FGC_SLEEP_ENABLE);
		if(mod_ret != DFGC_OK)
		{
			pfgc_log_i2c_error(PFGC_FGC_AIGET + 0x01 , 0);
			FGC_ERR("pfgc_i2c_sleep_set(D_FGC_SLEEP_ENABLE) failed. ret=%d\n", mod_ret);
			ret = -1;
		}

		memset(regdata, 0x00, sizeof(regdata));

		com_acc_data.offset = ( unsigned long * )&gfgc_fgic_offset[DFGC_REGGET_OFFSET_AI];
		com_acc_data.data   = ( void * )regdata;
		com_acc_data.number = sizeof(regdata);
		com_acc_data.size   = 1;
		com_acc_data.option = NULL;

		/* FG-IC register get */
		mod_ret = pfgc_fgc_regget( 0x00, &com_acc_data );
		if(mod_ret != 0)
		{
			pfgc_log_i2c_error(PFGC_FGC_AIGET + 0x02 , 0);
			FGC_ERR("pfgc_fgc_regget() failed. ret=%d\n", mod_ret);
			ret = -1;
		}
		else
		{
			FGC_CHG("AI_L = 0x%02X, AI_H = 0x%02X\n", regdata[0], regdata[1]);
	        *ai_data = ( signed short )(( regdata[1] << 8 ) | regdata[0] );
		}
	}
	else	/* ROM mode */
	{
		FGC_CHG("mode = 0x%08lx, ai = %d\n", mode, *ai_data);
	}

	FGC_FUNCOUT("ret = %d, ai=0x%04x\n", ret, *ai_data);
	return ret;
}

/* IDPower start */
/* Get Full Charge Capacity Method */
static int pfgc_fgc_fccget( signed int *fcc_data )
{
	int ret, mod_ret;
	TCOM_RW_DATA com_acc_data;
	unsigned char regdata[2];
	unsigned long mode;


	FGC_FUNCIN("\n");
	ret = 0;
	*fcc_data = 0;

	mode = gfgc_fgic_mode;
	if(mode == DCOM_ACCESS_MODE1)	/* Normal mode */
	{
		mod_ret = pfgc_i2c_sleep_set(D_FGC_SLEEP_ENABLE);
		if(mod_ret != DFGC_OK)
		{
			pfgc_log_i2c_error(PFGC_FGC_FCCGET + 0x01 , 0);
			FGC_ERR("pfgc_i2c_sleep_set(D_FGC_SLEEP_ENABLE) failed. ret=%d\n", mod_ret);
			ret = -1;
		}

		memset(regdata, 0x00, sizeof(regdata));

		com_acc_data.offset = ( unsigned long * )&gfgc_fgic_offset[DFGC_REGGET_OFFSET_FCC];
		com_acc_data.data   = ( void * )regdata;
		com_acc_data.number = sizeof(regdata);
		com_acc_data.size   = 1;
		com_acc_data.option = NULL;

		/* FG-IC register get */
		mod_ret = pfgc_fgc_regget( 0x00, &com_acc_data );
		if(mod_ret != 0)
		{
			pfgc_log_i2c_error(PFGC_FGC_FCCGET + 0x02 , 0);
			FGC_ERR("pfgc_fgc_regget() failed. ret=%d\n", mod_ret);
			ret = -1;
		}
		else
		{
			FGC_CHG("FCC_L = 0x%02X, FCC_H = 0x%02X\n", regdata[0], regdata[1]);
		        *fcc_data = ( signed int )(( regdata[1] << 8 ) | regdata[0] );
		}
	}
	else	/* ROM mode */
	{
		FGC_CHG("mode = 0x%08lx, fcc = %d\n", mode, *fcc_data);
	}

	FGC_FUNCOUT("ret = %d, fcc=%d(0x%08x)\n", ret, *fcc_data, *fcc_data);
	return ret;
}
/* IDPower end */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL( pfgc_fgc_mstate );               /*         DD    R/W          */
EXPORT_SYMBOL( pfgc_fgc_volt_send );
 
