/*
 * ipsec.c
 *
 */
#include <linux/spinlock.h>
#include <linux/slab.h>
#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#include "packet_buff.h"


extern spinlock_t GlobalLock;

/* 
    ipsec_attach
    Memory Allcate
*/
int ipsec_attach( void* Adapter )
{
  PIPSEC pIpsec=&((pADAPTER)Adapter)->Ipsec_SL.Ipsec;
  int i;

  DBG_ENTER(ipsec_attach);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_attach \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  /* Version Display */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT("<IPsec Ver.0.50 (2003.06.06)>\n");
#else /* BUILD_ANDROID */
  printk("<IPsec Ver.0.50 (2003.06.06)>\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  bzero( (char*)pIpsec, sizeof(IPSEC));

  pIpsec->hash_buf = (u_char*)kmalloc(HASHB_LEN,GFP_KERNEL);
  if(pIpsec->hash_buf == NULL){
    return IPSEC_ERROR;
  }

  pIpsec->hash_mbuf = (u_char*)kmalloc(HASHM_LEN,GFP_KERNEL);
  if(pIpsec->hash_mbuf == NULL){
    return IPSEC_ERROR;
  }

  for( i=0; i<4; i++ ) {
    pIpsec->sched[i] = (u_char*)kmalloc(DESKS_LEN,GFP_KERNEL);
    if(pIpsec->sched[i] == NULL){
      return IPSEC_ERROR;
    }
  }

  pIpsec->MBuf = (struct mbuf*)kmalloc(sizeof(struct mbuf)*MB_MAX,GFP_KERNEL);
  if(pIpsec->MBuf == NULL){
    return IPSEC_ERROR;
  }

  init_timer(&pIpsec->Key_LifeTimer[FIRST_OUT]);
  pIpsec->Key_LifeTimer[FIRST_OUT].function = ipsec_Timeup0;
  pIpsec->Key_LifeTimer[FIRST_OUT].data = (unsigned long)pIpsec;
  init_timer(&pIpsec->Key_LifeTimer[SECOND_OUT]);
  pIpsec->Key_LifeTimer[SECOND_OUT].function = ipsec_Timeup1;
  pIpsec->Key_LifeTimer[SECOND_OUT].data = (unsigned long)pIpsec;
  init_timer(&pIpsec->Key_LifeTimer[FIRST_IN]);
  pIpsec->Key_LifeTimer[FIRST_IN].function = ipsec_Timeup0_in;
  pIpsec->Key_LifeTimer[FIRST_IN].data = (unsigned long)pIpsec;
  init_timer(&pIpsec->Key_LifeTimer[SECOND_IN]);
  pIpsec->Key_LifeTimer[SECOND_IN].function = ipsec_Timeup1_in;
  pIpsec->Key_LifeTimer[SECOND_IN].data = (unsigned long)pIpsec;

  return( ipsec_init( Adapter ));

}

/* 
    ipsec_detach

                 Memory Free
*/
void ipsec_detach( void* Adapter )
{
  PIPSEC pIpsec=&((pADAPTER)Adapter)->Ipsec_SL.Ipsec;
  int i;

  DBG_ENTER(ipsec_detach);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_detach \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  /* Timer Cancel */
  for(i=0; i<KEY_NUM; i++){
    if( pIpsec->life_timer[i] )
      del_timer(&pIpsec->Key_LifeTimer[i]);
  }

  /* Memory Free */
  kfree(pIpsec->hash_buf);
  kfree(pIpsec->hash_mbuf);
  for( i=0; i<4; i++ ) {
    kfree(pIpsec->sched[i]);
  }

  kfree(pIpsec->MBuf);

  pIpsec->init = 0;
}


/* 
    ipsec_init

                 Initialize, Reset
*/
int ipsec_init( void* Adapter )
{
  PIPSEC pIpsec=&((pADAPTER)Adapter)->Ipsec_SL.Ipsec;
  int i;

  DBG_ENTER(ipsec_init);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  pIpsec->ip4_ah_cleartos = 1;
  pIpsec->ip4_ah_offsetmask = 0;
  pIpsec->ip4_ipsec_dfbit = 0;
  pIpsec->ip4_ipsec_ecn = ECN_NOCARE; /* ? */

  pIpsec->ip4_def_policy.policy = IPSEC_POLICY_NONE;
  pIpsec->ip4_def_policy.key_mode = IPSEC_KEY_MANUAL;

  pIpsec->ip4_def_policy.req = (struct ipsecrequest *)&pIpsec->def_isr[0];

  /* Request for IPsec */
  for(i=0; i<KEY_NUM; i++){
    if(i == KEY_NUM - 1)
      pIpsec->def_isr[i].next = 0; /* AH, ESP 両方はなし */
    else
      pIpsec->def_isr[i].next = &pIpsec->def_isr[i+1];
    pIpsec->def_isr[i].level = 0;
    pIpsec->def_isr[i].saidx.proto = 0;
    pIpsec->def_isr[i].saidx.mode = IPSEC_MODE_ANY;
    pIpsec->def_isr[i].sav = (struct secasvar *)&pIpsec->def_sa[i]; /* SA */
    pIpsec->def_isr[i].sp = (struct secpolicy *)&pIpsec->ip4_def_policy;
  }

  /*  pIpsec->tun_src = 0; */
  /*  pIpsec->tun_dst = 0; */

  memset( pIpsec->def_sa, 0, sizeof(struct secasvar)*4);
  /* SA */
  /* recv 1 */
  pIpsec->def_sa[0].refcnt = 0;
  pIpsec->def_sa[0].sa_next = (struct secasvar *)&pIpsec->def_sa[1];
  /* send 1 */
  pIpsec->def_sa[1].refcnt = 1;
  pIpsec->def_sa[1].sa_next = (struct secasvar *)&pIpsec->def_sa[2];
  /* recv 2 */
  pIpsec->def_sa[2].refcnt = 2;
  pIpsec->def_sa[2].sa_next = (struct secasvar *)&pIpsec->def_sa[3];
  pIpsec->def_sa[3].refcnt = 3;
  pIpsec->def_sa[3].sa_next = 0;

  pIpsec->now_key_no = -1;

  init_mbuf( (void *)pIpsec );

  /* Timer Cancel */
  for(i=0; i<KEY_NUM; i++){
    if( pIpsec->life_timer[i] ) {
      del_timer(&pIpsec->Key_LifeTimer[i]);
      pIpsec->life_timer[i] = 0;
    }
  }
 
  pIpsec->init = 1;
  return(IPSEC_SUCCESS);
}

/* 
    ipsec_key_set

                 Key Set 
*/
int ipsec_key_set( void* Adapter, struct set_ipsec *set )
{
  PIPSEC pIpsec=&((pADAPTER)Adapter)->Ipsec_SL.Ipsec;
  u_short pro;
  u_char mode;
  struct ipsecrequest *isr;

#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "===== Kernel SA Info [ipsec_key_set] =====\n");
#else /* BUILD_ANDROID */
  printk(KERN_ERR "===== Kernel SA Info [ipsec_key_set] =====\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif

  /* Version Display */
  DBG_ENTER(ipsec_key_set);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_key_set \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  mode = (u_char)set->mode;     /* IPSec mode */
  pro  = (u_short)set->protocol; /* IPSec Protocol */
  if(set->direction == OUT_KEY){
    /* now_key_noはkey_setsavalで変更されるので
       ここでは1の場合は1番目の鍵を、0の場合は2番目の鍵を変更する */
    if(pIpsec->now_key_no){
      pIpsec->now_key_no = 0;       
      isr = &pIpsec->def_isr[FIRST_OUT];
      ((pADAPTER)Adapter)->Ipsec_SL.key_lifetime[FIRST_OUT] = set->lifeTime;
#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "SA set FIRST_OUT erea!!\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "SA set FIRST_OUT erea!!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif 
    }
    else{
      pIpsec->now_key_no = 1;
      isr = &pIpsec->def_isr[SECOND_OUT];
      ((pADAPTER)Adapter)->Ipsec_SL.key_lifetime[SECOND_OUT] = set->lifeTime;
#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "SA set SECOND_OUT erea!!\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "SA set SECOND_OUT erea!!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif 
    }
  }
  else{
    if(pIpsec->def_isr[FIRST_IN].sav->spi == 0 || 
       pIpsec->def_isr[FIRST_IN].sav->spi == htonl(set->key.spi)){
      isr = &pIpsec->def_isr[FIRST_IN];
      ((pADAPTER)Adapter)->Ipsec_SL.key_lifetime[FIRST_IN] = set->lifeTime;
#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "SA set FIRST_IN erea!!\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "SA set FIRST_IN erea!!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif 
    }
    else if(pIpsec->def_isr[SECOND_IN].sav->spi == 0 || 
	    pIpsec->def_isr[SECOND_IN].sav->spi == htonl(set->key.spi)){
      isr = &pIpsec->def_isr[SECOND_IN];
      ((pADAPTER)Adapter)->Ipsec_SL.key_lifetime[SECOND_IN] = set->lifeTime;
#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "SA set SECOND_IN erea!!\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "SA set SECOND_IN erea!!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif
    }
    else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "rcv SA full \n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "rcv SA full \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
      return IPSEC_ERROR;
    }
  }

  /* Default Security Policy Data Base */
  pIpsec->ip4_def_policy.refcnt = 0;
  pIpsec->ip4_def_policy.spidx.dir = 0;

#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "mode: %d\n",mode);
  TNC_LOGOUT(KERN_ERR "protocol: %d\n",pro);
#else /* BUILD_ANDROID */
  printk(KERN_ERR "mode: %d\n",mode);
  printk(KERN_ERR "protocol: %d\n",pro);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
#endif

  isr->saidx.src.s_addr = set->tun_src;
  isr->saidx.dst.s_addr = set->tun_dst;
  /* pIpsec->tun_src = set->tun_src; */
  /* pIpsec->tun_dst = set->tun_dst; */
  if(set->direction == OUT_KEY){
    pIpsec->dst_ip = (set->dst_ip & set->ip_mask);
    pIpsec->dst_mask = set->ip_mask;
  }

#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "tun src:  0x%x\n",(u_int)isr->saidx.src.s_addr);
  TNC_LOGOUT(KERN_ERR "tun dst:  0x%x\n",(u_int)isr->saidx.dst.s_addr);
  TNC_LOGOUT(KERN_ERR "dst ip:   0x%x\n",(u_int)pIpsec->dst_ip);
  TNC_LOGOUT(KERN_ERR "dst mask: 0x%x\n",(u_int)pIpsec->dst_mask);
#else /* BUILD_ANDROID */
  printk(KERN_ERR "tun src:  0x%x\n",(u_int)isr->saidx.src.s_addr);
  printk(KERN_ERR "tun dst:  0x%x\n",(u_int)isr->saidx.dst.s_addr);
  printk(KERN_ERR "dst ip:   0x%x\n",(u_int)pIpsec->dst_ip);
  printk(KERN_ERR "dst mask: 0x%x\n",(u_int)pIpsec->dst_mask);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
#endif

  pIpsec->ip4_def_policy.spidx.prefs = (sizeof(struct in_addr) << 3);
  pIpsec->ip4_def_policy.spidx.prefd = (sizeof(struct in_addr) << 3);
  pIpsec->ip4_def_policy.spidx.ul_proto = IPSEC_ULPROTO_ANY;
 
  if( pro ) 
    pIpsec->ip4_def_policy.policy = IPSEC_POLICY_IPSEC;
  else {
    pIpsec->ip4_def_policy.policy = IPSEC_POLICY_NONE;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "key set: IPSEC_POLICY_NONE");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "key set: IPSEC_POLICY_NONE");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    return(IPSEC_SUCCESS);
  }
  /* Manual-Key or IKE */
  pIpsec->ip4_def_policy.key_mode = set->key_mode;

