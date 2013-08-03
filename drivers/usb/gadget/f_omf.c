/*
 * Gadget Driver for MCPC OMF
 *
 * Copyright (C) 2011 Panasonic Mobile Communications, Inc.
 *
 * This code borrows from f_adb.c, which is
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/usb.h>
#include <linux/usb_usual.h>
#include <linux/usb/ch9.h>
#include <linux/usb/cdc.h>
// <iDPower_002> mod S
//#include <linux/usb/android_composite.h>
#include <linux/usb/composite.h>
// <iDPower_002> mod E
#include <linux/usb/f_omf.h>

static struct t_omf_iffunc omf_iff;

#define _dbgmsg(fmt, ...)    printk(KERN_INFO "<< OMF >>[%s]:[%d]:"fmt, __func__,__LINE__, ##__VA_ARGS__)

/* PipeGroup 1 */
/* Interface Association Descriptor */
/* DEL-S */
#if 0
static struct usb_interface_assoc_descriptor pg1_iad_desc = {
	.bLength            = sizeof pg1_iad_desc,
	.bDescriptorType    = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface    = 0,
	.bInterfaceCount    = 2,
	.bFunctionClass     = USB_CLASS_COMM,
	.bFunctionSubClass  = 0x88,
	.bFunctionProtocol  = USB_CDC_ACM_PROTO_AT_V25TER,
	.iFunction          = 0,
};
#endif
/* DEL-E */

/* Standard interface Descriptor (communication) */
static struct usb_interface_descriptor pg1_intf_desc_comm = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bAlternateSetting      = 0,
	.bNumEndpoints          = 1,
	.bInterfaceClass        = USB_CLASS_COMM,
	.bInterfaceSubClass     = 0x88,
	.bInterfaceProtocol     = 1,
	.iInterface             = 0,
};

/* Header Functional Descriptor */
static struct usb_cdc_header_desc pg1_cdc_header = {
	.bLength            = sizeof pg1_cdc_header,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = USB_CDC_HEADER_TYPE,
	.bcdCDC             = __constant_cpu_to_le16(0x0110),
};

/* Control Management Functional Descriptor */
static struct usb_cdc_call_mgmt_descriptor pg1_call_mgmt = {
	.bLength            =  sizeof pg1_call_mgmt,
	.bDescriptorType    =  USB_DT_CS_INTERFACE,
	.bDescriptorSubType =  USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities     =  (USB_CDC_CALL_MGMT_CAP_CALL_MGMT | USB_CDC_CALL_MGMT_CAP_DATA_INTF),
	.bDataInterface     =  0x01,
};

/* Abstract Controll Magagement  Descriptor */
static struct usb_cdc_acm_descriptor pg1_acm_desc = {
	.bLength            = sizeof pg1_acm_desc,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = USB_CDC_ACM_TYPE,
	.bmCapabilities     = (USB_CDC_CAP_LINE|USB_CDC_CAP_BRK),
};

/* Union Function Descriptor */
static struct usb_cdc_union_desc pg1_union_desc = {
	.bLength            = sizeof pg1_union_desc,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = USB_CDC_UNION_TYPE,
	.bMasterInterface0  = 0,
	.bSlaveInterface0   = 1,
};

/* Mobile Abstract Control Medel Specific Function Descriptor */
static struct usb_model_specific_func_desc pg1_model_desc = {
/* CHG-S */
/*	.bLength            = 6,*/
	.bLength            = 5,
/* CHG-E */
	.bDescriptorType    = (USB_TYPE_VENDOR | USB_DT_INTERFACE),
	.bDescriptorSubType = 0x11,
	.bType              = 0x02,
/* CHG-S */
/*	.bMode              = {0x01, 0xC0},*/
/*	.mode_num           = 2,*/
// <iDPower_002> mod S
//	.bMode              = 0xC0,
	.bMode              = {0xC0, 0x00, 0x00},
// <iDPower_002> mod E
	.mode_num           = 1,
/* CHG-E */
};

/* Standard endpoint Descriptor (interrupt) (full speed) */
static struct usb_endpoint_descriptor pg1_ep_intr_desc = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x01,
	.bmAttributes       = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize     = __constant_cpu_to_le16(16),
	.bInterval          = 0x10,
};

