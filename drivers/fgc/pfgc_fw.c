/*
 * drivers/fgc/pfgc_fw.c
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
/*                pfgc_fw_write                                               */
/*                        DD FW           FGIC FW                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                T_FGC_FW * fw_data FW                                       */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fw_write( T_FGC_FW * fw_data )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    unsigned int      retval;                     /*                          */
    T_FGC_FW_VER      fw_ver_data;                /* FGIC FW                  */
    unsigned int      mode_chk;                   /* FGIC           Work      */
    unsigned char     verup_type;                 /* VerUp Type               */
    unsigned int      old_status;                 /* Seal/FAS                 */
    T_FGC_SYSERR_DATA syserr_info;                /* SYSERR         <PCIO034> */

    MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0000 );

    retval = DFGC_OK;                             /*                          */
    verup_type = DFGC_VERUP_INSTDF;               /* VerUp Type               */
    old_status = 0x00;                            /* Seal/FAS                 */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( fw_data == NULL )                         /*     NULL                 */
    {
        MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_FW_WRITE | 0x0001 );

        MFGC_SYSERR_DATA_SET(                     /* SYSERR         <PCIO034> */
                    CSYSERR_ALM_RANK_B,                          /* <PCIO034> */
                    DFGC_SYSERR_PARAM_ERR,                       /* <PCIO034> */
                    ( DFGC_FW_WRITE | 0x0001 ),                  /* <PCIO034> */
                    ( unsigned long )fw_data,                    /* <PCIO034> */
                    syserr_info );                               /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*  V SYSERR      <PCIO034> */

        retval = DFGC_NG;                         /*                          */

    } /* fw_data ==NULL */
    else
    {
        MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0002 );

/*<3130560> mode_chk = pfgc_fgic_modecheck( );  *//*   C FGIC                 */
        mode_chk = gfgc_fgic_mode;                /*<3130560>                 */

        if( mode_chk == DCOM_ACCESS_MODE2 )       /*           :ROM           */
        {
            MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0003 );

            fw_ver_data.ver_up = DFGC_VER_UP;     /*                          */
            verup_type = DFGC_VERUP_INSTDF;       /* Instruction/DF           */
                                                  /*                          */
                                                  /*                          */

        } /* mode_chk == DCOM_ACCESS_MODE2 */
        else if( mode_chk == DCOM_ACCESS_MODE1 )  /*           :Normal        */
        {
            MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0004 );

            switch( fw_data->type )               /*                          */
            {
                case DFGC_FW_EXT:                 /*                          */
                    MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0005 );

                    /* FOMA8iS                                         */
                    fw_ver_data.ver_up = DFGC_VER_DOWN;

                    break;

                case DFGC_FW_INT:                 /* NOR                      */
                    MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0006 );

                    retval = pfgc_ver_read( &fw_ver_data );/*   C Ver         */

                    if( ( retval == DFGC_NG ) || ( gfgc_verup_type == DFGC_VERUP_NONE ) )
                    {                             /* FW       or VerCheckNG   */
                        MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0007 );
                        MFGC_RDPROC_ERROR( DFGC_FW_WRITE | 0x0002 );

                        MFGC_SYSERR_DATA_SET2(    /* SYSERR         <PCIO034> */
                                    CSYSERR_ALM_RANK_B,          /* <PCIO034> */
                                    DFGC_SYSERR_INTERNAL_ERR,    /* <PCIO034> */
                                    ( DFGC_FW_WRITE | 0x0002 ),  /* <PCIO034> */
                                    retval,                      /* <PCIO034> */
                                    gfgc_verup_type,             /* <PCIO034> */
                                    syserr_info );               /* <PCIO034> */
                        pfgc_log_syserr( &syserr_info ); /*  V      <PCIO034> */

                        fw_ver_data.ver_up = DFGC_VER_DOWN; /* FW             */

                    } /* gfgc_verup_type == DFGC_VERUP_NONE */
                    else
                    {                             /* NOR  Ver                 */
                        MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0008 );

                        fw_ver_data.ver_up = DFGC_VER_UP; /* FW               */
                        verup_type = DFGC_VERUP_INSTDF;/* Instruction/DF      */

                    } /* gfgc_verup_type != DFGC_VERUP_NONE */

                    break;

                case DFGC_FW_INT_NEW:             /* NOR  FW                  */
                    MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x0009 );

                    retval = pfgc_ver_read( &fw_ver_data );/*   C Ver         */

                    if( retval == DFGC_NG )
                    {
                        MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x000A );
                        MFGC_RDPROC_ERROR( DFGC_FW_WRITE | 0x0003 );

                        MFGC_SYSERR_DATA_SET(     /* SYSERR         <PCIO034> */
                                    CSYSERR_ALM_RANK_B,          /* <PCIO034> */
                                    DFGC_SYSERR_INTERNAL_ERR,    /* <PCIO034> */
                                    ( DFGC_FW_WRITE | 0x0003 ),  /* <PCIO034> */
                                    retval,                      /* <PCIO034> */
                                    syserr_info );               /* <PCIO034> */
                        pfgc_log_syserr( &syserr_info ); /*  V      <PCIO034> */

                        fw_ver_data.ver_up = DFGC_VER_DOWN;
                    } /* pfgc_ver_read == DFGC_NG */

                    verup_type = gfgc_verup_type; /* chk    ( Inst/DF or DF ) */

                    break;

                default:                          /* type                     */
                    MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x000B );
                    MFGC_RDPROC_ERROR( DFGC_FW_WRITE | 0x0004 );

                    MFGC_SYSERR_DATA_SET(         /* SYSERR         <PCIO034> */
                                CSYSERR_ALM_RANK_B,              /* <PCIO034> */
                                DFGC_SYSERR_PARAM_ERR,           /* <PCIO034> */
                                ( DFGC_FW_WRITE | 0x0004 ),      /* <PCIO034> */
                                fw_data->type,                   /* <PCIO034> */
                                syserr_info );                   /* <PCIO034> */
                    pfgc_log_syserr( &syserr_info );     /*  V      <PCIO034> */

                    fw_ver_data.ver_up = DFGC_VER_DOWN;

                    break;

            } /* switch( fw_data->type ) */

            if( fw_ver_data.ver_up == DFGC_VER_UP )
            {                                     /* FW                       */
                MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x000C );

                                                  /* Full                     */
                retval = pfgc_fw_fullaccessmode_set( &old_status ); /*   C    */

                                                  /* ROM                      */
                retval |= pfgc_fgic_modechange( DCOM_ACCESS_MODE2 );/*   C    */

            }

        } /* mode_chk == DCOM_ACCESS_MODE1 */
        else                                      /*                          */
        {
            MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x000D );
            MFGC_RDPROC_ERROR( DFGC_FW_WRITE | 0x0005 );

            MFGC_SYSERR_DATA_SET(                 /* SYSERR         <PCIO034> */
                        CSYSERR_ALM_RANK_B,                      /* <PCIO034> */
                        DFGC_SYSERR_PARAM_ERR,                   /* <PCIO034> */
                        ( DFGC_FW_WRITE | 0x0005 ),              /* <PCIO034> */
                        mode_chk,                                /* <PCIO034> */
                        syserr_info );                           /* <PCIO034> */
            pfgc_log_syserr( &syserr_info );      /*  V SYSERR      <PCIO034> */

            retval = DFGC_NG;

        } /* pfgc_fgic_modecheck == ??? */

        if( ( retval != DFGC_NG ) && ( fw_ver_data.ver_up == DFGC_VER_UP ) )
        {                                         /*                          */
            MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x000E );

            retval = pfgc_fw_update( verup_type );/* FW             *//*   N                        */

                                                  /* Nomal                    */
            retval |= pfgc_fgic_modechange( DCOM_ACCESS_MODE1 );/*   N                        */

        } /* fw_ver_data.ver_up == DFGC_VER_UP */
        else
        {                                         /*                          */
            MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0x000F );

        } /* fw_ver_data.ver_up != DFGC_VER_UP */

    } /* fw_data !=NULL */

    MFGC_RDPROC_PATH( DFGC_FW_WRITE | 0xFFFF );

    return retval;                                /* return                   */

}

/******************************************************************************/
/*                pfgc_fw_update <3210043>                                    */
/*                        DD FW           FGIC FW                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                type               (DF    orINST+DF)                        */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fw_update( unsigned char type )
{
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   read_sts[1];                  /*         Read  Work       */
    unsigned int    i, j, retval;                 /* loop  work               */
    unsigned int    ret;                          /*                          */
    T_FGC_FW_COM_MEMMAP * rom_va;                 /* ioremap                  */
    unsigned char   row_temp[ DFGC_ROW_SIZE_INST ];/* Row          Work       */
    unsigned long   accs_option;                  /*                   Option */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */
    int             nand_ret;                     /*<QAIO027> NAND            */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0000 );

    ret = DFGC_OK;                                /*                          */

                                                  /*                 open     */
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */

    if( com_acc_ret == DCOM_OK )                  /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0001 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
        rate = DCOM_RATE400K;                     /* RATE  400K               */
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

        com_acc_rwdata.size   = 1;                /*                          */
/*<QAIO028>accs_option = DCOM_ACCESS_MODE2;                                   */
        accs_option = ( DCOM_ACCESS_MODE2 | DCOM_ERRLOG_NOWRITE );/*<QAIO028> */
        com_acc_rwdata.option = &accs_option;     /* ROM                      */

