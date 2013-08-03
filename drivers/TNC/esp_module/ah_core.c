/*
 * ah_core.c
 *
 */
#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#include "global.h"
#include "md5.h"
#include "sha1.h"
#include "sha2.h"


#define HMACSIZE        16

int
ah_hmac_md5_init(pIpsec, state, sav)
PIPSEC pIpsec;
struct ah_algorithm_state *state;
struct secasvar *sav;
{
  u_char *ipad;
  u_char *opad;
  u_char tk[16];
  u_char *key;
  size_t keylen;
  register size_t i;
  MD5_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_hmac_md5_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state) {
    dbg_printf(("AH: ah_hmac_md5_init: what?"));
  }

  state->sav = sav;
  state->foo = (void *)pIpsec->hash_buf;

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (MD5_CTX *)(opad + 64);

  /* compress the key if necessery */
  if (64 < _KEYLEN(state->sav->key_auth)) {
    MD5_Init(ctxt);
    MD5_Update(ctxt, _KEYBUF(state->sav->key_auth),
	      _KEYLEN(state->sav->key_auth));
    MD5_Final(&tk[0], ctxt);
    key = &tk[0];
    keylen = 16;
  }
  else {
    key = _KEYBUF(state->sav->key_auth);
    keylen = _KEYLEN(state->sav->key_auth);
  }

  bzero(ipad, 64);
  bzero(opad, 64);
  memcpy(ipad, key, keylen);
  memcpy(opad, key, keylen);
  for (i = 0; i < 64; i++) {
    ipad[i] ^= 0x36;
    opad[i] ^= 0x5c;
  }

  MD5_Init(ctxt);
  MD5_Update(ctxt, ipad, 64);
  
  return 0;
}

void
ah_hmac_md5_loop(state, addr, len)
struct ah_algorithm_state *state;
caddr_t addr;
size_t len;
{
  MD5_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_hmac_md5_loop \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state || !state->foo) {
    dbg_printf(("AH: ah_hmac_md5_loop: what?"));
  }
  ctxt = (MD5_CTX *)(((caddr_t)state->foo) + 128);
  MD5_Update(ctxt, addr, len);
}

void
ah_hmac_md5_result(state, addr)
struct ah_algorithm_state *state;
caddr_t addr;
{
  u_char digest[16];
  u_char *ipad;
  u_char *opad;
  MD5_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_hmac_md5_result \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state || !state->foo) {
    dbg_printf(("ah_hmac_md5_result: what?\n"));
  }


  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (MD5_CTX *)(opad + 64);

  MD5_Final(&digest[0], ctxt);

  MD5_Init(ctxt);
  MD5_Update(ctxt, opad, 64);
  MD5_Update(ctxt, &digest[0], sizeof(digest));
  MD5_Final(&digest[0], ctxt);
  memcpy((void *)addr, &digest[0], HMACSIZE);

  /* free(state->foo, M_TEMP); */
}


int
ah_hmac_sha1_init(pIpsec, state, sav)
PIPSEC pIpsec;
struct ah_algorithm_state *state;
struct secasvar *sav;
{
  u_char *ipad;
  u_char *opad;
  SHA1_CTX *ctxt;
  u_char tk[SHA1_RESULTLEN];	/* SHA-1 generates 160 bits */
  u_char *key;
  size_t keylen;
  register size_t i;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_hmac_sha1_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state) {
    dbg_printf(("ah_hmac_sha1_init: what?"));
  }

  state->sav = sav;
  state->foo = (void *)pIpsec->hash_buf;

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (SHA1_CTX *)(opad + 64);

  /* compress the key if necessery */
  if (64 < _KEYLEN(state->sav->key_auth)) {
    SHA1Init(ctxt);
    SHA1Update(ctxt, _KEYBUF(state->sav->key_auth),
	       _KEYLEN(state->sav->key_auth));
    SHA1Final(&tk[0], ctxt);
    key = &tk[0];
    keylen = SHA1_RESULTLEN;
  }
  else {
    key = _KEYBUF(state->sav->key_auth);
    keylen = _KEYLEN(state->sav->key_auth);
  }
  
  bzero(ipad, 64);
  bzero(opad, 64);
  memcpy(ipad, key, keylen);
  memcpy(opad, key, keylen);
  for (i = 0; i < 64; i++) {
    ipad[i] ^= 0x36;
    opad[i] ^= 0x5c;
  }
  
  SHA1Init(ctxt);
  SHA1Update(ctxt, ipad, 64);
  
  return 0;
}

