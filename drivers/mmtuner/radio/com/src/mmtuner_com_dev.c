#include "dtvd_tuner_com.h"
#include "dtvd_tuner_driver.h"
#include "dtvd_tuner_core.h"
#include "dtvd_tuner_tuner.h" /* Add：(BugLister ID52) 2011/09/15 */
#ifdef TUNER_MCBSP_ENABLE
#include <plat/mcbsp.h>
#endif /* TUNER_MCBSP_ENABLE */
//#include <linux/cfgdrv.h>
#ifdef TUNER_TSIF_ENABLE
#include <linux/err.h>          /* IS_ERR etc. */
#include <linux/tsif_api.h>
#include <linux/cdev.h>
#endif /* TUNER_TSIF_ENABLE */


/*****************************************************************************/
/* ローカル関数プロトタイプ宣言                                              */
/*****************************************************************************/
/* static void dtvd_tuner_com_dev_romdata_debugout( void ); *//* Arrietty 不揮発読出し処理不要の為、削除 2011/11/29 */

/* Add Start：(BugLister ID52) 2011/09/15 */
static void dtvd_tuner_com_dev_int_workqueue(struct work_struct* work);
DECLARE_WORK(tdtvd_tuner_int_workqueue, dtvd_tuner_com_dev_int_workqueue);
/* Add End  ：(BugLister ID52) 2011/09/15 */

static void dtvd_tuner_com_dev_trans_msg_syserr( unsigned char );

#ifdef TUNER_MCBSP_ENABLE
/*---------------------------------------------------------------------------*/
/* McBSP  config初期値設定レジスタテーブル                                   */
/*---------------------------------------------------------------------------*/
static const struct omap_mcbsp_reg_cfg dtvd_tuner_mcbsp_config
      = {	0,						/* spcr2 */
      		0,					/* spcr1 */
      		0,						/* rcr2  */
      		(RFRLEN1(51-1) | RWDLEN1(5)),		/* rcr1 RFRLEN1=102-1 RDWLEN1=5(32bits) */
      		0,						/* xcr2 */
      		0,						/* xcr1 */
      		(FPER(1632-1)),				/* srgr2 FPER2=1632 他は0 *//* Mod：(Sysdes対応) 2012/02/09 */
      		(FWID(188-1) | CLKGDV(1)),			/* FWID=188-1   他は0 */
      		0,						/* mcr2 */
      		0,						/* mcr1 */
      		(SCLKME),		        		/* pcr0 */
      		0,						/* rcerc */
      		0,						/* rcerd */
      		0, 						/* xcerc */
      		0,						/* xcerd */
      		0,						/* rcere */
      		0,						/* rcerf */
      		0,						/* xcere */
      		0,						/* xcerf */
      		0,						/* rcerg */
      		0,						/* rcerh */
      		0,						/* xcerg */
      		0,						/* xcerh */
      		0,						/* xccr */
      		(RDMAEN | RFULL_CYCLE)				/* rccr */
      };
/* Add Start：(BugLister ID86) 2011/11/07 */
static const struct omap_mcbsp_reg_cfg mm_tuner_mcbsp_config
      = {	0,						/* spcr2 */
      		0,					/* spcr1 */
      		0,						/* rcr2  */
      		(RFRLEN1(47-1) | RWDLEN1(5)),		/* rcr1 RFRLEN1=47-1 RDWLEN1=5(32bits) */
      		0,						/* xcr2 */
      		0,						/* xcr1 */
      		(FPER(1504-1)),				/* srgr2 FPER2=1504 他は0 *//* Mod：(Sysdes対応) 2012/02/09 */
      		(FWID(188-1) | CLKGDV(1)),			/* FWID=188-1   他は0 */
      		0,						/* mcr2 */
      		0,						/* mcr1 */
      		(SCLKME),		        		/* pcr0 */
      		0,						/* rcerc */
      		0,						/* rcerd */
      		0, 						/* xcerc */
      		0,						/* xcerd */
      		0,						/* rcere */
      		0,						/* rcerf */
      		0,						/* xcere */
      		0,						/* xcerf */
      		0,						/* rcerg */
      		0,						/* rcerh */
      		0,						/* xcerg */
      		0,						/* xcerh */
      		0,						/* xccr */
      		(RDMAEN | RFULL_CYCLE)			/* rccr */
      };

static struct omap_mcbsp_reg_cfg com_tuner_mcbsp_config;
#endif /* TUNER_MCBSP_ENABLE */
/* Add End：(BugLister ID86) 2011/11/07 */
static  int dtvint_enable ;
unsigned int D_DTVD_TUNER_TSP_LENGTH;	/* TSPパケット長 *//* Add：(BugLister ID93) 2011/12/08 */
unsigned char TS_RECV_MODE;				/* Add：(MMシステム対応) 2012/03/14 */

#ifdef TUNER_TSIF_ENABLE
/* TSIF enable */
struct tsif_chrdev {
	struct cdev cdev;
	struct device *dev;
	wait_queue_head_t wq_read;
	void *cookie;
	/* mirror for tsif data */
	void *data_buffer;
	unsigned int buf_size_packets; /**< buffer size in packets */
	unsigned int ri, wi;
	enum tsif_state state;
	unsigned rptr;
};
static unsigned int g_tuner_user = D_DTVD_TUNER_USER_DTV;	/* tsif buffer init is DTV setting */
#define TSIF_NUM_DEVS 1 /**< support this many devices */
static struct tsif_chrdev the_devices[TSIF_NUM_DEVS];
static int dtvd_tuner_com_dev_tsif_init_one(struct tsif_chrdev *the_dev, int index);
#endif /* TUNER_TSIF_ENABLE */

