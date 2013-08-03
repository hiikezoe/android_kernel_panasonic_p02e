/*
 * esp_core.c
 *
 */

#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#include "des.h"
#include "des_locl.h"
#include "rijndael.h"


static int esp_des_schedule(const struct esp_algorithm *,struct secasvar *);
static int esp_des_schedlen(const struct esp_algorithm *);
static int esp_des_blockencrypt(DES_LONG *, void *, int );

static int esp_3des_schedlen(const struct esp_algorithm *);
static int esp_3des_schedule(const struct esp_algorithm *,struct secasvar *);
static int esp_3des_blockencrypt(DES_LONG *, void *, int );

int esp_null_decrypt(struct mbuf *m, size_t off, struct secasvar *sav, 
		     const struct esp_algorithm *alg, int ivlen);
int esp_null_encrypt(struct mbuf *m, size_t off, struct secasvar *sav, 
		     const struct esp_algorithm *alg, int ivlen); 

const struct esp_algorithm esp_algorithms[] = {
  { 8, 8, 64, 64, esp_des_schedlen, "des-cbc",
    esp_cbc_decrypt, esp_cbc_encrypt, 
    esp_des_schedule,
    esp_des_blockencrypt, esp_des_blockencrypt, },

  { 8, 8, 192, 192, esp_3des_schedlen, "3des-cbc",
    esp_cbc_decrypt, esp_cbc_encrypt, 
    esp_3des_schedule,
    esp_3des_blockencrypt, esp_3des_blockencrypt, },

  { 16, 16, 128, 256, esp_rijndael_schedlen, "rijndael-cbc",
    esp_cbc_decrypt, esp_cbc_encrypt, 
    esp_rijndael_schedule,
    /* not use */
    esp_des_blockencrypt, esp_des_blockencrypt },
  
  { 4, 0, 0, 256, NULL, "null",
    esp_null_decrypt, esp_null_encrypt,
    NULL, NULL, NULL, },
  
};

int esp_null_decrypt(struct mbuf *m, size_t off, struct secasvar *sav, 
		     const struct esp_algorithm *alg, int ivlen){  return 0; }
int esp_null_encrypt(struct mbuf *m, size_t off, struct secasvar *sav, 
		     const struct esp_algorithm *alg, int ivlen){  return 0; }

int
esp_cbc_decrypt(m, off, sav, algo, ivlen)
struct mbuf *m;
size_t off;		/* offset to ESP header */
struct secasvar *sav;
const struct esp_algorithm *algo;
int ivlen;
{
  size_t ivoff = 0;
  size_t bodyoff = 0;
  u_int8_t *iv;
  size_t plen;
  u_int8_t tiv[MAXIVLEN];
  int derived;
  int error;

  DBG_ENTER(esp_cbc_decrypt);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_cbc_decrypt \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  derived = 0;
  ivoff = off + sizeof(struct newesp);
  bodyoff = off + sizeof(struct newesp) + ivlen;
  derived = 0;

  if(ivlen == algo->ivlenval) {
    iv = &tiv[0];
    m_copydata(m, ivoff, algo->ivlenval, &tiv[0]);
  }
  else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: esp_cbc_decrypt: unsupported ivlen %d\n",ivlen);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: esp_cbc_decrypt: unsupported ivlen %d\n",ivlen);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }
  
  plen = m->m_total;
  plen -= bodyoff;
  
  if( plen % algo->padbound ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: payload length must be multiple of %d\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: payload length must be multiple of %d\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		algo->padbound);
    return(-1);
  } 
  if( sav->alg_enc == SADB_X_EALG_AES ) /* SADB_X_EALG_RIJNDAELCBC / SADB_EALG_AESCBC 含 */
    error = esp_rijndael(m, bodyoff, plen, (u_int8_t*)sav->sched,
			    iv, algo, DES_DECRYPT);
  else
    error = des_cbc_encrypt(m, bodyoff, plen, sav->sched, 
			    (C_Block *)iv, algo, DES_DECRYPT);
  
  return(error);
}