void
ah_hmac_sha1_loop(state, addr, len)
struct ah_algorithm_state *state;
caddr_t addr;
size_t len;
{
  SHA1_CTX *ctxt;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_hmac_sha1_loop \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state || !state->foo) {
    dbg_printf(("ah_hmac_sha1_loop: what?"));
  }

  ctxt = (SHA1_CTX *)(((u_char *)state->foo) + 128);
  SHA1Update(ctxt, (caddr_t)addr, (size_t)len);
}

void
ah_hmac_sha1_result(state, addr)
struct ah_algorithm_state *state;
caddr_t addr;
{
  u_char digest[SHA1_RESULTLEN];	/* SHA-1 generates 160 bits */
  u_char *ipad;
  u_char *opad;
  SHA1_CTX *ctxt;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID  
    TNC_LOGOUT("Call ah_hmac_sha1_result \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state || !state->foo) {
    dbg_printf(("ah_hmac_sha1_result: what?"));
  }

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (SHA1_CTX *)(opad + 64);

  SHA1Final((caddr_t)&digest[0], ctxt);

  SHA1Init(ctxt);
  SHA1Update(ctxt, opad, 64);
  SHA1Update(ctxt, (caddr_t)&digest[0], sizeof(digest));
  SHA1Final((caddr_t)&digest[0], ctxt);

  memcpy((void *)addr, &digest[0], HMACSIZE);

  /* free(state->foo, M_TEMP); */
}


int
ah_sha224_init(pIpsec, state, sav)
PIPSEC pIpsec;
struct ah_algorithm_state *state;
struct secasvar *sav;
{
  u_char *ipad;
  u_char *opad;
  SHA256_CTX *ctxt;
  u_char tk[SHA224_DIGEST_LENGTH];	/* SHA2-224 generates 224 bits */
  u_char *key;
  size_t keylen;
  register size_t i;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha224_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state) {
    dbg_printf(("ah_sha224_init: what?"));
  }

  state->sav = sav;
  state->foo = (void *)pIpsec->hash_buf;

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (SHA256_CTX *)(opad + 64);

  /* compress the key if necessery */
  if (64 < _KEYLEN(state->sav->key_auth)) {
    SHA224_Init(ctxt);
    SHA224_Update(ctxt, _KEYBUF(state->sav->key_auth),
	       _KEYLEN(state->sav->key_auth));
    SHA224_Final(&tk[0], ctxt);
    key = &tk[0];
    keylen = SHA224_DIGEST_LENGTH;
  }
  else {
    key = _KEYBUF(state->sav->key_auth);
    keylen = _KEYLEN(state->sav->key_auth);
  }
  
  bzero(ipad, 64);
  bzero(opad, 64);
  memcpy(ipad, key, keylen);
  memcpy(opad, key, keylen);
  for (i = 0; i < 64; i++) {
    ipad[i] ^= 0x36;
    opad[i] ^= 0x5c;
  }
  
  SHA224_Init(ctxt);
  SHA224_Update(ctxt, ipad, 64);
  
  return 0;
}

void
ah_sha224_loop(state, addr, len)
struct ah_algorithm_state *state;
caddr_t addr;
size_t len;
{
  SHA256_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha224_loop \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state || !state->foo) {
    dbg_printf(("ah_sha224_loop: what?"));
  }

  ctxt = (SHA256_CTX *)(((u_char *)state->foo) + 128);
  SHA224_Update(ctxt, (caddr_t)addr, (size_t)len);
}

