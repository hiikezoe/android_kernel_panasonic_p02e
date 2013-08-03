/*
 * drivers/fgc/inc/pfgc_adrd_local.h
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

#ifndef _pfgc_adrd_local_h                                /* _pfgc_adrd_local_h         */
#define _pfgc_adrd_local_h                                /* _pfgc_adrd_local_h         */

#include "pfgc_compatibility.h"

/** structure definition **/
struct t_fgc_sysfs {
	const signed char	*name;
	struct device 		*dev;
};

struct t_fgc_i2c_dev_info {
	struct device 		*dev;
	int			id;
	struct i2c_client	*client;
};

struct t_fgc_device_info {
	struct device 			class_dev;
	struct t_fgc_i2c_dev_info	*i2c_dev;
	struct t_fgc_i2c_dev_info	*i2c_rom_dev;
};

/** function declaration **/
/* initialize/terminate */
extern unsigned char pfgc_adrd_init( void );
extern void pfgc_adrd_exit( void );

/* register access */
extern void __iomem *pfgc_get_reg_addr( unsigned int offset );

/* charac device */
extern int pfgc_register_chrdev( unsigned int major, const char *name, struct file_operations *fops );
extern void pfgc_unregister_chrdev( unsigned int major, const char *name );

/* irq */
extern int pfgc_request_irq( irqreturn_t ( *port_callback )( int irq, void *dev_id ) );
extern int pfgc_enable_irq_wake( void );
extern void pfgc_free_irq( void );

/* capacity */
extern int pfgc_get_capcity_ini( void );
extern int pfgc_get_capcity( void );
extern int pfgc_get_capcity_nolva( void );

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/* degradation */
extern int pfgc_get_degradation_info( int property_id ); // Conan
#endif /*<Conan>1111XX*/

/* sysfs */
extern int pfgc_class_register( void );
extern void pfgc_class_unregister( void );
extern int pfgc_get_property( struct power_supply *psy,
                enum power_supply_property psp, union power_supply_propval *val );

/* i2c */
extern int pfgc_i2c_init( void );
extern void pfgc_i2c_exit( void );
extern unsigned long pfgc_i2c_write( TCOM_RW_DATA *data );
extern unsigned long pfgc_i2c_read( TCOM_RW_DATA *data );
extern void pfgc_i2c_change_rate( unsigned long rate );
extern int pfgc_i2c_sleep_set( unsigned short type );
extern int pfgc_i2c_get_controlreg( unsigned short *pdata );

#endif                                                    /* _pfgc_adrd_local_h         */

