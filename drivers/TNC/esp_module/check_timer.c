/*
 * 実デバイスの状態監視処理
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>


#include "linux_precomp.h"

extern PADAPT pAdapt;

void dev_check_timeout(unsigned long timeout);

/* 実デバイスの状態をチェック
 *
 * 書式   : int check_real_device(struct net_device *realdev)
 * 引数   : struct net_device *realdev : 実デバイスのデバイス情報
 * 戻り値 : 成功 : CHECK_OK
 *          実デバイスのqueueがSTOP : REAL_DEV_QUEUE_STOP
 *          実デバイスリンクダウン : REAL_DEV_LINK_DOWN 
 */
int check_real_device(struct net_device *realdev)
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call dev_check_timeout \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(netif_queue_stopped(realdev)){
    return REAL_DEV_QUEUE_STOP;
  }
  if(!netif_carrier_ok(realdev)){
    return REAL_DEV_LINK_DOWN;
  }

  return CHECK_OK;
}

/* PMC-Viana-026-NEMO組み込み対応 delete start*/
#ifndef BUILD_ICS
/* 実デバイスの状態をチェックするタイマを設定
 *
 * 書式   : void set_timer_realdev_check(unsigned long timeout)
 * 引数   : unsigned long timeout : タイムアウト値(msec)
 * 戻り値 : なし
 *         
 */
void set_timer_realdev_check(unsigned long timeout)
{
  struct timer_list *timer = &pAdapt->timer_info.dev_checkTimer;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call set_timer_realdev_check timeout=%d \n",timeout);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  timer->function = dev_check_timeout;
  timer->data = timeout;
  timer->expires = jiffies + timeout * HZ / 1000;
   
  add_timer(timer);

  return;
}



/* 実デバイスの状態を監視するタイムアウト処理
 *
 * 書式   : void dev_check_timeout(unsigned long timeout)
 * 引数   : unsigned long timeout : 
 * 戻り値 : なし
 *  
 */
