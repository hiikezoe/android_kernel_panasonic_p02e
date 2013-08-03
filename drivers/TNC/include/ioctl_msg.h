/*
 * ioctl_msg.h
 *
 * I/O control ��å��������
 *
 */

#define NAT_TRAV_ENABLE  1
#define NAT_TRAV_DISABLE 0
#define DEV_NAME_LEN     32
#define MAX_SA_NUM       4
#define DRV_VER_LEN      8

enum _COMMAND{
  COM_IPSEC_INIT = 1, /* ipsec������� */
  COM_IPSEC_INFO,     /* ipsec������ */
  COM_STAT,     /* ipsec���׾��� */
  COM_LOAD,
  COM_UNLOAD,   
  COM_DEBUG,    /* debug�ѥ�����on/off */
  COM_TIMEOUT,  /* �̿��¥ǥХ����ƻ��ѥ��������� */
  COM_UDP_CHK,  /* UDP�����å���������å���ˡ���� */
  COM_DEBUG_GET,/* �ǥХå�������� */
  COM_SENDING_N,/* �������������� */
/* PMC-Viana-019-�ѥ��åȥե��륿���ϥ����ߥ��ѹ� add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
  COM_VIANA_INIT,   /* UDP/ESP�ѥ��åȥե��륿���� */
  COM_VIANA_EXIT,   /* UDP/ESP�ѥ��åȥե��륿��� */
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-�ѥ��åȥե��륿���ϥ����ߥ��ѹ� add end */
};
  
enum _KIND{
  KIND_SET = 1,   /* ���� */
  KIND_CLEAR,     /* ���ꥢ */
  KIND_QUERY,     /* ��������׵� */
};

/* ������������Υ��顼������ */
enum _ERROR_CODE{
  ERROR_CODE_SUCCESS,       /* 0 �������� */
  ERROR_CODE_ZERO_VAL,      /* 1 ������0 */
  ERROR_CODE_INVALID_VAL,   /* 2 ̵������ */
  ERROR_CODE_IKE_LIFE_ZERO, /* 3 IKE��ͭ������0 */
  ERROR_CODE_MASK_DST_ZERO, /* 4 mask����dst���ɥ쥹��0 */
  ERROR_CODE_ZERO_ADDR,     /* 5 ����8bit��0�Υ��ɥ쥹 */
  ERROR_CODE_BCAST_ADDR,    /* 6 ����8bit��0xff�Υ��ɥ쥹 */
  ERROR_CODE_NOT_FOUND,     /* 7 �ͤ����Ĥ���ʤ� */
  ERROR_CODE_NOT_ENOUGH,    /* 8 �礭�����Խ�ʬ */
  ERROR_CODE_SIZE_NOT_MATCH,/* 9 �����������פ��ʤ� */
  ERROR_CODE_RCV_SA_FULL,   /* 10 ����SA�����äѤ� */
};

/* ������������Υ��顼�ݥ���� */
enum _ERROR_KIND{
  ERROR_KIND_SUCCESS,        /* 0 �������� */
  ERROR_KIND_IPSEC_MODE,     /* 1 ipsec�⡼�� */
  ERROR_KIND_IPSEC_PROTOCOL, /* 2 protocol */
  ERROR_KIND_OWN_PORT,       /* 3 ��UDP�ݡ����ֹ� */
  ERROR_KIND_REMORT_PORT,    /* 4 ���UDP�ݡ����ֹ� */
  ERROR_KIND_AH_ALGO,        /* 5 AH���르�ꥺ�� */
  ERROR_KIND_ESP_ALGO,       /* 6 ESP���르�ꥺ�� */
  ERROR_KIND_LIFETIME,       /* 7 ����ͭ������ */
  ERROR_KIND_DIRECTION,      /* 8 �������� */
  ERROR_KIND_SPI,            /* 9 SPI */
  ERROR_KIND_IP_MASK,        /* 10 IPmask */
  ERROR_KIND_DST_IP,         /* 11 ������IP���ɥ쥹(�ȥ�ͥ���¦) */
  ERROR_KIND_TUN_SRC,        /* 12 ������IP���ɥ쥹(�ȥ�ͥ볰¦) */
  ERROR_KIND_TUN_DST,        /* 13 ������IP���ɥ쥹(�ȥ�ͥ볰¦) */
  ERROR_KIND_REAL_DEV_NAME,  /* 14 �̿��¥ǥХ���̾ */
  ERROR_KIND_MSG_LEN,        /* 15 ��å�����Ĺ */
  ERROR_KIND_TIMEOUT_CYCLE,  /* 16 �����ॢ���ȴֳ� */
  ERROR_KIND_RCV_SA,         /* 17 ����SA */
  ERROR_KIND_VIRTUAL_DEV,    /* 18 ���ۥǥХ��� */
  ERROR_KIND_SENDING_NUM,    /* 19 ���������楫���� */
  ERROR_KIND_REAL_DEV,       /* 20 �̿��¥ǥХ��� */
  ERROR_KIND_KEY_MODE,       /* 21 ��������⡼�� */
  ERROR_KIND_DEBUG_MODE,     /* 22 �ǥХå��⡼�� */
  ERROR_KIND_UDP_CHK_MODE,   /* 23 UDP�����å���������å��⡼�� */
};


