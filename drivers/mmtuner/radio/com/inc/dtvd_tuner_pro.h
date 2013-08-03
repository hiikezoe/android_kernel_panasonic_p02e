#ifndef _DTVD_TUNER_PRO_H_
#define _DTVD_TUNER_PRO_H_


#define DTVD_TUNER_ATTRIBUTE
/*****************************************************************************/
/* 関数プロトタイプ宣言                                                      */
/*****************************************************************************/
/*---------------------------------------------------------------------------*/
/* 共通関数                                                                  */
/*---------------------------------------------------------------------------*/
/* 標準ライブラリ */
void _dtvd_tuner_memset( void* dest, unsigned int val, unsigned int count );
void _dtvd_tuner_memcpy( void* dest, const void* src, unsigned int count );
void _dtvd_tuner_memmove( void* dest, const void* src, unsigned int count );
#ifdef D_DTVD_TUNER_NO_BUFFOVER_CHECK
#define dtvd_tuner_memset( dest, val, count, destsize )\
    (_dtvd_tuner_memset( (dest), (val), (count) ))
#define dtvd_tuner_memmcpy( dest, src, count, destsize )\
    (_dtvd_tuner_memxpy( (dest), (src), (count) ))
#define dtvd_tuner_memmove( dest, src, count, destsize )\
    (_dtvd_tuner_memmove( (dest), (src), (count) ))
#else
void dtvd_tuner_memset( void* dest, unsigned int val, unsigned int count, unsigned int destsize );
void dtvd_tuner_memcpy( void* dest, const void* src, unsigned int count, unsigned int destsize );
void dtvd_tuner_memmove( void* dest, const void* src, unsigned int count, unsigned int destsize );
#endif /* D_DTVD_TUNER_NO_BUFFOVER_CHECK */

/* デバイスアクセス */
signed int dtvd_tuner_com_dev_i2c_read( unsigned long, unsigned char*, unsigned char* );
signed int dtvd_tuner_com_dev_i2c_write( unsigned long, DTVD_TUNER_COM_I2C_DATA_t* );
signed int dtvd_tuner_com_dev_i2c_init( void );

/* void dtvd_tuner_com_dev_romdata_read( void ); *//* Arrietty 不揮発読出し処理不要の為、削除 2011/11/29 */
void dtvd_tuner_com_dev_int_regist( unsigned int );
void dtvd_tuner_com_dev_int_unregist( void );	/* Mod：(BugLister ID90) 2011/11/29 */
void dtvd_tuner_com_dev_int_enable( void );
void dtvd_tuner_com_dev_int_disable( void );
irqreturn_t dtvd_tuner_com_dev_int_handler(int irq, void *dev_id);
void dtvd_tuner_com_dev_ddsync_write( struct file *, void*, size_t );

#ifdef TUNER_MCBSP_ENABLE
signed int dtvd_tuner_com_dev_mcbsp_start( void );
void dtvd_tuner_com_dev_mcbsp_stop( void );
signed int dtvd_tuner_com_dev_mcbsp_read_start( dma_addr_t buffer[2], unsigned int length[2]);
signed int dtvd_tuner_com_dev_mcbsp_read( dma_addr_t buffer, unsigned int length);
void dtvd_tuner_com_dev_mcbsp_read_stop(void);
void dtvd_tuner_com_dev_mcbsp_read_cancel(void);
#endif /* TUNER_MCBSP_ENABLE */

signed int  dtvd_tuner_com_dev_pipe_open( unsigned char* pipename );	/* Mod ：(BugLister ID60) 2011/10/04 */
void dtvd_tuner_com_dev_pipe_close( void );								/* Add ：(BugLister ID60) 2011/10/04 */
void dtvd_tuner_com_dev_mcbspcfg_set( unsigned char mode );				/* Add：(BugLister ID86) 2011/11/07 */

