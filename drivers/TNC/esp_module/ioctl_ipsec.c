/*
 * I/O control 関連処理
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
#include <linux/skbuff.h>
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */
#include "linux_precomp.h"

extern PADAPT pAdapt;

/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
unsigned int g_viana_packet_state = 0;
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 add end */

int get_ipsec_key(PIPSEC pIpsec, struct set_ipsec* set,unsigned int sa_index);
int set_ipsec_info(PIPSECINFO_SET_MSG set_msg);
int check_real_device(struct net_device *realdev);
void set_timer_realdev_check(unsigned long timeout);
int key_clear(PIPSECINFO_CLEAR_MSG clear_msg);
void realdev_link_check(unsigned long timeout);

/* 鍵情報のチェック
 *
 * 書式   : int ipsec_key_check(PIPSECINFO_SET_MSG set_msg)
 * 引数   : PIPSECINFO_SET_MSG set_msg
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int ipsec_key_check(PIPSECINFO_SET_MSG set_msg)
{
  struct set_ipsec *set = &set_msg->ipsec_info.set;
  int error_code = ERROR_CODE_SUCCESS;
  int error_kind = ERROR_KIND_SUCCESS;
  int rtn = 0;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT("Call ipsec_key_check \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	
  /* メッセージ長が一致しない場合、エラーで返す */
  if(set_msg->msg_hdr.length != 
     sizeof(IPSECINFO_SET_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)set_msg->msg_hdr.length);
    set_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    set_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }
 

  if(set->mode != IPSEC_MODE_TUNNEL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_WARNING "Not Tunnel mode\n");
