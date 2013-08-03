/*
 *  linux/drivers/mmc/core/sd.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  SD support Copyright (C) 2004 Ian Molton, All Rights Reserved.
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/stat.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <linux/rtc.h>
#include <linux/cfgdrv.h>

#include "core.h"
#include "bus.h"
#include "mmc_ops.h"
#include "sd.h"
#include "sd_ops.h"

static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

/* For log  START */
static struct mmc_cid log_cid;
/* For log  END */

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
void mmc_decode_cid(struct mmc_card *card)
{
	u32 *resp = card->raw_cid;

	memset(&card->cid, 0, sizeof(struct mmc_cid));

	/*
	 * SD doesn't currently have a version field so we will
	 * have to assume we can parse this.
	 */
	card->cid.manfid		= UNSTUFF_BITS(resp, 120, 8);
	card->cid.oemid			= UNSTUFF_BITS(resp, 104, 16);
	card->cid.prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
	card->cid.prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
	card->cid.prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
	card->cid.prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
	card->cid.prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
	card->cid.hwrev			= UNSTUFF_BITS(resp, 60, 4);
	card->cid.fwrev			= UNSTUFF_BITS(resp, 56, 4);
	card->cid.serial		= UNSTUFF_BITS(resp, 24, 32);
	card->cid.year			= UNSTUFF_BITS(resp, 12, 8);
	card->cid.month			= UNSTUFF_BITS(resp, 8, 4);

	card->cid.year += 2000; /* SD cards year offset */
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static int mmc_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, csd_struct;
	u32 *resp = card->raw_csd;

	csd_struct = UNSTUFF_BITS(resp, 126, 2);

	switch (csd_struct) {
	case 0:
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);

		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
		csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
		csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
		csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
		csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
		csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
		csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

		if (UNSTUFF_BITS(resp, 46, 1)) {
			csd->erase_size = 1;
		} else if (csd->write_blkbits >= 9) {
			csd->erase_size = UNSTUFF_BITS(resp, 39, 7) + 1;
			csd->erase_size <<= csd->write_blkbits - 9;
		}
		break;
	case 1:
		/*
		 * This is a block-addressed SDHC or SDXC card. Most
		 * interesting fields are unused and have fixed
		 * values. To avoid getting tripped by buggy cards,
		 * we assume those fixed values ourselves.
		 */
		mmc_card_set_blockaddr(card);

		csd->tacc_ns	 = 0; /* Unused */
		csd->tacc_clks	 = 0; /* Unused */

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);
		csd->c_size	  = UNSTUFF_BITS(resp, 48, 22);

		/* SDXC cards have a minimum C_SIZE of 0x00FFFF */
		if (csd->c_size >= 0xFFFF)
			mmc_card_set_ext_capacity(card);

		m = UNSTUFF_BITS(resp, 48, 22);
		csd->capacity     = (1 + m) << 10;

		csd->read_blkbits = 9;
		csd->read_partial = 0;
		csd->write_misalign = 0;
		csd->read_misalign = 0;
		csd->r2w_factor = 4; /* Unused */
		csd->write_blkbits = 9;
		csd->write_partial = 0;
		csd->erase_size = 1;
		break;
	default:
		pr_err("%s: unrecognised CSD structure version %d\n",
			mmc_hostname(card->host), csd_struct);
		return -EINVAL;
	}

	card->erase_size = csd->erase_size;

	return 0;
}

/*
 * Given a 64-bit response, decode to our card SCR structure.
 */
static int mmc_decode_scr(struct mmc_card *card)
{
	struct sd_scr *scr = &card->scr;
	unsigned int scr_struct;
	u32 resp[4];

	resp[3] = card->raw_scr[1];
	resp[2] = card->raw_scr[0];

	scr_struct = UNSTUFF_BITS(resp, 60, 4);
	if (scr_struct != 0) {
		pr_err("%s: unrecognised SCR structure version %d\n",
			mmc_hostname(card->host), scr_struct);
		return -EINVAL;
	}

	scr->sda_vsn = UNSTUFF_BITS(resp, 56, 4);
	scr->bus_widths = UNSTUFF_BITS(resp, 48, 4);
	if (scr->sda_vsn == SCR_SPEC_VER_2)
		/* Check if Physical Layer Spec v3.0 is supported */
		scr->sda_spec3 = UNSTUFF_BITS(resp, 47, 1);

	if (UNSTUFF_BITS(resp, 55, 1))
		card->erased_byte = 0xFF;
	else
		card->erased_byte = 0x0;

	if (scr->sda_spec3)
		scr->cmds = UNSTUFF_BITS(resp, 32, 2);
	return 0;
}

/*
 * Fetch and process SD Status register.
 */
static int mmc_read_ssr(struct mmc_card *card)
{
	unsigned int au, es, et, eo;
	int err, i;
	u32 *ssr;

	if (!(card->csd.cmdclass & CCC_APP_SPEC)) {
		pr_warning("%s: card lacks mandatory SD Status "
			"function.\n", mmc_hostname(card->host));
		return 0;
	}

	ssr = kmalloc(64, GFP_KERNEL);
	if (!ssr)
		return -ENOMEM;

	err = mmc_app_sd_status(card, ssr);
	if (err) {
		pr_warning("%s: problem reading SD Status "
			"register.\n", mmc_hostname(card->host));
		err = 0;
		goto out;
	}

	for (i = 0; i < 16; i++)
		ssr[i] = be32_to_cpu(ssr[i]);

	/*
	 * UNSTUFF_BITS only works with four u32s so we have to offset the
	 * bitfield positions accordingly.
	 */
	au = UNSTUFF_BITS(ssr, 428 - 384, 4);
	if (au > 0 || au <= 9) {
		card->ssr.au = 1 << (au + 4);
		es = UNSTUFF_BITS(ssr, 408 - 384, 16);
		et = UNSTUFF_BITS(ssr, 402 - 384, 6);
		eo = UNSTUFF_BITS(ssr, 400 - 384, 2);
		if (es && et) {
			card->ssr.erase_timeout = (et * 1000) / es;
			card->ssr.erase_offset = eo * 1000;
		}
	} else {
		pr_warning("%s: SD Status: Invalid Allocation Unit "
			"size.\n", mmc_hostname(card->host));
	}
out:
	kfree(ssr);
	return err;
}

/*
 * Fetches and decodes switch information
 */