/* Standard endpoint Descriptor (interrupt) (high speed) */
static struct usb_endpoint_descriptor pg1_ep_intr_desc_hs = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x01,
	.bmAttributes       = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize     = __constant_cpu_to_le16(16),
	.bInterval          = 0x08,
};

/* Standard interface Descriptor */
static struct usb_interface_descriptor pg1_intf_desc_bulk = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 1,
	.bAlternateSetting      = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = USB_CLASS_CDC_DATA,
	.bInterfaceSubClass     = 0,
	.bInterfaceProtocol     = 0,
	.iInterface             = 0,
};

/* Standard endpoint Descriptor (bulk) (high speed) */
static struct usb_endpoint_descriptor pg1_ep_in_desc_hs = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x02,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = __constant_cpu_to_le16(512),
	.bInterval          = 0x00,
};

/* Standard endpoint Descriptor (bulk) (high speed) */
static struct usb_endpoint_descriptor pg1_ep_out_desc_hs = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_OUT | 0x03,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = __constant_cpu_to_le16(512),
	.bInterval          = 0x00,
};

/* Standard endpoint Descriptor (bulk) (full speed) */
static struct usb_endpoint_descriptor pg1_ep_in_desc = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x02,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.bInterval          = 0x00,
};

/* Standard endpoint Descriptor (bulk) (full speed) */
static struct usb_endpoint_descriptor pg1_ep_out_desc = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_OUT | 0x03,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.bInterval          = 0x00,
};

static struct usb_descriptor_header *fs_omf_descs[] = {
	/* PipeGroup 1 */
/* DEL-S */
/*	(struct usb_descriptor_header *) &pg1_iad_desc,*/
/* DEL-E */
	(struct usb_descriptor_header *) &pg1_intf_desc_comm,
	(struct usb_descriptor_header *) &pg1_cdc_header,
	(struct usb_descriptor_header *) &pg1_call_mgmt,
	(struct usb_descriptor_header *) &pg1_acm_desc,
	(struct usb_descriptor_header *) &pg1_union_desc,
	(struct usb_descriptor_header *) &pg1_model_desc,
	(struct usb_descriptor_header *) &pg1_ep_intr_desc,
	(struct usb_descriptor_header *) &pg1_intf_desc_bulk,
	(struct usb_descriptor_header *) &pg1_ep_in_desc,
	(struct usb_descriptor_header *) &pg1_ep_out_desc,

	NULL,
};

static struct usb_descriptor_header *hs_omf_descs[] = {
	/* PipeGroup 1 */
/* DEL-S */
/*	(struct usb_descriptor_header *) &pg1_iad_desc,*/
/* DEL-E */
	(struct usb_descriptor_header *) &pg1_intf_desc_comm,
	(struct usb_descriptor_header *) &pg1_cdc_header,
	(struct usb_descriptor_header *) &pg1_call_mgmt,
	(struct usb_descriptor_header *) &pg1_acm_desc,
	(struct usb_descriptor_header *) &pg1_union_desc,
	(struct usb_descriptor_header *) &pg1_model_desc,
	(struct usb_descriptor_header *) &pg1_ep_intr_desc_hs,
	(struct usb_descriptor_header *) &pg1_intf_desc_bulk,
	(struct usb_descriptor_header *) &pg1_ep_in_desc_hs,
	(struct usb_descriptor_header *) &pg1_ep_out_desc_hs,

	NULL,
};

static struct omf_dev *_omf_dev;

// <iDPower_002> mod S
//static inline struct omf_dev *func_to_dev(struct usb_function *f)
static inline struct omf_dev *func_to_omf(struct usb_function *f)
// <iDPower_002> mod E
{
	return container_of(f, struct omf_dev, function);
}

static void omf_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	_dbgmsg( "IN\n" );

	if( omf_iff.if_complete_in )
		(omf_iff.if_complete_in)(ep,req);

	_dbgmsg( "OUT\n" );
}

static void omf_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	_dbgmsg( "IN\n" );

	if( omf_iff.if_complete_out )
		(omf_iff.if_complete_out)(ep,req);

	_dbgmsg( "OUT\n" );
}

