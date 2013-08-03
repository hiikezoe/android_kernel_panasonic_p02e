/*
 * 仮想NICドライバ処理
 *
 */

#ifdef BUILD_TOTORO
#include <generated/autoconf.h>
#else /* BUILD_TOTORO */
#include <linux/config.h>
#endif /* BUILD_TOTORO */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <net/ip.h>
#include <linux/netfilter_ipv4.h>
#include <net/pkt_sched.h>
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
#include <linux/in_route.h>
#include <net/route.h>
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

#include "linux_precomp.h"
#include "pkt_buff.h"

#define IPSEC_HEADER_OFFSET 100
#define IPSEC_TAILER_OFFSET 32
#define MAX_IPSEC_HEADER    74 /* Eth(14)+IP(20)+UDP(8)+mark(8)+ESP(8)+IV(0,8,16) */
#define MAX_IPSEC_TAILER    22 /* pad(0-3)+padhead(2)+auth(12) */
#define ETHER_HEADER_LEN    14


/* PMC-Viana-xxx-NEMO組み込み対応(暫定) add start */
#define ip_route_output_key ip_route_output_key_viana
/* PMC-Viana-xxx-NEMO組み込み対応(暫定) add end */
extern PADAPT pAdapt;
extern spinlock_t GobaleLock;

int ipsec_output(PADAPT,struct pkt_buff*);
void hex_dump(char *msg, int msglen);

void SetPktBFromSkbuff(struct pkt_buff *pktb, struct sk_buff *skbf);
void SetSkbFromPktbuff(struct pkt_buff *pktb, struct sk_buff *skbf);
u_short	calc_udp_sum(struct ip_udp *ip_udp);   
int check_real_device(struct net_device *realdev);
int set_timer_realdev_check(unsigned int timeout);
int Nat_Trav_handle(struct sk_buff *skbf);
void dev_check_in_kfree_skb(struct sk_buff *skb);
void dev_check_only_in_kfree_skb(struct sk_buff *skb);
void count_check_only_in_kfree_skb(struct sk_buff *skb);
void count_check_in_kfree_skb(struct sk_buff *skb);


#if 0 /* TEST */
#include <linux/udp.h>
#include <linux/tcp.h>
static const char *inet_ntoa(unsigned long address) {
	static char	buffer[32];
	unsigned long	addr;

	addr = ntohl(address);
	snprintf(
		buffer,
		sizeof(buffer),
		"%d.%d.%d.%d",
		(int)(addr >> 24) & 0xFF,
		(int)(addr >> 16) & 0xFF,
		(int)(addr >>  8) & 0xFF,
		(int)(addr      ) & 0xFF
	);

	return buffer;
}

static unsigned long inet_aton(const char *name) {
	int	a, b, c, d, ret;
	unsigned long	addr;

	ret = sscanf(name, "%d.%d.%d.%d", &a, &b, &c, &d);
	if (ret != 4)
		return 0;

	addr = (a << 24) | (b << 16) | (c << 8) | d;
	addr = htonl(addr);
	return addr;
}

static void iphdr_dump(struct iphdr *iph) {
	if (iph == NULL) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "iphdr==NULL\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "iphdr==NULL\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		return;
	}
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "iphdr.ihl=%d\n", iph->ihl);
	TNC_LOGOUT(KERN_ERR "iphdr.version=%d\n", iph->version);
	TNC_LOGOUT(KERN_ERR "iphdr.tos=%d\n", iph->tos);
	TNC_LOGOUT(KERN_ERR "iphdr.tot_len=%d\n", iph->tot_len);
	TNC_LOGOUT(KERN_ERR "iphdr.id=%d\n", iph->id);
	TNC_LOGOUT(KERN_ERR "iphdr.frag_off=%d\n", iph->frag_off);
	TNC_LOGOUT(KERN_ERR "iphdr.ttl=%d\n", iph->ttl);
	TNC_LOGOUT(KERN_ERR "iphdr.protocol=%d\n", iph->protocol);
	TNC_LOGOUT(KERN_ERR "iphdr.check=%d\n", iph->check);
	TNC_LOGOUT(KERN_ERR "iphdr.saddr=%08X(%s)\n", iph->saddr, inet_ntoa(iph->saddr));
	TNC_LOGOUT(KERN_ERR "iphdr.daddr=%08X(%s)\n", iph->daddr, inet_ntoa(iph->daddr));