/*<QAIO027>        rom_va =                     *//* ROM    FW                */
/*<QAIO027>            ( T_FGC_FW_COM_MEMMAP * )ioremap(             *//*   C */
/*<QAIO027>               DMEM_NOR_FGC_FGICFW,                                */
        rom_va = ( T_FGC_FW_COM_MEMMAP * )        /*<QAIO027>                 */
            vmalloc(                              /*<QAIO027>  C NAND         */
               ( unsigned long )( sizeof( T_FGC_FW_COM_MEMMAP ) ));

        if( rom_va == NULL )
        {                                         /* ioremap                  */
            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0002 );
            MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0001 );

            MFGC_SYSERR_DATA_SET(                 /* SYSERR         <PCIO034> */
                        CSYSERR_ALM_RANK_B,                      /* <PCIO034> */
                        DFGC_SYSERR_IOREMAP_ERR,                 /* <PCIO034> */
                        ( DFGC_FW_UPDATE | 0x0001 ),             /* <PCIO034> */
                        ( unsigned long )rom_va,                 /* <PCIO034> */
                        syserr_info );                           /* <PCIO034> */
            pfgc_log_syserr( &syserr_info );      /*  V SYSERR      <PCIO034> */

            ret = DFGC_NG;

        } /* rom_va == NULL */
        else
        {                                         /* ioremap                  */
            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0028 );          /*<QAIO027> */

            nand_ret = mtdwrap_read_bbm(
                MMC_CFG_TBL_PART,   /* IDPower */
                MMC_FGIC_FW_OFFSET, /* IDPower */
                sizeof(T_FGC_FW_COM_MEMMAP),
                (void *)rom_va);
            if( nand_ret != 0 )
            {                                     /*<QAIO027>                 */
                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0029 );      /*<QAIO027> */
                MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0029 );     /*<QAIO027> */
                                                                  /*<QAIO027> */
                MFGC_SYSERR_DATA_SET(                       /*<QAIO027>SYSERR */
                            CSYSERR_ALM_RANK_B,             /*<QAIO027>       */
                            DFGC_SYSERR_NVM_ERR,            /*<QAIO027>       */
                            ( DFGC_FW_UPDATE | 0x0029 ),    /*<QAIO027>       */
                            ( unsigned long )nand_ret,      /*<QAIO027>       */
                            syserr_info );                  /*<QAIO027>       */
                pfgc_log_syserr( &syserr_info );            /*<QAIO027>  V    */
                                                            /*<QAIO027>       */
                ret = DFGC_NG;                    /*<QAIO027>                 */
            }                                     /*<QAIO027>                 */
        }                                         /*<QAIO027>                 */
                                                  /*<QAIO027>                 */
        if( ret == DFGC_OK )                      /*<QAIO027>                 */
        {                                         /*<QAIO027>                 */
            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0003 );

            for( j = 0; j <= DFGC_WHOLE_RTRYCNT; j++ ) /*                      */
            {
                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0004 );

                if( type == DFGC_VERUP_INSTDF )
                {                                 /* Verup      INST+DF       */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0005 );

                    for( i = 0; i <= DFGC_ERSWHOLE_RTRYCNT; i++ ) /*   R */
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0006 );

                                                  /* Instruction              */
                        MFGC_FGRW_DTSET( com_acc_rwdata, MassErsAdr, MassErsCmd );
                        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                        mdelay( DFGC_WAIT_ERS_INST ); /* Erase    Wait *//*  V*/

                        read_sts[0] = 0xFF;       /* Status      work         */

                                                  /* Erase                    */
                        MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_sts );
                        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                        if( read_sts[0] == 0x00 )
                        {                         /* Erase                    */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0007 );

                            break;                /*                          */

                        } /* read_sts[0] == 0x00 */
                        else
                        {                         /* Erase                    */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0008 );

                            continue;             /*                          */

                        } /* read_sts[0] != 0x00 */

                    } /* for( i = 0; i =< DFGC_ERSWHOLE_RTRYCNT; i++ ) */

                    if( i > DFGC_ERSWHOLE_RTRYCNT )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0009 );
                        MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0002 );

                        MFGC_SYSERR_DATA_SET(     /* SYSERR         <PCIO034> */
                                    CSYSERR_ALM_RANK_B,          /* <PCIO034> */
                                    DFGC_SYSERR_INTERNAL_ERR,    /* <PCIO034> */
                                    ( DFGC_FW_UPDATE | 0x0002 ), /* <PCIO034> */
                                    ( unsigned long )NULL,       /* <PCIO034> */
                                    syserr_info );               /* <PCIO034> */
                        pfgc_log_syserr( &syserr_info ); /*  V      <PCIO034> */

                        ret = DFGC_NG;            /*         NG               */

                        break;                    /*                          */

                    } /* i > DFGC_ERSWHOLE_RTRYCNT */

                } /* type == DFGC_VERUP_INSTDF */
                else if( type == DFGC_VERUP_DF )
                {                                 /* Verup      DF            */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x000A );

                    for( i = 0; i <= DFGC_ERSONEP_RTRYCNT; i++ ) /*   R */
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x000B );

                                                  /*     2Row  Erase          */
                        MFGC_FGRW_DTSET( com_acc_rwdata, ROMCmdAdr, OnePageErs );
                        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                        mdelay( DFGC_WAIT_ERS_ONEP );/* Erase    Wait *//*  V */

                        read_sts[0] = 0xFF;       /* Status      work         */

                                                  /* Erase                    */
                        MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_sts );
                        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                        if( read_sts[0] == 0x00 )
                        {                         /* Erase                    */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x000C );

                            break;                /*                          */

                        } /* read_sts[0] == 0x00 */
                        else
                        {                         /* Erase                    */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x000D );

                            continue;             /*                          */

                        } /* read_sts[0] != 0x00 */

                    } /* for( i = 0; i =< DFGC_ERSONEP_RTRYCNT; i++ ) */

                    if( i > DFGC_ERSONEP_RTRYCNT )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x000E );
                        MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0003 );

                        MFGC_SYSERR_DATA_SET(     /* SYSERR         <PCIO034> */
                                    CSYSERR_ALM_RANK_B,          /* <PCIO034> */
                                    DFGC_SYSERR_INTERNAL_ERR,    /* <PCIO034> */
                                    ( DFGC_FW_UPDATE | 0x0003 ), /* <PCIO034> */
                                    ( unsigned long )NULL,       /* <PCIO034> */
                                    syserr_info );               /* <PCIO034> */
                        pfgc_log_syserr( &syserr_info ); /*  V      <PCIO034> */

                        ret = DFGC_NG;            /*         NG               */

                        break;                    /*                          */

                    } /* i > DFGC_ERSONEP_RTRYCNT */

                } /* type == DFGC_VERUP_DF */
                else
                {                                 /* type                     */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x000F );
                    MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0004 );

                    MFGC_SYSERR_DATA_SET(         /* SYSERR         <PCIO034> */
                                CSYSERR_ALM_RANK_B,              /* <PCIO034> */
                                DFGC_SYSERR_PARAM_ERR,           /* <PCIO034> */
                                ( DFGC_FW_UPDATE | 0x0004 ),     /* <PCIO034> */
                                type,                            /* <PCIO034> */
                                syserr_info );                   /* <PCIO034> */
                    pfgc_log_syserr( &syserr_info );     /*  V      <PCIO034> */

                    ret = DFGC_NG;                /*         NG               */
                    break;                        /*                          */

                } /* type == ??? */

                /*                      */
                                                  /* NOR    Row1              */
                retval = pfgc_fw_rowwrite( com_acc_id, DFGC_WRTREQ_INST, /*   C */
                                  1, &rom_va->rom_InstFlash[ DFGC_ROW_SIZE_INST ] );

                                                  /* Row0  NOR    Work        */
                memcpy( row_temp, rom_va->rom_InstFlash, DFGC_ROW_SIZE_INST );/*   D   V */
                                                  /* Addr:05h  3FFFFFh        */
                row_temp[ DFGC_INTG_POS ]     = 0xFF;
                row_temp[ DFGC_INTG_POS + 1 ] = 0xFF;
                row_temp[ DFGC_INTG_POS + 2 ] = 0x3F;

                                                  /* Row0                     */
                retval |= pfgc_fw_rowwrite( com_acc_id, DFGC_WRTREQ_INST, 0, row_temp ); /*   C */

                if( retval != DFGC_OK )
                {                                 /* rowwrite                 */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0010 );

                    continue;                     /* Erase                    */

                } /* retval != DFGC_OK */

                /* Instruction                      Row2               */
                if( type == DFGC_VERUP_INSTDF )
                {                                 /* INST+DF                  */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0011 );

                    retval = DFGC_OK;             /*                          */
                    for( i = 2; i < DFGC_INST_ROW_MAX; i++ ) /*   D */
                    {                             /* Row2    Row              */

                                                  /* 1Row                     */
                        retval |= pfgc_fw_rowwrite( com_acc_id, /*   C        */
                                     DFGC_WRTREQ_INST, ( unsigned short )i,
                                     &rom_va->rom_InstFlash[ DFGC_ROW_SIZE_INST * i ]  );

                    } /* for( i = 2; i < DFGC_INST_ROW_MAX; i++ ) */

                    if( retval == DFGC_OK )
                    {                             /* rowwrite                 */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0012 );

                        break;                    /* DF                       */

                    } /* retval == DFGC_OK */

                } /* type == DFGC_VERUP_INSTDF */
                else
                {
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0013 );

                    break;                        /* DF                       */

                } /* type == DFGC_VERUP_DF */

            } /* for( j = 0; j <= DFGC_WHOLE_RTRYCNT; j++ ) */

            if( j > DFGC_WHOLE_RTRYCNT )
            {                                     /* Instruction              */
                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0014 );
                MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0005 );

                MFGC_SYSERR_DATA_SET(             /* SYSERR         <PCIO034> */
                            CSYSERR_ALM_RANK_B,                  /* <PCIO034> */
                            DFGC_SYSERR_INTERNAL_ERR,            /* <PCIO034> */
                            ( DFGC_FW_UPDATE | 0x0005 ),         /* <PCIO034> */
                            ( unsigned long )NULL,               /* <PCIO034> */
                            syserr_info );                       /* <PCIO034> */
                pfgc_log_syserr( &syserr_info );  /*  V SYSERR      <PCIO034> */

                ret = DFGC_NG;                    /*         NG               */

            } /* j > DFGC_WHOLE_RTRYCNT */

            if( ret == DFGC_OK )
            {                                     /* InstWrite                */
                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0015 );

                for( j = 0; j <= DFGC_DFWHOLE_RTRYCNT; j++ ) /* DF             */
                {
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0016 );

                    /* DF  Erase  Write */
                    for( i = 0; i <= DFGC_ERSDF_RTRYCNT; i++ ) /*   R */
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0017 );

                                                  /* DataFlash Mass Erase Cmd */
                        MFGC_FGRW_DTSET( com_acc_rwdata, DFErsAdr, DFErsCmd );
                        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                        mdelay( DFGC_WAIT_ERS_DF ); /* Erase    Wait *//*   V */

                        read_sts[0] = 0xFF;       /* Status      work         */

                                                  /* Erase                    */
                        MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_sts );
                        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                        if( read_sts[0] == 0x00 )
                        {                         /* Erase                    */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0018 );

                            break;                /*                          */

                        } /* read_sts[0] == 0x00 */
                        else
                        {                         /* Erase                    */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0019 );

                            continue;             /*                          */
                        }

                    } /* for( i = 0; i <= DFGC_ERSDF_RTRYCNT; i++ ) */

                    if( i > DFGC_ERSDF_RTRYCNT )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x001A );
                        MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0006 );

                        MFGC_SYSERR_DATA_SET(     /* SYSERR         <PCIO034> */
                                    CSYSERR_ALM_RANK_B,          /* <PCIO034> */
                                    DFGC_SYSERR_INTERNAL_ERR,    /* <PCIO034> */
                                    ( DFGC_FW_UPDATE | 0x0006 ), /* <PCIO034> */
                                    ( unsigned long )NULL,       /* <PCIO034> */
                                    syserr_info );               /* <PCIO034> */
                        pfgc_log_syserr( &syserr_info );  /*  V     <PCIO034> */

                        ret = DFGC_NG;            /*         NG               */
                        break;                    /* DF    Retry              */

                    } /* i == DFGC_ERSDF_RTRYCNT */

                    retval = DFGC_OK;             /*                          */

                    for( i = 0; i < DFGC_DF_ROW_MAX; i++ ) /*   D */
                    {                             /* DataFlash  Row           */

                                                  /* DF:1Row                  */
                        retval |= pfgc_fw_rowwrite( com_acc_id, DFGC_WRTREQ_DF, /*   C */
                                     ( unsigned short )i ,
                                     &rom_va->rom_DataFlash[ DFGC_ROW_SIZE_DF * i ]  );

                    } /* for( i = 0; i < DFGC_DF_ROW_MAX; i++ ) */

                    if( retval == DFGC_OK )
                    {                             /* rowwrite                 */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x001B );

                        break;                    /* DF    Retry              */

                    } /* NG        DFErase                 */

                } /* for( j = 0; j <= DFGC_DFWHOLE_RTRYCNT; j++ ) */

                if( j > DFGC_DFWHOLE_RTRYCNT )
                {                                 /* DFWrite                  */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x001C );
                    MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0007 );

                    MFGC_SYSERR_DATA_SET(         /* SYSERR         <PCIO034> */
                                CSYSERR_ALM_RANK_B,              /* <PCIO034> */
                                DFGC_SYSERR_INTERNAL_ERR,        /* <PCIO034> */
                                ( DFGC_FW_UPDATE | 0x0007 ),     /* <PCIO034> */
                                ( unsigned long )NULL,           /* <PCIO034> */
                                syserr_info );                   /* <PCIO034> */
                    pfgc_log_syserr( &syserr_info );     /*  V      <PCIO034> */

                    ret = DFGC_NG;                /*         NG               */

                } /* j > DFGC_DFWHOLE_RTRYCNT */

            } /* ret == DFGC_OK */

            if( ret == DFGC_OK )
            {                                 /* DFWrite                  */
                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x001D );

                for( j = 0; j <= DFGC_2ROWWRT_RTRYCNT; j++ ) /*     2ROW retry*/
                {
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x001E );

                    /*     2Row         */
                                                  /* Row1                     */
                    retval = pfgc_fw_rowwrite( com_acc_id, DFGC_WRTREQ_INST, /*   C */
                                     1, &rom_va->rom_InstFlash[ DFGC_ROW_SIZE_INST ] );
                                                  /* Row0                     */
                    retval |= pfgc_fw_rowwrite( com_acc_id, DFGC_WRTREQ_INST, /*   C */
                                     0, rom_va->rom_InstFlash );

                    if( retval == DFGC_OK )
                    {                         /* rowwrite                 */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x001F );

                        pfgc_fgc_ocv_request( );/*   V IPL    OCV         */
                        break;                /* FW                       */

                    } /* retval == DFGC_OK */
                    else
                    {                         /* rowwrite  NG             */
                        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0020 );

                        for( i = 0; i <= DFGC_ERSONEP_RTRYCNT; i++ ) /*   R*/
                        {                     /*                          */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0021 );

                                                  /*     2Row  Erase          */
                            MFGC_FGRW_DTSET( com_acc_rwdata, ROMCmdAdr, OnePageErs );
                            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                            mdelay( DFGC_WAIT_ERS_ONEP );/*Erase  Wait*//*  V*/

                            read_sts[0] = 0xFF; /* Status    work         */

                                                  /* Erase                    */
                            MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_sts );
                            com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                            if( read_sts[0] == 0x00 )
                            {                 /* Erase                    */
                                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0022 );

                                break;        /*                          */

                            } /* read_sts[0] == 0x00 */
                            else
                            {                 /* Erase                    */
                                MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0023 );

                                continue;     /*                          */

                            } /* read_sts[0] != 0x00 */

                        } /* for( i = 0; i <= DFGC_ERSONEP_RTRYCNT; i++ ) */

                        if( i > DFGC_ERSONEP_RTRYCNT )
                        {                     /*                          */
                            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0024 );
                            MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0008 );

                            MFGC_SYSERR_DATA_SET(     /* SYSERR         <PCIO034> */
                                        CSYSERR_ALM_RANK_B,          /* <PCIO034> */
                                        DFGC_SYSERR_INTERNAL_ERR,    /* <PCIO034> */
                                        ( DFGC_FW_UPDATE | 0x0008 ), /* <PCIO034> */
                                        ( unsigned long )NULL,       /* <PCIO034> */
                                        syserr_info );               /* <PCIO034> */
                            pfgc_log_syserr( &syserr_info );  /*  V     <PCIO034> */

                            ret = DFGC_NG;    /*         NG               */

                            break;            /* 2Row                     */

                        } /* i == DFGC_ERSONEP_RTRYCNT */

                    } /* retval != DFGC_OK */

                } /* for( j = 0; j <= DFGC_2ROWWRT_RTRYCNT; j++ ) */

                if( j > DFGC_2ROWWRT_RTRYCNT )
                {                                 /* rowwriteNG    retry      */
                    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0025 );
                    MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x0009 );

                    MFGC_SYSERR_DATA_SET(         /* SYSERR         <PCIO034> */
                                CSYSERR_ALM_RANK_B,              /* <PCIO034> */
                                DFGC_SYSERR_INTERNAL_ERR,        /* <PCIO034> */
                                ( DFGC_FW_UPDATE | 0x0009 ),     /* <PCIO034> */
                                ( unsigned long )NULL,           /* <PCIO034> */
                                syserr_info );                   /* <PCIO034> */
                    pfgc_log_syserr( &syserr_info );     /*  V      <PCIO034> */

                    ret = DFGC_NG;                /*         NG               */
                } /* j > DFGC_2ROWWRT_RTRYCNT */

            } /* ret == DFGC_OK */

        } /* rom_va != NULL */

        if( rom_va != NULL )                      /*<QAIO027>                 */
        {                                         /*<QAIO027>                 */
            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x002A );          /*<QAIO027> */
                                                                  /*<QAIO027> */
            vfree( rom_va );                      /*<QAIO027>   V             */
            rom_va = NULL;                        /*<QAIO027> NULL            */
        }                                         /*<QAIO027>                 */

                                                  /*                 close    */
        com_acc_ret |= pcom_acc_close( com_acc_id ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                 close    */
            MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0026 );
            MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x000A );

            ret = DFGC_NG;                        /*         NG               */

        } /* pcom_acc_close != DCOM_OK */

    } /* pcom_acc_open == DCOM_OK */
    else
    {
        MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0x0027 );
        MFGC_RDPROC_ERROR( DFGC_FW_UPDATE | 0x000B );

        ret = DFGC_NG;                            /*         NG               */

    } /* pcom_acc_open != DCOM_OK */

    MFGC_RDPROC_PATH( DFGC_FW_UPDATE | 0xFFFF );

    return ret;

}

