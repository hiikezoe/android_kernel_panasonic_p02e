/*
 * �¥ǥХ����ξ��ִƻ����
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>


#include "linux_precomp.h"

extern PADAPT pAdapt;

void dev_check_timeout(unsigned long timeout);

/* �¥ǥХ����ξ��֤�����å�
 *
 * ��   : int check_real_device(struct net_device *realdev)
 * ����   : struct net_device *realdev : �¥ǥХ����ΥǥХ�������
 * ����� : ���� : CHECK_OK
 *          �¥ǥХ�����queue��STOP : REAL_DEV_QUEUE_STOP
 *          �¥ǥХ�����󥯥����� : REAL_DEV_LINK_DOWN 
 */
int check_real_device(struct net_device *realdev)
{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call dev_check_timeout \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  if(netif_queue_stopped(realdev)){
    return REAL_DEV_QUEUE_STOP;
  }
  if(!netif_carrier_ok(realdev)){
    return REAL_DEV_LINK_DOWN;
  }

  return CHECK_OK;
}

/* PMC-Viana-026-NEMO�Ȥ߹����б� delete start*/
#ifndef BUILD_ICS
/* �¥ǥХ����ξ��֤�����å����륿���ޤ�����
 *
 * ��   : void set_timer_realdev_check(unsigned long timeout)
 * ����   : unsigned long timeout : �����ॢ������(msec)
 * ����� : �ʤ�
 *         
 */
void set_timer_realdev_check(unsigned long timeout)
{
  struct timer_list *timer = &pAdapt->timer_info.dev_checkTimer;
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call set_timer_realdev_check timeout=%d \n",timeout);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  timer->function = dev_check_timeout;
  timer->data = timeout;
  timer->expires = jiffies + timeout * HZ / 1000;
   
  add_timer(timer);

  return;
}



/* �¥ǥХ����ξ��֤�ƻ뤹�륿���ॢ���Ƚ���
 *
 * ��   : void dev_check_timeout(unsigned long timeout)
 * ����   : unsigned long timeout : 
 * ����� : �ʤ�
 *  
 */
