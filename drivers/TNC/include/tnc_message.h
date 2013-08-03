/*
 *******************************************************************
 File Name	: tnc_message.h

 Description	: "P2P トンネルデバイス API 定義"

 Remark		: 

 Copyright (c) 2004 by Panasonic Mobile Communications Co., Ltd.
 作成日:	2004/02/10 [kaneko]
 修正日:	2011/06/30 [Yoshino]
 *******************************************************************
 */

#ifndef _tnc_message_h_
#define _tnc_message_h_

#include	"p2p_client_common.h"

#define TNC_SPI_KEY_LENGTH		128

/* パラメータ型定義 */
typedef struct TNC_NAT_info {
  unsigned short local_port;     /* local UDP port for NAT traversal */
  unsigned short remote_port;    /* remote UDP port for NAT traversal */
  unsigned long  src_ip;         /* Virtual(inner) Src IP-Address for SPD */
  unsigned long  dst_ip;         /* Virtual(inner) Dst IP-Address for SPD */
  unsigned long  tun_src;        /* Tunnel(outer) Src IP-Address */
  unsigned long  tun_dst;        /* Tunnel(outer) Dst IP-Address */
  char real_dev_name[TNC_DEV_NAME_LEN];
} ST_TNC_NAT_info_t;

struct TNC_key_spi {
  unsigned int  auth_key_len;        /* Auth key length */
  unsigned char auth_key_val[TNC_SPI_KEY_LENGTH];   /* Auth Key value */
  unsigned int  enc_key_len;         /* Enc key length */
  unsigned char enc_key_val[TNC_SPI_KEY_LENGTH];    /* Enc Key value */
  SPIno    	spi;		     /* SPI */
};

typedef struct TNC_SA_info {
  unsigned int   mode;            /* Tunnel / Transport(unused) */
  unsigned int   protocol;        /* ESP / Adhoc(unused) */
  UINT8          auth_algo;       /* HMAC-MD5 / HMAC-SHA1 / None (SADB_*) */
  UINT8          enc_algo;        /* DES / 3DES / AES / None (SADB_*) */
  unsigned long   lifetime;       /* unused->used */
  struct  TNC_key_spi key;        /* Key-Value */
  unsigned long	 src_ip;          /* Virtual(inner) Src IP-Address(unused) */
  unsigned long  dst_ip;          /* Virtual(inner) Dst IP-Address(only OUT) */
  unsigned short src_port;        /* Virtual(inner) Src port (ANY) */
  unsigned short dst_port;        /* Virtual(inner) Dst port (ANY) */
  unsigned long  tun_src;         /* Tunnel(outer) Src IP-Address */
  unsigned long  tun_dst;         /* Tunnel(outer) Dst IP-Address */
  unsigned int   direction;       /* TNC_DIR_OUTBOUND:1 TNC_DIR_INBOUND:2 */  
} ST_TNC_SA_info_t;

typedef struct TNC_Statistic {
  unsigned int in_success;	 /* succeeded inbound process */
  unsigned int in_nosa;		 /* inbound SA is unavailable */
  unsigned int in_inval;	 /* inbound processing failed due to EINVAL */
  unsigned int in_badspi;	 /* failed getting a SPI */

  unsigned int in_espreplay;	 /* ESP replay check failed */
  unsigned int in_espauthsucc;	 /* ESP authentication success */
  unsigned int in_espauthfail;	 /* ESP authentication failure */

  unsigned int out_success;	 /* succeeded outbound process */
  unsigned int out_polvio;
			/* security policy violation for outbound process */
  unsigned int out_inval;	 /* outbound process failed due to EINVAL */

} ST_TNC_Statistic_t;


/* API プロトタイプ宣言 */
extern int TNC_SetUDPTunnel(TunnelID tunnelID, ST_TNC_NAT_info_t *info);

extern int TNC_ClearUDPTunnel(TunnelID tunnelID);

extern int TNC_SetSA(TunnelID tunnelID, ST_TNC_SA_info_t *info);

extern int TNC_ClearSA(TunnelID tunnelID, SPIno spi, int direction);

extern int TNC_GetStatictic(TunnelID tunnelID, ST_TNC_Statistic_t *stat);

extern int TNC_ChangeMTU(TunnelID tunnelID, int mtu);

#endif 	/*  _tnc_message_h_  */