#ifdef TUNER_TSIF_ENABLE
/* TSIF */
signed int dtvd_tuner_com_dev_tsif_start( void );
void dtvd_tuner_com_dev_tsif_stop( void );
signed int dtvd_tuner_com_dev_tsif_read ( void __user *buffer , size_t read_len );
void dtvd_tuner_com_dev_tsif_user_setting( unsigned int user );
#endif /* TUNER_TSIF_ENABLE */
/*---------------------------------------------------------------------------*/
/* マクロ関数                                                                */
/*---------------------------------------------------------------------------*/
#define DTVD_TUNER_COM_DEV_I2C_WRITE( _cmd_dat_ )                             \
    ( dtvd_tuner_com_dev_i2c_write(                                           \
        (unsigned long) ( sizeof( _cmd_dat_ ) / sizeof(DTVD_TUNER_COM_I2C_DATA_t) ) , \
        ( _cmd_dat_ ) ) )


/*---------------------------------------------------------------------------*/
/* SYSERR関連定義                                                            */
/*---------------------------------------------------------------------------*/
/* チューナドライバ機能ブロックID */
#define DTV_DRV    0x1234    /*!!!暫定!!!*/
#define D_DTVD_TUNER_SYSERR_BLKID               ( ( DTV_DRV << 16 ) | 0x00002000 )

/* チューナドライバ */
#define D_DTVD_TUNER_SYSERR_DRV_SYSTEM          0   /* システムエラー */
#define D_DTVD_TUNER_SYSERR_DRV_PARAM           1   /* パラメータエラー */
#define D_DTVD_TUNER_SYSERR_DRV_STATE           2   /* 状態不一致エラー */
#define D_DTVD_TUNER_SYSERR_DRV_I2C_READ        3   /* I2C：Readエラー */
#define D_DTVD_TUNER_SYSERR_DRV_I2C_WRITE       4   /* I2C：Writeエラー */
#define D_DTVD_TUNER_SYSERR_DRV_I2C_INIT        5   /* I2C：Initエラー */
#define D_DTVD_TUNER_SYSERR_DRV_GPIO_READ       6   /* GPIO：Readエラー */
#define D_DTVD_TUNER_SYSERR_DRV_GPIO_WRITE      7   /* GPIO：Writeエラー */
#define D_DTVD_TUNER_SYSERR_DRV_MCBSP_READ      8   /* McBSP：READエラー */
#define D_DTVD_TUNER_SYSERR_DRV_NONVOLA_READ    9   /* 不揮発読み出しエラー */
#define D_DTVD_TUNER_SYSERR_DRV_SPI_READ        10  /* SPI：Readエラー */
#define D_DTVD_TUNER_SYSERR_DRV_SPI_WRITE       11  /* SPI：Writeエラー */
#define D_DTVD_TUNER_SYSERR_DRV_PIPE_WRITE      12  /* PIPE writeエラー */
#define D_DTVD_TUNER_SYSERR_DRV_PINT_REG        13  /* 割り込み制御IF:割り込み登録エラー */
#define D_DTVD_TUNER_SYSERR_DRV_PINT_UNREG      14  /* 割り込み制御IF:割り込み登録削除エラー */
#define D_DTVD_TUNER_SYSERR_DRV_PINT_ENABLE     15  /* 割り込み制御IF:割り込み許可設定エラー */
#define D_DTVD_TUNER_SYSERR_DRV_PINT_DISABLE    16  /* 割り込み制御IF:割り込み禁止設定エラー */
#define D_DTVD_TUNER_SYSERR_DRV_3WIRE_TRANS     17  /* 3wire転送エラー */
#define D_DTVD_TUNER_SYSERR_DRV_PSEQ_CHKSUM     18  /* PSEQチェックサムエラー */
#define D_DTVD_TUNER_SYSERR_DRV_INT_CLEAR       19  /* 割り込み要因クリアエラー */
#define D_DTVD_TUNER_SYSERR_DRV_INT_UNKNOWN     20  /* 予期せぬ割り込み発生 */
#define D_DTVD_TUNER_SYSERR_DRV_ETC             21  /* その他エラー */
#define D_DTVD_TUNER_SYSERR_DRV_DDSYNC_WRITE    22

