/*
 * ioctl_msg.h
 *
 * I/O control メッセージ定義
 *
 */

#define NAT_TRAV_ENABLE  1
#define NAT_TRAV_DISABLE 0
#define DEV_NAME_LEN     32
#define MAX_SA_NUM       4
#define DRV_VER_LEN      8

enum _COMMAND{
  COM_IPSEC_INIT = 1, /* ipsec初期設定 */
  COM_IPSEC_INFO,     /* ipsec鍵情報 */
  COM_STAT,     /* ipsec統計情報 */
  COM_LOAD,
  COM_UNLOAD,   
  COM_DEBUG,    /* debug用スタブon/off */
  COM_TIMEOUT,  /* 通信実デバイス監視用タイマ設定 */
  COM_UDP_CHK,  /* UDPチェックサムチェック方法設定 */
  COM_DEBUG_GET,/* デバッグ情報取得 */
  COM_SENDING_N,/* 最大送信数設定 */
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
  COM_VIANA_INIT,   /* UDP/ESPパケットフィルタ開始 */
  COM_VIANA_EXIT,   /* UDP/ESPパケットフィルタ停止 */
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add end */
};
  
enum _KIND{
  KIND_SET = 1,   /* 設定 */
  KIND_CLEAR,     /* クリア */
  KIND_QUERY,     /* 情報取得要求 */
};

/* 鍵情報設定時のエラーコード */
enum _ERROR_CODE{
  ERROR_CODE_SUCCESS,       /* 0 設定成功 */
  ERROR_CODE_ZERO_VAL,      /* 1 設定値0 */
  ERROR_CODE_INVALID_VAL,   /* 2 無効な値 */
  ERROR_CODE_IKE_LIFE_ZERO, /* 3 IKEの有効時間0 */
  ERROR_CODE_MASK_DST_ZERO, /* 4 maskしたdstアドレスが0 */
  ERROR_CODE_ZERO_ADDR,     /* 5 下位8bitが0のアドレス */
  ERROR_CODE_BCAST_ADDR,    /* 6 下位8bitが0xffのアドレス */
  ERROR_CODE_NOT_FOUND,     /* 7 値が見つからない */
  ERROR_CODE_NOT_ENOUGH,    /* 8 大きさが不十分 */
  ERROR_CODE_SIZE_NOT_MATCH,/* 9 サイズが一致しない */
  ERROR_CODE_RCV_SA_FULL,   /* 10 受信SAがいっぱい */
};

/* 鍵情報設定時のエラーポイント */
enum _ERROR_KIND{
  ERROR_KIND_SUCCESS,        /* 0 設定成功 */
  ERROR_KIND_IPSEC_MODE,     /* 1 ipsecモード */
  ERROR_KIND_IPSEC_PROTOCOL, /* 2 protocol */
  ERROR_KIND_OWN_PORT,       /* 3 自UDPポート番号 */
  ERROR_KIND_REMORT_PORT,    /* 4 相手UDPポート番号 */
  ERROR_KIND_AH_ALGO,        /* 5 AHアルゴリズム */
  ERROR_KIND_ESP_ALGO,       /* 6 ESPアルゴリズム */
  ERROR_KIND_LIFETIME,       /* 7 鍵の有効時間 */
  ERROR_KIND_DIRECTION,      /* 8 鍵の方向 */
  ERROR_KIND_SPI,            /* 9 SPI */
  ERROR_KIND_IP_MASK,        /* 10 IPmask */
  ERROR_KIND_DST_IP,         /* 11 送信先IPアドレス(トンネル内側) */
  ERROR_KIND_TUN_SRC,        /* 12 送信元IPアドレス(トンネル外側) */
  ERROR_KIND_TUN_DST,        /* 13 送信先IPアドレス(トンネル外側) */
  ERROR_KIND_REAL_DEV_NAME,  /* 14 通信実デバイス名 */
  ERROR_KIND_MSG_LEN,        /* 15 メッセージ長 */
  ERROR_KIND_TIMEOUT_CYCLE,  /* 16 タイムアウト間隔 */
  ERROR_KIND_RCV_SA,         /* 17 受信SA */
  ERROR_KIND_VIRTUAL_DEV,    /* 18 仮想デバイス */
  ERROR_KIND_SENDING_NUM,    /* 19 送信処理中カウンタ */
  ERROR_KIND_REAL_DEV,       /* 20 通信実デバイス */
  ERROR_KIND_KEY_MODE,       /* 21 鍵の設定モード */
  ERROR_KIND_DEBUG_MODE,     /* 22 デバッグモード */
  ERROR_KIND_UDP_CHK_MODE,   /* 23 UDPチェックサムチェックモード */
};