#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "key-mode: %d\n",pIpsec->ip4_def_policy.key_mode);
#else /* BUILD_ANDROID */
  printk(KERN_ERR "key-mode: %d\n",pIpsec->ip4_def_policy.key_mode);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
#endif
  /* Request for IPsec */
  isr->level = 0;
  isr->saidx.proto = pro;
  isr->saidx.mode = mode;

  /* key set */
  if(key_setsaval( pIpsec, set ))
    return IPSEC_ERROR; 

  return(IPSEC_SUCCESS);
}

/*
  ipsec_output()

            from driver-core
*/
int
ipsec_output( void* Adapter, struct pkt_buff *pktb )
{
  PIPSEC pIpsec=&((pADAPTER)Adapter)->Ipsec_SL.Ipsec;
  struct ipsec_output_state state;
  struct ip *ip;
  int ret;
  u_char *buf;
  int out_len;

  /*printk(KERN_ERR "-----------------ipsec_output-------------------\n");*/
  DBG_ENTER(ipsec_output);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_output \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  if( !pIpsec->init ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec_core Don't initialize\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ipsec_core Don't initialize\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    return(0);
  }

  ip = (struct ip *)pktb->data;


/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "Call ipsec_output \n");
  TNC_LOGOUT(KERN_ERR "host: 0x%x\n",(u_int)ip->ip_dst.s_addr);
  TNC_LOGOUT(KERN_ERR "pIpsec->dst_ip: 0x%x\n",(u_int)pIpsec->dst_ip);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  if( pIpsec->dst_ip != (ip->ip_dst.s_addr & pIpsec->dst_mask) ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "Unknown host: IPSEC_DST=0x%x IP_DST=0x%x IPSEC_MASK=0x%x\n", (u_int)pIpsec->dst_ip, (u_int)ip->ip_dst.s_addr, (u_int)pIpsec->dst_mask);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "Unknown host: IPSEC_DST=0x%x IP_DST=0x%x IPSEC_MASK=0x%x\n", (u_int)pIpsec->dst_ip, (u_int)ip->ip_dst.s_addr, (u_int)pIpsec->dst_mask);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
