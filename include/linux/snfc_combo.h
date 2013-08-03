#ifndef ___SNFC_COMBO_H___
#define ___SNFC_COMBO_H___

/******************************************************************************/
/*  外部関数定義 ( extern )                                                   */
/******************************************************************************/
extern int Uartdrv_uartcc_state_read_dd(unsigned int *uartcc_state);
extern int Uartdrv_pollstart_set_uartcc_state(unsigned int polling_data);
extern int Uartdrv_pollend_set_uartcc_state(void);
extern int SnfcCtrl_terminal_hsel_read_dd(unsigned int *hsel_data);

/******************************************************************************/
/*  外部変数定義 ( extern )                                                   */
/******************************************************************************/
/*
 * UART Collision Control state
 */
enum UARTCC_STATE {
    SNFC_STATE_WAITING_POLL_END = 1,            /* FeliCa:ready,  NFC:Used   */
    SNFC_STATE_NFC_POLLING,                     /* FeliCa:NoUsed, NFC:Used   */
    SNFC_STATE_FELICA_ACTIVE,                   /* FeliCa:Used,   NFC:NoUsed */
    SNFC_STATE_IDLE                             /* FeliCa:NoUsed, NFC:NoUsed */
};

#endif /* ___SNFC_COMBO_H___ */
