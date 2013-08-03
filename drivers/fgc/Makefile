#obj-$(CONFIG_PFGC_IF)		+= pfgc_if.o
#obj-$(CONFIG_PFGC_DF)		+= pfgc_df.o
#obj-$(CONFIG_PFGC_FW)		+= pfgc_fw.o
#obj-$(CONFIG_PFGC_DFS)		+= pfgc_dfs.o
#obj-$(CONFIG_PFGC_VER)		+= pfgc_ver.o
#obj-$(CONFIG_PFGC_TABLE)	+= pfgc_table.o
#obj-$(CONFIG_PFGC_INFO)	+= pfgc_info.o
#obj-$(CONFIG_PFGC_LOG)		+= pfgc_log.o
#obj-$(CONFIG_PFGC_FGC)		+= pfgc_fgc.o
#obj-$(CONFIG_PFGC_ADRD)	+= pfgc_adrd_dev.o
#obj-$(CONFIG_PFGC_I2C)		+= pfgc_i2c.o
#obj-$(CONFIG_PFGC_SYSFS)	+= pfgc_sysfs.o
#obj-$(CONFIG_PFGC_DUMMY)	+= pfgc_dummy.o

ifeq ($(CONFIG_BATTERY_DEGRADATION_JUDGEMENT),y)
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_if.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_df.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_fw.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_dfs.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_ver.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_table.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_info.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_log.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_fgc.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_adrd_dev.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_i2c.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_sysfs.o
obj-$(CONFIG_BATTERY_DEGRADATION_JUDGEMENT)	+= pfgc_common.o
else
obj-y	+= pfgc_if.o
obj-y	+= pfgc_df.o
obj-y	+= pfgc_fw.o
obj-y	+= pfgc_dfs.o
obj-y	+= pfgc_ver.o
obj-y	+= pfgc_table.o
obj-y	+= pfgc_info.o
obj-y	+= pfgc_log.o
obj-y	+= pfgc_fgc.o
obj-y	+= pfgc_adrd_dev.o
obj-y	+= pfgc_i2c.o
obj-y	+= pfgc_sysfs.o
obj-y	+= pfgc_common.o
endif
#obj-y	+= pfgc_dummy.o
EXTRA_CFLAGS += -DDISABLE_ONENAND_ACCESS #<ADMMC036>