/******************************************************************************/
/*                pfgc_fgic_fullaccessmode_set                                */
/*                        DD FW           FGIC                                */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                unsigned char * old_status         Control()                */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fw_fullaccessmode_set( unsigned int  * old_status )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   read_reg[2];                  /* Control()      Work      */
    unsigned int    i, retval;                    /* loop  Work               */
    unsigned long   accs_option;                  /*                 Option   */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0000 );

    retval = DFGC_OK;                             /*                          */

                                                  /*                 open     */
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */

    if( com_acc_ret == DCOM_OK )                  /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0001 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
        rate = DCOM_RATE400K;                     /* RATE  400K               */
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

        com_acc_rwdata.size   = 1;                /*                          */
        accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
        com_acc_rwdata.option = &accs_option     ;/* Normal                   */

                                                  /* CTRL_STATUS Cmd          */
        MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, CtrlStsCmd );
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

        read_reg[0] = 0xFF;                       /* Work                     */
        read_reg[1] = 0xFF;                       /*                          */

                                                  /* Control()                */
        accs_option = DCOM_ACCESS_MODE1;
        com_acc_rwdata.option = &accs_option;
        MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, read_reg );
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

        *old_status = read_reg[1];                /*                          */

        for( i = 0; i <= DFGC_UNSEAL_RTRYCNT; i++ ) /*   R */
        {                                         /*                          */
            MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0002 );

            if( ( read_reg[1] & DFGC_MASK_SEALSTS ) != DFGC_MASK_SEALSTS )
            {                                     /* Seal                     */
                MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0003 );

                break;                            /*                          */

            } /* read_reg[1] != DFGC_MASK_SEALSTS */
            else
            {                                     /* Seal                     */
                MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0004 );

                if( i < DFGC_UNSEAL_RTRYCNT )
                {                                 /*                          */
                    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0005 );

                    accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
                    com_acc_rwdata.option = &accs_option     ;/* Normal                   */
                                                  /* Unseal_Key_1 Cmd         */
                    MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, UnsealCmd1 );
                    com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /* Unseal_Key_0 Cmd         */
                    MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, UnsealCmd0 );
                    com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /* CTRL_STATUS Cmd          */
                    MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, CtrlStsCmd );
                    com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                    read_reg[0] = 0xFF;           /* Work                     */
                    read_reg[1] = 0xFF;           /*                          */

                    accs_option = DCOM_ACCESS_MODE1;
                    com_acc_rwdata.option = &accs_option     ;/* Normal                   */
                                                  /* Control()                */
                    MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, read_reg );
                    com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                } /* i < DFGC_UNSEAL_RTRYCNT */
                else
                {                                 /*                          */
                    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0006 );
                    MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_SET | 0x0001 );

                    MFGC_SYSERR_DATA_SET(         /* SYSERR         <PCIO034> */
                           CSYSERR_ALM_RANK_B,                   /* <PCIO034> */
                           DFGC_SYSERR_INTERNAL_ERR,             /* <PCIO034> */
                           ( DFGC_FW_FLACCSMODE_SET | 0x0001 ),  /* <PCIO034> */
                           ( unsigned long )NULL,                /* <PCIO034> */
                           syserr_info );                        /* <PCIO034> */
                    pfgc_log_syserr( &syserr_info );     /*  V      <PCIO034> */

                    retval = DFGC_NG;             /*       NG                 */

                } /* i >= DFGC_UNSEAL_RTRYCNT */

            } /* read_reg[1] == DFGC_MASK_SEALSTS */

        } /* for( i = 0; i <= DFGC_UNSEAL_RTRYCNT; i++ ) */

        if( ( retval == DFGC_OK ) && ( com_acc_ret == DCOM_OK ) )
        {                                         /*                          */
            MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0007 );

            for( i = 0; i <= DFGC_FLACCS_RTRYCNT; i++ ) /*   R */
            {                                     /*                          */
                MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0008 );

                accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
                com_acc_rwdata.option = &accs_option     ;/* Normal                   */
                                                  /* CTRL_STATUS Cmd          */
                MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, CtrlStsCmd );
                com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                read_reg[0] = 0xFF;               /* Work                     */
                read_reg[1] = 0xFF;               /*                          */

                accs_option = DCOM_ACCESS_MODE1;
                com_acc_rwdata.option = &accs_option;
                                                  /* Control()                */
                MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, read_reg );
                com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                if( ( read_reg[1] & DFGC_MASK_FLACCSSTS ) != DFGC_MASK_FLACCSSTS )
                {                                 /* FullAccess               */
                    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x0009 );

                    break;                        /* loop                     */

                } /* read_reg[1] != DFGC_MASK_FLACCSSTS */
                else
                {
                    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x000A );

                    if( i < DFGC_FLACCS_RTRYCNT )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x000B );

                        accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
                        com_acc_rwdata.option = &accs_option     ;/* Normal                   */
                                                  /* FullAccess_Key_1 Cmd     */
                        MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, FullAccsCmd1 );
                        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /* FullAccess_Key_0 Cmd     */
                        MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, FullAccsCmd0 );
                        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                    } /* i < DFGC_FLACCS_RTRYCNT */
                    else
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x000C );
                        MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_SET | 0x0002 );

                        MFGC_SYSERR_DATA_SET(         /* SYSERR     <PCIO034> */
                             CSYSERR_ALM_RANK_B,                 /* <PCIO034> */
                             DFGC_SYSERR_INTERNAL_ERR,           /* <PCIO034> */
                             ( DFGC_FW_FLACCSMODE_SET | 0x0002 ),/* <PCIO034> */
                             ( unsigned long )NULL,              /* <PCIO034> */
                             syserr_info );                      /* <PCIO034> */
                        pfgc_log_syserr( &syserr_info );     /*   V <PCIO034> */

                        retval = DFGC_NG;         /*       NG                 */

                    } /* i == DFGC_FLACCS_RTRYCNT */

                } /* read_reg[1] != DFGC_MASK_FLACCSSTS */

            } /* for( i = 0; i <= DFGC_FLACCS_RTRYCNT; i++ ) */

        } /* if( ( retval == DFGC_OK ) && ( com_acc_ret == DCOM_OK ) ) */

        if( ( retval != DFGC_OK ) || ( com_acc_ret != DCOM_OK ) )
        {
            MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x000D );
            MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_SET | 0x0003 );

            retval = DFGC_NG;                     /*       NG                 */

        } /* if( ( retval != DFGC_OK ) || ( com_acc_ret != DCOM_OK ) ) */

                                                  /*                 close    */
        com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                 close    */
            MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x000E );
            MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_SET | 0x0004 );

            retval = DFGC_NG;

        } /* pcom_acc_close != DCOM_OK */

    } /* pcom_acc_open == DCOM_OK */
    else                                          /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0x000F );
        MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_SET | 0x0005 );

        retval = DFGC_NG;                         /*       NG                 */

    } /* pcom_acc_open != DCOM_OK */

    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_SET | 0xFFFF );

    return retval;

}

