#ifndef _DTVD_TUNER_CORE_H_
#define _DTVD_TUNER_CORE_H_

/*****************************************************************************/
/* 定数定義                                                                  */
/*****************************************************************************/
/* 電界(AGC閾値)判定 物理チャンネルグループ分け */
#define D_DTVD_TUNER_CORE_CH_GRP_NUM        8       /* チャンネルグループ数 */


/*****************************************************************************/
/* 構造体定義                                                                */
/*****************************************************************************/

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_CORE_AGCTHR_t                                       */
/* ABSTRACT : 電界判定閾値構造体                                             */
/* NOTE     :                                                                */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_CORE_AGCTHR_t {
    unsigned char   ch_min;     /* チャンネルグループ最小チャンネル番号 */
    unsigned char   ch_max;     /* チャンネルグループ最大チャンネル番号 */
    unsigned char   agc_thr1;   /* 単発選局：AGC判定閾値 */
    unsigned char   agc_thr2;   /* サーチ＆スキャン：AGC判定閾値 */
} DTVD_TUNER_CORE_AGCTHR_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_CORE_INFO_t                                         */
/* ABSTRACT : 全体制御情報構造体                                             */
/* NOTE     :                                                                */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_CORE_INFO_t {
    signed int                  status;             /* 内部遷移状態 */
    struct file     *pipe_handle;                   /* pipe handle */
    DTVD_TUNER_CHANNEL_t        tune_ch;            /* 選局情報：チャンネル情報 */
    DTVD_TUNER_ADJUST_t         tune_adj;           /* 選局情報：チャンネル調整値 */
    unsigned long               tune_seq_id;        /* 選局情報：シーケンスID */
    unsigned char               tune_kind;          /* 選局情報：選局種別 */
    unsigned char               sync_flg;           /* 同期フラグ */
    unsigned char               tune_cause;         /* 選局失敗要因 */
    unsigned char               reserved1;          /* 予約 */
    unsigned long               cn_seq_id;          /* CN値測定用シーケンスID */
    unsigned long               tune_start_jiff;    /* 選局開始時のjiffies値 */
    unsigned long               tune_end_jiff;      /* 選局成功時のjiffies値 */
    unsigned char               omt_flag;           /* 工程コマンドフラグ */
    unsigned char               omt_tune_kind;      /* 工程選局同期確認フラグ */

    unsigned char               eco_flag;           /* 省電力フラグ */
    unsigned char               reserved2;          /* 予約 */
    unsigned char               stop_state_fst;     /* 初回停止状態フラグ */
    unsigned char               end_retry_flag;     /* 初期化失敗後の終了リトライフラグ */
    unsigned char               reserved[2];        /* 予約 */

    unsigned int                cn_qpsk1_2_m;       /*  通常モード、 QPSK、R=1/2（弱⇔中）   */
    unsigned int                cn_qpsk1_2_h;       /*  通常モード、 QPSK、R=1/2（中⇔強）   */
    unsigned int                cn_qpsk2_3_m;       /*  通常モード、 QPSK、R=2/3（弱⇔中）   */
    unsigned int                cn_qpsk2_3_h;       /*  通常モード、 QPSK、R=2/3（中⇔強）   */
    unsigned int                cn_16qam1_2_m;      /*  通常モード、16QAM、R=1/2（弱⇔中）  */
    unsigned int                cn_16qam1_2_h;      /*  通常モード、16QAM、R=1/2（中⇔強）  */
    unsigned int                cn_qpsk1_2_eco_m;   /*  ECOモード、  QPSK、R=1/2（弱⇔中）    */
    unsigned int                cn_qpsk1_2_eco_h;   /*  ECOモード、  QPSK、R=1/2（中⇔強）    */
    unsigned int                cn_qpsk2_3_eco_m;   /*  ECOモード、  QPSK、R=2/3（弱⇔中）    */
    unsigned int                cn_qpsk2_3_eco_h;   /*  ECOモード、  QPSK、R=2/3（中⇔強）    */
    unsigned int                cn_16qam1_2_eco_m;  /*  ECOモード、 16QAM、R=1/2（弱⇔中）   */
    unsigned int                cn_16qam1_2_eco_h;  /*  ECOモード、 16QAM、R=1/2（中⇔強）   */

    DTVD_TUNER_CORE_AGCTHR_t    agc_normal[D_DTVD_TUNER_CORE_CH_GRP_NUM]; /* 通常AGC判定閾値 */
    DTVD_TUNER_CORE_AGCTHR_t    agc_eco[D_DTVD_TUNER_CORE_CH_GRP_NUM];    /* 省電力AGC判定閾値 */

} DTVD_TUNER_CORE_INFO_t;


/*****************************************************************************/
/* 関数プロトタイプ宣言                                                      */
/*****************************************************************************/
/* メッセージ送信関数 */
void dtvd_tuner_core_trans_msg_tune_ok( unsigned char );
void dtvd_tuner_core_trans_msg_tune_ng( unsigned char );
void dtvd_tuner_core_trans_msg_deverr( void );

/*****************************************************************************/
/* 外部参照宣言                                                              */
/*****************************************************************************/
extern DTVD_TUNER_CORE_INFO_t tdtvd_tuner_core;

#endif /* _DTVD_TUNER_CORE_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