int
esp_cbc_encrypt(m, plen, sav, algo, ivlen)
struct mbuf *m;
size_t plen;	/* payload length (to be decrypted) */
struct secasvar *sav;
const struct esp_algorithm *algo;
int ivlen;
{
  size_t bodyoff;
  u_int8_t *iv;
  int derived;
  int error;

  DBG_ENTER(esp_cbc_encrypt);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_cbc_encrypt \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  derived = 0;

  /* sanity check */
  if (plen % algo->padbound) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: payload length must be multiple of 8\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: payload length must be multiple of 8\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }
  if (sav->ivlen != (u_int)ivlen) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: esp_cbc_encrypt: bad ivlen %d/%d\n", ivlen, sav->ivlen);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: esp_cbc_encrypt: bad ivlen %d/%d\n", ivlen, sav->ivlen);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }
  if( ivlen != algo->ivlenval ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: unsupported ivlen %d\n", ivlen);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: unsupported ivlen %d\n", ivlen);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }

  derived = 0;
 
  /* set IV */
  iv = mtod(m->m_next, u_char *);
  iv += sizeof(struct newesp);

  memcpy( (caddr_t)iv,(caddr_t)sav->iv, ivlen );
  bodyoff = m->m_len + sizeof(struct newesp) + ivlen;

  /* AES を同じmoduleにするのは現状困難 */
  if( sav->alg_enc == SADB_X_EALG_AES ) /* SADB_X_EALG_RIJNDAELCBC / SADB_EALG_AESCBC 含 */
    error = esp_rijndael(m, bodyoff, plen, (u_int8_t*)sav->sched,
			    iv, algo, DES_ENCRYPT);
  else
    error = des_cbc_encrypt(m, bodyoff, plen, sav->sched,
			    (C_Block *)iv, algo, DES_ENCRYPT);

  RAND_bytes(&sav->iv[0],algo->ivlenval);

  return error;
}


caddr_t 
mbuf_find_offset(
		 struct mbuf *m,
		 size_t tolen,
		 size_t off,
		 size_t len)
{
  struct mbuf *n;
  size_t cnt;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call mbuf_find_offset \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if((tolen < off)||(tolen < off + len))
    return( (caddr_t)NULL );
  cnt = 0;
  for(n = m; n; n = n->m_next) {
    if((cnt + n->m_len) <= off) {
      cnt += n->m_len;
      continue;
    }
    if(cnt <= off && off < cnt + n->m_len
       && cnt <= off + len && off + len <= cnt + n->m_len) {
      return( mtod(n, caddr_t) + off - cnt );
    }
    else
      return((caddr_t)NULL);
  }
  return((caddr_t)NULL);
}

/*------------------------------------------------------------*/

#define panic(x) do {dbg_printf((x)); return -1;} while (0)

/* does not free m0 on error */
int
esp_auth(pIpsec, m0, skip, length, sav, sum)
PIPSEC pIpsec;
struct mbuf *m0;
size_t skip;	/* offset to ESP header */
size_t length;	/* payload length */
struct secasvar *sav;
u_char *sum;
{
  struct mbuf *m;
  struct ah_algorithm_state s;
  u_char sumbuf[AH_MAXSUMSIZE];
  size_t siz;
  size_t off;

  DBG_ENTER(esp_auth);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_auth \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  /*
   * length of esp part (excluding authentication data) must be 4n,
   * since nexthdr must be at offset 4n+3.
   */
  if (length % 4) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: esp_auth: length is not multiple of 4\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: esp_auth: length is not multiple of 4\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }
  off = 0;
  m = m0;
  /* skip over the IP header */
  while( skip ) {
    if (!m)
      panic("mbuf chain?\n");
    if (m->m_len <= (int)skip) {
      skip -= m->m_len;
      m = m->m_next;
      off = 0;
    }
    else {
      off = skip;
      skip = 0;
    }
  }

  siz = 12; /* siz1 = ((siz + 3) & ~(4 - 1)) */

  if( sav->alg_auth == SADB_AALG_MD5HMAC ) {
    if( ah_hmac_md5_init(pIpsec, &s, sav) )
      return(-1);
    ah_hmac_md5_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_hmac_md5_result(&s, &sumbuf[0]);
  }
  else if( sav->alg_auth == SADB_AALG_SHA1HMAC ) {
    if( ah_hmac_sha1_init(pIpsec, &s, sav) )
      return(-1);
    ah_hmac_sha1_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_hmac_sha1_result(&s, &sumbuf[0]);
  }
