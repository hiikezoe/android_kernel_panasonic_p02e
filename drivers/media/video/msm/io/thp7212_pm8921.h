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

#ifndef THP7212_PM8921_H
#define THP7212_PM8921_H

enum pm8921_redled_onoff {
        PM8921_REDLED_ON,
        PM8921_REDLED_OFF
};

int thp7212_pm8921_red_led( int onoff);

#endif
