/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_sharp.h"

static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 60 fps video mode */
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0xe7, 0x46, 0x3b, 0x00, 0xac, 0xa9, 0x3c, 0x48,
	0x41, 0x03, 0x04, 0x00},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x00, 0xbf, 0x31, 0xda, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};

static int __init mipi_video_sharp_hd_pt_init(void)
{
	int ret;

	if (msm_fb_detect_client("mipi_video_sharp_hd"))
		return 0;

	pinfo.xres = 1080;
	pinfo.yres = 1920;

	pinfo.mipi.xres_pad = 0;
	pinfo.mipi.yres_pad = 0;

	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 48;
	pinfo.lcdc.h_front_porch = 152;
	pinfo.lcdc.h_pulse_width = 12;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.lcdc.border_clr = 0;	        /* blk  */
	pinfo.lcdc.underflow_clr = 0x000000;	/* black */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = MIPI_SHARP_PWM_LEVEL;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 896000000;         /* MIPI CLK 448.0 MHZ*/

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
#ifdef MIPI_SHARP_FUNC_REFRESH_LP
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
#else
	pinfo.mipi.eof_bllp_power_stop = FALSE;
	pinfo.mipi.bllp_power_stop = FALSE;
#endif
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
	pinfo.mipi.t_clk_post = 0x19;
	pinfo.mipi.t_clk_pre = 0x37;
	pinfo.mipi.stream = 0;
	pinfo.mipi.mdp_trigger = 0;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.esc_byte_ratio = 2;

	ret = mipi_sharp_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_HD_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_sharp_hd_pt_init);