#else
    printk(KERN_WARNING "Not Tunnel mode\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_IPSEC_MODE;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

  if(set->protocol != IPPROTO_ESP){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_WARNING "Protocol not ESP\n");
#else
    printk(KERN_WARNING "Protocol not ESP\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_IPSEC_PROTOCOL;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "(io_ctl_ipsec) ipsec_key_check set->AH_algo = %d\n",set->AH_algo);
#else
  printk(KERN_ERR "(io_ctl_ipsec) ipsec_key_check set->AH_algo = %d\n",set->AH_algo);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(set->AH_algo != SADB_AALG_MD5HMAC &&
     set->AH_algo != SADB_AALG_SHA1HMAC &&
     /* set->AH_algo != SADB_AALG_DESHMAC && */
     set->AH_algo != SADB_X_AALG_SHA2_224 &&
     set->AH_algo != SADB_X_AALG_SHA2_256 &&
     /* set->AH_algo != SADB_X_AALG_SHA2_384 && */
     /* set->AH_algo != SADB_X_AALG_SHA2_512 && */
     /* set->AH_algo != SADB_X_AALG_MD5 && */
     /* set->AH_algo != SADB_X_AALG_SHA && */
     /* set->AH_algo != SADB_X_AALG_AES && */
     set->AH_algo != SADB_AALG_NONE &&
     set->AH_algo != SADB_X_AALG_NULL &&
     set->AH_algo != SADB_ORG_AALG_NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "AH algo. error (%d)\n",set->AH_algo);
#else
    printk(KERN_ERR "AH algo. error (%d)\n",set->AH_algo);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_AH_ALGO;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

  /* printk(KERN_ERR "SADB_EALG_NULL = %d\n",SADB_EALG_NULL); */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "(io_ctl_ipsec) ipsec_key_check set->ESP_algo = %d\n",set->ESP_algo);
#else
  printk(KERN_ERR "(io_ctl_ipsec) ipsec_key_check set->ESP_algo = %d\n",set->ESP_algo);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(set->ESP_algo != SADB_EALG_NONE &&
     set->ESP_algo != SADB_EALG_DESCBC &&
     set->ESP_algo != SADB_EALG_3DESCBC &&
     set->ESP_algo != SADB_EALG_NULL &&
     /* set->ESP_algo != SADB_EALG_DES_IV64 &&    */
     /* set->ESP_algo != SADB_EALG_RC5 &&         */
     /* set->ESP_algo != SADB_EALG_IDEA &&        */
     /* set->ESP_algo != SADB_EALG_CAST128CBC &&  */
     /* set->ESP_algo != SADB_EALG_BLOWFISHCBC && */
     /* set->ESP_algo != SADB_EALG_3IDEA &&       */
     /* set->ESP_algo != SADB_EALG_DES_IV32 &&    */
     /* set->ESP_algo != SADB_EALG_RC4 &&         */
     set->ESP_algo != SADB_X_EALG_AES){ /* SADB_EALG_AESCBC / SADB_X_EALG_RIJNDAELCBC 含 */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ESP algo. error (%d)\n",set->ESP_algo);
#else
    printk(KERN_ERR "ESP algo. error (%d)\n",set->ESP_algo);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_ESP_ALGO;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

  if(set->key_mode != IPSEC_KEY_MANUAL &&
     set->key_mode != IPSEC_KEY_IKE ){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "key mode is not IKE or MANUAL\n");
#else
    printk(KERN_ERR "key mode is not IKE or MANUAL\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_KEY_MODE;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }


  if(set->key_mode == IPSEC_KEY_IKE && set->lifeTime == 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "key mode is IKE but lifeTime = 0\n");
#else
    printk(KERN_ERR "key mode is IKE but lifeTime = 0\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_LIFETIME;
    error_code = ERROR_CODE_IKE_LIFE_ZERO;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

  if( set->direction != OUT_KEY && set->direction != IN_KEY){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error direction (%d)\n",set->direction);
#else
    printk(KERN_ERR "error direction (%d)\n",set->direction);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_DIRECTION;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  } 

  if( set->key.spi == 0 ){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error spi == 0\n");
#else
    printk(KERN_ERR "error spi == 0\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_SPI;
    error_code = ERROR_CODE_ZERO_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

  if( set->key.spi > 0 && set->key.spi < 256){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error spi 0< %d <256\n",set->key.spi);
#else
    printk(KERN_ERR "error spi 0< %d <256\n",set->key.spi);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_SPI;
    error_code = ERROR_CODE_INVALID_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

  if(set->ip_mask == 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ip mask error\n");
#else
    printk(KERN_ERR "ip mask error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_IP_MASK;
    error_code = ERROR_CODE_ZERO_VAL;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

/* for DIGA @yoshino */
  if((set->dst_ip & set->ip_mask) == 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "dst ip error\n");
#else
    printk(KERN_ERR "dst ip error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    error_kind = ERROR_KIND_DST_IP;
    error_code = ERROR_CODE_MASK_DST_ZERO;
    rtn = IPSEC_ERROR;
    goto check_finish;
  }

check_finish:

  set_msg->msg_hdr.error_info.error_code = error_code;
  set_msg->msg_hdr.error_info.error_kind = error_kind;
  return rtn;

}


/* key情報の削除
 * 
 * 書式   : int key_clear(PIPSECINFO_CLEAR_MSG clear_msg)
 * 引数   : PIPSECINFO_CLEAR_MSG clear_msg : クリアメッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int key_clear(PIPSECINFO_CLEAR_MSG clear_msg)
{
  PIPSEC_SL ipsec_SL = &pAdapt->Ipsec_SL;
  unsigned int spi;
  int rtn = IPSEC_SUCCESS;
  int i;
  struct ipsecrequest *isr_FO,*isr_FI,*isr_SO,*isr_SI;
  int direction;
  
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call key_clear \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が一致しない場合、エラーで返す */
  if(clear_msg->msg_hdr.length != 
     sizeof(IPSECINFO_CLEAR_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)clear_msg->msg_hdr.length);
    clear_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    clear_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }
 
  spi = htonl(clear_msg->key_id_info.spi);
  direction = clear_msg->key_id_info.direction;
  if(spi != 0 && !(direction == OUT_KEY || direction == IN_KEY)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : direction = %d\n",direction); 
#else
    printk(KERN_ERR "error : direction = %d\n",direction); 
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    clear_msg->msg_hdr.error_info.error_code = ERROR_CODE_INVALID_VAL;
    clear_msg->msg_hdr.error_info.error_kind = ERROR_KIND_DIRECTION;
    return IPSEC_ERROR;
  }
  DBG_printf("spi = %d\n",clear_msg->key_id_info.spi);
  spin_lock(&ipsec_SL->IpsecLock);
  isr_FO = &ipsec_SL->Ipsec.def_isr[FIRST_OUT];
  isr_FI = &ipsec_SL->Ipsec.def_isr[FIRST_IN];
  isr_SO = &ipsec_SL->Ipsec.def_isr[SECOND_OUT];
  isr_SI = &ipsec_SL->Ipsec.def_isr[SECOND_IN];
  /* spiが0の場合 全消去 */
  if(spi == 0){
    for(i=0; i<KEY_NUM; i++){
      if( ipsec_SL->Ipsec.life_timer[i] )
	del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[i]);
    }
    isr_FO->sav->spi = 0;
    isr_FI->sav->spi = 0;
    isr_SO->sav->spi = 0;
    isr_SI->sav->spi = 0;
  }/* 1番目の送信鍵情報のspiと一致した場合 */
  else if(isr_FO->sav->spi == spi && 
	  /* tun_dstとprotocolは打合せにより確認しない 
	     isr_FO->saidx.dst.s_addr == clear_msg->key_id_info.tun_dst &&
	     isr_FO->saidx.proto == clear_msg->key_id_info.protocol*/
	  direction == OUT_KEY){
   /* 現在使用中の送信鍵情報が1番目の送信鍵情報の場合
      もしくは2番目の送信鍵情報が一致した場合
       2番目の送信鍵情報も削除する */
    if(ipsec_SL->Ipsec.now_key_no == 0 ||
       (isr_SO->sav->spi == spi &&
	/* isr_SO->saidx.dst.s_addr == clear_msg->key_id_info.tun_dst &&
	   isr_SO->saidx.proto == clear_msg->key_id_info.protocol)*/
	direction == OUT_KEY)){
	  isr_SO->sav->spi = 0;
      if( ipsec_SL->Ipsec.life_timer[SECOND_OUT] )
	del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[SECOND_OUT]);
    }
    if( ipsec_SL->Ipsec.life_timer[FIRST_OUT] )
      del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[FIRST_OUT]);
    isr_FO->sav->spi = 0;
  }  /* 2番目の送信鍵情報のspiと一致した場合 */
  else if(isr_SO->sav->spi == spi &&
	  /* isr_SO->saidx.dst.s_addr == clear_msg->key_id_info.tun_dst &&
	     isr_SO->saidx.proto == clear_msg->key_id_info.protocol*/
	  direction == OUT_KEY){
    /* 現在使用中の送信鍵情報が2番目の送信鍵情報の場合
       1番目の送信鍵情報も削除する */ 
    if(ipsec_SL->Ipsec.now_key_no == 1){
      isr_FO->sav->spi = 0;
      if( ipsec_SL->Ipsec.life_timer[FIRST_OUT] )
	del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[FIRST_OUT]);
    }
    if( ipsec_SL->Ipsec.life_timer[SECOND_OUT] )
      del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[SECOND_OUT]);
    isr_SO->sav->spi = 0;
  }
  /* 1番目の受信鍵情報のspiと一致した場合 */
  else if(isr_FI->sav->spi == spi &&
	  /* isr_FI->saidx.dst.s_addr == clear_msg->key_id_info.tun_dst &&
	     isr_FI->saidx.proto == clear_msg->key_id_info.protocol*/
	  direction == IN_KEY){
    if(ipsec_SL->Ipsec.life_timer[FIRST_IN])
      del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[FIRST_IN]);
    isr_FI->sav->spi = 0;
  }
  /* 2番目の受信鍵情報のspiと一致した場合 */
  else if(isr_SI->sav->spi == spi &&
	  /* isr_SI->saidx.dst.s_addr == clear_msg->key_id_info.tun_dst &&
	     isr_SI->saidx.proto == clear_msg->key_id_info.protocol*/
	  direction == IN_KEY){
    if(ipsec_SL->Ipsec.life_timer[SECOND_IN])
      del_timer(&ipsec_SL->Ipsec.Key_LifeTimer[SECOND_IN]);
    isr_SI->sav->spi = 0;
  }
  else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "spi(%d) not found\n",ntohl(spi)); 
#else
    printk(KERN_ERR "spi(%d) not found\n",ntohl(spi)); 
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    clear_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
    clear_msg->msg_hdr.error_info.error_kind = ERROR_KIND_SPI;
    rtn = IPSEC_ERROR;
  }
  spin_unlock(&ipsec_SL->IpsecLock);

  return rtn;
}


/* ipsec情報のチェックと設定
 * 
 * 
 * 書式   : int set_ipsec_info(PIPSECINFO_SET_MSG set_msg)
 * 引数   : PIPSECINFO_SET_MSG set_msg : 鍵情報設定メッセージ
 * 戻り値 : 成功 : 0
 *          失敗 : エラーコード
 */