static int mmc_read_switch(struct mmc_card *card)
{
	int err;
	u8 *status;
/* ADD-S [SDDriver_010] */
	int retry_chk = 0;
	unsigned int card_status = 0;
/* ADD-S [SDDriver_010] */

	if (card->scr.sda_vsn < SCR_SPEC_VER_1)
		return 0;

	if (!(card->csd.cmdclass & CCC_SWITCH)) {
		pr_warning("%s: card lacks mandatory switch "
			"function, performance might suffer.\n",
			mmc_hostname(card->host));
		return 0;
	}

	err = -EIO;

	status = kmalloc(64, GFP_KERNEL);
	if (!status) {
		pr_err("%s: could not allocate a buffer for "
			"switch capabilities.\n",
			mmc_hostname(card->host));
		return -ENOMEM;
	}

/* ADD-S [SDDriver_010] */
	for ( retry_chk = 0; retry_chk < 10 ; retry_chk++ )
	{
/* ADD-E [SDDriver_010] */
	/* Find out the supported Bus Speed Modes. */
	err = mmc_sd_switch(card, 0, 0, 1, status);
/* ADD-S [SDDriver_010] */
		if( err != 0 )
		{
			card_status = mmc_get_send_status( card );
			printk("[MMC]%s(%d):CURRENT_STATE[%d] retry_chk[%d]\n",__FUNCTION__,__LINE__,R1_CURRENT_STATE(card_status),retry_chk );
			
			/* CURRENT_STATE = data(5)/rcv(6) */
			if ( (R1_CURRENT_STATE(card_status) == 5) ||
				 (R1_CURRENT_STATE(card_status) == 6) )
			{
				mmc_send_stop_transmission( card );
			}
		}
		else
		{
			break;
		}
	}
/* END-S [SDDriver_010] */

	if (err) {
/* ADD-S [SDDriver_008] */
        printk("[MMC]%s(%d):CMD6 mode0 err[%d]\n",__FUNCTION__,__LINE__,err );
/* ADD-E [SDDriver_008] */

		/*
		 * If the host or the card can't do the switch,
		 * fail more gracefully.
		 */
		if (err != -EINVAL && err != -ENOSYS && err != -EFAULT)
			goto out;

		pr_warning("%s: problem reading Bus Speed modes.\n",
			mmc_hostname(card->host));
		err = 0;

		goto out;
	}

	if (status[13] & SD_MODE_HIGH_SPEED)
		card->sw_caps.hs_max_dtr = HIGH_SPEED_MAX_DTR;

	if (card->scr.sda_spec3) {
		card->sw_caps.sd3_bus_mode = status[13];

		/* Find out Driver Strengths supported by the card */
		err = mmc_sd_switch(card, 0, 2, 1, status);
		if (err) {
			/*
			 * If the host or the card can't do the switch,
			 * fail more gracefully.
			 */
			if (err != -EINVAL && err != -ENOSYS && err != -EFAULT)
				goto out;

			pr_warning("%s: problem reading "
				"Driver Strength.\n",
				mmc_hostname(card->host));
			err = 0;

			goto out;
		}

		card->sw_caps.sd3_drv_type = status[9];

		/* Find out Current Limits supported by the card */
		err = mmc_sd_switch(card, 0, 3, 1, status);
		if (err) {
			/*
			 * If the host or the card can't do the switch,
			 * fail more gracefully.
			 */
			if (err != -EINVAL && err != -ENOSYS && err != -EFAULT)
				goto out;

			pr_warning("%s: problem reading "
				"Current Limit.\n",
				mmc_hostname(card->host));
			err = 0;

			goto out;
		}

		card->sw_caps.sd3_curr_limit = status[7];
	}

out:
	kfree(status);

	return err;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
int mmc_sd_switch_hs(struct mmc_card *card)
{
	int err;
	u8 *status;
/* ADD-S [SDDriver_010] */
	int retry_chk = 0;
	unsigned int card_status = 0;
/* ADD-S [SDDriver_010] */

	if (card->scr.sda_vsn < SCR_SPEC_VER_1)
		return 0;

	if (!(card->csd.cmdclass & CCC_SWITCH))
		return 0;

	if (!(card->host->caps & MMC_CAP_SD_HIGHSPEED))
		return 0;

	if (card->sw_caps.hs_max_dtr == 0)
		return 0;

	err = -EIO;

	status = kmalloc(64, GFP_KERNEL);
	if (!status) {
		pr_err("%s: could not allocate a buffer for "
			"switch capabilities.\n", mmc_hostname(card->host));
		return -ENOMEM;
	}

/* ADD-S [SDDriver_010] */
	for ( retry_chk = 0; retry_chk < 10 ; retry_chk++ )
	{
/* ADD-E [SDDriver_010] */
	err = mmc_sd_switch(card, 1, 0, 1, status);
/* ADD-S [SDDriver_010] */
		if( err != 0 )
		{
			card_status = mmc_get_send_status( card );
			printk("[MMC]%s(%d):CURRENT_STATE[%d] retry_chk[%d]\n",__FUNCTION__,__LINE__,R1_CURRENT_STATE(card_status),retry_chk );
			
			/* CURRENT_STATE = data(5)/rcv(6) */
			if ( (R1_CURRENT_STATE(card_status) == 5) ||
				 (R1_CURRENT_STATE(card_status) == 6) )
			{
				mmc_send_stop_transmission( card );
			}
		}
		else
		{
			break;
		}
	}
/* ADD-E [SDDriver_010] */

	if (err)
/* ADD-S [SDDriver_008] */
	{
        printk("[MMC]%s(%d):CMD6 mode1 err[%d]\n",__FUNCTION__,__LINE__,err );
/* ADD-E [SDDriver_008] */
		goto out;
/* ADD-S [SDDriver_008] */
	}
/* ADD-E [SDDriver_008] */

	if ((status[16] & 0xF) != 1) {
		pr_warning("%s: Problem switching card "
			"into high-speed mode!\n",
			mmc_hostname(card->host));
		err = 0;
	} else {
		err = 1;
	}

out:
	kfree(status);

	return err;
}

static int sd_select_driver_type(struct mmc_card *card, u8 *status)
{
	int host_drv_type = SD_DRIVER_TYPE_B;
	int card_drv_type = SD_DRIVER_TYPE_B;
	int drive_strength;
	int err;

	/*
	 * If the host doesn't support any of the Driver Types A,C or D,
	 * or there is no board specific handler then default Driver
	 * Type B is used.
	 */
	if (!(card->host->caps & (MMC_CAP_DRIVER_TYPE_A | MMC_CAP_DRIVER_TYPE_C
	    | MMC_CAP_DRIVER_TYPE_D)))
		return 0;

	if (!card->host->ops->select_drive_strength)
		return 0;

	if (card->host->caps & MMC_CAP_DRIVER_TYPE_A)
		host_drv_type |= SD_DRIVER_TYPE_A;

	if (card->host->caps & MMC_CAP_DRIVER_TYPE_C)
		host_drv_type |= SD_DRIVER_TYPE_C;

	if (card->host->caps & MMC_CAP_DRIVER_TYPE_D)
		host_drv_type |= SD_DRIVER_TYPE_D;

	if (card->sw_caps.sd3_drv_type & SD_DRIVER_TYPE_A)
		card_drv_type |= SD_DRIVER_TYPE_A;

	if (card->sw_caps.sd3_drv_type & SD_DRIVER_TYPE_C)
		card_drv_type |= SD_DRIVER_TYPE_C;

	if (card->sw_caps.sd3_drv_type & SD_DRIVER_TYPE_D)
		card_drv_type |= SD_DRIVER_TYPE_D;

	/*
	 * The drive strength that the hardware can support
	 * depends on the board design.  Pass the appropriate
	 * information and let the hardware specific code
	 * return what is possible given the options
	 */
	mmc_host_clk_hold(card->host);
	drive_strength = card->host->ops->select_drive_strength(
		card->sw_caps.uhs_max_dtr,
		host_drv_type, card_drv_type);
	mmc_host_clk_release(card->host);

	err = mmc_sd_switch(card, 1, 2, drive_strength, status);
	if (err)
		return err;

	if ((status[15] & 0xF) != drive_strength) {
		pr_warning("%s: Problem setting drive strength!\n",
			mmc_hostname(card->host));
		return 0;
	}

	mmc_set_driver_type(card->host, drive_strength);

	return 0;
}

static void sd_update_bus_speed_mode(struct mmc_card *card)
{
	/*
	 * If the host doesn't support any of the UHS-I modes, fallback on
	 * default speed.
	 */
	if (!(card->host->caps & (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
	    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50))) {
		card->sd_bus_speed = 0;
		return;
	}

	if ((card->host->caps & MMC_CAP_UHS_SDR104) &&
	    (card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR104)) {
			card->sd_bus_speed = UHS_SDR104_BUS_SPEED;
	} else if ((card->host->caps & MMC_CAP_UHS_DDR50) &&
		   (card->sw_caps.sd3_bus_mode & SD_MODE_UHS_DDR50)) {
			card->sd_bus_speed = UHS_DDR50_BUS_SPEED;
	} else if ((card->host->caps & (MMC_CAP_UHS_SDR104 |
		    MMC_CAP_UHS_SDR50)) && (card->sw_caps.sd3_bus_mode &
		    SD_MODE_UHS_SDR50)) {
			card->sd_bus_speed = UHS_SDR50_BUS_SPEED;
	} else if ((card->host->caps & (MMC_CAP_UHS_SDR104 |
		    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR25)) &&
		   (card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR25)) {
			card->sd_bus_speed = UHS_SDR25_BUS_SPEED;
	} else if ((card->host->caps & (MMC_CAP_UHS_SDR104 |
		    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR25 |
		    MMC_CAP_UHS_SDR12)) && (card->sw_caps.sd3_bus_mode &
		    SD_MODE_UHS_SDR12)) {
			card->sd_bus_speed = UHS_SDR12_BUS_SPEED;
	}
}

static int sd_set_bus_speed_mode(struct mmc_card *card, u8 *status)
{
	int err;
	unsigned int timing = 0;

	switch (card->sd_bus_speed) {
	case UHS_SDR104_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR104;
		card->sw_caps.uhs_max_dtr = UHS_SDR104_MAX_DTR;
		break;
	case UHS_DDR50_BUS_SPEED:
		timing = MMC_TIMING_UHS_DDR50;
		card->sw_caps.uhs_max_dtr = UHS_DDR50_MAX_DTR;
		break;
	case UHS_SDR50_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR50;
		card->sw_caps.uhs_max_dtr = UHS_SDR50_MAX_DTR;
		break;
	case UHS_SDR25_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR25;
		card->sw_caps.uhs_max_dtr = UHS_SDR25_MAX_DTR;
		break;
	case UHS_SDR12_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR12;
		card->sw_caps.uhs_max_dtr = UHS_SDR12_MAX_DTR;
		break;
	default:
		return 0;
	}

	err = mmc_sd_switch(card, 1, 0, card->sd_bus_speed, status);
	if (err)
		return err;

	if ((status[16] & 0xF) != card->sd_bus_speed)
		pr_warning("%s: Problem setting bus speed mode!\n",
			mmc_hostname(card->host));
	else {
		mmc_set_timing(card->host, timing);
		mmc_set_clock(card->host, card->sw_caps.uhs_max_dtr);
	}

	return 0;
}

static int sd_set_current_limit(struct mmc_card *card, u8 *status)
{
	int current_limit = 0;
	int err;

	/*
	 * Current limit switch is only defined for SDR50, SDR104, and DDR50
	 * bus speed modes. For other bus speed modes, we set the default
	 * current limit of 200mA.
	 */
	if ((card->sd_bus_speed == UHS_SDR50_BUS_SPEED) ||
	    (card->sd_bus_speed == UHS_SDR104_BUS_SPEED) ||
	    (card->sd_bus_speed == UHS_DDR50_BUS_SPEED)) {
		if (card->host->caps & MMC_CAP_MAX_CURRENT_800) {
			if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_800)
				current_limit = SD_SET_CURRENT_LIMIT_800;
			else if (card->sw_caps.sd3_curr_limit &
					SD_MAX_CURRENT_600)
				current_limit = SD_SET_CURRENT_LIMIT_600;
			else if (card->sw_caps.sd3_curr_limit &
					SD_MAX_CURRENT_400)
				current_limit = SD_SET_CURRENT_LIMIT_400;
			else if (card->sw_caps.sd3_curr_limit &
					SD_MAX_CURRENT_200)
				current_limit = SD_SET_CURRENT_LIMIT_200;
		} else if (card->host->caps & MMC_CAP_MAX_CURRENT_600) {
			if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_600)
				current_limit = SD_SET_CURRENT_LIMIT_600;
			else if (card->sw_caps.sd3_curr_limit &
					SD_MAX_CURRENT_400)
				current_limit = SD_SET_CURRENT_LIMIT_400;
			else if (card->sw_caps.sd3_curr_limit &
					SD_MAX_CURRENT_200)
				current_limit = SD_SET_CURRENT_LIMIT_200;
		} else if (card->host->caps & MMC_CAP_MAX_CURRENT_400) {
			if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_400)
				current_limit = SD_SET_CURRENT_LIMIT_400;
			else if (card->sw_caps.sd3_curr_limit &
					SD_MAX_CURRENT_200)
				current_limit = SD_SET_CURRENT_LIMIT_200;
		} else if (card->host->caps & MMC_CAP_MAX_CURRENT_200) {
			if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_200)
				current_limit = SD_SET_CURRENT_LIMIT_200;
		}
	} else
		current_limit = SD_SET_CURRENT_LIMIT_200;

	err = mmc_sd_switch(card, 1, 3, current_limit, status);
	if (err)
		return err;

	if (((status[15] >> 4) & 0x0F) != current_limit)
		pr_warning("%s: Problem setting current limit!\n",
			mmc_hostname(card->host));

	return 0;
}

