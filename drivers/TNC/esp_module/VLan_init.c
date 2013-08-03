/*
 * ����NIC�ɥ饤�н��������
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
#include <linux/mii.h>

#include "linux_precomp.h"

extern PADAPT pAdapt;

int Vlan_change_mtu(struct net_device *dev, int new_mtu);
int VLan_xmit(struct sk_buff *skb, struct net_device *dev);
struct net_device_stats *VLan_get_stats(struct net_device *dev);
int Vlan_open(struct net_device *dev);
int Vlan_stop(struct net_device *dev);
int Vlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

int apr_msg_handle(char *data);

/* �ǥХ�����¤�ν�����ؿ�
 *
 * ��   : static int VLan_init(struct net_device *dev)
 * ����   : struct net_device *dev : �ǥХ�����¤��
 * ����� : ���� : 0
 *          ������­ : -ENOMEM 
 */
/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6

static void VLan_init(struct net_device *dev)
{
/* PMC-Viana-029 Quad�Ȥ߹����б� add */
  struct net_device_stats *priv;
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
static const struct net_device_ops vlan_netdev_ops = {
  .ndo_open		= Vlan_open,
  .ndo_stop		= Vlan_stop,
  .ndo_start_xmit		= VLan_xmit,
  .ndo_get_stats		= VLan_get_stats,
  .ndo_change_mtu		= Vlan_change_mtu,
  .ndo_do_ioctl		= Vlan_ioctl,
};    
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
  /* �ǥХ������׾������� */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT("Call VLan_init \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

/* PMC-Viana-029 Quad�Ȥ߹����б� mod */
//  struct net_device_stats *priv = netdev_priv(dev);
  priv = (struct net_device_stats*)netdev_priv(dev);
  memset(priv, 0, sizeof(struct net_device_stats));  

  /* �ǥХ�����¤������ */
  ether_setup(dev);

/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  dev->netdev_ops = &vlan_netdev_ops;
#else   /* BUILD_ANDROID */
  dev->get_stats = VLan_get_stats;
  dev->hard_start_xmit = VLan_xmit;
  dev->change_mtu = Vlan_change_mtu;
  dev->open = Vlan_open;
  dev->stop = Vlan_stop;
  dev->do_ioctl = Vlan_ioctl;
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */

  dev->flags |= IFF_NOARP;
  dev->flags &= ~IFF_MULTICAST;
  dev->mtu = 1300;
}

#else

static int VLan_init(struct net_device *dev)
{
  /* �ǥХ������׾������� */
  dev->priv = kmalloc(sizeof(struct net_device_stats),GFP_KERNEL);
  if(dev->priv == NULL){
    return -ENOMEM;
  }
  memset(dev->priv, 0, sizeof(struct net_device_stats));


  /* �ǥХ�����¤������ */
  ether_setup(dev);

  dev->get_stats = VLan_get_stats;
  dev->hard_start_xmit = VLan_xmit;
  dev->change_mtu = Vlan_change_mtu;
  dev->open = Vlan_open;
  dev->stop = Vlan_stop;
  dev->do_ioctl = Vlan_ioctl;
 
  dev->flags |= IFF_NOARP;
  dev->flags &= ~IFF_MULTICAST;
  dev->mtu = 1300;

  return 0;
}

#endif


/* IO control ����
 *
 * ��   : int Vlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
 * ����   : struct net_device *dev : �ǥХ�����¤��
 *          strcut ifreq *ifr : 
 *          int cmd : command
 * ����� : ���� : 0
 *          ̵���ʰ��� : -EINVAL
 */
int Vlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
  struct mii_ioctl_data *data = (struct mii_ioctl_data *)&ifr->ifr_data;
  int rtn,ret = 0;

  switch(cmd) {
  case SIOCDEVPRIVATE:
    DBG_printf("IO control msg rcv\n");
    /* ���ץꥱ������󤫤�Υ�å����������  */
    rtn = apr_msg_handle((char*)ifr->ifr_data);
    if(rtn == IPSEC_ERROR){
      return -1;
    }
    return 0;
  case SIOCGMIIPHY:		/* Get address of MII PHY in use. */
    data->phy_id = 0xff; /* pgr0007 */
  case SIOCGMIIREG:		/* Read MII PHY register. */
    switch(data->reg_num){
    case MII_BMCR:
      data->val_out = BMCR_FULLDPLX | BMCR_SPEED100;
      break;
      /* link state */
    case MII_BMSR:
      if(netif_carrier_ok(dev)){
	data->val_out = BMSR_LSTATUS;
      }
      break;
    default:
      data->val_out = 0;
      break;
    }
    ret = 0;
    break;
  default:
    ret = -EOPNOTSUPP;
    break;
  }
  
  DBG_printf("ret = %d, cmd = 0x%x, reg_num = %d, data = 0x%x\n",ret,cmd,data->reg_num,data->val_out);
  return ret;
}

/* MTU�ѹ�����
 *
 * ��   : int Vlan_change_mtu(struct net_device *dev, int new_mtu)
 * ����   : struct net_device *dev : �ǥХ�����¤��
 *          int new_mtu : MTU��
 * ����� : ���� : 0
 *          ̵���ʰ��� : -EINVAL
 */
int Vlan_change_mtu(struct net_device *dev, int new_mtu)
{
  if ((new_mtu < 68) || (new_mtu > 1400))
    return -EINVAL;
  dev->mtu = new_mtu;
  return 0;
}


