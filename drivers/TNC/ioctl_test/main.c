#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>

#include "main.h"
#include "ipsec_key.h"
#include "ioctl_msg.h"

#define IPSEC_PROC_FS_FILE_NAME "/proc/net/ipsec_ctl"
int load_data(void *usedata, FILE *file);
int load_data2(void *usedata, FILE *file);
void output_data(PMSG_HDR msg_hdr);

/* ファイルへ書き込み
 *
 * 書式   : int proc_fs_write(char *data , int datalen, char *filename)
 * 引数   : char *data : ドライバへ送るメッセージ
 *        : int datalen : メッセージ長
 *        : char *filename : procファイル名
 * 戻り値 : 成功 : 0
 *        : 失敗 : -1
 */
int send_msg(char *data)
{
  int rtn=0;
  PMSG_HDR msg_hdr = (PMSG_HDR)data;
  struct ifreq ifr;
  int fd;
  int ret=0;

  fd = socket(AF_INET, SOCK_DGRAM,0);
  if(fd<0){
    printf("socket error\n");
    return -1;
  }
  //msg_hdr->length++;
  ifr.ifr_data = data;
  strcpy(ifr.ifr_name, "vlan0");
  rtn = ioctl(fd, SIOCDEVPRIVATE, &ifr);
  if(rtn < 0 ){
    ret = -1;
    printf("rtn = %d\n",rtn);
  }
  if(msg_hdr->error_info.error_code)
    printf("error : code = %d, kind = %d\n",msg_hdr->error_info.error_code,
	   msg_hdr->error_info.error_kind);

  rtn = close(fd);
  if(rtn < 0){
    ret = -1;
    printf("rtn = %d\n",rtn);
  }

  return ret;
}


/* 鍵設定ファイル読み込み
 *
 * 書式   : int read_conf_file(char *filename, PIPSEC_INFO ipsec_info)
 * 引数   : char *filename : 設定ファイル名
 *        : PIPSEC_INFO ipsec_info : ipsec情報
 * 戻り値 : 成功 : 0
 *        : 失敗 : -1
 */
int read_conf_file(char *filename, PIPSEC_INFO ipsec_info)
{
  FILE *fp;
  int rtn;

  fp = fopen(filename,"r");
  if(fp == NULL){
    printf("fopen error\n");
    return -1;
  }

  rtn = load_data(ipsec_info,fp);
  if(rtn == FALSE ){
    fclose(fp);
    return -1;
  }

  if(fclose(fp)){
    printf("fclose error\n");
    return -1;
  }

  return 0;
}

/* 初期設定ファイル読み込み
 *
 * 書式   : int read_conf_file2(char *filename, PIPSEC_INFO ipsec_info)
 * 引数   : char *filename : 設定ファイル名
 *        : PIPSEC_INFO ipsec_info : ipsec情報
 * 戻り値 : 成功 : 0
 *        : 失敗 : -1
 */
int read_conf_file2(char *filename, PINIT_INFO init_info)
{
  FILE *fp;
  int rtn;

  fp = fopen(filename,"r");
  if(fp == NULL){
    printf("fopen error\n");
    return -1;
  }

  rtn = load_data2(init_info,fp);
  if(rtn == FALSE ){
    fclose(fp);
    return -1;
  }

  if(fclose(fp)){
    printf("fclose error\n");
    return -1;
  }

  return 0;
}


/* メイン関数
 *
 * 書式   : int main(int argc,char* argv[])
 * 引数   : int argc : コマンド引数の数
 *        : char* argv[] : コマンド引数ポインタ配列
 * 戻り値 : 成功 : 0
 *        : 失敗 : -1
 */