void
ah_sha224_result(state, addr)
struct ah_algorithm_state *state;
caddr_t addr;
{
  u_char digest[SHA224_DIGEST_LENGTH];	/* SHA2-224 generates 224 bits */
  u_char *ipad;
  u_char *opad;
  SHA256_CTX *ctxt;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID  
    TNC_LOGOUT("Call ah_sha224_result \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state || !state->foo) {
    dbg_printf(("ah_sha224_result: what?"));
  }

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (SHA256_CTX *)(opad + 64);

  SHA224_Final((caddr_t)&digest[0], ctxt);

  SHA224_Init(ctxt);
  SHA224_Update(ctxt, opad, 64);
  SHA224_Update(ctxt, (caddr_t)&digest[0], sizeof(digest));
  SHA224_Final((caddr_t)&digest[0], ctxt);

  memcpy((void *)addr, &digest[0], HMACSIZE);

  /* free(state->foo, M_TEMP); */
}


int
ah_sha256_init(pIpsec, state, sav)
PIPSEC pIpsec;
struct ah_algorithm_state *state;
struct secasvar *sav;
{
  u_char *ipad;
  u_char *opad;
  SHA256_CTX *ctxt;
  u_char tk[SHA256_DIGEST_LENGTH];	/* SHA2-256 generates 256 bits */
  u_char *key;
  size_t keylen;
  register size_t i;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha256_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if (!state) {
    dbg_printf(("ah_sha256_init: what?"));
  }

  state->sav = sav;
  state->foo = (void *)pIpsec->hash_buf;

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (SHA256_CTX *)(opad + 64);

  /* compress the key if necessery */
  if (64 < _KEYLEN(state->sav->key_auth)) {
    SHA256_Init(ctxt);
    SHA256_Update(ctxt, _KEYBUF(state->sav->key_auth),
	       _KEYLEN(state->sav->key_auth));
    SHA256_Final(&tk[0], ctxt);
    key = &tk[0];
    keylen = SHA256_DIGEST_LENGTH;
  }
  else {
    key = _KEYBUF(state->sav->key_auth);
    keylen = _KEYLEN(state->sav->key_auth);
  }
  
  bzero(ipad, 64);
  bzero(opad, 64);
  memcpy(ipad, key, keylen);
  memcpy(opad, key, keylen);
  for (i = 0; i < 64; i++) {
    ipad[i] ^= 0x36;
    opad[i] ^= 0x5c;
  }
  
  SHA256_Init(ctxt);
  SHA256_Update(ctxt, ipad, 64);
  
  return 0;
}

void
ah_sha256_loop(state, addr, len)
struct ah_algorithm_state *state;
caddr_t addr;
size_t len;
{
  SHA256_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha256_loop \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state || !state->foo) {
    dbg_printf(("ah_sha256_loop: what?"));
  }

  ctxt = (SHA256_CTX *)(((u_char *)state->foo) + 128);
  SHA256_Update(ctxt, (caddr_t)addr, (size_t)len);
}

void
ah_sha256_result(state, addr)
struct ah_algorithm_state *state;
caddr_t addr;
{
  u_char digest[SHA256_DIGEST_LENGTH];	/* SHA2-256 generates 256 bits */
  u_char *ipad;
  u_char *opad;
  SHA256_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID  
    TNC_LOGOUT("Call ah_sha256_result \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state || !state->foo) {
    dbg_printf(("ah_hmac_sha256_result: what?"));
  }

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 64);
  ctxt = (SHA256_CTX *)(opad + 64);

  SHA256_Final((caddr_t)&digest[0], ctxt);

  SHA256_Init(ctxt);
  SHA256_Update(ctxt, opad, 64);
  SHA256_Update(ctxt, (caddr_t)&digest[0], sizeof(digest));
  SHA256_Final((caddr_t)&digest[0], ctxt);

  memcpy((void *)addr, &digest[0], HMACSIZE);

  /* free(state->foo, M_TEMP); */
}