int set_ipsec_info(PIPSECINFO_SET_MSG set_msg)
{
  int rtn;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call set_ipsec_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  rtn = ipsec_key_check(set_msg);
  if(rtn < 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec_key_check error\n");
#else
    printk(KERN_ERR "ipsec_key_check error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return IPSEC_ERROR;
  }
  
  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  rtn = ipsec_key_set(pAdapt, &set_msg->ipsec_info.set);
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);
  if(rtn != IPSEC_SUCCESS){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "ipsec_key_set error\n");
#else
    printk(KERN_ERR "ipsec_key_set error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    set_msg->msg_hdr.error_info.error_code = ERROR_CODE_RCV_SA_FULL;
    set_msg->msg_hdr.error_info.error_kind = ERROR_KIND_RCV_SA;
    return IPSEC_ERROR;
  }

  return IPSEC_SUCCESS;

}


/* key情報の取得
 * 
 * 書式   : int get_ipsec_key(PIPSEC pIpsec, struct set_ipsec* set)
 * 引数   : PIPSEC pIpsec : ipsec構造体
 *        : strcut set_ipsec* set : 鍵情報
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_ipsec_key(PIPSEC pIpsec, struct set_ipsec* set,unsigned int sa_index)
{
  struct ipsecrequest *isr;
  struct secasvar *sav;
  struct sadb_key_d *key;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_ipsec_key \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  isr = &pIpsec->def_isr[sa_index];
  sav = isr->sav;

  if(sav->spi == 0){
    return IPSEC_SUCCESS;
  }
  
  set->mode = isr->saidx.mode;
  set->protocol = isr->saidx.proto;
  set->AH_algo = sav->alg_auth;
  set->ESP_algo = sav->alg_enc;
  set->key_mode = pIpsec->ip4_def_policy.key_mode;
  set->lifeTime = pAdapt->Ipsec_SL.key_lifetime[sa_index];
  set->key.spi = ntohl(sav->spi);
  key = (struct sadb_key_d*)sav->key_auth;
  set->key.auth_key_len = key->sadb_key_len;
  memcpy(set->key.auth_key_val, key->key, key->sadb_key_len);
  key = (struct sadb_key_d*)sav->key_enc;
  set->key.enc_key_len = key->sadb_key_len;
  memcpy(set->key.enc_key_val, key->key, key->sadb_key_len);
  if(sa_index % 2)
    set->direction = IN_KEY;
  else
    set->direction = OUT_KEY;

  set->src_ip = 0;
  if(set->direction == OUT_KEY)
    set->dst_ip = pIpsec->dst_ip;
  else
    set->dst_ip = 0;

  set->ip_mask = pIpsec->dst_mask;
  set->tun_src = isr->saidx.src.s_addr;
  set->tun_dst = isr->saidx.dst.s_addr;

  return IPSEC_SUCCESS;
}

/* ipsec情報の取得
 * 
 * 書式   : int get_ipsec_info(PIPSEC_INFO ipsec_info,unsigned int sa_index)
 * 引数   : PIPSEC_INFO ipsec_info : ipsec情報
 *        : unsigned int sa_index : SAのインデックス番号
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR          
 */
int get_ipsec_info(PIPSEC_INFO ipsec_info,unsigned int sa_index)
{
  int rtn,ret = IPSEC_SUCCESS;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_ipsec_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  memset(ipsec_info, 0, sizeof(IPSEC_INFO));

  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  rtn = get_ipsec_key(&pAdapt->Ipsec_SL.Ipsec,&ipsec_info->set,sa_index);
  if(rtn == IPSEC_ERROR){
    ret = IPSEC_ERROR;
  }
  else if(ipsec_info->set.key.spi != 0){
    if((pAdapt->Ipsec_SL.Ipsec.now_key_no == 0 && sa_index == 2) ||
       (pAdapt->Ipsec_SL.Ipsec.now_key_no == 1 && sa_index == 0)){
      ipsec_info->valid = SA_INVALID;
    }
    else{
      ipsec_info->valid = SA_VALID;
    }
  }
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);
  return ret;
}

/* SAのindex番号取得
 * 
 * 書式   : int GetIndexByKeyIdInfo(PKEY_ID_INFO key_id_info)
 * 引数   : PKEY_ID_INFO key_id_info : 鍵特定情報 
 * 戻り値 : 成功 : SAのindex番号
 *          失敗 : IPSEC_ERROR          
 */
int GetIndexByKeyIdInfo(PKEY_ID_INFO key_id_info)
{
  struct ipsecrequest *isr;

  int index=0,index2=-1;
  int now_key_no;
  
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call GetIndexByKeyIdInfo \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  now_key_no = pAdapt->Ipsec_SL.Ipsec.now_key_no;
  isr = &pAdapt->Ipsec_SL.Ipsec.def_isr[FIRST_OUT];
  do{
    if(isr->sav->spi == htonl(key_id_info->spi) && /* pgr0689 */
       /* isr->saidx.dst.s_addr == key_id_info->tun_dst &&
	  isr->saidx.proto == key_id_info->protocol*/
       ( (index % 2 == 0 && key_id_info->direction == OUT_KEY) ||
         (index % 2 == 1 && key_id_info->direction ==  IN_KEY) ) ){
      /* 有効な鍵が２番目の場合、２番目の鍵もチェックする */
      if(now_key_no == 1 && index == 0){
	index2 = index;
      }
      else{
	break;
      }
    }
    index++;
    isr = isr->next;
  }while(isr);
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);

  if(isr){
    return index;
  }
  else{
    if(index2 != -1)
      return index2;
  }
  return IPSEC_ERROR;

}


/* ipsec情報の通知
 * 
 * 書式   : int ind_ipsec_info(PIPSECINFO_QUERY_MSG query_msg)
 * 引数   : PIPSECINFO_QUERY_MSG query_msg : 
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR          
 */
