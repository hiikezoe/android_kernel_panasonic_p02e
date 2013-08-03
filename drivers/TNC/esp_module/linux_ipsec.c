/*
 * Linux版IPSec初期化＆ESP受信処理
 *
 */
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/protocol.h>
#include <linux/netdevice.h>
/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
#else
#include <linux/brlock.h>
#endif
#include "linux_precomp.h"
#include "pkt_buff.h"
#include "esp.h"

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
DEFINE_SPINLOCK(GlobalLock);
#else
spinlock_t GlobalLock;
#endif

PADAPT pAdapt;
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
int ipsec_debug = 1;
#else
int ipsec_debug = 0;
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

int ipsec_recv(struct sk_buff*);
int ipsec_input(PADAPT,struct pkt_buff*);
int udp_esp_rcv(struct sk_buff *skbf);
void udp_esp_rcv_err(struct sk_buff *skbf,u32 info);

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
int ipsec_inet_add_protocol(struct net_protocol *prot, unsigned char protocol);
/* カーネルのスピンロック変数 */
#ifdef BUILD_ANDROID
extern spinlock_t inet_proto_lock;
#else
DEFINE_SPINLOCK(inet_proto_lock);
#endif
extern void synchronize_net(void);
#else 
int ipsec_inet_add_protocol(struct inet_protocol *prot);
#endif

void SetPktBFromSkbuff(struct pkt_buff *pktb, struct sk_buff *skbf);
void SetSkbFromPktbuff(struct pkt_buff *pktb, struct sk_buff *skbf);
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
static void dump_skb(struct sk_buff *skb);   // tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6

/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
/* PMC-Viana-029 Quad組み込み対応 */
//	void test1()
	void test1(void)
	{
		TNC_LOGOUT("Call test1 \n");
		
	}
/* PMC-Viana-029 Quad組み込み対応 */
//	void test2()
	void test2(void)
	{
		TNC_LOGOUT("Call test2 \n");
		
	}
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

/* ESP プロトコル用構造体 */

