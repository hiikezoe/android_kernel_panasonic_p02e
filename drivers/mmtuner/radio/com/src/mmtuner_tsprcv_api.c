#include "dtvd_tuner_com.h"

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_init                                         */
/*****************************************************************************/
void dtvd_tuner_tsprcv_init( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );
#ifdef TUNER_MCBSP_ENABLE
    /* TSP受信スレッド管理情報クリア */
    dtvd_tuner_memset( &tdtvd_tuner_tsprcv_ctrl, 0x00,
                       sizeof( DTVD_TUNER_TSPRCV_CTRL_t ),
                       sizeof( DTVD_TUNER_TSPRCV_CTRL_t ) );
#endif /* TUNER_MCBSP_ENABLE */
    DTVD_DEBUG_MSG_EXIT();
    return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_start_receiving                              */
/*****************************************************************************/
signed int dtvd_tuner_tsprcv_start_receiving( void )
{
    signed int  ret;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

#ifdef TUNER_MCBSP_ENABLE
    /* TSP受信スレッド状態が起動中の場合は無処理 */
    if( tdtvd_tuner_tsprcv_ctrl.mode != 0 )
    {
        DTVD_DEBUG_MSG_ENTER( 1, 0, 0 );
        DTVD_DEBUG_MSG_EXIT();
        return D_DTVD_TUNER_OK;
    }

    /* TSP受信スレッド管理情報クリア */
    dtvd_tuner_memset( &tdtvd_tuner_tsprcv_ctrl, 0x00,
                       sizeof( DTVD_TUNER_TSPRCV_CTRL_t ),
                       sizeof( DTVD_TUNER_TSPRCV_CTRL_t ) );
    
    /* McBSP開始 */
    ret = dtvd_tuner_com_dev_mcbsp_start();
    if( ret != D_DTVD_TUNER_OK )
    {
        return D_DTVD_TUNER_NG;
    }

    /* リングバッファ生成 */
    ret = dtvd_tuner_tsprcv_create_ringbuffer();
    if( ret != D_DTVD_TUNER_OK )
    {
        return D_DTVD_TUNER_NG;
    }

    /* TSP受信スレッド生成 */
    ret = dtvd_tuner_tsprcv_create_recv_thread();
    if( ret != D_DTVD_TUNER_OK )
    {
        return D_DTVD_TUNER_NG;
    }
    
#endif /* TUNER_MCBSP_ENABLE */
#ifdef TUNER_TSIF_ENABLE
	ret = dtvd_tuner_com_dev_tsif_start();
	if( ret != D_DTVD_TUNER_OK )
	{
		return D_DTVD_TUNER_NG;
	}
#endif /* TUNER_TSIF_ENABLE */

    DTVD_DEBUG_MSG_EXIT();

    return D_DTVD_TUNER_OK;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_stop_receiving                               */
/*****************************************************************************/
void dtvd_tuner_tsprcv_stop_receiving( void )
{
    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

#ifdef TUNER_MCBSP_ENABLE
    /* TSP受信スレッド状態が停止中の場合は無処理 */
    if( tdtvd_tuner_tsprcv_ctrl.mode != 0 )
    {
        /* TSP受信スレッド停止依頼 */
        tdtvd_tuner_tsprcv_ctrl.reqstop = 1;

        /* McBSP chain受信停止 */
        dtvd_tuner_com_dev_mcbsp_read_cancel();

        /* TSP受信スレッド破棄(wait) */
        dtvd_tuner_tsprcv_stop_recv_thread();

        /* McBSP破棄 */
        dtvd_tuner_com_dev_mcbsp_stop();
        /* リングバッファ破棄 */
        dtvd_tuner_tsprcv_delete_ringbuffer();
        tdtvd_tuner_tsprcv_ctrl.mode = 0;
    }
#endif /* TUNER_MCBSP_ENABLE */
#ifdef TUNER_TSIF_ENABLE
	dtvd_tuner_com_dev_tsif_stop();
#endif /* TUNER_TSIF_ENABLE */

    DTVD_DEBUG_MSG_EXIT();

    return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_tsprcv_read_tsp                                     */
/*****************************************************************************/
signed int dtvd_tuner_tsprcv_read_tsp
(
    char __user    *buffer,         /* READ先バッファアドレス               */
    size_t         count            /* READサイズ                           */
)
{
#ifdef TUNER_MCBSP_ENABLE
    DTVD_TUNER_TSP_DMA_t *dma_adr;  /* DMA情報アドレスアドレス */
#endif /* TUNER_MCBSP_ENABLE */
    unsigned int   len;
	signed int ret = 0;

    DTVD_DEBUG_MSG_ENTER( count, 0, 0 );

    if( buffer == D_DTVD_TUNER_NULL )
    {
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_TSPRCV_API,
                                  0, 0, 0, 0, 0, 0 );
        return D_DTVD_TUNER_NG;
    }

#ifdef TUNER_MCBSP_ENABLE
    /* 受信TSP リングバッファのリードアドレス取得 */
    dma_adr = dtvd_tuner_tsprcv_read_dma_from_ringbuffer( 0 );
    if(( dma_adr == D_DTVD_TUNER_NULL ) || ( D_DTVD_TUNER_NULL == dma_adr->vptr ))  /* Mod：(npdc100492146) 2012/05/21 */
    {
        /* 受信データ無し */
        DTVD_DEBUG_MSG_INFO("Read TS-Data = 0\n");              /* Add：(npdc100492146) 2012/05/21 */
        return 0;
    }

    /* 転送サイズチェック */
    len = count;
    if( count > D_DTVD_TUNER_TSP_LENGTH )
    {
        len = D_DTVD_TUNER_TSP_LENGTH;
    }

    /* 受信パケットをユーザバッファに転送 */
    if( copy_to_user( (void __user *)buffer, ( const void * )dma_adr->vptr, len) )
    {
        len = D_DTVD_TUNER_NG; 
    }

    /* 受信TSP リングバッファのリードポインタ更新 */
    dtvd_tuner_tsprcv_read_dma_from_ringbuffer( 1 );
#endif /* TUNER_MCBSP_ENABLE */
#ifdef TUNER_TSIF_ENABLE
	/* 転送サイズチェック */
	len = count;
	if( count > D_DTVD_TUNER_TSP_LENGTH )
	{
		len = D_DTVD_TUNER_TSP_LENGTH;
	}
	/* get tsif read address */
	ret = dtvd_tuner_com_dev_tsif_read( (void __user *)buffer , len );
	if( ret < 0 ){
		return D_DTVD_TUNER_NG;
	}
	len = (unsigned int)ret;                                                   /* 20120611 modified for 0 byte reading */
#endif /* TUNER_TSIF_ENABLE */

    DTVD_DEBUG_MSG_EXIT();

    return len;
}
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