void dev_check_timeout(unsigned long timeout)
{

  struct net_device *realdev, *virtualdev;
  struct net_device_stats *stats;
  int rtn;
  static int count = 0;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID    
    TNC_LOGOUT("Call dev_check_timeout timeout=%d \n",timeout);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  /* 実デバイスのデバイス情報取得 */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
    //printk("pAdapt->DevName_SL.real_dev_name=%s \n",pAdapt->DevName_SL.real_dev_name);
    //printk("real_dev->name:%s in dev_check_timeout \n",realdev->name);
#else   /* BUILD_ANDROID */
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
    spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_timeout(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_timeout(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->DevName_SL.real_dev_name);
    spin_unlock(&pAdapt->DevName_SL.DevNameLock);
    return ;
  }
  /* 仮想デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
   // printk("pAdapt->drv_info.Vlan_name=%s \n",pAdapt->drv_info.Vlan_name);
   // printk("virtualdev->name:%s in dev_check_timeout \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

  if(virtualdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(v_dev) error in dev_check_timeout(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(v_dev) error in dev_check_timeout(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->drv_info.Vlan_name);
    return ;
  }

  /* 実デバイスの状態をチェック */
  rtn = check_real_device(realdev);
  if(rtn == CHECK_OK){
    DBG_printf("check ok in timer count(%d)\n",count);
    if(pAdapt->timer_info.pool_skb){
      /* 統計情報設定 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
      stats = netdev_priv(virtualdev);
#else   /* BUILD_ANDROID */
      stats = virtualdev->priv;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
      stats->tx_packets++;
      stats->tx_bytes += pAdapt->timer_info.pool_skb->len;

      /* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
      ip_queue_xmit(pAdapt->timer_info.pool_skb);
#else   /* BUILD_ANDROID */
      ip_queue_xmit(pAdapt->timer_info.pool_skb,0);
      /* ip_output(pAdapt->timer_info.pool_skb); */
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
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
    /* 実デバイスのqueueがstopの場合
       仮想デバイスもqueueをstopにする */
    netif_stop_queue(virtualdev);
  }
  else{
    /* 実デバイスがリンクダウンのとき
       仮想デバイスもリンクダウンにする */
    netif_carrier_off(virtualdev);
    /* carrier off にしてもパケットが落ちてくるので
       queueもstopにする */
    netif_stop_queue(virtualdev);
  }
 
  set_timer_realdev_check(timeout);
  count ++;

  if(pAdapt->timer_info.pool_skb){
    /* 指定回数以上保持した場合dropする */
    if(count > pAdapt->timer_info.timeout_info.timeout_count){
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
      ((struct net_device_stats*)netdev_priv(virtualdev))->tx_dropped++;
#else   /* BUILD_ANDROID */
      ((struct net_device_stats*)virtualdev->priv)->tx_dropped++;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
      kfree_skb(pAdapt->timer_info.pool_skb);
      pAdapt->timer_info.pool_skb = NULL;    
      DBG_printf("pool skb dropped (count = %d)\n",count);
    }
  }

NoSet:
  return ;
}
#endif /* BUILD_ICS */
/* PMC-Viana-026-NEMO組み込み対応 delete end */



/* 通信実デバイスのqueueの状態をチェックする
 *
 * 書式   : void dev_check_only_in_kfree_skb(struct sk_buff *skb)
 * 引数   : struct sk_buff *skb : パケット情報
 * 戻り値 : なし
 *  
 */
void dev_check_only_in_kfree_skb(struct sk_buff *skb)
{

  struct net_device *realdev, *virtualdev;
  int rtn;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call dev_check_only_in_kfree_skb \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* 実デバイスのデバイス情報取得 */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
//    printk("realdev->name:%s in dev_check_only_in_kfree_skb \n",realdev->name);
#else   /* BUILD_ANDROID */
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
    spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_only_in_kfree_skb(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in dev_check_only_in_kfree_skb(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->DevName_SL.real_dev_name);
    spin_unlock(&pAdapt->DevName_SL.DevNameLock);
    return ;
  }
  /* 仮想デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//     printk("virtualdev->name:%s in dev_check_only_in_kfree_skb \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  if(virtualdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name error in dev_check_only_in_kfree_skb(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name error in dev_check_only_in_kfree_skb(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->drv_info.Vlan_name);
    return ;
  }

  /* 実デバイスの状態をチェック */
  rtn = check_real_device(realdev);
  if(rtn == CHECK_OK){
    netif_wake_queue(virtualdev);
    netif_carrier_on(virtualdev);
    return;
  }
  else if( rtn == REAL_DEV_QUEUE_STOP){
    /* 実デバイスのqueueがstopの場合
    仮想デバイスもqueueをstopにする */
    netif_stop_queue(virtualdev);
  }
  else{
    /* 実デバイスがリンクダウンのとき
    仮想デバイスもリンクダウンにする */
    netif_carrier_off(virtualdev);
    /* carrier off にしてもパケットが落ちてくるので
    queueもstopにする */
    netif_stop_queue(virtualdev);
  }
  DBG_printf("set timer in dev_check_in_kfree_skb\n");


  return ;
}


/* 通信実デバイスのqueueの状態をチェックと
 * skbのdestructorを呼び出す
 *
 * 書式   : void dev_check_in_kfree_skb(struct sk_buff *skb)
 * 引数   : struct sk_buff *skb : パケット情報
 * 戻り値 : なし
 *  
 */
void dev_check_in_kfree_skb(struct sk_buff *skb)
{

  /* 通信実デバイスのチェック */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call dev_check_in_kfree_skb \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  dev_check_only_in_kfree_skb(skb);

  /* destructor */
  pAdapt->org_destructor(skb);


}


/* 送信数カウンタチェック
 *
 * 書式   : void count_check_only_in_kfree_skb(struct sk_buff *skb)
 * 引数   : struct sk_buff *skb : パケット情報
 * 戻り値 : なし
 *  
 */
void count_check_only_in_kfree_skb(struct sk_buff *skb)
{

  struct net_device *virtualdev;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID    
    TNC_LOGOUT("Call count_check_only_in_kfree_skb\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* 仮想デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//     printk("virtualdev->name:%s in count_check_only_in_kfree_skb \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
  if(virtualdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name error in count_check_only_in_kfree_skb(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name error in count_check_only_in_kfree_skb(%s)\n",
#endif /* BUILD_ANDROID */
	   pAdapt->drv_info.Vlan_name);
    return ;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(" ret1 count_check_only_in_kfree_skb\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  }

  /* 送信数カウンタをディクリメントし、最大送信数より少ない場合は
     仮想デバイスのqueueをスタートする */
  pAdapt->drv_info.sending_count--;
  if(pAdapt->drv_info.sending_count < pAdapt->drv_info.max_sending_num){
    netif_wake_queue(virtualdev);
  } 

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(" ret2 count_check_only_in_kfree_skb\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
}


/* 送信数カウンタチェックと
 * skbのdestructorを呼び出す
 *
 * 書式   : void count_check_in_kfree_skb(struct sk_buff *skb)
 * 引数   : struct sk_buff *skb : パケット情報
 * 戻り値 : なし
 *  
 */
void count_check_in_kfree_skb(struct sk_buff *skb)
{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */  
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call count_check_in_kfree_skb \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* 送信中カウンタチェック */
  count_check_only_in_kfree_skb(skb);

  
  /* destructorの呼び出し */
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

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call realdev_link_check timeout=%d \n",timeout);
//    printk("set timeout -> 10 \n");
// kawa android-patch
//    timeout = 10;
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  /* 実デバイスのデバイス情報取得 */
  spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  realdev = __dev_get_by_name(&init_net,pAdapt->DevName_SL.real_dev_name);
//     printk("realdev->name:%s in realdev_link_check \n",realdev->name);
#else
  realdev = __dev_get_by_name(pAdapt->DevName_SL.real_dev_name);
#endif
/* PMC-Viana-001-Android_Build対応 add end */
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);
  if(realdev == NULL){
    spin_lock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in realdev_link_check(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->DevName_SL.real_dev_name);
    spin_unlock(&pAdapt->DevName_SL.DevNameLock);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("ret1 realdev_link_check  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return ;
  }
  /* 仮想デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  virtualdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//     printk("virtualdev->name:%s in realdev_link_check \n",virtualdev->name);
#else   /* BUILD_ANDROID */
  virtualdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add emd */
  if(virtualdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#else /* BUILD_ANDROID */
    printk(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->drv_info.Vlan_name);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("ret2 realdev_link_check  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return ;
  }
/* PMC-Viana-015-netif_carrier_ok_チェック外し対応 add start */
#ifndef BUILD_ANDROID
  /* 実デバイスのLink状態をチェック */
  if(netif_carrier_ok(realdev)){
    netif_carrier_on(virtualdev);
  }
  else{
    netif_carrier_off(virtualdev);
  }
#endif /* BUILD_ANDROID */
/* PMC-Viana-015-netif_carrier_ok_チェック外し対応 add end */

  pAdapt->timer_info.link_check_timer.data = timeout;
  pAdapt->timer_info.link_check_timer.expires = jiffies + timeout * HZ;
  add_timer(&pAdapt->timer_info.link_check_timer);
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("ret3 realdev_link_check  \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  return;
}

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
EXPORT_SYMBOL(count_check_in_kfree_skb);
EXPORT_SYMBOL(count_check_only_in_kfree_skb);
#endif
