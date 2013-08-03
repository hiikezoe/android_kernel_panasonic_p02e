#ifndef _DTVD_TUNER_TUNER_H_
#define _DTVD_TUNER_TUNER_H_

#define D_DTVD_TUNER_SEARCH_CHECK_MASK   0x10   /* サーチ結果調査MASK(13ch地:bit4) */
extern signed int dtvd_tuner_tuner_com_search_result_read( unsigned int *result );
extern signed int dtvd_tuner_tuner_com_search_resreg_set( void );

#endif /* _DTVD_TUNER_TUNER_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
