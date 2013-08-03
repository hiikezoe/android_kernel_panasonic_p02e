/*
 * drivers/fgc/pfgc_df.c
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
/*                pfgc_df_get                                                 */
/*                        DD DF       DF                                      */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                T_FGC_RW_DF * df_data DF                                    */
/*                unsigned char * df_rd_data DF                               */
/******************************************************************************/
unsigned int pfgc_df_get( T_FGC_RW_DF * df_data, unsigned char * df_rd_data  )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    unsigned int         retval;                  /*                          */
    T_FGC_DF_SUBCLASS    request_block;           /*                          */
    unsigned int         old_status;              /*<3131157> Seal/FAS        */

    MFGC_RDPROC_PATH( DFGC_DF_GET | 0x0000 );

    retval = DFGC_OK;                             /*                          */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( ( df_data == NULL ) || ( df_rd_data == NULL ) ) /*     NULL           */
    {
        MFGC_RDPROC_PATH( DFGC_DF_GET | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_DF_GET | 0x0001 );

        retval = DFGC_NG;                         /*                          */
    } /* ( df_data == NULL ) || ( df_rd_data == NULL ) */
    else
    {
        MFGC_RDPROC_PATH( DFGC_DF_GET | 0x0002 );

                              /* FullAccessMode                               *//*<3131157>*/
                              /* Sealed        Read                           *//*<3131157>*/
        retval = pfgc_fw_fullaccessmode_set( &old_status ); /*   N                        *//*<3131157>*/

        request_block.subclass = df_data->id;     /* SubClass_ID              */
        request_block.block = df_data->block;     /* Block No.                */
        request_block.block_data = df_data->block_data; /* BlockData No.      */

                                                  /* DF	                      */
/*<3131157>retval = pfgc_fgic_dfread( ( const T_FGC_DF_SUBCLASS * )&request_block, df_rd_data );*//*   N                        */
        retval |= pfgc_fgic_dfread( ( const T_FGC_DF_SUBCLASS * )&request_block, df_rd_data ); /*   N     check       *//*<3131157>*/

                              /* FullAccessMode                               *//*<3131157>*/
                              /* Sealed        Read                           *//*<3131157>*/
        retval |= pfgc_fw_fullaccessmode_rev( &old_status ); /*   N                        *//*<3131157>*/

    }

    MFGC_RDPROC_PATH( DFGC_DF_GET | 0xFFFF );

    return retval;                                /* return                   */
}

