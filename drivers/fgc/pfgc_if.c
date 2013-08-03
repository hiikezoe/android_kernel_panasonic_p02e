/*
 * drivers/fgc/pfgc_if.c
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
#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
static unsigned char                              /*<QCIO017>                 */
    gfgc_ibp_open_cnt[ DFGC_IBP_MINOR_MAX ] =     /*<QCIO017> IBP open        */
{                                                 /*<QCIO017>                 */
    0, 0                                          /*<QCIO017>                 */
};                                                /*<QCIO017>                 */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*                pfgc_init                                                   */
/*                        DD init                                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                0                                                           */
/*                -EIO               DD                                       */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
int __init pfgc_init( void )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    int ret;                                      /*                          */
    unsigned char init_ret;                       /*                          */
    struct task_struct *fgc_kthread_id;
    unsigned long flags;                          /*                          */
    int mode;                                     /* FG-IC                    */
    unsigned long wakeup;

#ifdef CONFIG_PROC_FS_FGC                         /*         DD proc          */
#if DFGC_SW_TEST_DEBUG
    struct proc_dir_entry *entry;                 /* proc                     */
#endif /* DFGC_SW_TEST_DEBUG */
#endif /* CONFIG_PROC_FS_FGC */                   /*         DD proc          */

    DEBUG_FGIC(("pfgc_init start\n"));

    MFGC_RDPROC_PATH( DFGC_INIT | 0x0000 );

#ifdef CONFIG_PROC_FS_FGC                         /*         DD proc          */
    gfgc_path_debug.path_cnt = DFGC_CLEAR;        /*                          */
    gfgc_path_error.path_cnt = DFGC_CLEAR;        /*                          */
#endif /* CONFIG_PROC_FS_FGC */                   /*         DD proc          */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    ret = pfgc_register_chrdev( DFGC_MAJOR, DFGC_DEV, &gfgc_fops );
#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
    ret |= register_chrdev( DFGC_IBP_MAJOR,       /*<QCIO017>  C              */
                            DIBP_DEV,             /*<QCIO017>                 */
                            &gfgc_fops );         /*<QCIO017>                 */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */
                                                  /*         DD               */
    if( ret == 0 )                                /*         DD               */
    {
        MFGC_RDPROC_PATH( DFGC_INIT | 0x0001 );

        init_ret = DFGC_OK;                       /*                          */

        /*--------------------------------------------------------------------*/
        /*                                                                    */
        /*--------------------------------------------------------------------*/
        init_ret |= pfgc_adrd_init( );
        init_ret |= pfgc_info_init( );            /*  C                       */
        init_ret |= pfgc_ver_init( );             /*  C FW                    */
#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
        init_ret |= pfgc_ibp_init( );             /*<QCIO017>  C              */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */

        if( init_ret != DFGC_OK )                 /* init                     */
        {
            MFGC_RDPROC_PATH( DFGC_INIT | 0x0002 );

            pfgc_unregister_chrdev( DFGC_MAJOR, DFGC_DEV );
                                                  /*         DD               */

#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
            unregister_chrdev( DFGC_IBP_MAJOR,    /*<QCIO017>  N              */
                               DIBP_DEV );        /*<QCIO017>                 */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */

            MFGC_RDPROC_ERROR( DFGC_INIT | 0x0001 );
            ret = -EIO;                           /*                          */
        }
        else
        {
            MFGC_RDPROC_PATH( DFGC_INIT | 0x0003 );

            /*----------------------------------------------------------------*/
            /* read proc                                                      */
            /*----------------------------------------------------------------*/
#ifdef CONFIG_PROC_FS_FGC                         /*         DD proc          */
            pfgc_log_proc_init( );

#ifdef    SYS_HAVE_IBP                            /*<2040002>                 */
            entry = create_proc_read_entry(       /*<2040002>  C              */
                        "driver/ibp",             /*<2040002>                 */
                        0,                        /*<2040002>                 */
                        NULL,                     /*<2040002>                 */
                        pfgc_log_rdproc_ibp,      /*<2040002>                 */
                        NULL );                   /*<2040002>                 */
            if( entry == NULL )                   /*<2040002>                 */
            {                                     /*<2040002>                 */
                MFGC_RDPROC_PATH( DFGC_INIT | 0x0006 );     /*<2040002>       */
                MFGC_RDPROC_ERROR( DFGC_INIT | 0x0006 );    /*<2040002>       */
                ret = -EIO;                       /*<2040002>                 */
            }                                     /*<2040002>                 */
#endif /* SYS_HAVE_IBP */                         /*<2040002>                 */

#if DFGC_SW_TEST_DEBUG
            entry = create_proc_read_entry(
                        "driver/fgc_sdram",
                        0,
                        NULL,
                        pfgc_log_rdproc_sdram,
                        NULL );
            if( entry == NULL )
            {
                MFGC_RDPROC_PATH( DFGC_INIT | 0x0007 );
                MFGC_RDPROC_ERROR( DFGC_INIT | 0x0007 );
                ret = -EIO;
            }
            entry = create_proc_read_entry(
                        "driver/fgc_nand1",
                        0,
                        NULL,
                        pfgc_log_rdproc_nand1,
                        NULL );
            if( entry == NULL )
            {
                MFGC_RDPROC_PATH( DFGC_INIT | 0x0008 );
                MFGC_RDPROC_ERROR( DFGC_INIT | 0x0008 );
                ret = -EIO;
            }
            entry = create_proc_read_entry(
                        "driver/fgc_nand2",
                        0,
                        NULL,
                        pfgc_log_rdproc_nand2,
                        NULL );
            if( entry == NULL )
            {
                MFGC_RDPROC_PATH( DFGC_INIT | 0x0009 );
                MFGC_RDPROC_ERROR( DFGC_INIT | 0x0009 );
                ret = -EIO;
            }
#endif /* DFGC_SW_TEST_DEBUG */
#endif /* CONFIG_PROC_FS_FGC */                   /*         DD proc          */

            init_completion(&gfgc_comp);
            sema_init( &gfgc_sem_update, 1 );     /*<1905359>                 */
            init_waitqueue_head( &gfgc_task_wq ); /*<1905359>                 */

            atomic_set(&gfgc_susres_flag, 1);       /* npdc300035452 */
            init_waitqueue_head( &gfgc_susres_wq ); /* npdc300035452 */

            fgc_kthread_id = kthread_create( pfgc_fgc_kthread, NULL, "fgc_kthread" );
            if( IS_ERR(fgc_kthread_id) )
            {
                FGC_ERR( "##### fgc_thread ERROR!! #####\n" );
                pfgc_unregister_chrdev( DFGC_MAJOR, DFGC_DEV );
                return -EIO;
            }
            wake_up_process( fgc_kthread_id );

            mode = gfgc_fgic_mode;
            if( mode == DCOM_ACCESS_MODE1 )   /*                          */
            {
                    MFGC_RDPROC_PATH( DFGC_INIT | 0x000A );
                    gfgc_ctl.update_enable = DFGC_ON;
                    wakeup = pfgc_info_update( D_FGC_FG_INIT_IND );/*  C               */
            }

            ret = pfgc_request_irq( &pfgc_fgc_int );
            if( ret < 0 )
            {
                FGC_ERR( "pfgc_request_irq() failed. ret = %d\n", ret );
                spin_lock_irqsave( &gfgc_spinlock_if, flags );
                gfgc_kthread_flag |= D_FGC_EXIT_IND;
                spin_unlock_irqrestore( &gfgc_spinlock_if, flags );
                wake_up( &gfgc_task_wq );                             /*   V  */
                pfgc_unregister_chrdev( DFGC_MAJOR, DFGC_DEV );
                return ret;
            }
            ret = pfgc_enable_irq_wake();
            if(ret < 0)
            {
                FGC_ERR( "pfgc_enable_irq_wake() failed. ret = %d\n", ret );
                spin_lock_irqsave( &gfgc_spinlock_if, flags );
                gfgc_kthread_flag |= D_FGC_EXIT_IND;
                spin_unlock_irqrestore( &gfgc_spinlock_if, flags );
                wake_up( &gfgc_task_wq );
                pfgc_unregister_chrdev( DFGC_MAJOR, DFGC_DEV );
            }
        }
    }
    else                                          /*         DD               */
    {
        MFGC_RDPROC_PATH( DFGC_INIT | 0x0005 );
        MFGC_RDPROC_ERROR( DFGC_INIT | 0x0003 );
        ret = -EIO;                               /*                          */
    }

    MFGC_RDPROC_PATH( DFGC_INIT | 0xFFFF );

    DEBUG_FGIC(("pfgc_init end\n"));
    return ret;                                   /* return                   */
}

