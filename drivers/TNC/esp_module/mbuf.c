/*
 * mbuf.c
 *
 */
#include <linux/spinlock.h>
#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"

extern spinlock_t GlobalLock;

void
init_mbuf( void *p )
{
  PIPSEC pIpsec=(PIPSEC)p;
  int i;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call init_mbuf \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  for(i=0; i<MB_MAX; i++ ) {
    pIpsec->MBuf[i].use = 0;
    pIpsec->MBuf[i].flg = 0;
  }
  for(i=0; i<BM_SEND_BUFF; i++) {
	pIpsec->bm_buff[i].use = 0;
	pIpsec->bm_buff[i].flg = 0;
  }

  for(i=0; i<BM_RECV_BUFF; i++) {
	pIpsec->bm_buff2[i].use = 0;
	pIpsec->bm_buff2[i].flg = 0;
  }
}

struct mbuf *
mget( PIPSEC pIpsec )
{
  int i;
  struct mbuf *ret=NULL;


/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call mget \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  spin_lock(&GlobalLock);
  for(i=0; i<MB_MAX; i++ ) {
    if( !pIpsec->MBuf[i].use ) {
      pIpsec->MBuf[i].m_data = (u_char *)&(pIpsec->MBuf[i].buf[10]);
      pIpsec->MBuf[i].offset = 10;
      pIpsec->MBuf[i].m_len = 0;
      pIpsec->MBuf[i].m_next = 0;
      pIpsec->MBuf[i].use = 1;
      ret = (struct mbuf *)&pIpsec->MBuf[i];
	  break;
    }
  }
  spin_unlock(&GlobalLock);

  return ret;
}

void
mfree( m )
struct mbuf *m;
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call mfree \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  m->use = 0;
}

void
mfree_m(m)
struct mbuf *m;
{
  struct mbuf *m1;


/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call mfree_m \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  m1 = m;
  while(m1) {
    /* if( !m->flg ) */
      m1->use = 0;
    m = m1->m_next;
	m1->m_next = NULL;
	m1 = m;
  }
}

#define MIN(a,b) (((a)<(b))?(a):(b))

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void    
m_copydata(m, off, len, cp)
register struct mbuf *m;
register int off;
register int len;
caddr_t cp;
{
  register unsigned count;


/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call m_copydata \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if (off < 0 || len < 0) {
    dbg_printf(("Error m_copydata\n"));
  }

  while (off > 0) {
    if (m == 0) {
      dbg_printf(("Error m_copydata 2\n"));
    }
    if (off < m->m_len)
      break;
    off -= m->m_len;
    m = m->m_next;
  }
  while (len > 0) {
    if (m == 0){
      dbg_printf(("Error m_copydata 3\n"));
    }
    count = MIN(m->m_len - off, len);
    memcpy(cp, mtod(m, caddr_t) + off, count);
    len -= count;
    cp += count;
    off = 0;
    m = m->m_next;
  }
}


