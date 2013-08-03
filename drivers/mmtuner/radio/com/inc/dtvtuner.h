#ifndef _DTVTUNER_H_
#define _DTVTUNER_H_

/*****************************************************************************/
/* 定数定義                                                                  */
/*****************************************************************************/

/* 関数戻り値 */
#define D_DTVD_TUNER_OK             0                   /* 成功 */
#define D_DTVD_TUNER_NG             (-1)                /* 失敗 */

/* メッセージID */
#define D_DTVD_TUNER_MSGID_TUNE_OK      1   /* 選局成功 */
#define D_DTVD_TUNER_MSGID_TUNE_NG      2   /* 選局失敗 */
#define D_DTVD_TUNER_MSGID_DEVERR       3   /* 異常通知 */

/* チューナDDの機能ブロックID */
#define D_DTVD_TUNER_BLOCK_ID_TUNERDD   6   /* チューナDDの機能ブロックID   */

/* ミドル制御の機能ブロックID */
#define D_DTVD_TUNER_BLOCK_ID_DMM       1   /* ミドル制御の機能ブロックID   */
/* 選局失敗要因 */
#define D_DTVD_TUNER_TUNE_NG_NONE       0   /* 未設定      */


/*****************************************************************************/
/* 構造体定義                                                               */
/*****************************************************************************/

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_CHANNEL_t                                           */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_CHANNEL_t {
    unsigned char   no;             /* 物理チャンネル番号 */
    unsigned char   seg;            /* 中心セグメント番号 */
    unsigned char   reserved[2];    /* 予約 */
} DTVD_TUNER_CHANNEL_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_ADJUST_t                                            */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_ADJUST_t {
    unsigned char   mode;           /* TS伝送モード */
    unsigned char   gi;             /* ガードインターバル比 */
    unsigned char   reserved[2];    /* 予約 */
} DTVD_TUNER_ADJUST_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MONITOR_RFIC_t                                      */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MONITOR_RFIC_t {
    unsigned char   ant;            /* アンテナ種別 */
    unsigned char   power;          /* 電源状態 */
    unsigned char   reserved[2];    /* 予約 */
} DTVD_TUNER_MONITOR_RFIC_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MONITOR_TUNE_t                                      */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MONITOR_TUNE_t {
    DTVD_TUNER_CHANNEL_t    ch;         /* チャンネル情報 */
    DTVD_TUNER_ADJUST_t     adj;        /* チャンネル調整値情報 */
    unsigned char           sync_state; /* 同期状態 */
    unsigned char           reserved[3];/* 予約 */
    /* unsigned long          sync_time; */      /* フレーム同期捕捉時間 */
    volatile unsigned long   sync_time; /* フレーム同期捕捉時間(Linux2.6対応)*/
} DTVD_TUNER_MONITOR_TUNE_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_TMCCD_TRANS_PARAM_t                                 */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_TMCCD_TRANS_PARAM_t {
    unsigned char   modulation;     /* キャリア変調方式 */
    unsigned char   coderate;       /* 畳み込み符号化率 */
    unsigned char   interleave;     /* インターリーブ長 */
    unsigned char   seg;            /* セグメント数 */
} DTVD_TUNER_TMCCD_TRANS_PARAM_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_TMCC_DATA_t                                         */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_TMCC_DATA_t {
    unsigned char                   part;           /* 形式識別フラグ */
    unsigned char                   reserved[3];    /* 予約 */
    DTVD_TUNER_TMCCD_TRANS_PARAM_t  layer_a;        /* A階層伝送パラメータ情報 */
    DTVD_TUNER_TMCCD_TRANS_PARAM_t  layer_b;        /* B階層伝送パラメータ情報 */
} DTVD_TUNER_TMCC_DATA_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_TMCC_INFO_t                                         */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_TMCC_INFO_t {
    unsigned char           system;     /* システム識別子 */
    unsigned char           cntdwn;     /* 伝送パラメータ切り替え指標(0-15) */
    unsigned char           emgflg;     /* 緊急警報放送用起動フラグ */
    unsigned char           reserved;   /* 予約 */
    DTVD_TUNER_TMCC_DATA_t  curr;       /* カレント情報 */
    DTVD_TUNER_TMCC_DATA_t  next;       /* ネクスト情報 */
} DTVD_TUNER_TMCC_INFO_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MONITOR_TMCC_t                                      */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MONITOR_TMCC_t {
    unsigned char           error;          /* TMCCエラーなし／あり */
    unsigned char           nonstd;         /* 規格違反 */
    unsigned char           reserved[2];    /* 予約 */
    DTVD_TUNER_TMCC_INFO_t  info;           /* TMCC情報 */
} DTVD_TUNER_MONITOR_TMCC_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MEASURE_AGC_t                                       */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MEASURE_AGC_t {
    unsigned char   state;      /* 測定状況 */
    unsigned char   value_x;    /* ブランチX(RFICメイン)のAGC値 */
    unsigned char   value_y;    /* ブランチY(RFICサブ)のAGC値 */
    unsigned char   reserved;   /* 予約 */
} DTVD_TUNER_MEASURE_AGC_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MEASURE_VALUE_t                                     */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MEASURE_VALUE_t {
    unsigned char   result;         /* 測定結果 */
    unsigned char   up;             /* 上位値 */
    unsigned char   low;            /* 下位値 */
    unsigned char   ext;            /* 拡張領域 *//* 測定値が3byteとなる場合使用する */
} DTVD_TUNER_MEASURE_VALUE_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MEASURE_CNA_t                                       */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MEASURE_CNA_t {
    unsigned char               state;          /* 測定状況 */
    unsigned char               reserved[3];    /* 予約 */
    DTVD_TUNER_MEASURE_VALUE_t  value_x;        /* ブランチX(RFICメイン)のCN値 */
                                                /* up:CNRDXU low:CNRDXL */
    DTVD_TUNER_MEASURE_VALUE_t  value_y;        /* ブランチY(RFICサブ)のCN値 */
                                                /* up:CNRDYU low::CNRDYL */
    DTVD_TUNER_MEASURE_VALUE_t  value_comp;     /* 合成後の値 */
                                                /* up:CNRDSU low:CNRDSL */
} DTVD_TUNER_MEASURE_CNA_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MEASURE_BER_t                                       */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MEASURE_BER_t {
    unsigned char               state;          /* 測定状況 */
    unsigned char               measure_type;   /* BER値測定種別 */
    unsigned char               reserved[2];    /* 予約 */
    DTVD_TUNER_MEASURE_VALUE_t  value;          /* BER値 */
} DTVD_TUNER_MEASURE_BER_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MONITOR_RX_t                                        */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MONITOR_RX_t {
    DTVD_TUNER_MEASURE_AGC_t agc;   /* 測定AGC値 */
    DTVD_TUNER_MEASURE_CNA_t cna;   /* 測定CNA値 */
    DTVD_TUNER_MEASURE_BER_t ber;   /* 測定BER値 */
} DTVD_TUNER_MONITOR_RX_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MONITOR_INFO_t                                      */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MONITOR_INFO_t {
    unsigned char               ant_ext_state;  /* イヤホンアンテナ接続状態 */
    unsigned char               diver;          /* ダイバシチ設定・解除状態 */
    unsigned char               reserved[2];    /* 予約 */
    DTVD_TUNER_MONITOR_RFIC_t   rfic_main;      /* RFICメイン情報 */
    DTVD_TUNER_MONITOR_RFIC_t   rfic_sub;       /* RFICサブ情報 */
    DTVD_TUNER_MONITOR_TUNE_t   tune;           /* 選局情報 */
    DTVD_TUNER_MONITOR_TMCC_t   tmcc;           /* TMCC情報 */
    DTVD_TUNER_MONITOR_RX_t     rx;             /* 無線部監視情報 */
} DTVD_TUNER_MONITOR_INFO_t;