int main(int argc,char* argv[])
{
  PIPSECINFO_SET_MSG ipsecinfo_set;
  PIPSECINFO_CLEAR_MSG ipsecinfo_clear;
  PIPSECINFO_QUERY_MSG ipsecinfo_query;
  PIPSECSTAT_GET_MSG ipsecstat_get;
  PIPSECSTAT_CLEAR_MSG ipsecstat_clear;
  PDEBUG_MODE_MSG d_mode_msg;
  PTIMEOUT_SET_MSG timeout_set;
  PTIMEOUT_QUERY_MSG timeout_get;
  PUDP_CHK_MODE_MSG udp_chk_set;
  PUDP_CHK_MODE_MSG udp_chk_get;
  PMSG_HDR msg_hdr;
  PDEBUG_GET_MSG debug_get_msg;
  PSENDING_NUM_MSG sending_num_msg;
  PSENDING_NUM_MSG get_sending_num_msg;
  PINIT_INFO_SET_MSG init_info_set_msg;
  PINIT_INFO_QUERY_MSG init_info_query_msg;
  PINIT_INFO_CLEAR_MSG init_info_clear_msg;
  int datalen = 0;
  char *pname;
  char *data;
  int rtn;


  if(argc < 2 || argc > 5){
  usage:
    pname = strrchr(argv[0],'/') + 1;
    if(!pname) pname = argv[0];
    printf("USAGE : %s -k [filename] (key set)\n"
	   "        %s -c [spi] [1:OUT 2:IN](key clear)\n"
	   "        %s -g (key get)\n"
	   "        %s -G [spi] [1:OUT 2:IN](key get)\n"
	   "        %s -f [filename] (init info set)\n"
	   "        %s -I (init info get)\n"
	   "        %s -C (init info clear)\n"
	   "        %s -s (ipsec stat get)\n"
           "        %s -S (ipsec stat clear)\n"
	   "        %s -d [1:on,2:off](debug mode)\n"
	   "        %s -D (debug mode query)\n"
	   "        %s -t [timer cycle(msec)] [skb timeout count]"
	                  "(timeout info)\n"
	   "        %s -T (timeout info query)\n"
	   "        %s -u [1:no check, 2:null ok, 3:null ng](udp check mode)\n"
	   "        %s -U (udp check mode query)\n"
	   "        %s -i (debug info)\n"
	   "        %s -n [max sending num](MAX sending num)\n"
	   "        %s -N (MAX sending num)\n"
	   ,pname,pname,pname,pname,pname,pname,pname,pname,pname
	   ,pname,pname,pname,pname,pname,pname,pname,pname,pname);
    return 1;
  }
 
  data = malloc(MAXMSGLEN);
  if(data == NULL){
    printf("malloc error\n");
    return -1;
  }
  memset(data, 0, MAXMSGLEN);


  ipsecinfo_set = (PIPSECINFO_SET_MSG)data;
  ipsecinfo_clear =(PIPSECINFO_CLEAR_MSG)data;
  ipsecinfo_query = (PIPSECINFO_QUERY_MSG)data;
  ipsecstat_get = (PIPSECSTAT_GET_MSG)data;
  ipsecstat_clear = (PIPSECSTAT_CLEAR_MSG)data;
  timeout_set = (PTIMEOUT_SET_MSG)data;
  timeout_get = (PTIMEOUT_QUERY_MSG)data;
  udp_chk_set = (PUDP_CHK_MODE_MSG)data;
  udp_chk_get = (PUDP_CHK_MODE_MSG)data;
  d_mode_msg = (PDEBUG_MODE_MSG)data;
  msg_hdr = (PMSG_HDR)data;
  debug_get_msg = (PDEBUG_GET_MSG)data; 
  sending_num_msg = (PSENDING_NUM_MSG)data;
  get_sending_num_msg = (PSENDING_NUM_MSG)data;
  init_info_set_msg = (PINIT_INFO_SET_MSG)data;
  init_info_query_msg = (PINIT_INFO_QUERY_MSG)data;
  init_info_clear_msg = (PINIT_INFO_CLEAR_MSG)data;

  while((rtn = getopt(argc,argv,"k:Cc:gG:f:IsSd:Dt:Tu:Uin:Nh")) != -1){
    switch (rtn){
    case 'f':
      init_info_set_msg->msg_hdr.command = COM_IPSEC_INIT;
      init_info_set_msg->msg_hdr.kind = KIND_SET;
      if(read_conf_file2(optarg,&init_info_set_msg->init_info)){
	printf("read_conf_file2 error\n");
	goto error;
      }
      init_info_set_msg->msg_hdr.length = 
	sizeof(INIT_INFO_SET_MSG) - sizeof(MSG_HDR);
      datalen = sizeof(INIT_INFO_SET_MSG);
      break;
    case 'C':
      init_info_clear_msg->msg_hdr.command = COM_IPSEC_INIT;
      init_info_clear_msg->msg_hdr.kind = KIND_CLEAR;
      init_info_clear_msg->msg_hdr.length = 
	sizeof(INIT_INFO_CLEAR_MSG) - sizeof(MSG_HDR);
      break;
    case 'I':
      init_info_query_msg->msg_hdr.command = COM_IPSEC_INIT;
      init_info_query_msg->msg_hdr.kind = KIND_QUERY;
      init_info_query_msg->msg_hdr.length = 
	sizeof(INIT_INFO_QUERY_MSG) - sizeof(MSG_HDR);
      datalen = sizeof(INIT_INFO_QUERY_MSG);
      break;
    case 'k':
      ipsecinfo_set->msg_hdr.command = COM_IPSEC_INFO;
      ipsecinfo_set->msg_hdr.kind = KIND_SET;
      if(read_conf_file(optarg,&ipsecinfo_set->ipsec_info)){
	printf("read_conf_file error\n");
	goto error;
      }
      datalen = sizeof(IPSECINFO_SET_MSG);
      ipsecinfo_set->msg_hdr.length = sizeof(IPSECINFO_SET_MSG) - sizeof(MSG_HDR);
      break;
    case 'c':    
      ipsecinfo_clear->key_id_info.spi = atoi(optarg);
      if(ipsecinfo_clear->key_id_info.spi){
	if(argc <= optind){
	  printf("direction nothing\n");
	  return -1;
	}
	ipsecinfo_clear->key_id_info.direction = atoi(argv[optind]);
      }
      ipsecinfo_clear->msg_hdr.command = COM_IPSEC_INFO;
      ipsecinfo_clear->msg_hdr.kind = KIND_CLEAR;
      ipsecinfo_clear->msg_hdr.length = 
	sizeof(IPSECINFO_CLEAR_MSG) - sizeof(MSG_HDR);
      ipsecinfo_clear->key_id_info.tun_dst = 0;//addr.s_addr;
      ipsecinfo_clear->key_id_info.protocol = 0;//IPPROTO_ESP;
      datalen = sizeof(IPSECINFO_CLEAR_MSG);
      printf("spi = %d\n",ipsecinfo_clear->key_id_info.spi);
      break;
    case 'G':
      if(argc <= optind){
	printf("direction nothing\n");
	return -1;
      }
      ipsecinfo_query->key_id_info.tun_dst = 0;//addr.s_addr;
      ipsecinfo_query->key_id_info.spi = atoi(optarg);
      ipsecinfo_query->key_id_info.protocol = 0;//IPPROTO_ESP;
      ipsecinfo_query->key_id_info.direction = atoi(argv[optind]);
      ipsecinfo_query->msg_hdr.command = COM_IPSEC_INFO;
      ipsecinfo_query->msg_hdr.kind = KIND_QUERY;
      ipsecinfo_query->msg_hdr.length = sizeof(IPSECINFO_QUERY_MSG)
	- sizeof(MSG_HDR);
      break;
    case 'g':
      ipsecinfo_query->msg_hdr.command = COM_IPSEC_INFO;
      ipsecinfo_query->msg_hdr.kind = KIND_QUERY;
      ipsecinfo_query->msg_hdr.length = sizeof(IPSECINFO_QUERY_MSG) 
	- sizeof(MSG_HDR) + sizeof(IPSEC_INFO) * (MAX_SA_NUM - 1);
      break;
    case 's':
      ipsecstat_get->msg_hdr.command = COM_STAT;
      ipsecstat_get->msg_hdr.kind = KIND_QUERY;
      ipsecstat_get->msg_hdr.length = sizeof(IPSECSTAT_GET_MSG) - sizeof(MSG_HDR);
      break;
    case 'S':
      ipsecstat_clear->msg_hdr.command = COM_STAT;
      ipsecstat_clear->msg_hdr.kind = KIND_CLEAR;
      ipsecstat_clear->msg_hdr.length = sizeof(IPSECSTAT_CLEAR_MSG) - sizeof(MSG_HDR);
      break;
    case 'd':
      d_mode_msg->msg_hdr.command = COM_DEBUG;
      d_mode_msg->msg_hdr.kind = KIND_SET;
      d_mode_msg->msg_hdr.length = sizeof(DEBUG_MODE_MSG) - sizeof(MSG_HDR);
      d_mode_msg->debug_mode = atoi(optarg);
      break;
    case 'D':
      d_mode_msg->msg_hdr.command = COM_DEBUG;
      d_mode_msg->msg_hdr.kind = KIND_QUERY;
      d_mode_msg->msg_hdr.length = sizeof(DEBUG_MODE_MSG) - sizeof(MSG_HDR);
      break;
    case 't':
      if(argc <= optind){
	printf("timeout_count nothing\n");
	return -1;
      }
      timeout_set->msg_hdr.command = COM_TIMEOUT;
      timeout_set->msg_hdr.kind = KIND_SET;
      timeout_set->timeout_info.timeout_cycle = atoi(optarg);
      timeout_set->timeout_info.timeout_count = atoi(argv[optind]);
      timeout_set->msg_hdr.length = sizeof(TIMEOUT_SET_MSG) - sizeof(MSG_HDR);
      break;
    case 'T':
      timeout_get->msg_hdr.command = COM_TIMEOUT;
      timeout_get->msg_hdr.kind = KIND_QUERY;
      timeout_get->msg_hdr.length = sizeof(TIMEOUT_QUERY_MSG) - sizeof(MSG_HDR);
      break;
    case 'u':
      udp_chk_set->msg_hdr.command = COM_UDP_CHK;
      udp_chk_set->msg_hdr.kind = KIND_SET;
      udp_chk_set->udp_check_mode = atoi(optarg);
      udp_chk_set->msg_hdr.length = sizeof(UDP_CHK_MODE_MSG) - sizeof(MSG_HDR);
      break;
    case 'U':
      udp_chk_get->msg_hdr.command = COM_UDP_CHK;
      udp_chk_get->msg_hdr.kind = KIND_QUERY;
      udp_chk_get->msg_hdr.length = sizeof(UDP_CHK_MODE_MSG) - sizeof(MSG_HDR);
      break;
    case 'i':
      debug_get_msg->msg_hdr.command = COM_DEBUG_GET;
      debug_get_msg->msg_hdr.kind = KIND_QUERY;
      debug_get_msg->msg_hdr.length = sizeof(DEBUG_GET_MSG) - sizeof(MSG_HDR);
      break;
    case 'n':
      sending_num_msg->msg_hdr.command = COM_SENDING_N;
      sending_num_msg->msg_hdr.kind = KIND_SET;
      sending_num_msg->msg_hdr.length = sizeof(SENDING_NUM_MSG) - sizeof(MSG_HDR);
      sending_num_msg->max_sending_num = atoi(optarg);
      break;
    case 'N':
      get_sending_num_msg->msg_hdr.command = COM_SENDING_N;
      get_sending_num_msg->msg_hdr.kind = KIND_QUERY;
      get_sending_num_msg->msg_hdr.length = sizeof(SENDING_NUM_MSG) - sizeof(MSG_HDR);
      break;
    case 'h':
    default:
      printf("option error\n");
      goto usage;
    }
  }
  if(optind == 1){
    ipsecinfo_set->msg_hdr.command = COM_IPSEC_INFO;
    ipsecinfo_set->msg_hdr.kind = KIND_SET;
    if(read_conf_file(argv[1],&ipsecinfo_set->ipsec_info)){
      printf("read_conf_file error\n");
      goto error;
    }
    datalen = sizeof(IPSECINFO_SET_MSG);
    ipsecinfo_set->msg_hdr.length = sizeof(IPSECINFO_SET_MSG) - sizeof(MSG_HDR);
  }
  
  if(send_msg(data)){
    printf("send_msg error\n");
    goto error;
  }

  output_data(msg_hdr);
 
  free(data);
  return 0;
  
 error:
  free(data);
  return -1;
 
}


