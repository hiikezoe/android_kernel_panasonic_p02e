# Makefile for mmtuner

#target module name
DRV_NAME = mmtuner

#
# Compile options for release version.
#

#EXTRA_CFLAGS += -DDEBUG
#EXTRA_CFLAGS += -D_DTVD_TUNER_DEBUG_OUT
#EXTRA_CFLAGS += -DKERNEL_2635
EXTRA_CFLAGS += -DKERNEL_2638
EXTRA_CFLAGS += -g0
EXTRA_CFLAGS += -Idrivers/mmtuner/radio
EXTRA_CFLAGS += -Idrivers/mmtuner/radio/com/inc

#I2C_EXCLUSIVE_CONTROL(BugLister ID45)
EXTRA_CFLAGS += -DI2C_EXCLUSIVE_CONTROL

#TUNER_ICS_PATCH(BugLister ID96)
#EXTRA_CFLAGS += -DTUNER_ICS_PATCH

#TUNER_MCBSP_ENABLE ( TI is enable )
#EXTRA_CFLAGS += -DTUNER_MCBSP_ENABLE

#TUNER_TSIF_ENABLE ( Qualcom is enable )
#radio/comsrc/mmtuner_tsprcv.c not build
EXTRA_CFLAGS += -DTUNER_TSIF_ENABLE


obj-$(CONFIG_MMTUNER_DRIVER) += $(DRV_NAME).o

$(DRV_NAME)-objs = \
	radio/radio-mb86a35.o \
	radio/com/src/mmtuner_com_dev.o \
	radio/com/src/mmtuner_com_std.o \
	radio/com/src/mmtuner_core_trans.o \
	radio/com/src/mmtuner_ram.o \
	radio/com/src/mmtuner_tsprcv_api.o
