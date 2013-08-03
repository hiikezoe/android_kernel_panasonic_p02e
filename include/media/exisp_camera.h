/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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
#ifndef __LINUX_EXISP_CAMERA_H
#define __LINUX_EXISP_CAMERA_H

typedef struct exisp_enable_s {
        unsigned int af:1;
        unsigned int af_cont:1;
#if 1 /* RupyAF13 */
        unsigned int flash:1;
#endif
} exisp_enable_t;

typedef struct exisp_state_s {
        unsigned int af:1;
        unsigned int af_cont:1;
        unsigned int power:1;
} exisp_state_t;

typedef struct exisp_doing_s {
        unsigned int af_cont:1;
        unsigned int still:1;
} exisp_doing_t;

#if 1 /* RupyAF10 */
typedef struct exisp_dimension_s {
        uint16_t video_width;
        uint16_t video_height;
        uint16_t picture_width;
        uint16_t picture_height;
        uint16_t display_width;
        uint16_t display_height;
} exisp_dimension_t;
#endif

#if 1 /* RupyAF11 */
typedef enum {
        EXISP_AF_MODE_AUTO,
        EXISP_AF_MODE_INFINITY,
        EXISP_AF_MODE_NORMAL,
        EXISP_AF_MODE_MACRO,
} exisp_af_mode_type_t ;
typedef enum {
        EXISP_CAF_OFF,
        EXISP_CAF_ON,
} exisp_caf_mode_type_t ;
#endif

#if 1 /* RupyAF12 */
#define EXISP_MAX_ROI 5

typedef struct {
    int x1, y1, x2, y2;
    int weight;
} exisp_area_t; /* base camera_area_t */

typedef struct {
        exisp_area_t mtr_area[EXISP_MAX_ROI];
        uint32_t     num_area;
        int          previewWidth;
        int          previewHeight;
} exisp_af_mtr_area_t;
#endif

#if 1 /* RupyAF13 */
typedef enum {
  LED_MODE_OFF,
  LED_MODE_AUTO,
  LED_MODE_ON,
  LED_MODE_TORCH,

  /*new mode above should be added above this line*/
  LED_MODE_MAX
} exisp_led_mode_t;
#endif

#endif /* __LINUX_EXISP_CAMERA_H */
