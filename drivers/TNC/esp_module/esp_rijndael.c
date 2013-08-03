/*
 * esp_rijndael.c
 *
 */

#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#include "des.h"
#include "rijndael.h"


#define panic(x) do {dbg_printf((x)); return -1;} while (0)


/* as rijndael uses assymetric scheduled keys, we need to do it twice. */
int
esp_rijndael_schedlen(algo)
const struct esp_algorithm *algo;
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("Call esp_rijndael_schedlen  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  return sizeof(keyInstance) * 2;
}

int
esp_rijndael_schedule(algo, sav)
const struct esp_algorithm *algo;
struct secasvar *sav;
{
  keyInstance *k;
  int ret;

  DBG_ENTER(esp_rijndael_schedule);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("Call esp_rijndael_schedule  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  k = (keyInstance *)sav->sched;
  ret = rijndael_makeKey(&k[0], DIR_DECRYPT, _KEYLEN(sav->key_enc) * 8,
			 _KEYBUF(sav->key_enc));
  if (ret < 0 ) {
    dbg_printf(("makeKey err(%d)\n",ret));
    return(-1);
  }
  ret = rijndael_makeKey(&k[1], DIR_ENCRYPT, _KEYLEN(sav->key_enc) * 8,
			 _KEYBUF(sav->key_enc));
  if (ret < 0 ) {
    dbg_printf(("makeKey err(%d)\n",ret));
    return(-1);
  }

  return(0);
}

int
esp_rijndael_blockdecrypt(algo, sched, s, d)
const struct esp_algorithm *algo;
u_char *sched;
u_int8_t *s;
u_int8_t *d;
{
  cipherInstance c;
  keyInstance *p;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("Call esp_rijndael_blockdecrypt  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* does not take advantage of CBC mode support */
  bzero(&c, sizeof(c));
  if (rijndael_cipherInit(&c, MODE_ECB, NULL) < 0)
    return(-1);
  p = (keyInstance *)sched;
  if (rijndael_blockDecrypt(&c, &p[0], s, algo->padbound * 8, d) < 0)
    return(-1);
	
  return(0);
}

int
esp_rijndael_blockencrypt(algo, sched, s, d)
const struct esp_algorithm *algo;
u_char *sched;
u_int8_t *s;
u_int8_t *d;
{
  cipherInstance c;
  keyInstance *p;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("Call esp_rijndael_blockencrypt  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* does not take advantage of CBC mode support */
  bzero(&c, sizeof(c));
  if (rijndael_cipherInit(&c, MODE_ECB, NULL) < 0)
    return(-1);
  p = (keyInstance *)sched;
  if (rijndael_blockEncrypt(&c, &p[1], s, algo->padbound * 8, d) < 0)
    return(-1);
	
  return(0);
}


int 
esp_rijndael(m0, skip, length, schedule, ivec, algo, mode)
struct mbuf *m0;
size_t skip;
size_t length;
u_int8_t *schedule;
u_int8_t *ivec;
const struct esp_algorithm *algo;
int mode;
{
  u_int8_t inbuf[MAXIVLEN];
  struct mbuf *m;
  size_t off;
  int blocklen;
  u_int8_t *ivp;
  u_int8_t *p, *q;
  u_int8_t *sp;
  int i;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("Call esp_algorithm  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  DBG_ENTER(esp_rijndael);

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

  ivp = NULL;

  if (mode == DES_ENCRYPT) {

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
	dbg_printf(("des_cbc_encr mbuf error\n"));
	return(-1);
      }
      sp = inbuf;

      /* xor */
      p = ivp ? ivp : ivec;
      q = sp;
      for (i = 0; i < blocklen; i++)
	q[i] ^= p[i];

      /* encrypt */
      esp_rijndael_blockencrypt(algo, schedule, sp, mtod(m, u_int8_t *)+off);

      /* next iv */
      ivp = mtod(m, u_int8_t *) + off;

      if ((int)(off + blocklen) < m->m_len) {
	off += blocklen;
      }
      else if ((int)(off + blocklen) == m->m_len) {
	m = 0;   /* none m_next */
	off = 0;
      }
      else {
	dbg_printf(("des_cbc_encr mbuf error 2\n"));
	return(-1);
      }
      length -= blocklen;
    }
  } 
  else if (mode == DES_DECRYPT) {
    u_int8_t ivbuf[MAXIVLEN];

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
	dbg_printf(("ESP: in mbuf error\n"));
	return(-1);
      }
      
      sp = inbuf;

      /* decrypt */
      esp_rijndael_blockdecrypt(algo, schedule, sp, mtod(m, u_int8_t *)+off);

      /* xor */
      p = ivp ? ivp : ivec;
      q = mtod(m, u_int8_t *)+off;
      for (i = 0; i < blocklen; i++)
	q[i] ^= p[i];

      /* next iv */
      memcpy(ivbuf, sp, blocklen);
      ivp = ivbuf;

      if((int)(off + blocklen) < m->m_len) {
	off += blocklen;
      }
      else if ((int)(off + blocklen) == m->m_len) {
	off = 0;
      }
      else {
	dbg_printf(("ESP: in mbuf error 2\n"));
      }
      length -= blocklen;
    }
  }
  return 0;
}
