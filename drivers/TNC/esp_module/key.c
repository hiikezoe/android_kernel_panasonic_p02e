/*
 * key.c
 *
 */
#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#include "des.h"


/*
 * copy SA values
 * OUT:	0:	success.
 *	!0:	failure.
 *
 */
int
key_setsaval( PIPSEC pIpsec,  struct set_ipsec *set )
{
  PDKEY d_key;
  struct secasvar *sav=pIpsec->def_isr[FIRST_OUT].sav;
  struct sadb_key_d *key;

/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call key_setsaval \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  /* out側の場合(送信鍵の設定) */
  if( set->direction == OUT_KEY ){
    /* 以下 元の論理 */
    if( pIpsec->now_key_no == 0) {
      d_key = &pIpsec->key[0];
      sav->iv = pIpsec->ivbuf[0];
    }
    else { /* second key */
      sav = sav->sa_next->sa_next;
      d_key = &pIpsec->key[1];
      sav->iv = pIpsec->ivbuf[1];
    }
  }
  else{
    /* SPIが同じ場合は上書きする */
    if( sav->sa_next->spi == htonl(set->key.spi)){
      d_key = &pIpsec->key[0];
      sav->iv = pIpsec->ivbuf[0];
    }
    else if(sav->sa_next->sa_next->sa_next->spi == htonl(set->key.spi)){
      sav = sav->sa_next->sa_next;
      d_key = &pIpsec->key[1];
      sav->iv = pIpsec->ivbuf[1];
    }
    else{
      if( sav->sa_next->spi == 0){
	d_key = &pIpsec->key[0];
	sav->iv = pIpsec->ivbuf[0];
      }
      else if(sav->sa_next->sa_next->sa_next->spi == 0){ /* second key */
	sav = sav->sa_next->sa_next;
	d_key = &pIpsec->key[1];
	sav->iv = pIpsec->ivbuf[1];
      }
      else{
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
	TNC_LOGOUT(KERN_ERR "error : rcv sa full\n");
#else /* BUILD_ANDROID */
	printk(KERN_ERR "error : rcv sa full\n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
	return -1;
      }
    }
  }


  dbg_printf(("key set: AH-alg : %d\n",set->AH_algo));
  dbg_printf(("key set: ESP-alg: %d\n",set->ESP_algo));
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifndef BUILD_ANDROID
  dbg_printf(("key set: OUT-SPI: %d\n",ntohl(set->out_key.spi)));
  dbg_printf(("key set: IN-SPI : %d\n",ntohl(set->in_key.spi)));
  dbg_printf(("key set: A-Olen : %d\n",set->out_key.auth_key_len));
  dbg_printf(("key set: A-Ilen : %d\n",set->in_key.auth_key_len));
  dbg_printf(("key set: E-Olen : %d\n",set->out_key.enc_key_len));
  dbg_printf(("key set: E-Ilen : %d\n",set->in_key.enc_key_len));
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
  
  dbg_printf(("<<key set: No.%d Set>>\n",pIpsec->now_key_no));

  if( set->direction == IN_KEY )
    sav = sav->sa_next;

  /* SA initialization */
  sav->replay = NULL;   
  sav->alg_auth = (u_char)set->AH_algo;
  sav->alg_enc =  (u_char)set->ESP_algo;
  sav->flags = SADB_X_EXT_NONE;
  sav->seq = 0;
  sav->spi = htonl(set->key.spi);
  sav->sched = NULL;
  sav->schedlen = 0;
  
  /* Auth key */
  if(set->direction == OUT_KEY)
    key = &d_key->out_akey;
  else
    key = &d_key->in_akey;
  memcpy(key->key, set->key.auth_key_val, set->key.auth_key_len);
  key->sadb_key_len = (u_int16_t)set->key.auth_key_len;
  key->sadb_key_bits = (u_int16_t) set->key.auth_key_len<<3;
  
  key->sadb_key_exttype = 0;
  key->sadb_key_reserved = 0;
  sav->key_auth = (struct sadb_key *)key;
  
  /* Encryption key */
  if(set->direction == OUT_KEY)
    key = &d_key->out_ekey;
  else
    key = &d_key->in_ekey;
  memcpy(key->key, set->key.enc_key_val, set->key.enc_key_len);
  key->sadb_key_len = (u_int16_t)set->key.enc_key_len;
  key->sadb_key_bits = (u_int16_t)set->key.enc_key_len<<3;
  
  key->sadb_key_exttype = 0;
  key->sadb_key_reserved = 0;
  sav->key_enc = (struct sadb_key *)key;
  
  /* set iv */
  if( set->ESP_algo == SADB_X_EALG_AES )
    sav->ivlen = 16;
  else if(set->ESP_algo == SADB_EALG_DESCBC || set->ESP_algo == SADB_EALG_3DESCBC)
    sav->ivlen = IV_LEN;
  else /* set->ESP_algo == SADB_EALG_NULL || set->ESP_algo == SADB_EALG_NONE */
    sav->ivlen = 0;
  
  if(set->direction == OUT_KEY)
    if( set->ESP_algo == SADB_X_EALG_AES )
      RAND_bytes2(sav->iv,sav->ivlen);
    else
      RAND_bytes(sav->iv,sav->ivlen);
  else
    sav->iv = NULL;

 
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
    TNC_LOGOUT("Call key_setsaval2 \n");
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */

  if( set->key_mode == IPSEC_KEY_IKE ) {
    /* Timer Start */
    if(set->direction == OUT_KEY){
      if( pIpsec->now_key_no == 0) {
	if( pIpsec->life_timer[FIRST_OUT] )
	  del_timer(&pIpsec->Key_LifeTimer[FIRST_OUT]);
	pIpsec->life_timer[FIRST_OUT] = 1;
	pIpsec->Key_LifeTimer[FIRST_OUT].expires = jiffies + set->lifeTime * HZ;
	add_timer(&pIpsec->Key_LifeTimer[FIRST_OUT]);
	dbg_printf(("Key-time Timer(%d sec.)\n",set->lifeTime));
      }
      else {
	if( pIpsec->life_timer[SECOND_OUT] )
	  del_timer(&pIpsec->Key_LifeTimer[SECOND_OUT]);
	pIpsec->life_timer[SECOND_OUT] = 1;
	pIpsec->Key_LifeTimer[SECOND_OUT].expires = jiffies + set->lifeTime * HZ;
	add_timer(&pIpsec->Key_LifeTimer[SECOND_OUT]);
	dbg_printf(("Key-time Timer(%d sec.)\n",set->lifeTime));
      }
    }
    else{
      sav=pIpsec->def_isr[FIRST_OUT].sav;
      if(sav->sa_next->spi == set->key.spi){
	if( pIpsec->life_timer[FIRST_IN] )
	  del_timer(&pIpsec->Key_LifeTimer[FIRST_IN]);
	pIpsec->life_timer[FIRST_IN] = 1;
	pIpsec->Key_LifeTimer[FIRST_IN].expires = jiffies + set->lifeTime * HZ;
        add_timer(&pIpsec->Key_LifeTimer[FIRST_IN]);
	dbg_printf(("Key-time Timer(%d sec.)\n",set->lifeTime));
      }
      else{
        if( pIpsec->life_timer[SECOND_IN] )
          del_timer(&pIpsec->Key_LifeTimer[SECOND_IN]);
        pIpsec->life_timer[SECOND_IN] = 1;
        pIpsec->Key_LifeTimer[SECOND_IN].expires = jiffies + set->lifeTime * HZ;
        add_timer(&pIpsec->Key_LifeTimer[SECOND_IN]);
        dbg_printf(("Key-time Timer(%d sec.)\n",set->lifeTime));
      }
    }
  }

#ifdef TNC_TANTAI_TEST
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add start */
#ifdef BUILD_ANDROID
  TNC_LOGOUT(KERN_ERR "Auth-alg : %d\n", set->AH_algo);
  TNC_LOGOUT(KERN_ERR "Enc-alg  : %d\n", set->ESP_algo);
  TNC_LOGOUT(KERN_ERR "SPI      : %d\n", set->key.spi);
  TNC_LOGOUT(KERN_ERR "Auth-len : %d\n", set->key.auth_key_len);
  TNC_LOGOUT(KERN_ERR "Enc-len  : %d\n", set->key.enc_key_len);
  TNC_LOGOUT(KERN_ERR "SA set No. = %d\n\n", pIpsec->now_key_no);
#else /* BUILD_ANDROID */
  printk(KERN_ERR "Auth-alg : %d\n", set->AH_algo);
  printk(KERN_ERR "Enc-alg  : %d\n", set->ESP_algo);
  printk(KERN_ERR "SPI      : %d\n", set->key.spi);
  printk(KERN_ERR "Auth-len : %d\n", set->key.auth_key_len);
  printk(KERN_ERR "Enc-len  : %d\n", set->key.enc_key_len);
  printk(KERN_ERR "SA set No. = %d\n\n", pIpsec->now_key_no);
#endif /* BUILD_ANDROID */
/* PMC-Viana-011-カーネルログ出力有効/無効切り替え対応 add end */
#endif

  return 0;
}

/*
 * to initialize a seed for random()
 */
static u_long
key_random(void)
{
  u_char buf[4];
  u_long ex;
  
  buf[0] = 0;
  RAND_bytes(buf,4);
  memcpy( &ex, buf, 4);

  return(ex);
}

/*
 * allocating new SPI
 * called by key_getspi().
 * OUT:
 *	0:	failure.
 *	others: success.
 */
u_int32_t
key_do_getnewspi()
{
  u_int32_t newspi;
  u_int32_t min, max;

  /* set spi range to allocate */
  min = 0x100;
  max = 0x0fffffff;

  /* init SPI */
  newspi = 0;
  /* generate pseudo-random SPI value ranged. */
  newspi = min + (key_random() % (max - min + 1));

  return(newspi);
}