/******************************************************************************/
/*                pfgc_open                                                   */
/*                        DD open                                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                0                                                           */
/*                -EFAULT            file        NULL                         */
/*                -EINVAL            inode        NULL                        */
/*                -EPERM                                                      */
/*                -EIO               MAJOR                                    */
/*                struct inode *inode   inode                                 */
/*                struct file  *file                                          */
/*                                                                            */
/******************************************************************************/
int pfgc_open( struct inode *inode, struct file *file )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    int ret;                                      /*                          */
#if DFGC_SW_DELETED_FUNC
    int mode;                                     /* FG-IC                    */
    unsigned char pint_ret;                       /*<3100736> pint            */
#endif /* DFGC_SW_DELETED_FUNC */

    MFGC_RDPROC_PATH( DFGC_OPEN | 0x0000 );

    ret = DFGC_OK;                                /*<3100736>                 */

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( file == NULL )                            /* file      NULL           */
    {
        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_OPEN | 0x0001 );
        ret = -EFAULT;                            /*                          */
    }
    else if( inode == NULL )                      /* inode      NULL          */
    {
        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0002 );
        MFGC_RDPROC_ERROR( DFGC_OPEN | 0x0002 );
        ret = -EINVAL;                            /*                          */
    }
    else if((((file->f_flags & ~O_LARGEFILE) != O_RDWR   ) &&
             ((file->f_flags & ~O_LARGEFILE) != ( O_RDWR   | O_NONBLOCK ))) &&
            (((file->f_flags & ~O_LARGEFILE) != O_RDONLY ) &&
             ((file->f_flags & ~O_LARGEFILE) != ( O_RDONLY | O_NONBLOCK ))) &&
            (((file->f_flags & ~O_LARGEFILE) != O_WRONLY ) &&
             ((file->f_flags & ~O_LARGEFILE) != ( O_WRONLY | O_NONBLOCK ))))
    {                                             /* open                     */
        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0003 );
        MFGC_RDPROC_ERROR( DFGC_OPEN | 0x0003 );
        ret = -EPERM;                             /*                          */
    }
/*<QCIO017>    else if( MAJOR( file->f_dentry->d_inode->i_rdev ) != DFGC_MAJOR ) *//*  C   */
/*<QCIO017>    {                                             *//* MAJOR                    */
/*<QCIO017>        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0004 );   */
/*<QCIO017>        MFGC_RDPROC_ERROR( DFGC_OPEN | 0x0004 );  */
/*<QCIO017>        ret = -ENODEV;                            *//*                          */
/*<QCIO017>    }                                             */
/*<QCIO017>    else                                          *//*                          */
    else if( MAJOR( file->f_dentry->d_inode->i_rdev ) == DFGC_MAJOR )/*<QCIO017>*/
    {
        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0005 );

        /*--------------------------------------------------------------------*/
        /*     MINOR    open                                                  */
        /*--------------------------------------------------------------------*/
        if( MINOR( file->f_dentry->d_inode->i_rdev ) < DFGC_MINOR_NUM_MAX ) /*  C*/
        {
            MFGC_RDPROC_PATH( DFGC_OPEN | 0x0006 );

            gfgc_open_cnt[ MINOR( file->f_dentry->d_inode->i_rdev ) ]++;/*  C */
                                                  /* open         ++          */

                                                  /* /dev/fgc/info            */
            if(( MINOR( file->f_dentry->d_inode->i_rdev ))
                                   == DFGC_INFO_MINOR )
            {
                MFGC_RDPROC_PATH( DFGC_OPEN | 0x0007 );  /*<PCIO034>          */
#if DFGC_SW_DELETED_FUNC
/*<3130560>  mode = pfgc_fgic_modecheck();      *//*  C FG-IC                 */
                mode = gfgc_fgic_mode;            /*<3130560>                 */
                if( mode == DCOM_ACCESS_MODE1 )   /*                          */
                {
                    MFGC_RDPROC_PATH( DFGC_OPEN | 0x0008 );  /*<PCIO034>      */
                                                  /* FGC-DD                   */
                    gfgc_ctl.remap_ocv->ocv_ctl_flag
                        &= ( ~DFGC_STATE_BIT );

                    pfgc_info_update( D_FGC_FG_INIT_IND );/*  C               */

/*<3100736>                    ret = pint_regist( DINT_FG_INT,*//*  C FG-IC                */
                    pint_ret = pint_regist( DINT_FG_INT,/*<3100736>  C FG-IC           */
                                       &pfgc_fgc_int,
                                       DINT_MASK_CLEAR );
/*<3100736>                    if( ret != DINT_OK )          *//*                          */
                    if( pint_ret != DINT_OK )     /*<3100736>                 */
                    {
                        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0009 );  /*<PCIO034>  */
                        MFGC_RDPROC_ERROR( DFGC_OPEN | 0x0009 ); /*<PCIO034>  */
                        ret = -ENODEV;            /*                          */
                    }
                                                  /*                          */
                                                  /* OCV                      */
                    if( gfgc_ctl.remap_ocv->fgc_ocv_data[ 0 ].ocv
                                           != DFGC_OFF )
                    {
                        MFGC_RDPROC_PATH( DFGC_OPEN | 0x000A );  /*<PCIO034>  */
                        pfgc_log_ocv();          /*  V                        */
                    }
                }
#endif /* DFGC_SW_DELETED_FUNC */
            }

        }