/*
 * UHS-I specific initialization procedure
 */
static int mmc_sd_init_uhs_card(struct mmc_card *card)
{
	int err;
	u8 *status;

	if (!card->scr.sda_spec3)
		return 0;

	if (!(card->csd.cmdclass & CCC_SWITCH))
		return 0;

	status = kmalloc(64, GFP_KERNEL);
	if (!status) {
		pr_err("%s: could not allocate a buffer for "
			"switch capabilities.\n", mmc_hostname(card->host));
		return -ENOMEM;
	}

	/* Set 4-bit bus width */
	if ((card->host->caps & MMC_CAP_4_BIT_DATA) &&
	    (card->scr.bus_widths & SD_SCR_BUS_WIDTH_4)) {
		err = mmc_app_set_bus_width(card, MMC_BUS_WIDTH_4);
		if (err)
			goto out;

		mmc_set_bus_width(card->host, MMC_BUS_WIDTH_4);
	}

	/*
	 * Select the bus speed mode depending on host
	 * and card capability.
	 */
	sd_update_bus_speed_mode(card);

	/* Set the driver strength for the card */
	err = sd_select_driver_type(card, status);
	if (err)
		goto out;

	/* Set current limit for the card */
	err = sd_set_current_limit(card, status);
	if (err)
		goto out;

	/* Set bus speed mode of the card */
	err = sd_set_bus_speed_mode(card, status);
	if (err)
		goto out;

	/* SPI mode doesn't define CMD19 */
	if (!mmc_host_is_spi(card->host) && card->host->ops->execute_tuning) {
		mmc_host_clk_hold(card->host);
		err = card->host->ops->execute_tuning(card->host,
						      MMC_SEND_TUNING_BLOCK);
		mmc_host_clk_release(card->host);
	}

out:
	kfree(status);

	return err;
}

MMC_DEV_ATTR(cid, "%08x%08x%08x%08x\n", card->raw_cid[0], card->raw_cid[1],
	card->raw_cid[2], card->raw_cid[3]);
MMC_DEV_ATTR(csd, "%08x%08x%08x%08x\n", card->raw_csd[0], card->raw_csd[1],
	card->raw_csd[2], card->raw_csd[3]);
MMC_DEV_ATTR(scr, "%08x%08x\n", card->raw_scr[0], card->raw_scr[1]);
MMC_DEV_ATTR(date, "%02d/%04d\n", card->cid.month, card->cid.year);
MMC_DEV_ATTR(erase_size, "%u\n", card->erase_size << 9);
MMC_DEV_ATTR(preferred_erase_size, "%u\n", card->pref_erase << 9);
MMC_DEV_ATTR(fwrev, "0x%x\n", card->cid.fwrev);
MMC_DEV_ATTR(hwrev, "0x%x\n", card->cid.hwrev);
MMC_DEV_ATTR(manfid, "0x%06x\n", card->cid.manfid);
MMC_DEV_ATTR(name, "%s\n", card->cid.prod_name);
MMC_DEV_ATTR(oemid, "0x%04x\n", card->cid.oemid);
MMC_DEV_ATTR(serial, "0x%08x\n", card->cid.serial);


static struct attribute *sd_std_attrs[] = {
	&dev_attr_cid.attr,
	&dev_attr_csd.attr,
	&dev_attr_scr.attr,
	&dev_attr_date.attr,
	&dev_attr_erase_size.attr,
	&dev_attr_preferred_erase_size.attr,
	&dev_attr_fwrev.attr,
	&dev_attr_hwrev.attr,
	&dev_attr_manfid.attr,
	&dev_attr_name.attr,
	&dev_attr_oemid.attr,
	&dev_attr_serial.attr,
	NULL,
};

static struct attribute_group sd_std_attr_group = {
	.attrs = sd_std_attrs,
};

static const struct attribute_group *sd_attr_groups[] = {
	&sd_std_attr_group,
	NULL,
};

struct device_type sd_type = {
	.groups = sd_attr_groups,
};

/*
 * Fetch CID from card.
 */
int mmc_sd_get_cid(struct mmc_host *host, u32 ocr, u32 *cid, u32 *rocr)
{
	int err;

	/*
	 * Since we're changing the OCR value, we seem to
	 * need to tell some cards to go back to the idle
	 * state.  We wait 1ms to give cards time to
	 * respond.
	 */
	mmc_go_idle(host);

	/*
	 * If SD_SEND_IF_COND indicates an SD 2.0
	 * compliant card and we should set bit 30
	 * of the ocr to indicate that we can handle
	 * block-addressed SDHC cards.
	 */
	err = mmc_send_if_cond(host, ocr);
	if (!err)
		ocr |= SD_OCR_CCS;

	/*
	 * If the host supports one of UHS-I modes, request the card
	 * to switch to 1.8V signaling level.
	 */
	if (host->caps & (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
	    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50))
		ocr |= SD_OCR_S18R;

	/* If the host can supply more than 150mA, XPC should be set to 1. */
	if (host->caps & (MMC_CAP_SET_XPC_330 | MMC_CAP_SET_XPC_300 |
	    MMC_CAP_SET_XPC_180))
		ocr |= SD_OCR_XPC;

try_again:
	err = mmc_send_app_op_cond(host, ocr, rocr);
	if (err)
		return err;

	/*
	 * In case CCS and S18A in the response is set, start Signal Voltage
	 * Switch procedure. SPI mode doesn't support CMD11.
	 */
	if (!mmc_host_is_spi(host) && rocr &&
	   ((*rocr & 0x41000000) == 0x41000000)) {
		err = mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180, true);
		if (err) {
			ocr &= ~SD_OCR_S18R;
			goto try_again;
		}
	}

	if (mmc_host_is_spi(host))
		err = mmc_send_cid(host, cid);
	else
		err = mmc_all_send_cid(host, cid);

	return err;
}

