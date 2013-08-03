/*
 * drivers/fgc/pfgc_ver.c
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
/*                pfgc_ver_init                                               */
/*                        DD                  init                            */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
unsigned char pfgc_ver_init( void )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    unsigned char ret;                            /*                          */

    MFGC_RDPROC_PATH( DFGC_VER_INIT | 0x0000 );

    ret = DFGC_OK;

    gfgc_verup_type = DFGC_VERUP_INSTDF;          /* VerUp type               */
    gfgc_fgic_mode = pfgc_fgic_modecheck();       /*<3130560> FGICMode        */
    if( gfgc_fgic_mode == DFGC_NG )               /*<3130560>       NG        */
    {                                             /*<3130560>                 */
        MFGC_RDPROC_PATH( DFGC_VER_INIT | 0x0001 );/*<3130560>                */
        gfgc_fgic_mode = DCOM_ACCESS_MODE2;       /*<3130560> ROMmode         */
    }                                             /*<3130560>                 */

    MFGC_RDPROC_PATH( DFGC_VER_INIT | 0xFFFF );

    return ret;                                   /* return                   */
}

/******************************************************************************/
/*                pfgc_ver_read                                               */
/*                        DD                  Version                         */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                T_FGC_FW * fw_data FW                                       */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_ver_read( T_FGC_FW_VER * fw_ver_data )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    TCOM_ID         com_acc_id;                   /*                 ID       */
    TCOM_FUNC       com_acc_ret;                  /*                          */
    TCOM_RW_DATA    com_acc_rwdata;               /*                          */
    unsigned char   read_reg_inst[2];             /* InstructionVer Read  Work*/
    T_FGC_FW_COM_MEMMAP * rom_va;                 /* NOR    FW    ioremap     */
    unsigned char   read_reg_df[4];               /* DataFlashVer Read  Work  */
    unsigned int    retval;                       /*                          */
    unsigned long   accs_option;                  /*                 Option   */
    unsigned int    old_status;                   /* Seal/FAS                 */
    T_FGC_SYSERR_DATA    syserr_info;             /* SYSERR         <PCIO034> */
    int             nand_ret;                     /*<QAIO027> NAND            */
#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
    unsigned long   rate;                         /* I2C        Rate          */
#endif /* DVDV_FGC_I2C_RATE_CONST */

    MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0000 );

    retval = DFGC_OK;                             /*                          */



