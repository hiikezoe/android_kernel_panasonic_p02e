#
# Makefile for misc devices that really don't fit anywhere else.
#

# totoro未定義のため
ANDROID_TOOLCHAIN =1

ifdef ANDROID_TOOLCHAIN

TNC_TOP = ./drivers/TNC

#ccflags-y += -I$(TOP)/prebuilt/ndk/android-ndk-r4/platforms/android-8/arch-arm/usr/include/
ccflags-y += -I$(TOP)/bionic/libc/kernel/common/
ccflags-y += -I$(TNC_TOP)/include/
ccflags-y += -I$(TNC_TOP)/esp_module/include/ -I$(TNC_TOP)/esp_module/crypto/sha1/ -I$(TNC_TOP)/esp_module/crypto/sha2/ -I$(TNC_TOP)/esp_module/crypto/md5/ -I$(TNC_TOP)/esp_module/crypto/rijndael/ -I$(TNC_TOP)/esp_module/crypto/des/

#EXTRA_CFLAGS += -Idrivers/media/dvb/dvb-core

ccflags-y += -DTNC_KERNEL_2_6 -DIP_STB
#PMC-Viana-001-Android_Build対応
ccflags-y += -DBUILD_ANDROID -DPACKET_FILTER_TIMING_CHANGE
ccflags-y += -O1 -Wall
ccflags-y +=  -D__KERNEL__ -DMODULE  -DTNC_TANTAI_TEST
ccflags-y +=  -D_EXPORT_SYMTAB
#PMC-Viana-011-カーネルログ出力有効/無効切り替え対応
ccflags-y +=  -DTNC_LOGLEVEL=0
#ccflags-y +=  -DDBG
##ccflags-y +=  -DMODULE -D"KBUILD_STR(s)=#s" -D"KBUILD_BASENAME=KBUILD_STR(check_timer)"
##ccflags-y +=  -D"KBUILD_MODNAME=KBUILD_STR(Esp_proto)"
#ccflags-y += -DHAVE_NET_DEVICE_OPS

#PMC-Viana-029 Quad組み込み対応 add
ccflags-y += -D"KBUILD_MODNAME=KBUILD_STR(Esp_proto)"
#PMC-Viana-025-Totoro組み込み対応
ccflags-y +=  -DBUILD_TOTORO
#PMC-Viana-026-NEMO組み込み対応
ccflags-y +=  -DBUILD_ICS

export EXTRA_CFLAGS

obj-m	+= Esp_proto.o 
#PMC-Viana-011-カーネルログ出力有効/無効切り替え対応
Esp_proto-y	+= linux_ipsec.o ah.o ah_core.o crypto/des/des_cbc.o esp.o esp_core.o \
	   esp_rijndael.o ip_p.o ipsec.o key.o mbuf.o crypto/md5/md5c.o \
	   nat_traversal.o ioctl_ipsec.o check_timer.o tnc_common_api.o \
	   crypto/rijndael/rijndael.o set_key.o crypto/sha1/sha1.o crypto/sha2/sha256.o
#	   crypto/sha2/sha512.o

obj-m	+= virtuallan.o 
#PMC-Viana-011-カーネルログ出力有効/無効切り替え対応
virtuallan-y	+=VLan_xmit.o tnc_common_api.o
virtuallan-y	+=VLan_init.o nat_traversal.o
else
# makefiles 

CROSS=mn10300-linux-
CC	:= $(CROSS)gcc

CFLAGS += -D__KERNEL__ -DMODULE  -DTNC_TANTAI_TEST
#CFLAGS += -O2 -Wall
CFLAGS += -O1 -Wall


# yでkernel2.6用、それ以外で2.4用
TNC_KERNEL_2_6_MAKE=yes
#TNC_KERNEL_2_6_MAKE=no

#ifdef TNC_KERNEL_2_6_MAKE
ifeq ($(TNC_KERNEL_2_6_MAKE),yes)

#VERBOSE MODE
VMODE=1

# パケットフィルタリングタイミング変更
EXTRA_CFLAGS += -DPACKET_FILTER_TIMING_CHANGE

EXTRA_CFLAGS += -DTNC_TANTAI_TEST
EXTRA_CFLAGS += -D_EXPORT_SYMTAB

# ここにカーネルのソースへのパスを追加すること 
VERSION_R = $(shell uname -r)
VERSION_M = $(shell uname -m)
#KERNEL_SRC = /usr/src/kernels/$(VERSION_R)-$(VERSION_M)
#KERNEL_SRC = /usr/src/kernels/linux-2.6.11
#KERNEL_SRC = /usr/src/linux-2.6.11
KERNEL_SRC = $(MAKETOP)/usr/src/linux

#CFLAGS += -fno-strict-aliasing #警告とるため
#CFLAGS += -fno-common #グローバル変数を使用

EXTRA_CFLAGS += -I$(KERNEL_SRC)/include -I$(KERNEL_SRC)/include/asm/mach-default
EXTRA_CFLAGS += -DTNC_KERNEL_2_6
EXTRA_CFLAGS += -DIP_STB


