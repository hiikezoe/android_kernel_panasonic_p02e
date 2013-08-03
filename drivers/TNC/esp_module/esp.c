/*
 * esp.c
 *
 */
#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#ifdef IPSEC_DEBUG
#include "debug.h"
#endif
extern PADAPT pAdapt;
/*
 * Modify the packet so that the payload is encrypted.
 * The mbuf (m) must start with IPv4 or IPv6 header.
 * On failure, free the given mbuf and return NULL.
 *
 * on invocation:
 *	m   nexthdrp md
 *	v   v        v
 *	IP ......... payload
 * during the encryption:
 *	m   nexthdrp mprev md
 *	v   v        v     v
 *	IP ............... esp iv payload pad padlen nxthdr
 *	                   <--><-><------><--------------->
 *	                   esplen plen    extendsiz
 *	                       ivlen
 *	                   <-----> esphlen
 *	<-> hlen
 *	<-----------------> espoff
 */
int
esp_output(pIpsec, m, nexthdrp, isr)
PIPSEC pIpsec;
struct mbuf *m;
u_char *nexthdrp;
struct ipsecrequest *isr;
{
  struct mbuf *md;
  struct newesp *esp;
  struct esptail *esptail;
  u_int8_t nxt = 0;
  size_t plen;	/*payload length to be encrypted*/
  size_t espoff;
  int ivlen;
  size_t extendsiz;
  const struct esp_algorithm *algo;
  struct secasvar *sav = isr->sav;

  /*printk(KERN_ERR "==================esp_output===================\n");*/
  DBG_ENTER(esp_output);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_output \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  md = m->m_next;

  ivlen = sav->ivlen;
  /* should be okey */
  if (ivlen < 0) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: invalid ivlen(%d)",ivlen);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: invalid ivlen(%d)",ivlen);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    mfree_m(m);
    return(-1);
  }

  {
    /*
     * insert ESP header.
     * XXX inserts ESP header right after IPv4 header.  should
     * chase the header chain.
     * XXX sequential number
     */
    struct ip *ip = NULL;
    size_t esphlen;	/*sizeof(struct esp/newesp) + ivlen*/
    size_t hlen = 0;	/*ip header len*/

    esphlen = sizeof(struct newesp) + ivlen;

    if( md->m_next ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: mbuf error buf is 2.\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: mbuf error buf is 2.\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      //	  hexdump("ip header", mtod(m, PCHAR), 20);
	  DBGPRINT("md:%x(%d),md->m_next:%x(%d)\n", (u_int)md, md->m_len, (u_int)md->m_next, md->m_next->m_len);
      mfree_m(m);
      return(-1);
    }

    plen = md->m_len;

    ip = mtod(m, struct ip *);
    hlen = ip->ip_hl << 2;

    espoff = m->m_len;
    if(md->offset  < (int)esphlen) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: offset error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: offset error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      mfree_m(m);
      return(-1);
    }
    md->m_len += esphlen;
    md->m_data -= esphlen;
    md->offset -= esphlen;
    m->m_total += esphlen;
    esp = mtod(md, struct newesp *);

    nxt = *nexthdrp;
    *nexthdrp = IPPROTO_ESP;
    if ((u_short)esphlen < (IP_MAXPACKET - ntohs(ip->ip_len)))
      ip->ip_len = htons((ntohs(ip->ip_len) + (u_short)esphlen));
    else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: size exceeds limit\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: size exceeds limit\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      pIpsec->ipsec_stat.out_inval++;
      mfree_m(m);
      return(-1);
    }
  }

  algo = esp_algorithm_lookup(sav->alg_enc);
  if (!algo) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "esp_output: unsupported algorithm: "