#else /* BUILD_ANDROID */
	printk(KERN_ERR "iphdr.ihl=%d\n", iph->ihl);
	printk(KERN_ERR "iphdr.version=%d\n", iph->version);
	printk(KERN_ERR "iphdr.tos=%d\n", iph->tos);
	printk(KERN_ERR "iphdr.tot_len=%d\n", iph->tot_len);
	printk(KERN_ERR "iphdr.id=%d\n", iph->id);
	printk(KERN_ERR "iphdr.frag_off=%d\n", iph->frag_off);
	printk(KERN_ERR "iphdr.ttl=%d\n", iph->ttl);
	printk(KERN_ERR "iphdr.protocol=%d\n", iph->protocol);
	printk(KERN_ERR "iphdr.check=%d\n", iph->check);
	printk(KERN_ERR "iphdr.saddr=%08X(%s)\n", iph->saddr, inet_ntoa(iph->saddr));
	printk(KERN_ERR "iphdr.daddr=%08X(%s)\n", iph->daddr, inet_ntoa(iph->daddr));
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
}

static void udphdr_dump(struct udphdr *udph) {
	if (udph == NULL) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "udph==NULL\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "udph==NULL\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		return;
	}
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "udphdr.source=%04X(%d)\n", udph->source, ntohs(udph->source));
	TNC_LOGOUT(KERN_ERR "udphdr.dest=%04X(%d)\n", udph->dest, ntohs(udph->dest));
	TNC_LOGOUT(KERN_ERR "udphdr.len=%d(%d)\n", udph->len, ntohs(udph->len));
	TNC_LOGOUT(KERN_ERR "udphdr.check=%04X\n", udph->check);
#else /* BUILD_ANDROID */
	printk(KERN_ERR "udphdr.source=%04X(%d)\n", udph->source, ntohs(udph->source));
	printk(KERN_ERR "udphdr.dest=%04X(%d)\n", udph->dest, ntohs(udph->dest));
	printk(KERN_ERR "udphdr.len=%d(%d)\n", udph->len, ntohs(udph->len));
	printk(KERN_ERR "udphdr.check=%04X\n", udph->check);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
}