/* �ǥХ��������ץ����
 *
 * ��   : int Vlan_open(struct net_device *dev)
 * ����   : struct net_device *dev : �ǥХ�����¤��
 * ����� : 0
 *           IFNAMSIZ
 */
int Vlan_open(struct net_device *dev)
{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Vlan_open1 \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  netif_stop_queue(dev);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("netif_stop_queue \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  netif_carrier_on(dev);
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("netif_carrier_on \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  
  /* for DIGA @yoshino */
#ifndef TNC_KERNEL_2_6
  MOD_INC_USE_COUNT;
#endif
  return 0;
}


/* �ǥХ�����߽���
 *
 * ��   : int Vlan_stop(struct net_device *dev)
 * ����   : struct net_device *dev : �ǥХ�����¤��
 * ����� : 0
 *           
 */
int Vlan_stop(struct net_device *dev)
{
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Vlan_stop 1 \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

  netif_stop_queue(dev);

  netif_carrier_off(dev);
  
  /* for DIGA @yoshino */
#ifndef TNC_KERNEL_2_6
  MOD_DEC_USE_COUNT;
#endif
  return 0;
}


/* ���׾�������ؿ�
 *
 * ��   : struct net_device_stats *VLan_get_stats(struct net_device *dev)
 * ����   : struct net_device *dev : �ǥХ�����¤��
 * ����� : ���׾���¤�ΤΥݥ���
 *           
 */

struct net_device_stats *VLan_get_stats(struct net_device *dev)
{
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  return (struct net_device_stats*)netdev_priv(dev);
#else   /* BUILD_ANDROID */
  return (struct net_device_stats*)dev->priv;
#endif  /*  BUILD_ANDROID*/
/* PMC-Viana-001-Android_Build�б� add end */
}



/* �⥸�塼�������ؿ�
 *
 * ��   : static int __init VLan_init_module(void)
 * ����   : �ʤ�
 * ����� : ���� : 0
 *          ���� : ��ο� 
 */

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6

static struct net_device *vlandev;

static int __init VLan_init_module(void)
{
  int rtn;
/* PMC-Viana-018-vlan0��viana0 add start */
#ifdef BUILD_ANDROID
  char name[IFNAMSIZ] = "viana%d";
#else /* BUILD_ANDROID */
  char name[IFNAMSIZ] = "vlan%d";
#endif /* BUILD_ANDROID */
/* PMC-Viana-018-vlan0��viana0 add end */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("VLan_init_module 1 \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */

  vlandev = alloc_netdev(sizeof(struct net_device_stats), name, VLan_init);
  if( vlandev == NULL ){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "alloc_netdev error in VLan_init_module\n");
#else
    printk(KERN_ERR "alloc_netdev error in VLan_init_module\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    return -ENOMEM;
  }

  /* �ǥХ�����Ͽ */
  rtn = register_netdev(vlandev);
  if (rtn < 0){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "register_netdev error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "register_netdev error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    free_netdev(vlandev);
    return rtn;
  }
  DBG_printf("start Virtual Lan I/F(%s)\n", vlandev->name);
  strcpy(pAdapt->drv_info.Vlan_name, vlandev->name);

  return 0;
}

#else 

static struct net_device devVLan;

static int __init VLan_init_module(void)
{
  int rtn;

  memset(&devVLan, 0, sizeof(struct net_device));

  devVLan.init = VLan_init;
  SET_MODULE_OWNER(&devVLan);

  /* �ǥХ���̾���� */
/* PMC-Viana-001-Android_Build�б� add start */
#ifdef BUILD_ANDROID
  rtn = dev_alloc_name(&devVLan,"viana%d");
#else
  rtn = dev_alloc_name(&devVLan,"vlan%d");
#endif /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build�б� add end */
  if(rtn < 0){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "dev_alloc_name error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "dev_alloc_name error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    return rtn;
  }
  dev_hold(&devVLan);

  /* �ǥХ�����Ͽ */
  rtn = register_netdev(&devVLan);
  if (rtn < 0){
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "register_netdev error\n");
#else /* BUILD_ANDROID */
    printk(KERN_ERR "register_netdev error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
    return rtn;
  }

  DBG_printf("start Virtual Lan I/F(%s)\n",devVLan.name);
  strcpy(pAdapt->drv_info.Vlan_name, devVLan.name);

  return 0;
}

#endif


/* �⥸�塼�뽪λ�ؿ�
 *
 * ��   : static void __exit VLan_cleanup_module(void)
 * ����   : �ʤ�
 * ����� : �ʤ�
 *          
 */
/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6

static void __exit VLan_cleanup_module(void)
{
  
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call VLan_cleanup_module \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  unregister_netdev(vlandev);
  del_timer(&pAdapt->timer_info.dev_checkTimer);
  free_netdev(vlandev);

  DBG_printf("stop Virtual I/F\n");

}

#else

static void __exit VLan_cleanup_module(void)
{

/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call VLan_cleanup_module \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-�����ͥ������ͭ��/̵���ڤ��ؤ��б� add end */
  del_timer(&pAdapt->timer_info.dev_checkTimer);
  dev_put(&devVLan);
  unregister_netdev(&devVLan);
  kfree(devVLan.priv);

  memset(&devVLan, 0, sizeof(devVLan));
  devVLan.init = VLan_init;

  DBG_printf("stop Virtual I/F\n");

}

#endif

module_init(VLan_init_module);
module_exit(VLan_cleanup_module);
MODULE_LICENSE("Proprietary");


