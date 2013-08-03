/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#if 0
#define DEBUG
#define PT_DEBUG
#endif

#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "board-8960.h"
#ifdef   PT_DEBUG
#include <mach/msm_iomap.h>
#include <asm/io.h>
#endif /*PT_DEBUG*/

/* The SPI configurations apply to GSBI 1*/
#if 0  /* IDPower GPIO ISV */
static struct gpiomux_setting spi_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting spi_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif /* IDPower GPIO ISV */

#if 0 /* GPIO_14 is not for SPI on Jessie */
static struct gpiomux_setting spi_active_config2 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting spi_suspended_config2 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif /* GPIO_14 */

static struct gpiomux_setting gsbi3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

/* <SlimAV1 Ver UP> Mod Start */
#if 0
static struct gpiomux_setting external_vfr[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
};
#endif
/* <SlimAV1 Ver UP> Mod End */

static struct gpiomux_setting gsbi_uart = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
/* IDPower for disabled gsbi ports */
static struct gpiomux_setting gsbi_disabled_pu = {
	.func = GPIOMUX_FUNC_GPIO,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting gsbi_disabled_pd = {
	.func = GPIOMUX_FUNC_GPIO,
	.pull = GPIOMUX_PULL_DOWN,
};


/* <SlimAV1> ADD START */
static struct gpiomux_setting gsbi8_3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting gsbi8_3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi8_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.pull = GPIOMUX_PULL_NONE,
};
/* <SlimAV1> ADD END */

// <iDPower_002> add S
#if 0
static struct gpiomux_setting gsbi9_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gsbi9_suspended_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#else
static struct gpiomux_setting gsbi9 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
// <iDPower_002> add E

static struct gpiomux_setting gsbi10 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

/* IDPower Start */
static struct gpiomux_setting gsbi1_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};
static struct gpiomux_setting gsbi1_suspend_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
/* IDPower end */

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

// <SlimAV1> del S
#if 0
static struct gpiomux_setting audio_auxpcm[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};
#endif
// <SlimAV1> del E

/* <SlimAV1> Add Start */
#if 0
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_DOWN, /* <npdc300034196> Fix GPIO setting  */
	.drv = GPIOMUX_DRV_2MA, /* <npdc300034196> Fix GPIO setting  */
	.func = GPIOMUX_FUNC_GPIO,
};
#endif
#else
static struct gpiomux_setting gpio_camera_mr_config = {
	.pull = GPIOMUX_PULL_NONE,
	.func = GPIOMUX_FUNC_GPIO,
};
#endif
/* <SlimAV1> Add End */

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_resout_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_resout_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting synaptics_sleep_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting synaptics_sleep_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting synaptics_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting synaptics_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_hub_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting hap_lvl_shft_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hap_lvl_shft_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

/* <SlimAV1 Ver UP> Add Start */
#if 0
static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
/* <SlimAV1 Ver UP> Add End */

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

/* <SlimAV1 Ver UP> Mod Start */
#if 0
static struct gpiomux_setting ap2mdm_kpdpwr_n_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
/* <SlimAV1 Ver UP> Mod End */

/* <SlimAV1 Ver UP> Add Start */
#if 0
static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif
/* <SlimAV1 Ver UP> Add End */

// <SlimAV1> add S
static struct gpiomux_setting flashled_cfg = {
    .func = GPIOMUX_FUNC_GPIO,  /* GPIO2/3/64 */
    .drv = GPIOMUX_DRV_8MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};
// <SlimAV1> add E

/* <SlimAV1> ADD START */
static struct gpiomux_setting wchg_uart_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO18 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting irda_pdn_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO38 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting wchg_fetcnt_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO51 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting compass_sensor_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO53 */
	.dir = GPIOMUX_OUT_HIGH,
};
static struct gpiomux_setting compass_sensor_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO53 */
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting wchg_ledmask_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO54 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};
static struct gpiomux_setting wchg_ledmask_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO54 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting wchg_ls_en_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO63 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};
static struct gpiomux_setting wchg_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* GPIO76 */
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
/* <SlimAV1> ADD END */

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#if defined(CONFIG_FB_MSM_HDMI_MHL_8334) || defined(CONFIG_FB_MSM_HDMI_MHL_9244)
static struct gpiomux_setting hdmi_active_3_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hdmi_active_4_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif
#endif

