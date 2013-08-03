/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h> /* kesasaki JB */
#include <mach/camera.h>
#include "msm_spi_thp7212.h"
#include "thp7212_imx119_fwtbl.c"
#include "thp7212_imx135_fwtbl.c"

#if 0
#undef CDBG
#define CDBG printk
#endif

/* SPI */
static struct spi_device *spidev;
static char *data;		/* npdc300089536 */

static int thp7212_spi_probe(struct spi_device *spi)
{
	if(spi == NULL){
		pr_err("failed... %s\n", __func__);
		return -EIO;
	}
	spi->bits_per_word = 32;	/* 32bit write */
	spi->max_speed_hz = 48000000;	/* 48Mbps */
	spi->mode = SPI_MODE_0;	/* mode0 */
	spi->chip_select = 0;
	spidev = spi;
	spi_setup(spi);
	return 0;
}

static int thp7212_spi_remove(struct spi_device *spi)
{
	if(spi == NULL){
		pr_err("failed... %s\n", __func__);
		return -EIO;
	}
	spidev = NULL;
	return 0;
}

static struct spi_driver thp7212_spi_driver = {
	.probe = thp7212_spi_probe,
	.remove = thp7212_spi_remove,
	.driver = {
		   .name = "thp7212_spi",
		   .owner = THIS_MODULE,
		   },
};

//#define EXISP_FW_FILE_LORD

int camera_fw_spi_write( int cam )
{
	int ret=0;
	int len;
#ifdef EXISP_FW_FILE_LORD
	struct file *fp;
	mm_segment_t seg;
#endif



	if (data == NULL) {
		pr_err("kmalloc() failed... %s\n", __func__);
		return -1;
	}

#ifdef EXISP_FW_FILE_LORD
	if(cam == THP7212_OUT){
	/* Out Camera */
		CDBG("######### imx135 camera_fw_spi_write \n");
		fp = filp_open("/data/imx135.bin", O_RDONLY | O_LARGEFILE, 0);	/* TODO */
	}else{
	/* In Camera */
		CDBG("######### imx119 camera_fw_spi_write \n");
		fp = filp_open("/data/imx119.bin", O_RDONLY | O_LARGEFILE, 0);	/* TODO */
	}

	if (IS_ERR(fp) == 1) {
		char *p;
		pr_info("fw read from tbl %s\n", __func__);
		if(cam == THP7212_OUT){
			p = (char*)&thp7212_imx135_fwtbl[0];
			len = sizeof(thp7212_imx135_fwtbl) /sizeof(thp7212_imx135_fwtbl[0]);
		}else{
			p = (char*)&thp7212_imx119_fwtbl[0];
			len = sizeof(thp7212_imx119_fwtbl) /sizeof(thp7212_imx119_fwtbl[0]);
		}

		memcpy(data, p, len);

		ret = spi_write((struct spi_device *)spidev, data, (size_t)len);
		CDBG("######### table spi_write ret = %d len = %d %s\n", ret, len, __func__);
	}else{
		pr_info("filp_open() success... %s\n", __func__);

		seg = get_fs();
		set_fs(get_ds());
		len = fp->f_op->read(fp, data, 200000, &fp->f_pos);	/* TODO 200K */
		set_fs(seg);

		ret = spi_write((struct spi_device *)spidev, data, (size_t)len);
		CDBG("######### file spi_write ret = %d len = %d %s\n", ret, len, __func__);
	
		filp_close(fp, 0);
	}
#else
	{
		char *p;
		pr_info("fw read from tbl %s\n", __func__);
		if(cam == THP7212_OUT){
			p = (char*)&thp7212_imx135_fwtbl[0];
			len = sizeof(thp7212_imx135_fwtbl) /sizeof(thp7212_imx135_fwtbl[0]);
		}else{
			p = (char*)&thp7212_imx119_fwtbl[0];
			len = sizeof(thp7212_imx119_fwtbl) /sizeof(thp7212_imx119_fwtbl[0]);
		}

		memcpy(data, p, len);

		ret = spi_write((struct spi_device *)spidev, data, (size_t)len);
		CDBG("######### table spi_write ret = %d len = %d %s\n", ret, len, __func__);
	}
#endif

	return ret;
}



static int __init thp7212_spi_init(void)
{
	int ret = 0;
	CDBG("---------------- thp7212_spi_init --------------\n");
#if 1			/* npdc300089536 */
	if(data == NULL){
		data = kmalloc(128*1024, GFP_KERNEL);	/* Camera FW size 128K */
		if (data == NULL) {
			pr_err("kmalloc() failed... %s\n", __func__);
		}
	}
#endif

	ret = spi_register_driver(&thp7212_spi_driver);
	if (ret)
		pr_err("%s: spi register failed: rc=%d\n", __func__, ret);

	return ret;
}

static void __exit thp7212_spi_exit(void)
{
	spi_unregister_driver(&thp7212_spi_driver);
}

module_init(thp7212_spi_init);
module_exit(thp7212_spi_exit);
MODULE_DESCRIPTION("Camera SPI driver");
MODULE_LICENSE("GPL v2");