static void tcphdr_dump(struct tcphdr *tcphdr) {
	if (tcphdr == NULL) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
		TNC_LOGOUT(KERN_ERR "tcphdr==NULL\n");
#else /* BUILD_ANDROID */
		printk(KERN_ERR "tcphdr==NULL\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
		return;
	}
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "tcphdr.source=%04X(%d)\n", tcphdr->source, ntohs(tcphdr->source));
	TNC_LOGOUT(KERN_ERR "tcphdr.dest=%04X(%d)\n", tcphdr->dest, ntohs(tcphdr->dest));
	TNC_LOGOUT(KERN_ERR "tcphdr.seq=%d\n", ntohl(tcphdr->seq));
	TNC_LOGOUT(KERN_ERR "tcphdr.ack=%d\n", ntohl(tcphdr->ack));
	TNC_LOGOUT(KERN_ERR "tcphdr.res1=%d\n", tcphdr->res1);
	TNC_LOGOUT(KERN_ERR "tcphdr.doff=%d\n", tcphdr->doff);
	TNC_LOGOUT(KERN_ERR "tcphdr.fin=%s\n", tcphdr->fin ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.syn=%s\n", tcphdr->syn ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.rst=%s\n", tcphdr->rst ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.psh=%s\n", tcphdr->psh ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.ack=%s\n", tcphdr->ack ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.urg=%s\n", tcphdr->urg ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.ece=%s\n", tcphdr->ece ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.cwr=%s\n", tcphdr->cwr ? "on" : "off");
	TNC_LOGOUT(KERN_ERR "tcphdr.window=%d\n", ntohs(tcphdr->window));
	TNC_LOGOUT(KERN_ERR "tcphdr.check=%04X\n", tcphdr->check);
	TNC_LOGOUT(KERN_ERR "tcphdr.urg_ptr=%d\n", ntohs(tcphdr->urg_ptr));
#else /* BUILD_ANDROID */

	printk(KERN_ERR "tcphdr.source=%04X(%d)\n", tcphdr->source, ntohs(tcphdr->source));
	printk(KERN_ERR "tcphdr.dest=%04X(%d)\n", tcphdr->dest, ntohs(tcphdr->dest));
	printk(KERN_ERR "tcphdr.seq=%d\n", ntohl(tcphdr->seq));
	printk(KERN_ERR "tcphdr.ack=%d\n", ntohl(tcphdr->ack));
	printk(KERN_ERR "tcphdr.res1=%d\n", tcphdr->res1);
	printk(KERN_ERR "tcphdr.doff=%d\n", tcphdr->doff);
	printk(KERN_ERR "tcphdr.fin=%s\n", tcphdr->fin ? "on" : "off");
	printk(KERN_ERR "tcphdr.syn=%s\n", tcphdr->syn ? "on" : "off");
	printk(KERN_ERR "tcphdr.rst=%s\n", tcphdr->rst ? "on" : "off");
	printk(KERN_ERR "tcphdr.psh=%s\n", tcphdr->psh ? "on" : "off");
	printk(KERN_ERR "tcphdr.ack=%s\n", tcphdr->ack ? "on" : "off");
	printk(KERN_ERR "tcphdr.urg=%s\n", tcphdr->urg ? "on" : "off");
	printk(KERN_ERR "tcphdr.ece=%s\n", tcphdr->ece ? "on" : "off");
	printk(KERN_ERR "tcphdr.cwr=%s\n", tcphdr->cwr ? "on" : "off");
	printk(KERN_ERR "tcphdr.window=%d\n", ntohs(tcphdr->window));
	printk(KERN_ERR "tcphdr.check=%04X\n", tcphdr->check);
	printk(KERN_ERR "tcphdr.urg_ptr=%d\n", ntohs(tcphdr->urg_ptr));
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
}
static void dump_skb(struct sk_buff *skb) {
	if (skb == NULL) {
		printk(KERN_ERR "skb==NULL\n");
		return;
	}

	printk(KERN_ERR "skb.next=%p\n", skb->next);
	printk(KERN_ERR "skb.prev=%p\n", skb->prev);
	printk(KERN_ERR "skb.list=%p\n", skb->list);
	printk(KERN_ERR "skb.sk=%p\n", skb->sk);
	printk(KERN_ERR "skb.stamp.tv_sec=%ld\n", skb->stamp.tv_sec);
	printk(KERN_ERR "skb.stamp.tv_usec=%ld\n", skb->stamp.tv_usec);
	printk(KERN_ERR "skb.dev=%p(%s)\n", skb->dev, skb->dev ? skb->dev->name : "NULL");
	printk(KERN_ERR "skb.input_dev=%p\n", skb->input_dev);
	printk(KERN_ERR "skb.real_dev=%p\n", skb->real_dev);
	printk(KERN_ERR "skb.h.th=%p\n", skb->h.th);
	printk(KERN_ERR "skb.h.uh=%p\n", skb->h.uh);
	printk(KERN_ERR "skb.h.icmph=%p\n", skb->h.icmph);
	printk(KERN_ERR "skb.h.igmph=%p\n", skb->h.igmph);
	printk(KERN_ERR "skb.h.ipiph=%p\n", skb->h.ipiph);
	printk(KERN_ERR "skb.h.ipv6h=%p\n", skb->h.ipv6h);
	printk(KERN_ERR "skb.h.raw=%p\n", skb->h.raw);

	printk(KERN_ERR "skb.nh.iph=%p\n", skb->nh.iph);
	printk(KERN_ERR "skb.nh.ipv6h=%p\n", skb->nh.ipv6h);
	printk(KERN_ERR "skb.nh.arph=%p\n", skb->nh.arph);
	printk(KERN_ERR "skb.nh.raw=%p\n", skb->nh.raw);

	printk(KERN_ERR "skb.dst=%p\n", skb->dst);
	printk(KERN_ERR "skb.sp=%p\n", skb->sp);

	printk(KERN_ERR "skb.head=%p\n", skb->head);
	printk(KERN_ERR "skb.data=%p\n", skb->data);
	printk(KERN_ERR "skb.tail=%p\n", skb->tail);
	printk(KERN_ERR "skb.end=%p\n", skb->end);
	printk(KERN_ERR "skb.end=%p\n", skb->end);

	printk(KERN_ERR "skb_headroom=%d\n", skb_headroom(skb));
	printk(KERN_ERR "skb_tailroom=%d\n", skb_tailroom(skb));
	printk(KERN_ERR "skb_headlen=%d\n", skb_headlen(skb));
}