/* ファイル番号 */
#define DTVD_TUNER_API              0
#define DTVD_TUNER_AGC_API          1
#define DTVD_TUNER_AGC_COM          2
#define DTVD_TUNER_AGC_MTX000       3
#define DTVD_TUNER_AGC_MTX001       4
#define DTVD_TUNER_AGC_MTX002       5
#define DTVD_TUNER_AGC_MTX003       6
#define DTVD_TUNER_AGC_MTX004       7
#define DTVD_TUNER_AGC_MTX005       8
#define DTVD_TUNER_AGC_MTX006       9
#define DTVD_TUNER_AGC_MTX007       10
#define DTVD_TUNER_AGC_MTX008       11
#define DTVD_TUNER_AGC_TRANS        12
#define DTVD_TUNER_ANT_API          13
#define DTVD_TUNER_ANT_COM          14
#define DTVD_TUNER_ANT_MTX000       15
#define DTVD_TUNER_ANT_MTX001       16
#define DTVD_TUNER_ANT_TRANS        17
#define DTVD_TUNER_BER_API          18
#define DTVD_TUNER_BER_COM          19
#define DTVD_TUNER_BER_MTX000       20
#define DTVD_TUNER_BER_MTX001       21
#define DTVD_TUNER_BER_MTX002       22
#define DTVD_TUNER_BER_MTX003       23
#define DTVD_TUNER_BER_MTX004       24
#define DTVD_TUNER_BER_MTX005       25
#define DTVD_TUNER_BER_MTX006       26
#define DTVD_TUNER_BER_TRANS        27
#define DTVD_TUNER_CN_API           28
#define DTVD_TUNER_CN_COM           29
#define DTVD_TUNER_CN_MTX000        30
#define DTVD_TUNER_CN_MTX001        31
#define DTVD_TUNER_CN_MTX002        32
#define DTVD_TUNER_CN_MTX003        33
#define DTVD_TUNER_CN_MTX004        34
#define DTVD_TUNER_CN_MTX005        35
#define DTVD_TUNER_CN_MTX006        36
#define DTVD_TUNER_CN_TRANS         37
#define DTVD_TUNER_COM_DEV          38
#define DTVD_TUNER_COM_STD          39
#define DTVD_TUNER_CORE_API         40
#define DTVD_TUNER_CORE_COM         41
#define DTVD_TUNER_CORE_MTX000      42
#define DTVD_TUNER_CORE_MTX001      43
#define DTVD_TUNER_CORE_MTX002      44
#define DTVD_TUNER_CORE_MTX003      45
#define DTVD_TUNER_CORE_MTX004      46
#define DTVD_TUNER_CORE_MTX005      47
#define DTVD_TUNER_CORE_MTX006      48
#define DTVD_TUNER_CORE_MTX007      49
#define DTVD_TUNER_CORE_MTX008      50
#define DTVD_TUNER_CORE_MTX009      51
#define DTVD_TUNER_CORE_MTX010      52
#define DTVD_TUNER_CORE_MTX011      53
#define DTVD_TUNER_CORE_MTX012      54
#define DTVD_TUNER_CORE_MTX013      55
#define DTVD_TUNER_CORE_MTX014      56
#define DTVD_TUNER_CORE_TRANS       57
#define DTVD_TUNER_DRIVER           58
#define DTVD_TUNER_MAIN             59
#define DTVD_TUNER_MAIN_ANLZ        60
#define DTVD_TUNER_MAIN_MTX         61
#define DTVD_TUNER_MSG              62
#define DTVD_TUNER_RAM              63
#define DTVD_TUNER_ROM              64
#define DTVD_TUNER_TIMER            65
#define DTVD_TUNER_TMCC_API         66
#define DTVD_TUNER_TMCC_COM         67
#define DTVD_TUNER_TMCC_MTX000      68
#define DTVD_TUNER_TMCC_MTX001      69
#define DTVD_TUNER_TMCC_MTX002      70
#define DTVD_TUNER_TMCC_MTX003      71
#define DTVD_TUNER_TMCC_TRANS       72
#define DTVD_TUNER_TUNER_API        73
#define DTVD_TUNER_TUNER_COM        74
#define DTVD_TUNER_TUNER_MTX000     75
#define DTVD_TUNER_TUNER_MTX001     76
#define DTVD_TUNER_TUNER_MTX002     77
#define DTVD_TUNER_TUNER_MTX003     78
#define DTVD_TUNER_TUNER_MTX004     79
#define DTVD_TUNER_TUNER_MTX005     80
#define DTVD_TUNER_TUNER_MTX006     81
#define DTVD_TUNER_TUNER_MTX007     82
#define DTVD_TUNER_TUNER_MTX008     83
#define DTVD_TUNER_TUNER_MTX009     84
#define DTVD_TUNER_TUNER_MTX010     85
#define DTVD_TUNER_TUNER_TRANS      86
#define DTVD_TUNER_STATE_API        87
#define DTVD_TUNER_STATE_COM        88
#define DTVD_TUNER_STATE_MTX000     89
#define DTVD_TUNER_STATE_MTX001     90
#define DTVD_TUNER_STATE_MTX002     91
#define DTVD_TUNER_STATE_TRANS      92
#define DTVD_TUNER_AUTOECO_API      93
#define DTVD_TUNER_AUTOECO_COM      94
#define DTVD_TUNER_AUTOECO_MTX000   95
#define DTVD_TUNER_AUTOECO_MTX001   96
#define DTVD_TUNER_AUTOECO_MTX002   97
#define DTVD_TUNER_AUTOECO_TRANS    98
#define DTVD_TUNER_AUTOECO_MTX003   99
#define DTVD_TUNER_TSPRCV          100
#define DTVD_TUNER_TSPRCV_API      101
#define DTVD_TUNER_TSPRCV_TRANS    102

