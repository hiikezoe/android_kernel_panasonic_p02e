#ifndef _LINUX_SPI_SUBPMIC_H
#define _LINUX_SPI_SUBPMIC_H

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/spi/spi.h>
#include <mach/irqs.h>

#include <linux/i2c.h>

/* Read and write several 8-bit registers at once. */
extern int subpmic_i2c_bitset_u8(u8 addr, u8 value, u8 mode);

enum {
	SUBPMIC_BITSET_ON = 0x00,
	SUBPMIC_BITSET_OFF,
	SUBPMIC_BITSET_NUM
};

#endif