#if 0
int
ah_sha384_init(pIpsec, state, sav)
PIPSEC pIpsec;
struct ah_algorithm_state *state;
struct secasvar *sav;
{
  u_char *ipad;
  u_char *opad;
  SHA512_CTX *ctxt;
  u_char tk[SHA384_DIGEST_LENGTH];	/* SHA2-384 generates 384 bits */
  u_char *key;
  size_t keylen;
  register size_t i;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha384_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state) {
    dbg_printf(("ah_sha384_init: what?"));
  }

  state->sav = sav;
  state->foo = (void *)pIpsec->hash_buf;

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 128);
  ctxt = (SHA512_CTX *)(opad + 128);

  /* compress the key if necessery */
  if (128 < _KEYLEN(state->sav->key_auth)) {
    SHA384_Init(ctxt);
    SHA384_Update(ctxt, _KEYBUF(state->sav->key_auth),
	       _KEYLEN(state->sav->key_auth));
    SHA384_Final(&tk[0], ctxt);
    key = &tk[0];
    keylen = SHA384_DIGEST_LENGTH;
  }
  else {
    key = _KEYBUF(state->sav->key_auth);
    keylen = _KEYLEN(state->sav->key_auth);
  }
  
  bzero(ipad, 128);
  bzero(opad, 128);
  memcpy(ipad, key, keylen);
  memcpy(opad, key, keylen);
  for (i = 0; i < 128; i++) {
    ipad[i] ^= 0x36;
    opad[i] ^= 0x5c;
  }
  
  SHA384_Init(ctxt);
  SHA384_Update(ctxt, ipad, 128);
  
  return 0;
}

void
ah_sha384_loop(state, addr, len)
struct ah_algorithm_state *state;
caddr_t addr;
size_t len;
{
  SHA512_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha384_loop \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state || !state->foo) {
    dbg_printf(("ah_sha384_loop: what?"));
  }

  ctxt = (SHA512_CTX *)(((u_char *)state->foo) + 128);
  SHA384_Update(ctxt, (caddr_t)addr, (size_t)len);
}

void
ah_sha384_result(state, addr)
struct ah_algorithm_state *state;
caddr_t addr;
{
  u_char digest[SHA384_DIGEST_LENGTH];	/* SHA2-384 generates 384 bits */
  u_char *ipad;
  u_char *opad;
  SHA512_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID  
    TNC_LOGOUT("Call ah_sha384_result \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state || !state->foo) {
    dbg_printf(("ah_sha384_result: what?"));
  }

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 128);
  ctxt = (SHA512_CTX *)(opad + 128);

  SHA384_Final((caddr_t)&digest[0], ctxt);

  SHA384_Init(ctxt);
  SHA384_Update(ctxt, opad, 128);
  SHA384_Update(ctxt, (caddr_t)&digest[0], sizeof(digest));
  SHA384_Final((caddr_t)&digest[0], ctxt);

  memcpy((void *)addr, &digest[0], HMACSIZE);

  /* free(state->foo, M_TEMP); */
}


int
ah_sha512_init(pIpsec, state, sav)
PIPSEC pIpsec;
struct ah_algorithm_state *state;
struct secasvar *sav;
{
  u_char *ipad;
  u_char *opad;
  SHA512_CTX *ctxt;
  u_char tk[SHA512_DIGEST_LENGTH];	/* SHA2-512 generates 512 bits */
  u_char *key;
  size_t keylen;
  register size_t i;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha512_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state) {
    dbg_printf(("ah_sha512_init: what?"));
  }

  state->sav = sav;
  state->foo = (void *)pIpsec->hash_buf;

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 128);
  ctxt = (SHA512_CTX *)(opad + 128);

  /* compress the key if necessery */
  if (128 < _KEYLEN(state->sav->key_auth)) {
    SHA512_Init(ctxt);
    SHA512_Update(ctxt, _KEYBUF(state->sav->key_auth),
	       _KEYLEN(state->sav->key_auth));
    SHA512_Final(&tk[0], ctxt);
    key = &tk[0];
    keylen = SHA512_DIGEST_LENGTH;
  }
  else {
    key = _KEYBUF(state->sav->key_auth);
    keylen = _KEYLEN(state->sav->key_auth);
  }
  
  bzero(ipad, 128);
  bzero(opad, 128);
  memcpy(ipad, key, keylen);
  memcpy(opad, key, keylen);
  for (i = 0; i < 128; i++) {
    ipad[i] ^= 0x36;
    opad[i] ^= 0x5c;
  }
  
  SHA512_Init(ctxt);
  SHA512_Update(ctxt, ipad, 128);
  
  return 0;
}