/* そのまま送信するのか？
 　それともエラーとして捨てるのか？*/
    return(-1);
  }

  state.m = out_state_set( pIpsec, pktb->data, pktb->len, &state );
  if(state.m == NULL) {
	  DBGPRINT(("Faile to get mbuf at out_state_set\n"));
	  return -1;
  }

  ret = ipsec4_output( pIpsec, &state );
  if( ret < 0  ) {
    return( ret );
  }
  if( ret == 1 ) {
    buf = restore_mbuf( state.m, &out_len );
#ifdef notdef /* AH Tunnel bug -> move */
    ip = (struct ip *)buf;
    if (pIpsec->def_isr.saidx.mode == IPSEC_MODE_TUNNEL) {
      ip->ip_dst.s_addr = pIpsec->tun_dst;
      ip->ip_src.s_addr = pIpsec->tun_src;
    }
    hlen = ip->ip_hl << 2;
    ip->ip_sum = 0;
    /* make ip checksum */
    sum = libnet_in_cksum((u_short *)ip, hlen);
    ip->ip_sum = (u_short)(LIBNET_CKSUM_CARRY(sum));
#endif
	if(((UINT)out_len > pktb->buff_len) || ((UINT)(buf + out_len) > (UINT)pktb->end)) {
		DbgPrint("out_len:%d,pktb->buff_len:%d, (buf + out_len):%x,pktb->end:%x\n",
				out_len, pktb->buff_len, (UINT)(buf + out_len), (u_int)pktb->end);
		return -1;
	}
	if(pktb->head > buf) {
		DBGPRINT("pktb->head:%x,buf:%x\n", (u_int)pktb->head, (u_int)buf);
		return -1;
	}

    pktb->len = out_len;
    pktb->data = buf;
    pktb->tail = pktb->data+pktb->len;
  }
  return( ret );
}

