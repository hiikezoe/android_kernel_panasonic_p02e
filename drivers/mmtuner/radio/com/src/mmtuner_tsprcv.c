#include "dtvd_tuner_com.h"
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#ifdef TUNER_MCBSP_ENABLE
#include <plat/mcbsp.h>
#endif /* TUNER_MCBSP_ENABLE */
#include <linux/sched.h> /* Add：(MMシステム対応) 2012/03/14 */
/*****************************************************************************/
/* ローカル関数プロトタイプ宣言                                              */
/*****************************************************************************/
#ifdef TUNER_MCBSP_ENABLE
static  int dtvd_tuner_tsprcv_recv_thread_main( void *param );   /* TSP受信スレッド */
#endif /* TUNER_MCBSP_ENABLE */

/*****************************************************************************/
/* ローカルマクロ宣言                                                        */
/*****************************************************************************/
#define NEXTPTR(cnt) \
         ( (((tdtvd_tuner_tsp_ringbuffer.write_idx+(cnt))%D_DTVD_TUNER_TSP_RINGBUFFER_MAX) == tdtvd_tuner_tsp_ringbuffer.read_idx)?\
          0:(&tdtvd_tuner_tsp_ringbuffer.dma[(tdtvd_tuner_tsp_ringbuffer.write_idx+(cnt))%D_DTVD_TUNER_TSP_RINGBUFFER_MAX]))