#endif /* TEST */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
static void dump_skb(struct sk_buff *skb) {
	if (skb == NULL) {
		TNC_LOGOUT(KERN_ERR "skb==NULL\n");
		return;
	}


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
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

/* for DIGA @yoshino */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
static inline int dev_requeue_skb(struct sk_buff *skb, struct Qdisc *q)
{
	TNC_LOGOUT("Call dev_requeue_skb \n");
	skb_dst_force(skb);
	q->gso_skb = skb;
	q->qstats.requeues++;
	q->q.qlen++;	/* it's still part of the queue */
	__netif_schedule(q);

	return 0;
}
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
/* 仮想NIC送信処理関数
 *
 * 書式   : int VLan_xmit(struct sk_buff *skb, struct net_device *dev)
 * 引数   : struct sk_buff *skb : パケット情報
 *          struct net_device *dev : デバイス構造体
 * 戻り値 : 成功 : 0
 *        
 */
int VLan_xmit(struct sk_buff *skb, struct net_device *dev)
{
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  struct net_device_stats *stats = netdev_priv(dev);
#else   /* BUILD_ANDROID */
  struct net_device_stats *stats = dev->priv;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  struct pkt_buff pktb;
  struct sk_buff *skbf;
  int rtn;
  struct net_device *realdev;
  struct rtable *route;
  int headroom,tailroom;
  unsigned char *data;
/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  struct flowi flowi_tnc;
#endif
#ifdef IP_STB
  struct iphdr  iphdr;
#endif

  skb = skb_unshare(skb, GFP_ATOMIC);
  if (skb == NULL) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "skb_unshare failure\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "skb_unshare failure\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return 0;
  }
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  dump_skb(skb);   // tomoharu
#else /* BUILD_ANDROID */
  /* dump_skb(skb); */
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* iphdr_dump(skb->nh.iph); */
  /* udphdr_dump(skb->h.uh); */
  /* tcphdr_dump(skb->h.th); */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call VLan_xmit \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

/* printk(KERN_ERR "%s:%d:%s:entering\n", __FILE__, __LINE__, __FUNCTION__); */
/* iphdr_dump(skb->nh.iph); */
/* for DIGA @yoshino */

  if(&(pAdapt->DevName_SL.real_dev_name[0]) == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "real device name nothing\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "real device name nothing\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    kfree_skb(skb);
    return 0;
  }

  /* 送信処理中カウンタチェック */
  if(pAdapt->drv_info.sending_count >= pAdapt->drv_info.max_sending_num){
    /* skbをqueueへ差し戻す */
    if(dev->qdisc){
      if(dev->qdisc->ops){
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
        int ret;
        TNC_LOGOUT("Call dev_requeue_skb \n");
        ret = dev_requeue_skb(skb,dev->qdisc);
        TNC_LOGOUT("dev_requeue_skb ret=%d \n",ret);
#else   /* BUILD_ANDROID */
	if(dev->qdisc->ops->requeue){
	  dev->qdisc->ops->requeue(skb,dev->qdisc);
	}
	else{
	  printk(KERN_ERR "error : dev->qdisc->ops->requeue is NULL\n");
	  kfree_skb(skb);
	}
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
      }
      else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : dev->qdisc->ops is NULL\n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "error : dev->qdisc->ops is NULL\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	kfree_skb(skb);
      }
    }
    else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "error : dev->qdisc is NULL\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "error : dev->qdisc is NULL\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      kfree_skb(skb);
    }
    return 0;
  }

  memset(&pktb, 0, sizeof(struct pkt_buff));

  /* buffer check */
  headroom = skb_headroom(skb);
  tailroom = skb_tailroom(skb);