#else /* BUILD_ANDROID */
    printk(KERN_ERR "esp_output: unsupported algorithm: "
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		"SPI=%u\n", (u_int32_t)ntohl(sav->spi));
    mfree_m(m);
    return(-1);
  }
  
  {
    u_long lo;
    /* initialize esp header. */
    memcpy(&esp->esp_spi, &sav->spi, sizeof(u_int32_t));
    sav->seq++;
    if(sav->seq == 0)
      sav->seq = 1;
    lo = htonl(sav->seq);
    memcpy(&esp->esp_seq, &lo, sizeof(u_int32_t));
  }
  
  {
    /*
     * find the last mbuf. make some room for ESP trailer.
     * XXX new-esp authentication data
     */
    struct ip *ip = NULL;
    size_t padbound;
    u_char *extend;
    int i;

    if( algo->padbound )
      padbound = algo->padbound;
    else
      padbound = 4;
	
    extendsiz = padbound - (plen % padbound);
    if (extendsiz == 1)
      extendsiz = padbound + 1;

    /*
       Add Padding 
     */
    i = md->offset + md->m_len;
    if( (int)extendsiz < (2048 - i) )  {
      extend = mtod(md, u_char *) + md->m_len;
      md->m_len += extendsiz;
      m->m_total += extendsiz;
    }
    else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: padding buff error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: padding buff error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      mfree_m(m);
      return(-1);
    }
    for (i = 0; i < (int)extendsiz; i++)
      extend[i] = (i + 1) & 0xff;


    /* initialize esp trailer. */
    esptail = (struct esptail *)
      (mtod(md, u_int8_t *) + md->m_len - sizeof(struct esptail));
    esptail->esp_nxt = nxt;
    esptail->esp_padlen = extendsiz - 2;

    /* modify IP header (for ESP header part only) */
    ip = mtod(m, struct ip *);
    if( (u_short)extendsiz < (IP_MAXPACKET - ntohs(ip->ip_len)) )
      ip->ip_len = htons((ntohs(ip->ip_len) + extendsiz));
    else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: size exceeds limit\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: size exceeds limit\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      pIpsec->ipsec_stat.out_inval++;
      mfree_m(m);
      return(-1);
    }
  }
  /* 
  printk(KERN_ERR "dump 4\n");
  //hex_dump(m->m_next->m_data,m->m_next->m_len);
  */
  /*
   * pre-compute and cache intermediate key
   */
  if( esp_schedule( pIpsec, algo, sav) < 0 ) {
    pIpsec->ipsec_stat.out_inval++;
    mfree_m(m);
    return(-1);
  }
  /*
   * encrypt the packet, based on security association
   * and the algorithm specified.
   */
  if ((*algo->encrypt)(m, plen + extendsiz, sav, algo, ivlen)) {
    /* m is already freed */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "packet encryption failure\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "packet encryption failure\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    pIpsec->ipsec_stat.out_inval++;
    mfree_m(m);
    return(-1);
  }

  /*
   Auth
   */

/*printk(KERN_ERR "================== IN esp_output===================\n");*/
if(sav->alg_auth != SADB_AALG_NONE) 
{
  if((sav->key_auth)&&(sav->alg_auth) 
     && (sav->alg_auth != SADB_X_AALG_NULL)) {
    u_char authbuf[AH_MAXSUMSIZE];
    struct mbuf *n;
    u_char *p;
    size_t siz;
    struct ip *ip;

    siz = 12; /* siz1 = ((siz + 3) & ~(4 - 1)) */
    if( esp_auth(pIpsec, m, espoff, m->m_total-espoff, sav, authbuf) ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: checksum generation failure\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: checksum generation failure\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      mfree_m(m);
      pIpsec->ipsec_stat.out_inval++;
      return(-1);
    }
    n = m;
    while (n->m_next)
      n = n->m_next;

    n->m_len += siz; /* must mbuf is two */
    m->m_total += siz;
    p = mtod(n, u_char *) + n->m_len - siz;
    memcpy(p, authbuf, siz);

    /* modify IP header (for ESP header part only) */
    ip = mtod(m, struct ip *);
    if ((u_short)siz < (IP_MAXPACKET - ntohs(ip->ip_len)))
      ip->ip_len = htons((ntohs(ip->ip_len) + siz));
    else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: output size exceeds limit\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: output size exceeds limit\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      pIpsec->ipsec_stat.out_inval++;
      mfree_m(m);
      return(-1);
    }
  }
}

  pIpsec->ipsec_stat.out_success++;
  pIpsec->ipsec_stat.out_esphist++;
  /* key_sa_recordxfer(sav, m); */
  return 0;
}




int
esp_input( pIpsec, m )
PIPSEC pIpsec;
struct mbuf *m;
{
  struct ip *ip;
  struct newesp *newesp;
  struct esptail esptail;
  u_int32_t spi;
  struct secasvar *sav = NULL;
  size_t taillen;
  u_int16_t nxt;
  int ivlen;
  size_t hlen;
  size_t esplen;
  const struct esp_algorithm *algo;
  struct ipsecstat *ipsecstat=&pIpsec->ipsec_stat;

  /*printk(KERN_ERR "==================esp_input===================\n");*/
  DBG_ENTER(esp_input);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call esp_input \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  ip = mtod(m, struct ip *);
  hlen = ip->ip_hl << 2;


