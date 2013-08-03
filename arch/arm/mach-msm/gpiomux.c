/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
#if 0
#define DEBUG
#define PT_DEBUG
#endif

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <mach/gpiomux.h>
#ifdef   PT_DEBUG
#include <mach/msm_iomap.h>
#include <asm/io.h>
#endif /*PT_DEBUG*/

struct msm_gpiomux_rec {
	struct gpiomux_setting *sets[GPIOMUX_NSETTINGS];
	int ref;
};
static DEFINE_SPINLOCK(gpiomux_lock);
static struct msm_gpiomux_rec *msm_gpiomux_recs;
static struct gpiomux_setting *msm_gpiomux_sets;
static unsigned msm_gpiomux_ngpio;

int msm_gpiomux_write(unsigned gpio, enum msm_gpiomux_setting which,
	struct gpiomux_setting *setting, struct gpiomux_setting *old_setting)
{
	struct msm_gpiomux_rec *rec = msm_gpiomux_recs + gpio;
	unsigned set_slot = gpio * GPIOMUX_NSETTINGS + which;
	unsigned long irq_flags;
	struct gpiomux_setting *new_set;
	int status = 0;

#ifdef  PT_DEBUG
	u32 val;
#endif /*PT_DEBUG*/
	if (!msm_gpiomux_recs)
		return -EFAULT;

	if (gpio >= msm_gpiomux_ngpio)
		return -EINVAL;

	spin_lock_irqsave(&gpiomux_lock, irq_flags);

	if (old_setting) {
		if (rec->sets[which] == NULL)
			status = 1;
		else
			*old_setting =  *(rec->sets[which]);
	}

	if (setting) {
		msm_gpiomux_sets[set_slot] = *setting;
		rec->sets[which] = &msm_gpiomux_sets[set_slot];
	} else {
		rec->sets[which] = NULL;
	}

	new_set = rec->ref ? rec->sets[GPIOMUX_ACTIVE] :
		rec->sets[GPIOMUX_SUSPENDED];
	if (new_set)
		__msm_gpiomux_write(gpio, *new_set);

	spin_unlock_irqrestore(&gpiomux_lock, irq_flags);
#ifdef  PT_DEBUG
#define GPIO_CONFIG(gpio)         (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)         (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))
	if (new_set) {
		pr_debug("%s:%d [%s]gpio=%d ref=%d func=%d,drv=%d,pull=%d,dir=%d\n",
				__func__, __LINE__, which == GPIOMUX_ACTIVE ? "GPIOMUX_ACTIVE" : "GPIOMUX_SUSPENDED",
				gpio, rec->ref,
				new_set->func, new_set->drv, new_set->pull,
				new_set->dir);
	}
	else {
		pr_debug("%s:%d [%s]gpio=%d ref=%d not set\n",
				__func__, __LINE__, which == GPIOMUX_ACTIVE ? "GPIOMUX_ACTIVE" : "GPIOMUX_SUSPENDED",
				gpio, rec->ref);
	}

	val = __raw_readl(GPIO_CONFIG(gpio));
	pr_info("GPIO_CONFIG(%03d):0x%8p:%08x\n",
			gpio, GPIO_CONFIG(gpio), val);
	val = __raw_readl(GPIO_IN_OUT(gpio));
	pr_info("GPIO_IN_OUT(%03d):0x%8p:%08x\n",
			gpio, GPIO_IN_OUT(gpio), val);
#endif /*PT_DEBUG*/
	return status;
}
EXPORT_SYMBOL(msm_gpiomux_write);

int msm_gpiomux_get(unsigned gpio)
{
	struct msm_gpiomux_rec *rec = msm_gpiomux_recs + gpio;
	unsigned long irq_flags;

#ifdef  PT_DEBUG
	u32 val;
#define GPIO_CONFIG(gpio)         (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)         (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))
#endif /*PT_DEBUG*/

	if (!msm_gpiomux_recs)
		return -EFAULT;

	if (gpio >= msm_gpiomux_ngpio)
		return -EINVAL;

#ifdef  PT_DEBUG
	pr_info( "Before Activate");
	val = __raw_readl(GPIO_CONFIG(gpio));
	pr_info("GPIO_CONFIG(%03d):0x%8p:%08x\n",
			gpio, GPIO_CONFIG(gpio), val);
	val = __raw_readl(GPIO_IN_OUT(gpio));
	pr_info("GPIO_IN_OUT(%03d):0x%8p:%08x\n",
			gpio, GPIO_IN_OUT(gpio), val);
#endif /*PT_DEBUG*/
	spin_lock_irqsave(&gpiomux_lock, irq_flags);
	if (rec->ref++ == 0 && rec->sets[GPIOMUX_ACTIVE])
		__msm_gpiomux_write(gpio, *rec->sets[GPIOMUX_ACTIVE]);
	spin_unlock_irqrestore(&gpiomux_lock, irq_flags);
#ifdef  PT_DEBUG
	pr_info( "After Activate");
	val = __raw_readl(GPIO_CONFIG(gpio));
	pr_info("GPIO_CONFIG(%03d):0x%8p:%08x\n",
			gpio, GPIO_CONFIG(gpio), val);
	val = __raw_readl(GPIO_IN_OUT(gpio));
	pr_info("GPIO_IN_OUT(%03d):0x%8p:%08x\n",
			gpio, GPIO_IN_OUT(gpio), val);
#endif /*PT_DEBUG*/
	return 0;
}
EXPORT_SYMBOL(msm_gpiomux_get);