/* デバッグメッセージの出力調整 */
enum _DEBUG_MODE{
  DEBUG_ON = 1,
  DEBUG_OFF,
};

/* UDP checksum のチェック方法 */
enum _UDP_CHK_MODE{
  NO_CHECK = 1, /* UDPのチェックサムをチェックしない */
  NULL_OK,      /* UDPのチェックサムNULLを許可する */
  NULL_NG,      /* UDPのチェックサムNULLを拒否する */
};

/* エラー情報 */
typedef struct _ERROR_INFO
{
  int error_code;
  int error_kind;
}ERROR_INFO, *PERROR_INFO;

/* SPI情報 */
typedef struct _SPI_INFO
{
  unsigned int out_spi;
  unsigned int in_spi;
}SPI_INFO, *PSPI_INFO;

/* ドライバーアプリ間メッセージヘッダ */
typedef struct _MSG_HDR
{
  unsigned long command; /* コマンド */
  unsigned long kind;    /* 種別 */
  ERROR_INFO error_info; /* エラー情報 */
  unsigned long length;  /* ヘッダを除くメッセージ長 */
}MSG_HDR, *PMSG_HDR;

/* NAT関連情報 */
typedef struct _NAT_INFO
{
  int enable;
  u_short own_port;    /* 自ポート番号 */
  u_short remort_port; /* 相手ポート番号 */
}__attribute__((packed))NAT_INFO,*PNAT_INFO;


/* IPSEC初期化情報 */
typedef struct _INIT_INFO
{
  NAT_INFO nat_info;
  char real_dev_name[DEV_NAME_LEN];
}__attribute__((packed))INIT_INFO,*PINIT_INFO;


/* IPSEC初期化情報設定メッセージ */
typedef struct _INIT_INFO_SET_MSG
{
  MSG_HDR msg_hdr;
  INIT_INFO init_info;
}__attribute__((packed))INIT_INFO_SET_MSG,*PINIT_INFO_SET_MSG;


/* IPSEC初期化情報クリアメッセージ */
typedef struct _INIT_INFO_CLEAR_MSG
{
  MSG_HDR msg_hdr;
}__attribute__((packed))INIT_INFO_CLEAR_MSG,*PINIT_INFO_CLEAR_MSG;


/* IPSEC初期化情報取得メッセージ */
typedef struct _INIT_INFO_QUERY_MSG
{
  MSG_HDR msg_hdr;
  INIT_INFO init_info;
}__attribute__((packed))INIT_INFO_QUERY_MSG,*PINIT_INFO_QUERY_MSG;


enum _SA_VALID{
  SA_INVALID,
  SA_VALID,
};

/* IPSec関連情報 */
typedef struct _IPSEC_INFO
{
  int valid;
  struct set_ipsec set;
}__attribute__((packed))IPSEC_INFO,*PIPSEC_INFO;


/* IPSec関連情報設定メッセージ */
typedef struct _IPSECINFO_SET_MSG
{
  MSG_HDR msg_hdr;
  IPSEC_INFO ipsec_info;
}__attribute__((packed))IPSECINFO_SET_MSG,*PIPSECINFO_SET_MSG;


/* 鍵特定情報 */
typedef struct _KEY_ID_INFO
{
  unsigned int  spi;
  unsigned long tun_dst;   /* Tunnel Dst IP-Address (network byte order) */
  unsigned int  protocol;
  int direction;
}__attribute__((packed))KEY_ID_INFO,*PKEY_ID_INFO;

  

/* 鍵情報クリアメッセージ */
typedef struct _IPSECINFO_CLEAR_MSG
{
  MSG_HDR msg_hdr;
  KEY_ID_INFO key_id_info;
}__attribute__((packed))IPSECINFO_CLEAR_MSG,*PIPSECINFO_CLEAR_MSG;