static void omf_complete_intr(struct usb_ep *ep, struct usb_request *req)
{
	_dbgmsg( "IN\n" );

	if( omf_iff.if_complete_intr )
		(omf_iff.if_complete_intr)(ep,req);

	_dbgmsg( "OUT\n" );
}

static struct usb_request *omf_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void omf_request_free(struct usb_request *req, struct usb_ep *ep)
{
	_dbgmsg( "IN\n" );
	if(req) {
		kfree( req->buf );
		usb_ep_free_request( ep, req );
	}
	_dbgmsg( "OUT\n" );
}

static int omf_allocate_interface_ids( struct usb_configuration *c, struct usb_function *f )
{
	int id;
// <iDPower_002> mod S
//	struct omf_dev *dev = func_to_dev(f);
	struct omf_dev *dev = func_to_omf(f);
// <iDPower_002> mod E

	_dbgmsg( "IN\n" );
	/* Allocate Interface ID: PipeGroup1 communication interface */
	id = usb_interface_id(c, f);
	_dbgmsg( "usb_interface_id() = %d\n", id );
	if( id < 0 )
		return id;

	dev->pg[0].ctrl_id = id;
	id = 0;     /* fixed interface number */
	pg1_intf_desc_comm.bInterfaceNumber = id;
/* DEL-S */
/*	pg1_iad_desc.bFirstInterface = id;*/
/* DEL-E */
	pg1_union_desc.bMasterInterface0 = id;

	/* Allocate Interface ID: PipeGroup1 bulk interface */
	id = usb_interface_id(c, f);
	_dbgmsg( "usb_interface_id() = %d(%d)\n", id, pg1_intf_desc_comm.bInterfaceNumber );
	if( id < 0 )
		return id;

	dev->pg[0].data_id = id;
	id = 1;     /* fixed interface number */
	pg1_intf_desc_bulk.bInterfaceNumber = id;
	pg1_union_desc.bSlaveInterface0 = id;
	pg1_call_mgmt.bDataInterface = id;
	_dbgmsg( "usb_interface_id() = %d(%d)\n", id, pg1_intf_desc_bulk.bInterfaceNumber );

	_dbgmsg( "OUT\n" );
	return 0;
}

static int omf_allocate_endpoints(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
// <iDPower_002> mod S
//	struct omf_dev *dev = func_to_dev(f);
	struct omf_dev *dev = func_to_omf(f);
// <iDPower_002> mod E
	struct usb_request *req;
	struct usb_ep *ep;

	/* allocate endpoints: PipeGroup1 intrrupt */
	ep = usb_ep_autoconfig(cdev->gadget, &pg1_ep_intr_desc);
	if( !ep ) {
		_dbgmsg( "usb_ep_autoconfig for PG1 ep_intr failed\n" );
		return -ENODEV;
	}
	_dbgmsg("usb_ep_autoconfig for PG1 ep_intr got %s\n", ep->name);
	ep->driver_data = dev;
	dev->pg[0].ep_intr = ep;

	/* allocate endpoints: PipeGroup1 bulk(in) */
	ep = usb_ep_autoconfig(cdev->gadget, &pg1_ep_in_desc);
	if( !ep ) {
		_dbgmsg( "usb_ep_autoconfig for PG1 ep_in failed\n" );
		return -ENODEV;
	}
	_dbgmsg("usb_ep_autoconfig for PG1 ep_in got %s\n", ep->name);
	ep->driver_data = dev;
	dev->pg[0].ep_in = ep;

	/* allocate endpoints: PipeGroup1 bulk(out) */
	ep = usb_ep_autoconfig(cdev->gadget, &pg1_ep_out_desc);
	if( !ep ) {
		_dbgmsg( "usb_ep_autoconfig for PG1 ep_out failed\n" );
		return -ENODEV;
	}
	_dbgmsg("usb_ep_autoconfig for PG1 ep_out got %s\n", ep->name);
	ep->driver_data = dev;
	dev->pg[0].ep_out = ep;

	/* support high speed hardware */
	if (gadget_is_dualspeed(cdev->gadget)) {
		pg1_ep_intr_desc_hs.bEndpointAddress = pg1_ep_intr_desc.bEndpointAddress;
		pg1_ep_in_desc_hs.bEndpointAddress = pg1_ep_in_desc.bEndpointAddress;
		pg1_ep_out_desc_hs.bEndpointAddress = pg1_ep_out_desc.bEndpointAddress;
	}

	_dbgmsg("%s speed %s: PG1[INTR/%s, IN/%s, OUT/%s]\n",
		gadget_is_dualspeed(cdev->gadget) ? "dual" : "full",
		f->name,
		dev->pg[0].ep_intr->name, dev->pg[0].ep_in->name, dev->pg[0].ep_out->name);

	/* allocate request for endpoints */
	req = omf_request_new( dev->pg[0].ep_intr, 16 );
	if(!req) {
		_dbgmsg( "create request fail\n" );
		return -ENODEV;
	}
	req->complete = omf_complete_intr;
	dev->pg[0].req_intr = req;

	req = omf_request_new( dev->pg[0].ep_in, 512 );
	if(!req) {
		_dbgmsg( "create request fail\n" );
		return -ENODEV;
	}
	req->complete = omf_complete_in;
	dev->pg[0].req_in = req;

	req = omf_request_new( dev->pg[0].ep_out, 512 );
	if(!req) {
		_dbgmsg( "create request fail\n" );
		return -ENODEV;
	}
	req->complete = omf_complete_out;
	dev->pg[0].req_out = req;

	_dbgmsg( "OUT\n" );
	return 0;
}

