/*
 * ipsec_core.h
 *
 */
#ifndef _IPSEC_CORE_H
#define	_IPSEC_CORE_H

#include "DumpData.h"	/* RAND_Bytes */
#ifdef KERNEL_NO_TNC_BUILD
#include <dtvrecdd/esp_ipsec_key.h>
#else
#include "ipsec_key.h"
#endif

/*
 * Security Policy Index
 * NOTE: Ensure to be same address family and upper layer protocol.
 * NOTE: ul_proto, port number, uid, gid:
 *	ANY: reserved for waldcard.
 *	0 to (~0 - 1): is one of the number of each value.
 */
struct secpolicyindex {
  u_char  dir;			/* direction of packet flow, see blow */
  struct  in_addr src;	        /* IP src address for SP */
  struct  in_addr dst;	        /* IP dst address for SP */
  u_char  prefs;		/* prefix length in bits for src */
  u_char  prefd;		/* prefix length in bits for dst */
  u_short ul_proto;		/* upper layer Protocol */
}__attribute__((packed));


/* Security Policy Data Base */
struct secpolicy {
	int refcnt;			/* reference count */
	struct secpolicyindex spidx;	/* selector */
	u_int id;			/* It's unique number on the system. */
	u_int state;			/* 0: dead, others: alive */
	u_int lifetime;                 /* Add ike */
#define IPSEC_SPSTATE_DEAD	0
#define IPSEC_SPSTATE_ALIVE	1
	u_int policy;		/* DISCARD, NONE or IPSEC, see keyv2.h */
  /* #define IPSEC_KEY_MANUAL        0 */
  /* #define IPSEC_KEY_IKE           1 */
	u_int key_mode;                 /* 0:Manual-key, 1:IKE */
#define IPSEC_KEY_IKE_PKI       1
	u_int key_type;                 /* 0:Pre-Shared, 1:PKI */
	struct ipsecrequest *req;
				/* pointer to the ipsec request tree, */
				/* if policy == IPSEC else this value == NULL.*/
}__attribute__((packed));




/* Request for IPsec */
struct ipsecrequest {
	struct ipsecrequest *next;
				/* pointer to next structure */
				/* If NULL, it means the end of chain. */
	struct secasindex saidx;/* hint for search proper SA */
				/* if __ss_len == 0 then no address specified.*/
	u_int level;		/* IPsec level defined below. */

  	struct secasvar *sav;	/* place holder of SA for use */
	struct secpolicy *sp;	/* back pointer to SP */
}__attribute__((packed));


/* security policy in PCB */
struct inpcbpolicy {
	struct secpolicy *sp_in;
	struct secpolicy *sp_out;
	int priv;			/* privileged socket ? */
};

/* SP acquiring list table. */
struct secspacq {
	struct secpolicyindex spidx;
	u_int  tick;		/* for lifetime */
	int count;		/* for lifetime */
	/* XXX: here is mbuf place holder to be sent ? */
}__attribute__((packed));


#define HASHB_LEN       384
#define HASHB_TAG       ((u_long)"HASH_B")
#define HASHM_LEN       64
#define HASHM_TAG       ((u_long)"HASH_M")
#define MBUF_TAG        ((u_long)"MBUF")
#define DESKS_LEN       768
#define DESKS_TAG       ((u_long)"DES_KS")

//#define MB_MAX    10
#define MB_MAX    2
#define MB_LEN    128

#define mtod(m,t)       ((t)((m)->m_data))

struct mbuf {
  int use;
  int flg;
  u_char *m_data;
  int m_len;
  int offset;
  int m_total;
  struct mbuf *m_next;
  u_char buf[MB_LEN];
}__attribute__((packed));

struct ipsec_output_state {
	struct mbuf *m;
	struct in_addr dst;
}__attribute__((packed));


struct default_key  {
  u_char alg_auth;
  u_char alg_enc;
  u_int  spi_out;
  u_int  spi_in;
}__attribute__((packed));


/* according to IANA assignment, port 0x0000 and proto 0xff are reserved. */
#define IPSEC_PORT_ANY		0
#define IPSEC_ULPROTO_ANY	255
#define IPSEC_PROTO_ANY		255

/*
 * Direction of security policy.
 * NOTE: Since INVALID is used just as flag.
 * The other are used for loop counter too.
 */
#define IPSEC_DIR_ANY		0
#define IPSEC_DIR_INBOUND	1
#define IPSEC_DIR_OUTBOUND	2
#define IPSEC_DIR_MAX		3
#define IPSEC_DIR_INVALID	4

/* Policy level */
/*
 * IPSEC, ENTRUST and BYPASS are allowd for setsockopt() in PCB,
 * DISCARD, IPSEC and NONE are allowd for setkey() in SPD.
 * DISCARD and NONE are allowd for system default.
 */
#define IPSEC_POLICY_DISCARD	0	/* discarding packet */
#define IPSEC_POLICY_NONE	1	/* through IPsec engine */
#define IPSEC_POLICY_IPSEC	2	/* do IPsec */
#define IPSEC_POLICY_ENTRUST	3	/* consulting SPD if present. */
#define IPSEC_POLICY_BYPASS	4	/* only for privileged socket. */

