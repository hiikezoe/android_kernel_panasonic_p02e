/*
 * NAT-Traversal��UDP��������
 *
 */
#include "linux_precomp.h"

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
#include <net/protocol.h>
#endif

extern PADAPT pAdapt;

#ifdef TNC_KERNEL_2_6
extern struct net_protocol udp_esp_proto;
#else 
extern struct inet_protocol udp_esp_proto;
#endif

u_short calc_udp_sum(struct ip_udp *ip_udp);

#ifdef SET_MARKER
char null_data[MARKER_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
#endif

/* NAT-Traversal�����å�
 *
 * ��   : int check_Nat_T(short dport, char *udp_pl)
 * ����   : short dport : UDP�إå���dst port
 *        : char *udp_pl : UDP�ڥ�����
 * ����� : ���� : IPSEC_SUCCESS
 *          ���� : IPSEC_ERROR
 */
int check_Nat_T(short dport, char *udp_pl)
{
  int ret = IPSEC_SUCCESS;
  
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call check_Nat_T \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  spin_lock(&pAdapt->NatInfo_SL.NatInfoLock);

  if(pAdapt->NatInfo_SL.nat_info.enable == NAT_TRAV_ENABLE &&
     ntohs(dport) == pAdapt->NatInfo_SL.nat_info.own_port 
#ifdef SET_MARKER
     && !memcmp(udp_pl, null_data, MARKER_LEN)
#endif
     ){
    ret = IPSEC_SUCCESS;
  }
  else{
    ret = IPSEC_ERROR;
  }
  spin_unlock(&pAdapt->NatInfo_SL.NatInfoLock);

  return ret;
}

/* NAT-Traversal�Ѽ�������
 * 
 * ��   : int udp_esp_rcv(struct sk_buff *skbf)
 * ����   : strcut sk_buff *skbf : �ѥ��åȾ���
 * ����� : ���� : 0
 *          ���� : -1
 */
int udp_esp_rcv(struct sk_buff *skb)
{
  struct iphdr  *iph;
  struct udphdr *udph;
  char *udp_pl;
  u_short sum;
  struct sk_buff *skbf = NULL;
  int nat_trav_headlen; /* header length of NAT Tarversal */
  int rtn;

 /* printk(KERN_ERR "----- UDP message rcv!! -----  \n"); */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call udp_esp_rcv \n");
  TNC_LOGOUT("----- UDP message rcv!! -----  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

#ifdef SET_MARKER
    nat_trav_headlen = sizeof(struct udphdr) + MARKER_LEN;
#else
    nat_trav_headlen = sizeof(struct udphdr);
#endif

  /* check ip fragment(UDP header) */
  if(!pskb_may_pull(skb,nat_trav_headlen)){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "pskb_may_pull error in udp_esp_rcv\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "pskb_may_pull error in udp_esp_rcv\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    kfree_skb(skb);
    return 0;
  }

  udph = (struct udphdr*)skb->data;
  udp_pl = (char*)(udph + 1);

  /* Nat traversal check */
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  rtn = check_Nat_T(udph->dest,udp_pl);
#else 
  rtn = check_Nat_T(udph->uh_dport,udp_pl);
#endif
  if(rtn == IPSEC_SUCCESS){
    /* Nat Traversal packet */
    /* check ip fragment */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("IPSEC_SUCCESS \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    if(!pskb_may_pull(skb,skb->len)){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "pskb_may_pull error in udp_esp_rcv\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "pskb_may_pull error in udp_esp_rcv\n");

#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
      kfree_skb(skb);
      return 0;
    }
    /* check skb clone */
    if(skb_cloned(skb)){
      DBG_printf("skb cloned\n");
      skbf = skb_copy(skb, GFP_ATOMIC);
      if(skbf == NULL){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : skb_copy in udp_esp_rcv\n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "error : skb_copy in udp_esp_rcv\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	kfree_skb(skb);
	return 0;
      }
      else{
	udph = (struct udphdr*)skbf->data;
	pAdapt->drv_stat.rcv_skb_copy_count++;
	kfree_skb(skb);
      }
    }
    else{
      skbf = skb;
    }

    /* check UDPheader */
    switch(pAdapt->drv_info.udp_check_mode){
    case NO_CHECK:
      DBG_printf("udp checksum no check\n");
      break;
    case NULL_OK:
    /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
      if(udph->check){
#else
      if(udph->uh_sum){
#endif
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
    if(calc_udp_sum((struct ip_udp*)skbf->network_header))   {
#else   /* BUILD_ANDROID */
	if(calc_udp_sum((struct ip_udp*)skbf->nh.iph)){
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
	  TNC_LOGOUT(KERN_ERR "UDP checksum error \n");
#else /* BUILD_ANDROID */
	  printk(KERN_ERR "UDP checksum error \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	  pAdapt->drv_stat.udp_checksum_error_count++;
	  goto error;
	}
      }
      break;
    case NULL_NG:
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
     if(calc_udp_sum((struct ip_udp*)skbf->network_header)){
#else   /* BUILD_ANDROID */
      if(calc_udp_sum((struct ip_udp*)skbf->nh.iph)){
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "UDP checksum error \n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "UDP checksum error \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	pAdapt->drv_stat.udp_checksum_error_count++;
	goto error;
      }
      break;
    default:
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "UDP check mode error (%d)\n",
#else /* BUILD_ANDROID */
      printk(KERN_ERR "UDP check mode error (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	     pAdapt->drv_info.udp_check_mode);
      goto error;
    }

    /* remove UDPheader and non-IKE marker */
    skbf->tail -= nat_trav_headlen;
    skbf->len -= nat_trav_headlen;
    memmove(skbf->data, (skbf->data + nat_trav_headlen), skbf->len);
    

    /* add New IPheader */
    iph = (struct iphdr*)skb_push(skbf, sizeof(struct iphdr));
    if(iph == NULL){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "skb_push error in udp_esp_rcv\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "skb_push error in udp_esp_rcv\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
      goto error;
    }
   
    /* remake ip header */
    iph->tot_len = htons(skbf->len);
    iph->protocol = IPPROTO_ESP;
    iph->check = 0;
    sum = in_cksum((u_short*)iph,iph->ihl<<2);
    iph->check = htons(sum);

/* PMC-Viana-007-CONFIG_NETFILTER����б� add start */    
#ifndef BUILD_ANDROID
#ifdef CONFIG_NETFILTER
    nf_conntrack_put(skbf->nfct);
    skbf->nfct = NULL;
#ifdef CONFIG_NETFILTER_DEBUG
    skbf->nf_debug = 0;
#endif
#endif
#endif  /* BUILD_ANDROID */
/* PMC-Viana-007-CONFIG_NETFILTER����б� add end */

    /* �ѥ��åȤ�ƹ��ۤ����Τǥ����å�����׻� */
    skbf->ip_summed = CHECKSUM_NONE; 

  rtn =  netif_rx(skbf);
  if(rtn != 0)
{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
   TNC_LOGOUT("netif_rx NAT_Travarsal:  %d\n",rtn);
#else /* BUILD_ANDROID */
  /* printk("netif_rx NAT_Travarsal:  %d\n",rtn); */
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
}

  }
  else {
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
   TNC_LOGOUT("not netif_rx NAT_Travarsal:  %d pAdapt->udp_prot:%x\n",rtn,pAdapt->udp_prot);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    /* non Nat Traversal packet */
    if(pAdapt->udp_prot){
      if(pAdapt->udp_prot->handler)
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
		{
        TNC_LOGOUT("pAdapt->udp_prot->handler(skb)");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	pAdapt->udp_prot->handler(skb);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
        }
#endif /* BUILD_ANDROID */
    }
    else{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "error : udp protocol nothing!\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "error : udp protocol nothing!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
      kfree_skb(skb);
    }
  }  

  return 0;

 error:
  kfree_skb(skbf);
  return 0;
}


/* UDP�����å�����׻� 
 *
 * ��   : u_short calc_udp_sum(struct ip_udp *ip_udp)   
 * ����   : struct ip_udp *ip_udp : ip/udp����
 * ����� : �ۥ��ȥХ��ȥ��������Υ����å�����
 */
u_short calc_udp_sum(struct ip_udp *ip_udp)   
{
  struct iphdr  *ip = (struct iphdr *)&ip_udp->iph;
  struct udphdr *udp=(struct udphdr *)&ip_udp->udph;
  u_long sum=0;
  u_short *sdatap;
  int udp_len;
  int i;

/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call calc_udp_sum \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  udp_len = ntohs(udp->len);
#else 
  udp_len = ntohs(udp->uh_ulen);
#endif
  sdatap = (u_short *)&ip->saddr;
  for(i=0; i<4; i++) {
    sum += ntohs(sdatap[i]); /* KW-ABR */
  }
  sum += ip->protocol + udp_len;
  sdatap = (u_short *)udp;

  for(i=0; i<udp_len/2; i++) {
    sum += ntohs(sdatap[i]);
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  sum = (short)(~sum & 0xffff);
  return((u_short)sum);

}


/* 
 *
 * ��   : int udp_esp_rcv_err(struct sk_buff *skb)
 * ����   : strcut sk_buff *skbf : �ѥ��åȾ���
 * ����� : �ʤ�
 *          
 */
void udp_esp_rcv_err(struct sk_buff *skb, u32 info)
{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("call org udp error handler\n");
#else /* BUILD_ANDROID */
  printk("call org udp error handler\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  /* ���ꥸ�ʥ��UDP error �ϥ�ɥ�ƤӽФ� */
  pAdapt->udp_prot->err_handler(skb,info);

}