/* for mmtuner RESET port (GPIO_14) */
static struct gpiomux_setting mmtuner_reset_suspend = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting mmtuner_reset_active = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
/* mmtuner kokomade */

/* <SlimAV1> Add Start */
#if 0
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct msm_gpiomux_config msm8960_ethernet_configs[] = {
	{
		.gpio = 90,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
};
#endif
#else
static struct msm_gpiomux_config msm8960_camera_mr_configs[] = {
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_camera_mr_config,
		}
	},
	{
		.gpio = 90,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_camera_mr_config,
		}
	},
};
#endif
/* <SlimAV1> Add End */

static struct msm_gpiomux_config msm8960_fusion_gsbi_configs[] = {
	{
		.gpio = 93,
		.settings = {
/* <SlimAV1 Ver UP> Mod Start */
			[GPIOMUX_SUSPENDED] = &gsbi9,
//			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
//			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9,
//			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
//			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
#if 0
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 96,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
#endif
/* <SlimAV1 Ver UP> Mod End */
};

static struct msm_gpiomux_config msm8960_gsbi_configs[] __initdata = {
	{
		.gpio      = 6,		/* GSBI1 QUP SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pu, /* IDPower */
			[GPIOMUX_ACTIVE] = &gsbi_disabled_pu,    /* IDPower */
		},
	},
	{
		.gpio      = 7,		/* GSBI1 QUP SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pu, /* IDPower */
			[GPIOMUX_ACTIVE] = &gsbi_disabled_pu,    /* IDPower */
		},
	},
	{
		.gpio      = 8,		/* GSBI1 QUP SPI_CS_N */
		.settings = {
/*IDPower			[GPIOMUX_SUSPENDED] = &spi_suspended_config, */
/*IDPower			[GPIOMUX_ACTIVE] = &spi_active, */
			[GPIOMUX_SUSPENDED] = &gsbi1_suspend_cfg, /* IDPower */
			[GPIOMUX_ACTIVE] = &gsbi1_active_cfg, /* IDPower */
		},
	},
	{
		.gpio      = 9,		/* GSBI1 QUP SPI_CLK */
		.settings = {
/*IDPower			[GPIOMUX_SUSPENDED] = &spi_suspended_config, */
/*IDPower			[GPIOMUX_ACTIVE] = &spi_active, */
			[GPIOMUX_SUSPENDED] = &gsbi1_suspend_cfg, /* IDPower */
			[GPIOMUX_ACTIVE] = &gsbi1_active_cfg, /* IDPower */
		},
	},
	{
		.gpio      = 14,		/* GSBI1 SPI_CS_1 */
		.settings = {
//			[GPIOMUX_SUSPENDED] = &spi_suspended_config2,
			[GPIOMUX_SUSPENDED] = &mmtuner_reset_suspend,                      /* MMTunerリセット端子 default=LOW */
//			[GPIOMUX_ACTIVE] = &spi_active_config2,
			[GPIOMUX_ACTIVE] = &mmtuner_reset_active,                          /* MMTunerリセット端子 default=LOW */
		},
	},
	{
		.gpio      = 16,	/* GSBI3 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 17,	/* GSBI3 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 44,	/* GSBI12 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 45,	/* GSBI12 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 73,	/* GSBI10 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		.gpio      = 74,	/* GSBI10 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
};

static struct msm_gpiomux_config msm8960_gsbi5_uart_configs[] __initdata = {
	{
		.gpio      = 22,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 23,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 24,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pd,
		},
	},
	{
		.gpio      = 25,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pd,
		},
	},
};

/* <SlimAV1 Ver UP> Mod Start */
#if 0
static struct msm_gpiomux_config msm8960_external_vfr_configs[] __initdata = {
	{
		.gpio      = 23,        /* EXTERNAL VFR */
		.settings = {
			[GPIOMUX_SUSPENDED] = &external_vfr[0],
			[GPIOMUX_ACTIVE] = &external_vfr[1],
		},
	},
};
#endif
/* <SlimAV1 Ver UP> Mod End */

static struct msm_gpiomux_config msm8960_gsbi8_uart_configs[] __initdata = {
	{
		.gpio      = 34,        /* GSBI8 UART3 */
		.settings = {
/* <SlimAV1 Ver UP> Mod Start */
//			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pd,
			[GPIOMUX_SUSPENDED] = &gsbi8_3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi8_3_active_cfg,
		},
	},
	{
		.gpio      = 35,        /* GSBI8 UART3 */
		.settings = {
//			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pd,
			[GPIOMUX_SUSPENDED] = &gsbi8_2_cfg,
		},
	},
#if 0
	{
		.gpio      = 36,        /* GSBI8 UART3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pd,
		},
	},
	{
		.gpio      = 37,        /* GSBI8 UART3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_disabled_pd,
		},
	},
#endif
/* <SlimAV1 Ver UP> Mod End */
};

static struct msm_gpiomux_config msm8960_slimbus_config[] __initdata = {
	{
		.gpio	= 60,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= 61,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct msm_gpiomux_config msm8960_audio_codec_configs[] __initdata = {
	{
		.gpio = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};

// <SlimAV1> del S
#if 0
static struct msm_gpiomux_config msm8960_audio_auxpcm_configs[] __initdata = {
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
};
#endif
// <SlimAV1> del E

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8960_synaptics_configs[] __initdata = {
	{	/* TS SLEEP */
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &synaptics_sleep_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_sleep_sus_cfg,
		},
	},
	{	/* TP PENIRQ_N */
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &synaptics_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_int_sus_cfg,
		},
	},
	{	/* TS RESOUT */
		.gpio = 52,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_resout_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_resout_sus_cfg,
		},
	},
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct msm_gpiomux_config msm8960_hsic_configs[] = {
	{
		.gpio = 150,               /*HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 151,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8960_hsic_hub_configs[] = {
	{
		.gpio = 91,               /* HSIC_HUB_RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_hub_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting sdcc4_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc4_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc4_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdcc4_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8960_sdcc4_configs[] __initdata = {
	{
		/* SDC4_DATA_3 */
		.gpio      = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_2 */
		.gpio      = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_1 */
		.gpio      = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_data_1_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_0 */
		.gpio      = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_CMD */
		.gpio      = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_CLK */
		.gpio      = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
};
#endif


static struct msm_gpiomux_config hap_lvl_shft_config[] __initdata = {
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_SUSPENDED] = &hap_lvl_shft_suspended_config,
			[GPIOMUX_ACTIVE] = &hap_lvl_shft_active_config,
		},
	},
};

static struct msm_gpiomux_config sglte_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
/* <SlimAV1 Ver UP> Mod Start */
#if 0
	/* MDM2AP_STATUS */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
#endif
/* <SlimAV1 Ver UP> Mod End */
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
/* <SlimAV1 Ver UP> Mod Start */
#if 0
	/* AP2MDM_KPDPWR_N */
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
	/* AP2MDM_PMIC_PWR_EN */
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET */
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
#endif
/* <SlimAV1 Ver UP> Mod End */
};

/* <SlimAV1 Ver UP> Add Start */
#if 0
static struct msm_gpiomux_config msm8960_mdp_vsync_configs[] __initdata = {
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	}
};
#endif
/* <SlimAV1 Ver UP> Add End */

