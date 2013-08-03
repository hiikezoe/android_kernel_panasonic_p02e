#
# Copyright (c) 2006 by Matsushita Electric Industrial Co., Ltd.
# All rights reserved.
#

# totoro–¢’è‹`‚Ì‚½‚ß
ANDROID_TOOLCHAIN =1

ifdef ANDROID_TOOLCHAIN
obj-y				+= esp_module/

#include $(MAKETOP)/usr/src/dtvrec/common/mk/dtvrec.mk
else

SUBDIRS = wrapper

#CFLAGS	+= -DKERNEL_TNC_BUILD
#SUBDIRS = wrapper esp_module


include $(MAKETOP)/usr/src/dtvrec/common/mk/dtvrec.mk
endif