/******************************************************************************/
/*                pfgc_fgic_fullaccessmode_rev                                */
/*                        DD FW           FGIC                                */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                unsigned char * old_status                                  */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fw_fullaccessmode_rev( unsigned int  * old_status )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned int    retval;                       /*                          */
    unsigned long   accs_option;                  /*                 Option   */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0x0000 );

    retval = DFGC_OK;                             /*                          */

    if( ( ( *old_status & DFGC_MASK_SEALSTS ) == DFGC_MASK_SEALSTS )
        || ( ( *old_status & DFGC_MASK_FLACCSSTS ) == DFGC_MASK_FLACCSSTS ) )
    {                                             /*         Status  SS/FAS On*/
        MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0x0001 );

                                                  /*                 open     */
/*<PCIO034>        com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
        com_acc_ret = pfgc_fgic_comopen( &com_acc_id );      /*   C <PCIO034> */

        if( com_acc_ret == DCOM_OK )              /*                 open     */
        {
            MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0x0002 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
            rate = DCOM_RATE400K;                 /* RATE  400K               */
            com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

            com_acc_rwdata.size   = 1;            /*                          */
            accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
            com_acc_rwdata.option = &accs_option; /* Normal                   */

                                                  /* Seal Cmd                 */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, SealCmd );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay( DFGC_WAIT_SEALED );           /* Seal Cmd Wait            */

            if( com_acc_ret != DCOM_OK )
            {
                MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0x0003 );
                MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_REV | 0x0001 );

                retval = DFGC_NG;                 /*       NG                 */

            } /* pcom_acc_write != DCOM_OK */

                                                  /*                 Close    */
            com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

            if( com_acc_ret != DCOM_OK )
            {                                     /*                 close    */
                MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0x0004 );
                MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_REV | 0x0002 );

                retval = DFGC_NG;

            } /* pcom_acc_close != DCOM_OK */

        } /* pcom_acc_open == DCOM_OK */
        else                                      /*                 open     */
        {
            MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0x0005 );
            MFGC_RDPROC_ERROR( DFGC_FW_FLACCSMODE_REV | 0x0003 );
            retval = DFGC_NG;
        } /* pcom_acc_open != DCOM_OK */

    } /* *old_status == DFGC_MASK_SEALSTS or DFGC_MASK_FLACCSSTS */

    MFGC_RDPROC_PATH( DFGC_FW_FLACCSMODE_REV | 0xFFFF );

    return retval;

}

/******************************************************************************/
/*                pfgc_fgic_modecheck                                         */
/*                FuelGauge-IC                                                */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                ( Normal        DCOM_ACCESS_MODE1           */
/*                                  ROM        DCOM_ACCESS_MODE2 )            */
/*                DFGC_NG                                                     */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fgic_modecheck( void )
{
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   read_reg[1];                  /* Status      Work         */
/*<3210046>    unsigned int    retval;          *//*                          */
    unsigned int    i, retval;                    /*<3210046>loop  Work,      */
    unsigned long   accs_option;                  /*                 Option   */
    unsigned long   rate;                         /* I2C        Rate          */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */

    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0000 );

    retval = DFGC_OK;                             /*                          */

                                                  /*                 open     */
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */

    if( com_acc_ret == DCOM_OK )                  /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0001 );