/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
static const struct net_protocol esp_proto = {
#else
struct net_protocol esp_proto =  {
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  .handler = ipsec_recv,
  .err_handler = NULL,
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  .no_policy = 1,
/* PMC-Viana-029 Quad組み込み対応 */
//	.gso_send_check = test1,
//	.gso_segment = test2,
	.netns_ok =     1,	
#else
  .no_policy = 1
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

};

/* NAT-T用UDPプロトコル構造体 */
struct net_protocol udp_esp_proto = {
  .handler = udp_esp_rcv,
  .err_handler = udp_esp_rcv_err,
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  .no_policy = 1,
/* PMC-Viana-029 Quad組み込み対応 */
//	.gso_send_check = test1,
//	.gso_segment = test2,
	.netns_ok =     1,
#else
  .no_policy = 1
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
};

#else 

/* ESP プロトコル用構造体 */
struct inet_protocol esp_proto =  {
  ipsec_recv,
  NULL,
  0,
  IPPROTO_ESP,
  0,
  NULL,
  "ESP"
};

/* NAT-T用UDPプロトコル構造体 */
struct inet_protocol udp_esp_proto = {
  udp_esp_rcv,
  udp_esp_rcv_err,
  0,
  IPPROTO_UDP,
  0,
  NULL,
  "UDP_ESP"
};

#endif


/* IPSec 初期化
 *
 * 書式   : int linux_ipsec_init(void)
 * 引数   : なし
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int linux_ipsec_init(void)
{
  int rtn;

  DBG_printf("IPSEC init\n");
    
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID 
    TNC_LOGOUT("Call linux_ipsec_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 end start */
  /* 共有リソース初期化 */ 
  pAdapt = kmalloc(sizeof(ADAPT),GFP_KERNEL);
  if(pAdapt == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID 
    TNC_LOGOUT(KERN_ERR "ADAPT kmalloc error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ADAPT kmalloc error\n");
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return -ENOMEM;
  }
  memset(pAdapt, 0, sizeof(ADAPT));
  spin_lock_init(&GlobalLock);
  spin_lock_init(&pAdapt->Ipsec_SL.IpsecLock);
  spin_lock_init(&pAdapt->NatInfo_SL.NatInfoLock);
  spin_lock_init(&pAdapt->DevName_SL.DevNameLock);
  pAdapt->timer_info.timeout_info.timeout_cycle = DEFAULT_TIMEOUT_CYCLE;
  pAdapt->timer_info.timeout_info.timeout_count = DEFAULT_TIMEOUT_COUNT;
  pAdapt->drv_info.udp_check_mode = NULL_OK;
  pAdapt->drv_info.sending_count = 0;
  pAdapt->drv_info.max_sending_num = MAX_SENT_NUM;
  init_timer(&pAdapt->timer_info.dev_checkTimer);
  init_timer(&pAdapt->timer_info.link_check_timer);
  pAdapt->timer_info.link_check_timer_flag = TIMER_OFF;

  /* IPSec 初期化処理 */
  rtn = ipsec_attach(pAdapt);
  if(rtn != IPSEC_SUCCESS){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID 
    TNC_LOGOUT(KERN_ERR "ipsec_attach = %d\n",rtn);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ipsec_attach = %d\n",rtn);
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    kfree(pAdapt);
    return rtn;
  }
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add start */
#ifndef PACKET_FILTER_TIMING_CHANGE
  /* プロトコル登録 */
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  inet_add_protocol(&esp_proto,IPPROTO_ESP);

  rtn = ipsec_inet_add_protocol(&udp_esp_proto,IPPROTO_UDP);
#else 
  inet_add_protocol(&esp_proto);
  rtn = ipsec_inet_add_protocol(&udp_esp_proto);
#endif


  if(rtn == IPSEC_ERROR){
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
    inet_del_protocol(&esp_proto, IPPROTO_ESP);
#endif
    ipsec_detach(pAdapt);
    printk(KERN_ERR "ipsec_inet_add_protocol error\n");
    kfree(pAdapt);
    return rtn;
  }
  
  /* NAT traversal情報初期化  */
  pAdapt->NatInfo_SL.nat_info.enable = NAT_TRAV_ENABLE;
  pAdapt->NatInfo_SL.nat_info.own_port = DEFAULT_PORT;
  pAdapt->NatInfo_SL.nat_info.remort_port = DEFAULT_PORT;
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add end */

  return IPSEC_SUCCESS;
}

/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
/* ESP/UDPパケットフィルタリング開始処理
 *
 * 書式   : int linux_ipsec_protocol_init(void)
 * 引数   : なし
 * 戻り値 : なし
 */
int linux_ipsec_protocol_init(void)
{
    int rtn;
    /* プロトコル登録 */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
/* PMC-Viana-029 Quad組み込み対応 mod start */
#ifdef CONFIG_INET_ESP
    rtn = inet_del_protocol(&esp4_protocol, IPPROTO_ESP);
    TNC_LOGOUT("rtn =%d in inet_del_protocol \n",rtn);
#endif /* CONFIG_INET_ESP */
/* PMC-Viana-029 Quad組み込み対応 mod end */
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    
    rtn = inet_add_protocol(&esp_proto,IPPROTO_ESP);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("rtn =%d in inet_add_protocol \n",rtn);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "rtn =%d in inet_add_protocol \n",rtn);
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    
    rtn = ipsec_inet_add_protocol(&udp_esp_proto,IPPROTO_UDP);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("rtn =%d in ipsec_inet_add_protocol \n",rtn);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "rtn =%d in ipsec_inet_add_protocol \n",rtn);
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

    if(rtn == IPSEC_ERROR){
        inet_del_protocol(&esp_proto, IPPROTO_ESP);
        ipsec_detach(pAdapt);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
        TNC_LOGOUT(KERN_ERR "ipsec_inet_add_protocol error\n");
#else /* BUILD_ANDROID */
        printk(KERN_ERR "ipsec_inet_add_protocol error\n");
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
        kfree(pAdapt);
        return rtn;
    }

    /* NAT traversal情報初期化  */
    pAdapt->NatInfo_SL.nat_info.enable = NAT_TRAV_ENABLE;
    pAdapt->NatInfo_SL.nat_info.own_port = DEFAULT_PORT;
    pAdapt->NatInfo_SL.nat_info.remort_port = DEFAULT_PORT;

    return 0;
}
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add end */