/*
  ipsec_input()

            from driver-core
*/
int
ipsec_input( void* Adapter, struct pkt_buff *pktb )
{
  PIPSEC pIpsec=&((pADAPTER)Adapter)->Ipsec_SL.Ipsec;
  struct mbuf *m=0;
  struct ip_packet *ip;
  struct udp_packet *udp;
  u_char *buf;

  /*printk(KERN_ERR "-----------------------ipsec_input-----------------------------\n");*/
  DBG_ENTER(ipsec_input);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_input \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 


  if( !pIpsec->init ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec_core Don't initialize\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ipsec_core Don't initialize\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

    return(-1);
  }

  /* IPsec */
  ip = (struct ip_packet *)pktb->data;
  udp = (struct udp_packet *)&ip->udp;
  if((IPPROTO_UDP == ip->head.ip_p )&&
     /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
     ( udp->head.source == htons(PORT_ISAKMP))) {
#else
     ( udp->head.uh_sport == htons(PORT_ISAKMP))) {
#endif

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "recv IKE packet\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "recv IKE packet\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

    return(-1);
  }

/* printk(KERN_ERR "-----------------------before in_mbuf_set start-----------------------------\n"); */
/* hex_dump(pktb->data,pktb->len); */
/* printk(KERN_ERR "-----------------------before in_mbuf_set end-----------------------------\n"); */
  m = in_mbuf_set( pIpsec, pktb->data, pktb->len ); /* mbuf set */
  if( !m )
    return(-1);

/* printk(KERN_ERR "-----------------------before esp_input start disp m----------------------------\n"); */
/* hex_dump(m->m_data,m->m_len); */
/* printk(KERN_ERR "-----------------------before esp_input end disp m------------------------------\n"); */

  if( IPPROTO_AH == ip->head.ip_p ) {
    if( ah_input( pIpsec, m ) ) { /* AH */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "ah_in err\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "ah_in err\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

      mfree_m(m);
      return(-1);
    }
  }
  else if( IPPROTO_ESP == ip->head.ip_p ) {
    if( esp_input( pIpsec, m ) ) { /* ESP */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "esp_in err\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "esp_in err\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
      mfree_m(m);
      return(-1);
    }
  }
  else if( pIpsec->ip4_def_policy.policy == IPSEC_POLICY_IPSEC ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "none esp,ah\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "none esp,ah\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    mfree_m(m);
    return(-1);
  }

  if( pIpsec->ip4_def_policy.policy == IPSEC_POLICY_IPSEC ) {
    buf = mtod( m, u_char *);
    pktb->len = m->m_total; 
    pktb->data = buf;
    pktb->tail = pktb->data+pktb->len;
/* printk(KERN_ERR "-----------------------ipsec_input in case IPSEC_POLICY_IPSEC start-----------------------------\n"); */
/* hex_dump(pktb->data,pktb->len); */
    mfree_m(m);
/* printk(KERN_ERR "-----------------------ipsec_input in case IPSEC_POLICY_IPSEC end-----------------------------\n"); */
    return(1);
  }
  mfree_m(m);

  return(0);
}


void ipsec_Timeup0(unsigned long FunctionContext)
{
  PIPSEC pIpsec = (PIPSEC)FunctionContext;
  struct secasvar *sav;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT("Key0 OUT Time up\n");
#else /* BUILD_ANDROID */
  printk("Key0 OUT Time up\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  pIpsec->life_timer[FIRST_OUT] = 0;
  sav = pIpsec->ip4_def_policy.req->sav;

  sav->spi = 0;        /* Out SPI */
}

void ipsec_Timeup1(unsigned long FunctionContext)
{
  PIPSEC pIpsec = (PIPSEC)FunctionContext;
  struct secasvar *sav;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "Key1 OUT Time up\n");
#else /* BUILD_ANDROID */
  printk(KERN_ERR "Key1 OUT Time up\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  pIpsec->life_timer[SECOND_OUT] = 0;
  sav = pIpsec->ip4_def_policy.req->sav->sa_next->sa_next;

  sav->spi = 0;        /* Out SPI */
}

void ipsec_Timeup0_in(unsigned long FunctionContext)
{
  PIPSEC pIpsec = (PIPSEC)FunctionContext;
  struct secasvar *sav;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "Key0 IN Time up\n");
#else /* BUILD_ANDROID */
  printk(KERN_ERR "Key0 IN Time up\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  pIpsec->life_timer[FIRST_IN] = 0;
  sav = pIpsec->ip4_def_policy.req->sav;

  sav->sa_next->spi = 0;  /* In  SPI */
}

void ipsec_Timeup1_in(unsigned long FunctionContext)
{
  PIPSEC pIpsec = (PIPSEC)FunctionContext;
  struct secasvar *sav;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "Key1 IN Time up\n");
#else /* BUILD_ANDROID */
  printk(KERN_ERR "Key1 IN Time up\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  pIpsec->life_timer[SECOND_IN] = 0;
  sav = pIpsec->ip4_def_policy.req->sav->sa_next->sa_next;

  sav->sa_next->spi = 0;  /* In  SPI */
}

/*
 * Chop IP header and option off from the payload.
 */
static struct mbuf *
ipsec4_splithdr(PIPSEC pIpsec,
		struct mbuf *m)
{
  struct mbuf *mh;
  struct ip *ip;
  int hlen;

  DBG_ENTER(ipsec4_splithdr);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec4_splithdr \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 


  ip = mtod(m, struct ip *);
  hlen = ip->ip_hl << 2;
  if( m->m_len > hlen ) {
    mh = mget(pIpsec);
    if (!mh) {
      mfree(m);
      return NULL;
    }
    m->m_len -= hlen;
    m->m_data += hlen;
    m->offset += hlen;
    mh->m_next = m;
    m = mh;
    m->m_len = hlen;
    m->m_total = hlen + m->m_next->m_len;
    memcpy(m->m_data, (u_char *)ip, hlen);
  } 
  else if (m->m_len < hlen) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "mbuf pullup error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "mbuf pullup error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

    return NULL;
  }
  return m;
}