/******************************************************************************/
/*                pfgc_df_set                                                 */
/*                        DD DF       DF                                      */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                T_FGC_RW_DF * df_data DF                                    */
/*                unsigned char * df_wr_data DF                               */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_df_set( T_FGC_RW_DF * df_data, unsigned char * df_wr_data  )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    unsigned int         retval;                  /*                          */
    T_FGC_DF_SUBCLASS    request_block;           /*                          */
    T_FGC_FW_VER         fw_ver_data;
    unsigned int         old_status;              /* Seal/FAS                 */

    MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0000 );

    retval = DFGC_OK;                             /*                          */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( df_data == NULL )                         /*     NULL                 */
    {
        MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_DF_SET | 0x0001 );

        retval = DFGC_NG;                         /*                          */

    } /* df_data == NULL */
    else
    {                                             /*     OK                   */
        MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0002 );

        if( df_data->type == DFGC_DF_EXT )
        {                                         /*               write      */
            MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0003 );

            if( df_wr_data == NULL )              /*     NULL                 */
            {
                MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0004 );
                MFGC_RDPROC_ERROR( DFGC_DF_SET | 0x0002 );

                retval = DFGC_NG;                 /*                          */

            } /* df_wr_data == NULL */
            else
            {                                     /*     OK                   */
                MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0005 );

                request_block.subclass = df_data->id; /* SubClass_ID          */
                request_block.block = df_data->block; /* Block#               */
                request_block.block_data = df_data->block_data; /* BlockData# */

                                                  /* DF	                      */
                retval = pfgc_fgic_dfwrite( ( const T_FGC_DF_SUBCLASS * )&request_block, df_wr_data ); /*   N                        */

            } /* df_wr_data != NULL */

        } /* df_data->type == DFGC_DF_EXT */
        else if( df_data->type == DFGC_DF_INT )
        {                                         /* NOR    DF                */
            MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0006 );

            retval = pfgc_ver_read( &fw_ver_data ); /* Version                *//*   N                        */

            if( ( retval == DFGC_OK )
             && ( gfgc_verup_type != DFGC_VERUP_NONE )
             && ( fw_ver_data.inst_ver_fgic == fw_ver_data.inst_ver_int ) )
            {                                     /* InstVer                  */
                MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0007 );

                                                  /* FullAccessMode           */
                retval |= pfgc_fw_fullaccessmode_set( &old_status );/*   N                        */

                                                  /* FGIC  ROM                */
                retval |= pfgc_fgic_modechange( DCOM_ACCESS_MODE2 );/*   N                        */

                                                  /* NOR    DF                */
                retval |= pfgc_fw_update( DFGC_VERUP_DF );/*   N                        */

                                                  /* FGIC  Normal             */
                retval |= pfgc_fgic_modechange( DCOM_ACCESS_MODE1 );/*   N                        */

            } /* fw_ver_data.inst_ver_fgic == fw_ver_data.inst_ver_int */
            else
            {                                     /* InstVer                  */
                MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0008 );
                MFGC_RDPROC_ERROR( DFGC_DF_SET | 0x0003 );

                retval |= DFGC_NG;                /*                          */

            } /* fw_ver_data.inst_ver_fgic != fw_ver_data.inst_ver_int */

        } /* df_data->type == DFGC_DF_INT */
        else
        {                                         /* type                     */
            MFGC_RDPROC_PATH( DFGC_DF_SET | 0x0009 );
            MFGC_RDPROC_ERROR( DFGC_DF_SET | 0x0004 );

            retval = DFGC_NG;                     /*                          */

        }

    } /* df_data != NULL */

    MFGC_RDPROC_PATH( DFGC_DF_SET | 0xFFFF );

    return retval;                                /* return                   */
}

/******************************************************************************/
/*                pfgc_df_study_check                                         */
/*                        DD DF       DF                                      */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                                                                            */
/*                T_FGC_DF_STUDY * df_study_data                              */
/******************************************************************************/
unsigned int pfgc_df_study_check( T_FGC_DF_STUDY * df_study_data )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    unsigned int       retval;                    /*                          */
    int                ret;                       /*                          */
    unsigned int       old_status;                /* Seal/FAS                 */
/*<3131157>unsigned char      qmax0_tmp,qmax1_tmp;*//* Qmax0/Qmax1              */
    unsigned char      qmax_tmp[2];               /* Qmax0/Qmax1              *//*<3131157>*/
    unsigned char      ra0sts_tmp,ra1sts_tmp;     /* Ra0Status/Ra1Status Tmp  */
    unsigned char      rax0sts_tmp,rax1sts_tmp;   /* Rax0Sts/Rax1Sts Tmp      */
    unsigned char      ra0_inp[18];               /* Ra0                  Tmp */
    T_FGC_FW_COM_MEMMAP * rom_va;                 /* NOR    FW                */
    signed int         nand_ret;                  /*<1100231> NAND            */
    T_FGC_SYSERR_DATA  syserr_info;               /*<1100231> SYSERR          */

    MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0000 );

    retval = DFGC_OK;                             /*                          */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( df_study_data == NULL )                   /*     NULL                 */
    {
        MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_DF_STUDY_CHECK | 0x0001 );

        retval = DFGC_NG;                         /*                          */

    } /* df_study_data == NULL */
    else
    {
        MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0002 );

                                                  /* FullAccessMode           */
        retval = pfgc_fw_fullaccessmode_set( &old_status );/*   N                        */

                                                  /* Qmax0/Qmax1              */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Qmax0, &qmax0_tmp );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Qmax1, &qmax1_tmp );*//*   N                        */
        retval |= pfgc_fgic_dfread_num( &DF_SC_Qmax0, qmax_tmp, Qmax_readtbl, sizeof( qmax_tmp ) );/*   N     check       *//*<3131157>*/

