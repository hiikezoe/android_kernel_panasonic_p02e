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
#include <linux/bitops.h>
#include <linux/io.h>
#include <mach/msm_iomap.h>
#include <mach/gpiomux.h>

#define GPIO_CFG(n)    (MSM_TLMM_BASE + 0x1000 + (0x10 * n))
#define GPIO_IN_OUT(n) (MSM_TLMM_BASE + 0x1004 + (0x10 * n))

/* <npdc300018062>  from here okajima 2012.4.12                 */
/* brief: to change setting for GPIO_IN_OUT resistor by GPIO No */
#define IN_CAMERA_GPIO 1
#define OUT_CAMERA_GPIO 15
/* <npdc300018062> to here okajima 2012.4.12                     */

void __msm_gpiomux_write(unsigned gpio, struct gpiomux_setting val)
{
	uint32_t bits;
	uint32_t gpio_in_out_value = 0;

/* <npdc300018062>  from here okajima 2012.4.12                 */
/* brief: to change setting for GPIO_IN_OUT resistor by GPIO No */
	if ((val.dir == GPIOMUX_OUT_HIGH) && (gpio != IN_CAMERA_GPIO) && (gpio != OUT_CAMERA_GPIO))
		gpio_in_out_value = BIT(1);
/* <npdc300018062> to here okajima 2012.4.12                     */

	bits = (val.drv << 6) | (val.func << 2) | val.pull;
	if (val.func == GPIOMUX_FUNC_GPIO) {
		bits |= val.dir > GPIOMUX_IN ? BIT(9) : 0;
/* <npdc300018062>  from here okajima 2012.4.12                 */
/* brief: to change setting for GPIO_IN_OUT resistor by GPIO No */
		/*__raw_writel(val.dir == GPIOMUX_OUT_HIGH ? BIT(1) : 0,*/
		__raw_writel(gpio_in_out_value,
/* <npdc300018062> to here okajima 2012.4.12                     */
			GPIO_IN_OUT(gpio));
	}
	__raw_writel(bits, GPIO_CFG(gpio));
	mb();
}