struct mbuf *
out_state_set( pIpsec, data, len, state )
PIPSEC pIpsec;
u_char *data;
int    len;
struct ipsec_output_state *state;
{
	struct ip *ip;
	struct mbuf *bm_buff=NULL;
	int i;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call out_state_set \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

	ip = (struct ip *)data;
	memcpy( &state->dst, &ip->ip_dst.s_addr, sizeof(struct in_addr));

	spin_lock(&GlobalLock);
	for(i=0; i<BM_SEND_BUFF; i++) {
		if( pIpsec->bm_buff[i].use == 0 ) {
			/* buffer -> mbuf */
			pIpsec->bm_buff[i].use = 1;
			pIpsec->bm_buff[i].flg = 1;
			pIpsec->bm_buff[i].m_data = data;
			pIpsec->bm_buff[i].m_len = len;
			pIpsec->bm_buff[i].offset = 100; 
			pIpsec->bm_buff[i].m_next = 0;
			pIpsec->bm_buff[i].m_total = len;

			bm_buff = &pIpsec->bm_buff[i];
			break;
		}
	}
	spin_unlock(&GlobalLock);

	return bm_buff;
} 


struct mbuf *
in_mbuf_set( pIpsec, data, len )
PIPSEC pIpsec;
u_char *data;
int    len;
{
	int	i;
	struct mbuf	*ret=NULL;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call in_mbuf_set \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

	spin_lock(&GlobalLock);

	for(i=0; i<BM_RECV_BUFF; i++) {
		if( pIpsec->bm_buff2[i].use == 0 ) {
			/* buffer -> mbuf */
			pIpsec->bm_buff2[i].use = 1;
			pIpsec->bm_buff2[i].flg = 1;
			pIpsec->bm_buff2[i].m_data = data;
			pIpsec->bm_buff2[i].m_len = len;
			pIpsec->bm_buff2[i].offset = 0; 
			pIpsec->bm_buff2[i].m_next = 0;
			pIpsec->bm_buff2[i].m_total = len;

			ret = &pIpsec->bm_buff2[i];
			break;
		}
	}
	spin_unlock(&GlobalLock);

	return ret;
}



/*
 * IPsec output logic for IPv4.

  retuen  0: no ipsec
          1: ipsec
         -1: discard
*/
int
ipsec4_output( pIpsec, state )
PIPSEC pIpsec; 
struct ipsec_output_state *state;
{
  struct secpolicy *sp;
  struct ip *ip = NULL;
  struct ipsecrequest *isr = NULL;
  struct ipsecstat *stat=&pIpsec->ipsec_stat;
  struct secasvar *sav;
  int error=0;

  
  /*printk(KERN_ERR "ipsec4_output\n");*/

