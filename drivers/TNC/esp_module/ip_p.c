/*
 * ip_p.c
 *
 */

#include "linux_precomp.h"

/*
 * make check sum
 */
u_short 
in_cksum(addr, len)
u_short *addr;
int len;
{
  register int nleft = len;
  register u_short *w = addr;
  register int sum = 0;
  u_short answer = 0;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call in_cksum \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)  {
    sum += htons(*w++);
    nleft -= 2;
  }
 
  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(u_char *)(&answer) = (u_char)htons(*(u_char *)w) ;
    sum += answer;
  }
 
  /* add back carry outs from top 16 bits to low 16 bits */
  sum = ((sum >> 16) + (sum & 0xffff));     /* add hi 16 to low 16 */
  sum += (sum >> 16);                     /* add carry */
  answer = ~sum;                          /* truncate to 16 bits */
  return (answer);
}



int
libnet_in_cksum(u_short *addr, int len)
{
  int sum;
  int nleft;
  u_short ans;
  u_short *w;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call libnet_in_cksum \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  sum = 0;
  ans = 0;
  nleft = len;
  w = addr;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }
  if (nleft == 1) {
    *(u_char *)(&ans) = *(u_char *)w;
    sum += ans;
  }
  return (sum);
}

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
EXPORT_SYMBOL(in_cksum);
#endif