#if 1 /* for DIGA @yoshino */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
TNC_LOGOUT(KERN_ERR "%s:%d:%s:before skb_copy_expand\n", __FILE__, __LINE__, __FUNCTION__);   // tomoharu
dump_skb(skb);   // tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#if 0
printk(KERN_ERR "%s:%d:%s:before skb_copy_expand\n", __FILE__, __LINE__, __FUNCTION__);
dump_skb(skb);
iphdr_dump(skb->nh.iph);
printk(KERN_ERR "%s:%d:%s:dump\n", __FILE__, __LINE__, __FUNCTION__);
printk(KERN_ERR "skb->destructor = %p\n", skb->destructor);
printk(KERN_ERR "skb->data = %p\n", skb->data);
printk(KERN_ERR "skb->iphdr = %p\n", skb->nh.iph);
printk(KERN_ERR "skb->iphdr->daddr = %p\n", &skb->nh.iph->daddr);
printk(KERN_ERR "offset = %d\n", (int)&skb->nh.iph->daddr - (int)skb->nh.iph);
printk(KERN_ERR "size = %d\n", sizeof(skb->nh.iph->daddr));
#endif
  /* パケット前後のバッファの空きをチェック */
  if(headroom < IPSEC_HEADER_OFFSET || tailroom < IPSEC_TAILER_OFFSET){
#if 1
    skbf = skb_copy_expand(skb, IPSEC_HEADER_OFFSET + headroom,
		  IPSEC_TAILER_OFFSET, GFP_ATOMIC);
#else
    skbf = skb_copy_expand(skb, IPSEC_HEADER_OFFSET,
		  IPSEC_TAILER_OFFSET, GFP_ATOMIC);
#endif
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
TNC_LOGOUT(KERN_ERR "%s:%d:%s:after skb_copy_expand\n", __FILE__, __LINE__, __FUNCTION__);   // tomoharu
TNC_LOGOUT(KERN_ERR "%s:%d:%s:dump\n", __FILE__, __LINE__, __FUNCTION__);   // tomoharu
dump_skb(skbf);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#if 0
printk(KERN_ERR "%s:%d:%s:dump\n", __FILE__, __LINE__, __FUNCTION__);
printk(KERN_ERR "skbf->destructor = %p\n", skbf->destructor);
printk(KERN_ERR "skbf->data = %p\n", skbf->data);
printk(KERN_ERR "iphdr = %p\n", skbf->nh.iph);
printk(KERN_ERR "iphdr->daddr = %p\n", &skbf->nh.iph->daddr);
printk(KERN_ERR "offset = %d\n", (int)&skbf->nh.iph->daddr - (int)skbf->nh.iph);
printk(KERN_ERR "size = %d\n", sizeof(skbf->nh.iph->daddr));
printk(KERN_ERR "%s:%d:%s:skb_copy_expand\n", __FILE__, __LINE__, __FUNCTION__);
dump_skb(skbf);
iphdr_dump(skbf->nh.iph);
#endif

    if(skbf == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "skb_copy_expand error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "skb_copy_expand error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      stats->tx_errors++;
      kfree_skb(skb);
      return 0;
    }

#if 1 /* for DIGA @yoshino */
    /* skb_unsharの影響でNULLが設定されている時がある */
    if (skb->sk != NULL) {
      skb_set_owner_w(skbf,skb->sk);
    }
#else
    skb_set_owner_w(skbf,skb->sk);
#endif
    kfree_skb(skb);
  }
  else{
    skbf = skb;
  }

#else /* for DIGA @yoshino */

  if (pskb_expand_head(skb, IPSEC_HEADER_OFFSET, IPSEC_TAILER_OFFSET, GFP_ATOMIC) != 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "pskb_expand error\n");
#else /* BUILD_ANDROID */
      printk(KERN_ERR "pskb_expand error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      stats->tx_errors++;
      kfree_skb(skb);
      return 0;
  }
  skbf = skb;