static int omf_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
// <iDPower_002> mod S
//	struct omf_dev *dev = func_to_dev(f);
	struct omf_dev *dev = func_to_omf(f);
// <iDPower_002> mod E
	int ret;

	_dbgmsg( "IN\n" );
	dev->cdev = cdev;
    
	/* allocate Interface IDs */
	ret = omf_allocate_interface_ids( c, f );
	if( ret < 0 ) {
		printk( KERN_ERR "allocate interface IDs error\n" );
		return ret;
	}

	/* allocate endpoints */
	ret = omf_allocate_endpoints( c, f );
	if( ret < 0 ) {
		printk( KERN_ERR "allocate endpoints error\n" );
		return ret;
	}

	_dbgmsg( "OUT\n" );
	return 0;
}

static void omf_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
// <iDPower_002> mod S
//	struct omf_dev *dev = func_to_dev(f);
	struct omf_dev *dev = func_to_omf(f);
// <iDPower_002> mod E

	_dbgmsg( "IN\n" );

	spin_lock_irq(&dev->lock);
	omf_request_free( dev->pg[0].req_intr, dev->pg[0].ep_intr );
	omf_request_free( dev->pg[0].req_in, dev->pg[0].ep_in );
	omf_request_free( dev->pg[0].req_out, dev->pg[0].ep_out );
	spin_unlock_irq(&dev->lock);

// <iDPower_002> del S
//	kfree(_omf_dev);
//	_omf_dev = NULL;
// <iDPower_002> del E

	_dbgmsg( "OUT\n" );
}

static int omf_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
// <iDPower_002> del S
//	struct omf_dev *dev = func_to_dev(f);
// <iDPower_002> del E
	int value = -EOPNOTSUPP;
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	_dbgmsg( "IN\n" );

// <iDPower_002> del S
//	/* do nothing if we are disabled */
//	if (dev->function.disabled) {
//		_dbgmsg( "disabled\n" );
//		return value;
//	}
// <iDPower_002> del E

	if( omf_iff.if_setup != NULL ) {
		value = (omf_iff.if_setup)( f, ctrl );
	}

	_dbgmsg("omf_function_setup "
		"%02x.%02x v%04x i%04x l%u\n",
		ctrl->bRequestType, ctrl->bRequest,
		w_value, w_index, w_length);

	_dbgmsg( "OUT\n" );

	return value;
}

/* ADD-S */
/**
 * ep_choose - select descriptor endpoint at current device speed
 * @g: gadget, connected and running at some speed
 * @hs: descriptor to use for high speed operation
 * @fs: descriptor to use for full or low speed operation
 */
