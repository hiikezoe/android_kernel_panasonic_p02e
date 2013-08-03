
/*----------------------------------------------------------------------------*/
/* Include								    */
/*----------------------------------------------------------------------------*/
/* Jessie add start for DEBUG */
/*#define SAFETYIC_DEBUG */
#ifdef SAFETYIC_DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#define DEBUG_PT
#endif
/* Jessie add end for DEBUG */
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/cfgdrv.h>
#include <linux/i2c/twl.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/power_supply.h>     /*<SlimID> add*/
//#include <linux/charger.h>          /*<SlimID> add*/
//#include "i2c.h"
#include <linux/i2c.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include "csv8x60.h"
//#include <plat/gpio.h>
#include <linux/mfd/pm8xxx/vibrator.h> /* IDPower */
#include <linux/mfd/pm8xxx/pwm.h>      /* IDPower */
#include <linux/mfd/pm8xxx/pm8921-charger.h>/* IDPower */

/* IDPower struct clk *clk_get_ret[I2C_NUM_IF]; */
struct device *dev1, *dev2, *dev3, *dev4;
struct current_chk_dev_list dev_list;
struct i2c_adapter *adapter_1 = NULL; /* IDPower */
struct i2c_adapter *adapter_3 = NULL; /* IDPower */
struct i2c_adapter *adapter_7 = NULL; /* Rupy Add */
static int csv_init_flg = 0;
static int spider3_soft2_enable = 0;

static int csv_i2c_read(struct i2c_adapter* i2c_adap, unsigned char slave, unsigned char addr, unsigned char* val);
static int csv_i2c_write(struct i2c_adapter* i2c_adap, unsigned char slave, unsigned char addr, unsigned char val);
static u32 msm_device_chk(void);
static int csv_clk_ctrl(int ctrl);

extern int pm8921_get_regulator_state(enum which_resource resource, int *enable, int *lpm); /* IDPower */
extern int pm8921_get_registor_state(u16 addr, u8 *val); /* IDPower */

#if ( defined(DEBUG_PT) || defined(DEBUG_IT) )   /* Jessie add start */
#define CSV_TEST_LOG(a, b, c) csv_test_log(a, b, c);
#else
#define CSV_TEST_LOG(a, b, c)
#endif /*( defined(DEBUG_PT) || defined(DEBUG_IT) ) Jessie add end */

#if ( defined(DEBUG_PT) || defined(DEBUG_IT) ) /* Jessie mod */
#include <linux/cfgdrv.h>
/*                                      */
#define D_HCM_AGING_PROC_FLG_SIZE (160)
#define D_TEST_LOG_SIZE (16)
#define D_TEST_LOG_MAX  (10)
static volatile int test_idx = 0;
static void csv_test_log(u8 id, u8 data, u32 err)
{
#if 0  /* Jessie mod start */
	static u8   buff[D_HCM_AGING_PROC_FLG_SIZE] = {0};
	u8   log[D_TEST_LOG_MAX] = {0};
	int  rslt = 0;
	int  offset, i;
	u64  usecs64;
	u64  tmp;
	u8   val;

	CSV_LOG_FUNCIN("\n");
	
	rslt = cfgdrv_read(D_HCM_AGING_PROC_FLG, D_HCM_AGING_PROC_FLG_SIZE,
			   buff);
	if (rslt < 0) {
		pr_err("cfgdrv_read error %d\n", rslt);
		return;
	}

	log[0] = test_idx;
	log[1] = id;
	log[2] = data;
	log[3] = 0;   // Spare

	tmp = err;
	for ( i = 0; i < 4; i++ ) {
		val = (u8)( tmp & 0xff );
		log[8 - 1 - i] = val;
		tmp >>= 8;
	}

	/*get time*/
	// usecs64 = ktime_to_ns(ktime_get()); //             
	usecs64 = sched_clock();

	tmp = usecs64;
	for ( i = 0; i < 8; i++ ) {
		val = (u8)( tmp & 0xff );
		log[D_TEST_LOG_SIZE - 1 - i] = val;
		tmp >>= 8;
	}

	offset = (test_idx % D_TEST_LOG_MAX) * D_TEST_LOG_SIZE;
	memcpy((buff + offset), log, D_TEST_LOG_SIZE);

	rslt = cfgdrv_write(D_HCM_AGING_PROC_FLG,
			    D_HCM_AGING_PROC_FLG_SIZE, buff);
	if (rslt < 0) {
		pr_err("cfgdrv_write error %d\n", rslt);
	}
	test_idx++;
#else
    DEBUG_CSV(("CSV:%d:%d:%d\n", id, data, err));
#endif /* Jessie del end */
	CSV_LOG_FUNCOUT("\n");
}
#endif /*( defined(DEBUG_PT) || defined(DEBUG_IT) )  Jessie mod */

