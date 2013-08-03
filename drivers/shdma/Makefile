# Makefile for shdma driver

#target module name
DRV_NAME = shdma

#
# Compile options for release version.
#
#DEBUG_SHDMA is debug log enable.
#EXTRA_CFLAGS += -DDEBUG_SHDMA
#EXTRA_CFLAGS += -g0


obj-$(CONFIG_SHDMA_DRIVER) += $(DRV_NAME).o

$(DRV_NAME)-objs = \
	shdma_driver.o \