extern int g_tuner_slave_addr;                                                 /* 20120604 チューナアドレス振替用 */

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_i2c_read                                    */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_i2c_read
(
    unsigned long length,                               /* データサイズ */
    unsigned char *addr,                                /* 読込みデータアドレス */
    unsigned char *data                                 /* データ格納先アドレス */
)
{
    int ret;
    
    /* i2c_transfer() パラメータ      */
    struct i2c_msg msgs[2] = { {0, 0, 0, 0},
                               {0, 0, 0, 0}};
    struct i2c_adapter *adap;

        /* パラメータチェック */
    if(( length > D_DTVD_TUNER_DATALEN_MAX ) ||
       ( length == 0 ))
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, length, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* NULLチェック */
    if( addr == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, 0, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    if( data == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, 0, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    msgs[0].addr = g_tuner_slave_addr;  /* slave address            */         /* 20120618 チューナアドレス可変対応       */
    msgs[0].flags = 0;                      /* write data           */
    msgs[0].len  = 1;                       /* msg length           */
    msgs[0].buf  = addr;                    /* pointer to msg data  */
    msgs[1].addr = g_tuner_slave_addr;  /* slave address            */         /*20120618 チューナアドレス可変対応       */
    msgs[1].flags = I2C_M_RD;               /* read data, from slave to master */
    msgs[1].len  = length;                  /* datalength           */
    msgs[1].buf  = data;                    /* pointer to msg data  */
    adap = tdtvd_tuner_info.adap;           /*  I2Cアダプタ         */
    ret = i2c_transfer(adap, msgs, 2);      /* i2c_transfer()       */
    if( ret != 2 )
    {
        /* Syserr(エラー要因出力) */
        DTVD_TUNER_SYSERR_RANK_B( D_DTVD_TUNER_SYSERR_DRV_I2C_READ,
                                  DTVD_TUNER_COM_DEV,
                                  ret, g_tuner_slave_addr, addr, *addr, 0, 0 ); /* 20120618 チューナアドレス可変対応 */

        return D_DTVD_TUNER_NG;
    }
    DTVD_DEBUG_MSG_PCOM_READ( "I2C", *addr, *data );


    return D_DTVD_TUNER_OK;

}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_i2c_write                                   */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_i2c_write
(
    unsigned long length,                               /* データサイズ */
    DTVD_TUNER_COM_I2C_DATA_t *data                     /* I2C書込み用データ構造 */
)
{
    struct i2c_msg msg;
    unsigned char  i2cdata[D_DTVD_TUNER_DATALEN_MAX*2];
    unsigned char  *pdata;
    int i;
    int ret;

    DTVD_DEBUG_MSG_ENTER( data[0].adr, data[0].data, 0 );	/* Add：(BugLister ID52) 2011/09/15 */
	
    /* NULLチェック */
    if( data == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, 0, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* パラメータチェック */
    if( length > D_DTVD_TUNER_DATALEN_MAX )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, length, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* lengthが0の場合は、書き込みを行わない */
    if( length == 0 )
    {
        return D_DTVD_TUNER_OK;
    }

    /* i2cdata へ I2C書込み用データよりadr,dataを格納する。*/
    pdata = &i2cdata[0];
    for( i = 0; i < length; i++ )
    {
        DTVD_DEBUG_MSG_PCOM_WRITE( "I2C", data[i].adr, data[i].data );
        pdata[0] = data[i].adr; 
        pdata[1] = data[i].data; 
        pdata +=2;
    }
    msg.addr = g_tuner_slave_addr;  /* slave address 20120618 チューナアドレス可変対応       */
    msg.flags = 0;                      /* write data           */
    msg.len  = length*2;                /* msg length           */
    msg.buf  = i2cdata;                 /* pointer to msg data  */
    ret = i2c_transfer(tdtvd_tuner_info.adap, &msg, 1);  /* i2c_transfer()       */
    if( ret != 1 )
    {
        /* Syserr(エラー要因出力) */
        DTVD_TUNER_SYSERR_RANK_B( D_DTVD_TUNER_SYSERR_DRV_I2C_WRITE,
                                  DTVD_TUNER_COM_DEV,
                                  ret, g_tuner_slave_addr, i2cdata, i2cdata[0], i2cdata[1], 0 ); /* 20120618 チューナアドレス可変対応 */
        return D_DTVD_TUNER_NG;
    }


    DTVD_DEBUG_MSG_EXIT();	/* Add：(BugLister ID52) 2011/09/15 */
    return D_DTVD_TUNER_OK;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_i2c_init                                    */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_i2c_init(void)
{

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );	/* Add：(BugLister ID52) 2011/09/15 */
	
	/* I２Cアダプタがチューナドライバ管理情報に登録済みの場合、正常終了する。*/
    if(tdtvd_tuner_info.adap)
    {
       return D_DTVD_TUNER_OK;
    }

    /* I2Cアダプタを取得して、チューナドライバ管理情報にI2Cアダプタを設定する。 */
    tdtvd_tuner_info.adap = i2c_get_adapter(D_DTVD_TUNER_DEV_I2C);
    if(tdtvd_tuner_info.adap == NULL) {
        /* Syserr(エラー要因出力) */
        DTVD_TUNER_SYSERR_RANK_B( D_DTVD_TUNER_SYSERR_DRV_I2C_INIT,
                                  DTVD_TUNER_COM_DEV,
                                  0, g_tuner_slave_addr, 0, 0, 0, 0 );         /* 20120618 チューナアドレス可変対応 */

        return D_DTVD_TUNER_NG;
    }
    
    DTVD_DEBUG_MSG_EXIT();	/* Add：(BugLister ID52) 2011/09/15 */
	return D_DTVD_TUNER_OK;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_romdata_read                                */
/*****************************************************************************/
#if 0 /* Arrietty 不揮発読出し処理不要の為、削除 2011/11/29 */
void dtvd_tuner_com_dev_romdata_read( void )
{
    signed int ret;


    /* 不揮発情報初期化 */
    dtvd_tuner_memset( &tdtvd_tuner_nonvola,
                       0x00,
                       sizeof(DTVD_TUNER_COM_NONVOLATILE_t),
                       sizeof(DTVD_TUNER_COM_NONVOLATILE_t) );

    /* 不揮発データを読み出す */
    ret = cfgdrv_read(  D_DTVD_TUNER_INFO_ID,
	                    sizeof(DTVD_TUNER_COM_NONVOLATILE_t),
	                    ( unsigned char* )(void*)&tdtvd_tuner_nonvola );
    if( ret != 0 )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_NONVOLA_READ,
                                  DTVD_TUNER_COM_DEV,
                                  (unsigned long)ret, 0, 0, 0, 0, 0 );
        return;
    } 
	ret = 0; /*ワーニング回避*/

	
    /* デバッグ用 */
    dtvd_tuner_com_dev_romdata_debugout();

    return;
}
#endif /* Arrietty 不揮発読出し処理不要の為、削除 2011/11/29 */

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_int_regist                                  */
/*****************************************************************************/
void dtvd_tuner_com_dev_int_regist
(
    unsigned int param      /* 割り込み設定 */
)
{
    int ret;

    DTVD_DEBUG_MSG_ENTER( param, 0, 0 );

    /* GPIO15端子割り込み設定 */
    ret = gpio_request( D_DTVD_TUNER_INT_PORT, "DTVINT" );
    if( ret )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PINT_REG,
                                  DTVD_TUNER_COM_DEV,
                                  ret,
                                  D_DTVD_TUNER_INT_PORT,
                                  param, 0, 0, 0 );
        return;
    }

    ret = gpio_direction_input( D_DTVD_TUNER_INT_PORT );
    if( ret )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PINT_REG,
                                  DTVD_TUNER_COM_DEV,
                                  ret,
                                  D_DTVD_TUNER_INT_PORT,
                                  param, 0, 0, 0 );
        return;
    }

    /* 割り込みハンドラ登録 */
    ret = request_irq( gpio_to_irq( D_DTVD_TUNER_INT_PORT ),
                       dtvd_tuner_com_dev_int_handler,      /* 割り込みハンドラ */
                       IRQF_TRIGGER_LOW ,                   /* SA_FLAGS フラグ */
                       D_DTVD_TUNER_DRV_NAME,               /* デバイス名 */
                       &tdtvd_tuner_info );                 /* デバイスID */
    if( ret )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PINT_REG,
                                  DTVD_TUNER_COM_DEV,
                                  ret,
                                  D_DTVD_TUNER_INT_PORT,
                                  dtvd_tuner_com_dev_int_handler,
                                  param, 0, 0 );
        return;
    }

    /* DTVD(GPIO_15)割り込み禁止設定 */
    dtvint_enable = 1;
    if( param != D_DTVD_TUNER_INT_MASK_CLEAR )
    {
        dtvd_tuner_com_dev_int_disable();
    }

    DTVD_DEBUG_MSG_EXIT();

    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_int_unregist                                */