/*<3210046> rate = DCOM_RATE400K;               *//* RATE  400K               */
        rate = DCOM_RATE100K;                     /*<3210046>Check    100K    */
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */

        com_acc_rwdata.size   = 1;                /*                          */
        accs_option = DCOM_ACCESS_MODE1;
        com_acc_rwdata.option = &accs_option;     /* Normal                   */

                                                  /* Control()                */
        MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, read_reg );
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

/*<QAIO028>        if( com_acc_ret != DCOM_OK )                               */
        if( com_acc_ret == DCOM_ERR_READ5 )       /*<QAIO028>       (      )  */
        {                                         /* DCOM_ACCESS_MODE1        */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0002 );

/*<3130561>                                     *//*                 close    */
/*<3130561>                                     *//* Sleep+                   */
/*<3130561>com_acc_ret = pcom_acc_close( com_acc_id );*/ /*   C */

/*<3130561>                                     *//* Sleep+                   */
/*<3130561>gcom_acc_fgcmode = DCOM_ACCESS_MODE2;*//* comacc                   */

/*<3130561>                                     *//*                 Open     */
/*<3130561>com_acc_ret |= pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL );*/ /*   C */

            accs_option = DCOM_ACCESS_MODE2;      /* ROM                      */

                                                  /* Control()                */
/*<3130561>com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata );*//*   C */
            com_acc_ret = pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C *//*<3130561>*/

            if( com_acc_ret == DCOM_OK )
            {                                     /* Read  OK      ROM        */
                MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0003 );
                                                                 /* I2C                       *//*<3210046>*/
                for( i = 0; i <= DFGC_I2CSPDSET_RTRYCNT; i++ )                                  /*<3210046>*/
                {                                                                               /*<3210046>*/
                    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0020 );                               /*<3210046>*/
                                                                                                /*<3210046>*/
                    MFGC_FGRW_DTSET( com_acc_rwdata, PkBtCmdRdAdr, I2CSpdGet );                 /*<3210046>*/
                    com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */ /*<3210046>*/
                                                                                                /*<3210046>*/
                    mdelay( DFGC_PEKBTCMD_WAIT );     /* PeekByteCmd Wait *//*   V */           /*<3210046>*/
                                                                                                /*<3210046>*/
                    read_reg[0] = 0xFF;                                                         /*<3210046>*/
                                                                                                /*<3210046>*/
                    MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_reg );                      /*<3210046>*/
                    com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */  /*<3210046>*/
                                                                                                /*<3210046>*/
                    if( read_reg[0] == 0x00 )                                                   /*<3210046>*/
                    {                                                                           /*<3210046>*/
                        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0021 );                           /*<3210046>*/
                                                                                                /*<3210046>*/
                        break;                                                                  /*<3210046>*/
                                                                                                /*<3210046>*/
                    } /* read_reg[0] == 0x00 */                                                 /*<3210046>*/
                                                                                                /*<3210046>*/
                } /* for( i = 0; i <= DFGC_I2CSPDSET_RTRYCNT; i++ ) */                          /*<3210046>*/
                                                                                                /*<3210046>*/
                if( i > DFGC_I2CSPDSET_RTRYCNT )                                                /*<3210046>*/
                {                                                                               /*<3210046>*/
                    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0022 );                               /*<3210046>*/
                    MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0020 );                              /*<3210046>*/

                    MFGC_SYSERR_DATA_SET(             /* SYSERR     <PCIO034> */
                         CSYSERR_ALM_RANK_B,                     /* <PCIO034> */
                         DFGC_SYSERR_INTERNAL_ERR,               /* <PCIO034> */
                         ( DFGC_FGIC_MDCHK | 0x0020 ),           /* <PCIO034> */
                         ( unsigned long )NULL,                  /* <PCIO034> */
                         syserr_info );                          /* <PCIO034> */
                    pfgc_log_syserr( &syserr_info );  /*   V        <PCIO034> */
                                                                                                /*<3210046>*/
                    retval = DFGC_NG;                                                           /*<3210046>*/
                                                                                                /*<3210046>*/
                } /* i > DFGC_I2CSPDSET_RTRYCNT */                                              /*<3210046>*/
                else                                                                            /*<3210046>*/
                {                                                                               /*<3210046>*/
                    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0021 );                               /*<3210046>*/
                                                                                                /*<3210046>*/
                    read_reg[0] = 0x00;                                                         /*<3210046>*/
                                                                                                /*<3210046>*/
                    MFGC_FGRW_DTSET( com_acc_rwdata, PkBtCmdRdReg, read_reg );                  /*<3210046>*/
                    com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata );/*   C*/    /*<3210046>*/
                                                                                                /*<3210046>*/
                    if( ( read_reg[0] & DFGC_I2C_SPEED_400K ) != DFGC_I2C_SPEED_400K )          /*<3210046>*/
                    {                                                                           /*<3210046>*/
                        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0022 );                           /*<3210046>*/
                                                                                                /*<3210046>*/
                        for( i = 0; i <= DFGC_I2CSPDSET_RTRYCNT2; i++ )                         /*<3210046>*/
                        {                                                                       /*<3210046>*/
                            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0023 );                       /*<3210046>*/
                                                                                                /*<3210046>*/
                            MFGC_FGRW_DTSET( com_acc_rwdata, PkBtCmdWtAdr, I2C400KSet );        /*<3210046>*/
                            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata );/*  C*//*<3210046>*/
                                                                                                /*<3210046>*/
                            mdelay( DFGC_POKBTCMD_WAIT );     /* PokeByteCmd Wait *//*   V */   /*<3210046>*/
                                                                                                /*<3210046>*/
                            read_reg[0] = 0xFF;                                                 /*<3210046>*/
                                                                                                /*<3210046>*/
                            MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_reg );              /*<3210046>*/
                            com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C *//*<3210046>*/
                                                                                                /*<3210046>*/
                            if( read_reg[0] != 0x00 )                                           /*<3210046>*/
                            {                                                                   /*<3210046>*/
                                MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0021 );                   /*<3210046>*/
                                                                                                /*<3210046>*/
                                continue;                                                       /*<3210046>*/
                                                                                                /*<3210046>*/
                            } /* read_reg[0] == 0x00 */                                         /*<3210046>*/
                                                                                                /*<3210046>*/
                            MFGC_FGRW_DTSET( com_acc_rwdata, PkBtCmdRdReg, read_reg );          /*<3210046>*/
                            com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata );/*   C*//*<3210046>*/
                                                                                                /*<3210046>*/
                            if( ( read_reg[0] & DFGC_I2C_SPEED_400K ) == DFGC_I2C_SPEED_400K )  /*<3210046>*/
                            {                                                                   /*<3210046>*/
                                MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0022 );                   /*<3210046>*/
                                                                                                /*<3210046>*/
                                break;                                                          /*<3210046>*/
                                                                                                /*<3210046>*/
                            } /* ( read_reg[0] & DFGC_I2C_SPEED_400K ) = DFGC_I2C_SPEED_400K */ /*<3210046>*/
                            else                                                                /*<3210046>*/
                            {                                                                   /*<3210046>*/
                                MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0022 );                   /*<3210046>*/
                                                                                                /*<3210046>*/
                                continue;                                                       /*<3210046>*/
                                                                                                /*<3210046>*/
                            } /* read_reg[0] != DFGC_I2C_SPEED_400K */                          /*<3210046>*/
                                                                                                /*<3210046>*/
                        } /* for( i = 0; i <= DFGC_I2CSPDSET_RTRYCNT2; i++ ) */                 /*<3210046>*/
                                                                                                /*<3210046>*/
                        if( i > DFGC_I2CSPDSET_RTRYCNT2 )                                       /*<3210046>*/
                        {                                                                       /*<3210046>*/
                            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0021 );                       /*<3210046>*/
                            MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0021 );                      /*<3210046>*/

                            MFGC_SYSERR_DATA_SET(     /* SYSERR     <PCIO034> */
                                 CSYSERR_ALM_RANK_B,             /* <PCIO034> */
                                 DFGC_SYSERR_INTERNAL_ERR,       /* <PCIO034> */
                                 ( DFGC_FGIC_MDCHK | 0x0021 ),   /* <PCIO034> */
                                 ( unsigned long )NULL,          /* <PCIO034> */
                                 syserr_info );                  /* <PCIO034> */
                            pfgc_log_syserr( &syserr_info ); /*   V <PCIO034> */
                                                                                                /*<3210046>*/
                            retval = DFGC_NG;                                                   /*<3210046>*/
                                                                                                /*<3210046>*/
                        } /* i > DFGC_I2CSPDSET_RTRYCNT */                                      /*<3210046>*/
                                                                                                /*<3210046>*/
                    } /* ( read_reg[0] & DFGC_I2C_SPEED_400K ) != DFGC_I2C_SPEED_400K */        /*<3210046>*/
                                                                                                /*<3210046>*/
                } /* i <= DFGC_I2CSPDSET_RTRYCNT */                                             /*<3210046>*/
                                                                                                /*<3210046>*/
                if( com_acc_ret != DCOM_OK )                                                    /*<3210046>*/
                {                                                                               /*<3210046>*/
                    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0024 );                               /*<3210046>*/
                    MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0021 );                              /*<3210046>*/
                                                                                                /*<3210046>*/
                    retval = DFGC_NG;                                                           /*<3210046>*/
                                                                                                /*<3210046>*/
                }                                                                               /*<3210046>*/
                else                                                                            /*<3210046>*/
                {                                                                               /*<3210046>*/
                    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0025 );                               /*<3210046>*/
                                                                                                /*<3210046>*/
                    retval = DCOM_ACCESS_MODE2;   /*         ROM              */                /*<3210046>*/
                                                                                                /*<3210046>*/
                }                                                                               /*<3210046>*/