void output_data(PMSG_HDR msg_hdr)
{
  PIPSEC_INFO ipsec_info;
  struct set_ipsec *set;
  PERROR_INFO error_info;
  struct in_addr addr;
  int count=0;
  struct ipsecstat *stat = &((PIPSECSTAT_GET_MSG)msg_hdr)->ipsec_stat;
  PDEBUG_GET_MSG debug_msg = (PDEBUG_GET_MSG)msg_hdr;
  PINIT_INFO_QUERY_MSG init_info_query_msg = (PINIT_INFO_QUERY_MSG)msg_hdr;
  PUDP_CHK_MODE_MSG udp_chk_get = (PUDP_CHK_MODE_MSG)msg_hdr;
  PTIMEOUT_QUERY_MSG timeout_get = (PTIMEOUT_QUERY_MSG)msg_hdr;
  PSENDING_NUM_MSG get_sending_num_msg = (PSENDING_NUM_MSG)msg_hdr;

  //debug
  error_info = &((PIPSECINFO_QUERY_MSG)msg_hdr)->msg_hdr.error_info;
  if(error_info->error_kind){
    printf("error kind = %d, error code = %d\n", 
	   error_info->error_kind, error_info->error_code);
    return;
  }

  if(msg_hdr->kind == KIND_QUERY){
    switch(msg_hdr->command){ 
    case COM_DEBUG:
      printf(" debug mode = %d\n",((PDEBUG_MODE_MSG)msg_hdr)->debug_mode);
      break;
    case COM_UDP_CHK:
      printf(" udp check mode = %d\n",
	     udp_chk_get->udp_check_mode);
      break;
    case COM_SENDING_N:
      printf(" max sending num = %d\n",
	     get_sending_num_msg->max_sending_num);
      break;
    case COM_TIMEOUT:
      printf(" timeout cycle = %d, timeout count = %d\n",
	     timeout_get->timeout_info.timeout_cycle,
	     timeout_get->timeout_info.timeout_count);
      break;
    case COM_DEBUG_GET:
      printf("driver version = %s\n",debug_msg->debug_drv_info.drv_ver);
      printf(" real device queue stopped num = %d\n",
	     debug_msg->debug_drv_info.drv_stat.realdev_queue_stop_num);
      printf(" decrypt count  sa0: %d   sa1: %d\n",
	     (int)debug_msg->debug_drv_info.dec_count.sa0,
	     (int)debug_msg->debug_drv_info.dec_count.sa1);
      printf(" rcv skb copy count = %d\n",
	     debug_msg->debug_drv_info.drv_stat.rcv_skb_copy_count);
      printf(" virtual device queue stopped = %d\n",
	     debug_msg->debug_drv_info.vdev_queue_stopped);      
      printf(" current sending count = %d\n",
	     debug_msg->debug_drv_info.current_sending);
      printf(" UDP checksum error = %d\n", 
	     debug_msg->debug_drv_info.drv_stat.udp_checksum_error_count);
      break;
    case COM_STAT:
      printf("in_success = %d\n", stat->in_success);
      printf("in_nosa = %d\n", stat->in_nosa);
      printf("in_inval = %d\n", stat->in_inval);
      printf("in_badspi = %d\n", stat->in_badspi);
      printf("in_ahreplay = %d\n", stat->in_ahreplay);
      printf("in_espreplay = %d\n", stat->in_espreplay);
      printf("in_ahauthsucc = %d\n", stat->in_ahauthsucc);
      printf("in_ahauthfail = %d\n", stat->in_ahauthfail);
      printf("in_espauthsucc = %d\n", stat->in_espauthsucc);
      printf("in_espauthfail = %d\n", stat->in_espauthfail);
      printf("in_esphist = %d\n", stat->in_esphist);
      printf("in_ahhist = %d\n", stat->in_ahhist);
      printf("out_success = %d\n", stat->out_success);
      printf("out_polvio = %d\n", stat->out_polvio);
      printf("out_inval = %d\n", stat->out_inval);
      printf("out_esphist = %d\n", stat->out_esphist);
      printf("out_ahhist = %d\n", stat->out_ahhist);
      break;
    case COM_IPSEC_INIT:      
      printf("==== device & NAT info. =========\n");
      printf("dev name     = %s\t",
	     init_info_query_msg->init_info.real_dev_name);
      printf("Nat Traversal   = %d\n", 
	     init_info_query_msg->init_info.nat_info.enable);
      printf("Nat own port = %d\t", 
	     init_info_query_msg->init_info.nat_info.own_port);
      printf("Nat remort port = %d\n\n", 
	     init_info_query_msg->init_info.nat_info.remort_port);
      break;
    case COM_IPSEC_INFO:
      printf("key num =%d\n",(int)((PIPSECINFO_QUERY_MSG)msg_hdr)->ipsec_info_num);
      ipsec_info = ((PIPSECINFO_QUERY_MSG)msg_hdr)->ipsec_info;
      do{
	set = &ipsec_info->set;
	printf("===========\\/ IPSec key(%d) \\/ ===========\n",count);
	printf("mode     = %d\t",set->mode);
	printf("protocol  = %d\n",set->protocol);
	printf("AH_algo  = %d\t",set->AH_algo);
	printf("ESP_algo  = %d\n",set->ESP_algo);
	printf("key_mode = %d\t",set->key_mode);
	printf("linfeTime = %d\n",set->lifeTime);
	if(set->direction == OUT_KEY)
	  printf("======== OUT key ========\n");
	else if (set->direction == IN_KEY)
	  printf("======== IN key =========\n");
	else
	  printf("===key direction(%d) ====\n",set->direction);
	printf("auth_key_len = %3d  ",set->key.auth_key_len);
	printf("auth_key_val = %s\n",set->key.auth_key_val);
	printf("enc_key_len  = %3d  ",set->key.enc_key_len);
	printf("enc_key_val  = %s\n",set->key.enc_key_val);
	printf("spi          = %d\t",set->key.spi);
	printf("key valid    = %d\n",ipsec_info->valid);
	printf("=====IP addresses========\n");
	addr.s_addr = set->src_ip; 
	printf("src_ip  = %s  ",inet_ntoa(addr));
	addr.s_addr = set->dst_ip;
	printf("dst_ip = %s\n",inet_ntoa((struct in_addr)addr));
	addr.s_addr = set->ip_mask;
	printf("ip_mask = %s\n",inet_ntoa((struct in_addr)addr));
	addr.s_addr = set->tun_src;
	printf("tun_src = %s  ",inet_ntoa((struct in_addr)addr));
	addr.s_addr = set->tun_dst;
	printf("tun_dst = %s\n",inet_ntoa((struct in_addr)addr));
	printf("===========/\\ IPSec key(%d) /\\ ===========\n\n",count);
	ipsec_info++;
	count++;
      }while(count < ((PIPSECINFO_QUERY_MSG)msg_hdr)->ipsec_info_num);
      break;
    }   
  }
}