void dtvd_tuner_com_dev_log
(
    unsigned char   rank,       /* アラームランク */
    unsigned long   kind,       /* エラー種別 */
    unsigned char   file_no,    /* ファイル番号 */
    unsigned long   line,       /* 行番号 */
    unsigned long   data1,      /* data1 */
    unsigned long   data2,      /* data2 */
    unsigned long   data3,      /* data3 */
    unsigned long   data4,      /* data4 */
    unsigned long   data5,      /* data5 */
    unsigned long   data6       /* data6 */
);

#ifdef _DTVD_TUNER_SYSERR
#if 1
#define DTVD_TUNER_SYSERR_RANK_A( kind, file_no, data1, data2, data3, data4, data5, data6 ) \
        ( printk( "SYSERR[RANK_A], kind=%d, file_no=%d, line=%d, data1=%d, data2=%d, data3=%d, data4=%d, data5=%d, data6=%d\n", \
                   kind, file_no, __LINE__, (int)(data1), (int)(data2), (int)(data3), (int)(data4), (int)(data5), (int)(data6) ) )
#define DTVD_TUNER_SYSERR_RANK_B( kind, file_no, data1, data2, data3, data4, data5, data6 ) \
        ( printk( "SYSERR[RANK_B], kind=%d, file_no=%d, line=%d, data1=%d, data2=%d, data3=%d, data4=%d, data5=%d, data6=%d\n", \
                   kind, file_no, __LINE__, (int)(data1), (int)(data2), (int)(data3), (int)(data4), (int)(data5), (int)(data6) ) )
#else
#define DTVD_TUNER_SYSERR_RANK_A( kind, file_no, data1, data2, data3, data4, data5, data6 ) \
        ( dtvd_tuner_com_dev_log( D_DTVD_TUNER_DEVERR_SYSERRRANKA, (kind), (file_no), __LINE__,  \
                                  (data1), (data2), (data3), (data4), (data5), (data6) ) )

