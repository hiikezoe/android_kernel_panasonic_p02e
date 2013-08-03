/*
 *******************************************************************
 File Name	: tnc_defs.h

 Description	: "P2P �ȥ�ͥ�ǥХ��� API ����������"

 Remark		: 

 Copyright (c) 2004 by Panasonic Mobile Communications Co., Ltd.
 ������:	2004/02/26 [kaneko]
 ������:	2011/06/30 [Yoshino]
 *******************************************************************
 */

#ifndef _tnc_defs_h_
#define _tnc_defs_h_

#define TNC_DEV_NAME_LEN	32
/* PMC-Viana-018-vlan0��viana0�б� add start */
#ifdef BUILD_ANDROID
#include "viana_common_setting.h"
#define	TNC_VIF_NAME		VIANA_NET_VLAN_IFNAME
#else
#define	TNC_VIF_NAME		"vlan0"
#endif /* BUILD_ANDROID */
/* PMC-Viana-018-vlan0��viana0�б� add end */

/* SA ���������°�� */
#define TNC_DIR_OUTBOUND	1
#define TNC_DIR_INBOUND		2

#endif /* _tnc_defs_h_ */