// <SlimAV1> add S
static struct msm_gpiomux_config msm8960_flashled_configs[] __initdata = {
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &flashled_cfg,
			[GPIOMUX_SUSPENDED] = &flashled_cfg,
		},
	},
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &flashled_cfg,
			[GPIOMUX_SUSPENDED] = &flashled_cfg,
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_ACTIVE]    = &flashled_cfg,
			[GPIOMUX_SUSPENDED] = &flashled_cfg,
		},
	},
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &flashled_cfg,
			[GPIOMUX_SUSPENDED] = &flashled_cfg,
		},
	},
};
// <SlimAV1> add E

/* <SlimAV1> ADD START */
static struct msm_gpiomux_config wireless_charge_configs[] __initdata = {
	{
		.gpio      = 18,
		.settings = {
			[GPIOMUX_SUSPENDED] = &wchg_uart_cfg
		},
	},
	{
		.gpio      = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &wchg_fetcnt_cfg
		},
	},
	{
		.gpio      = 53,
		.settings = {
			[GPIOMUX_ACTIVE] = &compass_sensor_act_cfg,
			[GPIOMUX_SUSPENDED] = &compass_sensor_sus_cfg
		},
	},
	{
		.gpio      = 54,
		.settings = {
			[GPIOMUX_ACTIVE] = &wchg_ledmask_act_cfg,
			[GPIOMUX_SUSPENDED] = &wchg_ledmask_sus_cfg
		},
	},
	{
		.gpio      = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &wchg_ls_en_cfg
		},
	},
	{
		.gpio      = 76,
		.settings = {
			[GPIOMUX_SUSPENDED] = &wchg_suspend_cfg
		},
	},
};
static struct msm_gpiomux_config irda_configs[] __initdata = {
	{
		.gpio      = 38,
		.settings = {
			[GPIOMUX_SUSPENDED] = &irda_pdn_cfg
		},
	},
};
/* <SlimAV1> ADD END */
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#if 0 /* IDPower GPIO ISV */
static struct msm_gpiomux_config msm8960_hdmi_configs[] __initdata = {
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#ifdef CONFIG_FB_MSM_HDMI_MHL_9244
		{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_4_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif
#ifdef CONFIG_FB_MSM_HDMI_MHL_8334
		{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_4_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif /* CONFIG_FB_MSM_HDMI_MHL */
};
#endif /* IDPower GPIO ISV */
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct gpiomux_setting sdcc2_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc2_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc2_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdcc2_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8960_sdcc2_configs[] __initdata = {
	{
		/* DATA_3 */
		.gpio      = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* DATA_2 */
		.gpio      = 91,
		.settings = {
		[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
		[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* DATA_1 */
		.gpio      = 90,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_data_1_suspend_cfg,
		},
	},
	{
		/* DATA_0 */
		.gpio      = 89,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* CMD */
		.gpio      = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* CLK */
		.gpio      = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
};
#endif

#ifdef   DEBUG
#ifdef   PT_DEBUG
#define GPIO_CONFIG(gpio)         (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)         (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))
static void debug_show_regsters(const int gpio_list[], size_t num)
{
	u32      val;
	int      gpio;
	size_t   i;

	for ( i = 0; i < num; i++ ) {
		gpio = gpio_list[i];
		val = __raw_readl(GPIO_CONFIG(gpio));
		pr_info("GPIO_CONFIG(%03d):0x%8p:%08x\n",
				gpio, GPIO_CONFIG(gpio), val);
	}
	for ( i = 0; i < num; i++ ) {
		gpio = gpio_list[i];
		val = __raw_readl(GPIO_IN_OUT(gpio));
		pr_info("GPIO_IN_OUT(%03d):0x%8p:%08x\n",
			gpio, GPIO_IN_OUT(gpio), val);
	}
}
#endif /*PT_DEBUG*/
#endif /*DEBUG*/
int __init msm8960_init_gpiomux(void)
{
#ifdef   PT_DEBUG
	static const int gpio_list[] = {
		2 , 3 , 14, 18, 19, 34, 35, 36, 37, 38,
		42, 51, 53, 54, 63, 64, 65, 76, 78, 79,
		81, 90,107,
	};
#endif /*PT_DEBUG*/
	int rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return rc;
	}

#ifdef   PT_DEBUG
	pr_debug("before msm_gpiomux_install\n");
	debug_show_regsters(gpio_list, ARRAY_SIZE(gpio_list));
#endif /*PT_DEBUG*/
/* <SlimAV1> Add Start */
#if 0
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(msm8960_ethernet_configs,
			ARRAY_SIZE(msm8960_ethernet_configs));
#endif
#else
	msm_gpiomux_install(msm8960_camera_mr_configs,
			ARRAY_SIZE(msm8960_camera_mr_configs));
#endif
/* <SlimAV1> Add End */

	msm_gpiomux_install(msm8960_gsbi_configs,
			ARRAY_SIZE(msm8960_gsbi_configs));

	msm_gpiomux_install(msm8960_synaptics_configs,
			ARRAY_SIZE(msm8960_synaptics_configs));

	msm_gpiomux_install(msm8960_slimbus_config,
			ARRAY_SIZE(msm8960_slimbus_config));

	msm_gpiomux_install(msm8960_audio_codec_configs,
			ARRAY_SIZE(msm8960_audio_codec_configs));

// <SlimAV1> del S
#if 0
	msm_gpiomux_install(msm8960_audio_auxpcm_configs,
			ARRAY_SIZE(msm8960_audio_auxpcm_configs));
#endif
// <SlimAV1> del E

	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	msm_gpiomux_install(msm8960_sdcc4_configs,
		ARRAY_SIZE(msm8960_sdcc4_configs));
#endif

	if (machine_is_msm8960_mtp() || machine_is_msm8960_fluid() ||
		machine_is_msm8960_liquid() || machine_is_msm8960_cdp())
		msm_gpiomux_install(hap_lvl_shft_config,
			ARRAY_SIZE(hap_lvl_shft_config));

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1) &&
		machine_is_msm8960_liquid())
		msm_gpiomux_install(msm8960_hsic_configs,
			ARRAY_SIZE(msm8960_hsic_configs));

	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1) &&
			machine_is_msm8960_liquid())
		msm_gpiomux_install(msm8960_hsic_hub_configs,
			ARRAY_SIZE(msm8960_hsic_hub_configs));
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#if 0  /* IDPower GPIO ISV */
	msm_gpiomux_install(msm8960_hdmi_configs,
			ARRAY_SIZE(msm8960_hdmi_configs));
#endif /* IDPower GPIO ISV */
#endif

/* <SlimAV1 Ver UP> Mod Start */
#if 0
	msm_gpiomux_install(msm8960_mdp_vsync_configs,
			ARRAY_SIZE(msm8960_mdp_vsync_configs));
#endif
/* <SlimAV1 Ver UP> Mod End */

/* <SlimAV1 Ver UP> Mod Start */
#if 0
	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE)
		msm_gpiomux_install(msm8960_gsbi8_uart_configs,
			ARRAY_SIZE(msm8960_gsbi8_uart_configs));
	else
		msm_gpiomux_install(msm8960_gsbi5_uart_configs,
			ARRAY_SIZE(msm8960_gsbi5_uart_configs));