int ind_ipsec_info(PIPSECINFO_QUERY_MSG query_msg)
{
  int sa_index;
  int msg_size;
  PIPSEC_INFO ipsec_info;
  int rtn;
  int count = 0,sa_c = 0;
  struct ipsecrequest *isr;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call ind_ipsec_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* SAの数をカウント */
  isr = &pAdapt->Ipsec_SL.Ipsec.def_isr[FIRST_OUT];
  do{
    if(isr->sav->spi)
      sa_c++;
    isr = isr->next;
  }while(isr);

  if(query_msg->msg_hdr.length == 0){
    /* 必要なバッファサイズ通知 */
    query_msg->msg_hdr.length = sizeof(IPSECINFO_QUERY_MSG) - sizeof(MSG_HDR) 
      + (sizeof(IPSEC_INFO) * (sa_c - 1));
    return IPSEC_SUCCESS;
  }
  else{
    ipsec_info = query_msg->ipsec_info;
    if(htonl(query_msg->key_id_info.spi) == 0){
      if(query_msg->msg_hdr.length < sizeof(IPSECINFO_QUERY_MSG) 
	 - sizeof(MSG_HDR) + (sizeof(IPSEC_INFO) * (sa_c - 1))){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "msg length too short (%d)\n",
#else
	printk(KERN_ERR "msg length too short (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	       (int)query_msg->msg_hdr.length);    
	query_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_ENOUGH;
	query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
	return IPSEC_ERROR;
      }
    }
    else{
      if(query_msg->msg_hdr.length != 
	 sizeof(IPSECINFO_QUERY_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "msg length not match (%d)\n",
#else
	printk(KERN_ERR "msg length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	       (int)query_msg->msg_hdr.length);    
	query_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
	query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
	return IPSEC_ERROR;
      }
    }
  }

  if(query_msg->key_id_info.spi != 0 && 
     !(query_msg->key_id_info.direction == OUT_KEY || 
       query_msg->key_id_info.direction == IN_KEY)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : direction = %d\n",
#else
    printk(KERN_ERR "error : direction = %d\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   query_msg->key_id_info.direction); 
    query_msg->msg_hdr.error_info.error_code = ERROR_CODE_INVALID_VAL;
    query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_DIRECTION;
    return IPSEC_ERROR;
  }

  /* spiが指定されていた場合 */
  if(htonl(query_msg->key_id_info.spi) != 0){
    sa_index = GetIndexByKeyIdInfo(&query_msg->key_id_info);
    if(sa_index < 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "spi not found(%d)\n",query_msg->key_id_info.spi);
#else
      printk(KERN_ERR "spi not found(%d)\n",query_msg->key_id_info.spi);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      query_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
      query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_SPI;
      return IPSEC_ERROR;
    }
    rtn = get_ipsec_info(ipsec_info,sa_index);
    if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "error : get_ipsec_info\n");
#else
      printk(KERN_ERR "error : get_ipsec_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      query_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
      query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_SPI;
      return IPSEC_ERROR;
    }
    query_msg->ipsec_info_num = 1;
  } /* spiが指定されていない場合 */
  else{
    msg_size = sizeof(IPSECINFO_QUERY_MSG) - sizeof(MSG_HDR) + 
      sizeof(IPSEC_INFO) * (sa_c - 1);
    if(query_msg->msg_hdr.length < msg_size){
      DBG_printf("msg size too short (%d)\n",(int)query_msg->msg_hdr.length);
      query_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_ENOUGH;
      query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
      return IPSEC_ERROR;
    }
    if(sa_c){
      for(sa_index = 0; sa_index < MAX_SA_NUM; sa_index++){
	rtn = get_ipsec_info(ipsec_info,sa_index);
	if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	  TNC_LOGOUT(KERN_ERR "error : get_ipsec_info\n");
#else
	  printk(KERN_ERR "error : get_ipsec_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	  query_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
	  query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_SPI;
	  return IPSEC_ERROR;
	}
	if(ipsec_info->set.key.spi){
	  count++;
	  if(sa_c < count)
	    break;
	  ipsec_info++;
	}
      }
    }
    query_msg->msg_hdr.length = msg_size;
    query_msg->ipsec_info_num = sa_c;
  }
  return IPSEC_SUCCESS;

}

/* アプリケーションへipsec統計情報の送信処理(I/O ctrl)
 *
 * 書式   : int send_stat_info(PIPSECSTAT_GET_MSG get_msg)
 * 引数   : PIPSECSTAT_GET_MSG get_msg : 統計情報取得メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int send_stat_info(PIPSECSTAT_GET_MSG get_msg)
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call send_stat_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(get_msg->msg_hdr.length != 
     sizeof(IPSECSTAT_GET_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",(int)get_msg->msg_hdr.length);
#else
    printk(KERN_ERR "error : length not match (%d)\n",(int)get_msg->msg_hdr.length);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    get_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    get_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }
    
  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  memcpy(&get_msg->ipsec_stat, &pAdapt->Ipsec_SL.Ipsec.ipsec_stat, 
	 sizeof(struct ipsecstat));
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);
  
  return IPSEC_SUCCESS;

}

/* ipsec統計情報のクリア(I/O ctrl)
 *
 * 書式   : int clear_stat_info(PIPSECSTAT_CLEAR_MSG clear_msg)
 * 引数   : PIPSECSTAT_CLEAR_MSG clear_msg : ipsec統計情報クリアメッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int clear_stat_info(PIPSECSTAT_CLEAR_MSG clear_msg)
{

  /* メッセージ長が一致しない場合、エラーで返す */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call clear_stat_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(clear_msg->msg_hdr.length != 
     sizeof(IPSECSTAT_CLEAR_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",(int)clear_msg->msg_hdr.length);
#else
    printk(KERN_ERR "error : length not match (%d)\n",(int)clear_msg->msg_hdr.length);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    clear_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    clear_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }
  
  /* ipsec統計情報をクリア */
  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  memset(&pAdapt->Ipsec_SL.Ipsec.ipsec_stat, 0, sizeof(struct ipsecstat));
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);
  
  return IPSEC_SUCCESS;

}


/* デバッグモード変更
 *
 * 書式   : int change_debug_mode(PDEBUG_MODE_MSG d_mode_msg)
 * 引数   : PDEBUG_MODE_MSG d_mode_msg : デバッグモード変更メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int change_debug_mode(PDEBUG_MODE_MSG d_mode_msg)
{
  /* メッセージ長が一致しない場合、エラーで返す */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call change_debug_mode \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(d_mode_msg->msg_hdr.length != 
     sizeof(DEBUG_MODE_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)d_mode_msg->msg_hdr.length);
    d_mode_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    d_mode_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  /* デバッグモード変更 */
  if(d_mode_msg->debug_mode == DEBUG_ON){
    ipsec_debug = 1;
    DBG_printf("debug mode ON\n");
  }
  else if(d_mode_msg->debug_mode == DEBUG_OFF){
    ipsec_debug = 0;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("debug mode OFF\n");
#else
    printk("debug mode OFF\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  }
  else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : debug mode is invalid value(%d)\n",
#else
    printk(KERN_ERR "error : debug mode is invalid value(%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)d_mode_msg->debug_mode);
    d_mode_msg->msg_hdr.error_info.error_code = ERROR_CODE_INVALID_VAL;
    d_mode_msg->msg_hdr.error_info.error_kind = ERROR_KIND_DEBUG_MODE;
    return IPSEC_ERROR;
  }

  return IPSEC_SUCCESS;

}


/* デバッグモード取得
 *
 * 書式   : int get_debug_mode(PDEBUG_MODE_MSG get_debug_mode)
 * 引数   : PDEBUG_MODE_MSG get_debug_mode : デバッグモード取得メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_debug_mode(PDEBUG_MODE_MSG get_debug_mode)
{
  /* メッセージ長が一致しない場合、エラーで返す */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_debug_mode \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(get_debug_mode->msg_hdr.length != 
     sizeof(DEBUG_MODE_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)get_debug_mode->msg_hdr.length);
    get_debug_mode->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    get_debug_mode->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }
  /* デバッグモード取得 */
  if(ipsec_debug){
    get_debug_mode->debug_mode = DEBUG_ON;
  }
  else{
    get_debug_mode->debug_mode = DEBUG_OFF;
  }

  return IPSEC_SUCCESS;
}