# 2.6ではモジュールはカーネルソースツリー上でコンパイルする必要がある
TNC_PWD := $(shell pwd)
EXTRA_CFLAGS += -I$(TNC_PWD)/include/ -I$(TNC_PWD)/../include \
	  -I$(TNC_PWD)/crypto/sha1/ -I$(TNC_PWD)/crypto/sha2/ -I$(TNC_PWD)/crypto/md5/ -I$(TNC_PWD)/crypto/rijndael/ -I$(TNC_PWD)/crypto/des/

export EXTRA_CFLAGS


objs_E	:= linux_ipsec.o ah.o ah_core.o crypto/des/des_cbc.o esp.o esp_core.o \
	   esp_rijndael.o ip_p.o ipsec.o key.o mbuf.o crypto/md5/md5c.o \
	   nat_traversal.o ioctl_ipsec.o check_timer.o \
	   crypto/rijndael/rijndael.o set_key.o crypto/sha1/sha1.o crypto/sha2/sha256.o
#	   crypto/sha2/sha512.o

objs_V	:= VLan_init.o VLan_xmit.o


all: Esp_proto.ko virtuallan.ko 
install : all
install-prog: all
	cp $(TNC_PWD)/Esp_proto.ko $(MAKETOP)/usr/dtvrec/bin/
	cp $(TNC_PWD)/virtuallan.ko $(MAKETOP)/usr/dtvrec/bin/

install-header:
install-lib:
install-data:

#Esp_proto.ko:
#	make -C $(KERNEL_SRC) M=`pwd` V=1 modules "CFLAGS += $(CFLAGS)"
#
#virtuallan.ko:
#	make -C $(KERNEL_SRC) M=`pwd` V=1 modules "CFLAGS += $(CFLAGS)"
#
#clean:
#	make -C $(KERNEL_SRC) M=`pwd` V=1 clean
Esp_proto.ko:
	make -C $(KERNEL_SRC) M=`pwd` V=$(VMODE) modules

virtuallan.ko:
	make -C $(KERNEL_SRC) M=`pwd` V=$(VMODE) modules

clean:
	make -C $(KERNEL_SRC) M=`pwd` V=$(VMODE) clean
clean-sub:
	make -C $(KERNEL_SRC) M=`pwd` V=$(VMODE) clean

obj-m := Esp_proto.o virtuallan.o
Esp_proto-objs := $(objs_E)
virtuallan-objs := $(objs_V)

clean-files := *.o *.ko *.mod.[co] *~


else
#TNC_KERNEL_2_6_MAKE no(=2.4)

CFLAGS += -Iinclude/ -I../include -I./crypto/sha1/ -I./crypto/sha2/ -I./crypto/md5/ -I./crypto/rijndael/ -I./crypto/des/

# ここにカーネルのソースへのパスを追加すること 
VERSION =`uname -r`
CFLAGS += -I/usr/src/linux-$(VERSION)/include/
#CFLAGS += -I/usr/src/linux/include/

LDFLAGS	:= 

objs_E	:= linux_ipsec.o ah.o ah_core.o crypto/des/des_cbc.o esp.o esp_core.o \
	   esp_rijndael.o ip_p.o ipsec.o key.o mbuf.o crypto/md5/md5c.o \
	   nat_traversal.o ioctl_ipsec.o check_timer.o \
	   crypto/rijndael/rijndael.o set_key.o crypto/sha1/sha1.o crypto/sha2/sha256.o
#	   crypto/sha2/sha512.o

objs_V	:= VLan_init.o VLan_xmit.o

module_E := Esp_proto.o
module_V := virtuallan.o

all: clean $(module_E) $(module_V)

$(module_E): $(objs_E)
	$(LD) -r $(LDFLAGS) $^ -o $@


$(module_V): $(objs_V)
	$(LD) -r $(LDFLAGS) $^ -o $@

install-header:
install-lib:
install-data:

clean: 
	rm -f *.o *~ ../include/*~ core crypto/des/*.o crypto/md5/*.o crypto/sha1/*.o crypto/sha2/*.o crypto/rijndael/*.o

# (for dtvrec.mk)
clean-sub: 
	rm -f *.o *~ ../include/*~ core crypto/des/*.o crypto/md5/*.o crypto/sha1/*.o crypto/sha2/*.o crypto/rijndael/*.o

x: $(module_E) $(module_V)

SRCS	= linux_ipsec.c ah.c ah_core.c crypto/des/des_cbc.c esp.c esp_core.c \
	   esp_rijndael.c ip_p.c ipsec.c key.c mbuf.c crypto/md5/md5c.c \
	   crypto/rijndael/rijndael.c set_key.c crypto/sha1/sha1.c crypto/sha2/sha256.c \
	   nat_traversal.c ioctl_ipsec.c check_timer.c \
	   VLan_init.c VLan_xmit.c
#	   crypto/sha2/sha512.c
printdep:
	@../../makeinc $(CFLAGS) $(SRCS)

endif

endif