/* ESP受信処理
 *
 * 書式   : int ipsec_recv(struct sk_buff *skbf)
 * 引数   : strcut sk_buff skbf : パケット情報
 * 戻り値 : 成功 : 0
 */
int ipsec_recv(struct sk_buff *skb)
{
  struct pkt_buff pktb;
  struct sk_buff *skbf;
  int rtn = 0;
  struct net_device *dev;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "--------------- ESP protocol rcv!! ------------------  \n");
#else /* BUILD_ANDROID */
  /*printk(KERN_ERR "--------------- ESP protocol rcv!! ------------------  \n");*/
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  
  if(!pskb_may_pull(skb,skb->len)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "pskb_may_pull error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "pskb_may_pull error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    kfree_skb(skb);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("Err1:ipsec_recv  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return 0;
  }

  if(skb_cloned(skb)){
    skbf = skb_copy(skb, GFP_ATOMIC);
    if(skbf == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "error : skb_copy in udp_esp_rcv\n");      
#else /* BUILD_ANDROID */
      printk(KERN_ERR "error : skb_copy in udp_esp_rcv\n");      
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      kfree_skb(skb);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("Err2:ipsec_recv  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      return 0;
    }
    kfree_skb(skb);
  }
  else{
    skbf = skb;
  }
  
  memset(&pktb, 0, sizeof(struct pkt_buff));

  /* set data to pkt_buff from sk_buff  */
  skb_push(skbf,sizeof(struct iphdr));
  SetPktBFromSkbuff(&pktb,skbf);
 
  /* 復号化 */
  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  rtn = ipsec_input(pAdapt,&pktb);
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);
  if(rtn < 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec_input error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ipsec_input error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("Err3:ipsec_recv  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }
  /*printk(KERN_ERR "ipsec_input success\n");*/
    
  /* set data to sk_buff from pkt_buff  */
  SetSkbFromPktbuff(&pktb,skbf);

  /* make new ethernet header */
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  skbf->mac_header = (unsigned char*)(skbf->data - sizeof(struct ethhdr));
  memset(skbf->mac_header, 0, sizeof(struct ethhdr));
  ((struct ethhdr*)skbf->mac_header)->h_proto = htons(ETH_P_IP);
  TNC_LOGOUT("h_proto:%d",((struct ethhdr*)skbf->mac_header)->h_proto);
#else   /* BUILD_ANDROID */
  skbf->mac.raw = (unsigned char*)(skbf->data - sizeof(struct ethhdr));
  memset(skbf->mac.raw, 0, sizeof(struct ethhdr));
  ((struct ethhdr*)skbf->mac.raw)->h_proto = htons(ETH_P_IP);
#endif  /* BUILD_ANDROID */    
/* PMC-Viana-001-Android_Build対応 add end */    
#else 
  skbf->mac.ethernet = (struct ethhdr*)(skbf->data - sizeof(struct ethhdr));
  memset(skbf->mac.ethernet, 0, sizeof(struct ethhdr));
  skbf->mac.ethernet->h_proto = htons(ETH_P_IP);
#endif

  skbf->protocol = htons(ETH_P_IP);
  skbf->ip_summed = CHECKSUM_NONE;
  
  /* device infomation overwrite */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  dev = __dev_get_by_name(sock_net(skbf->sk),pAdapt->drv_info.Vlan_name);
//     printk("vlan dev->name:%s in realdev_link_check \n",dev->name);
//    printk("pAdapt->drv_info.Vlan_name in ipsec_recv \n",pAdapt->drv_info.Vlan_name);
#else   /* BUILD_ANDROID */
  dev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  if(dev){
    skbf->dev = dev;
  }
  else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : dev_get_by_name (%s)\n",pAdapt->drv_info.Vlan_name);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "error : dev_get_by_name (%s)\n",pAdapt->drv_info.Vlan_name);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
//  dst_release(skb_dst(skbf));
  skb_dst_set(skbf, NULL);
#else   /* BUILD_ANDROID */
  dst_release(skbf->dst);
  skbf->dst = NULL;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

/* PMC-Viana-007-CONFIG_NETFILTER削除対応 add start */
#ifndef BUILD_ANDROID
#ifdef CONFIG_NETFILTER
  nf_conntrack_put(skbf->nfct);
  skbf->nfct = NULL;
#ifdef CONFIG_NETFILTER_DEBUG
  skbf->nf_debug = 0;
#endif
#endif
#endif  /* BUILD_ANDROID */
/* PMC-Viana-007-CONFIG_NETFILTER削除対応 add end */

/* skbf->data = (skbf->data)+20; */

/* hex_dump((char*)(skbf->data),skbf->len); */
/* rtn = netif_rx(skbf); */

  rtn = netif_rx(skbf);
  if(rtn != 0)
{  
  /* printk("netif_rx ESP_Tunnel: %d\n",rtn); */ /* for DIGA @yoshino */
}
  return 0 ;

 error:
  kfree_skb(skbf);
  return 0 ;
}


/* 16進ダンプ
 *
 * 書式   : void hex_dump(char *msg, int msglen)
 * 引数   : char *msg : ダンプ開始ポインタ
 *          int msglen : ダンプする長さ
 * 戻り値 : なし
 */
void hex_dump(char *msg, int msglen)
{
  int i;
  char tmp[64];

  if(msglen > 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("message length = %d\n",msglen);
#else /* BUILD_ANDROID */
    printk("message length = %d\n",msglen);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    
    for(i=0; i< msglen; i++){
      sprintf(&tmp[i%16 * 3], "%02x ", (u_char)msg[i]);
      if(i%16 == 15){
	tmp[16*3] = 0x00;
	tmp[8*3-1] = 0x00;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("%s   %s\n",tmp,&tmp[8*3]);
#else /* BUILD_ANDROID */
	printk("%s   %s\n",tmp,&tmp[8*3]);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      }
    }
    if(i%16 != 0){
      tmp[i%16*3] = 0x00;
      if(i%16 > 8){
	tmp[8*3-1] = 0x00;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("%s   %s\n",tmp,&tmp[8*3]);
#else /* BUILD_ANDROID */
	printk("%s   %s\n",tmp,&tmp[8*3]);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      }
      else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("%s\n",tmp);
#else /* BUILD_ANDROID */
	printk("%s\n",tmp);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      }
    }
  }
}

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6

