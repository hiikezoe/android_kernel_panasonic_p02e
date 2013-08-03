/* ah.c 
 * 
 *
 */
#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"


/*
 * Modify the packet so that it includes the authentication data.
 * The mbuf passed must start with IPv4 header.
 *
 * assumes that the first mbuf contains IPv4 header + option only.
 * the function does not modify m.
 */
int
ah_output(pIpsec, m, isr)
PIPSEC pIpsec;
struct mbuf *m;
struct ipsecrequest *isr;
{
  u_char *ahdrpos;
  u_char *ahsumpos = NULL;
  size_t hlen = 0;	/*IP header+option in bytes*/
  size_t plen = 0;	/*AH payload size in bytes*/
  size_t ahlen = 0;	/*plen + sizeof(ah)*/
  struct ip *ip;
  int error;
  struct secasvar *sav = isr->sav;
  DBG_ENTER(ah_output);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_output \n");
#endif /* BUILD_ANDRODI */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  /*
   * determine the size to grow.
   */
  /* RFC 2402 */
  plen = (12 + 3) & ~(4 - 1); /*XXX pad to 8byte?*/
  ahlen = plen + sizeof(struct newah);
  
  /*
   * grow the mbuf to accomodate AH.
   */
  ip = mtod(m, struct ip *);
  hlen = ip->ip_hl << 2;

  if ((size_t)m->m_len != hlen) {
    dbg_printf(("AH: ah4_output: assumption failed (first mbuf length)"));
  }

  if( m->m_next->offset < (int)ahlen ) {
    dbg_printf(("AH: mbuf offset error\n"));
    mfree_m(m);
    return(-1);
  }
  m->m_next->m_len += ahlen;
  m->m_next->m_data -= ahlen;
  m->m_next->offset -= ahlen;
  m->m_total += ahlen;
  ahdrpos = mtod(m->m_next, u_char *);
  ip = mtod(m, struct ip *);	/*just to be sure*/
  /*
   * initialize AH.
   */
  {
    struct newah *ahdr;
    u_long lo;

    ahdr = (struct newah *)ahdrpos;
    ahsumpos = (u_char *)(ahdr + 1);
    ahdr->ah_len = (plen >> 2) + 1;	/* plus one for seq# */
    ahdr->ah_nxt = ip->ip_p;
    ahdr->ah_reserve = 0;
    /* ahdr->ah_spi = sav->spi; */
    memcpy(&ahdr->ah_spi, &sav->spi, sizeof(u_int32_t) );
    sav->seq++;
    /* ahdr->ah_seq = htonl(1); */
    lo = htonl(sav->seq);
    memcpy(&ahdr->ah_seq, &lo, sizeof(u_int32_t) );
    bzero((u_char *)(ahdr + 1), plen);
  }

  /*
   * modify IPv4 header.
   */
  ip->ip_p = IPPROTO_AH;
  if ((u_long)ahlen < (IP_MAXPACKET - (u_long)(ntohs(ip->ip_len))))
    ip->ip_len = htons((ntohs(ip->ip_len) + (u_long)ahlen));
  else {
    dbg_printf(("AH: output: size exceeds limit\n"));
    pIpsec->ipsec_stat.out_inval++;
    mfree_m(m);
    return(-1);
  }
  {
    int sum;

    if ( (isr->saidx.mode == IPSEC_MODE_TUNNEL) ||
			(isr->saidx.mode == IPSEC_MODE_UDP_TUNNEL) ) {
      ip->ip_dst.s_addr = isr->saidx.src.s_addr;
      ip->ip_src.s_addr = isr->saidx.dst.s_addr;
    }
    ip->ip_sum = 0;
    /* make ip checksum */
    sum = libnet_in_cksum((u_short *)ip, hlen);
    ip->ip_sum = (u_short)(LIBNET_CKSUM_CARRY(sum));
  }
  /*
   * calcurate the checksum, based on security association
   * and the algorithm specified.
   */
  error = ah4_calccksum(pIpsec, m, (caddr_t)ahsumpos, plen, sav);
  if(error) {
    dbg_printf(("AH: error after ah4_calccksum, called from ah4_output\n"));
    mfree_m(m);
    m = NULL;
    pIpsec->ipsec_stat.out_inval++;
    return(error);
  }

  pIpsec->ipsec_stat.out_success++;
  pIpsec->ipsec_stat.out_ahhist++;
  /* key_sa_recordxfer(sav, m); */

  return 0;
}


int
ah_input( pIpsec, m )
PIPSEC pIpsec;
struct mbuf *m;
{
  struct ip *ip;
  struct newah *ah;
  u_int32_t spi;
  size_t siz1;
  u_char cksum[12];
  struct secasvar *sav = NULL;
  u_int16_t nxt;
  size_t hlen;
  struct ipsecstat *ipsecstat=&pIpsec->ipsec_stat;

  DBG_ENTER(ah_input);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ah_input \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  ip = mtod(m, struct ip *);
  hlen = ip->ip_hl << 2;

  ah = (struct newah *)((u_char *)ip + hlen);
  nxt = ah->ah_nxt;