static int csv_i2c_read(struct i2c_adapter* i2c_adap, unsigned char slave, unsigned char addr, unsigned char* val)
{
    int ret = 0;

    /* Create a i2c_msg buffer, that is used to put the controller into read
       mode and then to read some data. */
    struct i2c_msg msg_buf[] = {
        {slave, 0, 1, &addr},
        {slave, I2C_M_RD, 1, val}
    };

    ret = i2c_transfer(i2c_adap, msg_buf, 2);
	DEBUG_CSV(("csv_i2c_read addr = 0x%x, data = 0x%x\n", addr, *val));
	if(ret < 0){
		DEBUG_CSV(("csv_i2c_read error, ret = %d\n", ret));
	}

    return ret;
}

static int csv_i2c_write(struct i2c_adapter* i2c_adap, unsigned char slave, unsigned char addr, unsigned char val)
{
	int ret;
    unsigned char data_buf[] = { addr, val };

    /* Create a i2c_msg buffer, that is used to put the controller into write
       mode and then to write some data. */
    struct i2c_msg msg_buf[] = {
        {slave, 0, 2, data_buf}
    };

    ret = i2c_transfer(i2c_adap, msg_buf, 1);
    DEBUG_CSV(("csv_i2c_write addr = 0x%x, data = 0x%x\n", addr, val));
	if(ret < 0){
		DEBUG_CSV(("csv_i2c_write error, ret = %d\n", ret));
	}

    return ret;
}

int csv_enable_soft_mode2(void)
{
	int ret = 0;
	u32 dev_chk = 0;
	unsigned char buf;
	
	CSV_LOG_FUNCIN("\n");

	if(csv_init_flg == 0){
		pr_err("*** %s : no initialize\n", __func__);
		return CSV_INIT_ERR;
	}
	
	if (adapter_1 == NULL){
		pr_err("*** adapter_1 NULL\n");
	}
	
	if (adapter_3 == NULL){
		pr_err("*** adapter_3 NULL\n");
	}
	
#ifdef SAFETY_QUAD /* Rupy mod Start */
	if (adapter_7 == NULL){
		pr_err("*** adapter_7 NULL\n");
	}
#endif /* SAFETY_QUAD *//* Rupy mod end */
	
	/* enable i2c clock */
	ret = csv_clk_ctrl(CLK_ON);
	if (ret != 0) {
		pr_err("*** %s : csv_clk_ctrl()\n", __func__);
		return CSV_CLK_ERR;
	}
	
	/* device off check */
	dev_chk = msm_device_chk();

	if (dev_chk == 0){
		buf = 0x0F;
		spider3_soft2_enable = 1;
		pr_info("spider3_soft2_enable = 1\n");
		CSV_TEST_LOG(0x00, buf, 0);	/* Jessie  mod */
	} else {
		buf = 0x00;
		pr_info("*** %s : [dev_chk:0x%08x]\n", __func__, dev_chk);
		CSV_TEST_LOG(0x01, buf, dev_chk); /* Jessie mod */
	}
	
	/* write 0x00 or 0x0F to CURRENT_SET3 reg */
	ret = csv_i2c_write(adapter_1, SPIDER3_CHIP_ADDR, SPIDER3_CURRENT_SET3, buf);
	if (ret < 0) {
		pr_err("*** %s : i2c_write() error\n", __func__);
		csv_clk_ctrl(CLK_OFF);
		return CSV_I2C_NG;
	}
	
	/* disable i2c clock */
	csv_clk_ctrl(CLK_OFF);

	CSV_LOG_FUNCOUT("\n");
	return dev_chk;
}

