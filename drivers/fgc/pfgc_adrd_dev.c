/*
 * drivers/fgc/pfgc_adrd_dev.c
 *
 * Copyright(C) 2010 Panasonic Mobile Communications Co., Ltd.
 * Author: Naoto Suzuki <suzuki.naoto@kk.jp.panasonic.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "inc/pfgc_local.h"

static struct t_fgc_device_info *dev_info;
/* IDPower #12 static void __iomem* virt_base; */

static struct class *fgc_device_class;

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1201XX*/
unsigned char degra_batt_select_val = DFGC_SOH_INIT_INIT; /*                 [%] *//*<Conan>1201XX*/
#endif /*<Conan>1201XX*/

/*!
 Initialize i2c and sysfs.
 @return  0  success
          1  unsuccess
 @param   Nothing
 */
unsigned char pfgc_adrd_init( void )
{
	unsigned char ret;
	int modret;
	ret = DFGC_OK;

	gfgc_ctl.get_capacity  = pfgc_get_capcity_ini;
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
	gfgc_ctl.get_degradation_info = pfgc_get_degradation_info; // Conan
#endif /*<Conan>1111XX*/

	modret = pfgc_i2c_init();
	if (modret != 0) {
		ret = DFGC_NG;
		return ret;
	}

	modret = pfgc_class_register();
	if (modret != 0) {
		ret = DFGC_NG;
		return ret;
	}

#if 0 /* IDPower #12 */
	virt_base = ioremap(OMAP4_GPIO6_BASE, OMAP4_GPIO_AS_LEN);
	if (virt_base == NULL) {
		ret = DFGC_NG;
	}
#endif

	return ret;
}

/*!
 Terminate i2c and sysfs.
 @return  Nothing
 @param   Nothing
 */
void pfgc_adrd_exit( void )
{
/* IDPower #12	iounmap(virt_base); */
	pfgc_class_unregister();
	pfgc_i2c_exit();
}

#if 0 /* IDPower #12 */
/*!
 Get GPIO2 register address.
 @return  register's virtual address
 @param[in] offset register offset
 */
void __iomem* pfgc_get_reg_addr( unsigned int offset )
{
	return (virt_base + offset);
}
#endif

/*!
 Enable IRQ Wakeup.
 @return  0 success
          other unsuccess
 @param   Nothing
 */
int pfgc_enable_irq_wake( void )
{
	int ret;
	int irq_no;

	irq_no = gpio_to_irq(DFGC_PORT_NO);
	if (irq_no < 0) {
		FGC_ERR("gpio_to_irq() failed. ret=%d\n", irq_no);
		ret = -DFGC_NG;
	}
	else {
		ret = enable_irq_wake( irq_no );
	}
	return ret;
}

/*!
 Request IRQ.
 @return  0 success
          other unsuccess
 @param[in] port_callback interrupt function
 */
int pfgc_request_irq( irqreturn_t(*port_callback)(int irq, void *dev_id) )
{

	int ret, irq_no;
	
	FGC_FUNCIN("\n");

	dev_info = kzalloc(sizeof(*dev_info), GFP_KERNEL);		/* mrc23704 *//* mrc22401 */
	if (!dev_info) {
		FGC_ERR("kzalloc() failed.\n");
		ret = -ENOMEM;
		return ret;
	}

	gpio_free(DFGC_PORT_NO);

	ret = gpio_request(DFGC_PORT_NO, "DINT_FG_INT");
	if (ret != 0) {
		FGC_ERR("gpio_request() failed. ret=%d\n", ret);
		return ret;
	}

	ret = gpio_direction_input(DFGC_PORT_NO);
	if (ret != 0) {
		FGC_ERR("gpio_direction_input() failed. ret=%d\n", ret);
		return ret;
	}

	irq_no = gpio_to_irq(DFGC_PORT_NO);
	if (irq_no < 0) {
		FGC_ERR("gpio_to_irq() failed. ret=%d\n", irq_no);
		ret = irq_no;
		return ret;
	}

	ret = request_irq(irq_no, port_callback, IRQF_TRIGGER_RISING,
			DFGC_DRIVER_NAME, dev_info);
	if (ret != 0) {
		FGC_ERR("request_irq() failed. ret=%d\n", ret);
		return ret;
	}

	FGC_FUNCOUT("\n");
	return ret;
}