  /* find the sassoc. */
  memcpy(&spi, &ah->ah_spi, sizeof(u_int32_t));

  if((sav = get_sav(pIpsec, ip->ip_src.s_addr, ip->ip_dst.s_addr,
		IPPROTO_AH, spi)) == 0) {
    dbg_printf(("AH: Input no key association found for spi %d\n",ntohl(spi)));
    ipsecstat->in_nosa++;
    return(-1);
  }
/*
  if(sav->state != SADB_SASTATE_MATURE
      && sav->state != SADB_SASTATE_DYING) {
    dbg_printf("AH: input non-mature/dying SA found for spi %d\n",ntohl(spi));
    ipsecstat->in_badspi++;
    return(-1);
  }
*/
/*
  if(sav->alg_auth == SADB_AALG_NONE) {
    dbg_printf(("AH: input unspecified authentication algorithm for spi %d\n",ntohl(spi)));
    ipsecstat->in_badspi++;
    return(-1);
  }
*/

  siz1 = 12; /* siz1 = ((siz + 3) & ~(4 - 1)) */

  /*
   * sanity checks for header, 1.
   */
  if((size_t)((ah->ah_len << 2) - 4) != siz1) {
    dbg_printf(("aAH: sum length mismatch in IPv4 AH input (%d should be %lu)\n",
	       (ah->ah_len << 2) - 4, (u_long)siz1));
    ipsecstat->in_inval++;
    return(-1);
  }

#ifdef notdef
  /*
   * check for sequence number.
   */
  if( sav->replay ) {
    if(!ipsec_chkreplay(ntohl(ah->ah_seq), sav) ) {
      ipsecstat->in_ahreplay++;
      dbg_printf(("AH: in replay packet in IPv4 AH input: \n"));
      return(-1);
    }
  }
#endif
  if( ah4_calccksum(pIpsec, m, (caddr_t)cksum, siz1, sav) ) {
    dbg_printf(("AH: calccksum error\n"));
    ipsecstat->in_inval++;
    return(-1);
  }
  ipsecstat->in_ahhist++;

  {
    caddr_t sumpos = NULL;

    sumpos = (caddr_t)(((struct newah *)ah) + 1);
    if( memcmp(sumpos, cksum, siz1) != 0) {
      dbg_printf(("AH in checksum mismatch in IPv4 AH input\n"));
      ipsecstat->in_ahauthfail++;
      return(-1);
    }
  }
  ipsecstat->in_ahauthsucc++;

#ifdef notdef
  /*
   * update sequence number.
   */
  if( sav->replay ) {
    if( ipsec_updatereplay(ntohl(ah->ah_seq), sav)) {
      ipsecstat->in_ahreplay++;
      return(-1);
    }
  }
#endif
  
  /* was it transmitted over the IPsec tunnel SA? */
  if (ipsec4_tunnel_validate(ip, nxt, sav) && nxt == IPPROTO_IPV4) {
    /*
     * strip off all the headers that precedes AH.
     *	IP xx AH IP' payload -> IP' payload
     *
     * XXX more sanity checks
     * XXX relationship with gif?
     */
    size_t stripsiz = 0;
    u_int8_t tos;

    tos = ip->ip_tos;
    stripsiz = sizeof(struct newah) + siz1;
    hlen += stripsiz;
    /* m_adj(m, hlen) */
    m->m_data += hlen;
    m->m_len -= hlen;
    m->offset += hlen;
    m->m_total -= hlen;
    ip = mtod(m, struct ip *);
    /* ECN consideration. *
    ip_ecn_egress(ip4_ipsec_ecn, &tos, &ip->ip_tos);
     key_sa_recordxfer(sav, m); 
     */
  }
  else {
    /*
     * strip off AH.
     */
    size_t stripsiz = 0;
    int sum;
    u_char hdr[IP_HEADER_LEN];

    if( hlen != IP_HEADER_LEN ) {
      dbg_printf(("AH: bad ip header len\n"));
      ipsecstat->in_inval++;
      return(-1);
    }

    stripsiz = sizeof(struct newah) + siz1;

    ip = mtod(m, struct ip *);
    memcpy((caddr_t)hdr, (caddr_t)ip, hlen);
    //    memcpy((caddr_t)(((u_char *)ip) + stripsiz), (caddr_t)ip, hlen);
    m->m_data += stripsiz;
    m->m_len -= stripsiz;
    m->m_total -= stripsiz;
    m->offset += stripsiz;

    ip = mtod(m, struct ip *);
    memcpy((caddr_t)ip, hdr, hlen);

    ip = mtod(m, struct ip *);
    ip->ip_len = htons((ntohs(ip->ip_len) - (u_short)stripsiz));
    ip->ip_p = (u_char)nxt;

    ip->ip_sum = 0;
    /* make ip checksum */
    sum = libnet_in_cksum((u_short *)ip, hlen);
    ip->ip_sum = (u_short)(LIBNET_CKSUM_CARRY(sum));

    ipsecstat->in_success++;
  }

  return(0);
}