int csv_disable_soft_mode2(void)
{
	int ret = 0;
	unsigned char buf;
	
	CSV_LOG_FUNCIN("\n");

	if(csv_init_flg == 0){
		pr_info("*** %s : no initialize\n", __func__);
		return CSV_INIT_ERR;
	}
	
	ret = csv_i2c_read(adapter_1, SPIDER3_CHIP_ADDR, SPIDER3_CURRENT_SET3, &buf);
	DEBUG_CSV(("SPIDER3_CURRENT_SET3 value = 0x%x\n", buf));
	
	if(spider3_soft2_enable == 1){
		/* enable i2c clock */
		ret = csv_clk_ctrl(CLK_ON);
		if (ret != 0) {
			pr_info("*** %s : csv_clk_ctrl()\n", __func__);
			return CSV_CLK_ERR;
		}
		
		buf = 0;
		/* write 0x00 to CURRENT_SET3 reg */
		ret = csv_i2c_write(adapter_1, SPIDER3_CHIP_ADDR, SPIDER3_CURRENT_SET3, buf);
		CSV_TEST_LOG(0x10, buf, 0); /* Jessie mod */
		if(ret < 0){
			pr_err("*** %s : i2c_write(I2C3) error\n", __func__);
			/* disable i2c clock */
			csv_clk_ctrl(CLK_OFF);
			return CSV_I2C_NG;
		}
					
		/* disable i2c clock */
		csv_clk_ctrl(CLK_OFF);
		spider3_soft2_enable = 0;
		pr_info("spider3_soft2_enable = 0\n");
	}

	CSV_LOG_FUNCOUT("\n");
	return ret;
}

static int csv_clk_ctrl(int ctrl)
{
#if 0 /* IDPower */
	int ret = 0;

	if(ctrl == CLK_ON)
	{
		/* I2C CLK ON*/
		ret = clk_enable(clk_get_ret[I2C_1]);
		ret |=  clk_enable(clk_get_ret[I2C_3]);
		ret |=  clk_enable(clk_get_ret[I2C_4]);
		if (ret != 0) {
			printk("*** %s : clk_enable() error\n", __func__);
			return ret;
		}
	} else	{
		/* I2C CLK OFF */
		clk_disable(clk_get_ret[I2C_1]);
		clk_disable(clk_get_ret[I2C_3]);
		clk_disable(clk_get_ret[I2C_4]);
	}
#endif
	
	CSV_LOG_FUNCOUT("\n");
	return 0;
}