/* UDPプロトコル差し替え
 *
 * 書式   : int ipsec_inet_add_protocol(struct net_protocol *prot, unsigned char protocol)
 * 引数   : struct inet_protocol *prot, unsigned char protocol
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int ipsec_inet_add_protocol(struct net_protocol *prot, unsigned char protocol)
{
  int hash;
  int ret;
 
/* PMC-Viana-029 Quad組み込み対応 add */
   struct net_protocol __rcu* tempProtos;

  hash = protocol & (MAX_INET_PROTOS - 1);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_inet_add_protocol \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

/* PMC-Viana-026-NEMO組み込み対応 delete start */
#ifndef BUILD_ICS
    spin_lock_bh(&inet_proto_lock);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 delete end */

/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
/* PMC-Viana-029 Quad組み込み対応 mod */
//    struct net_protocol __rcu* tempProtos = inet_protos[hash];
   tempProtos = (struct net_protocol *)inet_protos[hash];

    ret = (cmpxchg((const struct net_protocol **)&inet_protos[hash],
        tempProtos, prot) != NULL) ? IPSEC_SUCCESS : IPSEC_ERROR;
    
    if (ret == IPSEC_SUCCESS)
    {
        pAdapt->udp_prot = tempProtos;
    }
    else
    {
        inet_protos[hash] = tempProtos;
        TNC_LOGOUT(KERN_ERR "error : no udp protocol in ipsec_inet_add_protocol\n");
    }