/*<3131157>if( qmax0_tmp != qmax1_tmp )*/
        if( qmax_tmp[0] != qmax_tmp[1] )          /*<3131157>                 */
        {                                         /* Qmax0!=Qmax1             */
            MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0003 );

            df_study_data->df_study_state = DFGC_ON;

        } /* qmax0_tmp != qmax1_tmp */
        else
        {                                         /*                          */
            MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0004 );

            retval |= pfgc_fgic_dfread( &DF_SC_Ra1Sts, &ra1sts_tmp );
            retval |= pfgc_fgic_dfread( &DF_SC_Rax0Sts, &rax0sts_tmp );
            retval |= pfgc_fgic_dfread( &DF_SC_Rax1Sts, &rax1sts_tmp );
                               /* Pack0 Ra1,Pack0 Rax,Pack1 Rax  Status       */

            if( ( ra1sts_tmp  != 0xFF ) &&
                ( rax0sts_tmp != 0xFF ) &&
                ( rax1sts_tmp != 0xFF ) )
            {                                     /*         FFh              */
                MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0005 );

                df_study_data->df_study_state = DFGC_ON;

            } /* ( ra1sts_tmp != 0xFF ) && ( rax0sts_tmp != 0xFF ) && ( rax1sts_tmp != 0xFF ) */
            else
            {                                     /*                          */
                MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0006 );

                retval |= pfgc_fgic_dfread( &DF_SC_Ra0Sts, &ra0sts_tmp );/*   N                        */
                                                  /* Pack0 Ra0 Status         */

                if( ra0sts_tmp != 0x00 )
                {                                 /* 0x00                     */
                    MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0007 );

                    df_study_data->df_study_state = DFGC_ON;

                } /* ra0sts_tmp != 0x00 */
                else
                {                                 /*                   read   */
                    MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0008 );

