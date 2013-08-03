/*
 * keydb.h
 *
 */


/* Security Assocciation Index */
/* NOTE: Ensure to be same address family */
struct secasindex {
  struct in_addr  src;	        /* srouce address for SA */
  struct in_addr  dst;	        /* destination address for SA */
  u_short proto;		/* IPPROTO_ESP or IPPROTO_AH */
  u_char  mode;		        /* mode of protocol, see ipsec.h */
  u_int   reqid;		/* reqid id who owned this SA */
				/* see IPSEC_MANUAL_REQID_MAX. */
}__attribute__((packed));

struct sadb_key {
  u_int16_t sadb_key_len;
  u_int16_t sadb_key_exttype;
  u_int16_t sadb_key_bits;
  u_int16_t sadb_key_reserved;
}__attribute__((packed));


struct sadb_key_d {
  u_int16_t sadb_key_len;
  u_int16_t sadb_key_exttype;
  u_int16_t sadb_key_bits;
  u_int16_t sadb_key_reserved;
  u_char key[128];
}__attribute__((packed));


struct sadb_ident {
  u_int16_t sadb_ident_len;
  u_int16_t sadb_ident_exttype;
  u_int16_t sadb_ident_type;
  u_int16_t sadb_ident_reserved;
  u_int32_t sadb_ident_id;
}__attribute__((packed));
 

/* replay prevention */
struct secreplay {
        u_int32_t count;
        u_int wsize;            /* window size, i.g. 4 bytes */
        u_int32_t seq;          /* used by sender */
        u_int32_t lastseq;      /* used by receiver */   
        caddr_t bitmap;         /* used by receiver */
        int overflow;           /* overflow flag */
}__attribute__((packed));


#ifdef notdef
/* Security Association Data Base */
struct secashead {
        struct secasindex saidx;

        struct sadb_ident *idents;      /* source identity */
        struct sadb_ident *identd;      /* destination identity */
                                        /* XXX I don't know how to use them. */

        u_int8_t state;                 /* MATURE or DEAD. */
        LIST_HEAD(_satree, secasvar) savtree[SADB_SASTATE_MAX+1];
                                        /* SA chain */
                                        /* The first of this list is newer SA */

        struct route sa_route;          /* route cache */
}__attribute__((packed));

#endif
/* Security Association */
struct secasvar {
        struct secasvar *sa_next;       /* chain */
        int refcnt;                     /* reference count */
        u_int8_t state;                 /* Status of this Association */

        u_int8_t alg_auth;              /* Authentication Algorithm Identifier*/
        u_int8_t alg_enc;               /* Cipher Algorithm Identifier */
        u_int32_t spi;                  /* SPI Value, network byte order */
        u_int32_t flags;                /* holder for SADB_KEY_FLAGS */

        struct sadb_key *key_auth;      /* Key for Authentication */
        struct sadb_key *key_enc;       /* Key for Encryption */
        caddr_t iv;                     /* Initilization Vector */
        u_int ivlen;                    /* length of IV */
	void *sched;			/* intermediate encryption key */
	int schedlen;

        struct secreplay *replay;       /* replay prevention */
        u_int32_t tick;                 /* for lifetime */
        u_int32_t seq;                  /* sequence number */
}__attribute__((packed));


#define SADB_EXT_RESERVED             0
#define SADB_EXT_SA                   1
#define SADB_EXT_LIFETIME_CURRENT     2
#define SADB_EXT_LIFETIME_HARD        3
#define SADB_EXT_LIFETIME_SOFT        4
#define SADB_EXT_ADDRESS_SRC          5
#define SADB_EXT_ADDRESS_DST          6
#define SADB_EXT_ADDRESS_PROXY        7
#define SADB_EXT_KEY_AUTH             8
#define SADB_EXT_KEY_ENCRYPT          9
#define SADB_EXT_IDENTITY_SRC         10
#define SADB_EXT_IDENTITY_DST         11
#define SADB_EXT_SENSITIVITY          12
#define SADB_EXT_PROPOSAL             13
#define SADB_EXT_SUPPORTED_AUTH       14
#define SADB_EXT_SUPPORTED_ENCRYPT    15
#define SADB_EXT_SPIRANGE             16
#define SADB_X_EXT_KMPRIVATE          17
#define SADB_X_EXT_POLICY             18
#define SADB_X_EXT_SA2                19
#define SADB_EXT_MAX                  19

#define SADB_SATYPE_UNSPEC	0
#define SADB_SATYPE_AH		2
#define SADB_SATYPE_ESP		3
#define SADB_SATYPE_RSVP	5
#define SADB_SATYPE_OSPFV2	6
#define SADB_SATYPE_RIPV2	7
#define SADB_SATYPE_MIP		8
#define SADB_X_SATYPE_IPCOMP	9
#define SADB_X_SATYPE_POLICY	10
#define SADB_SATYPE_MAX		11

#define SADB_SASTATE_LARVAL   0
#define SADB_SASTATE_MATURE   1
#define SADB_SASTATE_DYING    2
#define SADB_SASTATE_DEAD     3
#define SADB_SASTATE_MAX      3

#define SADB_SAFLAGS_PFS      1




/* `flags' in sadb_sa structure holds followings */
#define SADB_X_EXT_NONE		0x0000	/* i.e. new format. */
#define SADB_X_EXT_OLD		0x0001	/* old format. */

#define SADB_X_EXT_IV4B		0x0010	/* IV length of 4 bytes in use */
#define SADB_X_EXT_DERIV	0x0020	/* DES derived */
#define SADB_X_EXT_CYCSEQ	0x0040	/* allowing to cyclic sequence. */

#define _ARRAYLEN(p) (sizeof(p)/sizeof(p[0]))
#define _KEYLEN(key) ((u_int)((key)->sadb_key_bits >> 3))
#define _KEYBITS(key) ((u_int)((key)->sadb_key_bits))
#define _KEYBUF(key) ((caddr_t)((caddr_t)(key) + sizeof(struct sadb_key)))