/*<3100736>        ret = DFGC_OK;                            *//*                          */
    }
#ifdef    SYS_HAVE_IBP                                           /*<QCIO017>  */
    /* IBP                                                     *//*<QCIO017>  */
    else if( MAJOR( file->f_dentry->d_inode->i_rdev )            /*<QCIO017>  */
             == DFGC_IBP_MAJOR )                                 /*<QCIO017>  */
    {                                                            /*<QCIO017>  */
        MFGC_RDPROC_PATH( DFGC_OPEN | 0x000B );                  /*<QCIO017>  */
                                                                 /*<QCIO017>  */
        if( MINOR( file->f_dentry->d_inode->i_rdev )             /*<QCIO017>  */
            < DFGC_IBP_MINOR_MAX )                               /*<QCIO017>  */
        {                                                        /*<QCIO017>  */
            MFGC_RDPROC_PATH( DFGC_OPEN | 0x000C );              /*<QCIO017>  */
                                                                 /*<QCIO017>  */
            /*                                                 *//*<QCIO017>  */
            gfgc_ibp_open_cnt                                    /*<QCIO017>  */
                [ MINOR( file->f_dentry->d_inode->i_rdev ) ]++;  /*<QCIO017>  */
        }                                                        /*<QCIO017>  */
        else                                                     /*<QCIO017>  */
        {                                                        /*<QCIO017>  */
            /*                                                 *//*<QCIO017>  */
            MFGC_RDPROC_PATH( DFGC_OPEN | 0x000D );              /*<QCIO017>  */
            MFGC_RDPROC_ERROR( DFGC_OPEN | 0x000D );             /*<QCIO017>  */
            ret = -ENODEV;                                       /*<QCIO017>  */
        }                                                        /*<QCIO017>  */
    }                                                            /*<QCIO017>  */
#endif /* SYS_HAVE_IBP */                                        /*<QCIO017>  */
    else                                                         /*<QCIO017>  */
    {                                                            /*<QCIO017>  */
        MFGC_RDPROC_PATH( DFGC_OPEN | 0x0004 );                  /*<QCIO017>  */
        MFGC_RDPROC_ERROR( DFGC_OPEN | 0x0004 );                 /*<QCIO017>  */
        ret = -ENODEV;                                           /*<QCIO017>  */
    }                                                            /*<QCIO017>  */

    MFGC_RDPROC_PATH( DFGC_OPEN | 0xFFFF );

    return ret;                                   /* return                   */
}

/******************************************************************************/
/*                pfgc_ioctl                                                  */
/*                        DD ioctl                                            */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                0                                                           */
/*                -EFAULT                                                     */
/*                -ENODEV            MAJOR                                    */
/*                struct inode  *inode       inode                            */
/*                struct file   *file                                         */
/*                unsigned int  cmd                  ID                       */
/*                unsigned long arg                            (        )     */
/*                                                                            */
/******************************************************************************/
long pfgc_ioctl( struct file *file,
                unsigned int cmd,
                unsigned long arg   )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
//IDPower    int ret;                                      /*                          */
    long ret;                                      /*                          *//* IDPower */
    T_FGC_BATT_INFO batt_info;                    /*                          */
    T_FGC_RW_DF     df_data;                      /* DF                       */
    unsigned char   df_rw_data;                   /*                          */
    T_FGC_DF_STUDY  df_study_data;                /* DF                       */
    unsigned long   flags;

    MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0000 );

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( file == NULL )                            /*             NULL         */
    {
        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0001 );
        return -EFAULT;                           /* return                   */
    }

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    ret = DFGC_OK;                                /*                          */

    /*------------------------------------------------------------------------*/
    /* MAJOR                                                                  */
    /*------------------------------------------------------------------------*/
    if( MAJOR( file->f_dentry->d_inode->i_rdev ) == DFGC_MAJOR ) /*  C        */
    {                                             /* MAJOR              DD    */
        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0002 );

        /*--------------------------------------------------------------------*/
        /* MINOR                                                              */
        /*--------------------------------------------------------------------*/
        switch( MINOR( file->f_dentry->d_inode->i_rdev )) /*  C               */
        {
            /*----------------------------------------------------------------*/
            /*                                                                */
            /*----------------------------------------------------------------*/
            case DFGC_INFO_MINOR:                 /* /dev/fg/info             */

                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0003 );

                switch( cmd )                     /*                 ID       */
                {
                    case FGIOCFGINFO:             /*                          */

                        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0004 );

                        spin_lock_irqsave( &gfgc_spinlock_if, flags );/*  V         */
                        memcpy( &batt_info,       /*  V   S                   */
                                &gfgc_batt_info,
                                sizeof( batt_info ));
                        spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); /*  V          */

                        /*                           *//*<1210001>            */
                        if(( gfgc_ecurrent.detection   /*<1210001>            */
                             & DFGC_BITMASK_7 )        /*<1210001>            */
                           == DFGC_BITMASK_7 )         /*<1210001>            */
                        {                              /*<1210001>            */
                            MFGC_RDPROC_PATH(          /*<1210001>            */
                                DFGC_IOCTL | 0x001B ); /*<1210001>            */
                            batt_info.capacity         /*<1210001>100%        */
                                = DFGC_CAPACITY_MAX;   /*<1210001>            */
                            batt_info.health           /*<1210001>            */
                                = DFGC_HEALTH_GOOD;    /*<1210001>            */
                        }                              /*<1210001>            */

                        if( copy_to_user(( T_FGC_BATT_INFO * )arg, /*  C      */
                                         &batt_info,
                                         sizeof( T_FGC_BATT_INFO )) != 0 )
                        {                         /*                          */
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0005 );
                            MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0002 );
                            ret = -EFAULT;        /*                          */
                        }

                        break;                    /* break                    */

                    default:                      /*         ID               */
                        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0006 );
                        MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0003 );
                        ret = -ENOTTY;            /*                          */
                        break;                    /* break                    */
                }

                break;                            /* break                    */

            case DFGC_DF_MINOR:                   /* /dev/fg/df DF            */

                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0007 );

                switch( cmd )                     /* DF            ID         */
                {
                    case FGIOCDFGET:              /* DF                       */

                        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0008 );

                        if( copy_from_user( &df_data, /*  C                   */
                                 ( T_FGC_RW_DF * )arg,
                                 sizeof( df_data )) != 0 )
                        {                         /*                          */
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0009 );
                            MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0004 );
                            ret = -EFAULT;        /* return                   */
                        }
                        else
                        {                         /*                          */
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x000A );

                            ret = pfgc_df_get( &df_data, &df_rw_data );/*  C  */
                                                  /* DF                       */
                            if( ret != DFGC_OK )  /*                          */
                            {
                                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x000B );
                                MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0005 );
                                ret = -EFAULT;    /* return                   */
                            }
                            else
                            {
                                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x000C );

                                if( copy_to_user( df_data.data, /*  C           */
                                             &df_rw_data,
                                             sizeof( df_rw_data )) != 0 )
                                {
                                    MFGC_RDPROC_PATH( DFGC_IOCTL | 0x000D );
                                    MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0006 );
                                    ret = -EFAULT;/* return                   */
                                }
                            }
                        }

                        break;                    /* break                    */

                    case FGIOCDFSET:              /* DF                       */

                        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x000E );

                        if( copy_from_user( &df_data, /*  C                   */
                                 ( T_FGC_RW_DF * )arg,
                                 sizeof( df_data )) != 0 )
                        {
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x000F );
                            MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0007 );
                            ret = -EFAULT;        /* return                   */
                        }
                        else
                        {
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0010 );

                            if( copy_from_user( &df_rw_data, /*  C     data      */
                                         df_data.data,
                                         sizeof( df_rw_data )) != 0 )
                            {                     /*                          */
                                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0011 );
                                MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0008 );
                                ret = -EFAULT;    /* return                   */
                            }
                            else
                            {                     /*                          */
                                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0012 );

                                ret = pfgc_df_set( &df_data, &df_rw_data );/*  C*/
                                                  /* DF                       */
                                if( ret != DFGC_OK ) /*                       */
                                {
                                    MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0013 );
                                    MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x0009 );
                                    ret = -EFAULT;/* return                   */
                                }
                            }
                        }


                        break;                    /* break                    */

                    case FGIOCDFSTUDYGET:         /* DF                       */

                        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0014 );

                        ret = pfgc_df_study_check( &df_study_data );/*  C     */
                                                  /*                          */
                        if( ret == DFGC_OK )      /*                          */
                        {
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0015 );

                            if( copy_to_user( ( T_FGC_DF_STUDY * )arg,/*  C   */
                                         &df_study_data,
                                         sizeof( T_FGC_DF_STUDY )) != 0 )
                            {
                                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0016 );
                                MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x000A );
                                ret = -EFAULT;    /* return                   */
                            }
                        }
                        else                      /*                          */
                        {
                            MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0017 );
                            MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x000B );
                            ret = -EFAULT;        /* return                   */
                        }

                        break;                    /* break                    */

                    default:                      /*         ID               */
                        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0018 );
                        MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x000C );
                        ret = -ENOTTY;            /*                          */
                        break;                    /* break                    */
                }

                break;                            /* break                    */

            /*----------------------------------------------------------------*/
            /* MINOR                                                          */
            /*----------------------------------------------------------------*/
            default:                              /* MINOR                    */
                MFGC_RDPROC_PATH( DFGC_IOCTL | 0x0019 );
                MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x000D );
                ret = -ENODEV;                    /*                          */
                break;                            /* break                    */
        }
    }
    else                                          /* MAJOR                    */
    {
        MFGC_RDPROC_PATH( DFGC_IOCTL | 0x001A );
        MFGC_RDPROC_ERROR( DFGC_IOCTL | 0x000E );
        ret = -ENODEV;                            /*                          */
    }

    MFGC_RDPROC_PATH( DFGC_IOCTL | 0xFFFF );

    return ret;                                   /* return                   */
}