/* printk(KERN_ERR "%s:%d:%s:dump\n", __FILE__, __LINE__, __FUNCTION__); */
/* printk(KERN_ERR "skbf->data = %p\n", skbf->data); */
/* printk(KERN_ERR "iphdr = %p\n", skbf->nh.iph); */
/* printk(KERN_ERR "iphdr->daddr = %p\n", &skbf->nh.iph->daddr); */
/* printk(KERN_ERR "offset = %d\n", (int)&skbf->nh.iph->daddr - (int)skbf->nh.iph); */
/* printk(KERN_ERR "size = %d\n", sizeof(skbf->nh.iph->daddr)); */
/* printk(KERN_ERR "%s:%d:%s:skb_copy_expand\n", __FILE__, __LINE__, __FUNCTION__); */
/* iphdr_dump(skbf->nh.iph); */

#endif /* for DIGA @yoshino */


  /* Ether header 抜き取り */
  data = skb_pull(skbf,ETHER_HEADER_LEN);
  if(data == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : skb_pull!!");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "error : skb_pull!!");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }

  /* sk_buff -> pkbuff 変換 */

/* TEST */
/* printk(KERN_ERR "%s:%d:%s:before ipsec\n", __FILE__, __LINE__, __FUNCTION__); */
/* iphdr_dump(skbf->nh.iph); */

  SetPktBFromSkbuff(&pktb,skbf);

  /* 暗号化処理  */
  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  rtn = ipsec_output(pAdapt,&pktb);
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);
  if(rtn < 0 ){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec_output = %d\n",rtn);
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ipsec_output = %d\n",rtn);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }
 
  /* pkbuff -> sk_buff 変換 */
  SetSkbFromPktbuff(&pktb,skbf);

/* TEST */
/* printk(KERN_ERR "%s:%d:%s:before Nat_Trav_handle\n", __FILE__, __LINE__, __FUNCTION__); */
/* iphdr_dump(skbf->nh.iph); */

  /* NAT traversal処理 */
  rtn = Nat_Trav_handle(skbf);
  if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "Nat_Trav_handle error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "Nat_Trav_handle error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }

/* TEST */
/* printk(KERN_ERR "%s:%d:%s:after Nat_Trav_handle\n", __FILE__, __LINE__, __FUNCTION__); */
/* iphdr_dump(skbf->nh.iph); */

  /* 実デバイス情報取得 */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
    TNC_LOGOUT("realdev->name:%s in VLan_xmit \n",realdev->name);
#else   /* BUILD_ANDROID */
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__dev_get_by_name error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__dev_get_by_name error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }

  /* rtable構造体取得 */
/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  memset(&flowi_tnc, 0, sizeof(flowi_tnc));
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
  flowi_tnc.flowi_oif = realdev->iflink;
#else /* BUILD_ICS */
  flowi_tnc.oif = realdev->iflink;
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */

#ifdef IP_STB
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  memcpy(&iphdr, (struct iphdr *)skbf->data, sizeof(iphdr));    
#else   /* BUILD_ANDROID */
  memcpy(&iphdr, skbf->nh.iph, sizeof(iphdr));
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
  flowi_tnc.u.ip4.daddr = iphdr.daddr;
#else /* BUILD_ICS */
  flowi_tnc.nl_u.ip4_u.daddr = iphdr.daddr;
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("iphdr.daddr:%x \n",iphdr.daddr);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
  flowi_tnc.u.ip4.saddr = iphdr.saddr;    
#else /* BUILD_ICS */
  flowi_tnc.nl_u.ip4_u.saddr = iphdr.saddr;
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("iphdr.saddr:%x \n",iphdr.saddr);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
  flowi_tnc.flowi_tos = RT_TOS(iphdr.tos);
#else /* BUILD_ICS */
  flowi_tnc.nl_u.ip4_u.tos = RT_TOS(iphdr.tos);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */
#else /* !IP_STB */
  flowi_tnc.nl_u.ip4_u.daddr = skbf->nh.iph->daddr;
  flowi_tnc.nl_u.ip4_u.saddr = skbf->nh.iph->saddr;
  flowi_tnc.nl_u.ip4_u.tos = RT_TOS(skbf->nh.iph->tos);
#endif /* !IP_STB */

/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
  route = ip_route_output_key(dev_net(dev),&(flowi_tnc.u.ip4));
#else /* BUILD_ICS */
  int ret = ip_route_output_key(dev_net(dev),&route,&flowi_tnc);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */

#ifdef BUILD_ICS
  TNC_LOGOUT("route:%p \n",route);
#else /* BUILD_ICS */
  TNC_LOGOUT("ret:%d \n",ret);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
// PMC-Viana-Nemo-IT-004 mod start
  if(IS_ERR(route)){
// PMC-Viana-Nemo-IT-004 mod end
#else /* BUILD_ICS */
  if(ret){
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */
#else   /* BUILD_ANDROID */
  if (ip_route_output_key(&route,&flowi_tnc)) {
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ip_route_output_key error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ip_route_output_key error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }
#else /* !TNC_KERNEL_2_6 */
  if (ip_route_output(&route,skbf->nh.iph->daddr,skbf->nh.iph->saddr,
		      RT_TOS(skbf->nh.iph->tos),realdev->iflink)) {
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ip_route_output error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "ip_route_output error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }
#endif /* !TNC_KERNEL_2_6 */


  /* dst_entry構造体取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  dst_release(skb_dst(skbf));
/* PMC-Viana-026-NEMO組み込み対応 modify start */
#ifdef BUILD_ICS
  skb_dst_set(skbf, &route->dst);
#else /* BUILD_ICS */
  skb_dst_set(skbf, &route->u.dst);
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 modify end */
  (skb_dst(skbf))->dev = realdev;
#else   /* BUILD_ANDROID */
  dst_release(skbf->dst);
  skbf->dst = &route->u.dst;
  skbf->dst->dev = realdev;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add start */

/* PMC-Viana-007-CONFIG_NETFILTER削除対応 add start */
#ifndef BUILD_ANDROID
#ifdef CONFIG_NETFILTER
  nf_conntrack_put(skb->nfct);
  skb->nfct = NULL;
#ifdef CONFIG_NETFILTER_DEBUG
  skb->nf_debug = 0;
#endif
#endif
#endif  /* BUILD_ANDROID */
/* PMC-Viana-007-CONFIG_NETFILTER削除対応応 add end */

  /* destructor 差し替え */
  if(skbf->destructor){
    if(pAdapt->org_destructor){
      if(pAdapt->org_destructor != skbf->destructor){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "!!!Two or more destructor exists!!!\n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "!!!Two or more destructor exists!!!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	goto error;
      }
    }
    pAdapt->org_destructor = skbf->destructor;
    skbf->destructor = count_check_in_kfree_skb;
  }
  else{
    skbf->destructor = count_check_only_in_kfree_skb;
  }

  /* 統計情報設定 */
  stats->tx_packets++;
  stats->tx_bytes += skbf->len;

  pAdapt->drv_info.sending_count++;

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  rtn = ip_output(skbf);
  /* rtn = ip_queue_xmit(skbf,0); */
  if(rtn < 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : ip_output\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "error : ip_output\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  }
#else 
  rtn = ip_send(skbf);
  if(rtn < 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : ip_send\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "error : ip_send\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  }
#endif
  else{
    DBG_printf("===== send message =====\n");
    if(pAdapt->drv_info.sending_count >= pAdapt->drv_info.max_sending_num){
      netif_stop_queue(dev);
    }
  }

  return 0;

 error:
  kfree_skb(skbf);
  stats->tx_errors++;

  return 0;
}


/* NatTravasal処理
 *
 * 書式   : int Nat_Trav_handle(struct sk_buff *skbf)
 * 引数   : struct sk_buff *skbf : パケット情報
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int Nat_Trav_handle(struct sk_buff *skbf)
{
  int nat_trav_headlen; /* header length of NAT Tarversal */
  struct iphdr *iph;
  struct udphdr *udph;
  u_short sum;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("***Call Nat_Trav_handle***\n");   //tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* check Nat Traversal */
  spin_lock(&pAdapt->NatInfo_SL.NatInfoLock);
  if(pAdapt->NatInfo_SL.nat_info.enable == NAT_TRAV_DISABLE){
    spin_unlock(&pAdapt->NatInfo_SL.NatInfoLock);
    return 0;
  }
  spin_unlock(&pAdapt->NatInfo_SL.NatInfoLock);

#ifdef SET_MARKER
  nat_trav_headlen = sizeof(struct udphdr) + MARKER_LEN;
#else
  nat_trav_headlen = sizeof(struct udphdr);
#endif

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("   before skb_headroom()\n");   // tomoharu
  dump_skb(skbf);   // tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(skb_headroom(skbf) < nat_trav_headlen + sizeof(struct ethhdr)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "no space for Nat Traversal\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "no space for Nat Traversal\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    goto error;
  }
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("   after skb_headroom()\n");   // tomoharu
  dump_skb(skbf);   // tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  /* renew IP header */
  iph = (struct iphdr*)skbf->data;
  skb_push(skbf,nat_trav_headlen);
  memmove(skbf->data, (char*)iph, sizeof(struct iphdr));
  iph = (struct iphdr*)skbf->data;
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  skbf->network_header = (char *)iph;
#else   /* BUILD_ANDROID */
  skbf->nh.iph = iph;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  iph->tot_len = htons(skbf->len);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("* setting iph->protocol = IPPROTO_UDP\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  iph->protocol = IPPROTO_UDP;
  iph->check = 0;
  sum = in_cksum((u_short*)iph,iph->ihl<<2);
  iph->check = htons(sum);

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("   after renewing IP header\n");   // tomoharu
  dump_skb(skbf);   // tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* make UDP header */
  udph = (struct udphdr*)(iph + 1);
#ifdef SET_MARKER
  memset((char*)(udph + 1), 0, MARKER_LEN);
#endif

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  udph->source = htons(pAdapt->NatInfo_SL.nat_info.own_port);
  udph->dest = htons(pAdapt->NatInfo_SL.nat_info.remort_port);
  udph->len  = htons(skbf->len - sizeof(struct iphdr));
  udph->check   = 0;
#if 1 /* M9春CVC検証 UDPchecksum計算対応 @yoshino */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  udph->check = htons(calc_udp_sum((struct ip_udp*)skbf->network_header));
#else   /* BUILD_ANDROID */
  udph->check = htons(calc_udp_sum((struct ip_udp*)skbf->nh.iph));
#endif  /* BUILD_ANDROID */
#endif
#else 
  udph->uh_sport = htons(pAdapt->NatInfo_SL.nat_info.own_port);
  udph->uh_dport = htons(pAdapt->NatInfo_SL.nat_info.remort_port);
  udph->uh_ulen  = htons(skbf->len - sizeof(struct iphdr));
  udph->uh_sum   = 0;
#if 1 /* M9春CVC検証 UDPchecksum計算対応 @yoshino */
  udph->check = htons(calc_udp_sum((struct ip_udp*)skbf->nh.iph));
#endif
#endif

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("   after making UDP header\n");   // tomoharu
  dump_skb(skbf);   // tomoharu
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  return IPSEC_SUCCESS;
  
 error:
  return IPSEC_ERROR;

}

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
// tomoharu
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
    TNC_LOGOUT("message length = %d\n",msglen);
    
    for(i=0; i< msglen; i++){
      sprintf(&tmp[i%16 * 3], "%02x ", (u_char)msg[i]);
      if(i%16 == 15){
	tmp[16*3] = 0x00;
	tmp[8*3-1] = 0x00;
	TNC_LOGOUT("%s   %s\n",tmp,&tmp[8*3]);
      }
    }
    if(i%16 != 0){
      tmp[i%16*3] = 0x00;
      if(i%16 > 8){
	tmp[8*3-1] = 0x00;
	TNC_LOGOUT("%s   %s\n",tmp,&tmp[8*3]);
      }
      else{
	TNC_LOGOUT("%s\n",tmp);
      }
    }
  }
}
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