/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0Flg,   &ra0_inp[  0 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0Btr_L, &ra0_inp[  1 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0Btr_H, &ra0_inp[  2 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0Gain,  &ra0_inp[  3 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_1,    &ra0_inp[  4 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_2,    &ra0_inp[  5 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_3,    &ra0_inp[  6 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_4,    &ra0_inp[  7 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_5,    &ra0_inp[  8 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_6,    &ra0_inp[  9 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_7,    &ra0_inp[ 10 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_8,    &ra0_inp[ 11 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_9,    &ra0_inp[ 12 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_10,   &ra0_inp[ 13 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_11,   &ra0_inp[ 14 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_12,   &ra0_inp[ 15 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_13,   &ra0_inp[ 16 ] );*//*   N                        */
/*<3131157>retval |= pfgc_fgic_dfread( &DF_SC_Ra0_14,   &ra0_inp[ 17 ] );*//*   N                        */
                    retval |= pfgc_fgic_dfread_num( &DF_SC_Ra0Flg, ra0_inp, &Ra0_readtbl[1], sizeof( ra0_inp ) );/*   N     check       *//*<3131157>*/

/*<1100231>                    rom_va =                  *//* ROM    FW                */
/*<1100231>                            ( T_FGC_FW_COM_MEMMAP * )ioremap(      *//*   C */
/*<1100231>                               DMEM_NOR_FGC_FGICFW,                */
                    rom_va = ( T_FGC_FW_COM_MEMMAP * )vmalloc( /*<1100231>  C */
                               ( unsigned long )( sizeof( T_FGC_FW_COM_MEMMAP ) ));

                    if( rom_va == NULL )
                    {                             /* ioremap                  */
                        MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x0009 );
                        MFGC_RDPROC_ERROR( DFGC_DF_STUDY_CHECK | 0x0002 );

                        retval = DFGC_NG;         /*                          */

                    } /* rom_va == NULL */
                    else                          /*<3100734>                 */
                    {                             /*<3100734>                 */
                        MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x000C );       /*<1100231>       */
                                                                                /*<1100231>       */
                        nand_ret = mtdwrap_read_bbm(
                                        MMC_CFG_TBL_PART,   /* IDPower */
                                        MMC_FGIC_FW_OFFSET, /* IDPower */
                                        sizeof(T_FGC_FW_COM_MEMMAP),
                                        (void *)rom_va);
                        if( nand_ret != 0 )
                        {                                                       /*<1100231>       */
                            MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x000D );   /*<1100231>       */
                            MFGC_RDPROC_ERROR( DFGC_DF_STUDY_CHECK | 0x000D );  /*<1100231>       */
                                                                                /*<1100231>       */
                            MFGC_SYSERR_DATA_SET(                               /*<1100231>SYSERR */
                                        CSYSERR_ALM_RANK_B,                     /*<1100231>       */
                                        DFGC_SYSERR_NVM_ERR,                    /*<1100231>       */
                                        ( DFGC_DF_STUDY_CHECK | 0x000D ),       /*<1100231>       */
                                        ( unsigned long )nand_ret,              /*<1100231>       */
                                        syserr_info );                          /*<1100231>       */
                            pfgc_log_syserr( &syserr_info );                    /*<1100231>  V    */
                                                                                /*<1100231>       */
                            retval = DFGC_NG;                                   /*<1100231>       */
                        }                                                       /*<1100231>       */

                    ret = memcmp( ra0_inp, &rom_va->rom_Pack0_Ra[1], 18 ); /*   C */

                    if( ret != 0 )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x000A );

                        df_study_data->df_study_state = DFGC_ON;

                    } /* memcmp == FALSE */
                    else
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0x000B );

                        df_study_data->df_study_state = DFGC_OFF;

                    } /* memcmp == TRUE */

                        vfree( rom_va );          /*<1100231>  V              */
                        rom_va = NULL;            /*<1100231>                 */

                    } /* ioremap == NULL */       /*<3100734>                 */

                } /* ra0sts_tmp == 0x00 */

            } /* !( ( ra1sts_tmp != 0xFF ) && ( rax0sts_tmp != 0xFF ) && ( rax1sts_tmp != 0xFF ) ) */

        } /* qmax0_tmp == qmax1_tmp */

        retval |= pfgc_fw_fullaccessmode_rev( &old_status ); /*   N                        */
                                                  /* FullAccessMode           */

    } /* df_study_data != NULL */

    MFGC_RDPROC_PATH( DFGC_DF_STUDY_CHECK | 0xFFFF );

    return retval;                                /* return                   */
}

/******************************************************************************/
/*                pfgc_fgic_dfread                                            */
/*                FuelGauge-IC Dataflash Read                                 */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                const T_FGC_DF_SUBCLASS * df_subclass                       */
/*                unsigned char * data                                        */
/******************************************************************************/
unsigned int pfgc_fgic_dfread( const T_FGC_DF_SUBCLASS * df_subclass , unsigned char * rw_data )
{
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned long   accs_option;                  /*                          */
    unsigned char   df_blockcmd[3];               /* DataFlash BlockCTRLCmd   */
    unsigned long   df_readadr[1];                /* DataFlash Read           */
    unsigned char   read_data[1];                 /* ReadData                 */
    unsigned int    retval;                       /*                          */
/*<3131157>unsigned int    old_status;          *//* Seal/FAS                 */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0000 );

    retval = DFGC_OK;                             /*                          */

                              /* FullAccessMode                               */
                              /* Sealed        Read                           */
/*<3131157>retval |= pfgc_fw_fullaccessmode_set( &old_status );*/ /*   N                        */

                                                  /* I2C                open  */
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */

    if( com_acc_ret == DCOM_OK )                  /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0001 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
        rate = DCOM_RATE400K;                     /* RATE  400K               */
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

        com_acc_rwdata.size   = 1;                /*                          */
        accs_option = DCOM_ACCESS_MODE1;          /*                          */
        com_acc_rwdata.option = &accs_option     ;/* Normal                   */

        df_blockcmd[0] = 0x00;                    /* BlockCTRL        1byte   */
        df_blockcmd[1] = df_subclass->subclass;   /* SubClass_ID              */
        df_blockcmd[2] = df_subclass->block;      /* Block#                   */

                                                  /* BlockCTRL                */
        MFGC_FGRW_DTSET( com_acc_rwdata, DFBlkCtrlAdr, df_blockcmd );
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /* Read    BlockData#       */
        df_readadr[0] = ( unsigned long )df_subclass->block_data;

        com_acc_rwdata.option = NULL;             /*<QAIO026> option          */
                                                  /* BlockData Read           */
        MFGC_FGRW_DTSET( com_acc_rwdata, df_readadr, read_data );
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                          */
            MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0002 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_DFREAD | 0x0001 );

            retval |= DFGC_NG;                    /*       NG                 */

        } /* pcom_acc_read/write != DCOM_OK */

        *rw_data = read_data[0];                  /*              *//* mrc22001 */


                                                  /*                 close    */
        com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                 close    */
            MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0003 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_DFREAD | 0x0002 );

            retval |= DFGC_NG;                    /*       NG                 */

        } /* pcom_acc_close != DCOM_OK */

    } /* pcom_acc_open == DCOM_OK */
    else
    {                                             /*                 open     */
        MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0004 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_DFREAD | 0x0003 );

        retval |= DFGC_NG;                        /*       NG                 */

    } /* pcom_acc_open != DCOM_OK */

                              /* FullAccessMode                               */
                              /* Sealed        Read                           */
