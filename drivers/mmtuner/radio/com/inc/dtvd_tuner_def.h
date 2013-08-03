#ifndef _DTVD_TUNER_DEF_H_
#define _DTVD_TUNER_DEF_H_

/*---------------------------------------------------------------------------*/
/* 共通定義                                                                  */
/*---------------------------------------------------------------------------*/
/* ブール値 */
#define D_DTVD_TUNER_TRUE     1                     /* 真   */
#define D_DTVD_TUNER_FALSE    0                     /* 偽   */

/* NULLポインタ */
#define D_DTVD_TUNER_NULL     ((void*)0)

/* レジスタ書込み回数 */
#define D_DTVD_TUNER_REG_NO1  1

/* チューナ使用設定 */
#define D_DTVD_TUNER_USER_DTV 0x00
#define D_DTVD_TUNER_USER_MM  0x01

/* TSIFタイムアウト設定 */
#define D_DTVD_TUNER_TIMEOUT_DISABLE 0

/* TSIF受信モード設定*/
#define D_DTVD_TUNER_SET_MODE_1 1
#define D_DTVD_TUNER_SET_MODE_2 2

#endif /* _DTVD_TUNER_DEF_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