/*****************************************************************************/
void dtvd_tuner_com_dev_int_unregist( void ) /* Mod：(BugLister ID90) 2011/11/29 */
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* DTVD(GPIO_15)割り込み禁止設定 */
    dtvd_tuner_com_dev_int_disable();

    /* 割り込みハンドラ登録解除 */
    free_irq( gpio_to_irq( D_DTVD_TUNER_INT_PORT ),     /* 割り込みハンドラ */
              &tdtvd_tuner_info );                      /* デバイスID */

    /* GPIO15端子解放 */
    gpio_free( D_DTVD_TUNER_INT_PORT );

    DTVD_DEBUG_MSG_EXIT();

    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_int_enable                                  */
/*****************************************************************************/
void dtvd_tuner_com_dev_int_enable( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    if(!dtvint_enable) 
    {
        dtvint_enable =1;
        enable_irq( gpio_to_irq( D_DTVD_TUNER_INT_PORT ) );
    }
    DTVD_DEBUG_MSG_EXIT();

    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_int_disable                                 */
/*****************************************************************************/
void dtvd_tuner_com_dev_int_disable( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    if(dtvint_enable)
    {
        dtvint_enable = 0;
        disable_irq( gpio_to_irq( D_DTVD_TUNER_INT_PORT ) );
    }
    DTVD_DEBUG_MSG_EXIT();

    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_int_handler                                 */
/*****************************************************************************/
irqreturn_t dtvd_tuner_com_dev_int_handler( int irq, void *dev_id )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );	/* Mod：(BugLister ID52) 2011/09/15 */

    dtvint_enable = 0;
    disable_irq_nosync( gpio_to_irq( D_DTVD_TUNER_INT_PORT ) );

/* Mod Start：(BugLister ID52) 2011/09/15 */
	/* ワークキュー登録 */
	schedule_work(&tdtvd_tuner_int_workqueue);
/* Mod End  ：(BugLister ID52) 2011/09/15 */

    DTVD_DEBUG_MSG_EXIT();	/* Mod：(BugLister ID52) 2011/09/15 */
    return IRQ_HANDLED;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_ddsync_write                                */
/*****************************************************************************/
void dtvd_tuner_com_dev_ddsync_write
(
    struct file *file,      /* PIPE　file情報 */
    void *      data,       /* データアドレス */
    size_t      data_size   /* データサイズ */
 )
{
    signed int ret;

    DTVD_DEBUG_MSG_ENTER( 0,  0, data_size );

    if( file == D_DTVD_TUNER_NULL )
    {
        /* リセット(ランクA) */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    if( data == D_DTVD_TUNER_NULL )
    {
        /* リセット(ランクA) */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    /* pipe書き込み */
    if( file->f_op->write)
    {
        ret = file->f_op->write(file, data, data_size, &(file->f_pos));
    }
    else
    {
        ret = do_sync_write(file, data, data_size, &(file->f_pos));
    }
    if( ret != (signed int)data_size )
    {
        /* リセット(ランクA) */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_DDSYNC_WRITE,
                                  DTVD_TUNER_COM_DEV,
                                  (unsigned long)ret, (unsigned long)file,
                                  file->f_op->write, (unsigned long)data, (unsigned long)data_size, 0 );
        return;
    }

    DTVD_DEBUG_MSG_EXIT();
    return;
}


#ifdef TUNER_MCBSP_ENABLE
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbsp_start                                 */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_mcbsp_start(void)
{
    int ret;
    
    DTVD_DEBUG_MSG_ENTER( 0,  0, 0 );

    /* McBSP通信タイプ設定（POLLING）*/
    ret = omap_mcbsp_set_io_type(D_DTVD_TUNER_MCBSP,OMAP_MCBSP_POLL_IO);
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  ret, D_DTVD_TUNER_MCBSP, OMAP_MCBSP_POLL_IO, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* McBSPスタートアップ処理 */
    ret = omap_mcbsp_request(D_DTVD_TUNER_MCBSP);
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  ret, D_DTVD_TUNER_MCBSP, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* McBSPレジスタ設定 */
    omap_mcbsp_config(D_DTVD_TUNER_MCBSP, &com_tuner_mcbsp_config);	/* Mod：(BugLister ID86) 2011/11/07 */
    
    /* McBSPの受信開始 */
    omap_mcbsp_start(D_DTVD_TUNER_MCBSP,  0, 1);
    
    DTVD_DEBUG_MSG_EXIT();
    return D_DTVD_TUNER_OK;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbsp_stop                                  */
/*****************************************************************************/
void dtvd_tuner_com_dev_mcbsp_stop(void)
{
    
    DTVD_DEBUG_MSG_ENTER( 0,  0, 0 );

    /* McBSP受信停止*/
    omap_mcbsp_stop(D_DTVD_TUNER_MCBSP, 0, 1);

    /* McBSPポートの解放 */
    omap_mcbsp_free(D_DTVD_TUNER_MCBSP);

    DTVD_DEBUG_MSG_EXIT();
    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbsp_read_start                            */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_mcbsp_read_start
(
    dma_addr_t      buffer[5],    /* DMA転送先アドレス */	/* Mod：(MMシステム対応) 2012/03/14 */
    unsigned int    length[5]     /* 受信データ長      */ 	/* Mod：(MMシステム対応) 2012/03/14 */
)
{
    int ret;
    
    /* パラメータチェック */
    if( (buffer == D_DTVD_TUNER_NULL) || (length == D_DTVD_TUNER_NULL) )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, length[0], length[1], 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    if( (length[0] < 2) || (length[1] < 2) || (length[2] < 2)|| (length[3] < 2)|| (length[4] < 2) )	/* Mod：(MMシステム対応) 2012/03/14 */
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, length[0], length[1], 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* DMA chain 受信処理によりTSパケットを受信する。 */
    ret = omap2_mcbsp_recv_buffer_chain_start2(D_DTVD_TUNER_MCBSP,						/* Mod：(MMシステム対応) 2012/03/14 */
                                            buffer,      /* 1st DMAハンドル */			/* Mod：(MMシステム対応) 2012/03/14 */
                                            length[0],   /* 1st 受信TSPパケット長 */	/* Mod：(MMシステム対応) 2012/03/14 */
    	                                    5);			 								/* Mod：(MMシステム対応) 2012/03/14 */
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_B( D_DTVD_TUNER_SYSERR_DRV_MCBSP_READ,
                                  DTVD_TUNER_COM_DEV,
                                  ret, g_tuner_slave_addr, length, 0, 0, 0 );  /* 20120618 チューナアドレス可変対応 */
        return D_DTVD_TUNER_NG;
    }
    return D_DTVD_TUNER_OK;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbsp_read                                  */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_mcbsp_read
(
    dma_addr_t      buffer,    /* DMA転送先アドレス */
    unsigned int    length     /* 受信データ長      */ 
)
{
    int ret;
    
    /* パラメータチェック */
    if( length < 2 )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, length, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }
    /* SimpleDMA受信処理によりTSパケットを受信する。 */
    ret = omap2_mcbsp_recv_buffer_chain_continue(D_DTVD_TUNER_MCBSP, buffer, length);
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_B( D_DTVD_TUNER_SYSERR_DRV_MCBSP_READ,
                                  DTVD_TUNER_COM_DEV,
                                  ret, g_tuner_slave_addr, length, 0, 0, 0 );  /* 20120618 チューナアドレス可変対応 */
        return D_DTVD_TUNER_NG;
    }
    return D_DTVD_TUNER_OK;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbsp_read_stop                             */
/*****************************************************************************/
void dtvd_tuner_com_dev_mcbsp_read_stop(void)
{
    int ret;
    
    /* McBSP DMA chain 停止 */
    ret = omap2_mcbsp_recv_buffer_chain_stop(D_DTVD_TUNER_MCBSP);
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_MCBSP_READ,
                                  DTVD_TUNER_COM_DEV,
                                  ret, 0, 0, 0, 0, 0 );
        return;
    }

    /* McBSP DMA chain 解放 */
    ret =omap2_mcbsp_recv_buffer_chain_free(D_DTVD_TUNER_MCBSP);
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_MCBSP_READ,
                                  DTVD_TUNER_COM_DEV,
                                  ret, 0, 0, 0, 0, 0 );
        return;
    }
    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbsp_read_cancel                           */
