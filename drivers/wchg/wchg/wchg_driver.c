/*
 * Wireless Charging linux driver
 * 
 * Usage: Wireless Charging driver for linux
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html
 *
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <mach/gpiomux.h>
#include <linux/regulator/consumer.h>
#include <linux/miscdevice.h>
#include <linux/delay.h> /* Quad add: npdc300052029, npdc300052031 */
#include <linux/wchg_driver.h>

/******************************************************************************/
static int wchg_drv_open(struct inode *inode, struct file *filp);
static int wchg_drv_close(struct inode *inode, struct file *filp);
static long wchg_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static struct regulator *wchg_regulator_l8_get(void);

unsigned char wchg_drv_charge_sts = WCHG_CHG_NORMAL; /* Quad add: npdc300052029, npdc300052031 */

/******************************************************************************/
static struct file_operations wchg_drv_fops =
{
    .owner          = THIS_MODULE,
    .open           = wchg_drv_open,
    .unlocked_ioctl = wchg_drv_ioctl,
    .release        = wchg_drv_close,
};

static struct miscdevice wchg_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = D_WCHG_DRV_NAME,
    .fops = &wchg_drv_fops
};

static unsigned char open_count = 0;

static struct gpiomux_setting gpio_in_wchg_config = {
    .func = GPIOMUX_FUNC_GPIO,  /* GPIO10/GPIO11 */
    .drv = GPIOMUX_DRV_8MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_IN,
};

static struct gpiomux_setting gsbi4 = {
    .func = GPIOMUX_FUNC_1,
    .drv = GPIOMUX_DRV_8MA,
    .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_out_wchg_config = {
    .func = GPIOMUX_FUNC_GPIO,  /* GPIO10/GPIO11 */
    .drv = GPIOMUX_DRV_8MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config wchg_init_configs[]  = {
    {
        .gpio      = 10,    /* GSBI4 GPIO OUT */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gpio_out_wchg_config,
            [GPIOMUX_ACTIVE] = &gpio_out_wchg_config,
        },
    },
    {
        .gpio      = 11,    /* GSBI4 GPIO IN */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gpio_in_wchg_config,
            [GPIOMUX_ACTIVE] = &gpio_in_wchg_config,
        },
    },
};
static struct msm_gpiomux_config wchg_tx_configs[]  = {
    {
        .gpio      = 10,    /* GSBI4 UART-TX */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gsbi4,
            [GPIOMUX_ACTIVE] = &gsbi4,
        },
    },
    {
        .gpio      = 11,    /* GSBI4 GPIO IN */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gpio_in_wchg_config,
            [GPIOMUX_ACTIVE] = &gpio_in_wchg_config,
        },
    },
};
static struct msm_gpiomux_config wchg_rx_configs[]  = {
    {
        .gpio      = 10,    /* GSBI4 GPIO IN */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gpio_in_wchg_config,
            [GPIOMUX_ACTIVE] = &gpio_in_wchg_config,
        },
    },
    {
        .gpio      = 11,    /* GSBI4 UART-RX */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gsbi4,
            [GPIOMUX_ACTIVE] = &gsbi4,
        },
    },
};

/******************************************************************************/
/*  function    : wchg driver open                                            */
/******************************************************************************/
static int wchg_drv_open(struct inode *inode, struct file *filp)
{
    if(open_count > 0){
        return -1;
    }
    open_count ++;
    return 0;
}

/******************************************************************************/
/*  function    : wchg driver close                                           */
/******************************************************************************/
static int wchg_drv_close(struct inode *inode, struct file *filp)
{
    open_count --;
    return 0;
}

/******************************************************************************/
/*  function    : wchg driver ioctl                                           */
/******************************************************************************/
static long wchg_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = D_LIBWCHG_RET_OK;
    struct regulator *l8_reg = NULL;
    
    switch(cmd){
    case D_WCHGDD_IOCTL_ENABLE:
        wchg_drv_charge_sts = WCHG_CHG_FW_REWRITE; /* Quad add: npdc300052029, npdc300052031 */
        
        /* PMIC GPIO_10 High */
        gpio_set_value(PMIC_GPIO_10, WCHG_GPIO_HIGH);
        
        usleep(100 * 1000); /* Quad add: npdc300052029, npdc300052031 wait for stopping charge */
        
        /* PMIC L8 Regulator ON */
        l8_reg = wchg_regulator_l8_get();
        if(l8_reg == NULL){
            return D_LIBWCHG_RET_NG;
        }
        regulator_enable(l8_reg);

        /* PMIC GPIO_42 High */
        gpio_set_value(PMIC_GPIO_42, WCHG_GPIO_HIGH);
        break;
        
    case D_WCHGDD_IOCTL_DISABLE:
        /* PMIC GPIO_42 Low */
        gpio_set_value(PMIC_GPIO_42, WCHG_GPIO_LOW);
        
        /* PMIC L8 Regulator OFF */
        l8_reg = wchg_regulator_l8_get();
        if(l8_reg == NULL){
            return D_LIBWCHG_RET_NG;
        }
        regulator_disable(l8_reg);
        
        /* PMIC GPIO_10 Low */
        gpio_set_value(PMIC_GPIO_10, WCHG_GPIO_LOW);
        
        wchg_drv_charge_sts = WCHG_CHG_NORMAL; /* Quad add: npdc300052029, npdc300052031 */
        break;
        
    case D_WCHGDD_IOCTL_UART_TX:
        msm_gpiomux_install(wchg_tx_configs, ARRAY_SIZE(wchg_tx_configs));
        break;
        
    case D_WCHGDD_IOCTL_UART_RX:
        msm_gpiomux_install(wchg_rx_configs, ARRAY_SIZE(wchg_rx_configs));
        break;
        
    case D_WCHGDD_IOCTL_UART_INIT:
        msm_gpiomux_install(wchg_init_configs, ARRAY_SIZE(wchg_init_configs));
        break;
        
    default:
        ret = D_LIBWCHG_RET_NG;
        break;
    }

    return ret;
}

/******************************************************************************/
/*  function    : wchg regulator L8 get                                       */
/******************************************************************************/
static struct regulator *wchg_regulator_l8_get(void)
{
    static struct regulator *l8_reg = NULL;
    int ret = 0;
    
    if (l8_reg == NULL) {
        l8_reg = regulator_get(NULL, "wchg_vdd");
        if(l8_reg == NULL){
            return NULL;
        }
        ret = regulator_set_voltage(l8_reg, 2800000, 2800000);
        if(ret != 0){
            l8_reg = NULL;
            return NULL;
        }
    }
    return l8_reg;
}

/******************************************************************************/
/*  function    : wchg driver init                                            */
/******************************************************************************/
int __init wchg_drv_init(void)
{
    int     ret = 0;

    ret = misc_register(&wchg_dev);
    if(ret){
        return ret;
    }
    open_count = 0;
    return 0;
}

/******************************************************************************/
/*  function    : wchg driver exit                                            */
/******************************************************************************/
void wchg_drv_exit( void )
{
    misc_deregister(&wchg_dev);
    return;
}

/******************************************************************************/
module_init(wchg_drv_init);
module_exit(wchg_drv_exit);

MODULE_DESCRIPTION("WCHG driver");
MODULE_LICENSE("GPL");