/******************************************************************************/
/*                pfgc_read                                                   */
/*                        DD read                                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                retval                                                      */
/*                -EFAULT                                                     */
/*                -ENODEV            MAJOR                                    */
/*                -EAGAIN                                                     */
/*                -ERESTARTSYS                                                */
/*                struct file *file                                           */
/*                size_t count                                                */
/*                loff_t *ppos                                                */
/*                                                                            */
/*                char   *buf                                                 */
/******************************************************************************/
int pfgc_read( struct file *file,
                char *buf,
                size_t count,
                loff_t *ppos )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    int             retval;                       /*                          */
    T_FGC_BATT_INFO batt_info;                    /*                          */
    T_FGC_FW_VER    fw_ver_data;                  /*                          */
    unsigned short  dfs_data;                     /* DF                       */
    unsigned long   flags;

    MFGC_RDPROC_PATH( DFGC_READ | 0x0000 );

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
/*<QCIO017>    if( file == NULL )               *//*             NULL         */
    if(( file == NULL ) || ( buf == NULL ))       /*<QCIO017>     NULL        */
    {
        MFGC_RDPROC_PATH( DFGC_READ | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_READ | 0x0001 );
        return -EFAULT;                           /* return                   */
    }

    retval = DFGC_OK;                             /*             table        */

    /*------------------------------------------------------------------------*/
    /* MAJOR                                                                  */
    /*------------------------------------------------------------------------*/
    if( MAJOR( file->f_dentry->d_inode->i_rdev ) == DFGC_MAJOR ) /*  C        */
    {                                             /* MAJOR              DD    */

        MFGC_RDPROC_PATH( DFGC_READ | 0x0002 );

        /*--------------------------------------------------------------------*/
        /* MINOR                                                              */
        /*--------------------------------------------------------------------*/
        switch( MINOR( file->f_dentry->d_inode->i_rdev )) /*  C               */
        {
            case DFGC_INFO_MINOR:                 /* /dev/fginfo              */

                MFGC_RDPROC_PATH( DFGC_READ | 0x0003 );

                /*------------------------------------------------------------*/
                /*                                                            */
                /*------------------------------------------------------------*/
                if( gfgc_ctl.flag == DFGC_OFF )
                {                                 /*                          */
                    MFGC_RDPROC_PATH( DFGC_READ | 0x0004 );

                    if(( file->f_flags & O_NONBLOCK ) == O_NONBLOCK )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_READ | 0x0005 );
                        MFGC_RDPROC_ERROR( DFGC_READ | 0x0005 );
                        retval = -EAGAIN;         /*                          */
                    }
                    else                          /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_READ | 0x0006 );

                        /*----------------------------------------------------*/
                        /*                                                    */
                        /*----------------------------------------------------*/
                        wait_event_interruptible( gfgc_fginfo_wait,      /*  V*/
                                                  ( gfgc_ctl.flag !=
                                                    DFGC_OFF ));

                        if( signal_pending( current ))          /*  C         */
                        {                         /*                          */
                            MFGC_RDPROC_PATH( DFGC_READ | 0x0007 );
                            MFGC_RDPROC_ERROR( DFGC_READ | 0x0007 );
                            retval = -ERESTARTSYS;
                                                  /*                          */
                        }
                    }
                }

                if( retval == DFGC_OK )           /*                          */
                {
                    MFGC_RDPROC_PATH( DFGC_READ | 0x0008 );

                    spin_lock_irqsave( &gfgc_spinlock_if, flags ); /*  V         */

                    memcpy( &batt_info, &gfgc_batt_info,        /*  S         */
                                        sizeof( batt_info ));

                    spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); /*  V          */

                    /*                          *//*<1210001>                 */
                    if(( gfgc_ecurrent.detection  /*<1210001>                 */
                         & DFGC_BITMASK_7 )       /*<1210001>                 */
                       == DFGC_BITMASK_7 )        /*<1210001>                 */
                    {                             /*<1210001>                 */
                        MFGC_RDPROC_PATH(         /*<1210001>                 */
                            DFGC_READ | 0x001A ); /*<1210001>                 */
                        batt_info.capacity        /*<1210001> 100%            */
                            = DFGC_CAPACITY_MAX;  /*<1210001>                 */
                        batt_info.health          /*<1210001>                 */
                            = DFGC_HEALTH_GOOD;   /*<1210001>                 */
                    }                             /*<1210001>                 */

                    if( copy_to_user( buf,               /*  C                */
                                     &batt_info,
                                     sizeof( T_FGC_BATT_INFO  )) == DFGC_OK )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_READ | 0x0009 );

                        retval = sizeof( T_FGC_BATT_INFO );
                                                  /*                          */
                        spin_lock_irqsave( &gfgc_spinlock_if, flags ); /*  V         */
                        gfgc_ctl.capacity_old = gfgc_batt_info.capacity;
                        gfgc_ctl.health_old   = gfgc_batt_info.health;
                        if( gfgc_int_flag == 0 )  /*<3210076>                 */
                        {                         /*<3210076>                 */
                            MFGC_RDPROC_PATH( DFGC_READ | 0x0014 );/*<3210076>*/
                            gfgc_ctl.flag = DFGC_OFF;
                                                  /*     level    flg         */
                            spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); /*  V          *//* SlimID Add */
                        }                         /*<3210076>                 */
                        else                      /*<3210076>                 */
                        {                         /*<3210076>                 */
                            MFGC_RDPROC_PATH( DFGC_READ | 0x0015 );/*<3210076>*/
                            /* FGIC                              *//*<3210076>*/
                            gfgc_ctl.flag = D_FGC_FG_INT_IND;      /*<3210076>*/
                            /*                                   *//*<1905359>*/
                            gfgc_kthread_flag |= D_FGC_FG_INT_IND; /*<1905359>*/
                            spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); /*  V          *//* SlimID Add */
                            wake_up( &gfgc_task_wq );       /*  V*//*<1905359>*/
                        }                         /*<3210076>                 */
