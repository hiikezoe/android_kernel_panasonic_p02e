
ccflags-$(CONFIG_PM_DEBUG)	:= -DDEBUG
ifdef CONFIG_MMTUNER_DRIVER
ccflags-y			+= -DCONFIG_MMTUNER_DRIVER
endif

obj-y				+= qos.o
obj-$(CONFIG_PM)		+= main.o
obj-$(CONFIG_VT_CONSOLE_SLEEP)	+= console.o
obj-$(CONFIG_FREEZER)		+= process.o
obj-$(CONFIG_SUSPEND)		+= suspend.o csv8x60.o
obj-$(CONFIG_PM_TEST_SUSPEND)	+= suspend_test.o
obj-$(CONFIG_HIBERNATION)	+= hibernate.o snapshot.o swap.o user.o \
				   block_io.o
obj-$(CONFIG_WAKELOCK)		+= wakelock.o
obj-$(CONFIG_USER_WAKELOCK)	+= userwakelock.o
obj-$(CONFIG_EARLYSUSPEND)	+= earlysuspend.o
obj-$(CONFIG_CONSOLE_EARLYSUSPEND)	+= consoleearlysuspend.o
obj-$(CONFIG_FB_EARLYSUSPEND)	+= fbearlysuspend.o
obj-$(CONFIG_SUSPEND_TIME)	+= suspend_time.o

obj-$(CONFIG_MAGIC_SYSRQ)	+= poweroff.o
#ccflags-y += SAFETYIC_DEBUG