#else /* BUILD_ICS */

    if (inet_protos[hash]) {
       /* swap existing protocol(UPD_PROTOCOL) to ESP */
       pAdapt->udp_prot = inet_protos[hash];
       inet_protos[hash] = prot;
       ret = IPSEC_SUCCESS; 
    } else {
       /* not registered protocol(UPD_PROTOCOL) */
       ret = IPSEC_ERROR;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
       TNC_LOGOUT(KERN_ERR "error : no udp protocol in ipsec_inet_add_protocol\n");
#else /* BUILD_ANDROID */
       printk(KERN_ERR "error : no udp protocol in ipsec_inet_add_protocol\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    }
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */    
    
/* PMC-Viana-026-NEMO組み込み対応 delete start */
#ifndef BUILD_ICS
    spin_unlock_bh(&inet_proto_lock);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 delete end */
    return ret;
}


/* UDPプロトコル差し戻し
 *
 * 書式   : int ipsec_inet_del_protocol(struct inet_protocol *prot, unsigned char protocol)
 * 引数   : strcut inet_protocol *prot, unsigned char protocol
 * 戻り値 : 成功 : 0
 *          失敗 : -1
 */
int ipsec_inet_del_protocol(struct net_protocol *prot, unsigned char protocol)
{

  int hash; 
  int ret;

  hash = protocol & (MAX_INET_PROTOS - 1);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ipsec_inet_del_protocol \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
/* PMC-Viana-026-NEMO組み込み対応 delete start */
#ifndef BUILD_ICS
  spin_lock_bh(&inet_proto_lock);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 delete end */

/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS

    ret = (cmpxchg((const struct net_protocol **)&inet_protos[hash],
                 prot, pAdapt->udp_prot) == prot) ? IPSEC_SUCCESS : IPSEC_ERROR;

    if (ret == IPSEC_ERROR)
    {
        TNC_LOGOUT(KERN_ERR "error : no ipsec(udp) protocol in ipsec_inet_del_protocol\n");
    }
#else /* BUILD_ICS */
  if (inet_protos[hash] == prot) {
    inet_protos[hash] = pAdapt->udp_prot;
    ret = IPSEC_SUCCESS; 
  } else {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : no ipsec(udp) protocol in ipsec_inet_del_protocol\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "error : no ipsec(udp) protocol in ipsec_inet_del_protocol\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    ret = IPSEC_ERROR;
  }
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */

/* PMC-Viana-026-NEMO組み込み対応 delete start */
#ifndef BUILD_ICS
  spin_unlock_bh(&inet_proto_lock);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 delete end */

  synchronize_net();

  return ret;
}

#else 

/* UDPプロトコル差し替え
 *
 * 書式   : int ipsec_inet_add_protocol(struct inet_protocol *prot)
 * 引数   : struct inet_protocol *prot
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int ipsec_inet_add_protocol(struct inet_protocol *prot)
{
   
  struct inet_protocol *p1;

  inet_add_protocol(prot);

  br_write_lock_bh(BR_NETPROTO_LOCK);
  /* check different protocols */
  p1 = prot->next;
  do{
    if(prot->protocol != p1->protocol){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "find different protocol");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "find different protocol");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      br_write_unlock_bh(BR_NETPROTO_LOCK);
      return IPSEC_ERROR;
    }
    p1 = p1->next;
  }while(p1);

  pAdapt->udp_prot = prot->next;
  br_write_unlock_bh(BR_NETPROTO_LOCK);

  if(inet_del_protocol(prot->next)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : inet_del_protocol in ipsec_inet_add_protocol\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "error : inet_del_protocol in ipsec_inet_add_protocol\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return IPSEC_ERROR;
  }
  
  return IPSEC_SUCCESS;
}

/* UDPプロトコル差し戻し
 *
 * 書式   : int ipsec_inet_del_protocol(struct inet_protocol *prot)
 * 引数   : strcut inet_protocol *prot :
 * 戻り値 : 成功 : 0
 *          失敗 : -1
 */
