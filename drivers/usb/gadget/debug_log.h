#ifndef __DEBUG_LOG__
#define __DEBUG_LOG__

#include <linux/printk.h>
#include <linux/string.h>

#define __FILENAME__ (strrchr(__FILE__,'/') + 1)

#define MAX_DUMP_SIZE 64
//#define MAX_DUMP_SIZE 16384

#if 0
// log on
#define FUNC_IN()	printk("%s(%d):%s IN\n", __FILENAME__, __LINE__, __FUNCTION__);
#define FUNC_OUT()	printk("%s(%d):%s OUT\n", __FILENAME__, __LINE__, __FUNCTION__);
#define CPRM_LOGOUT(x,...)   CPRM_PRINTF(x, ## __VA_ARGS__);
#define CPRM_TAGGED_LOGOUT(x,...)   CPRM_PRINTF("[CPRM]"x, ## __VA_ARGS__);

#define DUMP_SEND_DATA(req) {\
	CPRM_TAGGED_LOGOUT("__________ DUMP_SEND_DATA START __________\n");\
	local_hex_dump(req->buf, req->length);\
	CPRM_TAGGED_LOGOUT("__________ DUMP_SEND_DATA END __________\n");\
}

#define DUMP_CMD(common) {\
	CPRM_TAGGED_LOGOUT("__________ DUMP_COMMAND START __________\n");\
	CPRM_TAGGED_LOGOUT("command=0x%X\n", common->cmnd[0]);\
	local_hex_dump(common->cmnd, common->cmnd_size);\
	CPRM_TAGGED_LOGOUT("__________ DUMP_COMMAND END __________\n");\
}

#define DUMP_RECV_DATA(req) {\
	CPRM_TAGGED_LOGOUT("__________ DUMP_RECV_DATA START __________\n");\
	local_hex_dump(req->buf, req->length);\
	CPRM_TAGGED_LOGOUT("__________ DUMP_RECV_DATA END __________\n");\
}

#define DUMP_BINARY(msg, msglen) local_hex_dump(msg, msglen)


void CPRM_PRINTF(const char *str, ...)
{
    va_list arg; 
    va_start(arg, str); 

    vprintk(str, arg);
}

void local_hex_dump(char *msg, int msglen)
{
  int i;
  char tmp[64];
  if(msglen > 0){
    CPRM_TAGGED_LOGOUT("length = %d\n",msglen);
    if (MAX_DUMP_SIZE > 0) {
		for(i=0; i< msglen && i < MAX_DUMP_SIZE; i++){
		  sprintf(&tmp[i%16 * 3], "%02X ", (u_char)msg[i]);
		  if(i%16 == 15){
		    tmp[16*3] = 0x00;
		    tmp[8*3-1] = 0x00;
		    CPRM_TAGGED_LOGOUT("%s   %s\n",tmp,&tmp[8*3]);
		  }
		}
		if(i%16 != 0){
		  tmp[i%16*3] = 0x00;
		  if(i%16 > 8){
		    tmp[8*3-1] = 0x00;
		    CPRM_TAGGED_LOGOUT("%s   %s\n",tmp,&tmp[8*3]);
		  }
		  else{
		    CPRM_TAGGED_LOGOUT("%s\n",tmp);
		  }
		}
	}
  }
}

#else
// log off

#define FUNC_IN()
#define FUNC_OUT()
#define CPRM_LOGOUT(x,...)
#define CPRM_TAGGED_LOGOUT(x,...)

#define DUMP_SEND_DATA(req)
#define DUMP_CMD(common)
#define DUMP_RECV_DATA(req)
#define DUMP_BINARY(msg, msglen)

#endif

#endif