/* �ǥХå���å������ν���Ĵ�� */
enum _DEBUG_MODE{
  DEBUG_ON = 1,
  DEBUG_OFF,
};

/* UDP checksum �Υ����å���ˡ */
enum _UDP_CHK_MODE{
  NO_CHECK = 1, /* UDP�Υ����å����������å����ʤ� */
  NULL_OK,      /* UDP�Υ����å�����NULL����Ĥ��� */
  NULL_NG,      /* UDP�Υ����å�����NULL����ݤ��� */
};

/* ���顼���� */
typedef struct _ERROR_INFO
{
  int error_code;
  int error_kind;
}ERROR_INFO, *PERROR_INFO;

/* SPI���� */
typedef struct _SPI_INFO
{
  unsigned int out_spi;
  unsigned int in_spi;
}SPI_INFO, *PSPI_INFO;

/* �ɥ饤�С����ץ�֥�å������إå� */
typedef struct _MSG_HDR
{
  unsigned long command; /* ���ޥ�� */
  unsigned long kind;    /* ���� */
  ERROR_INFO error_info; /* ���顼���� */
  unsigned long length;  /* �إå��������å�����Ĺ */
}MSG_HDR, *PMSG_HDR;

/* NAT��Ϣ���� */
typedef struct _NAT_INFO
{
  int enable;
  u_short own_port;    /* ���ݡ����ֹ� */
  u_short remort_port; /* ���ݡ����ֹ� */
}__attribute__((packed))NAT_INFO,*PNAT_INFO;


/* IPSEC��������� */
typedef struct _INIT_INFO
{
  NAT_INFO nat_info;
  char real_dev_name[DEV_NAME_LEN];
}__attribute__((packed))INIT_INFO,*PINIT_INFO;


/* IPSEC��������������å����� */
typedef struct _INIT_INFO_SET_MSG
{
  MSG_HDR msg_hdr;
  INIT_INFO init_info;
}__attribute__((packed))INIT_INFO_SET_MSG,*PINIT_INFO_SET_MSG;


/* IPSEC��������󥯥ꥢ��å����� */
typedef struct _INIT_INFO_CLEAR_MSG
{
  MSG_HDR msg_hdr;
}__attribute__((packed))INIT_INFO_CLEAR_MSG,*PINIT_INFO_CLEAR_MSG;


/* IPSEC��������������å����� */
typedef struct _INIT_INFO_QUERY_MSG
{
  MSG_HDR msg_hdr;
  INIT_INFO init_info;
}__attribute__((packed))INIT_INFO_QUERY_MSG,*PINIT_INFO_QUERY_MSG;


enum _SA_VALID{
  SA_INVALID,
  SA_VALID,
};

/* IPSec��Ϣ���� */
typedef struct _IPSEC_INFO
{
  int valid;
  struct set_ipsec set;
}__attribute__((packed))IPSEC_INFO,*PIPSEC_INFO;


/* IPSec��Ϣ���������å����� */
typedef struct _IPSECINFO_SET_MSG
{
  MSG_HDR msg_hdr;
  IPSEC_INFO ipsec_info;
}__attribute__((packed))IPSECINFO_SET_MSG,*PIPSECINFO_SET_MSG;


/* ��������� */
typedef struct _KEY_ID_INFO
{
  unsigned int  spi;
  unsigned long tun_dst;   /* Tunnel Dst IP-Address (network byte order) */
  unsigned int  protocol;
  int direction;
}__attribute__((packed))KEY_ID_INFO,*PKEY_ID_INFO;

  

/* �����󥯥ꥢ��å����� */
typedef struct _IPSECINFO_CLEAR_MSG
{
  MSG_HDR msg_hdr;
  KEY_ID_INFO key_id_info;
}__attribute__((packed))IPSECINFO_CLEAR_MSG,*PIPSECINFO_CLEAR_MSG;