/*<3131157>retval |= pfgc_fw_fullaccessmode_rev( &old_status );*/ /*   N                        */

    MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0xFFFF );

    return retval;

}

/******************************************************************************//*<3131157>*/
/*                pfgc_fgic_dfread_num                                        *//*<3131157>*/
/*                FuelGauge-IC Dataflash Read                                 *//*<3131157>*/
/*                2008.10.01                                                  *//*<3131157>*/
/*                NTTDMSE                                                     *//*<3131157>*/
/*                                                                            *//*<3131157>*/
/*                DFGC_OK                                                     *//*<3131157>*/
/*                DFGC_NG                                                     *//*<3131157>*/
/*                const T_FGC_DF_SUBCLASS * df_subclass                       *//*<3131157>*/
/*                unsigned char * data                                        *//*<3131157>*/
/*                unsigned char num                                           *//*<3131157>*/
/*                unsigned char * data                                        *//*<3131157>*/
/******************************************************************************//*<3131157>*/
unsigned int pfgc_fgic_dfread_num( const T_FGC_DF_SUBCLASS * df_subclass ,      /*<3131157>*/
                                    unsigned char * rw_data,                    /*<3131157>*/
                                     const unsigned long * rd_tbl,              /*<3131157>*/
                                      unsigned char num )                       /*<3131157>*/
{                                                                               /*<3131157>*/
    TCOM_ID         com_acc_id;                   /*                 ID       *//*<3131157>*/
    TCOM_FUNC       com_acc_ret;                  /*                          *//*<3131157>*/
    TCOM_RW_DATA    com_acc_rwdata;               /*                          *//*<3131157>*/
    unsigned long   accs_option;                  /*                          *//*<3131157>*/
    unsigned char   df_blockcmd[3];               /* DataFlash BlockCTRLCmd   *//*<3131157>*/
    unsigned int    retval;                       /*                          *//*<3131157>*/
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          *//*<3131157>*/
    unsigned long   rate;                         /* I2C        Rate          *//*<3131157>*/
#endif /* DVDV_FGC_I2C_RATE_CONST */                                            /*<3131157>*/
                                                                                /*<3131157>*/
    MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0000 );                              /*<3131157>*/
                                                                                /*<3131157>*/
    retval = DFGC_OK;                             /*                          *//*<3131157>*/
                                                                                /*<3131157>*/
                                                  /* I2C                open  *//*<3131157>*/
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL );*//*   C *//*<3131157>*/
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */
                                                                                /*<3131157>*/
    if( com_acc_ret == DCOM_OK )                  /*                 open     *//*<3131157>*/
    {                                                                           /*<3131157>*/
        MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0001 );                          /*<3131157>*/
                                                                                /*<3131157>*/
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          *//*<3131157>*/
        rate = DCOM_RATE400K;                     /* RATE  400K               *//*<3131157>*/
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C *//*<3131157>*/
#endif /* DVDV_FGC_I2C_RATE_CONST */                                            /*<3131157>*/
                                                                                /*<3131157>*/
        com_acc_rwdata.size   = 1;                /*                          *//*<3131157>*/
        accs_option = DCOM_ACCESS_MODE1;          /*                          *//*<3131157>*/
        com_acc_rwdata.option = &accs_option     ;/* Normal                   *//*<3131157>*/
                                                                                /*<3131157>*/
        df_blockcmd[0] = 0x00;                    /* BlockCTRL        1byte   *//*<3131157>*/
        df_blockcmd[1] = df_subclass->subclass;   /* SubClass_ID              *//*<3131157>*/
        df_blockcmd[2] = df_subclass->block;      /* Block#                   *//*<3131157>*/
                                                                                /*<3131157>*/
                                                  /* BlockCTRL                *//*<3131157>*/
        MFGC_FGRW_DTSET( com_acc_rwdata, DFBlkCtrlAdr, df_blockcmd );           /*<3131157>*/
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C *//*<3131157>*/
                                                                                /*<3131157>*/
                                                  /* BlockData Read           *//*<3131157>*/
        com_acc_rwdata.offset = ( unsigned long * )rd_tbl;                      /*<3131157>*/
        com_acc_rwdata.data = ( void * )rw_data;                                /*<3131157>*/
        com_acc_rwdata.number = num;                                            /*<3131157>*/
        com_acc_rwdata.option = NULL;             /*<QAIO026> option          */
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata );/*  C*/ /*<3131157>*/
                                                                                /*<3131157>*/
        if( com_acc_ret != DCOM_OK )                                            /*<3131157>*/
        {                                         /*                          *//*<3131157>*/
            MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0002 );                      /*<3131157>*/
            MFGC_RDPROC_ERROR( DFGC_FGIC_DFREAD | 0x0001 );                     /*<3131157>*/
                                                                                /*<3131157>*/
            retval |= DFGC_NG;                    /*       NG                 *//*<3131157>*/
                                                                                /*<3131157>*/
        } /* pcom_acc_read/write != DCOM_OK */                                  /*<3131157>*/
                                                                                /*<3131157>*/
                                                  /*                 close    *//*<3131157>*/
        com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */                   /*<3131157>*/
                                                                                /*<3131157>*/
        if( com_acc_ret != DCOM_OK )                                            /*<3131157>*/
        {                                         /*                 close    *//*<3131157>*/
            MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0003 );                      /*<3131157>*/
            MFGC_RDPROC_ERROR( DFGC_FGIC_DFREAD | 0x0002 );                     /*<3131157>*/
                                                                                /*<3131157>*/
            retval |= DFGC_NG;                    /*       NG                 *//*<3131157>*/
                                                                                /*<3131157>*/
        } /* pcom_acc_close != DCOM_OK */                                       /*<3131157>*/
                                                                                /*<3131157>*/
    } /* pcom_acc_open == DCOM_OK */                                            /*<3131157>*/
    else                                                                        /*<3131157>*/
    {                                             /*                 open     *//*<3131157>*/
        MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0x0004 );                          /*<3131157>*/
        MFGC_RDPROC_ERROR( DFGC_FGIC_DFREAD | 0x0003 );                         /*<3131157>*/
                                                                                /*<3131157>*/
        retval |= DFGC_NG;                        /*       NG                 *//*<3131157>*/
                                                                                /*<3131157>*/
    } /* pcom_acc_open != DCOM_OK */                                            /*<3131157>*/
                                                                                /*<3131157>*/
    MFGC_RDPROC_PATH( DFGC_FGIC_DFREAD | 0xFFFF );                              /*<3131157>*/
                                                                                /*<3131157>*/
    return retval;                                                              /*<3131157>*/
                                                                                /*<3131157>*/
}                                                                               /*<3131157>*/