void
ah_sha512_loop(state, addr, len)
struct ah_algorithm_state *state;
caddr_t addr;
size_t len;
{
  SHA512_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_sha512_loop \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state || !state->foo) {
    dbg_printf(("ah_sha512_loop: what?"));
  }

  ctxt = (SHA512_CTX *)(((u_char *)state->foo) + 128);
  SHA512_Update(ctxt, (caddr_t)addr, (size_t)len);
}

void
ah_sha512_result(state, addr)
struct ah_algorithm_state *state;
caddr_t addr;
{
  u_char digest[SHA512_DIGEST_LENGTH];	/* SHA2-512 generates 512 bits */
  u_char *ipad;
  u_char *opad;
  SHA512_CTX *ctxt;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID  
    TNC_LOGOUT("Call ah_sha512_result \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (!state || !state->foo) {
    dbg_printf(("ah_sha512_result: what?"));
  }

  ipad = (u_char *)state->foo;
  opad = (u_char *)(ipad + 128);
  ctxt = (SHA512_CTX *)(opad + 128);

  SHA512_Final((caddr_t)&digest[0], ctxt);

  SHA512_Init(ctxt);
  SHA512_Update(ctxt, opad, 128);
  SHA512_Update(ctxt, (caddr_t)&digest[0], sizeof(digest));
  SHA512_Final((caddr_t)&digest[0], ctxt);

  memcpy((void *)addr, &digest[0], HMACSIZE);

  /* free(state->foo, M_TEMP); */
}
#endif

/*
 * go generate the checksum.
 */
void ah_update_mbuf(
		    struct mbuf *m,
		    int off,
		    int len,
		    u_int8_t auth,
		    struct ah_algorithm_state *algos)
{
  struct mbuf *n;
  int tlen;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_update_mbuf \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* easy case first */
  if(off + len <= m->m_len) {
    if( auth == SADB_AALG_MD5HMAC )
      ah_hmac_md5_loop(algos, mtod(m, caddr_t) + off, len);
    else if ( auth == SADB_AALG_SHA1HMAC )
      ah_hmac_sha1_loop(algos, mtod(m, caddr_t) + off, len);
#if 0
    else if ( auth == SADB_AALG_DESHMAC )
      ah_hmac_des_loop(algos, mtod(m, caddr_t) + off, len);
#endif
    else if ( auth == SADB_X_AALG_SHA2_224)
      ah_sha224_loop(algos, mtod(m, caddr_t) + off, len);
    else if ( auth == SADB_X_AALG_SHA2_256)
      ah_sha256_loop(algos, mtod(m, caddr_t) + off, len);
#if 0
    else if ( auth == SADB_X_AALG_SHA2_384)
      ah_sha384_loop(algos, mtod(m, caddr_t) + off, len);
    else if ( auth == SADB_X_AALG_SHA2_512)
      ah_sha512_loop(algos, mtod(m, caddr_t) + off, len);
#endif
    return;
  }

  for (n = m; n; n = n->m_next) {
    if (off < n->m_len)
      break;
    off -= n->m_len;
  }
  
  if (!n) {
    dbg_printf(("AH: ah_update_mbuf wrong offset specified"));
  }

  for (/*nothing*/; n && len > 0; n = n->m_next) {
    if (n->m_len == 0)
      continue;
    if (n->m_len - off < len)
      tlen = n->m_len - off;
    else
      tlen = len;

    if( auth == SADB_AALG_MD5HMAC )
      ah_hmac_md5_loop(algos, mtod(n, caddr_t) + off, tlen);
    else if( auth == SADB_AALG_SHA1HMAC )
      ah_hmac_sha1_loop(algos, mtod(n, caddr_t) + off, tlen);
#if 0
    else if( auth == SADB_AALG_DESHMAC )
      ah_hmac_des_loop(algos, mtod(n, caddr_t) + off, tlen);
#endif
    else if( auth == SADB_X_AALG_SHA2_224 )
      ah_sha224_loop(algos, mtod(n, caddr_t) + off, tlen);
    else if( auth == SADB_X_AALG_SHA2_256 )
      ah_sha256_loop(algos, mtod(n, caddr_t) + off, tlen);
#if 0
    else if( auth == SADB_X_AALG_SHA2_384 )
      ah_sha384_loop(algos, mtod(n, caddr_t) + off, tlen);
    else if( auth == SADB_X_AALG_SHA2_512 )
      ah_sha512_loop(algos, mtod(n, caddr_t) + off, tlen);
#endif
    
    len -= tlen;
    off = 0;
  }
}