/*****************************************************************************/
void dtvd_tuner_com_dev_mcbsp_read_cancel(void)
{
    int ret;
    
    /* McBSP DMA chain 停止 */
    ret = omap2_mcbsp_recv_buffer_chain_cancel(D_DTVD_TUNER_MCBSP);
    if(ret != 0)
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_MCBSP_READ,
                                  DTVD_TUNER_COM_DEV,
                                  ret, 0, 0, 0, 0, 0 );
        return;
    }
    return;
}
#endif /* TUNER_MCBSP_ENABLE */

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_log                                         */
/*****************************************************************************/
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
)
{
    unsigned long func_no;
    DTVD_TUNER_COM_SYSERR_t err_log;

    dtvd_tuner_memset( &err_log, 0x00,
                       sizeof(DTVD_TUNER_COM_SYSERR_t),
                       sizeof(DTVD_TUNER_COM_SYSERR_t) );

    func_no = ( D_DTVD_TUNER_SYSERR_BLKID | kind );

    err_log.file_no = file_no;
    err_log.line = line;
    err_log.log_data[0] = data1;
    err_log.log_data[1] = data2;
    err_log.log_data[2] = data3;
    err_log.log_data[3] = data4;
    err_log.log_data[4] = data5;
    err_log.log_data[5] = data6;

    /* syserrをミドルに通知する */
    dtvd_tuner_com_dev_trans_msg_syserr(rank);

    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_romdata_debugout                            */
/*****************************************************************************/
#if 0 /* Arrietty 不揮発読出し処理不要の為、削除 2011/11/29 */
static void dtvd_tuner_com_dev_romdata_debugout( void )
{

    DTVD_DEBUG_MSG_INFO( "wait_tmcc             =%x\n"  ,   tdtvd_tuner_nonvola.wait_tmcc );
    DTVD_DEBUG_MSG_INFO( "cn_wait               =%x\n"  ,   tdtvd_tuner_nonvola.cn_wait );
    DTVD_DEBUG_MSG_INFO( "wait_ts               =%x\n"  ,   tdtvd_tuner_nonvola.wait_ts );
    DTVD_DEBUG_MSG_INFO( "cn_ave_count          =%x\n"  ,   tdtvd_tuner_nonvola.cn_ave_count );
    DTVD_DEBUG_MSG_INFO( "cn_cycle              =%x\n"  ,   tdtvd_tuner_nonvola.cn_cycle );
    DTVD_DEBUG_MSG_INFO( "cn_alpha              =%x\n"  ,   tdtvd_tuner_nonvola.cn_alpha );
    DTVD_DEBUG_MSG_INFO( "cn_beta               =%x\n"  ,   tdtvd_tuner_nonvola.cn_beta );
    DTVD_DEBUG_MSG_INFO( "cn_gamma              =%x\n"  ,   tdtvd_tuner_nonvola.cn_gamma );
    DTVD_DEBUG_MSG_INFO( "agc_wait              =%x\n"  ,   tdtvd_tuner_nonvola.agc_wait );
    DTVD_DEBUG_MSG_INFO( "agc_cycle             =%x\n"  ,   tdtvd_tuner_nonvola.agc_cycle );
    DTVD_DEBUG_MSG_INFO( "ber_wait              =%x\n"  ,   tdtvd_tuner_nonvola.ber_wait );
    DTVD_DEBUG_MSG_INFO( "ber_cycle             =%x\n"  ,   tdtvd_tuner_nonvola.ber_cycle );
    DTVD_DEBUG_MSG_INFO( "state_monitor_cycle   =%x\n"  ,   tdtvd_tuner_nonvola.state_monitor_cycle );
    DTVD_DEBUG_MSG_INFO( "wait_search_int       =%x\n"  ,   tdtvd_tuner_nonvola.wait_search_int );
    DTVD_DEBUG_MSG_INFO( "wait_mnt_sync         =%x\n"  ,   tdtvd_tuner_nonvola.wait_mnt_sync );
    DTVD_DEBUG_MSG_INFO( "omt_agc_thr           =%x\n"  ,   tdtvd_tuner_nonvola.omt_agc_thr );
    DTVD_DEBUG_MSG_INFO( "auto_n2e_ifagc_qp12   =%x\n"  ,   tdtvd_tuner_nonvola.auto_n2e_ifagc_qp12 );
    DTVD_DEBUG_MSG_INFO( "auto_n2e_ifagc_qp23   =%x\n"  ,   tdtvd_tuner_nonvola.auto_n2e_ifagc_qp23 );
    DTVD_DEBUG_MSG_INFO( "auto_n2e_ifagc_qam12  =%x\n"  ,   tdtvd_tuner_nonvola.auto_n2e_ifagc_qam12 );
    DTVD_DEBUG_MSG_INFO( "auto_e2n_ifagc_qp12   =%x\n"  ,   tdtvd_tuner_nonvola.auto_e2n_ifagc_qp12 );
    DTVD_DEBUG_MSG_INFO( "auto_e2n_ifagc_qp23   =%x\n"  ,   tdtvd_tuner_nonvola.auto_e2n_ifagc_qp23 );
    DTVD_DEBUG_MSG_INFO( "auto_e2n_ifagc_qam12  =%x\n"  ,   tdtvd_tuner_nonvola.auto_e2n_ifagc_qam12 );
/*$M #Rev.00.02 CHG-S 1511023 */
/*$M DTVD_DEBUG_MSG_INFO( "reserved0             =%x\n"  ,   tdtvd_tuner_nonvola.reserved0   );*/
    DTVD_DEBUG_MSG_INFO( "wait_refclk            =%x\n"  ,   tdtvd_tuner_nonvola.wait_refclk );
/*$M #Rev.00.02 CHG-E 1511023 */
/*$M #Rev.00.03 CHG-S 1511038 */
/*$M DTVD_DEBUG_MSG_INFO( "reserved1             =%x\n"  ,   tdtvd_tuner_nonvola.reserved1   );*/
    DTVD_DEBUG_MSG_INFO( "search_thres           =%x\n"  ,   tdtvd_tuner_nonvola.search_thres );
/*$M #Rev.00.03 CHG-E 1511038 */
/*<5210011>    DTVD_DEBUG_MSG_INFO( "reserved2             =%x\n"  ,   tdtvd_tuner_nonvola.reserved2   );*/
/*<5210011>    DTVD_DEBUG_MSG_INFO( "reserved3             =%x\n"  ,   tdtvd_tuner_nonvola.reserved3   );*/
    DTVD_DEBUG_MSG_INFO( "wait_lock1            =%x\n"  ,   tdtvd_tuner_nonvola.wait_lock1  );/*<5210011>*/
    DTVD_DEBUG_MSG_INFO( "wait_lock2            =%x\n"  ,   tdtvd_tuner_nonvola.wait_lock2  );/*<5210011>*/
    DTVD_DEBUG_MSG_INFO( "reserved4             =%x\n"  ,   tdtvd_tuner_nonvola.reserved4   );
    DTVD_DEBUG_MSG_INFO( "reserved5             =%x\n"  ,   tdtvd_tuner_nonvola.reserved5   );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk1_2_m          =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk1_2_m    );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk1_2_h          =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk1_2_h    );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk2_3_m          =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk2_3_m    );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk2_3_h          =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk2_3_h    );
    DTVD_DEBUG_MSG_INFO( "cn_16qam1_2_m         =%x\n"  ,   tdtvd_tuner_nonvola.cn_16qam1_2_m   );
    DTVD_DEBUG_MSG_INFO( "cn_16qam1_2_h         =%x\n"  ,   tdtvd_tuner_nonvola.cn_16qam1_2_h   );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk1_2_eco_m      =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk1_2_eco_m    );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk1_2_eco_h      =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk1_2_eco_h    );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk2_3_eco_m      =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk2_3_eco_m    );
    DTVD_DEBUG_MSG_INFO( "cn_qpsk2_3_eco_h      =%x\n"  ,   tdtvd_tuner_nonvola.cn_qpsk2_3_eco_h    );
    DTVD_DEBUG_MSG_INFO( "cn_16qam1_2_eco_m     =%x\n"  ,   tdtvd_tuner_nonvola.cn_16qam1_2_eco_m   );
    DTVD_DEBUG_MSG_INFO( "cn_16qam1_2_eco_h     =%x\n"  ,   tdtvd_tuner_nonvola.cn_16qam1_2_eco_h   );
    DTVD_DEBUG_MSG_INFO( "mode_on               =%x\n"  ,   tdtvd_tuner_nonvola.mode_on );
    DTVD_DEBUG_MSG_INFO( "tmcc_on               =%x\n"  ,   tdtvd_tuner_nonvola.tmcc_on );
    DTVD_DEBUG_MSG_INFO( "mode_dec_count        =%x\n"  ,   tdtvd_tuner_nonvola.mode_dec_count  );
    DTVD_DEBUG_MSG_INFO( "search_success_count  =%x\n"  ,   tdtvd_tuner_nonvola.search_success_count    );
    DTVD_DEBUG_MSG_INFO( "search_success_thr    =%x\n"  ,   tdtvd_tuner_nonvola.search_success_thr  );
    DTVD_DEBUG_MSG_INFO( "search_time           =%x\n"  ,   tdtvd_tuner_nonvola.search_time );
    DTVD_DEBUG_MSG_INFO( "scan_mode_dec_count   =%x\n"  ,   tdtvd_tuner_nonvola.scan_mode_dec_count );
    DTVD_DEBUG_MSG_INFO( "scan_search_success_count =%x\n", tdtvd_tuner_nonvola.scan_search_success_count   );
    DTVD_DEBUG_MSG_INFO( "reserved10            =%x\n"  ,   tdtvd_tuner_nonvola.reserved10  );
    DTVD_DEBUG_MSG_INFO( "reserved11            =%x\n"  ,   tdtvd_tuner_nonvola.reserved11  );
    DTVD_DEBUG_MSG_INFO( "reserved12            =%x\n"  ,   tdtvd_tuner_nonvola.reserved12  );
    DTVD_DEBUG_MSG_INFO( "reserved13            =%x\n"  ,   tdtvd_tuner_nonvola.reserved13  );
    DTVD_DEBUG_MSG_INFO( "reserved14            =%x\n"  ,   tdtvd_tuner_nonvola.reserved14  );
    DTVD_DEBUG_MSG_INFO( "reserved15            =%x\n"  ,   tdtvd_tuner_nonvola.reserved15  );
    DTVD_DEBUG_MSG_INFO( "reserved16            =%x\n"  ,   tdtvd_tuner_nonvola.reserved16  );
    DTVD_DEBUG_MSG_INFO( "reserved17            =%x\n"  ,   tdtvd_tuner_nonvola.reserved17  );
    DTVD_DEBUG_MSG_INFO( "auto_n2e_cn_qp12      =%x\n"  ,   tdtvd_tuner_nonvola.auto_n2e_cn_qp12    );
    DTVD_DEBUG_MSG_INFO( "auto_n2e_cn_qp23      =%x\n"  ,   tdtvd_tuner_nonvola.auto_n2e_cn_qp23    );
    DTVD_DEBUG_MSG_INFO( "auto_n2e_cn_qam12     =%x\n"  ,   tdtvd_tuner_nonvola.auto_n2e_cn_qam12   );
    DTVD_DEBUG_MSG_INFO( "auto_e2n_cn_qp12      =%x\n"  ,   tdtvd_tuner_nonvola.auto_e2n_cn_qp12    );
    DTVD_DEBUG_MSG_INFO( "auto_e2n_cn_qp23      =%x\n"  ,   tdtvd_tuner_nonvola.auto_e2n_cn_qp23    );
    DTVD_DEBUG_MSG_INFO( "auto_e2n_cn_qam12     =%x\n"  ,   tdtvd_tuner_nonvola.auto_e2n_cn_qam12   );