/*<QAIO027>    rom_va =                         *//* ROM    FW                */
/*<QAIO027>        ( T_FGC_FW_COM_MEMMAP * )ioremap(                 *//*   C */
/*<QAIO027>           DMEM_NOR_FGC_FGICFW,                                    */
    rom_va = ( T_FGC_FW_COM_MEMMAP * )vmalloc(    /*<QAIO027>  C              */
           ( unsigned long )( sizeof( T_FGC_FW_COM_MEMMAP ) ) );

    if( rom_va == NULL )
    {                                             /* ioremap                  */
        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_VER_READ | 0x0001 );

        MFGC_SYSERR_DATA_SET(                     /* SYSERR         <PCIO034> */
                    CSYSERR_ALM_RANK_B,                          /* <PCIO034> */
                    DFGC_SYSERR_IOREMAP_ERR,                     /* <PCIO034> */
                    ( DFGC_VER_READ | 0x0001 ),                  /* <PCIO034> */
                    ( unsigned long )rom_va,                     /* <PCIO034> */
                    syserr_info );                               /* <PCIO034> */
        pfgc_log_syserr( &syserr_info );          /*  V SYSERR      <PCIO034> */

        retval = DFGC_NG;                         /*       NG                 */

    } /* rom_va == NULL */
    else
    {                                             /* ioremap                  */
        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0010 );         /*<QAIO027>       */
                                                            /*<QAIO027>       */
        nand_ret = mtdwrap_read_bbm(
            MMC_CFG_TBL_PART,   /* IDPower */
            MMC_FGIC_FW_OFFSET, /* IDPower */
            sizeof(T_FGC_FW_COM_MEMMAP),
            (void *)rom_va);
        if( nand_ret != 0 )
        {                                                   /*<QAIO027>       */
            MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0011 );     /*<QAIO027>       */
            MFGC_RDPROC_ERROR( DFGC_VER_READ | 0x0011 );    /*<QAIO027>       */
                                                            /*<QAIO027>       */
            MFGC_SYSERR_DATA_SET(                           /*<QAIO027>SYSERR */
                        CSYSERR_ALM_RANK_B,                 /*<QAIO027>       */
                        DFGC_SYSERR_NVM_ERR,                /*<QAIO027>       */
                        ( DFGC_VER_READ | 0x0011 ),         /*<QAIO027>       */
                        ( unsigned long )nand_ret,          /*<QAIO027>       */
                        syserr_info );                      /*<QAIO027>       */
            pfgc_log_syserr( &syserr_info );                /*<QAIO027>  V    */
                                                            /*<QAIO027>       */
            retval = DFGC_NG;                               /*<QAIO027>       */
        }                                                   /*<QAIO027>       */
    }                                                       /*<QAIO027>       */
                                                  /*<QAIO027>                 */
    if( retval == DFGC_OK )                       /*<QAIO027>                 */
    {                                             /*<QAIO027>                 */
        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0002 );

                                                  /* Nor    InstructionVer    */
        fw_ver_data->inst_ver_int = ( unsigned short )(
                            ( ( unsigned long )( rom_va->rom_inst_ver << 8 ) & 0xFF00 ) +
                            ( ( unsigned long )( rom_va->rom_inst_ver >> 8 ) & 0x00FF ) );

                                                  /* Nor    DataFlashVer      */
        fw_ver_data->df_ver_int =
                           ( ( rom_va->rom_df_ver << 24 ) & 0xFF000000 ) +
                           ( ( rom_va->rom_df_ver << 8  ) & 0x00FF0000 ) +
                           ( ( rom_va->rom_df_ver >> 8  ) & 0x0000FF00 ) +
                           ( ( rom_va->rom_df_ver >> 24 ) & 0x000000FF );

        FGC_FWUP( "NAND : INST Ver=0x%04X, DF Ver=0x%08lX\n", fw_ver_data->inst_ver_int, fw_ver_data->df_ver_int );

        if(   ( fw_ver_data->inst_ver_int != 0xFFFF )
           && ( fw_ver_data->inst_ver_int != 0x0000 )
           && ( fw_ver_data->df_ver_int != 0xFFFFFFFF )
           && ( fw_ver_data->df_ver_int != 0x00000000 ) )
        {                                         /* Ver                      */
            MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0003 );
            FGC_FWUP("Valid FW Version.\n");

                                                  /* FullAccessMode           */
            retval = pfgc_fw_fullaccessmode_set( &old_status ); /*   N                        */

                                                  /*                 open     */
/*<PCIO034>            com_acc_ret = pcom_acc_open( DCOM_DEV_DEVICE4, &com_acc_id, NULL ); *//*   C */
            com_acc_ret = pfgc_fgic_comopen( &com_acc_id );  /*   C <PCIO034> */

            if( com_acc_ret == DCOM_OK )          /*                 open     */
            {
                MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0004 );

#ifndef DVDV_FGC_I2C_RATE_CONST                   /*                          */
                rate = DCOM_RATE400K;             /* RATE  400K               */
                com_acc_ret = pcom_acc_ioctl( com_acc_id, DCOM_DEV_DEVICE4, DCOM_IOCTL_RATE, &rate ); /*   C */