/*<3210046>       retval = DCOM_ACCESS_MODE2;   *//*         ROM              */
            } /* pcom_acc_read:DCOM_ACCESS_MODE2 == DCOM_OK */
            else
            {                                     /*                   NG     */
                MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0004 );
                MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0001 );

/*<3130561>gcom_acc_fgcmode = DCOM_ACCESS_MODE1;*//* comacc               */
                retval = DFGC_NG;                 /*         NG               */

            }
        } /* pcom_acc_read:DCOM_ACCESS_MODE1 == DCOM_NG */
        else if( com_acc_ret != DCOM_OK )         /*<QAIO028> MODE1           */
        {                                         /*<QAIO028> (          )    */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0026 );   /*<QAIO028>       */
            MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0026 );  /*<QAIO028>       */
                                                  /*<QAIO028>                 */
            retval = DFGC_NG;                     /*<QAIO028>                 */
        } /* DCOM_ACCESS_MODE1 == DCOM_ERR_READ5*//*<QAIO028>                 */
        else
        {                                         /* MODE1          OK        */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0005 );

            retval = DCOM_ACCESS_MODE1;           /*         Normal           */

        } /* pcom_acc_read:DCOM_ACCESS_MODE1 == DCOM_OK */

        rate = DCOM_RATE400K;                     /*<3210046>400K             */
        com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C *//*<3210046>*/

                                                  /*                 close    */
/*<3210046>com_acc_ret = pcom_acc_close( com_acc_id );*/ /*   C */
        com_acc_ret |= pcom_acc_close( com_acc_id ); /*   C *//*<3210046>     */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                 close    */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0006 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0002 );

            retval = DFGC_NG;                     /*         NG               */

        } /* pcom_acc_close != DCOM_OK */

    } /* pcom_acc_open == DCOM_OK */
    else
    {                                             /*                 Open     */
        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0x0007 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHK | 0x0003 );

        retval = DFGC_NG;                         /*         NG               */

    } /* pcom_acc_open != DCOM_NG */

    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHK | 0xFFFF );

    return retval;

}

/******************************************************************************/
/*                pfgc_fgic_modechange                                        */
/*                FuelGauge-IC                                                */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                unsigned char mode                                          */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fgic_modechange( unsigned char mode )
{
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned int    retval;                       /*                          */
    unsigned long   accs_option;                  /*                 Option   */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0000 );

    retval = DFGC_OK;                             /*                          */

/*<3130561>if( mode == DCOM_ACCESS_MODE2 )      */
/*<3130561>{                                    *//*             ROM          */
/*<3130561>    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0001 );*/
/*<3130561>                                     */
/*<3130561>gcom_acc_fgcmode = DCOM_ACCESS_MODE2;*//* comacc                   */
/*<3130561>                                     *//* I/F          EXPORTSYMBOL*/
/*<3130561>                                     *//*                          */
/*<3130561>                                     *//* MODE2  Open              */
/*<3130561>                                     */
/*<3130561>   }*/ /* mode== DCOM_ACCESS_MODE2 */

                                                  /*                 open     */
/*<PCIO034>    com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
    com_acc_ret = pfgc_fgic_comopen( &com_acc_id );          /*   C <PCIO034> */

    if( com_acc_ret == DCOM_OK )                  /*                 open     */
    {
        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0002 );

        if( mode == DCOM_ACCESS_MODE1 )           /* ROM  Normal              */
        {
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0003 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
            rate = DCOM_RATE400K;                 /* RATE  400K               */
            com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

            com_acc_rwdata.size   = 1;            /*                          */
/*<QAIO028> accs_option = DCOM_ACCESS_MODE2;                                  */
            accs_option = ( DCOM_ACCESS_MODE2 | DCOM_ERRLOG_NOWRITE );/*<QAIO028>*/
            com_acc_rwdata.option = &accs_option; /* ROM                      */

                                                  /* Normal          Cmd      */
            MFGC_FGRW_DTSET( com_acc_rwdata, ROMCmdAdr, NormalMode );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay( DFGC_WAIT_MODE_NML );         /* ROM  Normal    Wait *//*   V */

            gfgc_fgic_mode = DCOM_ACCESS_MODE1;   /*<3130560>                 */

        } /* mode == DCOM_ACCESS_MODE1 */
        else if( mode == DCOM_ACCESS_MODE2 )      /* Normal  ROM              */
        {
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0004 );

            com_acc_rwdata.size   = 1;            /*                          */
/*<QAIO028> accs_option = DCOM_ACCESS_MODE1;                                  */
            accs_option = ( DCOM_ACCESS_MODE1 | DCOM_ERRLOG_NOWRITE | DCOM_SEQUENTIAL_ADDR);
            com_acc_rwdata.option = &accs_option; /* Normal                   */

                                                  /* ROM          Cmd         */
            MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, BootRomMode );
            com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

            mdelay( DFGC_WAIT_MODE_ROM );         /* Normal  ROM    Wait *//*   V */

            gfgc_fgic_mode = DCOM_ACCESS_MODE2;   /*<3130560>                 */

        } /* mode == DCOM_ACCESS_MODE2 */
        else
        {                                         /* mode                     */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0005 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHG | 0x0001 );

            MFGC_SYSERR_DATA_SET(                     /* SYSERR     <PCIO034> */
                 CSYSERR_ALM_RANK_B,                             /* <PCIO034> */
                 DFGC_SYSERR_PARAM_ERR,                          /* <PCIO034> */
                 ( DFGC_FGIC_MDCHG | 0x0001 ),                   /* <PCIO034> */
                 mode,                                           /* <PCIO034> */
                 syserr_info );                                  /* <PCIO034> */
            pfgc_log_syserr( &syserr_info );          /*   V        <PCIO034> */

            retval = DFGC_NG;                     /*       NG                 */

        } /* mode == ??? */

        if( com_acc_ret != DCOM_OK )
        {                                         /* comacc                   */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0006 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHG | 0x0002 );

            retval = DFGC_NG;                     /*       NG                 */

        } /* pcom_acc_write == DCOM_NG */

                                                  /*                 close    */
        com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

        if( com_acc_ret != DCOM_OK )
        {                                         /*                 close    */
            MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0007 );
            MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHG | 0x0003 );

            retval = DFGC_NG;                     /*       NG                 */

        } /* pcom_acc_close != DCOM_OK */

    } /* pcom_acc_open == DCOM_OK */
    else
    {                                             /*                 open     */
        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0008 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_MDCHG | 0x0004 );

        retval = DFGC_NG;                         /*       NG                 */

    } /* pcom_acc_open != DCOM_NG */

/*<3130561>    if( mode == DCOM_ACCESS_MODE1 )  */
/*<3130561>    {                                *//*             Normal       */
/*<3130561>        MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0x0009 );              */
/*<3130561>                                     */
/*<3130561>gcom_acc_fgcmode = DCOM_ACCESS_MODE1;*//* comacc                   */
/*<3130561>                                     *//* I/F          EXPORTSYMBOL*/
/*<3130561>                                     *//*                          */
/*<3130561>                                     *//* MODE1  Close             */
/*<3130561>    }*/                               /* mode == DCOM_ACCESS_MODE1 */

    MFGC_RDPROC_PATH( DFGC_FGIC_MDCHG | 0xFFFF );

    return retval;

}

/******************************************************************************/
/*                pfgc_fw_rowwrite                                            */
/*                FuelGauge-IC FW 1Row Write                                  */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                TCOM_ID com_acc_id                     ID                   */
/*                unsigned char type             type  Inst/DF)               */
/*                unsigned short row_no              row                      */
/*                unsigned char * rw_data                                     */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_fw_rowwrite( TCOM_ID com_acc_id, unsigned char type,
                       unsigned short row_no , unsigned char * rw_data )
{
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   writecmd[3];                  /* WriteCmd                 */
/*<3210043>unsigned int    retval,i,j;          *//*                          */
    unsigned int    retval,j;                     /*<3210043>          loopcnt*/
    unsigned short  row_size;                     /* Row                      */
    unsigned short  chksumcmd[1];                 /*                          */
    unsigned long   checksum_work;                /*                          */
    unsigned char   read_sts[1];                  /*                          */
    unsigned long   accs_option;                  /*                 Option   */
    unsigned long   wait_time;                    /*         Wait             */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */

    MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0000 );

    retval = DFGC_OK;                             /*                          */
    row_size = DFGC_ROW_SIZE_INST;                /* Row                      */
    wait_time = DFGC_ROWWRITE_WAIT_INST;          /*         Wait             */

    if( type == DFGC_WRTREQ_INST )
    {                                             /*             Instruction  */
        MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0001 );

        writecmd[0] = DFGC_ROWWRTCMD_INST;        /* WriteCmd  Instruction    */
        row_size = DFGC_ROW_SIZE_INST;            /* Row                      */
        wait_time = DFGC_ROWWRITE_WAIT_INST;      /* Instruction        Wait  */
        if(  row_no <= 1 )                        /*<3210044>     2Row        */
        {                                         /*<3210044>                 */
            wait_time = DFGC_ROWWRITE_WAIT_INST_2ROW; /*<3210044> Wait        */
        }                                         /*<3210044>                 */

    } /* type == DFGC_WRTREQ_INST */
    else if( type == DFGC_WRTREQ_DF )
    {                                             /*             DataFlash    */
        MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0002 );

        writecmd[0] = DFGC_ROWWRTCMD_DF;          /* WriteCmd  DF             */
        row_size = DFGC_ROW_SIZE_DF;              /* Row                      */
        wait_time = DFGC_ROWWRITE_WAIT_DF;        /* DataFlash            Wait*/

    } /* type == DFGC_WRTREQ_DF */
    else
    {                                             /* Type                     */
        MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0003 );
        MFGC_RDPROC_ERROR( DFGC_FW_ROWWRITE | 0x0001 );

        MFGC_SYSERR_DATA_SET(                     /* SYSERR         <PCIO034> */
             CSYSERR_ALM_RANK_B,                                 /* <PCIO034> */
             DFGC_SYSERR_PARAM_ERR,                              /* <PCIO034> */
             ( DFGC_FW_ROWWRITE | 0x0001 ),                      /* <PCIO034> */
             type,                                               /* <PCIO034> */
             syserr_info );                                      /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*   V SYSERR     <PCIO034> */

        retval = DFGC_NG;                         /*         NG               */

    } /* type == ??? */

    writecmd[1] = ( unsigned char )row_no;        /* WriteCmd Row#(  8bit)    */
    writecmd[2] = ( unsigned char )( ( ( unsigned short )( row_no >> 8 ) ) & 0x00ff ); /*   8bit    */

    com_acc_rwdata.size   = 1;                    /*                          */