  /* sanity check for alignment. */
  if (hlen % 4 != 0 || m->m_total % 4 != 0) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP:  input packet alignment problem (off=%d, pktlen=%d)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP:  input packet alignment problem (off=%d, pktlen=%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	       hlen, m->m_total);
    ipsecstat->in_inval++;
    return(-1);
  }

  if (m->m_len < (int)(hlen + sizeof(struct newesp))) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: input can't pullup in esp4_input\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: input can't pullup in esp4_input\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ipsecstat->in_inval++;
    return(-1);
  }

  newesp = (struct newesp *)(((u_int8_t *)ip) + hlen);
  
  /* find the sassoc. */
  memcpy(&spi, &newesp->esp_spi, sizeof(u_int32_t));
  if((sav = get_sav(pIpsec, ip->ip_src.s_addr, ip->ip_dst.s_addr,
		IPPROTO_ESP, spi)) == 0) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: input no key association found for spi 0x%x\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: input no key association found for spi 0x%x\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		    (u_int32_t)ntohl(spi));
    ipsecstat->in_nosa++;
    return(-1);
  }
  /*
  if(sav->state != SADB_SASTATE_MATURE
     && sav->state != SADB_SASTATE_DYING) {
    printk(KERN_ERR "ESP: input non-mature/dying SA found for spi 0x%x\n",
	       (u_int32_t)ntohl(spi));
    ipsecstat->in_badspi++;
    return(-1);
  }
  */
  /*
  if (sav->alg_enc == SADB_EALG_NONE) {
    printk(KERN_ERR "ESP: input unspecified encryption algorithm for spi 0x%x\n",
	       (u_int32_t)ntohl(spi));
    ipsecstat->in_badspi++;
    return(-1);
  }
  */
  /* check if we have proper ivlen information */
  ivlen = sav->ivlen;
  if(ivlen < 0) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "inproper ivlen in IPv4 ESP input\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "inproper ivlen in IPv4 ESP input\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ipsecstat->in_inval++;
    return(-1);
  }

#ifdef notdef
  /*
   * check for sequence number.
   */
  if( sav->replay ) {
    if(ipsec_chkreplay(ntohl((newesp->esp_seq), sav))
      ; /*okey*/
    else {
      ipsecstat->in_espreplay++;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: replay packet in IPv4 ESP input: \n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: replay packet in IPv4 ESP input: \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      return(-1);
    }
  }
#endif
  
/*printk(KERN_ERR "==================IN esp_input===================\n");*/
if(sav->alg_auth != SADB_AALG_NONE) 
{
  if( (sav->alg_auth) && (sav->key_auth) 
     && (sav->alg_auth != SADB_X_AALG_NULL) )
  /* check ICV */
  {
    u_char sum0[AH_MAXSUMSIZE];
    u_char sum[AH_MAXSUMSIZE];
    size_t siz;
    
    siz = 12; 

    if(sav->alg_auth != SADB_ORG_AALG_NULL){
      m_copydata(m, m->m_total - siz, siz, &sum0[0]);
      if(esp_auth(pIpsec, m, hlen, m->m_total - hlen - siz, sav, sum)) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "ESP: auth fail in IPv4 ESP input\n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "ESP: auth fail in IPv4 ESP input\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	ipsecstat->in_espauthfail++;
	return(-1);
      }
      
      if(memcmp(sum0, sum, siz) != 0) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "ESP: auth fail(memcmp) in IPv4 ESP input\n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "ESP: auth fail(memcmp) in IPv4 ESP input\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	ipsecstat->in_espauthfail++;
	return(-1);
      }
    }

    /* strip off the authentication data */
    /* m_adj(m, -siz); */
    m->m_len -= siz;
    m->m_total -= siz;
    ip = mtod(m, struct ip *);
    ip->ip_len = htons((ntohs(ip->ip_len) - siz));
    ipsecstat->in_espauthsucc++;
  }
}

#ifdef notdef
  /*
   * update sequence number.
   */
  if( sav->replay ) {
    if(ipsec_updatereplay(ntohl(((struct newesp *)esp)->esp_seq), sav)) {
      ipsecstat->in_espreplay++;
      return(-1);
    }
  }
#endif

  algo = esp_algorithm_lookup(sav->alg_enc);
  if (!algo) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "esp_input: unsupported algorithm: "