#define DTVD_TUNER_SYSERR_RANK_B( kind, file_no, data1, data2, data3, data4, data5, data6 ) \
    ( dtvd_tuner_com_dev_log( D_DTVD_TUNER_DEVERR_SYSERRRANKB, (kind), (file_no), __LINE__,  \
                                  (data1), (data2), (data3), (data4), (data5), (data6) ) )



#endif /* _DTVD_TUNER_DEBUG_OUT */

#define DTVD_DEBUG_MSG_ERR( fmt, args... )    \
                   printk( "%s %s %s %d %s : " fmt,__DATE__,       \
                           __TIME__,__FILE__,__LINE__,__FUNCTION__,## args )


#else
#define DTVD_TUNER_SYSERR_RANK_A( kind, file_no, data1, data2, data3, data4, data5, data6 )
#define DTVD_TUNER_SYSERR_RANK_B( kind, file_no, data1, data2, data3, data4, data5, data6 )
#define DTVD_DEBUG_MSG_ERR( fmt, args... )

#endif /* _DTVD_TUNER_SYSERR */

#ifdef _DTVD_TUNER_DEBUG_OUT
#define DTVD_DEBUG_MSG_ENTER( p1, p2, p3 )       \
                   printk( "%s %s %s %d ENTER :%s p1=%d p2=%d p3=%d\n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__, p1, p2, p3 )
#define DTVD_DEBUG_MSG_EXIT( )       \
                   printk(  "%s %s %s %d EXIT  :%s \n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__ )
#define DTVD_DEBUG_MSG_CALL( funcname )       \
                   printk( "%s %s %s %d CALL  :%s \n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (funcname) )
#define DTVD_DEBUG_MSG_PCOM_READ( kind, addr, data )       \
                   printk( "%s %s %s %d  %s READ:addr:%x data:%x\n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (kind), (unsigned int)(addr), (unsigned int)(data) )
#define DTVD_DEBUG_MSG_PCOM_WRITE( kind, addr, data )       \
                   printk( "%s %s %s %d  %s WRITE:addr:%x data:%x\n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (kind), (unsigned int)(addr), (unsigned int)(data) )
#define DTVD_DEBUG_MSG_INFO( fmt, args... )    \
                   printk( "%s %s %s %d %s : " fmt,__DATE__,       \
                           __TIME__,__FILE__,__LINE__,__FUNCTION__,## args )

#else
#define DTVD_DEBUG_MSG_ENTER( p1, p2, p3 )
#define DTVD_DEBUG_MSG_EXIT(  )
#define DTVD_DEBUG_MSG_CALL( funcname )
#ifdef _DTVD_TUNER_DEBUG_IO_OUT
#define DTVD_DEBUG_MSG_PCOM_READ( kind, addr, data )       \
                   printk( "%s %s %s %d  %s READ:addr:%x data:%x\n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (kind), (addr), (int)(data) )
#define DTVD_DEBUG_MSG_PCOM_WRITE( kind, addr, data )       \
                   printk( "%s %s %s %d  %s WRITE:addr:%x data:%x\n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (kind), (addr), (int)(data) )
#else
#define DTVD_DEBUG_MSG_PCOM_READ( kind, addr, data )
#define DTVD_DEBUG_MSG_PCOM_WRITE( kind, addr, data )
#endif /*_DTVD_TUNER_DEBUG_IO_OUT*/
#ifndef WIN32
#define DTVD_DEBUG_MSG_INFO( fmt, args... )
#else
#define DTVD_DEBUG_MSG_INFO( fmt, args )
#endif /* WIN32 */
#endif /* _DTVD_TUNER_DEBUG_OUT */


/*****************************************************************************/
/* 外部参照宣言                                                              */
/*****************************************************************************/
extern DTVD_TUNER_CONTROL_t         tdtvd_tuner_info;       /* チューナドライバ管理情報 */
extern DTVD_TUNER_MONITOR_INFO_t    tdtvd_tuner_monitor;/* チューナ情報 */
extern DTVD_TUNER_COM_NONVOLATILE_t tdtvd_tuner_nonvola;/* 不揮発情報 */

#endif /* _DTVD_TUNER_PRO_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