  /* get SP for this packet */
  sp = &pIpsec->ip4_def_policy;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
  TNC_LOGOUT("ipsec4_output sp->policy:%d  \n",sp->policy);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  /* check policy */
  switch (sp->policy) {
  case IPSEC_POLICY_DISCARD:
    /*
     * This packet is just discarded.
     */
    stat->out_polvio++;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "IPSEC: output discard.\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "IPSEC: output discard.\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    mfree_m( state->m );
    return(-1);
  case IPSEC_POLICY_BYPASS:
  case IPSEC_POLICY_NONE:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("IPSEC: No IPsec mode.\n");
#else /* BUILD_ANDROID */
    printk("IPSEC: No IPsec mode.\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    mfree_m( state->m );
    return(0);
  case IPSEC_POLICY_IPSEC:
    if (sp->req == NULL) {
      /* XXX should be panic ? */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "IPSEC: No IPsec request specified.\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "IPSEC: No IPsec request specified.\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
      mfree_m( state->m );
      return(-1);
    }
    break;
  case IPSEC_POLICY_ENTRUST:
  default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "IPSEC: Invalid policy found. %d\n", sp->policy);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "IPSEC: Invalid policy found. %d\n", sp->policy);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    mfree_m( state->m );
    return(-1);
  }
  /* make SA index for search proper SA */
  ip = mtod(state->m, struct ip *);

  if( pIpsec->now_key_no == 0 ){
    isr = sp->req;
  }
  else{
    isr = sp->req->next->next;
  }
  sav = isr->sav;

  /* 鍵のチェック */
  if( sav->spi == 0 ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "A key isn't configured.\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "A key isn't configured.\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    mfree_m( state->m );
    return(-1);
  }

  if ( (isr->saidx.mode == IPSEC_MODE_TUNNEL) ||
			(isr->saidx.mode == IPSEC_MODE_UDP_TUNNEL) ) {
    /*
     * build IPsec tunnel.
     */
    state->m = ipsec4_splithdr(pIpsec, state->m);
    if (!state->m) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "IPSEC: ipsec4_splithdr error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "IPSEC: ipsec4_splithdr error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
      return(-1);
    }
    error = ipsec4_encapsulate( pIpsec, state->m );
    if( error ) {
      mfree_m( state->m );
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "IPSEC: ipsec4_encapsulate error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "IPSEC: ipsec4_encapsulate error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
      return(-1);
    }
    ip = mtod(state->m, struct ip *);
  }
  state->m = ipsec4_splithdr(pIpsec, state->m);
  if (!state->m) {
	return(-1);
  }

  switch (isr->saidx.proto) {
  case IPPROTO_ESP:
    ip = mtod(state->m, struct ip *);
    if ((error = esp_output(pIpsec, state->m, &ip->ip_p, isr)) != 0) {
      mfree_m(state->m);
      return( error );
    }

    {
      int sum;
      int hlen;

      if ( (isr->saidx.mode == IPSEC_MODE_TUNNEL) ||
			(isr->saidx.mode == IPSEC_MODE_UDP_TUNNEL) ) {
		ip->ip_dst.s_addr = isr->saidx.dst.s_addr;
		ip->ip_src.s_addr = isr->saidx.src.s_addr;
      }
      ip->ip_sum = 0;
      hlen = ip->ip_hl << 2;
      /* make ip checksum */
      sum = libnet_in_cksum((u_short *)ip, hlen);
      ip->ip_sum = (u_short)(LIBNET_CKSUM_CARRY(sum));
    }
    break;
  case IPPROTO_AH:
    if ((error = ah_output(pIpsec, state->m, isr)) != 0) {
      mfree_m(state->m);
      state->m = NULL;
      return(-1);
    }
    break;
  default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec4_output: unknown ipsec protocol %d\n",isr->saidx.proto);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ipsec4_output: unknown ipsec protocol %d\n",isr->saidx.proto);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
    mfree_m(state->m);
    state->m = NULL;
    return(-1);
  }
  if (state->m == 0) {
    return(-1);
  }
  return(1);
}

int
ipsec4_encapsulate( pIpsec, m )
PIPSEC pIpsec;
struct mbuf *m;
{
  struct ip *oip;
  struct ip *ip;
  size_t hlen;
  size_t plen;
  int sum;

  DBG_ENTER(ipsec4_encapsulate);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec4_encapsulate \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  ip = mtod(m, struct ip *);
  hlen = ip->ip_hl << 2;

  if ((size_t)m->m_len != hlen)
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
     TNC_LOGOUT(KERN_ERR "IPSEC: ipsec4_encapsulate: assumption failed (first mbuf length)");
#else /* BUILD_ANDROID */
     printk(KERN_ERR "IPSEC: ipsec4_encapsulate: assumption failed (first mbuf length)");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  /* generate header checksum ? */
  ip->ip_sum = 0;


  sum = libnet_in_cksum((u_short *)ip, hlen);
  ip->ip_sum = (u_short)(LIBNET_CKSUM_CARRY(sum));
  plen = m->m_total;
  /*
   * grow the mbuf to accomodate the new IPv4 header.
   * NOTE: IPv4 options will never be copied.
   */

  if( m->m_next->offset < (int)hlen )
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "IPSEC: mbuf offset error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "IPSEC: mbuf offset error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  m->m_next->m_len += hlen;
  m->m_next->m_data -= hlen;
  m->m_next->offset -= hlen;

  m->m_total += sizeof(struct ip);
  m->m_len = sizeof(struct ip);
  oip = mtod(m->m_next, struct ip *);
  /* ?? */
  memcpy( (caddr_t)oip, (caddr_t)ip , hlen ); 

  /* construct new IPv4 header. see RFC 2401 5.1.2.1 */
  /* ECN consideration. */
  ip_ecn_ingress(pIpsec->ip4_ipsec_ecn, &ip->ip_tos, &oip->ip_tos);
  ip->ip_hl = sizeof(struct ip) >> 2;
  ip->ip_off &= htons(~IP_OFFMASK);
  ip->ip_off &= htons(~IP_MF);
  switch( pIpsec->ip4_ipsec_dfbit ) {
  case 0:	/*clear DF bit*/
    ip->ip_off &= htons(~IP_DF);
    break;
  case 1:	/*set DF bit*/
    ip->ip_off |= htons(IP_DF);
    break;
  default:	/*copy DF bit*/
    break;
  }