/* DTVD_DEBUG_MSG_INFO( "reserved20            =%x\n"  ,   tdtvd_tuner_nonvola.reserved20  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved21            =%x\n"  ,   tdtvd_tuner_nonvola.reserved21  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved22            =%x\n"  ,   tdtvd_tuner_nonvola.reserved22  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved23            =%x\n"  ,   tdtvd_tuner_nonvola.reserved23  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved24            =%x\n"  ,   tdtvd_tuner_nonvola.reserved24  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved25            =%x\n"  ,   tdtvd_tuner_nonvola.reserved25  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved26            =%x\n"  ,   tdtvd_tuner_nonvola.reserved26  ); */
/*    DTVD_DEBUG_MSG_INFO( "reserved27            =%x\n"  ,   tdtvd_tuner_nonvola.reserved27  ); */

    return;
}
#endif /* Arrietty 不揮発読出し処理不要の為、削除 2011/11/29 */

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_trans_msg_syserr                            */
/*****************************************************************************/
static void dtvd_tuner_com_dev_trans_msg_syserr
(
    unsigned char   rank       /* アラームランク */
)
{
    DTVD_TUNER_MSG_t    msg;
    unsigned int        len = sizeof(DTVD_TUNER_MSG_t);

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* 通知データ初期化 */
    dtvd_tuner_memset( &msg, 0x00, len, len );

    if(tdtvd_tuner_core.pipe_handle != D_DTVD_TUNER_NULL)
    {
        /* メッセージID */
        msg.header.msg_id = D_DTVD_TUNER_MSGID_DEVERR;
        /* 受信側ブロックID */
        msg.header.receiver_id = D_DTVD_TUNER_BLOCK_ID_DMM;
        /* 送信側ブロックID */
        msg.header.sender_id = D_DTVD_TUNER_BLOCK_ID_TUNERDD;
        /* エラー種別 */ 
		msg.data.deverr.error_type = rank;

        /* メッセージ送信 */
        dtvd_tuner_com_dev_ddsync_write( tdtvd_tuner_core.pipe_handle,
                                     ( void * )&msg,
                                     len );
    }
    DTVD_DEBUG_MSG_EXIT();

    return;
}

