/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/vibrator.h>
#include "thp7212_pm8921.h"

struct pm8921_redled {
	spinlock_t lock;
	struct work_struct work;
	struct device *dev;
	const struct pm8xxx_vibrator_platform_data *pdata;
	int state;
	int level;
	u8  reg_vib_drv;
};

static struct pm8921_redled *vib_dev;

#define VIB_DRV_SEL_MASK	0xf8
#define VIB_DRV			0x4A

int thp7212_pm8921_red_led( int onoff )
{
	int rc;
	u8 val=0;

	rc = pm8xxx_readb(vib_dev->dev->parent, VIB_DRV, &val);
	if (rc){
		dev_warn(vib_dev->dev, "Error reading pm8xxx: %X - ret %X\n",
				onoff, rc);
		return rc;
	}
	
	if (onoff == PM8921_REDLED_ON){
	/* Red LED ON */
		val = val | VIB_DRV_SEL_MASK;
//		printk("###### RED LED ON val = %x",val);
		rc = pm8xxx_writeb(vib_dev->dev->parent, VIB_DRV, val);
	}else{
	/* Red LED OFF */
		val &= ~VIB_DRV_SEL_MASK;
//		printk("###### RED LED OFF val = %x",val);
		rc = pm8xxx_writeb(vib_dev->dev->parent, VIB_DRV, val);
	}

	if (rc){
		dev_warn(vib_dev->dev, "Error writing pm8xxx: %X - ret %X\n",
				onoff, rc);
	}
	return rc;
}

static int __devinit pm8921_redled_probe(struct platform_device *pdev)

{
	const struct pm8xxx_vibrator_platform_data *pdata =
						pdev->dev.platform_data;
	struct pm8921_redled *vib;

	vib = kzalloc(sizeof(*vib), GFP_KERNEL);
	if (!vib)
		return -ENOMEM;

	vib->pdata	= pdata;
	vib->level	= 3000 / 100;		/* 3000mV */
	vib->dev	= &pdev->dev;

	spin_lock_init(&vib->lock);

	platform_set_drvdata(pdev, vib);

	vib_dev = vib;

	return 0;
}

static int __devexit pm8921_redled_remove(struct platform_device *pdev)
{
	struct pm8921_redled *vib = platform_get_drvdata(pdev);

	cancel_work_sync(&vib->work);
	platform_set_drvdata(pdev, NULL);
	kfree(vib);

	return 0;
}

static struct platform_driver pm8921_redled_driver = {
	.probe		= pm8921_redled_probe,
	.remove		= __devexit_p(pm8921_redled_remove),
	.driver		= {
		.name	= PM8XXX_VIBRATOR_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8921_redled_init(void)
{
	return platform_driver_register(&pm8921_redled_driver);
}
module_init(pm8921_redled_init);

static void __exit pm8921_redled_exit(void)
{
	platform_driver_unregister(&pm8921_redled_driver);
}
module_exit(pm8921_redled_exit);

MODULE_ALIAS("platform:" PM8XXX_VIBRATOR_DEV_NAME);
MODULE_DESCRIPTION("pm8921 Red LED driver");
MODULE_LICENSE("GPL v2");