/* Security protocol level */
#define	IPSEC_LEVEL_DEFAULT	0	/* reference to system default */
#define	IPSEC_LEVEL_USE		1	/* use SA if present. */
#define	IPSEC_LEVEL_REQUIRE	2	/* require SA. */
#define	IPSEC_LEVEL_UNIQUE	3	/* unique SA. */

#define IPSEC_MANUAL_REQID_MAX	0x3fff
				/*
				 * if security policy level == unique, this id
				 * indicate to a relative SA for use, else is
				 * zero.
				 * 1 - 0x3fff are reserved for manual keying.
				 * 0 are reserved for above reason.  Others is
				 * for kernel use.
				 * Note that this id doesn't identify SA
				 * by only itself.
				 */
#define IPSEC_REPLAYWSIZE  32


#define ECN_ALLOWED     1       /* ECN allowed */
#define ECN_FORBIDDEN   0       /* ECN forbidden */
#define ECN_NOCARE      (-1)    /* no consideration to ECN */

/* ECN bits proposed by Sally Floyd */
#define IPTOS_CE                0x01    /* congestion experienced */
#define IPTOS_ECT               0x02    /* ECN-capable transport */
 
#define	IV_LEN			8
#define MAXIVLEN                16

#define PORT_ISAKMP             500

typedef struct _D_KEY {
int   flg;
struct sadb_key_d out_akey;
struct sadb_key_d in_akey;
struct sadb_key_d out_ekey;
struct sadb_key_d in_ekey;
}__attribute__((packed))DKEY,*PDKEY;



enum _KEY_INDEX{
  FIRST_OUT,
  FIRST_IN,
  SECOND_OUT,
  SECOND_IN,
  KEY_NUM,
};

typedef struct _IPSEC {
  int init;
  struct timer_list Key_LifeTimer[KEY_NUM];
  int life_timer[KEY_NUM];
  struct ipsecrequest def_isr[KEY_NUM];
  struct secasvar def_sa[KEY_NUM];
#define	BM_RECV_BUFF	2
#define	BM_SEND_BUFF	2
  struct mbuf bm_buff[BM_SEND_BUFF];
  struct mbuf bm_buff2[BM_RECV_BUFF];
  struct secpolicy ip4_def_policy;
  struct ipsecstat ipsec_stat;

  int ip4_ah_cleartos;
  int ip4_ah_offsetmask;
  int ip4_ipsec_dfbit;
  int ip4_ipsec_ecn;
  u_long dst_ip;
  u_long dst_mask;

  u_short ip_id;
  int now_key_no;
  DKEY  key[2];
  u_char ivbuf[2][MAXIVLEN];
  u_char *hash_buf;
  u_char *hash_mbuf;
  u_char *sched[4];
  struct mbuf *MBuf;
}__attribute__((packed))IPSEC,*PIPSEC;



#define IPSEC_RSV      64

#define	IPSEC_SEND		1
#define	IPSEC_RECV		2

/*
  There should be no DbgPrint's in the Free version of the driver
*/
#ifdef DBG
/* PMC-Viana-002-ログ出力対応 add start */
#ifndef BUILD_ANDROID
#define dbg_printf(Fmt)	do {		\
	KdPrint(Fmt);			\
} while (0)
#else /* BUILD_ANDROID */
#define dbg_printf(Fmt) printk(Fmt)
#endif /* BUILD_ANDROID */
/* PMC-Viana-002-ログ出力対応 add end */
#else
#define dbg_printf(Fmt)
#define DEBUG_PR(Fmt)
#endif 


int ipsec_attach( void* );
void ipsec_detach( void* );
int ipsec_init( void* );
int  ipsec_key_set( void*, struct set_ipsec* );

int    ipsec4_encapsulate( PIPSEC, struct mbuf* );
void   ip_ecn_ingress(int, u_int8_t *, u_int8_t* );
struct secasvar *get_sav( PIPSEC, u_long, u_long, u_char, int );
int    ipsec4_tunnel_validate( struct ip *, u_int, struct secasvar* );
struct mbuf *in_mbuf_set( PIPSEC, u_char *, int );
struct mbuf *out_state_set( PIPSEC, u_char*, int, struct ipsec_output_state* );
int    ipsec4_output( PIPSEC, struct ipsec_output_state* );
u_char *restore_mbuf( struct mbuf*, int* );
void   ipsec_stat_print(PIPSEC);

int key_setsaval( PIPSEC, struct set_ipsec* );


void init_mbuf(void*);
struct mbuf *mget(PIPSEC);
void mfree( struct mbuf *);
void mfree_m( struct mbuf *);
void m_copydata( struct mbuf *, int, int, caddr_t );

u_int32_t key_do_getnewspi(void);
void ipsec_Timeup0(unsigned long FunctionContext);
void ipsec_Timeup1(unsigned long FunctionContext);

void ipsec_Timeup0_in(unsigned long FunctionContext);
void ipsec_Timeup1_in(unsigned long FunctionContext);

#endif		// _IPSEC_CORE_H


