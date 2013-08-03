/*
 * Linux IPSec メインヘッダファイル
 *
 */

#include <linux/socket.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <net/ip.h>
/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
#include <linux/udp.h>
#endif

/* PMC-Viana-002-ログ出力対応 add start */
#ifdef BUILD_ANDROID
#include "tnc_common_api.h"
#endif  /* BUILD_ANDROID */
/* PMC-Viana-002-ログ出力対応 mod end */

#define _X86_


typedef void VOID, *PVOID;
typedef int INT;
typedef unsigned long ULONG;
typedef char CHAR, *PCHAR;
typedef int BOOLEAN;
typedef unsigned int UINT;
#define IN

/* PMC-Viana-002-ログ出力対応 add start */
#ifdef BUILD_ANDROID
extern int ipsec_debug;

#define bzero(d, n)    memset((d), 0, (n))
#define bcopy(src, dest, n) memcpy ((dest), (src), (n))

#if  TNC_LOGLEVEL>0
#define TNC_LOGOUT(x,...) TNC_PRINTF(x,## __VA_ARGS__)
#define DBG_ENTER(x) TNC_PRINTF("x \n")
#define DBGPRINT(x,...) TNC_PRINTF(x,## __VA_ARGS__)
#define DbgPrint(x,...) TNC_PRINTF(x,## __VA_ARGS__)
#define DBG_printf(args...) \
   {if (ipsec_debug) \
	TNC_PRINTF(args);}
#else
#define TNC_LOGOUT(x,...)
#define DBG_ENTER(x)
#define DBGPRINT(x,...)
#define DbgPrint(x,...)
#define DBG_printf(args...)
#endif

#else /* BUILD_ANDROID */
#define DBG_ENTER(x)
#define DBGPRINT printk
#define DbgPrint printk

extern int ipsec_debug;
#define DBG_printf(args...) \
   {if (ipsec_debug) \
	printk(args);}
#endif /* BUILD_ANDROID */
/* PMC-Viana-002-ログ出力対応 add end */

#include "ip_p.h"
#include "keydb.h"
#include "ipsec_core.h"
#ifdef KERNEL_NO_TNC_BUILD
#include <dtvrecdd/esp_ioctl_msg.h>
#else
#include "ioctl_msg.h"
#endif

#define DRV_VERSION "1.3"

#define DEFAULT_PORT     500
#define MARKER_LEN       8
#define DEFAULT_TIMEOUT_CYCLE  10 /* msec */
#define DEFAULT_TIMEOUT_COUNT  5
/* #define DEFAULT_REAL_DEV_NAME  "eth0" */
#define DEFAULT_REAL_DEV_NAME  "eth1"

#define MAX_SENT_NUM 50

/* PMC-Viana-019-TNC_パケットフィルタタイミング変更 add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
int linux_ipsec_protocol_init(void);
void linux_ipsec_protocol_exit(void);
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-TNC_パケットフィルタタイミング変更 add end */

typedef struct _NAT_INFO_SL
{
  NAT_INFO nat_info;
  spinlock_t NatInfoLock;
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
}NAT_INFO_SL,*PNAT_INFO_SL;
#else
}__attribute__((packed))NAT_INFO_SL,*PNAT_INFO_SL;
#endif




typedef struct IPSEC_SL_
{
  IPSEC Ipsec;
  unsigned long key_lifetime[KEY_NUM];
  DEC_COUNT dec_count;
  spinlock_t IpsecLock;
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
}IPSEC_SL,*PIPSEC_SL;
#else
}__attribute__((packed))IPSEC_SL,*PIPSEC_SL;
#endif


typedef struct DEV_NAME_SL_
{
  char real_dev_name[DEV_NAME_LEN];
  spinlock_t DevNameLock;
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
}DEV_NAME_SL,*PDEV_NAME_SL;
#else
}__attribute__((packed))DEV_NAME_SL,*PDEV_NAME_SL;
#endif

 

#define TIMER_ON  1
#define TIMER_OFF 0
typedef struct TIMER_INFO_
{
  TIMEOUT_INFO timeout_info;
  int link_check_timer_flag;
  struct timer_list link_check_timer;
  struct timer_list dev_checkTimer;
  struct sk_buff *pool_skb;
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
}TIMER_INFO,*PTIMER_INFO;
#else
}__attribute__((packed))TIMER_INFO,*PTIMER_INFO;
#endif


typedef struct DRV_INFO_
{
  char Vlan_name[DEV_NAME_LEN];
  int udp_check_mode;	
  int sending_count;
  int max_sending_num;
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
}DRV_INFO,*PDRV_INFO;
#else
}__attribute__((packed))DRV_INFO,*PDRV_INFO;
#endif


typedef struct _ADAPT
{
  IPSEC_SL Ipsec_SL;
  NAT_INFO_SL NatInfo_SL;
  DEV_NAME_SL DevName_SL;
  DRV_INFO drv_info;
  DRV_STAT drv_stat;
  TIMER_INFO timer_info;
  void (*org_destructor)(struct sk_buff *skb);
  /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
  struct net_protocol *udp_prot;
#else
  struct inet_protocol *udp_prot;
#endif
#ifdef TNC_KERNEL_2_6
}ADAPT,ADAPTER,*PADAPT,*pADAPTER;
#else
}__attribute__((packed))ADAPT,ADAPTER,*PADAPT,*pADAPTER;
#endif



struct ip_udp {
  volatile struct iphdr iph;
  struct udphdr udph;
}__attribute__((packed));



#define IPSEC_ERROR -1
#define IPSEC_SUCCESS 0
#define CHECK_OK 0
#define REAL_DEV_QUEUE_STOP 1
#define REAL_DEV_LINK_DOWN  2

void hex_dump(char *msg, int msglen);