int ipsec_inet_del_protocol(struct inet_protocol *prot)
{
  inet_add_protocol(pAdapt->udp_prot);
  return inet_del_protocol(prot);
}

#endif



/* sk_buff - pktbuff 変換関数
 *
 * 書式   : void SetPktBFromSkbuff(struct pkt_buff *pktb, 
 *                                            struct sk_buff *skb)
 * 引数   : struct pkt_buff *pktb : IPSec用パケット情報
 *          struct sk_buff *skbf : パケット情報
 * 戻り値 : なし
 *           
 */
void SetPktBFromSkbuff(struct pkt_buff *pktb, struct sk_buff *skbf)
{

  /* pkt_buffの設定 */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call SetPktBFromSkbuff \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  pktb->head = skbf->head;
  pktb->end  = skbf->end;
  pktb->data = skbf->data;
  pktb->tail = skbf->tail;
  pktb->len  = skbf->len;
  pktb->buff_len = skbf->end - skbf->head;
/* printk(KERN_ERR "-------------------sk_buff - pktbuff------------------\n"); */
/* hex_dump((char*)(skbf->data),skbf->len); */
  return ;

}

/* pktbuff - sk_buff 変換関数
 *
 * 書式   : void SetSkbFromPktbuff(struct pkt_buff *pktb, struct sk_buff *skbf)
 * 引数   : struct pkt_buff *pktb : IPSec用パケット情報
 *          struct sk_buff *skb : パケット情報
 * 戻り値 : なし
 *           
 */
void SetSkbFromPktbuff(struct pkt_buff *pktb, struct sk_buff *skbf)
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call SetSkbFromPktbuff \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* sk_buff設定 */
  skbf->data = pktb->data;
  skbf->tail = pktb->tail;
  skbf->len  = pktb->len;
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  skbf->network_header = (char*)pktb->data;
//  printk(KERN_ERR "skbf->data:%s\n skbf->tail:%s\n skbf->len:%d\n ((struct iphdr*)(skbf->network_header)).saddr:%x \n ((struct iphdr*)(skbf->network_header)).daddr:%x \n s_udpport: %x \n d_udpport: %x \n ",skbf->data,skbf->tail,skbf->len,((struct iphdr*)(skbf->network_header))->saddr,((struct iphdr*)(skbf->network_header))->daddr,*(short*)(((struct iphdr*)(skbf->network_header)) + 1),*(short*)((((struct iphdr*)(skbf->network_header)) + 1)+1));
  dump_skb(skbf); 
#else   /* BUILD_ANDROID */
  skbf->nh.iph = (struct iphdr*)pktb->data;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
/* printk(KERN_ERR "skbf->data:%s\n skbf->tail:%s\n skbf->len:%d\n skbf->nh.iph.saddr:%x \n skbf->nh.iph.daddr:%x \n s_udpport: %x \n d_udpport: %x \n ",skbf->data,skbf->tail,skbf->len,skbf->nh.iph->saddr,skbf->nh.iph->daddr,*(short*)(skbf->nh.iph + 1),*(short*)((skbf->nh.iph + 1)+1)); */
  return;

}


/* IPSec終了処理
 *
 * 書式   : void linux_ipsec_exit(void)
 * 引数   : なし
 * 戻り値 : なし
 */
void linux_ipsec_exit(void)
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call linux_ipsec_exit \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(pAdapt->timer_info.link_check_timer_flag)
    del_timer(&pAdapt->timer_info.link_check_timer);

/* PMC-Viana-019-TNC_パケットフィルタタイミング変更 add start */
#ifndef PACKET_FILTER_TIMING_CHANGE
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6

  if(ipsec_inet_del_protocol(&udp_esp_proto, IPPROTO_UDP))
     printk(KERN_ERR "ipsec_inet_del_protocol error\n");

  if(inet_del_protocol(&esp_proto, IPPROTO_ESP))
    printk(KERN_ERR "error : esp protocol delete\n");

