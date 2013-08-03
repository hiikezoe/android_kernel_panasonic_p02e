/*
 * @file felica_ctrl.h
 * @brief Header file of FeliCa Control driver
 *
 * @date 2010/12/17
 */

#ifndef __DV_FELICA_CTRL_H__
#define __DV_FELICA_CTRL_H__

/*
 * The FeliCa terminal condition to acquire and to set.
 */
#define FELICA_OUT_L         (0x00) /* condition L */
#define FELICA_OUT_H         (0x01) /* condition H */
#define FELICA_OUT_H_NOWAIT  (0x02) /* H with nowait option (only applicable on PON) *//* npdc300054413 add */
#define FELICA_OUT_RFS_L     (0x01) /* RFS condition L */
#define FELICA_OUT_RFS_H     (0x00) /* RFS condition H */

#define FELICA_OUT_SIZE      (1)    /* size of to acquire and to set */
// <Combo_chip> add S
#define FELICA_OUT_SNFC_ENDPRC (2)  /* condition SNFC endProc */
// <Combo_chip> add E

/*
 * The Felica interrupt condition to acquire.
 */
#define FELICA_EDGE_L        (0x00) /* changed condition from H to L */
#define FELICA_EDGE_H        (0x01) /* changed condition from L to H */
#define FELICA_EDGE_NON      (0xFF) /* non changed */

#define FELICA_EDGE_OUT_SIZE (2)    /* size of to acquire and to set for EDGE */

#endif /* __DV_FELICA_CTRL_H__ */