/* タイムアウト値変更
 *
 * 書式   : int change_timeout(PTIMEOUT_SET_MSG t_out_msg)
 * 引数   : PTIMEOUT_SET_MSG t_out_msg : タイムアウト値変更メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int change_timeout(PTIMEOUT_SET_MSG t_out_msg)
{
  /* メッセージ長が一致しない場合、エラーで返す */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call change_timeout \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(t_out_msg->msg_hdr.length != 
     sizeof(TIMEOUT_SET_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length too short (%d)\n",(int)t_out_msg->msg_hdr.length);
#else
    printk(KERN_ERR "error : length too short (%d)\n",(int)t_out_msg->msg_hdr.length);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    t_out_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    t_out_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  /* タイムアウト間隔が0の場合、エラーで返す */
  if(t_out_msg->timeout_info.timeout_cycle == 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "timeout cycle = 0\n");
#else
    printk(KERN_ERR "timeout cycle = 0\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    t_out_msg->msg_hdr.error_info.error_code = ERROR_CODE_ZERO_VAL;
    t_out_msg->msg_hdr.error_info.error_kind = ERROR_KIND_TIMEOUT_CYCLE;
    return IPSEC_ERROR;
  }
  pAdapt->timer_info.timeout_info.timeout_cycle = t_out_msg->timeout_info.timeout_cycle;
  pAdapt->timer_info.timeout_info.timeout_count = t_out_msg->timeout_info.timeout_count;
  
  return IPSEC_SUCCESS;
}


/* タイムアウト値取得
 *
 * 書式   : 
 * 引数   :  : タイムアウト値変更メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_timeout(PTIMEOUT_QUERY_MSG timeout_query_msg)
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_timeout \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が一致しない場合、エラーで返す */
  if(timeout_query_msg->msg_hdr.length != 
     sizeof(TIMEOUT_QUERY_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)timeout_query_msg->msg_hdr.length);
    timeout_query_msg->msg_hdr.error_info.error_code = 
      ERROR_CODE_SIZE_NOT_MATCH;
    timeout_query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  /* timeout情報取得 */
  timeout_query_msg->timeout_info.timeout_cycle = 
    pAdapt->timer_info.timeout_info.timeout_cycle;
  timeout_query_msg->timeout_info.timeout_count = 
    pAdapt->timer_info.timeout_info.timeout_count;
  
  return IPSEC_SUCCESS;
}


/* UDPcheckモード変更
 *
 * 書式   : int change_udp_chk_mode(PUDP_CHK_MODE_MSG udp_chk_mode_msg)
 * 引数   : PUDP_CHK_MODE_MSG udp_chk_mode_msg : UDPcheckモード変更 
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int change_udp_chk_mode(PUDP_CHK_MODE_MSG udp_chk_mode_msg)
{
  /* メッセージ長が一致しない場合、エラーで返す */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call change_udp_chk_mode \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(udp_chk_mode_msg->msg_hdr.length != 
     sizeof(UDP_CHK_MODE_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)udp_chk_mode_msg->msg_hdr.length);
    udp_chk_mode_msg->msg_hdr.error_info.error_code = 
      ERROR_CODE_SIZE_NOT_MATCH;
    udp_chk_mode_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  if(udp_chk_mode_msg->udp_check_mode != NO_CHECK &&
     udp_chk_mode_msg->udp_check_mode != NULL_OK &&
     udp_chk_mode_msg->udp_check_mode != NULL_NG ){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : udp check mode (%d)\n",
#else
    printk(KERN_ERR "error : udp check mode (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)udp_chk_mode_msg->udp_check_mode);
    udp_chk_mode_msg->msg_hdr.error_info.error_code = ERROR_CODE_INVALID_VAL;
    udp_chk_mode_msg->msg_hdr.error_info.error_kind = ERROR_KIND_UDP_CHK_MODE;
    return IPSEC_ERROR;
    
  }
  /* UDPチェックサムのチェック方法を変更 */
  pAdapt->drv_info.udp_check_mode = udp_chk_mode_msg->udp_check_mode;

  return IPSEC_SUCCESS;
}



/* UDPcheckモード取得
 *
 * 書式   : int get_udp_chk_mode(PUDP_CHK_MODE_MSG get_chk_mode_msg)
 * 引数   : PUDP_CHK_MODE_MSG get_chk_mode_msg : UDPcheckモード取得
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_udp_chk_mode(PUDP_CHK_MODE_MSG get_chk_mode_msg)
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_udp_chk_mode \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が一致しない場合、エラーで返す */
  if(get_chk_mode_msg->msg_hdr.length != 
     sizeof(UDP_CHK_MODE_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)get_chk_mode_msg->msg_hdr.length);
    get_chk_mode_msg->msg_hdr.error_info.error_code = 
      ERROR_CODE_SIZE_NOT_MATCH;
    get_chk_mode_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  /* UDPチェックサムのチェック方法を取得 */
  get_chk_mode_msg->udp_check_mode = pAdapt->drv_info.udp_check_mode;

  return IPSEC_SUCCESS;

}

