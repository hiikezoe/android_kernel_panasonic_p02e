#ifndef _DTVD_TUNER_TSPRCV_H_
#define _DTVD_TUNER_TSPRCV_H_

#define D_DTVD_TUNER_TSPRCV_THREAD  "tunerrcv"      /* 仮????????????????????????*/

/* リングバッファ関連 */
#ifdef TUNER_MCBSP_ENABLE
#define D_DTVD_TUNER_TSP_FRAME_ENABLE_RS    204         /* TSPフレーム長(204byte) *//* Mod：(BugLister ID93) 2011/12/08 */
#define D_DTVD_TUNER_TSP_FRAME_DISABLE_RS   188         /* TSPフレーム長(188byte) *//* Add：(BugLister ID93) 2011/12/08 */
#define D_DTVD_TUNER_TSP_FRAMENUM_ENABLE_RS  20         /* TSPフレーム数(RS有り) *//* Add：(BugLister ID93) 2011/12/08 */
#define D_DTVD_TUNER_TSP_FRAMENUM_DISABLE_RS 20         /* TSPフレーム数(RS無し) *//* Add：(BugLister ID93) 2011/12/08 */
#endif /* TUNER_MCBSP_ENABLE */
#ifdef TUNER_TSIF_ENABLE
#define D_DTVD_TUNER_TSP_FRAME				192	/* TSPフレーム長(192byte)固定 */
#define D_DTVD_TUNER_TSP_PAKETS_NUM_DTV			20	/* DTVパケット数(20paket)固定 */
#define D_DTVD_TUNER_TSP_PAKETS_NUM_MM			80	/* MMパケット数(40paket)固定 */
#define D_DTVD_TUNER_TSP_TSIF_RINGBUFFER_NUM_DTV	8	/* DTV時TSIFリングバッファ数 */
#define D_DTVD_TUNER_TSP_TSIF_RINGBUFFER_NUM_MM		12	/* MM時TSIFリングバッファ数 */ /* 20120605 20→12変更 */
#endif /* TUNER_TSIF_ENABLE */
#define D_DTVD_TUNER_TSP_RINGBUFFER_MAX     20           /* リングバッファ数     *//* Mod：(MMシステム対応) 2012/03/14 *//* Mod：(バッファ数変更 8→20) 2012/04/26 */

#ifdef TUNER_MCBSP_ENABLE
/* 受信スレッド状態フラグ             */
enum dvd_tuner_tsprcv_mode {
    D_DTVD_TUNER_TSPRCV_MODE_STOP = 0,   /* 停止中       */
    D_DTVD_TUNER_TSPRCV_MODE_1RCV,       /* 初回受信待ち */
    D_DTVD_TUNER_TSPRCV_MODE_RECV        /* 受信中       */
};

/* 使用中フラグ          */
enum dtvd_tuner_tsp_dma_flag {
    D_DTVD_TUNER_TSP_DMA_FLAG_OFF = 0,  /* 未使用       */
    D_DTVD_TUNER_TSP_DMA_FLAG_ON        /* 使用中       */
};
#endif /* TUNER_MCBSP_ENABLE */


/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_TSP_DMA_t                                           */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_TSP_DMA_t {
    void        *vptr;              /* DMAバッファポインタ   */
    dma_addr_t  dma_handle;         /* DMAハンドル           */
    int         flag;               /* 使用中フラグ          */
} DTVD_TUNER_TSP_DMA_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_TSP_RINGBUFFER_t                                    */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_TSP_RINGBUFFER_t{
    spinlock_t              lock;                   /* リングバッファのロック       */
    int                     write_idx;              /* ライトインデックス           */
    int                     read_idx;               /* リードインデックス           */
    DTVD_TUNER_TSP_DMA_t    dma[D_DTVD_TUNER_TSP_RINGBUFFER_MAX]; /* リングバッファ */
} DTVD_TUNER_TSP_RINGBUFFER_t;

/*---------------------------------------------------------------------------*/
/* TAG      : DTVD_TUNER_TSPRCV_CTRL_t                                       */
/*---------------------------------------------------------------------------*/
typedef struct _DTVD_TUNER_TSPRCV_CTRL_t {
    unsigned char   mode;           /* 受信スレッド状態    */
    unsigned char   reqstop;        /* 要求                */
    unsigned long   tspcount;       /* TSP受信数(DEBUG用)  */
    unsigned long   overcount;      /* overflow数(DEBUG用) */
} DTVD_TUNER_TSPRCV_CTRL_t;

/*****************************************************************************/
/* 関数プロトタイプ宣言                                                      */
/*****************************************************************************/

/* TSパケット受信API処理 */
void dtvd_tuner_tsprcv_init( void );
#ifdef TUNER_MCBSP_ENABLE
signed int dtvd_tuner_tsprcv_create_recv_thread( void );
void  dtvd_tuner_tsprcv_stop_recv_thread( void );
#endif /* TUNER_MCBSP_ENABLE */

signed int dtvd_tuner_tsprcv_start_receiving( void );
void dtvd_tuner_tsprcv_stop_receiving( void );
signed int dtvd_tuner_tsprcv_read_tsp( char __user *buffer, size_t count );

#ifdef TUNER_MCBSP_ENABLE
signed int dtvd_tuner_tsprcv_create_ringbuffer( void );
void dtvd_tuner_tsprcv_delete_ringbuffer( void );

DTVD_TUNER_TSP_DMA_t * dtvd_tuner_tsprcv_write_dma_from_ringbuffer( int next ); 
DTVD_TUNER_TSP_DMA_t * dtvd_tuner_tsprcv_read_dma_from_ringbuffer( int next ); 
#endif /* TUNER_MCBSP_ENABLE */


/*****************************************************************************/
/* 外部参照宣言                                                              */
/*****************************************************************************/

/* TSP受信スレッド情報 */
/* kthread_createで作成したTSP受信スレッドのスレッド情報。 */
extern  struct task_struct *tdtvd_tuner_tsprcv_thread; 

extern  DTVD_TUNER_TSP_RINGBUFFER_t  tdtvd_tuner_tsp_ringbuffer;
extern  DTVD_TUNER_TSPRCV_CTRL_t     tdtvd_tuner_tsprcv_ctrl;

extern  unsigned int D_DTVD_TUNER_TSP_LENGTH;	/* Add：(BugLister ID93) 2011/12/08 */
extern  unsigned char TS_RECV_MODE;				/* Add：(MMシステム対応) 2012/03/14 */

#endif /* _DTVD_TUNER_TSPRCV_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