static inline struct usb_endpoint_descriptor *
ep_choose(struct usb_gadget *g, struct usb_endpoint_descriptor *hs,
		struct usb_endpoint_descriptor *fs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}
/* ADD-S */

static int omf_function_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
// <iDPower_002> mod S
//	struct omf_dev *dev = func_to_dev(f);
	struct omf_dev *dev = func_to_omf(f);
// <iDPower_002> mod S
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	_dbgmsg("omf_function_set_alt intf: %d alt: %d\n", intf, alt);

/* DEL-S */
/*	if( omf_iff.if_set_alt != NULL ) {*/
/*		ret  = (omf_iff.if_set_alt)( f, intf, alt );*/
/*	}*/
/* DEL-S */

	if( dev->pg[0].ctrl_id == intf ) {
/* MOD-S */
/*		ret = usb_ep_enable( dev->pg[0].ep_intr, */
/*	     		ep_choose( cdev->gadget, &pg1_ep_intr_desc_hs, &pg1_ep_intr_desc ) ); */
/* MOD-S */
		dev->pg[0].ep_intr->desc = ep_choose( cdev->gadget, &pg1_ep_intr_desc_hs, &pg1_ep_intr_desc );
		ret = usb_ep_enable( dev->pg[0].ep_intr );
		if( ret ) {
			_dbgmsg( "usb_ep_enable error pg1 ep_intr ret = %d\n", ret );
			return ret;
		}
        dev->ctrl_ep_enable = 1;
	} else if( dev->pg[0].data_id == intf ) {
/* MOD-S */
/*		ret = usb_ep_enable( dev->pg[0].ep_in, */
/*				     ep_choose( cdev->gadget, &pg1_ep_in_desc_hs, &pg1_ep_in_desc ) ); */
/* MOD-S */
		dev->pg[0].ep_in->desc = ep_choose( cdev->gadget, &pg1_ep_in_desc_hs, &pg1_ep_in_desc );
		ret = usb_ep_enable( dev->pg[0].ep_in );
		if( ret ) {
			_dbgmsg( "usb_ep_enable error pg1 ep_in ret = %d\n", ret );
			return ret;
		}
/* MOD-S */
/*		ret = usb_ep_enable( dev->pg[0].ep_out, */
/*				     ep_choose( cdev->gadget, &pg1_ep_out_desc_hs, &pg1_ep_out_desc ) ); */
		dev->pg[0].ep_out->desc = ep_choose( cdev->gadget, &pg1_ep_out_desc_hs, &pg1_ep_out_desc );
		ret = usb_ep_enable( dev->pg[0].ep_out );
		if( ret ) {
			_dbgmsg( "usb_ep_enable error pg1 ep_out ret = %d\n", ret );
			usb_ep_disable(dev->pg[0].ep_in);
			return ret;
		}
        dev->data_ep_enable = 1;

	} else {
		_dbgmsg( "unknown interface number\n" );
	}

/* ADD-S */
	if( omf_iff.if_set_alt != NULL ) {
		ret  = (omf_iff.if_set_alt)( f, intf, alt );
	}
/* ADD-E */

	return 0;
}

static void omf_function_disable(struct usb_function *f)
{
// <iDPower_002> mod S
//	struct omf_dev *dev = func_to_dev(f);
	struct omf_dev *dev = func_to_omf(f);
// <iDPower_002> mod E

	_dbgmsg( "IN\n" );

	if( omf_iff.if_disable != NULL ) {
		(omf_iff.if_disable)( f );
	}

    if( dev->ctrl_ep_enable == 1 )
    {
		_dbgmsg( "usb_ep_disable %s\n",dev->pg[0].ep_intr->name );
		usb_ep_disable( dev->pg[0].ep_intr );
        dev->ctrl_ep_enable = 0;
    }

    if( dev->data_ep_enable == 1 )
    {
		_dbgmsg( "usb_ep_disable %s\n",dev->pg[0].ep_intr->name );
		usb_ep_disable( dev->pg[0].ep_in );
		_dbgmsg( "usb_ep_disable %s\n",dev->pg[0].ep_out->name );
		usb_ep_disable( dev->pg[0].ep_out );
        dev->data_ep_enable = 0;
    }

	_dbgmsg( "OUT\n" );
}