/* 送信処理中パケットの閾値変更
 *
 * 書式   : int change_sending_num(PSENDING_NUM_MSG sending_num_msg)
 * 引数   : PSENDING_NUM_MSG sending_num_msg : 送信処理中パケットの閾値変更メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int change_sending_num(PSENDING_NUM_MSG sending_num_msg)
{
  /* メッセージ長が短い場合、エラーで返す */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call change_sending_num \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  if(sending_num_msg->msg_hdr.length != 
     sizeof(SENDING_NUM_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)sending_num_msg->msg_hdr.length);
    sending_num_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    sending_num_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  if(sending_num_msg->max_sending_num == 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : sending num = 0\n");
#else
    printk(KERN_ERR "error : sending num = 0\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    sending_num_msg->msg_hdr.error_info.error_code = ERROR_CODE_ZERO_VAL;
    sending_num_msg->msg_hdr.error_info.error_kind = ERROR_KIND_SENDING_NUM;
    return IPSEC_ERROR;
  }
  pAdapt->drv_info.max_sending_num = sending_num_msg->max_sending_num;

  return IPSEC_SUCCESS;
  
}

/* 送信処理中パケットの閾値取得
 *
 * 書式   : int get_sending_num(PSENDING_NUM_MSG get_sending_num_msg)
 * 引数   : PSENDING_NUM_MSG get_sending_num_msg : 送信処理中パケット数の閾値取得メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_sending_num(PSENDING_NUM_MSG get_sending_num_msg)
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_sending_num \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が短い場合、エラーで返す */
  if(get_sending_num_msg->msg_hdr.length != 
     sizeof(SENDING_NUM_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)get_sending_num_msg->msg_hdr.length);
    get_sending_num_msg->msg_hdr.error_info.error_code = 
      ERROR_CODE_SIZE_NOT_MATCH;
    get_sending_num_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  get_sending_num_msg->max_sending_num = pAdapt->drv_info.max_sending_num;

  return IPSEC_SUCCESS;
}
/* デバッグ情報の取得
 *
 * 書式   : int get_debug_info(PDEBUG_GET_MSG debug_get_msg)
 * 引数   : PDEBUG_GET_MSG debug_get_msg : デバッグ情報取得メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_debug_info(PDEBUG_GET_MSG debug_get_msg)
{

  struct net_device *vdev;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_debug_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が短い場合、エラーで返す */
  if(debug_get_msg->msg_hdr.length != 
     sizeof(DEBUG_GET_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length too short (%d)\n",
#else
    printk(KERN_ERR "error : length too short (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)debug_get_msg->msg_hdr.length);
    debug_get_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    debug_get_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  /* 通信実デバイスのqueueがstoppedになっているのを検知した回数 */
  debug_get_msg->debug_drv_info.drv_stat.realdev_queue_stop_num = 
    pAdapt->drv_stat.realdev_queue_stop_num;

  spin_lock(&pAdapt->Ipsec_SL.IpsecLock);
  /* 各saで復号化した回数 */
  debug_get_msg->debug_drv_info.dec_count.sa0 = pAdapt->Ipsec_SL.dec_count.sa0;
  debug_get_msg->debug_drv_info.dec_count.sa1 = pAdapt->Ipsec_SL.dec_count.sa1;
  spin_unlock(&pAdapt->Ipsec_SL.IpsecLock);

  /* driver version */
  strcpy(debug_get_msg->debug_drv_info.drv_ver,DRV_VERSION);
  /* skbをコピーした回数 */
  debug_get_msg->debug_drv_info.drv_stat.rcv_skb_copy_count = 
    pAdapt->drv_stat.rcv_skb_copy_count; 

  /* 仮想デバイスのqueue状態 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  vdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//    TNC_LOGOUT("vdev->name:%s in get_debug_info \n",vdev->name);
#else   /* BUILD_ANDROID */
  vdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

  if(vdev == NULL){
    debug_get_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
    debug_get_msg->msg_hdr.error_info.error_kind = ERROR_KIND_VIRTUAL_DEV;
    return IPSEC_ERROR;
  }
  debug_get_msg->debug_drv_info.vdev_queue_stopped = netif_queue_stopped(vdev);
  debug_get_msg->debug_drv_info.current_sending = pAdapt->drv_info.sending_count;
  /* UDP checksum エラーの数 */
  debug_get_msg->debug_drv_info.drv_stat.udp_checksum_error_count =
    pAdapt->drv_stat.udp_checksum_error_count;

  return IPSEC_SUCCESS;
}

/* IPSEC初期化情報設定
 *
 * 書式   : int set_ipsec_init_info(PINIT_INFO_SET_MSG init_set_msg)
 * 引数   : PINIT_INFO_SET_MSG init_set_msg : IPSEC初期設定メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR 
 */
int set_ipsec_init_info(PINIT_INFO_SET_MSG init_set_msg)
{
  PNAT_INFO nat_info = &init_set_msg->init_info.nat_info;
  struct net_device *vdev,*real_dev;
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID  
    TNC_LOGOUT("Call set_ipsec_init_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    
  /* メッセージ長が一致しない場合、エラーで返す */
  if(init_set_msg->msg_hdr.length != 
     sizeof(INIT_INFO_SET_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)init_set_msg->msg_hdr.length);
    init_set_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    init_set_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }
 
  /* check port num */
  if(nat_info->enable == NAT_TRAV_ENABLE){
    if(nat_info->own_port == 0 || nat_info->remort_port == 0){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "port error\n");
#else
      printk(KERN_ERR "port error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      init_set_msg->msg_hdr.error_info.error_kind = 
	(nat_info->own_port == 0) ? ERROR_KIND_OWN_PORT : ERROR_KIND_REMORT_PORT;
      init_set_msg->msg_hdr.error_info.error_code = ERROR_CODE_ZERO_VAL;
      return IPSEC_ERROR;
    }
  }

  /* 通信実デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  real_dev = __dev_get_by_name(&init_net,init_set_msg->init_info.real_dev_name);
//    printk("real_dev->name:%s in set_ipsec_init_info \n",real_dev->name);
#else   /* BUILD_ANDROID */
  real_dev = __dev_get_by_name(init_set_msg->init_info.real_dev_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */

  if(real_dev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(real_dev) error in realdev_link_check(%s)\n",
#else
    printk(KERN_ERR "__get_dev_by_name(real_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   init_set_msg->init_info.real_dev_name);
      init_set_msg->msg_hdr.error_info.error_kind = ERROR_KIND_REAL_DEV;
      init_set_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
    return IPSEC_ERROR;
  }
  /* 仮想デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  vdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//    printk("vdev->name:%s in set_ipsec_init_info \n",vdev->name);
#else
  vdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif
/* PMC-Viana-001-Android_Build対応 add end */

  if(vdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#else
    printk(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->drv_info.Vlan_name);
      init_set_msg->msg_hdr.error_info.error_kind = ERROR_KIND_VIRTUAL_DEV;
      init_set_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
    return IPSEC_ERROR;
  }
 
  spin_lock(&pAdapt->NatInfo_SL.NatInfoLock);
  memcpy(&pAdapt->NatInfo_SL.nat_info, nat_info, sizeof(NAT_INFO));
  spin_unlock(&pAdapt->NatInfo_SL.NatInfoLock);

  spin_lock(&pAdapt->DevName_SL.DevNameLock);
  strcpy(pAdapt->DevName_SL.real_dev_name, init_set_msg->init_info.real_dev_name);
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);

  /* 実デバイスリンク監視タイマ開始 */
  if(pAdapt->timer_info.link_check_timer_flag)
    del_timer(&pAdapt->timer_info.link_check_timer);
  pAdapt->timer_info.link_check_timer_flag = TIMER_ON;
  pAdapt->timer_info.link_check_timer.function = realdev_link_check;
  pAdapt->timer_info.link_check_timer.data = 1;
  pAdapt->timer_info.link_check_timer.expires = jiffies + HZ;
  add_timer(&pAdapt->timer_info.link_check_timer);

  /* 仮想デバイスのqueueをstart */
  netif_wake_queue(vdev);


#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "===== Kernel NAT Info [set_ipsec_init_info] =====\n");
  TNC_LOGOUT(KERN_ERR "nat_info->enable      = %d\n", nat_info->enable);
  TNC_LOGOUT(KERN_ERR "nat_info->own_port    = %d\n", nat_info->own_port);
  TNC_LOGOUT(KERN_ERR "nat_info->remort_port = %d\n", nat_info->remort_port);
  TNC_LOGOUT(KERN_ERR "nat_info->real_dev_name = %s\n\n", init_set_msg->init_info.real_dev_name);
#else
  printk(KERN_ERR "===== Kernel NAT Info [set_ipsec_init_info] =====\n");
  printk(KERN_ERR "nat_info->enable      = %d\n", nat_info->enable);
  printk(KERN_ERR "nat_info->own_port    = %d\n", nat_info->own_port);
  printk(KERN_ERR "nat_info->remort_port = %d\n", nat_info->remort_port);
  printk(KERN_ERR "nat_info->real_dev_name = %s\n\n", init_set_msg->init_info.real_dev_name);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif


  return IPSEC_SUCCESS;
}

/* IPSEC初期化情報の取得
 *
 * 書式   : int get_ipsec_init_info(PINIT_INFO_QUERY_MSG init_info_query_msg)
 * 引数   : PINIT_INFO_QUERY_MSG init_info_query_msg : IPSEC初期化情報取得メッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int get_ipsec_init_info(PINIT_INFO_QUERY_MSG init_info_query_msg)
{

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call get_ipsec_init_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が一致しない場合、エラーで返す */
  if(init_info_query_msg->msg_hdr.length != 
     sizeof(INIT_INFO_QUERY_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)init_info_query_msg->msg_hdr.length);
    init_info_query_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    init_info_query_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }


  spin_lock(&pAdapt->NatInfo_SL.NatInfoLock);
  memcpy(&init_info_query_msg->init_info.nat_info, 
	 &pAdapt->NatInfo_SL.nat_info, sizeof(NAT_INFO));
  spin_unlock(&pAdapt->NatInfo_SL.NatInfoLock);

  spin_lock(&pAdapt->DevName_SL.DevNameLock);
  strcpy(init_info_query_msg->init_info.real_dev_name, 
	 pAdapt->DevName_SL.real_dev_name);
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);


  return IPSEC_SUCCESS;
}