int mmc_sd_get_csd(struct mmc_host *host, struct mmc_card *card)
{
	int err;

	/*
	 * Fetch CSD from card.
	 */
	err = mmc_send_csd(card, card->raw_csd);
	if (err)
		return err;

	err = mmc_decode_csd(card);
	if (err)
		return err;

	return 0;
}

int mmc_sd_setup_card(struct mmc_host *host, struct mmc_card *card,
	bool reinit)
{
	int err;
#ifdef CONFIG_MMC_PARANOID_SD_INIT
	int retries;
#endif

	if (!reinit) {
		/*
		 * Fetch SCR from card.
		 */
		err = mmc_app_send_scr(card, card->raw_scr);
		if (err)
			return err;

		err = mmc_decode_scr(card);
		if (err)
			return err;

		/*
		 * Fetch and process SD Status register.
		 */
		err = mmc_read_ssr(card);
		if (err)
			return err;

		/* Erase init depends on CSD and SSR */
		mmc_init_erase(card);

		/*
		 * Fetch switch information from card.
		 */
#ifdef CONFIG_MMC_PARANOID_SD_INIT
		for (retries = 1; retries <= 3; retries++) {
			err = mmc_read_switch(card);
			if (!err) {
				if (retries > 1) {
					printk(KERN_WARNING
					       "%s: recovered\n", 
					       mmc_hostname(host));
				}
				break;
			} else {
				printk(KERN_WARNING
				       "%s: read switch failed (attempt %d)\n",
				       mmc_hostname(host), retries);
			}
		}
#else
		err = mmc_read_switch(card);
#endif

		if (err)
			return err;
	}

	/*
	 * For SPI, enable CRC as appropriate.
	 * This CRC enable is located AFTER the reading of the
	 * card registers because some SDHC cards are not able
	 * to provide valid CRCs for non-512-byte blocks.
	 */
	if (mmc_host_is_spi(host)) {
		err = mmc_spi_set_crc(host, use_spi_crc);
		if (err)
			return err;
	}

	/*
	 * Check if read-only switch is active.
	 */
	if (!reinit) {
		int ro = -1;

		if (host->ops->get_ro) {
			mmc_host_clk_hold(host);
			ro = host->ops->get_ro(host);
			mmc_host_clk_release(host);
		}

		if (ro < 0) {
			pr_warning("%s: host does not "
				"support reading read-only "
				"switch. assuming write-enable.\n",
				mmc_hostname(host));
		} else if (ro > 0) {
			mmc_card_set_readonly(card);
		}
	}

	return 0;
}

unsigned mmc_sd_get_max_clock(struct mmc_card *card)
{
	unsigned max_dtr = (unsigned int)-1;

	if (mmc_card_highspeed(card)) {
		if (max_dtr > card->sw_caps.hs_max_dtr)
			max_dtr = card->sw_caps.hs_max_dtr;
	} else if (max_dtr > card->csd.max_dtr) {
		max_dtr = card->csd.max_dtr;
	}

	return max_dtr;
}

void mmc_sd_go_highspeed(struct mmc_card *card)
{
	mmc_card_set_highspeed(card);
	mmc_set_timing(card->host, MMC_TIMING_SD_HS);
}

/*
 * Handle the detection and initialisation of a card.
 *
 * In the case of a resume, "oldcard" will contain the card
 * we're trying to reinitialise.
 */
int mmc_sd_init_card(struct mmc_host *host, u32 ocr,
	struct mmc_card *oldcard)
{
	struct mmc_card *card;
	int err;
	u32 cid[4];
	u32 rocr = 0;
/* ADD-S [SDDriver_001] */
    u32 cnt_chk;
/* ADD-E [SDDriver_001] */

	BUG_ON(!host);
	WARN_ON(!host->claimed);

	/* The initialization should be done at 3.3 V I/O voltage. */
	mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330, 0);

	err = mmc_sd_get_cid(host, ocr, cid, &rocr);
	if (err)
		return err;

	if (oldcard) {
		if (memcmp(cid, oldcard->raw_cid, sizeof(cid)) != 0)
			return -ENOENT;

		card = oldcard;
	} else {
		/*
		 * Allocate card structure.
		 */
		card = mmc_alloc_card(host, &sd_type);
		if (IS_ERR(card))
			return PTR_ERR(card);

		card->type = MMC_TYPE_SD;
		memcpy(card->raw_cid, cid, sizeof(card->raw_cid));
	}

	/*
	 * For native busses:  get card RCA and quit open drain mode.
	 */
	if (!mmc_host_is_spi(host)) {
/* ADD-S [SDDriver_001] */
		for ( cnt_chk = 0 ; cnt_chk < 3 ; cnt_chk++ )
		{
/* ADD-E [SDDriver_001] */
		err = mmc_send_relative_addr(host, &card->rca);
		if (err)
			return err;
/* ADD-S [SDDriver_001] */
			if ( card->rca != 0 )
			{
				break;
			}
		}

		if ( card->rca == 0 )
		{
			return err;
		}
/* ADD-E [SDDriver_001] */

	}

	if (!oldcard) {
		err = mmc_sd_get_csd(host, card);
		if (err)
			return err;

		mmc_decode_cid(card);
/* For log START */
		if(&(card->cid))
			memcpy(&log_cid, &(card->cid), sizeof(struct mmc_cid));
/* For log END */
	}

	/*
	 * Select card, as all following commands rely on that.
	 */
	if (!mmc_host_is_spi(host)) {
		err = mmc_select_card(card);
		if (err)
			return err;
	}

	err = mmc_sd_setup_card(host, card, oldcard != NULL);
	if (err)
		goto free_card;

	/* Initialization sequence for UHS-I cards */
	if (rocr & SD_ROCR_S18A) {
		err = mmc_sd_init_uhs_card(card);
		if (err)
			goto free_card;

		/* Card is an ultra-high-speed card */
		mmc_card_set_uhs(card);

		/*
		 * Since initialization is now complete, enable preset
		 * value registers for UHS-I cards.
		 */
		if (host->ops->enable_preset_value) {
			mmc_host_clk_hold(host);
			host->ops->enable_preset_value(host, true);
			mmc_host_clk_release(host);
		}
	} else {
		/*
		 * Attempt to change to high-speed (if supported)
		 */
		err = mmc_sd_switch_hs(card);
		if (err > 0)
			mmc_sd_go_highspeed(card);
		else if (err)
			goto free_card;

		/*
		 * Set bus speed.
		 */
		mmc_set_clock(host, mmc_sd_get_max_clock(card));

		/*
		 * Switch to wider bus (if supported).
		 */
		if ((host->caps & MMC_CAP_4_BIT_DATA) &&
			(card->scr.bus_widths & SD_SCR_BUS_WIDTH_4)) {
			err = mmc_app_set_bus_width(card, MMC_BUS_WIDTH_4);
			if (err)
				goto free_card;

			mmc_set_bus_width(host, MMC_BUS_WIDTH_4);
		}
	}

/* ADD-S [SDDriver_002] */
	err = mmc_app_set_clr_card_detect( card, SD_PULLUP_DISCONNECT );
	if ( err != 0 )
	{
		goto free_card;
	}
/* ADD-E [SDDriver_002] */

	host->card = card;
	return 0;

free_card:
	if (!oldcard)
		mmc_remove_card(card);

	return err;
}

/*
 * Host is being removed. Free up the current card.
 */
static void mmc_sd_remove(struct mmc_host *host)
{
	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_remove_card(host->card);

	mmc_claim_host(host);
	host->card = NULL;
	mmc_release_host(host);
}

/*
 * Card detection - card is alive.
 */
static int mmc_sd_alive(struct mmc_host *host)
{
	return mmc_send_status(host->card, NULL);
}

/*
 * Card detection callback from host.
 */
static void mmc_sd_detect(struct mmc_host *host)
{
	int err = 0;
#ifdef CONFIG_MMC_PARANOID_SD_INIT
        int retries = 5;
#endif

	BUG_ON(!host);
	BUG_ON(!host->card);
       
	mmc_claim_host(host);

	/*
	 * Just check if our card has been removed.
	 */
#ifdef CONFIG_MMC_PARANOID_SD_INIT
	while(retries) {
		err = mmc_send_status(host->card, NULL);
		if (err) {
			retries--;
			udelay(5);
			continue;
		}
		break;
	}
	if (!retries) {
		printk(KERN_ERR "%s(%s): Unable to re-detect card (%d)\n",
		       __func__, mmc_hostname(host), err);
	}
#else
	err = _mmc_detect_card_removed(host);
#endif
	mmc_release_host(host);

	if (err) {
		mmc_sd_remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
	}
}

/*
 * Suspend callback from host.
 */
