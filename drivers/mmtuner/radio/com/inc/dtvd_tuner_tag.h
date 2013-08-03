#ifndef _DTVD_TUNER_TAG_H_
#define _DTVD_TUNER_TAG_H_

/*****************************************************************************/
/* 構造体定義                                                                */
/*****************************************************************************/

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_COM_SYSERR_t                                        */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_COM_SYSERR_t {
    unsigned char   file_no;        /* ファイル番号 */
    unsigned char   reserved[3];    /* 予約 */
    unsigned long   line;           /* 行番号 */
    unsigned long   log_data[6];    /* ログデータ */
} DTVD_TUNER_COM_SYSERR_t;


/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_CONTROL_t                                           */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_CONTROL_t {
    unsigned int                count;              /* オープンカウント */
    atomic_t                    event_flg;          /* 要求イベントフラグ */
    wait_queue_head_t           wait_queue;         /* スレッド用ウェイトキュー */
    volatile unsigned long      block_flg;          /* ブロックモード用コンディションフラグ */
    wait_queue_head_t           wait_queue_block;   /* ブロックウェイトキュー */
    signed int                  result;             /* 同期処理の処理結果 */
    signed int                  omt_tune_result;    /* 工程用選局要求の選局結果 */
    struct i2c_adapter          *adap;              /* I2Cアダプタ */
} DTVD_TUNER_CONTROL_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_COM_I2C_DATA_t                                      */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_COM_I2C_DATA_t
{
    unsigned char adr;                                  /* アドレス部 */
    unsigned char data;                                 /* データ部 */
    unsigned char reserved[2];                          /* 予約 */
} DTVD_TUNER_COM_I2C_DATA_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_COM_NONVOLATILE_t                                   */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_COM_NONVOLATILE_t {
    unsigned short  wait_tmcc;                  /*  TMCC取得待ちタイマ値    */
    unsigned short  cn_wait;                    /*  初回CN安定待ち  */
    unsigned short  wait_ts;                    /*  TS出力待ちタイマ値  */
    unsigned short  cn_ave_count;               /*  CN値安定待ち平滑化回数  */
    unsigned char   cn_cycle;                   /*  CN測定待ちタイマ値  */
    unsigned char   cn_alpha;                   /*  CN平滑化式パラメータα  */
    unsigned char   cn_beta;                    /*  CN平滑化式パラメータβ  */
    unsigned char   cn_gamma;                   /*  CN平滑化式パラメータγ  */
    unsigned char   agc_wait;                   /*  初回AGC安定待ち時間 */
    unsigned char   agc_cycle;                  /*  AGC読み込み周期 */
    unsigned char   ber_wait;                   /*  BER安定待ち時間 */
    unsigned char   ber_cycle;                  /*  BER測定周期 */
    unsigned char   state_monitor_cycle;        /*  同期ステート読み込み周期    */
    unsigned char   wait_search_int;            /*  サーチ割り込み監視タイマー  */
    unsigned char   wait_mnt_sync;              /*  フレーム同期確立監視タイマー    */
    unsigned char   omt_agc_thr;                /*  工程検査用ＡＧＣ値閾値  */
    unsigned char   auto_n2e_ifagc_qp12;        /*  適応制御1-1(AUTO_N2E_IFAGC_QP12)    */
    unsigned char   auto_n2e_ifagc_qp23;        /*  適応制2-1(AUTO_N2E_IFAGC_QP23)  */
    unsigned char   auto_n2e_ifagc_qam12;       /*  適応制御3-1(AUTO_N2E_IFAGC_QAM12)   */
    unsigned char   auto_e2n_ifagc_qp12;        /*  適応制御4-1(AUTO_E2N_IFAGC_QP12)    */
    unsigned char   auto_e2n_ifagc_qp23;        /*  適応制5-1(AUTO_E2N_IFAGC_QP23)  */
    unsigned char   auto_e2n_ifagc_qam12;       /*  適応制御6-1(AUTO_E2N_IFAGC_QAM12)   */
/*$M #Rev.00.02 CHG-S 1511023 */
/*$M unsigned char   reserved0;                *//*  予約ビット  */
    unsigned char   wait_refclk;                /*  ＣＬＫ立ち上がり待ち時間(WAIT_REFCLK) */
/*$M #Rev.00.02 CHG-E 1511023 */
/*$M #Rev.00.03 CHG-S 1511038 */
/*$M unsigned char   reserved1;                *//*  予約ビット  */
    unsigned char   search_thres;               /*  チャネルサーチ閾値(SEARCH_THRES)  */
/*$M #Rev.00.03 CHG-E 1511038 */
/*<5210011>    unsigned char   reserved2;      *//*  予約ビット  */
/*<5210011>    unsigned char   reserved3;      *//*  予約ビット  */
    unsigned char   wait_lock1;                 /*  シンセLOCK待ち時間1 */
    unsigned char   wait_lock2;                 /*  シンセLOCK待ち時間2 */
    unsigned char   reserved4;                  /*  予約ビット  */
    unsigned char   reserved5;                  /*  予約ビット  */
    unsigned int    cn_qpsk1_2_m;               /*  通常モード、QPSK、R=1/2（弱⇔中）   */
    unsigned int    cn_qpsk1_2_h;               /*  通常モード、QPSK、R=1/2（中⇔強）   */
    unsigned int    cn_qpsk2_3_m;               /*  通常モード、QPSK、R=2/3（弱⇔中）   */
    unsigned int    cn_qpsk2_3_h;               /*  通常モード、QPSK、R=2/3（中⇔強）   */
    unsigned int    cn_16qam1_2_m;              /*  通常モード、16QAM、R=1/2（弱⇔中）  */
    unsigned int    cn_16qam1_2_h;              /*  通常モード、16QAM、R=1/2（中⇔強）  */
    unsigned int    cn_qpsk1_2_eco_m;           /*  ECOモード、QPSK、R=1/2（弱⇔中）    */
    unsigned int    cn_qpsk1_2_eco_h;           /*  ECOモード、QPSK、R=1/2（中⇔強）    */
    unsigned int    cn_qpsk2_3_eco_m;           /*  ECOモード、QPSK、R=2/3（弱⇔中）    */
    unsigned int    cn_qpsk2_3_eco_h;           /*  ECOモード、QPSK、R=2/3（中⇔強）    */
    unsigned int    cn_16qam1_2_eco_m;          /*  ECOモード、16QAM、R=1/2（弱⇔中）   */
    unsigned int    cn_16qam1_2_eco_h;          /*  ECOモード、16QAM、R=1/2（中⇔強）   */
    unsigned char   mode_on;                    /*  モード検出ON/OFF（MODE_ON） */
    unsigned char   tmcc_on;                    /*  TMCC情報使用ON/OFF(TMCC_ON) */
    unsigned char   mode_dec_count;             /*  モード検出回数（MODE_DEC_COUNT）    */
    unsigned char   search_success_count;       /*  チャンネルサーチ回数・判定(SEARCH_SUCCESS_COUNT) */
    unsigned char   search_success_thr;         /*  サーチ成功判定閾値(SEARCH_SUCCESS_THR)  */
    unsigned char   search_time;                /*  サーチ時間(SEARCH_TIME) */
    unsigned char   scan_mode_dec_count;        /*  スキャンモード検出回数（SCAN_MODE_DEC_COUNT）   */
    unsigned char   scan_search_success_count;  /*  スキャンチャンネルサーチ回数・判定回数(SCAN_SEARCH_SUCCESS_COUNT)    */
    unsigned char   reserved10;                 /*  予約ビット  */
    unsigned char   reserved11;                 /*  予約ビット  */
    unsigned char   reserved12;                 /*  予約ビット  */
    unsigned char   reserved13;                 /*  予約ビット  */
    unsigned char   reserved14;                 /*  予約ビット  */
    unsigned char   reserved15;                 /*  予約ビット  */
    unsigned char   reserved16;                 /*  予約ビット  */
    unsigned char   reserved17;                 /*  予約ビット  */
    unsigned int    auto_n2e_cn_qp12;           /*  適応制御1-2 (AUTO_N2E_CN_QP12)  */
    unsigned int    auto_n2e_cn_qp23;           /*  適応制御2-2 (AUTO_N2E_CN_QP23)  */
    unsigned int    auto_n2e_cn_qam12;          /*  適応制御3-2 (AUTO_N2E_CN_QAM12) */
    unsigned int    auto_e2n_cn_qp12;           /*  適応制御4-2 (AUTO_E2N_CN_QP12)  */
    unsigned int    auto_e2n_cn_qp23;           /*  適応制御5-2 (AUTO_E2N_CN_QP23)  */
    unsigned int    auto_e2n_cn_qam12;          /*  適応制御6-2 (AUTO_E2N_CN_QAM12) */
#if 0 //2010.12.14 m-shimizu
    unsigned char   reserved20;                 /*  予約ビット  */
    unsigned char   reserved21;                 /*  予約ビット  */
    unsigned char   reserved22;                 /*  予約ビット  */
    unsigned char   reserved23;                 /*  予約ビット  */
    unsigned char   reserved24;                 /*  予約ビット  */
    unsigned char   reserved25;                 /*  予約ビット  */
    unsigned char   reserved26;                 /*  予約ビット  */
    unsigned char   reserved27;                 /*  予約ビット  */
#endif
} DTVD_TUNER_COM_NONVOLATILE_t;

#endif /* _DTVD_TUNER_TAG_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
