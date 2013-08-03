#include "dtvd_tuner_com.h"
#include "dtvd_tuner_core.h"

/*****************************************************************************/
/* ローカル関数プロトタイプ宣言                                              */
/*****************************************************************************/
static void dtvd_tuner_core_trans_mdl_send_msg( unsigned short, void*, unsigned int );

/*****************************************************************************/
/* 外部イベント                                                              */
/*****************************************************************************/
/*****************************************************************************/
/* MODULE   : dtvd_tuner_core_trans_msg_tune_ok                              */
/*****************************************************************************/
void dtvd_tuner_core_trans_msg_tune_ok
(
    unsigned char agc       /* AGC値 */
)
{
    unsigned int                    len;
    DTVD_TUNER_MSGDATA_TUNE_OK_t    data;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    len = sizeof( DTVD_TUNER_MSGDATA_TUNE_OK_t );

    dtvd_tuner_memset( &data, 0x00, len, len );

    /* シーケンスID */
    data.seq_id = tdtvd_tuner_core.tune_seq_id;
    /* モード */
    data.adj.mode = tdtvd_tuner_monitor.tune.adj.mode;
    /* ガード */
    data.adj.gi = tdtvd_tuner_monitor.tune.adj.gi;
    /* AGC値 */
    data.agc = (unsigned char)( 0xFF - agc );

    /* メッセージ送信 */
    dtvd_tuner_core_trans_mdl_send_msg( D_DTVD_TUNER_MSGID_TUNE_OK,
                                        &data, len );

    DTVD_DEBUG_MSG_EXIT();
    return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_core_trans_msg_tune_ng                              */
/*****************************************************************************/
void dtvd_tuner_core_trans_msg_tune_ng
(
    unsigned char cause     /* 失敗要因 */
)
{
    unsigned int                    len;
    DTVD_TUNER_MSGDATA_TUNE_NG_t    data;

    DTVD_DEBUG_MSG_ENTER( cause, 0, 0 );

    len = sizeof( DTVD_TUNER_MSGDATA_TUNE_NG_t );

    dtvd_tuner_memset( &data, 0x00, len, len );

    /* シーケンスID */
    data.seq_id = tdtvd_tuner_core.tune_seq_id;
    /* 失敗要因 */
    data.cause = cause;

    /* メッセージ送信 */
    dtvd_tuner_core_trans_mdl_send_msg( D_DTVD_TUNER_MSGID_TUNE_NG,
                                        &data, len );

    DTVD_DEBUG_MSG_EXIT();
    return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_core_trans_msg_deverr                               */
/*****************************************************************************/
void dtvd_tuner_core_trans_msg_deverr( void )
{
    unsigned int                len;
    DTVD_TUNER_MSGDATA_DEVERR_t data;

    DTVD_DEBUG_MSG_ENTER( 0, 0, 0 );

    len = sizeof( DTVD_TUNER_MSGDATA_DEVERR_t );

    dtvd_tuner_memset( &data, 0x00, len, len );

    /* メッセージ送信 */
    dtvd_tuner_core_trans_mdl_send_msg( D_DTVD_TUNER_MSGID_DEVERR,
                                        &data, len );

    DTVD_DEBUG_MSG_EXIT();
    return;
}

/*****************************************************************************/
/* MODULE   : dtvd_tuner_core_trans_mdl_send_msg                             */
/*****************************************************************************/
static void dtvd_tuner_core_trans_mdl_send_msg
(
    unsigned short msgid,   /* イベントID */
    void* data,             /* メッセージ本体 */
    unsigned int length     /* メッセージ長 */
)
{
    DTVD_TUNER_MSG_t    msg;
    unsigned int        data_len;

    DTVD_DEBUG_MSG_ENTER( msgid, 0, length );

    /* NULLポインタ逆参照のチェック */
    if( data == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_CORE_TRANS,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    /* lengthのチェック */
    data_len = sizeof(DTVD_TUNER_MSG_DATA_u);
    if( length > data_len )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_CORE_TRANS,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    /* メッセージID */
    msg.header.msg_id = msgid;
    /* 受信側ブロックID */
    msg.header.receiver_id = D_DTVD_TUNER_BLOCK_ID_DMM;
    /* 送信側ブロックID */
    msg.header.sender_id = D_DTVD_TUNER_BLOCK_ID_TUNERDD;

    /* メッセージデータ */
    dtvd_tuner_memset( &msg.data, 0x00, data_len, data_len );
    dtvd_tuner_memcpy( &msg.data, data, length, sizeof(DTVD_TUNER_MSG_DATA_u) );

    dtvd_tuner_com_dev_ddsync_write( tdtvd_tuner_core.pipe_handle,
                                     (void *)&msg,
                                     sizeof(DTVD_TUNER_MSG_t) );

    DTVD_DEBUG_MSG_EXIT();
    return;
}
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