static int mmc_sd_suspend(struct mmc_host *host)
{
	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_claim_host(host);
	if (!mmc_host_is_spi(host))
		mmc_deselect_cards(host);
	host->card->state &= ~MMC_STATE_HIGHSPEED;
	mmc_release_host(host);

	return 0;
}

/*
 * Resume callback from host.
 *
 * This function tries to determine if the same card is still present
 * and, if so, restore all state to it.
 */
static int mmc_sd_resume(struct mmc_host *host)
{
	int err;
#ifdef CONFIG_MMC_PARANOID_SD_INIT
	int retries;
#endif
/* For log START */
	int offset1;
	int offset2;
	int offset3;
	int offset4;
	int log_i;
	unsigned char woffset1[1];
	unsigned char woffset2[1];
	unsigned char sd_log_time1[4*8];
	unsigned char sd_log_time2[4*8];
	unsigned char sd_log[12*8];
	unsigned char sd_log_data[12];
	
	unsigned char sd_init_err_log[1*8];
	unsigned char sd_init_err_log_data[1];
	
	int log_ret;

	unsigned long timestamp;
	struct rtc_time tm;
	struct timeval tv;
        tm.tm_mon = 0;
        tm.tm_mday = 0;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
/* For log END */

	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_claim_host(host);
#ifdef CONFIG_MMC_PARANOID_SD_INIT
	retries = 5;
	while (retries) {
		err = mmc_sd_init_card(host, host->ocr, host->card);

		if (err) {
			printk(KERN_ERR "%s: Re-init card rc = %d (retries = %d)\n",
			       mmc_hostname(host), err, retries);
			retries--;
			mmc_power_off(host);
			usleep_range(5000, 5500);
			mmc_power_up(host);
			mmc_select_voltage(host, host->ocr);
			continue;
		}
		break;
	}
#else
	err = mmc_sd_init_card(host, host->ocr, host->card);
#endif

/* For log  START */
	if(!retries){
		log_ret = cfgdrv_read(D_HCM_E_SD_INIT_ERR_HST_CNT, 1, (unsigned char *)woffset1);
    	if(log_ret != 0 )
    	{
        	printk("cfgdrv_read error!! for woffset1 \n");
    		goto log_skip2;
    	}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_read((D_HCM_E_SD_INIT_ERR_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time1 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_log_time1 \n");
    			goto log_skip2;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_read((D_HCM_E_SD_INIT_ERR_FACTOR1 + log_i), 1, (unsigned char *)(sd_init_err_log + (1*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_init_err_log \n");
    			goto log_skip2;
    		}
		}
		
		log_ret = cfgdrv_read(D_HCM_E_SD_MFRINFO_HST_CNT, 1, (unsigned char *)woffset2);
    	if(log_ret != 0 )
    	{
			printk("cfgdrv_read error!! for woffset2 \n");
			goto log_skip2;
    	}
	
		for(log_i=0; log_i<8; log_i++){
			log_ret = cfgdrv_read((D_HCM_E_SD_MFRINFO_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time2 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_log_time2 \n");
    			goto log_skip2;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_read((D_HCM_E_SD_MFRINFO_HST1 + log_i), 12, (unsigned char *)(sd_log + (12*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_log \n");
    			goto log_skip2;
    		}
		}
		
#if 0
		printk("[in]offset1     : %d \n", woffset1[0]);
		printk("[in]sd_log_time1: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time1[0], sd_log_time1[1], sd_log_time1[2], sd_log_time1[3],
			sd_log_time1[4], sd_log_time1[5], sd_log_time1[6], sd_log_time1[7],
			sd_log_time1[8], sd_log_time1[9], sd_log_time1[10], sd_log_time1[11],
			sd_log_time1[12], sd_log_time1[13], sd_log_time1[14], sd_log_time1[15],
			sd_log_time1[16], sd_log_time1[17], sd_log_time1[18], sd_log_time1[19],
			sd_log_time1[20], sd_log_time1[21], sd_log_time1[22], sd_log_time1[23],
			sd_log_time1[24], sd_log_time1[25], sd_log_time1[26], sd_log_time1[27],
			sd_log_time1[28], sd_log_time1[29], sd_log_time1[30], sd_log_time1[31]
		);
		printk("[in]sd_init_err_log : %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \n",
			sd_init_err_log[0], sd_init_err_log[1], sd_init_err_log[2], sd_init_err_log[3], 
			sd_init_err_log[4], sd_init_err_log[5], sd_init_err_log[6], sd_init_err_log[7]);
		
		printk("[in]offset3     : %d \n", woffset2[0]);
		printk("[in]sd_log_time2: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time2[0], sd_log_time2[1], sd_log_time2[2], sd_log_time2[3],
			sd_log_time2[4], sd_log_time2[5], sd_log_time2[6], sd_log_time2[7],
			sd_log_time2[8], sd_log_time2[9], sd_log_time2[10], sd_log_time2[11],
			sd_log_time2[12], sd_log_time2[13], sd_log_time2[14], sd_log_time2[15],
			sd_log_time2[16], sd_log_time2[17], sd_log_time2[18], sd_log_time2[19],
			sd_log_time2[20], sd_log_time2[21], sd_log_time2[22], sd_log_time2[23],
			sd_log_time2[24], sd_log_time2[25], sd_log_time2[26], sd_log_time2[27],
			sd_log_time2[28], sd_log_time2[29], sd_log_time2[30], sd_log_time2[31]
		);
		printk("[in]sd_log(0-3): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[0], sd_log[1], sd_log[2], sd_log[3], sd_log[4], sd_log[5], sd_log[6], sd_log[7], sd_log[8], sd_log[9], sd_log[10], sd_log[11], 
			sd_log[12], sd_log[13], sd_log[14], sd_log[15], sd_log[16], sd_log[17], sd_log[18], sd_log[19], sd_log[20], sd_log[21], sd_log[22], sd_log[23], 
			sd_log[24], sd_log[25], sd_log[26], sd_log[27], sd_log[28], sd_log[29], sd_log[30], sd_log[31], sd_log[32], sd_log[33], sd_log[34], sd_log[35], 
			sd_log[36], sd_log[37], sd_log[38], sd_log[39], sd_log[40], sd_log[41], sd_log[42], sd_log[43], sd_log[44], sd_log[45], sd_log[46], sd_log[47] 
		);
		printk("[in]sd_log(4-7): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[48], sd_log[49], sd_log[50], sd_log[51], sd_log[52], sd_log[53], sd_log[54], sd_log[55], sd_log[56], sd_log[57], sd_log[58], sd_log[59], 
			sd_log[60], sd_log[61], sd_log[62], sd_log[63], sd_log[64], sd_log[65], sd_log[66], sd_log[67], sd_log[68], sd_log[69], sd_log[70], sd_log[71], 
			sd_log[72], sd_log[73], sd_log[74], sd_log[75], sd_log[76], sd_log[77], sd_log[78], sd_log[79], sd_log[80], sd_log[81], sd_log[82], sd_log[83], 
			sd_log[84], sd_log[85], sd_log[86], sd_log[87], sd_log[88], sd_log[89], sd_log[90], sd_log[91], sd_log[92], sd_log[93], sd_log[94], sd_log[95] 
		);
#endif /* #if 0 */

		offset1 = (woffset1[0] & 0xff);
		offset2 = (woffset1[0] & 0xff);
		
		offset3 = (woffset2[0] & 0xff);
		offset4 = (woffset2[0] & 0xff);
		
    	/* increment offset position */
	    offset1++;
    	offset2++;
    	offset3++;
		offset4++;
    	((offset1 % 8) == 0) ? (offset1 = 0) : (offset1 = 4*(offset1 % 8));
    	((offset2 % 8) == 0) ? (offset2 = 0) : (offset2 = 1*(offset2 % 8));
    	((offset3 % 8) == 0) ? (offset3 = 0) : (offset3 = 4*(offset3 % 8));
    	((offset4 % 8) == 0) ? (offset4 = 0) : (offset4 = 12*(offset4 % 8));
		
    	/* get time info */
		do_gettimeofday(&tv);
		rtc_time_to_tm(tv.tv_sec, &tm);
		timestamp = ((((((tm.tm_mon +1) << 22)
						| ((unsigned long)tm.tm_mday << 17))
						| ((unsigned long)tm.tm_hour << 12))
						| ((unsigned long)tm.tm_min << 6))
						| (unsigned long)tm.tm_sec);
		
		memcpy(&(sd_log_time1[offset1]), (unsigned char *)&timestamp, sizeof(timestamp));
		memcpy(&(sd_log_time2[offset3]), (unsigned char *)&timestamp, sizeof(timestamp));
		
		sd_init_err_log_data[0] = 2; /* sd init err on resume factor number */
		
		memcpy(&(sd_init_err_log[offset2]), sd_init_err_log_data, sizeof(sd_init_err_log_data));
		
		offset1 = (woffset1[0] & 0xff);
    	offset1++;
    	((offset1 % 8) == 0) ? (offset1 = 0) : (offset1 = (offset1 % 8));
		woffset1[0] = offset1;
		
		memcpy((unsigned char *)&(sd_log_data[0]), (unsigned char *)&(log_cid.manfid), sizeof(log_cid.manfid));
		memcpy((unsigned char *)&(sd_log_data[4]), (unsigned char *)&(log_cid.oemid), sizeof(log_cid.oemid));
		memcpy((unsigned char *)&(sd_log_data[6]), (unsigned char *)(log_cid.prod_name), 5);
		sd_log_data[11] =  (unsigned char)0;
		
		memcpy(&(sd_log[offset4]), sd_log_data, sizeof(sd_log_data));
		
		offset3 = (woffset2[0] & 0xff);
    	offset3++;
    	((offset3 % 8) == 0) ? (offset3 = 0) : (offset3 = (offset3 % 8));
		woffset2[0] = offset3;
		
#if 0
		printk("timestamp : mon+1 = %d, day = %d, hour = %d, min = %d, sec = %d \n",
				tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		printk("[out]offset1     : %d \n", woffset1[0]);
		printk("[out]sd_log_time1: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time1[0], sd_log_time1[1], sd_log_time1[2], sd_log_time1[3],
			sd_log_time1[4], sd_log_time1[5], sd_log_time1[6], sd_log_time1[7],
			sd_log_time1[8], sd_log_time1[9], sd_log_time1[10], sd_log_time1[11],
			sd_log_time1[12], sd_log_time1[13], sd_log_time1[14], sd_log_time1[15],
			sd_log_time1[16], sd_log_time1[17], sd_log_time1[18], sd_log_time1[19],
			sd_log_time1[20], sd_log_time1[21], sd_log_time1[22], sd_log_time1[23],
			sd_log_time1[24], sd_log_time1[25], sd_log_time1[26], sd_log_time1[27],
			sd_log_time1[28], sd_log_time1[29], sd_log_time1[30], sd_log_time1[31]
		);
		printk("[out]sd_init_err_log : %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \n",
			sd_init_err_log[0], sd_init_err_log[1], sd_init_err_log[2], sd_init_err_log[3], 
			sd_init_err_log[4], sd_init_err_log[5], sd_init_err_log[6], sd_init_err_log[7]);
		
		printk("[out]log_cid.manfid   :  0x%X \n", log_cid.manfid);
		printk("[out]log_cid.oemid    :  0x%X \n", log_cid.oemid);
		
		printk("timestamp : mon+1 = %d, day = %d, hour = %d, min = %d, sec = %d \n",
			tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		
		printk("[out]offset3     : %d \n", woffset2[0]);
		printk("[out]sd_log_time2: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time2[0], sd_log_time2[1], sd_log_time2[2], sd_log_time2[3],
			sd_log_time2[4], sd_log_time2[5], sd_log_time2[6], sd_log_time2[7],
			sd_log_time2[8], sd_log_time2[9], sd_log_time2[10], sd_log_time2[11],
			sd_log_time2[12], sd_log_time2[13], sd_log_time2[14], sd_log_time2[15],
			sd_log_time2[16], sd_log_time2[17], sd_log_time2[18], sd_log_time2[19],
			sd_log_time2[20], sd_log_time2[21], sd_log_time2[22], sd_log_time2[23],
			sd_log_time2[24], sd_log_time2[25], sd_log_time2[26], sd_log_time2[27],
			sd_log_time2[28], sd_log_time2[29], sd_log_time2[30], sd_log_time2[31]
		);
		printk("[out]sd_log(0-3): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[0], sd_log[1], sd_log[2], sd_log[3], sd_log[4], sd_log[5], sd_log[6], sd_log[7], sd_log[8], sd_log[9], sd_log[10], sd_log[11], 
			sd_log[12], sd_log[13], sd_log[14], sd_log[15], sd_log[16], sd_log[17], sd_log[18], sd_log[19], sd_log[20], sd_log[21], sd_log[22], sd_log[23], 
			sd_log[24], sd_log[25], sd_log[26], sd_log[27], sd_log[28], sd_log[29], sd_log[30], sd_log[31], sd_log[32], sd_log[33], sd_log[34], sd_log[35], 
			sd_log[36], sd_log[37], sd_log[38], sd_log[39], sd_log[40], sd_log[41], sd_log[42], sd_log[43], sd_log[44], sd_log[45], sd_log[46], sd_log[47] 
		);
		printk("[out]sd_log(4-7): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[48], sd_log[49], sd_log[50], sd_log[51], sd_log[52], sd_log[53], sd_log[54], sd_log[55], sd_log[56], sd_log[57], sd_log[58], sd_log[59], 
			sd_log[60], sd_log[61], sd_log[62], sd_log[63], sd_log[64], sd_log[65], sd_log[66], sd_log[67], sd_log[68], sd_log[69], sd_log[70], sd_log[71], 
			sd_log[72], sd_log[73], sd_log[74], sd_log[75], sd_log[76], sd_log[77], sd_log[78], sd_log[79], sd_log[80], sd_log[81], sd_log[82], sd_log[83], 
			sd_log[84], sd_log[85], sd_log[86], sd_log[87], sd_log[88], sd_log[89], sd_log[90], sd_log[91], sd_log[92], sd_log[93], sd_log[94], sd_log[95] 
		);
#endif /* #if 0 */

		for(log_i=0; log_i<8; log_i++){
			log_ret = cfgdrv_write((D_HCM_E_SD_INIT_ERR_FACTOR1 + log_i), 1, (unsigned char *)(sd_init_err_log + (1*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_init_err_log \n");
    			goto log_skip2;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_write((D_HCM_E_SD_INIT_ERR_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time1 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_log_time1 \n");
    			goto log_skip2;
    		}
		}
		
    	log_ret = cfgdrv_write(D_HCM_E_SD_INIT_ERR_HST_CNT, 1, (unsigned char *)woffset1);
    	if(log_ret != 0 )
    	{
        	printk("cfgdrv_write error!! for woffset1 \n");
    		goto log_skip2;
    	}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_write((D_HCM_E_SD_MFRINFO_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time2 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_log_time2 \n");
    			goto log_skip2;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_write((D_HCM_E_SD_MFRINFO_HST1 + log_i), 12, (unsigned char *)(sd_log + (12*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_log \n");
    			goto log_skip2;
    		}
		}
		
    	log_ret = cfgdrv_write(D_HCM_E_SD_MFRINFO_HST_CNT, 1, (unsigned char *)woffset2);
    	if(log_ret != 0 )
    	{
        	printk("cfgdrv_write error!! for woffset2 \n");
    		goto log_skip2;
    	}
	}

log_skip2:
/* For log  END */

	mmc_release_host(host);

	return err;
}

static int mmc_sd_power_restore(struct mmc_host *host)
{
	int ret;

	host->card->state &= ~MMC_STATE_HIGHSPEED;
	mmc_claim_host(host);
	ret = mmc_sd_init_card(host, host->ocr, host->card);
	mmc_release_host(host);

	return ret;
}

static const struct mmc_bus_ops mmc_sd_ops = {
	.remove = mmc_sd_remove,
	.detect = mmc_sd_detect,
	.suspend = NULL,
	.resume = NULL,
	.power_restore = mmc_sd_power_restore,
	.alive = mmc_sd_alive,
};

static const struct mmc_bus_ops mmc_sd_ops_unsafe = {
	.remove = mmc_sd_remove,
	.detect = mmc_sd_detect,
	.suspend = mmc_sd_suspend,
	.resume = mmc_sd_resume,
	.power_restore = mmc_sd_power_restore,
	.alive = mmc_sd_alive,
};

static void mmc_sd_attach_bus_ops(struct mmc_host *host)
{
	const struct mmc_bus_ops *bus_ops;

	if (!mmc_card_is_removable(host))
		bus_ops = &mmc_sd_ops_unsafe;
	else
		bus_ops = &mmc_sd_ops;
	mmc_attach_bus(host, bus_ops);
}

/*
 * Starting point for SD card init.
 */
int mmc_attach_sd(struct mmc_host *host)
{
	int err;
	u32 ocr;
#ifdef CONFIG_MMC_PARANOID_SD_INIT
	int retries;
#endif
/* For log START */
	int offset1;
	int offset2;
	int offset3;
	int offset4;
	int log_i;
	unsigned char woffset1[1];
	unsigned char woffset2[1];
	unsigned char sd_log_time1[4*8];
	unsigned char sd_log_time2[4*8];
	unsigned char sd_log[12*8];
	unsigned char sd_log_data[12];
	
	unsigned char sd_init_err_log[1*8];
	unsigned char sd_init_err_log_data[1];
	
	int log_ret;

	unsigned long timestamp;
	struct rtc_time tm;
	struct timeval tv;
        tm.tm_mon = 0;
        tm.tm_mday = 0;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

	memset(&log_cid, 0, sizeof(struct mmc_cid));
/* For log END */

	BUG_ON(!host);
	WARN_ON(!host->claimed);

	/* Disable preset value enable if already set since last time */
	if (host->ops->enable_preset_value) {
		mmc_host_clk_hold(host);
		host->ops->enable_preset_value(host, false);
		mmc_host_clk_release(host);
	}

	err = mmc_send_app_op_cond(host, 0, &ocr);
	if (err)
		return err;

	mmc_sd_attach_bus_ops(host);
	if (host->ocr_avail_sd)
		host->ocr_avail = host->ocr_avail_sd;

	/*
	 * We need to get OCR a different way for SPI.
	 */
	if (mmc_host_is_spi(host)) {
		mmc_go_idle(host);

		err = mmc_spi_read_ocr(host, 0, &ocr);
		if (err)
			goto err;
	}

	/*
	 * Sanity check the voltages that the card claims to
	 * support.
	 */
	if (ocr & 0x7F) {
		pr_warning("%s: card claims to support voltages "
		       "below the defined range. These will be ignored.\n",
		       mmc_hostname(host));
		ocr &= ~0x7F;
	}

	if ((ocr & MMC_VDD_165_195) &&
	    !(host->ocr_avail_sd & MMC_VDD_165_195)) {
		pr_warning("%s: SD card claims to support the "
		       "incompletely defined 'low voltage range'. This "
		       "will be ignored.\n", mmc_hostname(host));
		ocr &= ~MMC_VDD_165_195;
	}

	host->ocr = mmc_select_voltage(host, ocr);

	/*
	 * Can we support the voltage(s) of the card(s)?
	 */
	if (!host->ocr) {
		err = -EINVAL;
		goto err;
	}

	/*
	 * Detect and init the card.
	 */
#ifdef CONFIG_MMC_PARANOID_SD_INIT
	retries = 5;
	while (retries) {
		err = mmc_sd_init_card(host, host->ocr, NULL);
		if (err) {
			retries--;
			mmc_power_off(host);
			usleep_range(5000, 5500);
			mmc_power_up(host);
			mmc_select_voltage(host, host->ocr);
			continue;
		}
		break;
	}

	if (!retries) {
		printk(KERN_ERR "%s: mmc_sd_init_card() failure (err = %d)\n",
		       mmc_hostname(host), err);
		
/* For log  START */
		mmc_delay(3000);	/* wait for ready on cfgdrv */
		/* polling if cfgdrv initialize finished while max 3000msec */
		for(log_i=0; log_i<80; log_i++){
			log_ret = cfgdrv_read(D_HCM_E_SD_INIT_ERR_HST_CNT, 1, (unsigned char *)woffset1);
    		if(log_ret == 0 )
    		{
    			break;
    		}
			mmc_delay(50);
		}
		
		if(log_i >= 80){
        	printk("cfgdrv_read error!! for woffset1 \n");
    		goto err;
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_read((D_HCM_E_SD_INIT_ERR_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time1 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_log_time1 \n");
    			goto err;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_read((D_HCM_E_SD_INIT_ERR_FACTOR1 + log_i), 1, (unsigned char *)(sd_init_err_log + (1*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_init_err_log \n");
    			goto err;
    		}
		}
		
		log_ret = cfgdrv_read(D_HCM_E_SD_MFRINFO_HST_CNT, 1, (unsigned char *)woffset2);
    	if(log_ret != 0 )
    	{
			printk("cfgdrv_read error!! for woffset2 \n");
			goto err;
    	}
	
		for(log_i=0; log_i<8; log_i++){
			log_ret = cfgdrv_read((D_HCM_E_SD_MFRINFO_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time2 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_log_time2 \n");
    			goto err;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_read((D_HCM_E_SD_MFRINFO_HST1 + log_i), 12, (unsigned char *)(sd_log + (12*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_read error!! for sd_log \n");
    			goto err;
    		}
		}
		
#if 0
		printk("[in]offset1     : %d \n", woffset1[0]);
		printk("[in]sd_log_time1: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time1[0], sd_log_time1[1], sd_log_time1[2], sd_log_time1[3],
			sd_log_time1[4], sd_log_time1[5], sd_log_time1[6], sd_log_time1[7],
			sd_log_time1[8], sd_log_time1[9], sd_log_time1[10], sd_log_time1[11],
			sd_log_time1[12], sd_log_time1[13], sd_log_time1[14], sd_log_time1[15],
			sd_log_time1[16], sd_log_time1[17], sd_log_time1[18], sd_log_time1[19],
			sd_log_time1[20], sd_log_time1[21], sd_log_time1[22], sd_log_time1[23],
			sd_log_time1[24], sd_log_time1[25], sd_log_time1[26], sd_log_time1[27],
			sd_log_time1[28], sd_log_time1[29], sd_log_time1[30], sd_log_time1[31]
		);
		printk("[in]sd_init_err_log : %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \n",
			sd_init_err_log[0], sd_init_err_log[1], sd_init_err_log[2], sd_init_err_log[3], 
			sd_init_err_log[4], sd_init_err_log[5], sd_init_err_log[6], sd_init_err_log[7]);
		
		printk("[in]offset3     : %d \n", woffset2[0]);
		printk("[in]sd_log_time2: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time2[0], sd_log_time2[1], sd_log_time2[2], sd_log_time2[3],
			sd_log_time2[4], sd_log_time2[5], sd_log_time2[6], sd_log_time2[7],
			sd_log_time2[8], sd_log_time2[9], sd_log_time2[10], sd_log_time2[11],
			sd_log_time2[12], sd_log_time2[13], sd_log_time2[14], sd_log_time2[15],
			sd_log_time2[16], sd_log_time2[17], sd_log_time2[18], sd_log_time2[19],
			sd_log_time2[20], sd_log_time2[21], sd_log_time2[22], sd_log_time2[23],
			sd_log_time2[24], sd_log_time2[25], sd_log_time2[26], sd_log_time2[27],
			sd_log_time2[28], sd_log_time2[29], sd_log_time2[30], sd_log_time2[31]
		);
		printk("[in]sd_log(0-3): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[0], sd_log[1], sd_log[2], sd_log[3], sd_log[4], sd_log[5], sd_log[6], sd_log[7], sd_log[8], sd_log[9], sd_log[10], sd_log[11], 
			sd_log[12], sd_log[13], sd_log[14], sd_log[15], sd_log[16], sd_log[17], sd_log[18], sd_log[19], sd_log[20], sd_log[21], sd_log[22], sd_log[23], 
			sd_log[24], sd_log[25], sd_log[26], sd_log[27], sd_log[28], sd_log[29], sd_log[30], sd_log[31], sd_log[32], sd_log[33], sd_log[34], sd_log[35], 
			sd_log[36], sd_log[37], sd_log[38], sd_log[39], sd_log[40], sd_log[41], sd_log[42], sd_log[43], sd_log[44], sd_log[45], sd_log[46], sd_log[47] 
		);
		printk("[in]sd_log(4-7): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[48], sd_log[49], sd_log[50], sd_log[51], sd_log[52], sd_log[53], sd_log[54], sd_log[55], sd_log[56], sd_log[57], sd_log[58], sd_log[59], 
			sd_log[60], sd_log[61], sd_log[62], sd_log[63], sd_log[64], sd_log[65], sd_log[66], sd_log[67], sd_log[68], sd_log[69], sd_log[70], sd_log[71], 
			sd_log[72], sd_log[73], sd_log[74], sd_log[75], sd_log[76], sd_log[77], sd_log[78], sd_log[79], sd_log[80], sd_log[81], sd_log[82], sd_log[83], 
			sd_log[84], sd_log[85], sd_log[86], sd_log[87], sd_log[88], sd_log[89], sd_log[90], sd_log[91], sd_log[92], sd_log[93], sd_log[94], sd_log[95] 
		);
#endif /* #if 0 */

		offset1 = (woffset1[0] & 0xff);
		offset2 = (woffset1[0] & 0xff);
		
		offset3 = (woffset2[0] & 0xff);
		offset4 = (woffset2[0] & 0xff);
		
    	/* increment offset position */
	    offset1++;
    	offset2++;
    	offset3++;
		offset4++;
    	((offset1 % 8) == 0) ? (offset1 = 0) : (offset1 = 4*(offset1 % 8));
    	((offset2 % 8) == 0) ? (offset2 = 0) : (offset2 = 1*(offset2 % 8));
    	((offset3 % 8) == 0) ? (offset3 = 0) : (offset3 = 4*(offset3 % 8));
    	((offset4 % 8) == 0) ? (offset4 = 0) : (offset4 = 12*(offset4 % 8));
		
    	/* get time info */
		do_gettimeofday(&tv);
		rtc_time_to_tm(tv.tv_sec, &tm);
		timestamp = ((((((tm.tm_mon +1) << 22)
						| ((unsigned long)tm.tm_mday << 17))
						| ((unsigned long)tm.tm_hour << 12))
						| ((unsigned long)tm.tm_min << 6))
						| (unsigned long)tm.tm_sec);
		
		memcpy(&(sd_log_time1[offset1]), (unsigned char *)&timestamp, sizeof(timestamp));
		memcpy(&(sd_log_time2[offset3]), (unsigned char *)&timestamp, sizeof(timestamp));
		
		sd_init_err_log_data[0] = 1; /* sd init err on terminal power on factor number */
		
		memcpy(&(sd_init_err_log[offset2]), sd_init_err_log_data, sizeof(sd_init_err_log_data));
		
		offset1 = (woffset1[0] & 0xff);
    	offset1++;
    	((offset1 % 8) == 0) ? (offset1 = 0) : (offset1 = (offset1 % 8));
		woffset1[0] = offset1;
		
		memcpy((unsigned char *)&(sd_log_data[0]), (unsigned char *)&(log_cid.manfid), sizeof(log_cid.manfid));
		memcpy((unsigned char *)&(sd_log_data[4]), (unsigned char *)&(log_cid.oemid), sizeof(log_cid.oemid));
		memcpy((unsigned char *)&(sd_log_data[6]), (unsigned char *)(log_cid.prod_name), 5);
		sd_log_data[11] =  (unsigned char)0;
		
		memcpy(&(sd_log[offset4]), sd_log_data, sizeof(sd_log_data));
		
		offset3 = (woffset2[0] & 0xff);
    	offset3++;
    	((offset3 % 8) == 0) ? (offset3 = 0) : (offset3 = (offset3 % 8));
		woffset2[0] = offset3;
		
#if 0
		printk("timestamp : mon+1 = %d, day = %d, hour = %d, min = %d, sec = %d \n",
				tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		printk("[out]offset1     : %d \n", woffset1[0]);
		printk("[out]sd_log_time1: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time1[0], sd_log_time1[1], sd_log_time1[2], sd_log_time1[3],
			sd_log_time1[4], sd_log_time1[5], sd_log_time1[6], sd_log_time1[7],
			sd_log_time1[8], sd_log_time1[9], sd_log_time1[10], sd_log_time1[11],
			sd_log_time1[12], sd_log_time1[13], sd_log_time1[14], sd_log_time1[15],
			sd_log_time1[16], sd_log_time1[17], sd_log_time1[18], sd_log_time1[19],
			sd_log_time1[20], sd_log_time1[21], sd_log_time1[22], sd_log_time1[23],
			sd_log_time1[24], sd_log_time1[25], sd_log_time1[26], sd_log_time1[27],
			sd_log_time1[28], sd_log_time1[29], sd_log_time1[30], sd_log_time1[31]
		);
		printk("[out]sd_init_err_log : %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \n",
			sd_init_err_log[0], sd_init_err_log[1], sd_init_err_log[2], sd_init_err_log[3], 
			sd_init_err_log[4], sd_init_err_log[5], sd_init_err_log[6], sd_init_err_log[7]);
		
		printk("[out]log_cid.manfid   :  0x%X \n", log_cid.manfid);
		printk("[out]log_cid.oemid    :  0x%X \n", log_cid.oemid);
		
		printk("timestamp : mon+1 = %d, day = %d, hour = %d, min = %d, sec = %d \n",
			tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		
		printk("[out]offset3     : %d \n", woffset2[0]);
		printk("[out]sd_log_time2: %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X\n", 
			sd_log_time2[0], sd_log_time2[1], sd_log_time2[2], sd_log_time2[3],
			sd_log_time2[4], sd_log_time2[5], sd_log_time2[6], sd_log_time2[7],
			sd_log_time2[8], sd_log_time2[9], sd_log_time2[10], sd_log_time2[11],
			sd_log_time2[12], sd_log_time2[13], sd_log_time2[14], sd_log_time2[15],
			sd_log_time2[16], sd_log_time2[17], sd_log_time2[18], sd_log_time2[19],
			sd_log_time2[20], sd_log_time2[21], sd_log_time2[22], sd_log_time2[23],
			sd_log_time2[24], sd_log_time2[25], sd_log_time2[26], sd_log_time2[27],
			sd_log_time2[28], sd_log_time2[29], sd_log_time2[30], sd_log_time2[31]
		);
		printk("[out]sd_log(0-3): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[0], sd_log[1], sd_log[2], sd_log[3], sd_log[4], sd_log[5], sd_log[6], sd_log[7], sd_log[8], sd_log[9], sd_log[10], sd_log[11], 
			sd_log[12], sd_log[13], sd_log[14], sd_log[15], sd_log[16], sd_log[17], sd_log[18], sd_log[19], sd_log[20], sd_log[21], sd_log[22], sd_log[23], 
			sd_log[24], sd_log[25], sd_log[26], sd_log[27], sd_log[28], sd_log[29], sd_log[30], sd_log[31], sd_log[32], sd_log[33], sd_log[34], sd_log[35], 
			sd_log[36], sd_log[37], sd_log[38], sd_log[39], sd_log[40], sd_log[41], sd_log[42], sd_log[43], sd_log[44], sd_log[45], sd_log[46], sd_log[47] 
		);
		printk("[out]sd_log(4-7): %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X, %.2X%.2X%.2X%.2X %.2X%.2X %.2X%.2X%.2X%.2X%.2X %.2X\n",
			sd_log[48], sd_log[49], sd_log[50], sd_log[51], sd_log[52], sd_log[53], sd_log[54], sd_log[55], sd_log[56], sd_log[57], sd_log[58], sd_log[59], 
			sd_log[60], sd_log[61], sd_log[62], sd_log[63], sd_log[64], sd_log[65], sd_log[66], sd_log[67], sd_log[68], sd_log[69], sd_log[70], sd_log[71], 
			sd_log[72], sd_log[73], sd_log[74], sd_log[75], sd_log[76], sd_log[77], sd_log[78], sd_log[79], sd_log[80], sd_log[81], sd_log[82], sd_log[83], 
			sd_log[84], sd_log[85], sd_log[86], sd_log[87], sd_log[88], sd_log[89], sd_log[90], sd_log[91], sd_log[92], sd_log[93], sd_log[94], sd_log[95] 
		);
#endif /* #if 0 */

		for(log_i=0; log_i<8; log_i++){
			log_ret = cfgdrv_write((D_HCM_E_SD_INIT_ERR_FACTOR1 + log_i), 1, (unsigned char *)(sd_init_err_log + (1*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_init_err_log \n");
    			goto err;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_write((D_HCM_E_SD_INIT_ERR_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time1 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_log_time1 \n");
    			goto err;
    		}
		}
		
    	log_ret = cfgdrv_write(D_HCM_E_SD_INIT_ERR_HST_CNT, 1, (unsigned char *)woffset1);
    	if(log_ret != 0 )
    	{
        	printk("cfgdrv_write error!! for woffset1 \n");
    		goto err;
    	}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_write((D_HCM_E_SD_MFRINFO_HST_TIMESTAMP1 + log_i), 4, (unsigned char *)(sd_log_time2 + (4*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_log_time2 \n");
    			goto err;
    		}
		}
		
		for(log_i=0; log_i<8; log_i++){
    		log_ret = cfgdrv_write((D_HCM_E_SD_MFRINFO_HST1 + log_i), 12, (unsigned char *)(sd_log + (12*log_i)));
    		if(log_ret != 0 )
    		{
        		printk("cfgdrv_write error!! for sd_log \n");
    			goto err;
    		}
		}
		
    	log_ret = cfgdrv_write(D_HCM_E_SD_MFRINFO_HST_CNT, 1, (unsigned char *)woffset2);
    	if(log_ret != 0 )
    	{
        	printk("cfgdrv_write error!! for woffset2 \n");
    		goto err;
    	}
/* For log  END */
		
		goto err;
	}
#else
	err = mmc_sd_init_card(host, host->ocr, NULL);
	if (err)
		goto err;
#endif

	mmc_release_host(host);
	err = mmc_add_card(host->card);
	mmc_claim_host(host);
	if (err)
		goto remove_card;

	return 0;

remove_card:
	mmc_release_host(host);
	mmc_remove_card(host->card);
	host->card = NULL;
	mmc_claim_host(host);
err:
	mmc_detach_bus(host);

	pr_err("%s: error %d whilst initialising SD card\n",
		mmc_hostname(host), err);

	return err;
}