#if 0
  else if( sav->alg_auth == SADB_AALG_DESHMAC ) {
    if( ah_hmac_des_init(pIpsec, &s, sav) )
      return(-1);
    ah_hmac_des_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_hmac_des_result(&s, &sumbuf[0]);
  }
#endif
  else if( sav->alg_auth == SADB_X_AALG_SHA2_224 ) {
    if( ah_sha224_init(pIpsec, &s, sav) )
      return(-1);
    ah_sha224_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_sha224_result(&s, &sumbuf[0]);
  }
  else if( sav->alg_auth == SADB_X_AALG_SHA2_256 ) {
    if( ah_sha256_init(pIpsec, &s, sav) )
      return(-1);
    ah_sha256_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_sha256_result(&s, &sumbuf[0]);
  }
#if 0
  else if( sav->alg_auth == SADB_X_AALG_SHA2_384 ) {
    if( ah_sha384_init(pIpsec, &s, sav) )
      return(-1);
    ah_sha384_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_sha384_result(&s, &sumbuf[0]);
  }
  else if( sav->alg_auth == SADB_X_AALG_SHA2_512 ) {
    if( ah_sha512_init(pIpsec, &s, sav) )
      return(-1);
    ah_sha512_loop(&s, mtod(m, u_char *) + off, length); /* pgr0689 */
    ah_sha512_result(&s, &sumbuf[0]);
  }
#endif
  else if( sav->alg_auth == SADB_AALG_NONE || sav->alg_auth == SADB_ORG_AALG_NULL || sav->alg_auth == SADB_X_AALG_NULL ) {
    memset(sum, 0, siz);
    return 0;
  }
  else {
    /* SADB_X_AALG_MD5 / SADB_X_AALG_SHA / SADB_X_AALG_AES */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: AH Unknown algorithm (%d)\n",sav->alg_auth);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: AH Unknown algorithm (%d)\n",sav->alg_auth);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }

  memcpy(sum, sumbuf, siz);	/*XXX*/
	
  return 0;
}

const struct esp_algorithm *
esp_algorithm_lookup(idx)
int idx;
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_algorithm_lookup \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  switch (idx) {
  case SADB_EALG_DESCBC:        /* DES */
    return &esp_algorithms[0];
  case SADB_EALG_3DESCBC:       /* 3DES */
    return &esp_algorithms[1];
  case SADB_X_EALG_AES:         /* AES */ /* SADB_EALG_AESCBC / SADB_X_EALG_RIJNDAELCBC 含 */
    return &esp_algorithms[2];
  case SADB_EALG_NULL:          /* NULL */
  case SADB_EALG_NONE:          /* NONE */
    return &esp_algorithms[3];
  /* case SADB_EALG_DES_IV64:    */
  /* case SADB_EALG_RC5:         */
  /* case SADB_EALG_IDEA:        */
  /* case SADB_EALG_CAST128CBC:  */
  /* case SADB_EALG_BLOWFISHCBC: */
  /* case SADB_EALG_3IDEA:       */
  /* case SADB_EALG_DES_IV32:    */
  /* case SADB_EALG_RC4:         */
  default:
    return NULL;
  }
}