/* Add Start：(BugLister ID52) 2011/09/15 */
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_pipe_open                                   */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_pipe_open
(
    unsigned char* pipename      /* PIPE ファイルパス名 */
)
{
/* Mod Start：(BugLister ID60) 2011/10/04 */
	struct file* file = NULL;

	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	/* 通知用名前付きPIPEオープン */
	tdtvd_tuner_core.pipe_handle = D_DTVD_TUNER_NULL;
	file = filp_open( ( const char * )pipename, O_WRONLY | O_NONBLOCK, 0 );
	if(IS_ERR(file))
	{
		DTVD_DEBUG_MSG_INFO("* Error: PIPE Open failed.\n");
		return D_DTVD_TUNER_NG;
	}
	tdtvd_tuner_core.pipe_handle = file;

    DTVD_DEBUG_MSG_EXIT();

	return D_DTVD_TUNER_OK;
/* Mod End  ：(BugLister ID60) 2011/10/04 */
}


/* Add Start：(BugLister ID60) 2011/10/04 */
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_pipe_close                                  */
/*****************************************************************************/
void dtvd_tuner_com_dev_pipe_close( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	/* PIPEクローズ */
	if(D_DTVD_TUNER_NULL != tdtvd_tuner_core.pipe_handle)
	{
		filp_close( tdtvd_tuner_core.pipe_handle, 0 );
		tdtvd_tuner_core.pipe_handle = D_DTVD_TUNER_NULL;
		DTVD_DEBUG_MSG_INFO("* PIPE Closed.\n");
	}
	
    DTVD_DEBUG_MSG_EXIT();

	return;
}
/* Add End  ：(BugLister ID60) 2011/10/04 */

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_int_workqueue                               */
/*****************************************************************************/
static void dtvd_tuner_com_dev_int_workqueue
(
    struct work_struct* work
)
{
	int ret = D_DTVD_TUNER_NG;
	DTVD_TUNER_COM_I2C_DATA_t i2c_wdata;				/* I2Cライトデータ */
	unsigned int search_result = D_DTVD_TUNER_FALSE;	/* サーチ結果 */
	
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	/* 1. 選局結果 割込みフラグリセット・マスク */
	/* 1-1. 0xEEに 0x13をライト */
	i2c_wdata.adr = D_DTVD_TUNER_REG_SEARCH_IRQCTL;
	i2c_wdata.data = 0x13;
	ret = dtvd_tuner_com_dev_i2c_write(D_DTVD_TUNER_REG_NO1, &i2c_wdata);
	if(D_DTVD_TUNER_OK != ret)
	{
		DTVD_DEBUG_MSG_INFO("* Error: I2C Write failed.\n");
		/* ハード異常通知 */
		dtvd_tuner_core_trans_msg_deverr();
		return;
	}
	/* 1-2. 0xEEに 0x03をライト */
	i2c_wdata.data = 0x03;
	ret = dtvd_tuner_com_dev_i2c_write(D_DTVD_TUNER_REG_NO1, &i2c_wdata);
	if(D_DTVD_TUNER_OK != ret)
	{
		DTVD_DEBUG_MSG_INFO("* Error: I2C Write failed.\n");
		/* ハード異常通知 */
		dtvd_tuner_core_trans_msg_deverr();
		return;
	}
	
	/* Add Start：(BugLister ID76) 2011/10/18 */
	/* 2. チャネルサーチ終了 */
	/* 0xE6に 0x20をライト */
	i2c_wdata.adr = D_DTVD_TUNER_REG_SEARCH_CTRL;
	i2c_wdata.data = 0x20;	/* Mod：(BugLister ID86) 2011/11/01 */
	ret = dtvd_tuner_com_dev_i2c_write(D_DTVD_TUNER_REG_NO1, &i2c_wdata);
	if(D_DTVD_TUNER_OK != ret)
	{
		DTVD_DEBUG_MSG_INFO("* Error: I2C Write failed.\n");
		/* ハード異常通知 */
		dtvd_tuner_core_trans_msg_deverr();
		return;
	}
	/* Add End  ：(BugLister ID76) 2011/10/18 */

	/* 3. チャネルサーチ結果読出し */
	ret = dtvd_tuner_tuner_com_search_result_read(&search_result);
	if(D_DTVD_TUNER_OK != ret)
	{
		DTVD_DEBUG_MSG_INFO("* Error: I2C Write failed.\n");
		/* ハード異常通知 */
		dtvd_tuner_core_trans_msg_deverr();
		return;
	}
	
	/* 4. 選局結果通知 */
	if(D_DTVD_TUNER_TRUE == search_result)	/* チャネル有 */
	{
		/* 選局成功通知 */
		dtvd_tuner_core_trans_msg_tune_ok(0);
	}
	else									/* チャネル無 */
	{
		/* 選局失敗通知 */
		dtvd_tuner_core_trans_msg_tune_ng(D_DTVD_TUNER_TUNE_NG_NONE);
	}
	
	DTVD_DEBUG_MSG_EXIT();
    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_tuner_com_search_resreg_set                         */
/*****************************************************************************/
signed int dtvd_tuner_tuner_com_search_resreg_set( void )
{
    signed   int                ret;
    unsigned long               length;
    DTVD_TUNER_COM_I2C_DATA_t   dtvd_tuner_com_i2c_data[1];

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );	/* Add：(BugLister ID52) 2011/09/15 */

    dtvd_tuner_com_i2c_data[ 0 ].adr  = D_DTVD_TUNER_REG_SEARCH_CHANNEL;
    dtvd_tuner_com_i2c_data[ 0 ].data = 0x0D;

    length = 1;
    ret = dtvd_tuner_com_dev_i2c_write(
        length,
        dtvd_tuner_com_i2c_data
    );
    /* 書込み失敗 */
    if( ret != D_DTVD_TUNER_OK )
    {
        return D_DTVD_TUNER_NG;
    }

    DTVD_DEBUG_MSG_EXIT();	/* Add：(BugLister ID52) 2011/09/15 */
    return D_DTVD_TUNER_OK;

}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_tuner_com_search_result_read                        */
/*****************************************************************************/
signed int dtvd_tuner_tuner_com_search_result_read( unsigned int *result )
{
    signed   int                ret;
    unsigned long               length;
    DTVD_TUNER_COM_I2C_DATA_t   dtvd_tuner_com_i2c_data;
    unsigned char               addr;
    unsigned char               data;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );	/* Add：(BugLister ID52) 2011/09/15 */

	/* パラメタチェック */
    if ( result == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_TUNER_COM,
                                  0, 0, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    *result = D_DTVD_TUNER_FALSE;

    /* サブレジスタ アドレス設定 */
    length = D_DTVD_TUNER_REG_NO1;
    dtvd_tuner_com_i2c_data.adr  = D_DTVD_TUNER_REG_SEARCH_SUBA;
    dtvd_tuner_com_i2c_data.data = D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL1;
    ret = dtvd_tuner_com_dev_i2c_write( length,                      /* データサイズ */
                                        &dtvd_tuner_com_i2c_data );  /* I2C書込み用データ */
    if( ret != D_DTVD_TUNER_OK )
    {
        return D_DTVD_TUNER_NG;
    }

    /* 読み出し */
    length = D_DTVD_TUNER_REG_NO1;
    addr = D_DTVD_TUNER_REG_SEARCH_SUBD;
    data = 0x00;
    ret = dtvd_tuner_com_dev_i2c_read( length, &addr, &data );
    if( ret != D_DTVD_TUNER_OK )
    {
        return D_DTVD_TUNER_NG;
    }

    /* 結果判定 */
    if( ( data & D_DTVD_TUNER_SEARCH_CHECK_MASK ) == D_DTVD_TUNER_SEARCH_CHECK_MASK )
    {
        *result = D_DTVD_TUNER_TRUE;
    }
    else
    {
        *result = D_DTVD_TUNER_FALSE;
    }

    DTVD_DEBUG_MSG_EXIT();	/* Add：(BugLister ID52) 2011/09/15 */
    return D_DTVD_TUNER_OK;
}
/* Add End  ：(BugLister ID52) 2011/09/15 */
/* Add Start：(BugLister ID86) 2011/11/07 */
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_mcbspcfg_set                                */
/*****************************************************************************/
void dtvd_tuner_com_dev_mcbspcfg_set( unsigned char mode )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );
	