  ip->ip_p = IPPROTO_IPIP;
  if((plen + sizeof(struct ip)) < IP_MAXPACKET)
    ip->ip_len = htons((plen + sizeof(struct ip)));
  else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("IPSEC: ipsec: size exceeds limit: leave ip_len as is (invalid packet)\n");
#else /* BUILD_ANDROID */
    printk("IPSEC: ipsec: size exceeds limit: leave ip_len as is (invalid packet)\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  }
  ip->ip_id = htons(pIpsec->ip_id++);

  return 0;
}

/*
 * Check the variable replay window.
 * ipsec_chkreplay() performs replay check before ICV verification.
 * ipsec_updatereplay() updates replay bitmap.  This must be called after
 * ICV verification (it also performs replay check, which is usually done
 * beforehand).
 * 0 (zero) is returned if packet disallowed, 1 if packet permitted.
 *
 * based on RFC 2401.
 */
int
ipsec_chkreplay(u_int32_t seq,
		struct secasvar *sav)
{
  struct secreplay *replay;
  u_int32_t diff;
  int fr;
  u_int32_t wsizeb;	/* constant: bits of window size */
  int frlast;		/* constant: last frame */

  replay = sav->replay;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_chkreplay \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  if (replay->wsize == 0)
    return 1;	/* no need to check replay. */

  /* constant */
  frlast = replay->wsize - 1;
  wsizeb = replay->wsize << 3;

  /* sequence number of 0 is invalid */
  if (seq == 0)
    return 0;

  /* first time is always okay */
  if (replay->count == 0)
    return 1;

  if (seq > replay->lastseq) {
    /* larger sequences are okay */
    return 1;
  }
  else {
    /* seq is equal or less than lastseq. */
    diff = replay->lastseq - seq;

    /* over range to check, i.e. too old or wrapped */
    if (diff >= wsizeb)
      return 0;

    fr = frlast - diff / 8;
    /* this packet already seen ? */
    if ((replay->bitmap)[fr] & (1 << (diff % 8)))
      return 0;

    /* out of order but good */
    return 1;
  }
}

/*
 * check replay counter whether to update or not.
 * OUT:	0:	OK
 *	1:	NG
 */
int
ipsec_updatereplay(u_int32_t seq,
		   struct secasvar *sav)
{
#ifdef notdef
  struct secreplay *replay;
  u_int32_t diff;
  int fr;
  u_int32_t wsizeb;	/* constant: bits of window size */
  int frlast;		/* constant: last frame */

  replay = sav->replay;

  if (replay->wsize == 0)
    goto ok;	/* no need to check replay. */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_updatereplay \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 

  /* constant */
  frlast = replay->wsize - 1;
  wsizeb = replay->wsize << 3;

  /* sequence number of 0 is invalid */
  if (seq == 0)
    return 1;

  /* first time */
  if (replay->count == 0) {
    replay->lastseq = seq;
    bzero(replay->bitmap, replay->wsize);
    (replay->bitmap)[frlast] = 1;
    goto ok;
  }

  if (seq > replay->lastseq) {
    /* seq is larger than lastseq. */
    diff = seq - replay->lastseq;

    /* new larger sequence number */
    if (diff < wsizeb) {
      /* In window */
      /* set bit for this packet */
      vshiftl(replay->bitmap, diff, replay->wsize);
      (replay->bitmap)[frlast] |= 1;
    }
    else {
      /* this packet has a "way larger" */
      bzero(replay->bitmap, replay->wsize);
      (replay->bitmap)[frlast] = 1;
    }
    replay->lastseq = seq;

    /* larger is good */
  }
  else {
    /* seq is equal or less than lastseq. */
    diff = replay->lastseq - seq;

    /* over range to check, i.e. too old or wrapped */
    if (diff >= wsizeb)
      return 1;

    fr = frlast - diff / 8;

    /* this packet already seen ? */
    if ((replay->bitmap)[fr] & (1 << (diff % 8)))
      return 1;

    /* mark as seen */
    (replay->bitmap)[fr] |= (1 << (diff % 8));

    /* out of order but good */
  }

ok:
  if (replay->count == ~0) {

    /* set overflow flag */
    replay->overflow++;

    /* don't increment, no more packets accepted */
    if ((sav->flags & SADB_X_EXT_CYCSEQ) == 0)
      return 1;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "AH: replay counter made %d cycle. \n", replay->overflow);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "AH: replay counter made %d cycle. \n", replay->overflow);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  }
  replay->count++;

#endif
  return 0;
}

/* validate inbound IPsec tunnel packet. */
int
ipsec4_tunnel_validate(ip, nxt0, sav)
struct ip *ip;
u_int nxt0;
struct secasvar *sav;
{
  u_int8_t nxt = nxt0 & 0xff;
  int hlen;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec4_tunnel_validate \n");
#else /* BUILD_ANDROID */

#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  if (nxt != IPPROTO_IPV4)
    return 0;
  hlen = ip->ip_hl << 2;
  if (hlen != sizeof(struct ip))
    return 0;

  return 1;
}

/*
 * modify outer ECN (TOS) field on ingress operation (tunnel encapsulation).
 * call it after you've done the default initialization/copy for the outer.
 */
void
ip_ecn_ingress(mode, outer, inner)
int mode;
u_int8_t *outer;
u_int8_t *inner;
{

  DBG_ENTER(ip_ecn_ingress);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ip_ecn_ingress \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  if (!outer || !inner)
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "NULL pointer passed to ip_ecn_ingress");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "NULL pointer passed to ip_ecn_ingress");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
  
  switch (mode) {
  case ECN_ALLOWED:		/* ECN allowed */
    *outer &= ~IPTOS_CE;
    break;
  case ECN_FORBIDDEN:		/* ECN forbidden */
    *outer &= ~(IPTOS_ECT | IPTOS_CE);
    break;
  case ECN_NOCARE:	/* no consideration to ECN */
    break;
  }
}