/*
 * Go generate the checksum. This function won't modify the mbuf chain
 * except AH itself.
 *
 * NOTE: the function does not free mbuf on failure.
 * Don't use m_copy(), it will try to share cluster mbuf by using refcnt.
 */
int
ah4_calccksum( pIpsec, m, ahdat, len, sav)
PIPSEC pIpsec;
struct mbuf *m;
caddr_t ahdat;
size_t len;
struct secasvar *sav;
{
  struct ah_algorithm_state algos;
  u_char sumbuf[AH_MAXSUMSIZE];
  struct ip iphdr;
  size_t hlen;
  int ahseen;
  int error;
  int off;
  int hdrtype;
  size_t advancewidth;

  DBG_ENTER(ah4_calccksum);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah4_calccksum \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	
  ahseen = 0;

  if( sav->alg_auth == SADB_AALG_MD5HMAC )
    error = ah_hmac_md5_init(pIpsec, &algos, sav);
  else if( sav->alg_auth == SADB_AALG_SHA1HMAC )
    error = ah_hmac_sha1_init(pIpsec, &algos, sav);
#if 0
  else if( sav->alg_auth == SADB_AALG_DESHMAC )
    error = ah_hmac_des_init(pIpsec, &algos, sav);
#endif
  else if( sav->alg_auth == SADB_X_AALG_SHA2_224 )
    error = ah_sha224_init(pIpsec, &algos, sav);
  else if( sav->alg_auth == SADB_X_AALG_SHA2_256 )
    error = ah_sha256_init(pIpsec, &algos, sav);
#if 0
  else if( sav->alg_auth == SADB_X_AALG_SHA2_384 )
    error = ah_sha384_init(pIpsec, &algos, sav);
  else if( sav->alg_auth == SADB_X_AALG_SHA2_512 )
    error = ah_sha512_init(pIpsec, &algos, sav);
#endif
  else {
    dbg_printf(("AH: Unknown algorithm (%d)\n",sav->alg_auth));
    return(-1); /* algo->update too */
  }
  if( error )
    return( error );