#ifdef TUNER_MCBSP_ENABLE
/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_check_and_recovery                           */
/*****************************************************************************/
static int dtvd_tuner_tsprcv_check_and_recovery(char frameSyncData)
{
    unsigned long           flags;
    int retval = D_DTVD_TUNER_FALSE;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

	if(D_DTVD_TUNER_MCBSPCFG_RS_ENABLE_RESET_DISABLE != TS_RECV_MODE)	/* Add：(MMシステム対応) 2012/03/14 */
	{
		DTVD_DEBUG_MSG_INFO("Checking frameSyncData is running\n");
    	/* 同期ずれ発生? */
    	if (frameSyncData != 0x47)
    	{
        	retval = D_DTVD_TUNER_TRUE;

	        spin_lock_irqsave(&tdtvd_tuner_tsp_ringbuffer.lock, flags);
    	    tdtvd_tuner_tsp_ringbuffer.write_idx = 0;
       	 	tdtvd_tuner_tsp_ringbuffer.read_idx = 0;
       		spin_unlock_irqrestore(&tdtvd_tuner_tsp_ringbuffer.lock, flags);

	        dtvd_tuner_com_dev_mcbsp_stop();
    	    dtvd_tuner_com_dev_mcbsp_start();
    	}
	}

	DTVD_DEBUG_MSG_EXIT();

    return retval;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_create_recv_thread                           */
/*****************************************************************************/
signed int dtvd_tuner_tsprcv_create_recv_thread( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* TSP受信スレッドの生成 */
    tdtvd_tuner_tsprcv_thread = kthread_create( dtvd_tuner_tsprcv_recv_thread_main,
                                               (void *)&tdtvd_tuner_tsprcv_ctrl,
                                               D_DTVD_TUNER_TSPRCV_THREAD);
    if( IS_ERR(tdtvd_tuner_tsprcv_thread) )
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_TSPRCV,
                                  PTR_ERR(tdtvd_tuner_tsprcv_thread), 0, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

    /* TSP受信スレッドの開始 */
    wake_up_process(tdtvd_tuner_tsprcv_thread);

    DTVD_DEBUG_MSG_EXIT();

    return D_DTVD_TUNER_OK;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_stop_recv_thread                             */
/*****************************************************************************/
void  dtvd_tuner_tsprcv_stop_recv_thread( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* TS受信スレッドの終了依頼と終了待ち合わせ */
    kthread_stop( tdtvd_tuner_tsprcv_thread );

    DTVD_DEBUG_MSG_EXIT();

    return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_recv_thread_main                             */
/*****************************************************************************/
static  int dtvd_tuner_tsprcv_recv_thread_main
(
    void *param     /* パラメータ(TSP受信スレッド管理情報アドレス) */
)
{
    DTVD_TUNER_TSPRCV_CTRL_t    *ctrl ;     /* パラメータ情報の保存領域 */
    DTVD_TUNER_TSP_DMA_t        *dma ;      /* TSP受信DMA情報:受信用DMA情報     */
    DTVD_TUNER_TSP_DMA_t        *dma_next ; /* TSP受信DMA情報:受信用DMA情報     */
    DTVD_TUNER_TSP_DMA_t        *dma_next2; /* TSP受信DMA情報:受信用DMA情報     */
    DTVD_TUNER_TSP_DMA_t        *dma_next3; /* TSP受信DMA情報:受信用DMA情報     *//* Add：(MMシステム対応) 2012/03/14 */
    DTVD_TUNER_TSP_DMA_t        *dma_next4; /* TSP受信DMA情報:受信用DMA情報     *//* Add：(MMシステム対応) 2012/03/14 */
    DTVD_TUNER_TSP_DMA_t        *dma_next5; /* TSP受信DMA情報:受信用DMA情報     *//* Add：(MMシステム対応) 2012/03/14 */

    dma_addr_t                  dma_handle[5];		/* Mod：(MMシステム対応) 2012/03/14 */
    unsigned int                rcvlength[5];		/* Mod：(MMシステム対応) 2012/03/14 */

    int ret;


    struct sched_param tparam = { .sched_priority= 1}; /* Add：(MMシステム対応) 2012/03/14 */
    sched_setscheduler(current,SCHED_FIFO,&tparam);    /* Add：(MMシステム対応) 2012/03/14 */
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* NULLチェック */
    if( param == D_DTVD_TUNER_NULL )
    {
        /* Syserr */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_TSPRCV,
                                  0, 0, 0, 0, 0, 0 );
        return -1;
    }

    for (;;)
    {
        /* パラメータ情報の保存 */
        ctrl = (DTVD_TUNER_TSPRCV_CTRL_t*)param;
        
        /* パラメータ情報の保存 */
        dma = dtvd_tuner_tsprcv_write_dma_from_ringbuffer( 0 );
        dma_next = NEXTPTR(1);
        dma_next2 = NEXTPTR(2);	/* Add：(MMシステム対応) 2012/03/14 */
        dma_next3 = NEXTPTR(3);	/* Add：(MMシステム対応) 2012/03/14 */
        dma_next4 = NEXTPTR(4);	/* Add：(MMシステム対応) 2012/03/14 */
    	

        /* スレッド状態を動作中(初回受信待ち)に変更 */
        ctrl->mode = D_DTVD_TUNER_TSPRCV_MODE_RECV ;
        ctrl->tspcount = 0;
        ctrl->overcount = 0;

        DTVD_DEBUG_MSG_INFO( "***tsp rcv start*** TSP Length = %d\n", D_DTVD_TUNER_TSP_LENGTH );		/* Mod：(BugLister ID93) 2011/12/08 */

        /* (i)McBSP受信開始                         */
        /*  受信用DMA情報のDMA情報.使用中フラグON   */
        dma->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;            /* 使用中       */
        dma_next->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;       /* 使用中       */
        dma_next2->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;      /* 使用中       *//* Add：(MMシステム対応) 2012/03/14 */
        dma_next3->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;      /* 使用中       *//* Add：(MMシステム対応) 2012/03/14 */
        dma_next4->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;      /* 使用中       *//* Add：(MMシステム対応) 2012/03/14 */

        /*  McBSP DMA受信開始    */
        dma_handle[0] = dma->dma_handle;
        dma_handle[1] = dma_next->dma_handle;
        dma_handle[2] = dma_next2->dma_handle;	/* Add：(MMシステム対応) 2012/03/14 */
        dma_handle[3] = dma_next3->dma_handle;	/* Add：(MMシステム対応) 2012/03/14 */
        dma_handle[4] = dma_next4->dma_handle;	/* Add：(MMシステム対応) 2012/03/14 */

        rcvlength[0] = D_DTVD_TUNER_TSP_LENGTH;  /*初回は1フレーム分*/
        rcvlength[1] = D_DTVD_TUNER_TSP_LENGTH;
        rcvlength[2] = D_DTVD_TUNER_TSP_LENGTH;	/* Add：(MMシステム対応) 2012/03/14 */
        rcvlength[3] = D_DTVD_TUNER_TSP_LENGTH;	/* Add：(MMシステム対応) 2012/03/14 */
        rcvlength[4] = D_DTVD_TUNER_TSP_LENGTH;	/* Add：(MMシステム対応) 2012/03/14 */
        ret = dtvd_tuner_com_dev_mcbsp_read_start(dma_handle, rcvlength);

        /* 受信処理(以下の処理を受信スレッド終了まで繰り返す） */
        for(;;)
        {
            if(ret != D_DTVD_TUNER_OK)
            {
                return -1;
            }

            /*  (ii)終了判定                            */
            if(ctrl->reqstop)
            {
                /* スレッド終了要求の有    */
                break;      /* 正常終了     */
            }
            if((ctrl->tspcount % 100)==0){
            DTVD_DEBUG_MSG_INFO( "***tsp rcv***\n" );
            }
            ctrl->tspcount++;       /* (DEBUG用)                            */

            /*  (iii)リングバッファ更新                 */
            if(dma_next4 != D_DTVD_TUNER_NULL)	/* Mod：(MMシステム対応) 2012/03/14 */
            {
                /* 受信用DMA情報のDMA情報.使用中フラグOFF   */
                dma->flag = D_DTVD_TUNER_TSP_DMA_FLAG_OFF;       /* 未使用       */

                dma_next5 = NEXTPTR(5);					/* Mod：(MMシステム対応) 2012/03/14 */
                if( dma_next5 != D_DTVD_TUNER_NULL )	/* Mod：(MMシステム対応) 2012/03/14 */
                {
                    /* リングバッファフル以外の場合（正常） */
                    dtvd_tuner_tsprcv_write_dma_from_ringbuffer( 1 );

                    /* フレーム同期チェック */
                    if (dtvd_tuner_tsprcv_check_and_recovery(((char*)dma->vptr)[0]) != D_DTVD_TUNER_FALSE)
                    {
                        break;
                    }

                    dma = dma_next;         /* 受信用DMA情報 = 次回受信用DMA情報    */
                    dma_next = dma_next2;
                    dma_next2 = dma_next3;		/* Add：(MMシステム対応) 2012/03/14 */
                    dma_next3 = dma_next4;		/* Add：(MMシステム対応) 2012/03/14 */
                    dma_next4 = dma_next5;		/* Add：(MMシステム対応) 2012/03/14 */
                    dma_next4->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;       /* 使用中       */	/* Mod：(MMシステム対応) 2012/03/14 */
                    ret = dtvd_tuner_com_dev_mcbsp_read(dma_next4->dma_handle,					/* Mod：(MMシステム対応) 2012/03/14 */
                                                        D_DTVD_TUNER_TSP_LENGTH);
                }
                else
                {
                    /* リングバッファフル発生の場合 */
                    dtvd_tuner_tsprcv_write_dma_from_ringbuffer( 1 );
                    
                    ctrl->overcount++;      /* (DEBUG用)                            */
                    DTVD_DEBUG_MSG_INFO( "***tsp rcv overflow(N:%ld)***\n", ctrl->overcount );
                    //dtvd_tuner_tsprcv_trans_msg_overflow();  /* ミドルへのイベント送信（リングバッファ溢れ通知）する。 *//* DEL ：(BugLister ID52) 2011/09/15 */

                    /* フレーム同期チェック */
                    if (dtvd_tuner_tsprcv_check_and_recovery(((char*)dma->vptr)[0]) != D_DTVD_TUNER_FALSE)
                    {
                        break;
                    }

                    /* 同一DMA情報をchainする */
                    dma = dma_next;
                    dma_next = dma_next2;			/* Mod：(MMシステム対応) 2012/03/14 */
                    dma_next2 = dma_next3;			/* Add：(MMシステム対応) 2012/03/14 */
                    dma_next3 = dma_next4;			/* Add：(MMシステム対応) 2012/03/14 */
                    dma_next4 = D_DTVD_TUNER_NULL;	/* Add：(MMシステム対応) 2012/03/14 */
                    ret = dtvd_tuner_com_dev_mcbsp_read(dma_next3->dma_handle,	/* Mod：(MMシステム対応) 2012/03/14 */
                                                        D_DTVD_TUNER_TSP_LENGTH);
                }

            }
            else
            {
                /* リングバッファフル発生中の場合 */
                dma_next4 = NEXTPTR(4);					/* Mod：(MMシステム対応) 2012/03/14 */
                if( dma_next4 != D_DTVD_TUNER_NULL)		/* Mod：(MMシステム対応) 2012/03/14 */
                {
                    /*リングバッファフル状態からの復旧 */
                    dma_next4->flag = D_DTVD_TUNER_TSP_DMA_FLAG_ON;       /* 使用中       */	/* Mod：(MMシステム対応) 2012/03/14 */
                    ret = dtvd_tuner_com_dev_mcbsp_read(dma_next4->dma_handle,					/* Mod：(MMシステム対応) 2012/03/14 */
                                                        D_DTVD_TUNER_TSP_LENGTH);
     
                    /* フレーム同期チェック */
                    if (dtvd_tuner_tsprcv_check_and_recovery(((char*)dma->vptr)[0]) != D_DTVD_TUNER_FALSE)
                    {
                        break;
                    }

                }else{
                    /* リングバッファフル継続中の場合 */
                    ctrl->overcount++;      /* (DEBUG用)                            */
                    if((ctrl->overcount%100) == 0){
                    DTVD_DEBUG_MSG_INFO( "***tsp rcv overflow(C:%ld)***\n", ctrl->overcount );
                    }
                    /* 同一DMA情報をchainする */
                    ret = dtvd_tuner_com_dev_mcbsp_read(dma_next3->dma_handle,	/* Mod：(MMシステム対応) 2012/03/14 */
                                                        D_DTVD_TUNER_TSP_LENGTH);
     
                    /* フレーム同期チェック */
                    if (dtvd_tuner_tsprcv_check_and_recovery(((char*)dma->vptr)[0]) != D_DTVD_TUNER_FALSE)
                    {
                        break;
                    }
                }
            }
        }

        dtvd_tuner_com_dev_mcbsp_read_stop();

        /*  (ii)終了判定                            */
        if(ctrl->reqstop)
        {
            /* スレッド終了要求の有    */
            break;      /* 正常終了     */
        }
    }

    for(;;) 
    {
        /*  (ii)終了判定                            */
        if(kthread_should_stop())
        {
            /* スレッド終了要求の有    */
            break;      /* 正常終了     */
        }
    }

    DTVD_DEBUG_MSG_EXIT();

    return 0;      /* 正常終了     */
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_create_ringbuffer                            */
/*****************************************************************************/
signed int dtvd_tuner_tsprcv_create_ringbuffer( void )
{
    int     i;
    void    *ptr;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* リングバッファ領域を０クリアする。   */
    dtvd_tuner_memset( (void*)&tdtvd_tuner_tsp_ringbuffer,      /* リングバッファ領域先頭アドレス   */
                        0,                                      /* クリアデータ(0)                  */
                        sizeof(tdtvd_tuner_tsp_ringbuffer),     /* クリアサイズ                     */
                        sizeof(tdtvd_tuner_tsp_ringbuffer));    /* クリアサイズ                     */

    /*スピンロック初期化 */
    spin_lock_init( &tdtvd_tuner_tsp_ringbuffer.lock );

    /* リングバッファ分のDMAメモリを確保する。  */
    for( i=0; i<D_DTVD_TUNER_TSP_RINGBUFFER_MAX; i++)
    {
        ptr = dma_alloc_coherent(D_DTVD_TUNER_NULL,             /* デバイス情報                 */
                                D_DTVD_TUNER_TSP_LENGTH,        /* DMAサイズ                    */
                                &tdtvd_tuner_tsp_ringbuffer.dma[i].dma_handle, /* 確保DMAハンドラ格納先    */
                                GFP_KERNEL);                    /* フラグ                       */
        if( ptr == D_DTVD_TUNER_NULL )
        {
            /* Syserr(エラー要因出力) */
            DTVD_TUNER_SYSERR_RANK_B( D_DTVD_TUNER_SYSERR_DRV_I2C_READ,
                                  DTVD_TUNER_TSPRCV,
                                  ptr, i, D_DTVD_TUNER_TSP_LENGTH, &tdtvd_tuner_tsp_ringbuffer.dma[i], 0, 0 );

            dtvd_tuner_tsprcv_delete_ringbuffer();     /* リングバッファ破棄 */

            return D_DTVD_TUNER_NG;
        }
        tdtvd_tuner_tsp_ringbuffer.dma[i].vptr = ptr;
    }

    DTVD_DEBUG_MSG_EXIT();

    return D_DTVD_TUNER_OK;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_delete_ringbuffer                            */
/*****************************************************************************/
void dtvd_tuner_tsprcv_delete_ringbuffer( void )
{
    int     i;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    /* リングバッファのDMAメモリを全て破棄する。  */
    for( i=0; i<D_DTVD_TUNER_TSP_RINGBUFFER_MAX; i++)
    {
        if( tdtvd_tuner_tsp_ringbuffer.dma[i].vptr != D_DTVD_TUNER_NULL)
        {
            /* DMAメモリを確保している場合、解放する。  */
            dma_free_coherent(D_DTVD_TUNER_NULL,                    /* デバイス情報                     */
                              D_DTVD_TUNER_TSP_LENGTH,              /* 解放DMAサイズ                    */
                              tdtvd_tuner_tsp_ringbuffer.dma[i].vptr,          /* CPU ADDR              */
                              tdtvd_tuner_tsp_ringbuffer.dma[i].dma_handle);   /* 解放DMAハンドラ       */
        }
        tdtvd_tuner_tsp_ringbuffer.dma[i].vptr = D_DTVD_TUNER_NULL; /* DMAバッファポインタ NULLクリア   */
    }

    DTVD_DEBUG_MSG_EXIT();

    return;
}


/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_read_dma_from_ringbuffer                     */
/*****************************************************************************/
DTVD_TUNER_TSP_DMA_t * dtvd_tuner_tsprcv_read_dma_from_ringbuffer
(
int next        /* １：リードポインタの更新、０：リードポインタ更新なし */ 
)
{
    DTVD_TUNER_TSP_DMA_t     *dma_p;
    unsigned long           flags;

    /*DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );*/

    /* リングバッファのデータ有無チェック  */
    if( tdtvd_tuner_tsp_ringbuffer.write_idx == tdtvd_tuner_tsp_ringbuffer.read_idx )
    {
        /* リードデータがない場合  */
        return  D_DTVD_TUNER_NULL;
    }

    if( next == 0 )
    {
        /* リードポインタ更新なしの場合 */
        /* リードポインタ(read_idx)の示すリングバッファ上のDMA情報アドレスを戻り値として通知する。 */
        dma_p = &tdtvd_tuner_tsp_ringbuffer.dma[tdtvd_tuner_tsp_ringbuffer.read_idx];
    }
    else
    {
        /* リードポインタの更新の場合 */

        /* スピンロック */
        spin_lock_irqsave(&tdtvd_tuner_tsp_ringbuffer.lock, flags);
        /* リードポインタ(read_idx)を更新する   */
        tdtvd_tuner_tsp_ringbuffer.read_idx = ((tdtvd_tuner_tsp_ringbuffer.read_idx +1) % D_DTVD_TUNER_TSP_RINGBUFFER_MAX);
        /* スピンロック解除 */
        spin_unlock_irqrestore(&tdtvd_tuner_tsp_ringbuffer.lock, flags);

        /* リングバッファのデータ有無チェック  */
        if( tdtvd_tuner_tsp_ringbuffer.write_idx == tdtvd_tuner_tsp_ringbuffer.read_idx )
        {
            /* リードデータがない場合  */
            return  D_DTVD_TUNER_NULL;
        }
        /* リードポインタ(read_idx)の示すリングバッファ上のDMA情報アドレスを戻り値として通知する。 */
        dma_p = &tdtvd_tuner_tsp_ringbuffer.dma[tdtvd_tuner_tsp_ringbuffer.read_idx];
        
    }

    /*DTVD_DEBUG_MSG_EXIT();*/

    return  dma_p;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_write_dma_from_ringbuffer                    */
/*****************************************************************************/
DTVD_TUNER_TSP_DMA_t * dtvd_tuner_tsprcv_write_dma_from_ringbuffer
(
int next        /* １：ライトポインタの更新, 0:ライトポインタ更新なし */ 
)
{
    DTVD_TUNER_TSP_DMA_t     *dma_p;
    unsigned long           flags;

    /*DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );*/


    if( next == 0 )
    {
        /* ライトポインタ更新なしの場合 */
        /* ライトポインタ(write_idx)の示すリングバッファ上のDMA情報アドレスを戻り値として通知する。 */
        dma_p = &tdtvd_tuner_tsp_ringbuffer.dma[tdtvd_tuner_tsp_ringbuffer.write_idx];
    }
    else
    {
        /* ライトポインタの更新の場合 */
        /* リングバッファフルチェック  */
        if( ((tdtvd_tuner_tsp_ringbuffer.write_idx +1) % D_DTVD_TUNER_TSP_RINGBUFFER_MAX) == tdtvd_tuner_tsp_ringbuffer.read_idx )
        {
            /* バッファフルの場合  */
            return  D_DTVD_TUNER_NULL;
        }

        /* スピンロック */
        spin_lock_irqsave(&tdtvd_tuner_tsp_ringbuffer.lock, flags);
        /* ライトポインタ(write_idx)を更新する   */
        tdtvd_tuner_tsp_ringbuffer.write_idx = ((tdtvd_tuner_tsp_ringbuffer.write_idx +1) % D_DTVD_TUNER_TSP_RINGBUFFER_MAX);
        /* スピンロック解除 */
        spin_unlock_irqrestore(&tdtvd_tuner_tsp_ringbuffer.lock, flags);

        /* ライトポインタ(write_idx)の示すリングバッファ上のDMA情報アドレスを戻り値として通知する。 */
        dma_p = &tdtvd_tuner_tsp_ringbuffer.dma[tdtvd_tuner_tsp_ringbuffer.write_idx];
        
    }

    /*DTVD_DEBUG_MSG_EXIT();*/

    return  dma_p;
}
#endif /* TUNER_MCBSP_ENABLE */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