/*!
 Free IRQ.
 @return  Nothing
 @param   Nothing
 */
void pfgc_free_irq( void )
{
	int irq_no;

	irq_no = gpio_to_irq(DFGC_PORT_NO);
	if (irq_no < 0) {
		FGC_ERR("gpio_to_irq() failed. ret=%d\n", irq_no);
	}
	else {
		free_irq(irq_no, dev_info);
	}
	gpio_free(DFGC_PORT_NO);

	return;
}

/*!
 Register character device.
 @return  0 success
          -ENOMEM Out of memory
          other unsuccess
 @param[in] major device major no
 @param[in] name  driver name
 @param[in] fops file operations
 */
int pfgc_register_chrdev( unsigned int major, const char *name, struct file_operations *fops )
{
	int ret, add_cnt, del_cnt;
	const char *dev_name[] = {"info", "df", "ver", "fw", "dfs"};
	struct device *fgc_device;

	ret = register_chrdev(major, name, fops);
	if (ret < 0) {
		FGC_ERR("register_chrdev() failed. ret=%d\n", ret);
		return ret;
	}

	fgc_device_class = class_create(THIS_MODULE, "fg");
	if (IS_ERR(fgc_device_class)) {
		ret = PTR_ERR(fgc_device_class);
		FGC_ERR("class_create() failed. ret=%d\n", ret);
		unregister_chrdev(major, name);
		return ret;
	}

	for (add_cnt = 0; add_cnt < DFGC_MINOR_NUM_MAX; add_cnt++) {
		fgc_device = device_create(fgc_device_class, NULL,
				MKDEV(major, add_cnt), NULL, dev_name[add_cnt] );
		if (IS_ERR(fgc_device)) {
			ret = PTR_ERR(fgc_device);
			FGC_ERR("device_create(%s) failed. ret=%d\n", dev_name[add_cnt], ret);

			for (del_cnt = 0; del_cnt < add_cnt; del_cnt++ ) {
				device_destroy(fgc_device_class, MKDEV(major, del_cnt));
			}
			class_destroy(fgc_device_class);
			unregister_chrdev(major, name);
			break;
		}
	}

	return ret;
}

/*!
 Unregister character device.
 @return  Nothing
 @param[in] major device major no
 @param[in] name deriver name
 */
void pfgc_unregister_chrdev( unsigned int major, const char *name )
{
	int cnt;

	for (cnt = 0; cnt < DFGC_MINOR_NUM_MAX; cnt++) {
		device_destroy(fgc_device_class, MKDEV(major, cnt));
	}
	class_destroy(fgc_device_class);
	unregister_chrdev(major, name);
}

/*!
 Get capacity.
 @return  battery capacity
 @param   Nothing
 */
int pfgc_get_capcity( void )
{
/*<npdc300030059>	return gfgc_batt_info.capacity; */
	return gfgc_batt_info.soc; /*<npdc300030059>*/
}

/*!
 Get capacity without notify low battery alarm.
 @return  battery capacity, 15% over
 @param   Nothing
 */
int pfgc_get_capcity_nolva( void )
{
	int ret_capacity;

	ret_capacity = pfgc_get_capcity();
	if(ret_capacity <= DFGC_CAPACITY_LVA) {
		ret_capacity = DFGC_CAPACITY_LVA + 1;
	}
	return ret_capacity;
}

/*!
 Get capacity function before acquiring no_lva flag.
 @return  battery capacity
 @param   Nothing
 */
