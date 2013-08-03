#include <linux/usb.h>
#include <linux/usb/android_composite.h>

#ifndef __F_OMF_H__
#define __F_OMF_H__

struct t_omf_iffunc {
    int (*if_setup)(struct usb_function *, const struct usb_ctrlrequest *);
    int (*if_set_alt)(struct usb_function *, unsigned , unsigned );
    void (*if_disable)(struct usb_function *);
    void (*if_suspend)(struct usb_function *);
    void (*if_resume)(struct usb_function *);
    void (*if_complete_in)(struct usb_ep *, struct usb_request *);
    void (*if_complete_out)(struct usb_ep *, struct usb_request *);
    void (*if_complete_intr)(struct usb_ep *, struct usb_request *);
};

struct usb_model_specific_func_desc {
	__u8    bLength;
	__u8    bDescriptorType;
	__u8    bDescriptorSubType;
	__u8    bType;
	__u8    bMode[3];
	int     mode_num;
} __attribute__ ((packed));;

struct omf_pg {
    unsigned ctrl_id;
    unsigned data_id;
    int      ctrl_set;
    int      data_set;

    struct usb_ep *ep_intr;
    struct usb_ep *ep_in;
    struct usb_ep *ep_out;

    struct usb_request *req_intr;
    struct usb_request *req_in;
    struct usb_request *req_out;

    int rx_done;
    int tx_done;
    int intr_done;
};

struct omf_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;
    struct omf_pg pg[1];
    int    ctrl_ep_enable;
    int    data_ep_enable;
};

extern struct omf_dev * omf_register_if( struct t_omf_iffunc *iff );

#endif /* __F_OMF_H__ */