#else /* BUILD_ANDROID */
    printk(KERN_ERR "esp_input: unsupported algorithm: "
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		"SPI=%u\n", (u_int32_t)ntohl(sav->spi));
    ipsecstat->in_inval++;
    return(-1);
  }

  esplen = sizeof(struct newesp);

  if(m->m_total < (int)(hlen + esplen + ivlen + sizeof(esptail))) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: input packet too short\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: input packet too short\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ipsecstat->in_inval++;
    return(-1);
  }

  if(m->m_len < (int)(hlen + esplen + ivlen)) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: input can't pullup in esp4_input\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: input can't pullup in esp4_input\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ipsecstat->in_inval++;
    return(-1);
  }

  /*
   * pre-compute and cache intermediate key
   */
  if( esp_schedule( pIpsec, algo, sav) < 0 ) {
    pIpsec->ipsec_stat.in_inval++;
    return(-1);
  }


  /*
   * decrypt the packet.
   */
  if ((*algo->decrypt)(m, hlen, sav, algo, ivlen)) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: decrypt fail in IPv4 ESP input\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: decrypt fail in IPv4 ESP input\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ipsecstat->in_inval++;
    return(-1);
  }
  ipsecstat->in_esphist++;


  /*
   * find the trailer of the ESP.
   */
  m_copydata(m, m->m_total - sizeof(esptail), sizeof(esptail),
	     (caddr_t)&esptail);
  nxt = esptail.esp_nxt;
  taillen = esptail.esp_padlen + sizeof(esptail);

  if((m->m_total < (int)taillen)||((m->m_total - (int)taillen) < (int)hlen)) {	/*?*/
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP: bad pad length(%d) in IPv4 ESP input\n",esptail.esp_padlen);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ESP: bad pad length(%d) in IPv4 ESP input\n",esptail.esp_padlen);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ipsecstat->in_inval++;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "enc_algo = %d\n",sav->alg_enc);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "enc_algo = %d\n",sav->alg_enc);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return(-1);
  }
  /* strip off the trailing pad area. 
	m_adj(m, -taillen);
  */
  m->m_len -= taillen;
  m->m_total -= taillen;

  ip->ip_len = htons((ntohs(ip->ip_len) - taillen));

  /* was it transmitted over the IPsec tunnel SA? */
  if( ipsec4_tunnel_validate(ip, nxt, sav) ) {

    /*
     * strip off all the headers that precedes ESP header.
     *	IP4 xx ESP IP4' payload -> IP4' payload
     *
     * XXX more sanity checks
     * XXX relationship with gif?
     */
    u_int8_t tos;

//    DBG_printf("over the tunnel\n");

    tos = ip->ip_tos;
    /* m_adj(m, hlen + esplen + ivlen); */
    hlen += (esplen + ivlen);
    m->m_data += hlen;
    m->m_len -= hlen;
    m->offset += hlen;
    m->m_total -= hlen;

    if(m->m_len < sizeof(*ip)) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: mbuf error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: mbuf error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      ipsecstat->in_inval++;
      return(-1);
    }
    /* ip = mtod(m, struct ip *);
    ECN consideration. *
    ip_ecn_egress(ip4_ipsec_ecn, &tos, &ip->ip_tos);
    key_sa_recordxfer(sav, m);
    */
  }
  else {
    /*
     * strip off ESP header and IV.
     * even in m_pulldown case, we need to strip off ESP so that
     * we can always compute checksum for AH correctly.
     */
    int sum;
    size_t stripsiz;
    u_char hdr[IP_HEADER_LEN];

    if( hlen != IP_HEADER_LEN ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ESP: bad ip header len\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ESP: bad ip header len\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      ipsecstat->in_inval++;
      return(-1);
    }
    stripsiz = esplen + ivlen;

    ip = mtod(m, struct ip *);

    memcpy((caddr_t)hdr, (caddr_t)ip, hlen);

    m->m_data += stripsiz;
    m->m_len -= stripsiz;
    m->m_total -= stripsiz;
    m->offset += stripsiz;

    ip = mtod(m, struct ip *);
    memcpy((caddr_t)ip, hdr, hlen);

    ip->ip_len = htons((ntohs(ip->ip_len) - (u_short)stripsiz));
    ip->ip_p = (u_char)nxt;

    ip->ip_sum = 0;
    /* make ip checksum */
    sum = libnet_in_cksum((u_short *)ip, hlen);
    ip->ip_sum = (u_short)(LIBNET_CKSUM_CARRY(sum));

    /* for DIGA @yoshino */
    ipsecstat->in_success++;
  }
  if(sav->sa_next){ //matsuura
    pAdapt->Ipsec_SL.dec_count.sa0++;
  }
  else{
    pAdapt->Ipsec_SL.dec_count.sa1++;
  }

  /* for DIGA @yoshino */
  /* ipsecstat->in_success++; */
  
  return(0);
}