int
esp_schedule(pIpsec, algo, sav)
PIPSEC pIpsec;
const struct esp_algorithm *algo;
struct secasvar *sav;
{
  int error;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_schedule \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  DBG_ENTER(esp_schedule);
  /* check for key length */
  if (_KEYBITS(sav->key_enc) < (u_int)algo->keymin ||
      _KEYBITS(sav->key_enc) > (u_int)algo->keymax) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "esp_schedule %s: unsupported key length %d: "
#else /* BUILD_ANDROID */
    printk(KERN_ERR "esp_schedule %s: unsupported key length %d: "
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	       "needs %d to %d bits\n", algo->name, _KEYBITS(sav->key_enc),
	       algo->keymin, algo->keymax);
    return(-1);
  }

  /* already allocated */
  if (sav->sched && sav->schedlen != 0)
    return 0;
  /* no schedule necessary */
  if (!algo->schedule || !algo->schedlen)
    return 0;
  
  sav->schedlen = (*algo->schedlen)(algo);
  if (sav->schedlen < 0)
    return(-1);
  sav->sched = pIpsec->sched[sav->refcnt];

  error = (*algo->schedule)(algo, sav);
  if (error) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "esp_schedule %s: error %d\n", algo->name, error);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "esp_schedule %s: error %d\n", algo->name, error);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    sav->sched = NULL;
    sav->schedlen = 0;
    return(-1);
  }
  return(0);
}


static int esp_des_schedule(const struct esp_algorithm *algo,
			    struct secasvar *sav)
{

  DBG_ENTER(esp_des_schedule);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_des_schedule \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (des_set_key((C_Block *)_KEYBUF(sav->key_enc),
		  *(des_key_schedule *)sav->sched)) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: key error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: key error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }
  else
    return(0);
}

static int
esp_3des_schedule(algo, sav)
const struct esp_algorithm *algo;
struct secasvar *sav;
{
  des_key_schedule *p;
  int i;
  char *k;

  DBG_ENTER(esp_3des_schedule);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_3des_schedule \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  p = (des_key_schedule *)sav->sched;
  k = _KEYBUF(sav->key_enc);
  for (i = 0; i < 3; i++) {
    if( des_key_sched((des_cblock *)(k + 8 * i), p[i])) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: 3des key error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: 3des key error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      return(-1);
    }
  }
  return(0);
}

static int 
esp_des_blockencrypt( data, ks, enc )
DES_LONG *data;
void *ks;
int enc;
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_des_blockencrypt \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  des_encrypt(data, ks, enc);
  return(0);
}

static int
esp_des_schedlen(algo)
const struct esp_algorithm *algo;
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_des_schedlen \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  return( sizeof(des_key_schedule) );
}

static int
esp_3des_schedlen(algo)
const struct esp_algorithm *algo;
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_3des_schedlen \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  return( sizeof(des_key_schedule) * 3 );
}

static int 
esp_3des_blockencrypt( data, ks, enc )
DES_LONG *data;
void *ks;
int enc;
{
  des_key_schedule *p;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_3des_blockencrypt \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  p = (des_key_schedule *)ks;
  if( enc == DES_ENCRYPT )
    des_encrypt3(data, p[0], p[1], p[2]);
  else
    des_decrypt3(data, p[0], p[1], p[2]);
  return(0);
}