#else
	msm_gpiomux_install(msm8960_gsbi8_uart_configs,
		ARRAY_SIZE(msm8960_gsbi8_uart_configs));
	msm_gpiomux_install(msm8960_gsbi5_uart_configs,
		ARRAY_SIZE(msm8960_gsbi5_uart_configs));
#endif
/* <SlimAV1 Ver UP> Add End */

/* <SlimAV1 Ver UP> Mod Start */
#if 0
	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE) {
		/* For 8960 Fusion 2.2 Primary IPC */
		msm_gpiomux_install(msm8960_fusion_gsbi_configs,
			ARRAY_SIZE(msm8960_fusion_gsbi_configs));
		/* For SGLTE 8960 Fusion External VFR */
		msm_gpiomux_install(msm8960_external_vfr_configs,
			ARRAY_SIZE(msm8960_external_vfr_configs));
	}
#else
	/* For 8960 Fusion 2.2 Primary IPC */
	msm_gpiomux_install(msm8960_fusion_gsbi_configs,
		ARRAY_SIZE(msm8960_fusion_gsbi_configs));
#endif
/* <SlimAV1 Ver UP> Add End */

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	msm_gpiomux_install(msm8960_sdcc2_configs,
		ARRAY_SIZE(msm8960_sdcc2_configs));
#endif

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE)
		msm_gpiomux_install(sglte_configs,
			ARRAY_SIZE(sglte_configs));

// <SlimAV1> add S
	msm_gpiomux_install(msm8960_flashled_configs,
			ARRAY_SIZE(msm8960_flashled_configs));
// <SlimAV1> add E
/* <SlimAV1> ADD START */
#ifdef   PT_DEBUG
		pr_debug("%s:%d size=%d, size=%d\n,", __func__, __LINE__,
				ARRAY_SIZE(wireless_charge_configs), ARRAY_SIZE(irda_configs));
#endif /*PT_DEBUG*/
		msm_gpiomux_install(wireless_charge_configs,
							ARRAY_SIZE(wireless_charge_configs));
		msm_gpiomux_install(irda_configs,
							ARRAY_SIZE(irda_configs));
/* <SlimAV1> ADD END */
#ifdef   PT_DEBUG
	pr_debug("after msm_gpiomux_install\n");
	debug_show_regsters(gpio_list, ARRAY_SIZE(gpio_list));
#endif /*PT_DEBUG*/
	return 0;
}
