#include "dtvd_tuner_com.h"
#include "dtvd_tuner_core.h"

/*****************************************************************************/
/*  RAMデータ定義                                                            */
/*****************************************************************************/
DTVD_TUNER_CONTROL_t            tdtvd_tuner_info;       /* チューナドライバ管理情報 */
DTVD_TUNER_MONITOR_INFO_t       tdtvd_tuner_monitor;    /* チューナ情報 */
DTVD_TUNER_COM_NONVOLATILE_t    tdtvd_tuner_nonvola;    /* 不揮発情報 */
DTVD_TUNER_CORE_INFO_t          tdtvd_tuner_core;       /* 全体制御部情報 */
struct task_struct *tdtvd_tuner_tsprcv_thread; 
DTVD_TUNER_TSP_RINGBUFFER_t     tdtvd_tuner_tsp_ringbuffer; /* TSP受信リングバッファ   */
DTVD_TUNER_TSPRCV_CTRL_t        tdtvd_tuner_tsprcv_ctrl;    /* TSP受信スレッド管理情報 */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
