/*
 *  linux/drivers/cfg/hcm_cfg/hcm_cfg_drv.c
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cfgdrv.h>

#define HCMCFG_DEBUG
#ifdef HCMCFG_DEBUG
#define DEBUG_HCM(X)            printk X
#else /* HCMCFG_DEBUG */
#define DEBUG_HCM(X)
#endif /* HCMCFG_DEBUG */

struct cfg_operations Hcm_cfg_ops;

/**
 * cfgdrv_read
 */
int cfgdrv_read(unsigned short id, unsigned short size, unsigned char *data )
{
    if (data == NULL)
    {
        DEBUG_HCM(("cfgdrv read : data error\n"));
        return D_HCM_PARA_NG;
    }

    if (Hcm_cfg_ops.read)
    {
        return Hcm_cfg_ops.read(id, size, data);
    }
    else
    {
        DEBUG_HCM(("cfgdrv read : init error\n"));
        return D_HCM_NOT_INITIALIZE;
    }
}

/**
 * cfgdrv_write
 */
int cfgdrv_write(unsigned short id, unsigned short size, unsigned char *data )
{
    if (data == NULL)
    {
        DEBUG_HCM(("cfgdrv write : data error\n"));
        return D_HCM_PARA_NG;
    }

    if (Hcm_cfg_ops.write)
    {
        return Hcm_cfg_ops.write(id, size, data);
    }
    else
    {
        DEBUG_HCM(("cfgdrv write : init error\n"));
        return D_HCM_NOT_INITIALIZE;
    }
}

/**
 * Hcm_cfg_init
 */
int __init Hcm_cfg_init(void)
{
    DEBUG_HCM(("cfg driver init start\n"));
    return 0;
}
core_initcall(Hcm_cfg_init); 


EXPORT_SYMBOL(Hcm_cfg_ops);
EXPORT_SYMBOL(cfgdrv_read);
EXPORT_SYMBOL(cfgdrv_write);