#ifdef TUNER_MCBSP_ENABLE
	TS_RECV_MODE = mode;	/* Add：(MMシステム対応) 2012/03/14 */

	if( (D_DTVD_TUNER_MCBSPCFG_RS_ENABLE == mode) || (D_DTVD_TUNER_MCBSPCFG_RS_ENABLE_RESET_DISABLE == mode) )	/* Mod：(MMシステム対応) 2012/03/14 */
	{
		com_tuner_mcbsp_config = dtvd_tuner_mcbsp_config;
		D_DTVD_TUNER_TSP_LENGTH = D_DTVD_TUNER_TSP_FRAME_ENABLE_RS * D_DTVD_TUNER_TSP_FRAMENUM_ENABLE_RS;	/* Add：(BugLister ID93) 2011/12/08 */
		DTVD_DEBUG_MSG_INFO("* MCBSP CFG(RS Enable) is set. rcr1 reg = %d  TSP Length = %d\n", com_tuner_mcbsp_config.rcr1, D_DTVD_TUNER_TSP_LENGTH);	/* Mod：(BugLister ID93) 2011/12/08 */
	}
	else
	{
		com_tuner_mcbsp_config = mm_tuner_mcbsp_config;
		D_DTVD_TUNER_TSP_LENGTH = D_DTVD_TUNER_TSP_FRAME_DISABLE_RS * D_DTVD_TUNER_TSP_FRAMENUM_DISABLE_RS;	/* Add：(BugLister ID93) 2011/12/08 */
		DTVD_DEBUG_MSG_INFO("* MCBSP CFG(RS Disable) is set. rcr1 reg = %d TSP Length = %d\n", com_tuner_mcbsp_config.rcr1, D_DTVD_TUNER_TSP_LENGTH);	/* Mod：(BugLister ID93) 2011/12/08 */
	}
	
#endif /* TUNER_MCBSP_ENABLE */
#ifdef TUNER_TSIF_ENABLE
	if( g_tuner_user == D_DTVD_TUNER_USER_MM ){    /* MM */
		D_DTVD_TUNER_TSP_LENGTH = D_DTVD_TUNER_TSP_FRAME * D_DTVD_TUNER_TSP_PAKETS_NUM_MM;
		DTVD_DEBUG_MSG_INFO("* TSIF BUFFER CONFIG MM is set. TSP Length = %d\n", D_DTVD_TUNER_TSP_LENGTH);
	} else {				/* DTV */
		D_DTVD_TUNER_TSP_LENGTH = D_DTVD_TUNER_TSP_FRAME * D_DTVD_TUNER_TSP_PAKETS_NUM_DTV;
		DTVD_DEBUG_MSG_INFO("* TSIF BUFFER CONFIG DTV is set. TSP Length = %d\n", D_DTVD_TUNER_TSP_LENGTH);
	}