struct secasvar *
get_sav( pIpsec, src, dst, proto, spi )
PIPSEC pIpsec;
u_long src, dst;
u_char proto;
int spi;
{
	struct secpolicy *sp=&pIpsec->ip4_def_policy;
	struct ipsecstat *ipsecstat=&pIpsec->ipsec_stat;
	struct ipsecrequest *isr;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_sav \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 


	/* get SP for this packet */
	if( !sp ) {
		ipsecstat->in_inval++;
		return( 0 );
	}
	if( !sp->req ) {
		ipsecstat->in_inval++;
		return( 0 );
	}

	isr = &pIpsec->def_isr[0];
	do{
	  /* Tunnelモードの場合は、Tunnelのpeerからの受信以外は受け付けない */
	  if ( (isr->saidx.mode == IPSEC_MODE_TUNNEL) ||
	       (isr->saidx.mode == IPSEC_MODE_UDP_TUNNEL) ) {
	    if( isr->saidx.src.s_addr == src && isr->saidx.dst.s_addr == dst){ 
	      /* ESP/AHのチェック */
	      if( isr->saidx.proto != proto ) {
		ipsecstat->in_inval++;
		return( 0 );
	      }
	      
	      /* SAの検索を行う */
	      if(isr->sav == NULL) {		/* 送信用SAは存在する? */
		ipsecstat->in_inval++;
		return( 0 );
	      }
	      
	      if( isr->sav->spi == (u_int32_t)spi )
		return( isr->sav );
	    }
	  }
	  isr = isr->next;
	}while(isr);

	if(!isr){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
	  TNC_LOGOUT(KERN_ERR "IN: Unknown host: 0x%0x\n",(u_int)src);
	  TNC_LOGOUT(KERN_ERR "IN: Bat local host: 0x%x\n",(u_int)dst);
#else /* BUILD_ANDROID */
	  printk(KERN_ERR "IN: Unknown host: 0x%0x\n",(u_int)src);
	  printk(KERN_ERR "IN: Bat local host: 0x%x\n",(u_int)dst);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
	}

	return NULL;
}

u_char *
restore_mbuf( m0, len )
struct mbuf *m0;
int *len;
{
  struct mbuf *m;
  u_char *buf;
  int hlen;

  hlen = m0->m_len;
  if( m0->m_next ) {
    m = m0->m_next; 
    if( hlen + m->m_len != m0->m_total ) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifdef BUILD_ANDROID
      TNC_LOGOUT("restore_mbuf Error (%d+%d:%d)\n",
#else /* BUILD_ANDROID */
      printk("restore_mbuf Error (%d+%d:%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
		 hlen, m->m_len, m0->m_total);
    }
    buf = mtod( m, u_char *);
    buf -= hlen;
    memcpy( buf, mtod( m0, u_char *), hlen);
  }
  else
    buf = mtod( m0, u_char *);
  
  *len = m0->m_total;
  mfree_m(m0);

  return( buf );
}

void
ipsec_stat_print( PIPSEC pIpsec )
{
/* PMC-Viana-029 Quad組み込み対応 */
//  struct ipsecstat *ipsecstat=&pIpsec->ipsec_stat;

  if( pIpsec->ip4_def_policy.policy == IPSEC_POLICY_NONE )
    return;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */ 
#ifndef BUILD_ANDROID /* BUILD_ANDROID */
  printk(KERN_ERR "== IPSEC Status==\n");
  if( ipsecstat->in_success )
    printk(KERN_ERR " in success    :%d\n",ipsecstat->in_success);
  if( ipsecstat->in_inval )
    printk(KERN_ERR " in inval      :%d\n",ipsecstat->in_inval);
  if( ipsecstat->in_badspi )
    printk(KERN_ERR " in badspi     :%d\n",ipsecstat->in_badspi);
  if( ipsecstat->in_ahauthsucc )
    printk(KERN_ERR " in ahauthsucc :%d\n",ipsecstat->in_ahauthsucc);
  if( ipsecstat->in_ahauthfail )
    printk(KERN_ERR " in ahauthfail :%d\n",ipsecstat->in_ahauthfail);
  if( ipsecstat->in_espauthsucc ) 
    printk(KERN_ERR " in espauthsucc:%d\n",ipsecstat->in_espauthsucc);
  if( ipsecstat->in_espauthfail ) 
    printk(KERN_ERR " in espauthfail:%d\n",ipsecstat->in_espauthfail);
  if( ipsecstat->out_success ) 
    printk(KERN_ERR "out success    :%d\n",ipsecstat->out_success);
  if( ipsecstat->out_polvio )
    printk(KERN_ERR "out polvio     :%d\n",ipsecstat->out_polvio);
  if( ipsecstat->out_inval )
    printk(KERN_ERR "out inval      :%d\n",ipsecstat->out_inval);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */ 
}

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
EXPORT_SYMBOL(ipsec_output);
#endif