/* SlimID Del                        spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); *//*  V          */
                    }
                    else                          /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_READ | 0x000A );
                        MFGC_RDPROC_ERROR( DFGC_READ | 0x000A );
                        retval = -EFAULT;         /*                          */
                    }
                }

                break;                            /* break                    */

            case DFGC_VER_MINOR:                  /* /dev/fg/ver Version      */

                MFGC_RDPROC_PATH( DFGC_READ | 0x000B );

                retval = pfgc_ver_read( &fw_ver_data ); /*  C Version         */
                if( retval != DFGC_OK )           /*                          */
                {
                    MFGC_RDPROC_PATH( DFGC_READ | 0x000C );
                    MFGC_RDPROC_ERROR( DFGC_READ | 0x000C );
                    retval = -EFAULT;             /*                          */
                }
                else
                {
                    if( copy_to_user( buf,        /*  C                       */
                                 &fw_ver_data,
                                 sizeof( T_FGC_FW_VER )) == DFGC_OK )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_READ | 0x000D );

                        retval = sizeof( T_FGC_FW_VER ); /*        copysize    */
                    }
                    else                          /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_READ | 0x000E );
                        MFGC_RDPROC_ERROR( DFGC_READ | 0x000E );
                        retval = -EFAULT;         /*                          */
                    }
                }

                break;                            /* break                    */

            case DFGC_DFS_MINOR:                  /* /dev/fg/dfs DF           */

                MFGC_RDPROC_PATH( DFGC_READ | 0x000F );

                retval = pfgc_dfs_read( &dfs_data );/*  C DF                  */
                if( retval != DFGC_OK )           /*                          */
                {
                    MFGC_RDPROC_PATH( DFGC_READ | 0x0010 );
                    MFGC_RDPROC_ERROR( DFGC_READ | 0x0010 );
                    retval = -EFAULT;             /*                          */
                }
                else
                {
                    if( copy_to_user( buf,        /*  C                       */
                                 &dfs_data,
                                 sizeof( unsigned short )) == DFGC_OK )
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_READ | 0x0011 );

                        retval = sizeof( unsigned short );/*         copysize    */
                    }
                    else                          /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_READ | 0x0011 );
                        MFGC_RDPROC_ERROR( DFGC_READ | 0x0011 );
                        retval = -EFAULT;         /*                          */
                    }
                }

                break;                            /* break                    */

            /*----------------------------------------------------------------*/
            /* MINOR                                                          */
            /*----------------------------------------------------------------*/
            default:                              /* MINOR                    */
                MFGC_RDPROC_PATH( DFGC_READ | 0x0012 );
                MFGC_RDPROC_ERROR( DFGC_READ | 0x0012 );
                retval = -ENODEV;                 /*                          */
                break;                            /* break                    */
        }
    }
#ifdef    SYS_HAVE_IBP                                      /*<QCIO017>       */
    /* IBP                                                *//*<QCIO017>       */
    else if( MAJOR( file->f_dentry->d_inode->i_rdev )       /*<QCIO017>       */
             == DFGC_IBP_MAJOR )                            /*<QCIO017>       */
    {                                                       /*<QCIO017>       */
        MFGC_RDPROC_PATH( DFGC_READ | 0x0016 );             /*<QCIO017>       */

        /*                                                *//*<1210001>       */
        if(( gfgc_ecurrent.detection & DFGC_BITMASK_7 )     /*<1210001>       */
           == DFGC_BITMASK_7 )                              /*<1210001>       */
        {                                                   /*<1210001>       */
            MFGC_RDPROC_PATH(                               /*<1210001>       */
                DFGC_READ | 0x001B );                       /*<1210001>       */
            return count;                                   /*<1210001>return */
        }                                                   /*<1210001>       */
                                                            /*<QCIO017>       */
        switch( MINOR( file->f_dentry->d_inode->i_rdev ))   /*<QCIO017>       */
        {                                                   /*<QCIO017>       */
            /* /dev/ibp/check1                            *//*<QCIO017>       */
            case DFGC_IBP_MINOR_NORMAL:                     /*<QCIO017>       */
                MFGC_RDPROC_PATH( DFGC_READ | 0x0017 );     /*<QCIO017>       */
                retval = pfgc_ibp_check_read(               /*<QCIO017>  C    */
                             buf,                           /*<QCIO017>       */
                             count,                         /*<QCIO017>       */
                             DFGC_IBP_MINOR_NORMAL );       /*<QCIO017>       */
                break;                                      /*<QCIO017>       */
                                                            /*<QCIO017>       */
            /* /dev/ibp/check2                            *//*<QCIO017>       */
            case DFGC_IBP_MINOR_TEST:                       /*<QCIO017>       */
                MFGC_RDPROC_PATH( DFGC_READ | 0x0018 );     /*<QCIO017>       */
                retval = pfgc_ibp_check_read(               /*<QCIO017>  C    */
                             buf,                           /*<QCIO017>       */
                             count,                         /*<QCIO017>       */
                             DFGC_IBP_MINOR_TEST );         /*<QCIO017>       */
                break;                                      /*<QCIO017>       */
                                                            /*<QCIO017>       */
            default:                                        /*<QCIO017>       */
                MFGC_RDPROC_PATH( DFGC_READ | 0x0019 );     /*<QCIO017>       */
                MFGC_RDPROC_ERROR( DFGC_READ | 0x0019 );    /*<QCIO017>       */
                retval = -ENODEV;                           /*<QCIO017>       */
                break;                                      /*<QCIO017>       */
        }                                                   /*<QCIO017>       */
    }                                                       /*<QCIO017>       */
