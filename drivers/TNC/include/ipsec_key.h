
enum _DIRECTION{
  OUT_KEY = 1,
  IN_KEY,
};


enum _IPSEC_KEY_MODE{
  IPSEC_KEY_MANUAL,
  IPSEC_KEY_IKE,
};

/* mode of security protocol */
/* NOTE: DON'T use IPSEC_MODE_ANY at SPD.  It's only use in SAD */
enum _IPSEC_MODE{
  IPSEC_MODE_ANY,
  IPSEC_MODE_TRANSPORT,
  IPSEC_MODE_TUNNEL,
  IPSEC_MODE_UDP_TUNNEL,
};
/* RFC2367 numbers - meets RFC2407 */
#define SADB_AALG_NONE          0
#define SADB_AALG_MD5HMAC       2
#define SADB_AALG_SHA1HMAC      3
/* #define SADB_AALG_DESHMAC       4 */ /* Ì¤ÄêµÁ */
#define SADB_AALG_MAX           8
/* private allocations - based on RFC2407/IANA assignment */
#define SADB_X_AALG_SHA2_224    5
#define SADB_X_AALG_SHA2_256    6
#define SADB_X_AALG_SHA2_384    7
#define SADB_X_AALG_SHA2_512    8
/* private allocations should use 249-255 (RFC2407) */
/* #define SADB_X_AALG_MD5         249 */ /* Keyed MD5 */
/* #define SADB_X_AALG_SHA         250 */ /* Keyed SHA */
/* #define SADB_X_AALG_AES         251 */
#define SADB_X_AALG_NULL        251 /* null authentication */
#define SADB_ORG_AALG_NULL      100

/* RFC2367 numbers - meets RFC2407 */
#define SADB_EALG_NONE          0
#define SADB_EALG_DESCBC        2
#define SADB_EALG_3DESCBC       3
#define SADB_EALG_NULL          11
#define SADB_EALG_MAX           12
/* #define SADB_EALG_AESCBC        12 */ /* SADB_X_EALG_AES, SADB_X_EALG_RIJNDAELCBC¤È3¼ïÎà»ÈÍÑ */
/* #define SADB_X_EALG_DES_IV64    1  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_RC5         4  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_IDEA        5  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_CAST128CBC  6  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_BLOWFISHCBC 7  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_3IDEA       8  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_DES_IV32    9  (Ì¤ÄêµÁ) */
/* #define SADB_X_EALG_RC4         10 (Ì¤ÄêµÁ) */
/* private allocations - based on RFC2407/IANA assignment */
/* #define SADB_X_EALG_RIJNDAELCBC 12 */
#define SADB_X_EALG_AES         12
/* private allocations should use 249-255 (RFC2407) */

#define SADB_SPI_KEY_LEN        128

struct key_spi {
  unsigned int  auth_key_len;        /* Auth key length */
  unsigned char auth_key_val[SADB_SPI_KEY_LEN];
                                     /* Auth Key value */
  unsigned int  enc_key_len;         /* Enc key length */
  unsigned char enc_key_val[SADB_SPI_KEY_LEN];
                                     /* Enc Key value */
  unsigned int spi;                  /* SPI */
}__attribute__((packed));


struct set_ipsec {
  unsigned int  mode;            /* Tunnel / Transport (_IPSEC_MODE) */
  unsigned int  protocol;        /* AH / ESP / None (IPPROTO_*) */
  unsigned int  AH_algo;         /* HMAC-MD5 / HMAC-SHA1 / None (SADB_*) */
  unsigned int  ESP_algo;        /* DES / 3DES / AES / None (SADB_*) */
  unsigned int  key_mode;        /* IKE / Manual (_IPSEC_KEY_MODE) */
  unsigned int  lifeTime;        /* Only IKE */
  struct key_spi key;            /* Key-Value */
  unsigned long	 src_ip;         /* Org Src IP-Address for SPD */
  unsigned long  dst_ip;         /* Org Dst IP-Address for SPD */
  unsigned long  ip_mask;        /* Org Dst IP Mask for SPD */
  unsigned long  tun_src;        /* Tunnel Src IP-Address */
  unsigned long  tun_dst;        /* Tunnel Dst IP-Address */
  unsigned int   direction;      /* OUT:1 IN:2 */
}__attribute__((packed));



/* statistics for ipsec processing */
struct ipsecstat {
	u_int in_success;     /* succeeded inbound process */
			/* security policy violation for inbound process */
	u_int in_nosa;        /* inbound SA is unavailable */
	u_int in_inval;       /* inbound processing failed due to EINVAL */
	u_int in_badspi;      /* failed getting a SPI */
	u_int in_ahreplay;    /* AH replay check failed */
	u_int in_espreplay;   /* ESP replay check failed */
	u_int in_ahauthsucc;  /* AH authentication success */
	u_int in_ahauthfail;  /* AH authentication failure */
	u_int in_espauthsucc; /* ESP authentication success */
	u_int in_espauthfail; /* ESP authentication failure */
	u_int in_esphist;
	u_int in_ahhist;
	u_int out_success;    /* succeeded outbound process */
	u_int out_polvio;
			/* security policy violation for outbound process */
	u_int out_inval;      /* outbound process failed due to EINVAL */
	u_int out_esphist;
	u_int out_ahhist;
}__attribute__((packed));