int 
des_cbc_encrypt(m0, skip, length, schedule, ivec, algo, mode)
struct mbuf *m0;
size_t skip;
size_t length;
des_key_schedule schedule;
des_cblock (*ivec);
const struct esp_algorithm *algo;
int mode;
{
  u_int8_t inbuf[MAXIVLEN]; /* outbuf[MAXIVLEN]; */
  struct mbuf *m;
  size_t off;
  register DES_LONG tin0, tin1;
  register DES_LONG tout0, tout1;
  DES_LONG tin[2];
  u_int8_t *iv;
  int blocklen;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_encrypt \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  DBG_ENTER(esp_encrypt);

  m = m0;
  off = 0;

  blocklen = algo->padbound;

  /* skip over the header */
  while (skip) {
    if (!m)
      panic("mbuf chain?\n");
    if (m->m_len <= (int)skip) {
      skip -= (size_t)m->m_len;
      m = m->m_next;
      off = 0;
    }
    else {
      off = skip;
      skip = 0;
    }
  }

  /* initialize */
  tin0 = tin1 = tout0 = tout1 = 0;
  tin[0] = tin[1] = 0;

  if (mode == DES_ENCRYPT) {
    u_int8_t *in, *out;

    iv = (u_int8_t *)ivec;
    c2l(iv, tout0);
    c2l(iv, tout1);

    while (0 < length) {
      if (!m)
		panic("mbuf chain?\n");

      /*
       * copy the source into input buffer.
       * don't update off or m, since we need to use them				 
       * later.
       */
      if ((int)(off + blocklen) <= m->m_len)
		memcpy(&inbuf[0], mtod(m, u_int8_t *) + off, blocklen);
      else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "des_cbc_encr mbuf error\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "des_cbc_encr mbuf error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		return(-1);
      }

      in = &inbuf[0];
#ifdef OUT_BUFF
      out = &outbuf[0];
#else
      out = mtod(m, u_int8_t *) +off;
#endif
      c2l(in, tin0);
      c2l(in, tin1);

      tin0 ^= tout0; tin[0] = tin0;
      tin1 ^= tout1; tin[1] = tin1;
      /* encrypt */
      (*algo->blockencrypt)((DES_LONG *)tin, schedule, DES_ENCRYPT);

      tout0 = tin[0]; l2c(tout0, out);
      tout1 = tin[1]; l2c(tout1, out);

      /*
       * copy the output buffer into the result.
       * need to update off and m.
       */
      if ((int)(off + blocklen) < m->m_len) {
#ifdef OUT_BUFF
		memcpy(mtod(m, u_int8_t *) + off, &outbuf[0], blocklen);
#endif
		off += blocklen;
      }
      else if ((int)(off + blocklen) == m->m_len) {
#ifdef OUT_BUFF
		memcpy(mtod(m, u_int8_t *) + off, &outbuf[0], blocklen);
#endif
		m = 0;   /* none m_next */
		off = 0;
      }
      else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "des_cbc_encr mbuf error 2\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "des_cbc_encr mbuf error 2\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		return(-1);
      }
      length -= blocklen;
    }

  } 
  else if (mode == DES_DECRYPT) {
    register DES_LONG xor0, xor1;
    u_int8_t *in, *out;
    
    xor0 = xor1 = 0;
    iv = (u_int8_t *)ivec;
    c2l(iv, xor0);
    c2l(iv, xor1);

    while (0 < length) {
      if (!m)
		panic("mbuf chain?\n");

      /*
       * copy the source into input buffer.
       * don't update off or m, since we need to use them				 
       * later.
       */
      if ((int)(off + blocklen) <= m->m_len)
		memcpy(&inbuf[0], mtod(m, u_int8_t *) + off, blocklen);
      else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "ESP: in mbuf error\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "ESP: in mbuf error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		return(-1);
      }
      
      in = &inbuf[0];
#ifdef OUT_BUFF
      out = &outbuf[0];
#else
      out = mtod(m, u_int8_t *) +off;
#endif
      c2l(in, tin0); tin[0] = tin0;
      c2l(in, tin1); tin[1] = tin1;
      /* Decrypt */
      (*algo->blockencrypt)((DES_LONG *)tin, schedule, DES_DECRYPT);
      tout0 = tin[0] ^ xor0;
      tout1 = tin[1] ^ xor1;
      l2c(tout0, out);
      l2c(tout1, out);
      xor0 = tin0;
      xor1 = tin1;
      
      /*
       * copy the output buffer into the result.
       * need to update off and m.
       */
      if((int)(off + blocklen) < m->m_len) {
#ifdef OUT_BUFF
		memcpy(mtod(m, u_int8_t *) + off, &outbuf[0], blocklen);
#endif
		off += blocklen;
      }
      else if ((int)(off + blocklen) == m->m_len) {
#ifdef OUT_BUFF
		memcpy(mtod(m, u_int8_t *) + off, &outbuf[0], blocklen);
#endif
		off = 0;
      }
      else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "ESP: in mbuf error 2\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "ESP: in mbuf error 2\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      }
      length -= blocklen;
    }
  }
  return 0;
}