#else 
  if(ipsec_inet_del_protocol(&udp_esp_proto))
     printk(KERN_ERR "ipsec_inet_del_protocol error\n");

  if(inet_del_protocol(&esp_proto))
    printk(KERN_ERR "error : esp protocol delete\n");

#endif
#endif /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-TNC_パケットフィルタタイミング変更 add end */

  ipsec_detach(pAdapt);
  DBG_printf("ipsec_detach done\n");

  kfree(pAdapt);

  return;
}

/* PMC-Viana-019-TNC_パケットフィルタタイミング変更 add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
/* ESP/UDPパケットフィルタリング停止処理
 *
 * 書式   : void linux_ipsec_protocol_exit(void)
 * 引数   : なし
 * 戻り値 : なし
 */
void linux_ipsec_protocol_exit(void)
{
  int rtn;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call linux_ipsec_protocol_exit \n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "Call linux_ipsec_protocol_exit \n");
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  rtn = ipsec_inet_del_protocol(&udp_esp_proto, IPPROTO_UDP);

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("rtn =%d in ipsec_inet_del_protocol \n",rtn);
#else /* BUILD_ANDROID */
  printk(KERN_ERR "rtn =%d in ipsec_inet_del_protocol \n",rtn);
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  rtn = inet_del_protocol(&esp_proto, IPPROTO_ESP);
  
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("rtn =%d in inet_del_protocol \n",rtn);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "rtn =%d in inet_del_protocol \n",rtn);
#endif /*BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 add start */
#ifdef BUILD_ANDROID
/* PMC-Viana-029 Quad組み込み対応 mod start */
#ifdef CONFIG_INET_ESP
  /* プロトコル登録 */
  rtn = inet_add_protocol(&esp4_protocol, IPPROTO_ESP);
  TNC_LOGOUT("rtn =%d in inet_add_protocol \n",rtn);
#endif /* CONFIG_INET_ESP */
/* PMC-Viana-029 Quad組み込み対応 mod end */
#endif /* BUILD_ANDROID */
/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 add end */

  return;
}
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-TNC_パケットフィルタタイミング変更 add end */