int pfgc_get_capcity_ini( void )
{
#if 1 /* IDPower npdc300034186 */
	unsigned int *no_lva_flg = NULL;
	
	no_lva_flg = ioremap(DMEM_NO_LVA_FLG, DMEM_NO_LVA_FLG_SIZE);
	if (no_lva_flg == NULL) {
		return pfgc_get_capcity();
	}
	else{
		gfgc_ctl.no_lva = (unsigned char)*no_lva_flg;
	}
	pr_info("gfgc_ctl.no_lva = 0x%x\n", gfgc_ctl.no_lva);
#else /* IDPower npdc300034186 */
	int ret;

	ret = cfgdrv_read(D_HCM_E_NO_LVA_FLG, 1, &gfgc_ctl.no_lva);
	if (ret != 0) {
		return pfgc_get_capcity();
	}
#endif /* IDPower npdc300034186 */
	
	if (gfgc_ctl.no_lva) {
		gfgc_ctl.get_capacity = pfgc_get_capcity_nolva;
	} else {
		gfgc_ctl.get_capacity = pfgc_get_capcity;
	}

	return gfgc_ctl.get_capacity();
}

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
// Conan Add Start
/*!
 Get degradation info.
 @return  battery degradation info
 @param   Properties ID
 */
int pfgc_get_degradation_info( int property_id )
{
    if( gfgc_ctl.remap_soh == NULL )
    {
        FGC_ERR("Get degradation info failed. \n");
        return 0;
    }

    if (property_id == POWER_SUPPLY_PROP_DEGRADATION_STATUS) { /* 1201XX */
        FGC_LOG("Get degradation status select_val=%d. \n", degra_batt_select_val);
        return degra_batt_select_val;
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_DECISION) {
        FGC_LOG("Get degradation status soh_health=%d. \n",gfgc_ctl.remap_soh->soh_health);
        return gfgc_ctl.remap_soh->soh_health;
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_MEASUREMENT) { /* 1201XX */
        FGC_LOG("Get degradation status degra_batt_measure_val=%d. \n",gfgc_ctl.remap_soh->degra_batt_measure_val);
        return gfgc_ctl.remap_soh->degra_batt_measure_val;
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_ESTIMATION) {
        FGC_LOG("Get degradation status degra_batt_estimate_val=%d. \n",gfgc_ctl.remap_soh->degra_batt_estimate_val);
        return gfgc_ctl.remap_soh->degra_batt_estimate_val;
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_CYCLE) {
        FGC_LOG("Get degradation status cycle_degra_batt_capa=%d. \n",gfgc_ctl.remap_soh->cycle_degra_batt_capa);
        return gfgc_ctl.remap_soh->cycle_degra_batt_capa;
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_STORE) {
        FGC_LOG("Get degradation status soh_degra_batt_capa=%d. \n",gfgc_ctl.remap_soh->soh_degra_batt_capa);
        return gfgc_ctl.remap_soh->soh_degra_batt_capa;
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_TIMESTAMP_HIGHBIT) {
        FGC_LOG("Get degradation status save_soh_degra_timestamp=0x%llx. \n",gfgc_ctl.save_soh_degra_timestamp);
        return (int)((gfgc_ctl.save_soh_degra_timestamp >> 32) & 0x00000000FFFFFFFF);/*<Conan time add>*/
    }
    else if (property_id == POWER_SUPPLY_PROP_DEGRADATION_TIMESTAMP_LOWBIT) {
        FGC_LOG("Get degradation status save_soh_degra_timestamp=0x%llx. \n",gfgc_ctl.save_soh_degra_timestamp);
        return (int)(gfgc_ctl.save_soh_degra_timestamp & 0x00000000FFFFFFFF);/*<Conan time add>*/
    }
    else {
        return 0;
    }
}
// Conan Add End
#endif /*<Conan>1111XX*/