/* 鍵情報要求メッセージ */
typedef struct _IPSECINFO_QUERY_MSG
{
  MSG_HDR msg_hdr;
  KEY_ID_INFO key_id_info;
  unsigned long ipsec_info_num; /* ipsec情報の数 */
  IPSEC_INFO ipsec_info[1];     /* ipsec情報 */
}__attribute__((packed))IPSECINFO_QUERY_MSG,*PIPSECINFO_QUERY_MSG;


/* 統計情報クリアメッセージ */
typedef struct _IPSECSTAT_CLEAR_MSG
{
  MSG_HDR msg_hdr;
}__attribute__((packed))IPSECSTAT_CLEAR_MSG,*PIPSECSTAT_CLEAR_MSG;


/* 統計情報取得メッセージ */ 
typedef struct _IPSECSTAT_GET_MSG
{
  MSG_HDR msg_hdr;
  struct ipsecstat ipsec_stat;
}__attribute__((packed))IPSECSTAT_GET_MSG,*PIPSECSTAT_GET_MSG;


/* debugモード変更メッセージ  */
typedef struct _DEBUG_MODE_MSG
{
  MSG_HDR msg_hdr;
  int debug_mode;
}__attribute__((packed))DEBUG_MODE_MSG,*PDEBUG_MODE_MSG;


/* 実デバイス監視タイムアウト情報 */
typedef struct _TIMEOUT_INFO
{
  unsigned int timeout_cycle;
  unsigned int timeout_count;
}__attribute__((packed))TIMEOUT_INFO,*PTIMEOUT_INFO;

  
/* 実デバイス監視タイムアウト情報設定メッセージ */
typedef struct _TIMEOUT_SET_MSG
{
  MSG_HDR msg_hdr;
  TIMEOUT_INFO timeout_info;
}__attribute__((packed))TIMEOUT_SET_MSG,*PTIMEOUT_SET_MSG;


/* 実デバイス監視タイムアウト情報取得メッセージ */
typedef struct _TIMEOUT_QUERY_MSG
{
  MSG_HDR msg_hdr;
  TIMEOUT_INFO timeout_info;
}__attribute__((packed))TIMEOUT_QUERY_MSG,*PTIMEOUT_QUERY_MSG;


/* UDPチェックサムモード設定メッセージ */
typedef struct _UDP_CHK_MODE_MSG
{
  MSG_HDR msg_hdr;
  int udp_check_mode;
}__attribute__((packed))UDP_CHK_MODE_MSG,*PUDP_CHK_MODE_MSG;


/* 最大送信数設定メッセージ */
typedef struct _SENDING_NUM_MSG
{
  MSG_HDR msg_hdr;
  int max_sending_num;
}__attribute__((packed))SENDING_NUM_MSG,*PSENDING_NUM_MSG;


/* 各SAでの復号回数構造体 */
typedef struct _DEC_COUNT
{
  unsigned long sa0;
  unsigned long sa1;
}__attribute__((packed))DEC_COUNT,*PDEC_COUNT;


typedef struct DRV_STAT_
{
  int realdev_queue_stop_num;   /* 通信実デバイスのqueueがstopした回数 */
  int rcv_skb_copy_count;       /* 受信したskbをcopyした回数 */
  int udp_checksum_error_count; /* UDP checksum エラーの数 */
}__attribute__((packed))DRV_STAT,*PDRV_STAT;


typedef struct _DEBUG_DRV_INFO
{
  DEC_COUNT dec_count;         /* 各SAでの復号回数構造体 */
  int now_key_no;              /* 現在使用中の鍵番号(送信) */
  char drv_ver[DRV_VER_LEN];   /* driver version */
  int vdev_queue_stopped;      /* 仮想デバイスのqueueがstopしているか */
  int current_sending;         /* 現在送信処理中の数 */
  DRV_STAT drv_stat;           /* ドライバの持つ統計情報 */
}__attribute__((packed))DEBUG_DRV_INFO,*PDEBUG_DRV_INFO;


/* デバッグ情報取得メッセージ */
typedef struct _DEBUG_GET_MSG
{
  MSG_HDR msg_hdr;
  DEBUG_DRV_INFO debug_drv_info;
}__attribute__((packed))DEBUG_GET_MSG,*PDEBUG_GET_MSG;



