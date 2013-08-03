#
# Makefile for FeliCa control driver.
#
# <iDPower_003> add S
include drivers/felica/inc/felica_debug.mk
# <iDPower_003> add E

# target module name
DRV_NAME = felica_ctrl

## Defined compiling option
# <iDPower_003> del S
#EXTRA_CFLAGS :=
# <iDPower_003> del E
EXTRA_CFLAGS += -Idrivers/felica/common
EXTRA_CFLAGS += -Idrivers/felica/inc

#
# Compile options for release version.
#

# <iDPower_003> mod S
#obj-m	:= $(DRV_NAME).o
obj-y	:= $(DRV_NAME).o
# <iDPower_003> mod E
$(DRV_NAME)-objs = \
	src/felica_ctrl.o

## End of Makefile