int msm_gpiomux_put(unsigned gpio)
{
	struct msm_gpiomux_rec *rec = msm_gpiomux_recs + gpio;
	unsigned long irq_flags;

#ifdef  PT_DEBUG
	u32 val;
#define GPIO_CONFIG(gpio)         (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)         (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))
#endif /*PT_DEBUG*/
	if (!msm_gpiomux_recs)
		return -EFAULT;

	if (gpio >= msm_gpiomux_ngpio)
		return -EINVAL;

#ifdef  PT_DEBUG
	pr_info( "Before Suspend");
	val = __raw_readl(GPIO_CONFIG(gpio));
	pr_info("GPIO_CONFIG(%03d):0x%8p:%08x\n",
			gpio, GPIO_CONFIG(gpio), val);
	val = __raw_readl(GPIO_IN_OUT(gpio));
	pr_info("GPIO_IN_OUT(%03d):0x%8p:%08x\n",
			gpio, GPIO_IN_OUT(gpio), val);
#endif /*PT_DEBUG*/
	spin_lock_irqsave(&gpiomux_lock, irq_flags);
	BUG_ON(rec->ref == 0);
	if (--rec->ref == 0 && rec->sets[GPIOMUX_SUSPENDED])
		__msm_gpiomux_write(gpio, *rec->sets[GPIOMUX_SUSPENDED]);
	spin_unlock_irqrestore(&gpiomux_lock, irq_flags);
#ifdef  PT_DEBUG
	pr_info( "After Suspend");
	val = __raw_readl(GPIO_CONFIG(gpio));
	pr_info("GPIO_CONFIG(%03d):0x%8p:%08x\n",
			gpio, GPIO_CONFIG(gpio), val);
	val = __raw_readl(GPIO_IN_OUT(gpio));
	pr_info("GPIO_IN_OUT(%03d):0x%8p:%08x\n",
			gpio, GPIO_IN_OUT(gpio), val);
#endif /*PT_DEBUG*/
	return 0;
}
EXPORT_SYMBOL(msm_gpiomux_put);

int msm_gpiomux_init(size_t ngpio)
{
	if (!ngpio)
		return -EINVAL;

	if (msm_gpiomux_recs)
		return -EPERM;

	msm_gpiomux_recs = kzalloc(sizeof(struct msm_gpiomux_rec) * ngpio,
				   GFP_KERNEL);
	if (!msm_gpiomux_recs)
		return -ENOMEM;

	/* There is no need to zero this memory, as clients will be blindly
	 * installing settings on top of it.
	 */
	msm_gpiomux_sets = kmalloc(sizeof(struct gpiomux_setting) * ngpio *
		GPIOMUX_NSETTINGS, GFP_KERNEL);
	if (!msm_gpiomux_sets) {
		kfree(msm_gpiomux_recs);
		msm_gpiomux_recs = NULL;
		return -ENOMEM;
	}

	msm_gpiomux_ngpio = ngpio;

	return 0;
}
EXPORT_SYMBOL(msm_gpiomux_init);

void msm_gpiomux_install(struct msm_gpiomux_config *configs, unsigned nconfigs)
{
	unsigned c, s;
	int rc;
#ifdef  PT_DEBUG
	struct gpiomux_setting *new_set;
#endif /*PT_DEBUG*/

	for (c = 0; c < nconfigs; ++c) {
		for (s = 0; s < GPIOMUX_NSETTINGS; ++s) {
			rc = msm_gpiomux_write(configs[c].gpio, s,
				configs[c].settings[s], NULL);
			if (rc)
				pr_err("%s: write failure: %d\n", __func__, rc);
#ifdef  PT_DEBUG
			new_set = configs[c].settings[s];
			if (new_set) {
				pr_debug("%s:%d gpio=%d ref=%d func=%d,drv=%d,pull=%d,dir=%d\n",
						__func__, __LINE__, configs[c].gpio, s,
						new_set->func, new_set->drv, new_set->pull,
						new_set->dir);
			}
			else {
				pr_debug("%s:%d ref=%d no setting\n",
						__func__, __LINE__, s);
			}
#endif /*PT_DEBUG*/
		}
	}
}
EXPORT_SYMBOL(msm_gpiomux_install);