#endif /* SYS_HAVE_IBP */                                   /*<QCIO017>       */
    else
    {
        MFGC_RDPROC_PATH( DFGC_READ | 0x0013 );
        MFGC_RDPROC_ERROR( DFGC_READ | 0x0013 );
        retval = -ENODEV;                         /*                           */
    }

    MFGC_RDPROC_PATH( DFGC_READ | 0xFFFF );

    return retval;                                /* return                    */
}


/******************************************************************************/
/*                pfgc_poll                                                   */
/*                        DD poll                                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                mask                                                        */
/*                -ENODEV                    MAJOR                            */
/*                struct file *file                                           */
/*                poll_table  *wait          polltable                        */
/*                                                                            */
/******************************************************************************/
unsigned int pfgc_poll( struct file *file,
                         poll_table *wait )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    unsigned int  mask;                           /*                          */
    unsigned long wakeup;                         /*                          */
    unsigned long   flags;                        /*<3040001>                 */

    MFGC_RDPROC_PATH( DFGC_POLL | 0x0000 );

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( file == NULL )                            /*             NULL         */
    {
        MFGC_RDPROC_PATH( DFGC_POLL | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_POLL | 0x0001 );
        return ( unsigned int )( -EFAULT );       /* return                   */
    }

    mask = DFGC_CLEAR;                            /*                          */

    /*------------------------------------------------------------------------*/
    /* MAJOR                                                                  */
    /*------------------------------------------------------------------------*/
    if( MAJOR( file->f_dentry->d_inode->i_rdev ) == DFGC_MAJOR ) /*  C        */
    {                                             /* MAJOR                    */
        MFGC_RDPROC_PATH( DFGC_POLL | 0x0002 );

        /*--------------------------------------------------------------------*/
        /* MINOR                                                              */
        /*--------------------------------------------------------------------*/
        switch( MINOR( file->f_dentry->d_inode->i_rdev )) /*  C               */
        {
            case DFGC_INFO_MINOR:                 /* /dev/fg/info             */

                MFGC_RDPROC_PATH( DFGC_POLL | 0x0003 );

                poll_wait( file, &gfgc_fginfo_wait, wait ); /*  N             */
                                                  /* wait                     */
                if(( gfgc_ctl.flag & D_FGC_FG_INT_IND ) == D_FGC_FG_INT_IND )
                {
                    MFGC_RDPROC_PATH( DFGC_POLL | 0x0004 );
/*<1905359>                    wakeup = pfgc_info_update( D_FGC_FG_INT_IND ); *//*  C      */
                    wakeup = DFGC_ON;                              /*<1905359>*/
                }
                else if( gfgc_ctl.flag != DFGC_OFF ) /*                       */
                {
                    MFGC_RDPROC_PATH( DFGC_POLL | 0x0005 );
/*<3040001>                    wakeup = DFGC_OFF;*/
                    wakeup = DFGC_ON;             /*<3040001>                 */
                }
                else
                {
                    MFGC_RDPROC_PATH( DFGC_POLL | 0x0006 );
                    wakeup = DFGC_OFF;
                }

                if( wakeup == DFGC_ON )
                {                                 /* wait                     */
                    MFGC_RDPROC_PATH( DFGC_POLL | 0x0007 );
                    mask |= ( POLLIN | POLLRDNORM ); /* bit Mask              */
                }
                else                              /*                          */
                {
                    MFGC_RDPROC_PATH( DFGC_POLL | 0x0008 );
                    spin_lock_irqsave( &gfgc_spinlock_if, flags ); /*<3040001>  V         */
                    if( gfgc_int_flag == 0 )      /*<3210076>                 */
                    {                             /*<3210076>                 */
                        MFGC_RDPROC_PATH( DFGC_POLL | 0x000B );    /*<3210076>*/
                                                  /*     level    flg         */
                    gfgc_ctl.flag                 /*                          */
                        &= ( ~( unsigned long )D_FGC_FG_INT_IND );
                    }                             /*<3210076>                 */
                    spin_unlock_irqrestore( &gfgc_spinlock_if, flags ); /*<3040001>  V          */
                }

                break;                            /* break                    */

            default:                              /*                          */

                MFGC_RDPROC_PATH( DFGC_POLL | 0x0009 );

                break;
        }
    }
    else                                          /* MAJOR              DD    */
    {
        MFGC_RDPROC_PATH( DFGC_POLL | 0x000A );
        MFGC_RDPROC_ERROR( DFGC_POLL | 0x0002 );
        mask = ( unsigned int )ENODEV;            /*                          */
    }

    MFGC_RDPROC_PATH( DFGC_POLL | 0xFFFF );

    return mask;                                  /* return                    */
}