/*****************************************************************************/
/* ミドル制御送信メッセージ情報構造体                                        */
/*****************************************************************************/
/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSGDATA_TUNE_OK_t                                   */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSGDATA_TUNE_OK_t {
    unsigned long           seq_id;         /* シーケンスID */
    DTVD_TUNER_ADJUST_t     adj;            /* チャンネル調整値情報 */
    unsigned char           agc;            /* AGC値 */
    unsigned char           reserved[3];    /* 予約 */
} DTVD_TUNER_MSGDATA_TUNE_OK_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSGDATA_TUNE_NG_t                                   */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSGDATA_TUNE_NG_t {
    unsigned long           seq_id;         /* シーケンスID */
    unsigned char           cause;          /* 失敗要因 */
    unsigned char           reserved[3];    /* 予約 */
} DTVD_TUNER_MSGDATA_TUNE_NG_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSGDATA_SYNC_t                                      */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSGDATA_SYNC_t {
    DTVD_TUNER_ADJUST_t     adj;            /* チャンネル調整値情報 */
} DTVD_TUNER_MSGDATA_SYNC_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSGDATA_ASYNC_t                                     */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSGDATA_ASYNC_t {
    unsigned char           reserved[4];    /* 予約 */
} DTVD_TUNER_MSGDATA_ASYNC_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSGDATA_CN_OK_t                                     */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSGDATA_CN_OK_t {
    unsigned char           rx_level;       /* 受信レベル */
    unsigned char           reserved[3];    /* 予約 */
} DTVD_TUNER_MSGDATA_CN_OK_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSGDATA_DEVERR_t                                    */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSGDATA_DEVERR_t {
    unsigned char           error_type;     /* ERROR TYPE */
    unsigned char           reserved[3];    /* 予約 */
} DTVD_TUNER_MSGDATA_DEVERR_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSG_HEADER_t                                        */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSG_HEADER_t {
    unsigned short  msg_id;         /* メッセージID */
    unsigned char   receiver_id;    /* 受信側ブロックID */
    unsigned char   sender_id;      /* 送信側ブロックID */
} DTVD_TUNER_MSG_HEADER_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSG_DATA_u                                          */
/*---------------------------------------------------------------------------*/
    typedef union DTVD_TUNER_MSG_DATA_u {
    DTVD_TUNER_MSGDATA_TUNE_OK_t    tune_ok;    /* 選局成功メッセージデータ */
    DTVD_TUNER_MSGDATA_TUNE_NG_t    tune_ng;    /* 選局失敗メッセージデータ */
    DTVD_TUNER_MSGDATA_SYNC_t       sync;       /* RF同期捕捉メッセージデータ */
    DTVD_TUNER_MSGDATA_ASYNC_t      async;      /* RF同期外れメッセージデータ */
    DTVD_TUNER_MSGDATA_CN_OK_t      cn_ok;      /* CN値測定開始メッセージデータ */
    DTVD_TUNER_MSGDATA_DEVERR_t     deverr;     /* 異常通知メッセージデータ */
    unsigned char                   dummy[60];  /* ダミー */
} DTVD_TUNER_MSG_DATA_u;


/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_MSG_t                                               */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_MSG_t {
    DTVD_TUNER_MSG_HEADER_t     header;     /* メッセージヘッダ */
    DTVD_TUNER_MSG_DATA_u       data;       /* メッセージデータ */
} DTVD_TUNER_MSG_t;

#endif /* _DTVTUNER_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