static u32 msm_device_chk(void)
{
	u32 err = 0;
	int ret = 0;
	unsigned char buf;
#if 0 /* Rupy Del start */
	int source;
#endif /* Rupy del end */
	int enable;
	int lpm;
	u8 val;
	
	CSV_LOG_FUNCIN("\n");
	
	/* APQ GPIO read */
#ifdef    CONFIG_MMTUNER_DRIVER   /* Jessie add start */
    buf = AN30183A_CNT;
#ifdef SAFETY_QUAD /* Rupy mod Start */
	ret = csv_i2c_read(adapter_7, AN30183A_CHIP_ADDR, buf,
                       &dev_list.mmtuner_reg);
#else /* SAFETY_QUAD */
	ret = csv_i2c_read(adapter_1, AN30183A_CHIP_ADDR, buf,
                       &dev_list.mmtuner_reg);
#endif /* SAFETY_QUAD *//* Rupy mod end */
	if (ret < 0) {
		printk("*** %s : i2c_read() error\n", __func__);
		return CSV_I2C_NG;
	}
	DEBUG_CSV(("dev_list.mmtuner = 0x%x\n", dev_list.mmtuner_reg));
#endif /* CONFIG_MMTUNER_DRIVER *//* Jessie add end */
	/* PM8921 GPIO/Register read */
	/* Main LCD */
	dev_list.lcd_dcdc_en = gpio_get_value_cansleep(PMIC_GPIONO16);			/* Rupy Mod */
	DEBUG_CSV(("dev_list.lcd_dcdc_en = 0x%x\n", dev_list.lcd_dcdc_en));		/* Rupy Mod */
	
	/* Main LCD BL */ /* Jessie mod */
	dev_list.bl_dcdc_en = gpio_get_value_cansleep(PMIC_GPIONO37);			/* Rupy Mod */
	DEBUG_CSV(("dev_list.bl_dcdc_en = 0x%x\n", dev_list.bl_dcdc_en));		/* Rupy Mod */
	
	/* Haptics(Vibration) */
	dev_list.haptics_en = gpio_get_value_cansleep(PMIC_GPIONO20);			/* Rupy Add */
	DEBUG_CSV(("dev_list.haptics_en = 0x%x\n", dev_list.haptics_en));		/* Rupy Add */
	
	/* Audio */ /* Jessie mod */
	dev_list.audio_gpoout = gpio_get_value_cansleep(PMIC_GPIONO18);			/* Rupy Mod */
	DEBUG_CSV(("dev_list.audio_gpoout = 0x%x\n", dev_list.audio_gpoout));	/* Rupy Mod */
	
	/* Vibration */
#if  0 /* Rupy Del start */
	ret = pm8921_get_registor_state(VIB_DRV, &val);
	DEBUG_CSV(("VIB_DRV value = 0x%x\n", val));
	if (ret < 0) {
		printk("pm8921_get_registor_state VIB error = %d\n", ret);
		err |= VIBRATION_VIB_DRV_NG;
		return err;
	}
	dev_list.vibration_vib_drv = val;
	DEBUG_CSV(("dev_list.vibration_vib_drv = 0x%x\n", dev_list.vibration_vib_drv));
#endif /* Rupy Del end */
	
	/* LED */
	ret = pm8921_get_registor_state(SSBI_REG_ADDR_LPG_BANK_EN, &val);
	DEBUG_CSV(("SSBI_REG_ADDR_LPG_BANK_EN value = 0x%x\n", val));
	if (ret < 0) {
		printk("pm8921_get_registor_state LED error = %d\n", ret);
		err |= LED_RED_NG;
		return err;
	}
	/* Jessie mod start */
	dev_list.led = val;
	DEBUG_CSV(("dev_list.led = 0x%x\n", dev_list.led));
	/* Jessie mod end */
	
	/* Camera */
#if 0 /* Rupy del start */
	/* IN Camera */
	dev_list.camera_v12cam = gpio_get_value(MSM_GPIONO1);
	DEBUG_CSV(("dev_list.camera_v12cam = 0x%x\n", dev_list.camera_v12cam));
	/* OUT Camera */
	dev_list.camera_v28cam = gpio_get_value(MSM_GPIONO15);
	DEBUG_CSV(("dev_list.camera_v28cam = 0x%x\n", dev_list.camera_v28cam));
#endif /* Rupy del end */
	
	/* Camera */
	dev_list.v12cam_en = gpio_get_value(APQ_GPIONO1);
	DEBUG_CSV(("dev_list.v12cam_en = 0x%x\n", dev_list.v12cam_en));
	
	dev_list.v11cam_en = gpio_get_value(APQ_GPIONO2);
	DEBUG_CSV(("dev_list.v11cam_en = 0x%x\n", dev_list.v11cam_en));
	
	dev_list.ncamisp_rst = gpio_get_value(APQ_GPIONO86);
	DEBUG_CSV(("dev_list.ncamisp_rst = 0x%x\n", dev_list.ncamisp_rst));
	
	/* Rupy Add start */
	ret = pm8921_get_registor_state(SSBI_REG_ADDR_VREG_LVS5_18, &val);
	DEBUG_CSV(("SSBI_REG_ADDR_VREG_LVS5_18 value = 0x%x\n", val));
	if (ret < 0) {
		printk("pm8921_get_registor_state Camera LVS5 error = %d\n", ret);
		err |= CAMERA_VREG_LVS5_18_NG;
		return err;
	}
	dev_list.vreg_lvs5_18 = val;
	DEBUG_CSV(("dev_list.vreg_lvs5_18 = 0x%x\n", dev_list.vreg_lvs5_18));
	
	ret = pm8921_get_registor_state(SSBI_REG_ADDR_VREG_L11_27, &val);
	DEBUG_CSV(("SSBI_REG_ADDR_VREG_L11_27 value = 0x%x\n", val));
	if (ret < 0) {
		printk("pm8921_get_registor_state Camera L11 error = %d\n", ret);
		err |= CAMERA_VREG_L11_27_NG;
		return err;
	}
	dev_list.vreg_l11_27 = val;
	DEBUG_CSV(("dev_list.vreg_l11_27 = 0x%x\n", dev_list.vreg_l11_27));
	
	ret = pm8921_get_registor_state(SSBI_REG_ADDR_VREG_L16_27, &val);
	DEBUG_CSV(("SSBI_REG_ADDR_VREG_L16_27 value = 0x%x\n", val));
	if (ret < 0) {
		printk("pm8921_get_registor_state Camera L16 error = %d\n", ret);
		err |= CAMERA_VREG_L16_27_NG;
		return err;
	}
	dev_list.vreg_l16_27 = val;
	DEBUG_CSV(("dev_list.vreg_l16_27 = 0x%x\n", dev_list.vreg_l16_27));
	
	ret = pm8921_get_registor_state(SSBI_REG_ADDR_VREG_L12_12, &val);
	DEBUG_CSV(("SSBI_REG_ADDR_VREG_L12_12 value = 0x%x\n", val));
	if (ret < 0) {
		printk("pm8921_get_registor_state Camera L12 error = %d\n", ret);
		err |= CAMERA_VREG_L12_12_NG;
		return err;
	}
	dev_list.vreg_l12_12 = val;
	DEBUG_CSV(("dev_list.vreg_l12_12 = 0x%x\n", dev_list.vreg_l12_12));
	/* Rupy Add end */
	
	/* microSD */
	ret = pm8921_get_regulator_state(L6, &enable, &lpm);
	DEBUG_CSV(("L6 enable = %d, lpm = %d\n", enable, lpm));
	if (ret != 0) {
		printk("L6 pm8921_get_regulator_state error = %d\n", ret);
		err |= MICROSD_VMMC1_NG;
		return err;
	}
	
//	if(((enable & 0x01) == 0) && ((lpm & 0x01) == 1)){
	if((enable & 0x01) == 0){
		dev_list.microSD_vmmc1 = 0;
	}
	else{
		dev_list.microSD_vmmc1 = 1;
		err |= MICROSD_VMMC1_NG;
	}
	DEBUG_CSV(("dev_list.microSD_vmmc1 = 0x%x\n", dev_list.microSD_vmmc1));
	
	/* WLAN */
	ret = pm8921_get_regulator_state(L10, &enable, &lpm);
	DEBUG_CSV(("L10 enable = %d, lpm = %d\n", enable, lpm));
	if (ret != 0) {
		printk("pm8921_get_regulator_state L10 error = %d\n", ret);
		err |= WLAN_GPIO_DATAOUT_NG;
		return err;
	}
	
//	if(((enable & 0x01) == 0) && ((lpm & 0x01) == 1)){
	if((enable & 0x01) == 0){
		dev_list.wlan_vreg = 0;
	}
	else{
		dev_list.wlan_vreg = 1;
		err |= WLAN_GPIO_DATAOUT_NG;
	}
	DEBUG_CSV(("dev_list.wlan_vreg = 0x%x\n", dev_list.wlan_vreg));
	
	/* FeliCa */
	dev_list.felica_gpoout = gpio_get_value_cansleep(PMIC_GPIONO1);				/* Rupy mod */
	DEBUG_CSV(("dev_list.felica_gpoout = 0x%x\n", dev_list.felica_gpoout));
	
#if  0	/* Rupy del start */
	/* Charger */
	dev_list.charging_status = pm8921_is_battery_charging(&source);
	DEBUG_CSV(("dev_list.charging_status = 0x%x\n", dev_list.charging_status));
#endif	/* Rupy del end */
	
#ifdef    CONFIG_MMTUNER_DRIVER   /* Jessie add start */
	/* MM Tuner */
#ifdef SAFETY_QUAD /* Rupy mod Start */
	ret = csv_i2c_read(adapter_7, AN30183A_CHIP_ADDR, AN30183A_CNT, &buf);
#else /* SAFETY_QUAD */
	ret = csv_i2c_read(adapter_1, AN30183A_CHIP_ADDR, AN30183A_CNT, &buf);
#endif /* SAFETY_QUAD *//* Rupy mod end */
	if (ret < 0) {
		printk("*** %s : i2c_read() error\n", __func__);
		return CSV_I2C_NG;
	}
	DEBUG_CSV(("AN30183A_CNT = 0x%x\n", buf));
	dev_list.mmtuner_reg = buf;
#endif /* CONFIG_MMTUNER_DRIVER *//* Jessie add end */
	
	/* device reg check */
	if(dev_list.lcd_dcdc_en != 0x0)			/* Rupy mod */
	{
		err |= LCD_DCDC_EN_NG;				/* Rupy mod */
	}
	
	if(dev_list.bl_dcdc_en != 0x0)			/* Rupy mod */
	{
		err |= BL_DCDC_EN_NG;				/* Rupy mod */
	}
	
	/* Rupy Add start */
	if(dev_list.haptics_en != 0x0)
	{
		err |= HAPTICS_EN_NG;
	}
	/* Rupy Add end */
	
	if(dev_list.audio_gpoout != 0x0)
	{
		err |= AUDIO_GPOOUT_NG;
	}
#if  0	/* Rupy del start */
	if((dev_list.vibration_vib_drv & CHK_VIBRATION_VIB_DRV_REG_BIT) != 0x0)
	{
		err |= VIBRATION_VIB_DRV_NG;
	}
#endif	/* Rupy del end */
	/* Jessie mod start */
	if((dev_list.led & CHK_LED_RED_REG_BIT) != 0x0)
	{
		err |= LED_RED_NG;
	}
	if((dev_list.led & CHK_LED_GREEN_REG_BIT) != 0x0)
	{
		err |= LED_GREEN_NG;
	}
	if((dev_list.led& CHK_LED_BLUE_REG_BIT) != 0x0)
	{
		err |= LED_BLUE_NG;
	}
	/* Jessie mod end */
/* Rupy Mod start */
	if(dev_list.v12cam_en != 0x0)
	{
		err |= CAMERA_V12CAM_EN_NG;
	}
	if(dev_list.v11cam_en != 0x0)
	{
		err |= CAMERA_V11CAM_EN_NG;
	}
	if(dev_list.ncamisp_rst != 0x0)
	{
		err |= CAMERA_nCAMISP_RST_NG;
	}
/* Rupy Mod end */
	/* Rupy Add start */
	if((dev_list.vreg_lvs5_18 & CHK_PM8921_REG_ENABLE_BIT) != 0x0)
	{
		err |= CAMERA_VREG_LVS5_18_NG;
	}
	
	if((dev_list.vreg_l11_27 & CHK_PM8921_REG_ENABLE_BIT) != 0x0)
	{
		err |= CAMERA_VREG_L11_27_NG;
	}
	
	if((dev_list.vreg_l16_27 & CHK_PM8921_REG_ENABLE_BIT) != 0x0)
	{
		err |= CAMERA_VREG_L16_27_NG;
	}
	
	if((dev_list.vreg_l12_12 & CHK_PM8921_REG_ENABLE_BIT) != 0x0)
	{
		err |= CAMERA_VREG_L12_12_NG;
	}
	/* Rupy Add end */
	
#ifdef    CONFIG_MMTUNER_DRIVER   /* Jessie add start */
	if ((dev_list.mmtuner_reg & CHK_MMTUNER_V18DTV_REG_BIT) != 0x0) {
		err |= MMTUNER_V18DTV_NG;
	}
	if ((dev_list.mmtuner_reg & CHK_MMTUNER_V28DTV_REG_BIT) != 0x0) {
		err |=MMTUNER_V28DTV_NG;
	}
	if ((dev_list.mmtuner_reg & CHK_MMTUNER_V12DTV_REG_BIT) != 0x0) {
		err |=MMTUNER_V12DTV_NG;
	}
#endif /* CONFIG_MMTUNER_DRIVER *//* Jessie add end */
	if(dev_list.felica_gpoout != 0x0)
	{
		err |= FELICA_GPOOUT_NG;
	}
	
#if  0 /* Rupy del start */
	if(dev_list.charging_status != 0x0)
	{
		err |= CHARGING_STATUS_NG;
	}
#endif /* Rupy del end */
	
	CSV_LOG_FUNCOUT("\n");
	return err;
}