static void omf_function_suspend(struct usb_function *f)
{
	_dbgmsg( "IN\n" );
	if( omf_iff.if_suspend != NULL ) {
		(omf_iff.if_suspend)( f );
	}
}

static void omf_function_resume(struct usb_function *f)
{
	_dbgmsg( "IN\n" );
	if( omf_iff.if_resume != NULL ) {
		(omf_iff.if_resume)( f );
	}
}

static int omf_bind_config(struct usb_configuration *c)
{
// <iDPower_002> mod S
//	struct omf_dev  *dev;
	struct omf_dev *dev = _omf_dev;
// <iDPower_002> mod E
	int ret;

	_dbgmsg( "IN\n" );

// <iDPower_002> del S
//	dev = kzalloc( sizeof( *dev ), GFP_KERNEL );
// <iDPower_002> del E
	if( !dev ) {
		printk( KERN_ERR "omf gadget driver failed to initialize nomem\n" );
		return -ENOMEM;
	}

	spin_lock_init(&dev->lock);

	dev->cdev = c->cdev;
	dev->function.name = "omf";
	dev->function.descriptors = fs_omf_descs;
	dev->function.hs_descriptors = hs_omf_descs;
	dev->function.bind = omf_function_bind;
	dev->function.unbind = omf_function_unbind;
	dev->function.setup = omf_function_setup;
	dev->function.set_alt = omf_function_set_alt;
	dev->function.disable = omf_function_disable;
	dev->function.suspend = omf_function_suspend;
	dev->function.resume = omf_function_resume;

// <iDPower_002> del S
//	/* start disabled */
//	dev->function.disabled = 1;
// <iDPower_002> del E

	dev->ctrl_ep_enable = 0;
	dev->data_ep_enable = 0;

// <iDPower_002> del S
//	_omf_dev = dev;
// <iDPower_002> del E

	ret = usb_add_function(c, &dev->function);
	if( ret != 0 ) {
		printk( KERN_ERR "omf gadget driver failed to initialize2 ret = %d\n", ret );
// <iDPower_002> del S
//		kfree( dev );
// <iDPower_002> del E
		return ret;
	}

	_dbgmsg( "OUT\n" );

	return 0;
}

// <iDPower_002> mod S
//static struct android_usb_function omf_function = {
//	.name = "omf",
//	.bind_config = omf_bind_config,
//};
//
//
//static int __init omf_init( void )
//{
//	_dbgmsg( "IN\n" );
//	memset( &omf_iff, 0, sizeof( omf_iff ) );
//	android_register_function(&omf_function);
//	_dbgmsg( "OUT\n" );
//	return 0;
//}
//
//module_init(omf_init);
static int omf_init( void )
{
	struct omf_dev  *dev;
	_dbgmsg( "IN\n" );
	memset( &omf_iff, 0, sizeof( omf_iff ) );

	dev = kzalloc( sizeof( *dev ), GFP_KERNEL );
	if( !dev ) {
		return -ENOMEM;
	}

	_omf_dev = dev;
	_dbgmsg( "OUT _omf_dev=%p\n", _omf_dev );

	return 0;
}

static void omf_cleanup( void )
{
	_dbgmsg( "IN\n" );
	if( _omf_dev != NULL ) {
		kfree(_omf_dev);
		_omf_dev = NULL;
	}
	_dbgmsg( "OUT\n" );
}
// <iDPower_002> mod E

struct omf_dev * omf_register_if( struct t_omf_iffunc *iff )
{
	_dbgmsg( "IN\n" );
	omf_iff.if_setup = iff->if_setup;
	omf_iff.if_set_alt = iff->if_set_alt;
	omf_iff.if_disable = iff->if_disable;
	omf_iff.if_suspend = iff->if_suspend;
	omf_iff.if_resume = iff->if_resume;
	omf_iff.if_complete_intr = iff->if_complete_intr;
	omf_iff.if_complete_in = iff->if_complete_in;
	omf_iff.if_complete_out = iff->if_complete_out;
	_dbgmsg( "OUT\n" );

	return _omf_dev;
}

EXPORT_SYMBOL( omf_register_if );