/******************************************************************************/
/*                pfgc_fgic_dfwrite                                           */
/*                FuelGauge-IC Dataflash Write                                */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                const T_FGC_DF_SUBCLASS * df_subclass                       */
/*                unsigned char * data                                        */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fgic_dfwrite( const T_FGC_DF_SUBCLASS * df_subclass , unsigned char * rw_data )
{
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned long   accs_option;                  /*                          */
    unsigned char   df_blockcmd[3];               /* DataFlash BlockCTRLCmd   */
    unsigned long   df_writeadr[1];               /* DataFlash Write          */
    unsigned char   df_data_old[1];               /* Write                    */
    unsigned char   df_data_new[1];               /* Write                    */
    unsigned char   df_checksum_old[1];           /* Write                    */
    unsigned char   df_checksum_new[1];           /* Write                    */
    unsigned int    retval;                       /*                          */
    unsigned int    old_status;                   /* Seal/FAS                 */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0000 );

    retval = DFGC_OK;                             /*                          */

                              /* FullAccessMode                               */
                              /* Sealed        Write                          */
    retval |= pfgc_fw_fullaccessmode_set( &old_status ); /*   N                        */

                                                  /* I2C                open  */
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */

    if( com_acc_ret == DCOM_OK )                  /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0001 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
        rate = DCOM_RATE400K;                     /* RATE  400K               */
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

        com_acc_rwdata.size   = 1;                /*                          */
        accs_option = DCOM_ACCESS_MODE1;          /*                          */
        com_acc_rwdata.option = &accs_option     ;/* Normal                   */

        df_blockcmd[0] = 0x00;                    /* BlockCTRL        1byte   */
        df_blockcmd[1] = df_subclass->subclass;   /* SubClass_ID              */
        df_blockcmd[2] = df_subclass->block;      /* Block#                   */

                                                  /* BlockCTRL                */
        MFGC_FGRW_DTSET( com_acc_rwdata, DFBlkCtrlAdr, df_blockcmd );
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

        com_acc_rwdata.option = NULL;             /*<QAIO026> option          */
                                                  /*                 Read     */
        MFGC_FGRW_DTSET( com_acc_rwdata, DFBlkSumAdr, df_checksum_old );
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /* Write    BlockData#      */
        df_writeadr[0] = ( unsigned long )df_subclass->block_data;

                                                  /* Write        Read        */
        MFGC_FGRW_DTSET( com_acc_rwdata, df_writeadr, df_data_old );
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

        df_data_new[0] = *rw_data;                /* Write                    */

        com_acc_rwdata.option = &accs_option;     /*<QAIO026> option          */
                                                  /* Write                    */
        MFGC_FGRW_DTSET( com_acc_rwdata, df_writeadr, df_data_new );
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /* Write                    */
        df_checksum_new[0] = ( unsigned char )( ( unsigned short )0x100
                                             + ( unsigned short )df_checksum_old[0]
                                             + ( unsigned short )df_data_old[0]
                                             - ( unsigned short )*rw_data );

                                                  /* Write                    */
        MFGC_FGRW_DTSET( com_acc_rwdata, DFBlkSumAdr, df_checksum_new );
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                   NG     */
            MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0002 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_DFWRITE | 0x0001 );

            retval |= DFGC_NG;                    /*       NG                 */

        } /* pcom_acc_read/write != DCOM_OK */

                                                  /*                 close    */
        com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                 close    */
            MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0003 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_DFWRITE | 0x0002 );

            retval |= DFGC_NG;

        } /* pcom_acc_close != DCOM_OK */

    } /* pcom_acc_open == DCOM_OK */
    else
    {                                             /*                 open     */
        MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0004 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_DFWRITE | 0x0003 );

        retval |= DFGC_NG;                        /*       NG                 */

    } /* pcom_acc_open != DCOM_OK */

                              /* FullAccessMode                               */
                              /* Sealed        Read                           */
    retval |= pfgc_fw_fullaccessmode_rev( &old_status ); /*   N                        */

    MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0xFFFF );

    return retval;

}