#endif /* TUNER_TSIF_ENABLE */
    DTVD_DEBUG_MSG_EXIT();

	return;
}
/* Add End  ：(BugLister ID86) 2011/11/07 */
#ifdef TUNER_TSIF_ENABLE
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_tsif_start                                  */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_tsif_start( void )
{
	int ret;
	int instance;
	unsigned int pakets_num;
	unsigned int ringbuffer_num;

	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	/* TSIF initial setting */
	/* TSIF get active id*/
	instance = tsif_get_active();
	if (instance >= 0)
	{
		/* TSIF attach */
		ret = dtvd_tuner_com_dev_tsif_init_one(&the_devices[0], instance);
		if( ret ){
			DTVD_DEBUG_MSG_INFO("* Error: TSIF attach NG.  ret = %d\n" , ret);
			return D_DTVD_TUNER_NG;
		}
	}
	else
	{
		DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
				DTVD_TUNER_COM_DEV,
				instance, 0, 0, 0, 0, 0 );
		DTVD_DEBUG_MSG_INFO("* Error: TSIF not active instance = %d.\n" , instance);
		return D_DTVD_TUNER_NG;
	}
	if (!the_devices->cookie)  /* not bound yet */
	{
		DTVD_DEBUG_MSG_INFO("* Error: TSIF not bound yet.\n");
		return D_DTVD_TUNER_NG;
	}

	/* TSIF time out disable*/
	ret = tsif_set_time_limit( the_devices->cookie, D_DTVD_TUNER_TIMEOUT_DISABLE );
	if ( ret ){
		DTVD_DEBUG_MSG_INFO("* Error: TSIF time out setting NG.  ret = %d\n" , ret);
		tsif_detach(the_devices->cookie);                                      /* 20120605 失敗時に解放処理を追加 */
		return D_DTVD_TUNER_NG;
	}

	if( g_tuner_user == D_DTVD_TUNER_USER_MM ){	/* MM */
		pakets_num = D_DTVD_TUNER_TSP_PAKETS_NUM_MM;
		ringbuffer_num = D_DTVD_TUNER_TSP_TSIF_RINGBUFFER_NUM_MM;
	} else {				/* DTV */
		pakets_num = D_DTVD_TUNER_TSP_PAKETS_NUM_DTV;
		ringbuffer_num = D_DTVD_TUNER_TSP_TSIF_RINGBUFFER_NUM_DTV;
	}

	/* TSIF buffer setting */
	ret = tsif_set_buf_config( the_devices->cookie,
				pakets_num,
				ringbuffer_num );
	if ( ret ){
		DTVD_DEBUG_MSG_INFO("* Error: TSIF buffer setting NG.  ret = %d\n" , ret);
		tsif_detach(the_devices->cookie);                                      /* 20120605 失敗時に解放処理を追加 */
		return D_DTVD_TUNER_NG;
	}

	/* TSIF set mode*/
	ret = tsif_set_mode( the_devices->cookie, D_DTVD_TUNER_SET_MODE_2 );
	if ( ret ){
		DTVD_DEBUG_MSG_INFO("* Error: TSIF mode setting NG.  ret = %d\n" , ret);
		tsif_detach(the_devices->cookie);                                      /* 20120605 失敗時に解放処理を追加 */
		return D_DTVD_TUNER_NG;
	}

	/* TSIF start */
	ret = tsif_start( the_devices->cookie );
	if( ret ){
		DTVD_DEBUG_MSG_INFO("* Error: TSIF start NG.  ret = %d\n" , ret);
		tsif_detach(the_devices->cookie);                                      /* 20120605 失敗時に解放処理を追加 */
		return D_DTVD_TUNER_NG;
	}
	tsif_get_info(the_devices->cookie, &the_devices->data_buffer,
			&the_devices->buf_size_packets);
	the_devices->rptr = 0;

	DTVD_DEBUG_MSG_EXIT();

	return D_DTVD_TUNER_OK;
}
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_tsif_stop                                   */
/*****************************************************************************/
void dtvd_tuner_com_dev_tsif_stop( void )
{
	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	tsif_stop( the_devices->cookie );
	tsif_detach( the_devices->cookie );

	DTVD_DEBUG_MSG_EXIT();
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_tsif_read_start                             */
/*****************************************************************************/
signed int dtvd_tuner_com_dev_tsif_read (void __user *buffer , size_t read_len )
{
	int avail = 0;
	int wi;
	int ri_check = 0;
	int pakets_num;

	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* パラメータチェック */
    if( buffer == D_DTVD_TUNER_NULL )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_DEV,
                                  0, 0, 0, 0, 0, 0 );
	DTVD_DEBUG_MSG_INFO("* Error: buffer address is null.\n");
        return D_DTVD_TUNER_NG;
    }
	tsif_get_state(the_devices->cookie, &the_devices->ri, &the_devices->wi,
			&the_devices->state);
	DTVD_DEBUG_MSG_INFO("* TSIF datainfo before  ri = %d  wi = %d  rptr = %d  state = %d\n  buf_size_packets = %d\n" ,
	the_devices->ri,
	the_devices->wi,
	the_devices->rptr,
	the_devices->state,
	the_devices->buf_size_packets );

	if( g_tuner_user == D_DTVD_TUNER_USER_MM ){     /* MM */
		pakets_num = D_DTVD_TUNER_TSP_PAKETS_NUM_MM;
        } else {                                        /* DTV */
		pakets_num = D_DTVD_TUNER_TSP_PAKETS_NUM_DTV;
        }

	ri_check = the_devices->ri % pakets_num;
	if( ri_check != 0 ){
		DTVD_DEBUG_MSG_INFO("* ri is shifted.  ri = %d \n", the_devices->ri );
		the_devices->ri = the_devices->ri - ri_check;
	}

	/* consistency check */
	if (the_devices->ri != (the_devices->rptr / TSIF_PKT_SIZE)) {
		dev_err(the_devices->dev,
			"%s: inconsistent read pointers: ri %d rptr %d\n",
			__func__, the_devices->ri, the_devices->rptr);
		the_devices->rptr = the_devices->ri * TSIF_PKT_SIZE;
	}
	/* 20120611 ri == wi if no data [midified forced return 0] */
	if (the_devices->ri == the_devices->wi)
	{
		DTVD_DEBUG_MSG_INFO("* read no data.\n");                              /* ログが邪魔なら消しても良い */
		return 0;                                                              /* すぐに0を返す */
	}

	/* contiguous chunk last up to wi or end of buffer */
	wi = (the_devices->wi > the_devices->ri) ? the_devices->wi : the_devices->buf_size_packets;
	avail = min((wi * TSIF_PKT_SIZE) - the_devices->rptr, read_len );
	if (copy_to_user(buffer, the_devices->data_buffer + the_devices->rptr, avail))
	{
		DTVD_DEBUG_MSG_INFO("* Error: copy_to_user.  dst_buffer = %p  src_buf = %p rptr = %d avail = %d\n", buffer, the_devices->data_buffer, the_devices->rptr, avail);
		return -EFAULT;
	}
	the_devices->rptr = ((the_devices->rptr + avail) % (TSIF_PKT_SIZE * the_devices->buf_size_packets));
	the_devices->ri = ( the_devices->rptr / TSIF_PKT_SIZE );
	tsif_reclaim_packets(the_devices->cookie, the_devices->ri);

	DTVD_DEBUG_MSG_INFO("* TSIF datainfo after  ri = %d  wi = %d  rptr = %d  state = %d\n  buf_size_packets = %d\n" ,
	the_devices->ri,
	the_devices->wi,
	the_devices->rptr,
	the_devices->state,
	the_devices->buf_size_packets );

	DTVD_DEBUG_MSG_EXIT();

	return avail;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_tsif_notify                                 */
/*****************************************************************************/
static void dtvd_tuner_com_dev_tsif_notify(void *data)
{
	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	if(data == NULL){
		DTVD_DEBUG_MSG_INFO("tsif notify data is null pointer. Don't update state and wake up for read event. \n");
		return;
	}

	memcpy( the_devices , data , sizeof(struct tsif_chrdev));
	tsif_get_state(the_devices->cookie, &the_devices->ri, &the_devices->wi,
			&the_devices->state);

	DTVD_DEBUG_MSG_EXIT();

	return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_tsif_init_one                               */
/*****************************************************************************/
static int dtvd_tuner_com_dev_tsif_init_one(struct tsif_chrdev *the_dev, int index)
{
	int rc;

	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	init_waitqueue_head(&the_dev->wq_read);

	the_dev->cookie = tsif_attach(index, dtvd_tuner_com_dev_tsif_notify, the_dev);
	if (IS_ERR(the_dev->cookie)) {
		rc = PTR_ERR(the_dev->cookie);
		DTVD_DEBUG_MSG_INFO("tsif_attach failed: %d\n", rc);
		return rc;
	}
	/* now data buffer is not allocated yet */
	tsif_get_info(the_dev->cookie, &the_dev->data_buffer, NULL);

	DTVD_DEBUG_MSG_EXIT();

	return 0;
}
/*****************************************************************************/
/* MODULE   : dtvd_tuner_com_dev_tsif_user_setting                           */
/* param[in]: user   ワンセグかMM                                            */
/*****************************************************************************/
void dtvd_tuner_com_dev_tsif_user_setting( unsigned int user )
{
	DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	g_tuner_user = user;	/* user_setting */

	DTVD_DEBUG_MSG_EXIT();
}
#endif /* TUNER_TSIF_ENABLE */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