#endif /* DVDV_FGC_I2C_RATE_CONST */

                com_acc_rwdata.size   = 1;        /*                          */
                accs_option = DCOM_ACCESS_MODE1 | DCOM_SEQUENTIAL_ADDR;
                com_acc_rwdata.option = &accs_option;/* Normal                */

                                                  /* FW Version Read Cmd      */
                MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, FwVersionCmd );
                com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                read_reg_inst[0] = 0xFF;          /* Work                     */
                read_reg_inst[1] = 0xFF;          /*                          */

                com_acc_rwdata.option = NULL;     /*<QAIO026> option          */
                                                  /* FW Version Read          */
                MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, read_reg_inst );
                com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                com_acc_rwdata.option = &accs_option;  /*<QAIO026> option     */
                                                  /* CTRL_STATUS Cmd          */
                MFGC_FGRW_DTSET( com_acc_rwdata, CtrlRegAdr, CtrlStsCmd );
                com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                accs_option = DCOM_ACCESS_MODE1;
                com_acc_rwdata.option = &accs_option;/* Normal                */
                                                  /* DF Version Read Cmd      */
                MFGC_FGRW_DTSET( com_acc_rwdata, DFVerReadAdr, DFVerReadCmd );
                com_acc_ret |= pfgc_fgic_comwrite( com_acc_id, &com_acc_rwdata ); /*   C */

                read_reg_df[0] = 0xFF;            /* DF Read Work             */
                read_reg_df[1] = 0xFF;            /*                          */
                read_reg_df[2] = 0xFF;            /*                          */
                read_reg_df[3] = 0xFF;            /*                          */

                com_acc_rwdata.option = NULL;     /*<QAIO026> option          */
                                                  /* DataFlash Version Read   */
                MFGC_FGRW_DTSET( com_acc_rwdata, DFVerAdr, read_reg_df );
                com_acc_ret |= pfgc_fgic_comread( com_acc_id, &com_acc_rwdata ); /*   C */

                if( com_acc_ret == DFGC_OK )
                {                                 /* comacc                   */
                    MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0005 );

                                                  /* FGIC  FW Ver             */
                    fw_ver_data->inst_ver_fgic = ( unsigned short )
                                            ( ( unsigned long )read_reg_inst[0] +
                                              ( unsigned long )( read_reg_inst[1] << 8 ) );

                    fw_ver_data->df_ver_fgic = ( unsigned long )read_reg_df[0] +
                                               ( unsigned long )( read_reg_df[1] << 8 ) +
                                               ( unsigned long )( read_reg_df[2] << 16 ) +
                                               ( unsigned long )( read_reg_df[3] << 24 );

                    FGC_FWUP( "FGIC : INST Ver=0x%04X, DF Ver=0x%08lX\n", fw_ver_data->inst_ver_fgic, fw_ver_data->df_ver_fgic );

                    if( fw_ver_data->inst_ver_fgic < fw_ver_data->inst_ver_int )
                    {                             /* FGIC  InstVer            */
                        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0006 );
                        FGC_FWUP("Inst+DF : Version Up.\n");

                        fw_ver_data->ver_up = DFGC_VER_UP;  /* VerUP          */
                        gfgc_verup_type = DFGC_VERUP_INSTDF;/* INST+DF        */

                    } /* inst_ver_fgic < inst_ver_int */
                    else if( ( fw_ver_data->df_ver_fgic & DFGC_MASK_DFVER_MODEL )
                             == ( fw_ver_data->df_ver_int & DFGC_MASK_DFVER_MODEL ) )
                    {                             /* DF                       */
                        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0007 );

                        if( ( fw_ver_data->df_ver_fgic & DFGC_MASK_DFVER_VER )
                            < ( fw_ver_data->df_ver_int & DFGC_MASK_DFVER_VER ) )
                        {                         /* FGIC  DFVer              */
                            MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0008 );
                            FGC_FWUP( "DF : Version Up.\n" );

                            fw_ver_data->ver_up = DFGC_VER_UP; /* VerUP       */
                            gfgc_verup_type = DFGC_VERUP_DF;   /* DF          */

                        } /* df_ver_fgic < df_ver_int (VER) */
                        else
                        {                         /* FGIC  DFVer              */
                            MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0009 );
                            FGC_FWUP( "FGIC Ver is not old, don't update.\n" );

                            fw_ver_data->ver_up = DFGC_VER_DOWN; /* VerUP     */

                        } /* df_ver_fgic >= df_ver_int (VER) */

                    } /* df_ver_fgic == df_ver_int (MODEL) */
                    else
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x000A );
                        FGC_FWUP( "DFVER_MODEL is mismatched.\n" );

                        fw_ver_data->ver_up = DFGC_VER_DOWN;  /* VerUP        */
                    }

                } /* pcom_acc_read/write == DCOM_OK */
                else
                {
                    MFGC_RDPROC_PATH( DFGC_VER_READ | 0x000B );
                    MFGC_RDPROC_ERROR( DFGC_VER_READ | 0x0002 );

                    retval = DFGC_NG;             /*       NG                 */

                } /* pcom_acc_read/write != DCOM_OK */

                                                  /*                 close    */
                com_acc_ret = pcom_acc_close( com_acc_id ); /*   C */

                if( com_acc_ret != DCOM_OK )
                {                                 /*                 close    */
                    MFGC_RDPROC_PATH( DFGC_DFS_READ | 0x000C );
                    MFGC_RDPROC_ERROR( DFGC_DFS_READ | 0x0003 );

                    retval = DFGC_NG;             /*       NG                 */

                } /* pcom_acc_close != DCOM_OK */

            } /* pcom_acc_open == DCOM_OK */
            else
            {                                     /*                 open     */
                    MFGC_RDPROC_PATH( DFGC_VER_READ | 0x000D );
                    MFGC_RDPROC_ERROR( DFGC_VER_READ | 0x0004 );

                    retval = DFGC_NG;             /*       NG                 */

            } /* pcom_acc_open != DCOM_OK */

                                                  /* FullAccessMode           */
            retval |= pfgc_fw_fullaccessmode_rev( &old_status ); /*   N                        */

        } /* inst_ver_int df_ver_int       */
        else
        {                                         /* NOR    Ver               */
                MFGC_RDPROC_PATH( DFGC_VER_READ | 0x000E );
                FGC_FWUP( "Invalid FW Version.\n" );

                fw_ver_data->ver_up = DFGC_VER_DOWN; /* VerUP                 */
                gfgc_verup_type = DFGC_VERUP_NONE;   /* FW                    */

        } /*inst_ver_int df_ver_int       */

    } /* rom_va != NULL */

    if( rom_va != NULL )                                /*<QAIO027>           */
    {                                                   /*<QAIO027>           */
        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x0012 );     /*<QAIO027>           */
                                                        /*<QAIO027>           */
        vfree( rom_va );                                /*<QAIO027>  V        */
        rom_va = NULL;                                  /*<QAIO027>           */
    }                                                   /*<QAIO027>           */

    if( gfgc_fgic_mode == DCOM_ACCESS_MODE2 )     /*<3130560>                 */
    {                                             /*<3130560>                 */
        MFGC_RDPROC_PATH( DFGC_VER_READ | 0x000F );/*<3130560>                */
        FGC_FWUP( "ROM mode. Inst+DF : Version Up.\n" );
                                                  /*<3130560>                 */
        fw_ver_data->ver_up = DFGC_VER_UP;        /*<3130560> VerUP           */
        fw_ver_data->inst_ver_fgic = 0x0000;      /*<3130560>     Ver         */
        fw_ver_data->df_ver_fgic   = 0x00000000;  /*<3130560>     Ver         */
        gfgc_verup_type = DFGC_VERUP_INSTDF;      /*<3130560> INST+DF         */
        retval = DFGC_OK;                         /*<3130560>                 */
                                                  /*<3130560>                 */
    }                                             /*<3130560>                 */

    MFGC_RDPROC_PATH( DFGC_VER_READ | 0xFFFF );

    return retval;

}