/* IPSEC初期化情報のクリア
 *
 * 書式   : int clear_ipsec_init_info(PINIT_INFO_QUERY_MSG init_info_clear_msg)
 * 引数   : PINIT_INFO_QUERY_MSG init_info_clear_msg : IPSEC初期化情報クリアメッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR
 */
int clear_ipsec_init_info(PINIT_INFO_CLEAR_MSG init_info_clear_msg)
{
  struct net_device *vdev;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call clear_ipsec_init_info \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* メッセージ長が一致しない場合、エラーで返す */
  if(init_info_clear_msg->msg_hdr.length != 
     sizeof(INIT_INFO_CLEAR_MSG) - sizeof(MSG_HDR)){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : length not match (%d)\n",
#else
    printk(KERN_ERR "error : length not match (%d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   (int)init_info_clear_msg->msg_hdr.length);
    init_info_clear_msg->msg_hdr.error_info.error_code = ERROR_CODE_SIZE_NOT_MATCH;
    init_info_clear_msg->msg_hdr.error_info.error_kind = ERROR_KIND_MSG_LEN;
    return IPSEC_ERROR;
  }

  /* 仮想デバイスのデバイス情報取得 */
/* PMC-Viana-001-Android_Build対応 add start */
#ifdef BUILD_ANDROID
  vdev = __dev_get_by_name(&init_net,pAdapt->drv_info.Vlan_name);
//    printk("vdev->name:%s in clear_ipsec_init_info \n",vdev->name);
#else   /* BUILD_ANDROID */
  vdev = __dev_get_by_name(pAdapt->drv_info.Vlan_name);
#endif  /* BUILD_ANDROID */
/* PMC-Viana-001-Android_Build対応 add end */    
    
  if(vdev == NULL){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#else
    printk(KERN_ERR "__get_dev_by_name(v_dev) error in realdev_link_check(%s)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	   pAdapt->drv_info.Vlan_name);
    init_info_clear_msg->msg_hdr.error_info.error_code = ERROR_CODE_NOT_FOUND;
    init_info_clear_msg->msg_hdr.error_info.error_kind = ERROR_KIND_VIRTUAL_DEV;
    return IPSEC_ERROR;
  }
  /* 仮想デバイスのqueueをstop */
  netif_stop_queue(vdev);

#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "TNC_ClearUDPTunnel() [clear_ipsec_init_info()] queue Stoped!!\n");
#else
  printk(KERN_ERR "TNC_ClearUDPTunnel() [clear_ipsec_init_info()] queue Stoped!!\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif

  /* 実デバイスリンク監視タイマ停止 */
  if(pAdapt->timer_info.link_check_timer_flag)
    del_timer(&pAdapt->timer_info.link_check_timer);
  pAdapt->timer_info.link_check_timer_flag = 0;

  spin_lock(&pAdapt->NatInfo_SL.NatInfoLock);
  memset(&pAdapt->NatInfo_SL.nat_info, 0, sizeof(NAT_INFO));
  spin_unlock(&pAdapt->NatInfo_SL.NatInfoLock);

  spin_lock(&pAdapt->DevName_SL.DevNameLock);
  memset(pAdapt->DevName_SL.real_dev_name, 0, DEV_NAME_LEN);
  spin_unlock(&pAdapt->DevName_SL.DevNameLock);


  return IPSEC_SUCCESS;
}



/* アプリケーションからのメッセージ受信処理
 *
 * 書式   : int apr_msg_handle(char *data)
 * 引数   : char *data : アプリケーションからのメッセージ
 * 戻り値 : 成功 : IPSEC_SUCCESS
 *          失敗 : IPSEC_ERROR 
 */
int apr_msg_handle(char *data)
{
  int rtn;
  PMSG_HDR msg_hdr = (PMSG_HDR)data;
  rtn=0;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call apr_msg_handle commnad:%d kind:%d \n", msg_hdr->command,msg_hdr->kind);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  switch(msg_hdr->command){
  case COM_IPSEC_INIT:
    switch(msg_hdr->kind){
    case KIND_SET:
      rtn = set_ipsec_init_info((PINIT_INFO_SET_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "set_ipsec_init_info error\n");
#else
	printk(KERN_ERR "set_ipsec_init_info error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_CLEAR:
      rtn = clear_ipsec_init_info((PINIT_INFO_CLEAR_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "clear_ipsec_init_info error\n");
#else
	printk(KERN_ERR "clear_ipsec_init_info error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = get_ipsec_init_info((PINIT_INFO_QUERY_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : get_drv_info\n");
#else
	printk(KERN_ERR "error : get_drv_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
    }
    break;
  case COM_IPSEC_INFO:
    switch(msg_hdr->kind){
    case KIND_SET:
      rtn = set_ipsec_info((PIPSECINFO_SET_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "set_ipsec_info error\n");
#else
	printk(KERN_ERR "set_ipsec_info error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_CLEAR:
      rtn = key_clear((PIPSECINFO_CLEAR_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "key_clear error\n");
#else
	printk(KERN_ERR "key_clear error\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = ind_ipsec_info((PIPSECINFO_QUERY_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : get_ipsec_info\n");
#else
	printk(KERN_ERR "error : get_ipsec_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
      break;
    }
    break;
  case COM_STAT:
    switch(msg_hdr->kind){
    case KIND_CLEAR:
      rtn = clear_stat_info((PIPSECSTAT_CLEAR_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : clear_stat_info\n");
#else
	printk(KERN_ERR "error : clear_stat_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = send_stat_info((PIPSECSTAT_GET_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : send_stat_info\n");
#else
	printk(KERN_ERR "error : send_stat_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
    }
    break;
  case COM_DEBUG:
    switch(msg_hdr->kind){
    case KIND_SET:
      rtn = change_debug_mode((PDEBUG_MODE_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : change_debug_mode\n");
#else
	printk(KERN_ERR "error : change_debug_mode\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = get_debug_mode((PDEBUG_MODE_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : change_debug_mode\n");
#else
	printk(KERN_ERR "error : change_debug_mode\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
    }
    break;
  case COM_TIMEOUT:
    switch(msg_hdr->kind){
    case KIND_SET:
      rtn = change_timeout((PTIMEOUT_SET_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : change_timeout\n");
#else
	printk(KERN_ERR "error : change_timeout\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = get_timeout((PTIMEOUT_QUERY_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : get_timeout\n");
#else
	printk(KERN_ERR "error : get_timeout\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
    }
    break;
  case COM_UDP_CHK:
    switch(msg_hdr->kind){
    case KIND_SET:
      rtn = change_udp_chk_mode((PUDP_CHK_MODE_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : change_udp_chk_mode\n");
#else
	printk(KERN_ERR "error : change_udp_chk_mode\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = get_udp_chk_mode((PUDP_CHK_MODE_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : get_udp_chk_mode\n");
#else
	printk(KERN_ERR "error : get_udp_chk_mode\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
    }
    break;
  case COM_SENDING_N:
    switch(msg_hdr->kind){
    case KIND_SET:
      rtn = change_sending_num((PSENDING_NUM_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : change_sending_num\n");
#else
	printk(KERN_ERR "error : change_sending_num\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    case KIND_QUERY:
      rtn = get_sending_num((PSENDING_NUM_MSG)data);
      if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : get__sending_num\n");
#else
	printk(KERN_ERR "error : get__sending_num\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return IPSEC_ERROR;
      }
      break;
    default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "unsupported (%d - %d)\n",
#else
      printk(KERN_ERR "unsupported (%d - %d)\n",
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	     (int)msg_hdr->command,(int)msg_hdr->kind);
    }
    break;
  case COM_DEBUG_GET:
    rtn = get_debug_info((PDEBUG_GET_MSG)data);
    if(rtn == IPSEC_ERROR){
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
      TNC_LOGOUT(KERN_ERR "error : get_debug_info\n");
#else
      printk(KERN_ERR "error : get_debug_info\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
      return IPSEC_ERROR;
    }
    break;
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add start */
#ifdef PACKET_FILTER_TIMING_CHANGE
  case COM_VIANA_INIT:
/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 mod start */
    if (g_viana_packet_state == 0)
    {
#ifdef BUILD_ANDROID
      TNC_LOGOUT("linux_ipsec_protocol_init() Call\n");
#else
      printk(KERN_ERR "linux_ipsec_protocol_init() Call\n");
#endif /* BUILD_ANDROID */
      linux_ipsec_protocol_init();
      g_viana_packet_state = 1;
    }
/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 mod end */
    break;
  case COM_VIANA_EXIT:
/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 mod start */
    if (g_viana_packet_state == 1)
    {
#ifdef BUILD_ANDROID
      TNC_LOGOUT("linux_ipsec_protocol_exit() Call\n");
#else
      printk(KERN_ERR "linux_ipsec_protocol_exit() Call\n");
#endif /* BUILD_ANDROID */
      linux_ipsec_protocol_exit();
      g_viana_packet_state = 0;
    }
/* PMC-Viana-019-PT-004 パケットフィルタバグ対応 mod end */
    break;
#endif  /* PACKET_FILTER_TIMING_CHANGE */
/* PMC-Viana-019-パケットフィルタ開始タイミング変更 add end */
  default:
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT(KERN_ERR "error : not support command(%d)\n",(int)msg_hdr->command);
#else
    printk(KERN_ERR "error : not support command(%d)\n",(int)msg_hdr->command);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
    return IPSEC_ERROR;
  }

  return IPSEC_SUCCESS;
}

/* for DIGA @yoshino */
#ifdef TNC_KERNEL_2_6
EXPORT_SYMBOL(apr_msg_handle);
#endif
