/*
 * drivers/fgc/pfgc_dfs.c
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
#include "inc/pfgc_local.h"                       /*         DD               */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*                pfgc_dfs_read                                               */
/*                        DD DFS       DF                                     */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                                                                            */
/*                unsigned short * dfs_rd_data DF                             */
/******************************************************************************/
unsigned int pfgc_dfs_read( unsigned short * dfs_rd_data )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   read_reg[2];                  /*                          */
    unsigned int    retval;                       /*                          */
    unsigned long   accs_option;                  /* I2C                      */
    unsigned long   rate;                         /* I2C        Rate          */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */

    MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0000 );

    retval = DFGC_OK;                             /*                          */

    if( dfs_rd_data == NULL )                     /*     NULL                 */
    {
        MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_DFS_READ | 0x0001 );

        MFGC_SYSERR_DATA_SET(                     /* SYSERR         <PCIO034> */
                    CSYSERR_ALM_RANK_B,                          /* <PCIO034> */
                    DFGC_SYSERR_PARAM_ERR,                       /* <PCIO034> */
                    ( DFGC_DFS_READ | 0x0001 ),                  /* <PCIO034> */
                    ( unsigned long )dfs_rd_data,                /* <PCIO034> */
                    syserr_info );                               /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*  V SYSERR      <PCIO034> */

        retval = DFGC_NG;                         /*                          */

    } /* dfs_rd_data == NULL */
    else
    {                                             /*       NULL               */
        MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0002 );

                                                  /*                 open     */
/*<PCIO034>        com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
        com_acc_ret = pfgc_fgic_comopen( &com_acc_id );     /*   C <PCIO034>  */

        if( com_acc_ret == DCOM_OK )              /*                 open     */
        {
            MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0003 );

            rate = DCOM_RATE400K;                 /* RATE  400K               */
            com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */

            com_acc_rwdata.size   = 1;            /*                          */
            accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
            com_acc_rwdata.option = &accs_option; /* Normal                   */

                                                  /* CTRL_STATUS Cmd          */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, CtrlStsCmd );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay(1); /*<SABU>*/

            read_reg[0] = 0xFF;
            read_reg[1] = 0xFF;

            com_acc_rwdata.option = NULL;         /*<QAIO026> option          */
                                                  /* Control()        Read    */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, read_reg );
            com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

            if( com_acc_ret == DCOM_OK )
            {
                MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0004 );

                                                  /* Read                     */
                *dfs_rd_data = ( unsigned short )( ( unsigned )( read_reg[1] << 8 ) | read_reg[0] );

            } /* pcom_acc_write/read == DCOM_OK */
            else
            {
                MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0005 );
                MFGC_RDPROC_ERROR( DFGC_DFS_READ | 0x0002 );

                retval = DFGC_NG;
            } /* pcom_acc_write/read != DCOM_OK */

                                                  /*                 close    */
            com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

            if( com_acc_ret != DCOM_OK )
            {                                    /*                 close     */
                MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0006 );
                MFGC_RDPROC_ERROR( DFGC_DFS_READ | 0x0003 );

                retval = DFGC_NG;

            } /* pcom_acc_close != DCOM_NG */

        } /* pcom_acc_open == DCOM_OK */
        else
        {
            MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x0007 );
            MFGC_RDPROC_ERROR( DFGC_DFS_READ | 0x0004 );

            retval = DFGC_NG;

        } /* pcom_acc_open == DCOM_NG */

    } /* dfs_rd_data != NULL */

    MFGC_RDPROC_PATH( DFGC_DFS_READ | 0xFFFF );

    return retval;                                /* return                   */

}

/******************************************************************************/
/*                pfgc_dfs_write                                              */
/*                        DD DFS       DF                                     */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                unsigned long * dfs_wr_data DF                              */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_dfs_write( unsigned long * dfs_wr_data )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned int    old_status;                   /* Seal/FAS                 */
    unsigned int    retval;                       /*                          */
    unsigned long   accs_option;                  /* I2C                      */
    unsigned long   rate;                         /* I2C        Rate          */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */

    MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0000 );

    retval = DFGC_OK;                             /*                          */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( dfs_wr_data == NULL )                     /*     NULL                 */
    {
        MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_DFS_WRITE | 0x0001 );

        MFGC_SYSERR_DATA_SET(                     /* SYSERR         <PCIO034> */
                    CSYSERR_ALM_RANK_B,                          /* <PCIO034> */
                    DFGC_SYSERR_PARAM_ERR,                       /* <PCIO034> */
                    ( DFGC_DFS_WRITE | 0x0001 ),                 /* <PCIO034> */
                    ( unsigned long )dfs_wr_data,                /* <PCIO034> */
                    syserr_info );                               /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*  V SYSERR      <PCIO034> */

        retval = DFGC_NG;                         /*                          */

    } /* dfs_wr_data == NULL */
    else if( *dfs_wr_data == DFGC_DFS_ON )
    {                                             /*         ON               */
        MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0002 );

                                                  /* FullAccess               */
        retval = pfgc_fw_fullaccessmode_set( &old_status ); /*   N                        */

                                                  /*                 open     */
/*<PCIO034>        com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
        com_acc_ret = pfgc_fgic_comopen( &com_acc_id );     /*   C <PCIO034>  */

        if( com_acc_ret == DCOM_OK )              /*                 open     */
        {
            MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0003 );

            rate = DCOM_RATE400K;                 /* RATE  400K               */
            com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */

            com_acc_rwdata.size   = 1;            /*                          */
            accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
            com_acc_rwdata.option = &accs_option; /* Normal                   */

                                                  /* RESET Cmd                */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, FgicResetCmd );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay(2000); /*<SABU>*/

                                                  /* IT_ENABLE Cmd            */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, ItEnableCmd );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay(2000); /*<SABU>*/

                                                  /* Seal Cmd                 */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, SealCmd );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay( DFGC_WAIT_SEALED );           /* Seal Cmd Wait            */

            if( com_acc_ret != DCOM_OK )
            {                                     /*                   err    */
                MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0004 );
                MFGC_RDPROC_ERROR( DFGC_DFS_WRITE | 0x0002 );
                retval = DFGC_NG;
            } /* pcom_acc_write != DCOM_OK */

                                                  /*                 close    */
            com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

            if( com_acc_ret != DCOM_OK )
            {                                     /*                 close    */
                MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0005 );
                MFGC_RDPROC_ERROR( DFGC_DFS_WRITE | 0x0003 );

                retval = DFGC_NG;

            } /* pcom_acc_close != DCOM_OK */

        } /* pcom_acc_open == DCOM_OK */
        else
        {
            MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0006 );
            MFGC_RDPROC_ERROR( DFGC_DFS_WRITE | 0x0004 );

            retval = DFGC_NG;

        } /* pcom_acc_open != DCOM_OK */

    } /* *dfs_wr_data == DFGC_DFS_ON */
    else if( *dfs_wr_data == DFGC_DFS_OFF )
    {                                             /*         OFF              */
        MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0007 );

                                                  /* FullAccess               */
        retval = pfgc_fw_fullaccessmode_set( &old_status ); /*   N                        */

    } /* *dfs_wr_data == DFGC_DFS_OFF */
    else
    {                                             /*                          */
        MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0x0008 );
        MFGC_RDPROC_ERROR( DFGC_DFS_WRITE | 0x0005 );

        retval = DFGC_NG;

    } /* *dfs_wr_data == ??? */

    MFGC_RDPROC_PATH( DFGC_DFS_WRITE | 0xFFFF );

    return retval;                                /* return                   */

}