  ahseen = 0;
  hdrtype = -1;	/*dummy, it is called IPPROTO_IP*/
  off = 0;
  advancewidth = 0;	/*safety*/

again:
	/* gory. */
  switch (hdrtype) {
  case -1:	/*first one only*/
/*  case IPPROTO_IPIP: */
    {
      /*
       * copy ip hdr, modify to fit the AH checksum rule,
       * then take a checksum.
       */
      m_copydata(m, 0, sizeof(iphdr), (caddr_t)&iphdr);
      hlen = iphdr.ip_hl << 2;  
      iphdr.ip_ttl = 0;
      iphdr.ip_sum = 0;
      if( pIpsec->ip4_ah_cleartos )
	iphdr.ip_tos = 0;
      iphdr.ip_off = htons((ntohs(iphdr.ip_off) & pIpsec->ip4_ah_offsetmask));
      
      if( sav->alg_auth == SADB_AALG_MD5HMAC )
	ah_hmac_md5_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
      else if ( sav->alg_auth ==  SADB_AALG_SHA1HMAC )
	ah_hmac_sha1_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
#if 0
      else if ( sav->alg_auth ==  SADB_AALG_DESHMAC )
	ah_hmac_des_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
#endif
      else if ( sav->alg_auth ==  SADB_X_AALG_SHA2_224 )
	ah_sha224_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
      else if ( sav->alg_auth ==  SADB_X_AALG_SHA2_256 )
	ah_sha256_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
#if 0
      else if ( sav->alg_auth ==  SADB_X_AALG_SHA2_384 )
	ah_sha384_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
      else if ( sav->alg_auth ==  SADB_X_AALG_SHA2_512 )
	ah_sha512_loop(&algos, (caddr_t)&iphdr, sizeof(struct ip));
#endif

      if(hlen != sizeof(struct ip)) {
	dbg_printf(("AH: hlen Error  none option.\n"));
	return(-1);
      }
      
      hdrtype = (iphdr.ip_p) & 0xff;
      advancewidth = hlen;
      break;
    }

  case IPPROTO_AH:
    {
      struct ah ah;
      int siz;
      int hdrsiz;
      int totlen;

      m_copydata(m, off, sizeof(ah), (caddr_t)&ah);
      hdrsiz = sizeof(struct newah);
      siz = 12;
      totlen = (ah.ah_len + 2) << 2;
      /*
       * special treatment is necessary for the first one, not others
       */
      if (!ahseen) {
	m_copydata(m, off, totlen, pIpsec->hash_mbuf);
	bzero(pIpsec->hash_mbuf + hdrsiz, siz);
	if( sav->alg_auth == SADB_AALG_MD5HMAC )
	  ah_hmac_md5_loop(&algos, pIpsec->hash_mbuf, totlen);
	else if ( sav->alg_auth == SADB_AALG_SHA1HMAC )
	  ah_hmac_sha1_loop(&algos, pIpsec->hash_mbuf, totlen);
#if 0
	else if ( sav->alg_auth == SADB_AALG_DESHMAC )
	  ah_hmac_des_loop(&algos, pIpsec->hash_mbuf, totlen);
#endif
	else if ( sav->alg_auth == SADB_X_AALG_SHA2_224 )
	  ah_sha224_loop(&algos, pIpsec->hash_mbuf, totlen);
	else if ( sav->alg_auth == SADB_X_AALG_SHA2_256 )
	  ah_sha256_loop(&algos, pIpsec->hash_mbuf, totlen);
#if 0
	else if ( sav->alg_auth == SADB_X_AALG_SHA2_384 )
	  ah_sha384_loop(&algos, pIpsec->hash_mbuf, totlen);
	else if ( sav->alg_auth == SADB_X_AALG_SHA2_512 )
	  ah_sha512_loop(&algos, pIpsec->hash_mbuf, totlen);
#endif
      }
      else
	ah_update_mbuf(m, off, totlen, sav->alg_auth, &algos);

      ahseen++;

      hdrtype = ah.ah_nxt;
      advancewidth = totlen;
      break;
    }

  default:
    ah_update_mbuf(m, off, m->m_total - off, sav->alg_auth, &algos);
    advancewidth = m->m_total - off;
    break;
  }

  off += advancewidth;
  if( off < m->m_total )
    goto again;

  if (len < 12 ) {
    dbg_printf(("AH: sumsiz Error %d\n",len));
    return(-1);
  }

  if( sav->alg_auth == SADB_AALG_MD5HMAC )
    ah_hmac_md5_result(&algos, &sumbuf[0]);
  else if( sav->alg_auth == SADB_AALG_SHA1HMAC )
    ah_hmac_sha1_result(&algos, &sumbuf[0]);
#if 0
  else if( sav->alg_auth == SADB_AALG_DESHMAC )
    ah_hmac_des_result(&algos, &sumbuf[0]);
#endif
  else if( sav->alg_auth == SADB_X_AALG_SHA2_224 )
    ah_sha224_result(&algos, &sumbuf[0]);
  else if( sav->alg_auth == SADB_X_AALG_SHA2_256 )
    ah_sha256_result(&algos, &sumbuf[0]);
#if 0
  else if( sav->alg_auth == SADB_X_AALG_SHA2_384 )
    ah_sha384_result(&algos, &sumbuf[0]);
  else if( sav->alg_auth == SADB_X_AALG_SHA2_512 )
    ah_sha512_result(&algos, &sumbuf[0]);
#endif
  
  memcpy(ahdat, &sumbuf[0], 12);
  
  return(error);
}