void dev_check_timeout(unsigned long timeout)
{

  struct net_device *realdev, *virtualdev;
  struct net_device_stats *stats;
  int rtn;
  static int count = 0;

/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID    
    TNC_LOGOUT("Call dev_check_timeout timeout=%d \n",timeout);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

  /* �¥ǥХ����ΥǥХ���������� */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
    //printk("pAdapt->DevName_SL.real_dev_name=%s \n",pAdapt->DevName_SL.real_dev_name);
    //printk("real_dev->name:%s in dev_check_timeout \n",realdev->name);
#else   /* BUILD_ANDROID */
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
    spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_timeout(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_timeout(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	   pAdapt->DevName_SL.real_dev_name);
    spin_unlock(&pAdapt->DevName_SL.DevNameLock);
    return ;
  }
  /* ���ۥǥХ����ΥǥХ���������� */
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
   // printk("pAdapt->drv_info.Vlan_name=%s \n",pAdapt->drv_info.Vlan_name);
   // printk("virtualdev->name:%s in dev_check_timeout \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */

  if(virtualdev == NULL){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(v_dev) error in dev_check_timeout(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(v_dev) error in dev_check_timeout(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	   pAdapt->drv_info.Vlan_name);
    return ;
  }

  /* �¥ǥХ����ξ��֤�����å� */
  rtn = check_real_device(realdev);
  if(rtn == CHECK_OK){
    DBG_printf("check ok in timer count(%d)\n",count);
    if(pAdapt->timer_info.pool_skb){
      /* ���׾������� */
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
      stats = netdev_priv(virtualdev);
#else   /* BUILD_ANDROID */
      stats = virtualdev->priv;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
      stats->tx_packets++;
      stats->tx_bytes += pAdapt->timer_info.pool_skb->len;

      /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
      ip_queue_xmit(pAdapt->timer_info.pool_skb);
#else   /* BUILD_ANDROID */
      ip_queue_xmit(pAdapt->timer_info.pool_skb,0);
      /* ip_output(pAdapt->timer_info.pool_skb); */
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
#else 
      ip_send(pAdapt->timer_info.pool_skb);
#endif

      pAdapt->timer_info.pool_skb = NULL;
      DBG_printf("===== send message =====\n");
    }
    count = 0;
    netif_wake_queue(virtualdev);
    netif_carrier_on(virtualdev);
    goto NoSet;
  }
  else if( rtn == REAL_DEV_QUEUE_STOP){
    /* �¥ǥХ�����queue��stop�ξ��
       ���ۥǥХ�����queue��stop�ˤ��� */
    netif_stop_queue(virtualdev);
  }
  else{
    /* �¥ǥХ�������󥯥�����ΤȤ�
       ���ۥǥХ������󥯥�����ˤ��� */
    netif_carrier_off(virtualdev);
    /* carrier off �ˤ��Ƥ�ѥ��åȤ�����Ƥ���Τ�
       queue��stop�ˤ��� */
    netif_stop_queue(virtualdev);
  }
 
  set_timer_realdev_check(timeout);
  count ++;

  if(pAdapt->timer_info.pool_skb){
    /* �������ʾ��ݻ��������drop���� */
    if(count > pAdapt->timer_info.timeout_info.timeout_count){
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
      ((struct net_device_stats*)netdev_priv(virtualdev))->tx_dropped++;
#else   /* BUILD_ANDROID */
      ((struct net_device_stats*)virtualdev->priv)->tx_dropped++;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
      kfree_skb(pAdapt->timer_info.pool_skb);
      pAdapt->timer_info.pool_skb = NULL;    
      DBG_printf("pool skb dropped (count = %d)\n",count);
    }
  }

NoSet:
  return ;
}
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO�Ȥ߹����б� delete end */



/* �̿��¥ǥХ�����queue�ξ��֤�����å�����
 *
 * ��   : void dev_check_only_in_kfree_skb(struct sk_buff *skb)
 * ����   : struct sk_buff *skb : �ѥ��åȾ���
 * ����� : �ʤ�
 *  
 */
void dev_check_only_in_kfree_skb(struct sk_buff *skb)
{

  struct net_device *realdev, *virtualdev;
  int rtn;
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call dev_check_only_in_kfree_skb \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  /* �¥ǥХ����ΥǥХ���������� */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
//    printk("realdev->name:%s in dev_check_only_in_kfree_skb \n",realdev->name);
#else   /* BUILD_ANDROID */
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
    spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_only_in_kfree_skb(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_only_in_kfree_skb(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	   pAdapt->DevName_SL.real_dev_name);
    spin_unlock(&pAdapt->DevName_SL.DevNameLock);
    return ;
  }
  /* ���ۥǥХ����ΥǥХ���������� */
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//     printk("virtualdev->name:%s in dev_check_only_in_kfree_skb \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
  if(virtualdev == NULL){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name error in dev_check_only_in_kfree_skb(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name error in dev_check_only_in_kfree_skb(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	   pAdapt->drv_info.Vlan_name);
    return ;
  }

  /* �¥ǥХ����ξ��֤�����å� */
  rtn = check_real_device(realdev);
  if(rtn == CHECK_OK){
    netif_wake_queue(virtualdev);
    netif_carrier_on(virtualdev);
    return;
  }
  else if( rtn == REAL_DEV_QUEUE_STOP){
    /* �¥ǥХ�����queue��stop�ξ��
    ���ۥǥХ�����queue��stop�ˤ��� */
    netif_stop_queue(virtualdev);
  }
  else{
    /* �¥ǥХ�������󥯥�����ΤȤ�
    ���ۥǥХ������󥯥�����ˤ��� */
    netif_carrier_off(virtualdev);
    /* carrier off �ˤ��Ƥ�ѥ��åȤ�����Ƥ���Τ�
    queue��stop�ˤ��� */
    netif_stop_queue(virtualdev);
  }
  DBG_printf("set timer in dev_check_in_kfree_skb\n");


  return ;
}


/* �̿��¥ǥХ�����queue�ξ��֤�����å���
 * skb��destructor��ƤӽФ�
 *
 * ��   : void dev_check_in_kfree_skb(struct sk_buff *skb)
 * ����   : struct sk_buff *skb : �ѥ��åȾ���
 * ����� : �ʤ�
 *  
 */
void dev_check_in_kfree_skb(struct sk_buff *skb)
{

  /* �̿��¥ǥХ����Υ����å� */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call dev_check_in_kfree_skb \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  dev_check_only_in_kfree_skb(skb);

  /* destructor */
  pAdapt->org_destructor(skb);


}


/* �����������󥿥����å�
 *
 * ��   : void count_check_only_in_kfree_skb(struct sk_buff *skb)
 * ����   : struct sk_buff *skb : �ѥ��åȾ���
 * ����� : �ʤ�
 *  
 */
void count_check_only_in_kfree_skb(struct sk_buff *skb)
{

  struct net_device *virtualdev;
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID    
    TNC_LOGOUT("Call count_check_only_in_kfree_skb\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  /* ���ۥǥХ����ΥǥХ���������� */
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//     printk("virtualdev->name:%s in count_check_only_in_kfree_skb \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
  if(virtualdev == NULL){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name error in count_check_only_in_kfree_skb(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name error in count_check_only_in_kfree_skb(%s)\n",
#endif /* BUILD_ANDROID */
	   pAdapt->drv_info.Vlan_name);
    return ;
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(" ret1 count_check_only_in_kfree_skb\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  }

  /* �����������󥿤�ǥ�������Ȥ���������������꾯�ʤ�����
     ���ۥǥХ�����queue�򥹥����Ȥ��� */
  pAdapt->drv_info.sending_count--;
  if(pAdapt->drv_info.sending_count < pAdapt->drv_info.max_sending_num){
    netif_wake_queue(virtualdev);
  } 

/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(" ret2 count_check_only_in_kfree_skb\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
}


/* �����������󥿥����å���
 * skb��destructor��ƤӽФ�
 *
 * ��   : void count_check_in_kfree_skb(struct sk_buff *skb)
 * ����   : struct sk_buff *skb : �ѥ��åȾ���
 * ����� : �ʤ�
 *  
 */
void count_check_in_kfree_skb(struct sk_buff *skb)
{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */  
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call count_check_in_kfree_skb \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  /* �����楫���󥿥����å� */
  count_check_only_in_kfree_skb(skb);

  
  /* destructor�θƤӽФ� */
  pAdapt->org_destructor(skb);
  

}

/*
 *
 *
 *
 */
void realdev_link_check(unsigned long timeout)
{

  struct net_device *realdev,*virtualdev;

/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call realdev_link_check timeout=%d \n",timeout);
//    printk("set timeout -> 10 \n");
// kawa android-patch
//    timeout = 10;
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

  /* �¥ǥХ����ΥǥХ���������� */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
//     printk("realdev->name:%s in realdev_link_check \n",realdev->name);
#else
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif
/* PMC-Viana-001-Android_Build�б� add end */
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
    spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in realdev_link_check(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	   pAdapt->DevName_SL.real_dev_name);
    spin_unlock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("ret1 realdev_link_check  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    return ;
  }
  /* ���ۥǥХ����ΥǥХ���������� */
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//     printk("virtualdev->name:%s in realdev_link_check \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add emd */
  if(virtualdev == NULL){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
	   pAdapt->drv_info.Vlan_name);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("ret2 realdev_link_check  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    return ;
  }
/* PMC-Viana-015-netif_carrier_ok_�����å������б� add start */
#ifndef BUILD_ANDROID
  /* �¥ǥХ�����Link���֤�����å� */
  if(netif_carrier_ok(realdev)){
    netif_carrier_on(virtualdev);
  }
  else{
    netif_carrier_off(virtualdev);
  }
#endif /* BUILD_ANDROID */
/* PMC-Viana-015-netif_carrier_ok_�����å������б� add end */

  pAdapt->timer_info.link_check_timer.data = timeout;
  pAdapt->timer_info.link_check_timer.expires = jiffies + timeout * HZ;
  add_timer(&pAdapt->timer_info.link_check_timer);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("ret3 realdev_link_check  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  return;
}

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
EXPORT_SYMBOL(count_check_in_kfree_skb);
EXPORT_SYMBOL(count_check_only_in_kfree_skb);
#endif