static int __init csv_init(void)
{
//	const char *qup_1 = "qup_i2c.1";
//	int i;

	CSV_LOG_FUNCIN("\n");

	/* set i2c bus */
	/* rupy mod start */
#ifdef SAFETY_QUAD /* Rupy mod Start */
	adapter_1 = i2c_get_adapter(4);
#else /* SAFETY_QUAD */
	adapter_1 = i2c_get_adapter(1);
#endif /* SAFETY_QUAD *//* Rupy mod end */
	/* rupy mod end */
	adapter_3 = i2c_get_adapter(3);
#ifdef SAFETY_QUAD /* Rupy mod Start */
	adapter_7 = i2c_get_adapter(7); /* Rupy Add */
#endif /* SAFETY_QUAD *//* Rupy mod end */
	
#if 0 /* IDPower */
	const char *omap_1 = "i2c_omap.1";
	const char *omap_2 = "i2c_omap.2";
	const char *omap_3 = "i2c_omap.3";
	const char *omap_4 = "i2c_omap.4";
	int i;
		
	/*initialize struct device*/
	dev1 = kmalloc(sizeof(struct device), GFP_KERNEL);
	dev2 = kmalloc(sizeof(struct device), GFP_KERNEL);
	dev3 = kmalloc(sizeof(struct device), GFP_KERNEL);
	dev4 = kmalloc(sizeof(struct device), GFP_KERNEL);
	
	if (dev1 == NULL || dev2 == NULL || dev3 == NULL || dev4 == NULL){
		printk("*** %s : kmalloc error\n", __func__);
		return -EIO;
	}
	
	/* set i2c device name */
	dev1->init_name = omap_1;
	dev2->init_name = omap_2;
	dev3->init_name = omap_3;
	dev4->init_name = omap_4;
	
	/* get i2c clock */
	clk_get_ret[I2C_1] = clk_get(dev1,"fck");
	clk_get_ret[I2C_2] = clk_get(dev2,"fck");
	clk_get_ret[I2C_3] = clk_get(dev3,"fck");
	clk_get_ret[I2C_4] = clk_get(dev4,"fck");
	for (i = 0; i < I2C_NUM_IF; i++){
		if(clk_get_ret[i] == NULL){
			printk("*** %s : glk_get() error\n", __func__);
			return -EIO;
		}
	}
#endif
	
	csv_init_flg = 1;

	CSV_LOG_FUNCOUT("\n");
	return 0;
}
late_initcall(csv_init);