/******************************************************************************/
/*                pfgc_write                                                  */
/*                        DD write                                            */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                retval                                                      */
/*                -EFAULT                                                     */
/*                -ENODEV            MAJOR                                    */
/*                struct file *file                                           */
/*                const char   *buf                                           */
/*                size_t count                                                */
/*                loff_t *ppos                                                */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
int pfgc_write( struct file *file,
                const char *buf,
                size_t count,
                loff_t *ppos )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    int             retval;                       /*                          */
    T_FGC_FW        fw_data;                      /* FW                       */
    unsigned long   dfs_data;                     /* DF                       */

    MFGC_RDPROC_PATH( DFGC_WRITE | 0x0000 );

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( file == NULL )                            /*             NULL         */
    {
        MFGC_RDPROC_PATH( DFGC_WRITE | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0001 );
        return -EFAULT;                           /* return                   */
    }

    retval = DFGC_OK;                             /*             table        */

    /*------------------------------------------------------------------------*/
    /* MAJOR                                                                  */
    /*------------------------------------------------------------------------*/
    if( MAJOR( file->f_dentry->d_inode->i_rdev ) == DFGC_MAJOR ) /*  C        */
    {                                             /* MAJOR              DD    */

        MFGC_RDPROC_PATH( DFGC_WRITE | 0x0002 );

        /*--------------------------------------------------------------------*/
        /* MINOR                                                              */
        /*--------------------------------------------------------------------*/
        switch( MINOR( file->f_dentry->d_inode->i_rdev )) /*  C               */
        {
            case DFGC_FW_MINOR:                   /* /dev/fg/fw FW            */

                MFGC_RDPROC_PATH( DFGC_WRITE | 0x0003 );

                /*------------------------------------------------------------*/
                /* FW                                                         */
                /*------------------------------------------------------------*/
                if( copy_from_user( &fw_data,     /*  C                       */
                         ( T_FGC_FW * )buf,
                         sizeof( fw_data )) != 0 )
                {                                 /*                          */
                    MFGC_RDPROC_PATH( DFGC_WRITE | 0x0004 );
                    MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0002 );
                    retval = -EFAULT;             /* return                   */
                }
                else
                {                                 /*                          */

                    MFGC_RDPROC_PATH( DFGC_WRITE | 0x0005 );

                    retval = pfgc_fw_write( &fw_data );/*  C                  */
                                                  /* FW                       */
                    if( retval == DFGC_OK )       /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_WRITE | 0x0006 );
                        retval = count;
                    }
                    else                          /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_WRITE | 0x0007 );
                        MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0003 );
                        retval = -EFAULT;         /* return                   */
                    }
                }

                break;                            /* break                    */

            case DFGC_DFS_MINOR:                  /* /dev/fg/dfs DF           */

                MFGC_RDPROC_PATH( DFGC_WRITE | 0x0008 );

                /*------------------------------------------------------------*/
                /* DF                                                         */
                /*------------------------------------------------------------*/
                if( copy_from_user( &dfs_data,    /*  C                       */
                         ( unsigned long * )buf,
                         sizeof( dfs_data )) != 0 )
                {                                 /*                          */
                    MFGC_RDPROC_PATH( DFGC_WRITE | 0x0009 );
                    MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0004 );
                    retval = -EFAULT;             /* return                   */
                }
                else
                {                                 /*                          */

                    MFGC_RDPROC_PATH( DFGC_WRITE | 0x000A );

                    retval = pfgc_dfs_write( &dfs_data );/*  C                */
                                                  /* DF                       */
                    if( retval == DFGC_OK )       /*                          */
                    {
                        MFGC_RDPROC_PATH( DFGC_WRITE | 0x000B );
                        retval = count;
                    }
                    else
                    {                             /*                          */
                        MFGC_RDPROC_PATH( DFGC_WRITE | 0x000C );
                        MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0005 );
                        retval = -EFAULT;         /* return                   */
                    }
                }

                break;                            /* break                    */

            /*----------------------------------------------------------------*/
            /* MINOR                                                          */
            /*----------------------------------------------------------------*/
            default:                              /* MINOR                    */
                MFGC_RDPROC_PATH( DFGC_WRITE | 0x000D );
                MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0006 );
                retval = -ENODEV;                 /*                          */
                break;                            /* break                    */
        }
    }
    else
    {                                             /* MAJOR                    */
        MFGC_RDPROC_PATH( DFGC_WRITE | 0x000E );
        MFGC_RDPROC_ERROR( DFGC_WRITE | 0x0007 );
        retval = -ENODEV;                         /*                          */
    }

    MFGC_RDPROC_PATH( DFGC_WRITE | 0xFFFF );

    return retval;                                /* return                    */
}

/******************************************************************************/
/*                pfgc_release                                                */
/*                        DD release                                          */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                0                                                           */
/*                -EINVAL                    inode        NULL                */
/*                -ENODEV                    MAJOR                            */
/*                struct inode *inode        inode                            */
/*                struct file  *file                                          */
/*                                                                            */
/******************************************************************************/
int pfgc_release( struct inode *inode,
                   struct file *file )
{
    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    int ret;                                      /*                          */

    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0000 );

    /*------------------------------------------------------------------------*/
    /*                                                                        */
    /*------------------------------------------------------------------------*/
    if( file == NULL )                            /*             NULL         */
    {
        MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_RELEASE | 0x0001 );
        ret = -EFAULT;                            /*                          */
    }
    else if( inode == NULL )                      /* inode           NULL     */
    {
        MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0002 );
        MFGC_RDPROC_ERROR( DFGC_RELEASE | 0x0002 );
        ret = -EINVAL;                            /*                          */
    }
/*<QCIO017>    else if( MAJOR( file->f_dentry->d_inode->i_rdev ) != DFGC_MAJOR )  *//*  C   */
/*<QCIO017>    {                                               *//* MAJOR                    */
/*<QCIO017>        MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0003 );  */
/*<QCIO017>        MFGC_RDPROC_ERROR( DFGC_RELEASE | 0x0003 ); */
/*<QCIO017>        ret = -ENODEV;                              *//*                          */
/*<QCIO017>    }                                                              */
/*<QCIO017>    else                                            */
    else if( MAJOR( file->f_dentry->d_inode->i_rdev ) == DFGC_MAJOR ) /*<QCIO017>*/
    {
        MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0004 );

        /*--------------------------------------------------------------------*/
        /*     MINOR    open                                                  */
        /*--------------------------------------------------------------------*/
        if( MINOR( file->f_dentry->d_inode->i_rdev ) < DFGC_MINOR_NUM_MAX ) /*  C*/
        {
            MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0005 );
            gfgc_open_cnt[ MINOR( file->f_dentry->d_inode->i_rdev ) ]--;
                                                  /* open         --          */
        }

        if( gfgc_open_cnt[ MINOR( file->f_dentry->d_inode->i_rdev )] ==
            DFGC_CLEAR )
        {                                         /* open          0          */
            MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0006 );

            switch( MINOR( file->f_dentry->d_inode->i_rdev )) /*  C           */
            {                                     /* MINOR                    */
                case DFGC_INFO_MINOR:             /* /dev/fg/info             */
                    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0007 );
#if DFGC_SW_DELETED_FUNC
                    pint_unregist( DINT_FG_INT ); /*  V FG_INT                */
                    gfgc_ctl.flag = DFGC_OFF;     /* read                     */
#endif /* DFGC_SW_DELETED_FUNC */
                    break;                        /* break                    */

                case DFGC_DF_MINOR:               /* /dev/fg/df DF            */
                    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0008 );
                    break;                        /* break                    */

                case DFGC_VER_MINOR:              /* /dev/fg/ver VER          */
                    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0009 );
                    break;                        /* break                    */

                case DFGC_FW_MINOR:               /* /dev/fg/fw FW            */
                    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x000A );
                    break;                        /* break                    */

                case DFGC_DFS_MINOR:              /* /dev/fg/df DF            */
                    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x000B );
                    break;                        /* break                    */

                default:                          /* MINOR                    */
                    MFGC_RDPROC_PATH( DFGC_RELEASE | 0x000C );
                    MFGC_RDPROC_ERROR( DFGC_RELEASE | 0x0004 );
                    break;                        /* break                    */

            }
        }

        ret = DFGC_OK;                            /*                          */
    }