/*<3210043>for( i = 0; i < DFGC_ROWWRT_RTRYCNT; i++ )*/ /*   R */
/*<3210043>    {                                *//*                          */
/*<3210043> MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0004 ); */

/*<QAIO028>accs_option = DCOM_ACCESS_MODE2;     *//*               (    )     */
        accs_option = ( DCOM_ACCESS_MODE2 |       /*<QAIO028>                 */
                        DCOM_ERRLOG_NOWRITE );    /*<QAIO028>         (    )  */
        com_acc_rwdata.option = &accs_option;     /* ROM                      */

                                                  /* WriteCmd                 */
        MFGC_FGRW_DTSET( com_acc_rwdata, ROMWrCmdAdr, writecmd );
        com_acc_ret = pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

/*<QAIO028>accs_option = DCOM_ACCESS_MODE2 | DCOM_SEQUENTIAL_ADDR;*//* Sequential */
        accs_option = ( DCOM_ACCESS_MODE2 |       /*<QAIO028>Sequential       */
                        DCOM_SEQUENTIAL_ADDR |    /*<QAIO028>    Write        */
                        DCOM_ERRLOG_NOWRITE );    /*<QAIO028>                 */
        com_acc_rwdata.offset = ( unsigned long * )ROMWrDatAdr; /* WriteData  */
        com_acc_rwdata.data   = rw_data;          /* Waite                    */
        com_acc_rwdata.number = row_size;         /*             =RowSize     */

                                                  /* Write                    */
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /*                          */

                                                  /* Cmd                      */
        checksum_work = ( unsigned long )writecmd[ 0 ]
                       + ( unsigned long )writecmd[ 1 ]
                       + ( unsigned long )writecmd[ 2 ];

                                                  /* WriteData                */
        for( j = 0; j < row_size; j++ )           /*   D */
        {                                         /* Row Size                 */
/*<3210043>  MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0005 ); */

            checksum_work += ( unsigned long )( *( rw_data + j ) );

        }

                                                  /* mod 0x10000              */
        chksumcmd[0] = ( unsigned short )checksum_work;

                                                  /*                          */
        MFGC_FGRW_DTSET( com_acc_rwdata, ROMWrSumAdr, chksumcmd );
        com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

        mdelay( wait_time );                      /* type        Wait *//*   V */

        read_sts[0] = 0xFF;                       /*           Work           */

                                                  /*                   Read   */
        accs_option = DCOM_ACCESS_MODE2;          /*<1100142>           (    )*/
        MFGC_FGRW_DTSET( com_acc_rwdata, ROMRwSts, read_sts );
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

/*<3210043>if( read_sts[0] == 0x00 )               */
/*<3210043>{                                       *//*         OK               */
/*<3210043>    MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0006 );                    */
/*<3210043>                                                                      */
/*<3210043>    break;                              *//* loop                     */
/*<3210043>                                                                      */
/*<3210043>}*/                                            /* read_sts[0] == 0x00 */
/*<3210043>else                                                                  */
/*<3210043>{                                       *//*         NG               */
/*<3210043>    MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0007 );                    */
/*<3210043>                                                                      */
/*<3210043>    continue;                           *//*                          */
/*<3210043>                                                                      */
/*<3210043>}*/                                            /* read_sts[0] != 0x00 */

/*<3210043>    }*/ /*  for( i = 0; i < DFGC_ROWWRT_RTRYCNT; i++ ) */


    if( com_acc_ret != DCOM_OK )
    {                                             /*                          */
        MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0008 );
        MFGC_RDPROC_ERROR( DFGC_FW_ROWWRITE | 0x0002 );

        retval = DFGC_NG;                         /*       NG                 */

    } /* pcom_acc_read/write != DCOM_OK */

/*<3210043>if( i == DFGC_ROWWRT_RTRYCNT )       */
/*<3210043>{                                    *//*                          */
    if( read_sts[0] != 0x00 )                     /*<3210043>                 */
    {                                             /*<3210043>                 */
        MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0x0009 );
        MFGC_RDPROC_ERROR( DFGC_FW_ROWWRITE | 0x0003 );

        MFGC_SYSERR_DATA_SET(                     /* SYSERR         <PCIO034> */
             CSYSERR_ALM_RANK_B,                                 /* <PCIO034> */
             DFGC_SYSERR_INTERNAL_ERR,                           /* <PCIO034> */
             ( DFGC_FW_ROWWRITE | 0x0003 ),                      /* <PCIO034> */
             ( unsigned long )NULL,                              /* <PCIO034> */
             syserr_info );                                      /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*   V SYSERR     <PCIO034> */

        retval = DFGC_NG;                         /*       NG                 */

/*<3210043>}*/                                    /* i == DFGC_ROWWRT_RTRYCNT */
    } /* read_sts[0] != 0x00 */

    MFGC_RDPROC_PATH( DFGC_FW_ROWWRITE | 0xFFFF );

    return retval;
}

/******************************************************************************/
/*                pfgc_fw_rowread                                             */
/*                FuelGauge-IC FW 1Row Read(INST)                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                TCOM_ID com_acc_id                     ID                   */
/*                unsigned short row_no              row                      */
/*                unsigned char * rw_data                                     */
/******************************************************************************/
unsigned int pfgc_fw_rowread( TCOM_ID com_acc_id,
                       unsigned short row_no , unsigned char * rw_data )
{
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   writecmd[3];                  /* ReadCmd  Work            */
    unsigned int    retval,i;                     /*         Loop  Work       */
    unsigned long   accs_option;                  /*                 Option   */
    unsigned long   offset_adr;                   /*                          */
    unsigned char * read_adr;                     /* Read                     */
    unsigned short  chksumcmd[1];                 /*                          */
    unsigned long   checksum_work;                /*                          */

    MFGC_RDPROC_PATH( DFGC_FW_ROWREAD | 0x0000 );

    retval = DFGC_OK;                             /*                          */

                                                  /* ReadCmd                  */
    writecmd[0] = 0x00;
    writecmd[1] = ( unsigned char )row_no;
    writecmd[2] = ( unsigned char )( ( ( unsigned short )( row_no >> 8 ) ) & 0x00ff ); /*   8bit    */

    com_acc_rwdata.size   = 1;                    /*                          */
/*<QAIO028>accs_option = DCOM_ACCESS_MODE2;                                   */
    accs_option = ( DCOM_ACCESS_MODE2 | DCOM_ERRLOG_NOWRITE );     /*<QAIO028>*/
    com_acc_rwdata.option = &accs_option;         /* ROM                      */

                                                  /* ReadCmd                  */
    MFGC_FGRW_DTSET( com_acc_rwdata, ROMWrCmdAdr, writecmd );
    com_acc_ret = pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                                                  /*                          */
    checksum_work = ( unsigned long )writecmd[ 0 ]
                   + ( unsigned long )writecmd[ 1 ]
                   + ( unsigned long )writecmd[ 2 ];
    chksumcmd[0] = ( unsigned short )checksum_work;

                                                  /*                          */
    MFGC_FGRW_DTSET( com_acc_rwdata, ROMWrSumAdr, chksumcmd );
    com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

    offset_adr = 0x04;                            /* Row                      */
    read_adr = rw_data;                           /*                          */

    for( i = 0; i < DFGC_ROW_SIZE_INST; i++ )     /*   D */
    {                                             /* RowSize  INST            */
        MFGC_RDPROC_PATH( DFGC_FW_ROWREAD | 0x0001 );

        com_acc_rwdata.offset = &offset_adr;      /*                          */
        com_acc_rwdata.data   = read_adr;         /*                          */
        com_acc_rwdata.number = 1;                /*                          */

                                                  /* Row                      */
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

        read_adr++;                               /*                          */
        offset_adr = offset_adr + 1;              /*                          */
    } /* for( i = 0; i < DFGC_ROW_SIZE_INST; i++ ) */

    if( com_acc_ret != DCOM_OK )
    {                                             /*                          */
        MFGC_RDPROC_PATH( DFGC_FW_ROWREAD | 0x0002 );
        MFGC_RDPROC_ERROR( DFGC_FW_ROWREAD | 0x0001 );

        retval = DFGC_NG;                         /*       NG                 */

    } /* pcom_acc_read != DCOM_OK */

    MFGC_RDPROC_PATH( DFGC_FW_ROWREAD | 0xFFFF );

    return retval;
}