/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
// tomoharu
static void dump_skb(struct sk_buff *skb) {
	if (skb == NULL) {
		TNC_LOGOUT(KERN_ERR "skb==NULL\n");
		return;
	}

#if 0   // tomoharu
	TNC_LOGOUT(KERN_ERR "skb.next=%p\n", skb->next);
	TNC_LOGOUT(KERN_ERR "skb.prev=%p\n", skb->prev);
	TNC_LOGOUT(KERN_ERR "skb.list=%p\n", skb->list);
	TNC_LOGOUT(KERN_ERR "skb.sk=%p\n", skb->sk);
	TNC_LOGOUT(KERN_ERR "skb.stamp.tv_sec=%ld\n", skb->stamp.tv_sec); /* for DIGA (Warning) @yoshino */
	TNC_LOGOUT(KERN_ERR "skb.stamp.tv_usec=%ld\n", skb->stamp.tv_usec); /* for DIGA (Warning) @yoshino */
	TNC_LOGOUT(KERN_ERR "skb.dev=%p(%s)\n", skb->dev, skb->dev ? skb->dev->name : "NULL");
	TNC_LOGOUT(KERN_ERR "skb.input_dev=%p\n", skb->input_dev);
	TNC_LOGOUT(KERN_ERR "skb.real_dev=%p\n", skb->real_dev);
	TNC_LOGOUT(KERN_ERR "skb.h.th=%p\n", skb->h.th);
	TNC_LOGOUT(KERN_ERR "skb.h.uh=%p\n", skb->h.uh);
	TNC_LOGOUT(KERN_ERR "skb.h.icmph=%p\n", skb->h.icmph);
	TNC_LOGOUT(KERN_ERR "skb.h.igmph=%p\n", skb->h.igmph);
	TNC_LOGOUT(KERN_ERR "skb.h.ipiph=%p\n", skb->h.ipiph);
	TNC_LOGOUT(KERN_ERR "skb.h.ipv6h=%p\n", skb->h.ipv6h);
	TNC_LOGOUT(KERN_ERR "skb.h.raw=%p\n", skb->h.raw);

	TNC_LOGOUT(KERN_ERR "skb.nh.iph=%p\n", skb->nh.iph);
	TNC_LOGOUT(KERN_ERR "skb.nh.ipv6h=%p\n", skb->nh.ipv6h);
	TNC_LOGOUT(KERN_ERR "skb.nh.arph=%p\n", skb->nh.arph);
	TNC_LOGOUT(KERN_ERR "skb.nh.raw=%p\n", skb->nh.raw);

	TNC_LOGOUT(KERN_ERR "skb.dst=%p\n", skb->dst);
	TNC_LOGOUT(KERN_ERR "skb.sp=%p\n", skb->sp);

	TNC_LOGOUT(KERN_ERR "skb.head=%p\n", skb->head);
	TNC_LOGOUT(KERN_ERR "skb.data=%p\n", skb->data);
	TNC_LOGOUT(KERN_ERR "skb.tail=%p\n", skb->tail);
	TNC_LOGOUT(KERN_ERR "skb.end=%p\n", skb->end);
	TNC_LOGOUT(KERN_ERR "skb.end=%p\n", skb->end);

	TNC_LOGOUT(KERN_ERR "skb_headroom=%d\n", skb_headroom(skb));
	TNC_LOGOUT(KERN_ERR "skb_tailroom=%d\n", skb_tailroom(skb));
	TNC_LOGOUT(KERN_ERR "skb_headlen=%d\n", skb_headlen(skb));
#endif   // tomoharu
	TNC_LOGOUT(KERN_ERR "skb.dev=%p(%s)\n", skb->dev, skb->dev ? skb->dev->name : "NULL");
	TNC_LOGOUT(KERN_ERR "skb._skb_refdst & SKB_DST_NOREF=%d\n", skb->_skb_refdst & SKB_DST_NOREF);
	TNC_LOGOUT(KERN_ERR "skb._skb_refdst & SKB_DST_PTRMASK=%p\n", skb->_skb_refdst & SKB_DST_PTRMASK);
	TNC_LOGOUT(KERN_ERR "skb.len=%d\n", skb->len);
	TNC_LOGOUT(KERN_ERR "skb.data_len=%d\n", skb->data_len);
	TNC_LOGOUT(KERN_ERR "skb.mac_len=%d\n", skb->mac_len);
	TNC_LOGOUT(KERN_ERR "skb.hdr_len=%d\n", skb->hdr_len);
	TNC_LOGOUT(KERN_ERR "skb.protocol=%d\n", skb->protocol);
	TNC_LOGOUT(KERN_ERR "skb.destructor=%p\n", skb->destructor);

	TNC_LOGOUT(KERN_ERR "skb.transport_header=%p\n", skb->transport_header);
	TNC_LOGOUT(KERN_ERR "skb.network_header=%p\n", skb->network_header);
	TNC_LOGOUT(KERN_ERR "skb.mac_header=%p\n", skb->mac_header);
	TNC_LOGOUT(KERN_ERR "skb.tail=%p\n", skb->tail);
	TNC_LOGOUT(KERN_ERR "skb.end=%p\n", skb->end);
	TNC_LOGOUT(KERN_ERR "skb.data=%p\n", skb->data);
	hex_dump(skb->data, skb->len);
	TNC_LOGOUT(KERN_ERR "skb.head=%p\n", skb->head);
	hex_dump(skb->head, skb_headroom(skb));
	TNC_LOGOUT(KERN_ERR "skb.tail=%p\n", skb->tail);
	hex_dump(skb->tail, skb_tailroom(skb));
}
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */


/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
EXPORT_SYMBOL(pAdapt);
EXPORT_SYMBOL(ipsec_debug);
EXPORT_SYMBOL(SetSkbFromPktbuff);
EXPORT_SYMBOL(SetPktBFromSkbuff);
#endif

MODULE_LICENSE("Proprietary");
module_init(linux_ipsec_init);
module_exit(linux_ipsec_exit);