/* �������׵��å����� */
typedef struct _IPSECINFO_QUERY_MSG
{
  MSG_HDR msg_hdr;
  KEY_ID_INFO key_id_info;
  unsigned long ipsec_info_num; /* ipsec����ο� */
  IPSEC_INFO ipsec_info[1];     /* ipsec���� */
}__attribute__((packed))IPSECINFO_QUERY_MSG,*PIPSECINFO_QUERY_MSG;


/* ���׾��󥯥ꥢ��å����� */
typedef struct _IPSECSTAT_CLEAR_MSG
{
  MSG_HDR msg_hdr;
}__attribute__((packed))IPSECSTAT_CLEAR_MSG,*PIPSECSTAT_CLEAR_MSG;


/* ���׾��������å����� */ 
typedef struct _IPSECSTAT_GET_MSG
{
  MSG_HDR msg_hdr;
  struct ipsecstat ipsec_stat;
}__attribute__((packed))IPSECSTAT_GET_MSG,*PIPSECSTAT_GET_MSG;


/* debug�⡼���ѹ���å�����  */
typedef struct _DEBUG_MODE_MSG
{
  MSG_HDR msg_hdr;
  int debug_mode;
}__attribute__((packed))DEBUG_MODE_MSG,*PDEBUG_MODE_MSG;


/* �¥ǥХ����ƻ륿���ॢ���Ⱦ��� */
typedef struct _TIMEOUT_INFO
{
  unsigned int timeout_cycle;
  unsigned int timeout_count;
}__attribute__((packed))TIMEOUT_INFO,*PTIMEOUT_INFO;

  
/* �¥ǥХ����ƻ륿���ॢ���Ⱦ��������å����� */
typedef struct _TIMEOUT_SET_MSG
{
  MSG_HDR msg_hdr;
  TIMEOUT_INFO timeout_info;
}__attribute__((packed))TIMEOUT_SET_MSG,*PTIMEOUT_SET_MSG;


/* �¥ǥХ����ƻ륿���ॢ���Ⱦ��������å����� */
typedef struct _TIMEOUT_QUERY_MSG
{
  MSG_HDR msg_hdr;
  TIMEOUT_INFO timeout_info;
}__attribute__((packed))TIMEOUT_QUERY_MSG,*PTIMEOUT_QUERY_MSG;


/* UDP�����å�����⡼�������å����� */
typedef struct _UDP_CHK_MODE_MSG
{
  MSG_HDR msg_hdr;
  int udp_check_mode;
}__attribute__((packed))UDP_CHK_MODE_MSG,*PUDP_CHK_MODE_MSG;


/* ���������������å����� */
typedef struct _SENDING_NUM_MSG
{
  MSG_HDR msg_hdr;
  int max_sending_num;
}__attribute__((packed))SENDING_NUM_MSG,*PSENDING_NUM_MSG;


/* ��SA�Ǥ���������¤�� */
typedef struct _DEC_COUNT
{
  unsigned long sa0;
  unsigned long sa1;
}__attribute__((packed))DEC_COUNT,*PDEC_COUNT;


typedef struct DRV_STAT_
{
  int realdev_queue_stop_num;   /* �̿��¥ǥХ�����queue��stop������� */
  int rcv_skb_copy_count;       /* ��������skb��copy������� */
  int udp_checksum_error_count; /* UDP checksum ���顼�ο� */
}__attribute__((packed))DRV_STAT,*PDRV_STAT;


typedef struct _DEBUG_DRV_INFO
{
  DEC_COUNT dec_count;         /* ��SA�Ǥ���������¤�� */
  int now_key_no;              /* ���߻�����θ��ֹ�(����) */
  char drv_ver[DRV_VER_LEN];   /* driver version */
  int vdev_queue_stopped;      /* ���ۥǥХ�����queue��stop���Ƥ��뤫 */
  int current_sending;         /* ��������������ο� */
  DRV_STAT drv_stat;           /* �ɥ饤�Фλ������׾��� */
}__attribute__((packed))DEBUG_DRV_INFO,*PDEBUG_DRV_INFO;


/* �ǥХå����������å����� */
typedef struct _DEBUG_GET_MSG
{
  MSG_HDR msg_hdr;
  DEBUG_DRV_INFO debug_drv_info;
}__attribute__((packed))DEBUG_GET_MSG,*PDEBUG_GET_MSG;