#ifdef    SYS_HAVE_IBP                                           /*<QCIO017>  */
    /* IBP                                                     *//*<QCIO017>  */
    else if( MAJOR( file->f_dentry->d_inode->i_rdev )            /*<QCIO017>  */
             == DFGC_IBP_MAJOR )                                 /*<QCIO017>  */
    {                                                            /*<QCIO017>  */
        MFGC_RDPROC_PATH( DFGC_RELEASE | 0x000D );               /*<QCIO017>  */
                                                                 /*<QCIO017>  */
        ret = DFGC_OK;                                           /*<QCIO017>  */
                                                                 /*<QCIO017>  */
        if( MINOR( file->f_dentry->d_inode->i_rdev )             /*<QCIO017>  */
            < DFGC_IBP_MINOR_MAX )                               /*<QCIO017>  */
        {                                                        /*<QCIO017>  */
            MFGC_RDPROC_PATH( DFGC_RELEASE | 0x000E );           /*<QCIO017>  */
                                                                 /*<QCIO017>  */
            /*                                                 *//*<QCIO017>  */
            gfgc_ibp_open_cnt                                    /*<QCIO017>  */
                [ MINOR( file->f_dentry->d_inode->i_rdev ) ]--;  /*<QCIO017>  */
        }                                                        /*<QCIO017>  */
        else                                                     /*<QCIO017>  */
        {                                                        /*<QCIO017>  */
            /*                                                 *//*<QCIO017>  */
            MFGC_RDPROC_PATH( DFGC_RELEASE | 0x000F );           /*<QCIO017>  */
            MFGC_RDPROC_ERROR( DFGC_RELEASE | 0x000F );          /*<QCIO017>  */
            ret = -ENODEV;                                       /*<QCIO017>  */
        }                                                        /*<QCIO017>  */
    }                                                            /*<QCIO017>  */
#endif /* SYS_HAVE_IBP */                                        /*<QCIO017>  */
    else                                                         /*<QCIO017>  */
    {                                                            /*<QCIO017>  */
        MFGC_RDPROC_PATH( DFGC_RELEASE | 0x0003 );               /*<QCIO017>  */
        MFGC_RDPROC_ERROR( DFGC_RELEASE | 0x0003 );              /*<QCIO017>  */
        ret = -ENODEV;                                           /*<QCIO017>  */
    }                                                            /*<QCIO017>  */

    MFGC_RDPROC_PATH( DFGC_RELEASE | 0xFFFF );

    return ret;                                   /* return                    */
}

/******************************************************************************/
/*                pfgc_exit                                                   */
/*                        DD exit                                             */
/*                2008.10.01                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pfgc_exit( void )
{
    unsigned long flags;                         /*<1100232>                  */

    MFGC_RDPROC_PATH( DFGC_EXIT | 0x0000 );

    /*------------------------------------------------------------------------*/
    /*         DD                                                             */
    /*------------------------------------------------------------------------*/
    pfgc_free_irq( );                             /* FuelGauge-IC             */

    /*------------------------------------------*//*<1100232>                 */
    /*                                          *//*<1100232>                 */
    /*------------------------------------------*//*<1100232>                 */
    spin_lock_irqsave( &gfgc_spinlock_if,         /*<1100232>                 */
                       flags );                   /*<1100232>                 */
    gfgc_kthread_flag |= D_FGC_EXIT_IND;          /*<1100232>                 */
    spin_unlock_irqrestore( &gfgc_spinlock_if,    /*<1100232>                 */
                            flags );              /*<1100232>                 */
    wake_up( &gfgc_task_wq );                     /*<1100232>  V              */
                                                  /*<1100232>                 */
    /*------------------------------------------*//*<1100232>                 */
    /* PROC                                     *//*<1100232>                 */
    /*------------------------------------------*//*<1100232>                 */
#ifdef CONFIG_PROC_FS_FGC                         /*<1100232>                 */
    remove_proc_entry( "driver/fgc", NULL );      /*<1100232>  V              */
#ifdef    SYS_HAVE_IBP                            /*<2040002>                 */
    remove_proc_entry( "driver/ibp", NULL );      /*<2040002>  V              */
#endif /* SYS_HAVE_IBP */                         /*<2040002>                 */
#if DFGC_SW_TEST_DEBUG
    remove_proc_entry( "driver/fgc_sdram", NULL );
    remove_proc_entry( "driver/fgc_nand1", NULL );
    remove_proc_entry( "driver/fgc_nand2", NULL );
#endif /* DFGC_SW_TEST_DEBUG */
#endif /* CONFIG_PROC_FS_FGC */                   /*<1100232>                 */

    pfgc_adrd_exit( );
    /*------------------------------------------------------------------------*/
    /*         DD                                                             */
    /*------------------------------------------------------------------------*/
    pfgc_unregister_chrdev( DFGC_MAJOR, DFGC_DEV );
#ifdef    SYS_HAVE_IBP                            /*<QCIO017>                 */
    unregister_chrdev( DFGC_IBP_MAJOR, DIBP_DEV );/*<QCIO017>  N              */
#endif /* SYS_HAVE_IBP */                         /*<QCIO017>                 */

    MFGC_RDPROC_PATH( DFGC_EXIT | 0xFFFF );

}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
struct file_operations gfgc_fops =               /*         DD file operations*/
{
/*<QAIO029>    llseek  : no_llseek,                                           */
/*<QAIO029>    read    : pfgc_read,                                           */
/*<QAIO029>    write   : pfgc_write,                                          */
/*<QAIO029>    poll    : pfgc_poll,                                           */
/*<QAIO029>    ioctl   : pfgc_ioctl,                                          */
/*<QAIO029>    open    : pfgc_open,                                           */
/*<QAIO029>    release : pfgc_release                                         */
    .llseek  = no_llseek,                         /*<QAIO029>                 */
    .read    = pfgc_read,                         /*<QAIO029>                 */
    .write   = pfgc_write,                        /*<QAIO029>                 */
    .poll    = pfgc_poll,                         /*<QAIO029>                 */
//    .ioctl   = pfgc_ioctl,                        /*<QAIO029>                 */
    .unlocked_ioctl   = pfgc_ioctl,               /* IDPower */
    .open    = pfgc_open,                         /*<QAIO029>                 */
    .release = pfgc_release                       /*<QAIO029>                 */
};

module_init( pfgc_init );                         /*  V         DD init       */
module_exit( pfgc_exit );                         /*  V         DD exit       */

