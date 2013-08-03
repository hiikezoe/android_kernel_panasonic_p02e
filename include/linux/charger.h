/* 
 * Battery Charger driver
 * Copyright(C) 2010 Panasonic Mobile Communications Co., Ltd.
 * Written by H.K.
 */

#ifndef _CHARGER_H
#define _CHARGER_H

/*<npdc300021082> extern int pm8921_set_battery_capacity(int capacity); */
extern int pm8921_set_battery_capacity(int soc, int capacity); /*<npdc300021082>*/

#endif /* _CHARGER_H */