#if 0 /*             Read             */
int pfgc_fw_rowread_df( TCOM_ID com_acc_id,
                       unsigned short row_no , unsigned char * rw_data )
{
    TCOM_FUNC       com_acc_ret;                 /*                           */
    TCOM_RW_DATA    com_acc_rwdata;              /*                           */
    unsigned char   writecmd[3];
    int             retval,i;
    unsigned short  row_size;
    unsigned long   accs_option;
    unsigned long   wait_time;
    unsigned long   offset_adr;
    unsigned char * read_adr;
    unsigned long   checksum_work;
    unsigned short  chksumcmd[1];

    MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0000 );

    retval = DFGC_OK;
    row_size = DFGC_ROW_SIZE_DF;
    wait_time = DFGC_ROWWRITE_WAIT_DF;

    writecmd[0] = 0x08;
    writecmd[1] = ( unsigned char )row_no;
    writecmd[2] = ( unsigned char )( ( row_no >> 8 ) & 0x00ff );

    com_acc_rwdata.size   = 1;                        /*                  */
    accs_option = DCOM_ACCESS_MODE2;
    com_acc_rwdata.option = &accs_option     ;/* ROM                      */

    MFGC_FGRW_DTSET( com_acc_rwdata, ROMWrCmdAdr, writecmd );
    com_acc_ret = pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata );

    checksum_work = ( unsigned long )writecmd[ 0 ]
                   + ( unsigned long )writecmd[ 1 ]
                   + ( unsigned long )writecmd[ 2 ];
    chksumcmd[0] = ( unsigned short )checksum_work;

    MFGC_FGRW_DTSET( com_acc_rwdata, ROMWrSumAdr, chksumcmd );
    com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata );

    offset_adr = 0x04;
    read_adr = rw_data;

    for( i = 0; i < DFGC_ROW_SIZE_DF; i++ )
    {

        com_acc_rwdata.offset = &offset_adr;         /*                           */
        com_acc_rwdata.data   = read_adr; /*                           */
        com_acc_rwdata.number = 1; /*                           */
        com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata );

        read_adr++;
        offset_adr = offset_adr + 1 ;
    }


    if( com_acc_ret != DCOM_OK )
    {
        MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0x0002 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_DFWRITE | 0x0001 );

        retval = DFGC_NG;

    }

    MFGC_RDPROC_PATH( DFGC_FGIC_DFWRITE | 0xFFFF );

    return retval;
}
#endif /*             Read             */

/******************************************************************************/
/*                pfgc_fgic_comread                                           */
/*                FuelGauge-IC command read                                   */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                TCOM_ID comread_id                     ID  open             */
/*                TCOM_RW_DATA * comread_rwdata                 RW            */
/*                TCOM_RW_DATA * comread_rwdata                 RW            */
/******************************************************************************/
TCOM_FUNC pfgc_fgic_comread( TCOM_ID comread_id, TCOM_RW_DATA * comread_rwdata )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_FUNC         comread_ret;                /*                          */

    FGC_FUNCIN("\n");

    MFGC_RDPROC_PATH( DFGC_FGIC_COMREAD | 0x0000 );

    comread_ret = pcom_acc_read( comread_id, DCOM_DEV_DEVICE4, comread_rwdata ); /*   C */

    if( comread_ret != DCOM_OK )                  /*                 write    */
    {
        pfgc_log_i2c_error(PFGC_FGIC_COMREAD + 0x01, 0);
        MFGC_RDPROC_PATH( DFGC_FGIC_COMREAD | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_COMREAD | 0x0001 );

    }

    MFGC_RDPROC_PATH( DFGC_FGIC_COMREAD | 0xFFFF );

    return comread_ret;
}

/******************************************************************************/
/*                pfgc_fw_com_write                                           */
/*                FuelGauge-IC command write                                  */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                TCOM_ID comread_id                     ID  open             */
/*                TCOM_RW_DATA * comread_rwdata                 RW            */
/*                                                                            */
/******************************************************************************/
TCOM_FUNC pfgc_fgic_comwrite( TCOM_ID comwrite_id, TCOM_RW_DATA * comwrite_rwdata )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_FUNC         comwrite_ret;               /*                          */

    FGC_FUNCIN("\n");
    MFGC_RDPROC_PATH( DFGC_FGIC_COMWRITE | 0x0000 );

    comwrite_ret = pcom_acc_write( comwrite_id, DCOM_DEV_DEVICE4, comwrite_rwdata ); /*   C */

    if( comwrite_ret != DCOM_OK )                  /*                 write    */
    {
        pfgc_log_i2c_error(PFGC_FGIC_COMWRITE + 0x01, 0);
        MFGC_RDPROC_PATH( DFGC_FGIC_COMWRITE | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_FGIC_COMWRITE | 0x0001 );

    }

    MFGC_RDPROC_PATH( DFGC_FGIC_COMWRITE | 0xFFFF );

    return comwrite_ret;
}

/*******************************************************************<PCIO034>**/
/*                pfgc_fgic_comopen                                 <PCIO034> */
/*                                open                              <PCIO034> */
/*                2009.02.05                                        <PCIO034> */
/*                NTTDMSE                                           <PCIO034> */
/*                DCOM_OK                                           <PCIO034> */
/*                DCOM_NG                                           <PCIO034> */
/*                TCOM_ID * comread_id                     ID       <PCIO034> */
/*                TCOM_ID * comread_id                     ID       <PCIO034> */
/*******************************************************************<PCIO034>**/
TCOM_FUNC pfgc_fgic_comopen( TCOM_ID * comopen_id )              /* <PCIO034> */
{                                                                /* <PCIO034> */
    /*--------------------------------------------------------------<PCIO034>-*/
    /*                                                              <PCIO034> */
    /*--------------------------------------------------------------<PCIO034>-*/
    TCOM_FUNC         com_acc_ret;                /*     ACC        <PCIO034> */
    T_FGC_SYSERR_DATA syserr_info;                /* SYSERR         <PCIO034> */
    unsigned long     cnt;                        /*                <PCIO034> */
    signed long       timeout;                    /*                <PCIO034> */
                                                                 /* <PCIO034> */
                                                                 /* <PCIO034> */
    MFGC_RDPROC_PATH( DFGC_FGIC_COMOPEN | 0x0000 );              /* <PCIO034> */
                                                                 /* <PCIO034> */
    for( cnt = 0; cnt < DFGC_FGC_COM_WAIT_CNT; cnt++ ) /*  D        <PCIO034> */
    {                                                            /* <PCIO034> */
        com_acc_ret = pcom_acc_open(              /*  C     ACC open<PCIO034> */
                        DCOM_DEV_DEVICE4,                        /* <PCIO034> */
                        comopen_id,                              /* <PCIO034> */
                        NULL );                                  /* <PCIO034> */
                                                                 /* <PCIO034> */
        if( com_acc_ret == DCOM_OK )              /*       OK       <PCIO034> */
        {                                                        /* <PCIO034> */
            MFGC_RDPROC_PATH( DFGC_FGIC_COMOPEN | 0x0001 );      /* <PCIO034> */
                                                                 /* <PCIO034> */
            break;                                               /* <PCIO034> */
        }                                                        /* <PCIO034> */
        else                                      /* OK             <PCIO034> */
        {                                                        /* <PCIO034> */
            MFGC_RDPROC_PATH( DFGC_FGIC_COMOPEN | 0x0002 );      /* <PCIO034> */
            set_current_state( TASK_INTERRUPTIBLE ); /*  V          <PCIO034> */
                                                  /*          WAIT  <PCIO034> */
            timeout = schedule_timeout(           /*  C             <PCIO034> */
                          DFGC_FGC_COM_WAIT_TIMEOUT );           /* <PCIO034> */
                                                                 /* <PCIO034> */
        }                                                        /* <PCIO034> */
    }                                                            /* <PCIO034> */
                                                                 /* <PCIO034> */
    if( cnt >= DFGC_FGC_COM_WAIT_CNT )            /* open           <PCIO034> */
    {                                                            /* <PCIO034> */
        MFGC_RDPROC_PATH( DFGC_FGIC_COMOPEN | 0x0003 );          /* <PCIO034> */
        MFGC_RDPROC_ERROR( DFGC_FGIC_COMOPEN | 0x0001 );         /* <PCIO034> */
        MFGC_SYSERR_DATA_SET2(                    /* SYSERR         <PCIO034> */
                    CSYSERR_ALM_RANK_A,                          /* <PCIO034> */
                    DFGC_SYSERR_COM_OPEN_ERR,                    /* <PCIO034> */
                    ( DFGC_FGIC_COMOPEN | 0x0001 ),              /* <PCIO034> */
                    DCOM_DEV_DEVICE4,                            /* <PCIO034> */
                    com_acc_ret,                                 /* <PCIO034> */
                    syserr_info );                               /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*  V SYSERR      <PCIO034> */
    }                                                            /* <PCIO034> */
                                                                 /* <PCIO034> */
    MFGC_RDPROC_PATH( DFGC_FGIC_COMOPEN | 0xFFFF );              /* <PCIO034> */
                                                                 /* <PCIO034> */
    return com_acc_ret;                                          /* <PCIO034> */
}                                                                /* <PCIO034> */

