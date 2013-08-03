#ifndef _DTVD_TUNER_HW_H_
#define _DTVD_TUNER_HW_H_

#ifdef TUNER_MCBSP_ENABLE
#include <linux/gpio_ex.h>
#endif /* TUNER_MCBSP_ENABLE */

/*****************************************************************************/
/* 定数定義                                                                  */
/*****************************************************************************/
/*---------------------------------------------------------------------------*/
/* 共通アクセスIF                                                            */
/*---------------------------------------------------------------------------*/
#define D_DTVD_TUNER_DEV_TUNER      0x10         /* DTV1チューナ     */
#define D_DTVD_TUNER_DEV_I2C           7         /* DTV1チューナ I2C */
#ifdef TUNER_MCBSP_ENABLE
#define D_DTVD_TUNER_MCBSP          OMAP_MCBSP4  /* Mod：(BugLister ID86) 2011/11/01 */
#endif /* TUNER_MCBSP_ENABLE */

/*---------------------------------------------------------------------------*/
/* 割り込み制御                                                              */
/*---------------------------------------------------------------------------*/
#define D_DTVD_TUNER_INT_PORT   44		/* チューナ割り込みのポート番号GPIO44 */
#define D_DTVD_TUNER_INT_MASK_CLEAR (0)             /* 登録時、本割り込みをクリア（許可） */
#define D_DTVD_TUNER_INT_MASK_SET   (1)             /* 登録時、本割り込みをマスク（禁止） */
#define D_DTVD_TUNER_MCBSPCFG_RS_ENABLE   (0)        /* MCBSP設定を204byte受信に設定 *//* Add：(BugLister ID86) 2011/11/07 */
#define D_DTVD_TUNER_MCBSPCFG_RS_DISABLE   (1)        /* MCBSP設定を188byte受信に設定 *//* Add：(BugLister ID86) 2011/11/07 */
#define D_DTVD_TUNER_MCBSPCFG_RS_ENABLE_RESET_DISABLE   (2) /* MCBSP設定204byte受信に設定 & 同期ずれリセット無効 *//* Add：(MMシステム対応) 2012/03/14 */

/*---------------------------------------------------------------------------*/
/* NORドライバ                                                               */
/*---------------------------------------------------------------------------*/
/* #define D_DTVD_TUNER_INFO_ID        D_HCM_D_DTV_TUNERS *//* チューナ不揮発情報ID*//* Arrietty 不揮発0.1版 対応 2011/11/29 */
/* #define D_DTVD_TUNER_INFO_NO        0 */               /* チューナ不揮発情報ID内番号 *//* Arrietty 不要の為、削除 2011/11/29 */

/*---------------------------------------------------------------------------*/
/* チューナOFDMアドレス                                                      */
/*---------------------------------------------------------------------------*/
#define D_DTVD_TUNER_REG_VERSION                    0x00 /* VERSION         */
#define D_DTVD_TUNER_REG_AGCCTRL                    0x01 /* AGCCTRL         */
#define D_DTVD_TUNER_REG_AGCLEVEL                   0x03 /* AGCLEVEL        */
#define D_DTVD_TUNER_REG_AGC_SUBA                   0x04 /* AGC_SUBA        */
#define D_DTVD_TUNER_REG_AGC_SUBD                   0x05 /* AGC_SUBD        */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFA_HI            0x00 /* IFA_HI          */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFA_LO            0x01 /* IFA_LO          */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFB_HI            0x02 /* IFB_HI          */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFB_LO            0x03 /* IFB_LO          */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFA_HI            0x04 /* RFA_HI          */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFA_LO            0x05 /* RFA_LO          */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFB_HI            0x06 /* RFB_HI          */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFB_LO            0x07 /* RFB_LO          */
#define D_DTVD_TUNER_REG_AGC_SUBA_DTS               0x08 /* DTS             */
#define D_DTVD_TUNER_REG_AGC_SUBA_MINIFAGC          0x09 /* MINIFAGC        */
#define D_DTVD_TUNER_REG_AGC_SUBA_MAXIFAGC          0x0A /* MAXIFAGC        */
#define D_DTVD_TUNER_REG_AGC_SUBA_AGAIN             0x0B /* AGAIN           */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFREF_HI          0x0E /* IFREF_HI        */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFREF_LO          0x0F /* IFREF_LO        */
#define D_DTVD_TUNER_REG_AGC_SUBA_MINRFAGC          0x12 /* MINRFAGC        */
#define D_DTVD_TUNER_REG_AGC_SUBA_MAXRFAGC          0x13 /* MAXRFAGC        */
#define D_DTVD_TUNER_REG_AGC_SUBA_BGAIN             0x14 /* BGAIN           */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFREF_HI          0x15 /* RFREF_HI        */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFREF_LO          0x16 /* RFREF_LO        */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFSAMPLE          0x29 /* IFSAMPLE        */
#define D_DTVD_TUNER_REG_AGC_SUBA_POWAGC            0x2D /* POWAGC          */
#define D_DTVD_TUNER_REG_AGC_SUBA_IFAGCDAC          0x3A /* IFAGCDAC        */
#define D_DTVD_TUNER_REG_AGC_SUBA_RFAGCDAC          0x3B /* RFAGCDAC        */
#define D_DTVD_TUNER_REG_MODE_CTRL                  0x06 /* MODE_CTRL       */
#define D_DTVD_TUNER_REG_MODE_STAT                  0x07 /* MODE_STAT       */
#define D_DTVD_TUNER_REG_STATE_INIT                 0x08 /* STATE_INIT      */
#define D_DTVD_TUNER_REG_IQINV                      0x09 /* IQINV           */
#define D_DTVD_TUNER_REG_SYNC_STATE                 0x0A /* SYNC_STATE      */
#define D_DTVD_TUNER_REG_AFC_M2A                    0x17 /* AFC_M2A         */
#define D_DTVD_TUNER_REG_AFC_M2B                    0x18 /* AFC_M2B         */
#define D_DTVD_TUNER_REG_AFC_M3A                    0x1A /* AFC_M3A         */
#define D_DTVD_TUNER_REG_AFC_M3B                    0x1B /* AFC_M3B         */
#define D_DTVD_TUNER_REG_FILTER                     0x1C /* FILTER          */
#define D_DTVD_TUNER_REG_AFC_CLKA                   0x20 /* AFC_CLKA        */
#define D_DTVD_TUNER_REG_AFC_CLKB                   0x21 /* AFC_CLKB        */
#define D_DTVD_TUNER_REG_AFC_CLK2A                  0x26 /* AFC_CLK2A       */
#define D_DTVD_TUNER_REG_AFC_CLK2B                  0x27 /* AFC_CLK2B       */
#define D_DTVD_TUNER_REG_SYNC_SUBA                  0x28 /* SYNC_SUBA       */
#define D_DTVD_TUNER_REG_SYNC_SUBD1                 0x29 /* SYNC_SUBD1      */
#define D_DTVD_TUNER_REG_SYNC_SUBD2                 0x2A /* SYNC_SUBD2      */
#define D_DTVD_TUNER_REG_SYNC_SUBD3                 0x2B /* SYNC_SUBD3      */
#define D_DTVD_TUNER_REG_SYNC_SUBA_IFGAIN           0x00 /* IFGAIN          */
#define D_DTVD_TUNER_REG_SYNC_SUBA_CLKOFF           0x2A /* CLKOFF          */
#define D_DTVD_TUNER_REG_SYNC_SUBA_CROFF            0x2D /* CROFF           */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_M2G4        0x43 /* MODE_M2G4       */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_M2G8        0x44 /* MODE_M2G8       */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_M3G4        0x47 /* MODE_M3G4       */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_M3G8        0x48 /* MODE_M3G8       */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_M3G16       0x49 /* MODE_M3G16      */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_THRES       0x4B /* MODE_THRES      */
#define D_DTVD_TUNER_REG_SYNC_SUBA_MODE_TIMER       0x4C /* MODE_TIMER      */
#define D_DTVD_TUNER_REG_SYNC_SUBA_S4_TIMER         0x54 /* S4_TIMER        */
#define D_DTVD_TUNER_REG_SYNC_SUBA_S1_TULD          0x6B /* S1_TULD         */
#define D_DTVD_TUNER_REG_SYNC_SUBA_S1_TIMER         0x6C /* S1_TIMER        */
#define D_DTVD_TUNER_REG_SYNC_SUBA_S3_TIMER         0x6D /* S3_TIMER        */
#define D_DTVD_TUNER_REG_WIN_CTRL                   0x3A /* WIN_CTRL        */
#define D_DTVD_TUNER_REG_WIN_SUBA                   0x3B /* WIN_SUBA        */
#define D_DTVD_TUNER_REG_WIN_SUBD                   0x3C /* WIN_SUBD        */
#define D_DTVD_TUNER_REG_WIN_SUBA_WIN_PEAKTH1       0x05 /* WIN_PEAKTH1     */
#define D_DTVD_TUNER_REG_WIN_SUBA_WIN_PEAKTH2       0x06 /* WIN_PEAKTH2     */
#define D_DTVD_TUNER_REG_WIN_SUBA_WIN_TIMER         0x07 /* WIN_TIMER       */
#define D_DTVD_TUNER_REG_DETECT_SUBA                0x40 /* DETECT_SUBA     */
#define D_DTVD_TUNER_REG_DETECT_SUBD                0x41 /* DETECT_SUBD     */
#define D_DTVD_TUNER_REG_DETECT_SUBA_SEARCH_TIMER   0x0D /* SEARCH_TIMER    */
#define D_DTVD_TUNER_REG_DETECT_SUBA_SEARCH_THRES   0x35 /* SEARCH_THRES    */
#define D_DTVD_TUNER_REG_DETECT_SUBA_LOCK_THRES     0x58 /* LOCK_THRES      */
#define D_DTVD_TUNER_REG_DETECT_SUBA_LOSS_THRES     0x59 /* LOSS_THRES      */
#define D_DTVD_TUNER_REG_DETECT_SUBA_LOCK_FRM       0x5A /* LOCK_FRM        */
#define D_DTVD_TUNER_REG_DETECT_SUBA_LOSS_FRM       0x5B /* LOSS_FRM        */
#define D_DTVD_TUNER_REG_SEGCNT                     0x44 /* SEGCNT          */
#define D_DTVD_TUNER_REG_CNCNT                      0x45 /* CNCNT           */
#define D_DTVD_TUNER_REG_CNDAT0                     0x46 /* CNDAT0          */
#define D_DTVD_TUNER_REG_CNDAT1                     0x47 /* CNDAT1          */
#define D_DTVD_TUNER_REG_CNMODE                     0x48 /* CNMODE          */
#define D_DTVD_TUNER_REG_FEC_SUBA                   0x50 /* FEC_SUBA        */
#define D_DTVD_TUNER_REG_FEC_SUBD                   0x51 /* FEC_SUBD        */
#define D_DTVD_TUNER_REG_FEC_SUBA_MERCTRL           0x50 /* MERCTRL         */
#define D_DTVD_TUNER_REG_FEC_SUBA_MERSTEP           0x51 /* MERSTEP         */
#define D_DTVD_TUNER_REG_FEC_SUBA_MERDT0            0x52 /* MERDT0          */
#define D_DTVD_TUNER_REG_FEC_SUBA_MERDT1            0x53 /* MERDT1          */
#define D_DTVD_TUNER_REG_FEC_SUBA_MERDT2            0x54 /* MERDT2          */
#define D_DTVD_TUNER_REG_FEC_SUBA_MEREND            0x58 /* MEREND          */
#define D_DTVD_TUNER_REG_FEC_SUBA_VBERSET0          0xA4 /* VBERSET0        */
#define D_DTVD_TUNER_REG_FEC_SUBA_VBERSET1          0xA5 /* VBERSET1        */
#define D_DTVD_TUNER_REG_FEC_SUBA_VBERSET2          0xA6 /* VBERSET2        */
#define D_DTVD_TUNER_REG_FEC_SUBA_PEREN             0xB0 /* PEREN           */
#define D_DTVD_TUNER_REG_FEC_SUBA_PERRST            0xB1 /* PERRST          */
#define D_DTVD_TUNER_REG_FEC_SUBA_PERSNUM0          0xB2 /* PERSNUM0        */
#define D_DTVD_TUNER_REG_FEC_SUBA_PERSNUM1          0xB3 /* PERSNUM1        */
#define D_DTVD_TUNER_REG_FEC_SUBA_PERFLG            0xB6 /* PERFLG          */
#define D_DTVD_TUNER_REG_FEC_SUBA_PERERR0           0xB7 /* PERERR0         */
#define D_DTVD_TUNER_REG_FEC_SUBA_PERERR1           0xB8 /* PERERR1         */
#define D_DTVD_TUNER_REG_FEC_SUBA_PCLKSEL           0xD2 /* PCLKSEL         */
#define D_DTVD_TUNER_REG_FEC_SUBA_TSOUT             0xD5 /* TSOUT           */
#define D_DTVD_TUNER_REG_FEC_SUBA_PBER              0xD6 /* PBER            */
#define D_DTVD_TUNER_REG_FEC_SUBA_SBER              0xD7 /* SBER            */
#define D_DTVD_TUNER_REG_FEC_SUBA_SBER_IRQ_MASK     0xDA /* SBER_IRQ_MASK   */
#define D_DTVD_TUNER_REG_FEC_SUBA_TSERR_IRQ_MASK    0xDB /* TSERR_IRQ_MASK  */
#define D_DTVD_TUNER_REG_FEC_SUBA_SBERSET0          0xDC /* SBERSET0        */
#define D_DTVD_TUNER_REG_FEC_SUBA_SBERSET1          0xDD /* SBERSET1        */
#define D_DTVD_TUNER_REG_VBERON                     0x52 /* VBERON          */
#define D_DTVD_TUNER_REG_VBERXRST                   0x53 /* VBERXRST        */
#define D_DTVD_TUNER_REG_VBERFLG                    0x54 /* VBERFLG         */
#define D_DTVD_TUNER_REG_VBERDT0                    0x55 /* VBERDT0         */
#define D_DTVD_TUNER_REG_VBERDT1                    0x56 /* VBERDT1         */
#define D_DTVD_TUNER_REG_VBERDT2                    0x57 /* VBERDT2         */
#define D_DTVD_TUNER_REG_RSBERON                    0x5E /* RSBERON         */
#define D_DTVD_TUNER_REG_RSBERXRST                  0x5F /* RSBERXRST       */
#define D_DTVD_TUNER_REG_RSBERCEFLG                 0x60 /* RSBERCEFLG      */
#define D_DTVD_TUNER_REG_TSERR_IRQ_RST              0x62 /* TSERR_IRQ_RST   */
#define D_DTVD_TUNER_REG_RSBERDT0                   0x64 /* RSBERDT0        */
#define D_DTVD_TUNER_REG_RSBERDT1                   0x65 /* RSBERDT1        */
#define D_DTVD_TUNER_REG_RSBERDT2                   0x66 /* RSBERDT2        */
#define D_DTVD_TUNER_REG_TMCC_IRQ                   0x6C /* TMCC_IRQ        */
#define D_DTVD_TUNER_REG_TMCC_SUBA                  0x6D /* TMCC_SUBA       */
#define D_DTVD_TUNER_REG_TMCC_SUBD                  0x6E /* TMCC_SUBD       */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCCLOCK         0x14 /* TMCCLOCK        */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC_IRQ_MASK    0x35 /* TMCC_IRQ_MASK   */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC_IRQ_RST     0x36 /* TMCC_IRQ_RST    */
#define D_DTVD_TUNER_REG_TMCC_SUBA_EMG_INV          0x38 /* EMG_INV         */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC0            0x80 /* TMCC0           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC1            0x81 /* TMCC1           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC2            0x82 /* TMCC2           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC3            0x83 /* TMCC3           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC4            0x84 /* TMCC4           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC5            0x85 /* TMCC5           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC6            0x86 /* TMCC6           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC7            0x87 /* TMCC7           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC8            0x88 /* TMCC8           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC9            0x89 /* TMCC9           */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC10           0x8A /* TMCC10          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC11           0x8B /* TMCC11          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC12           0x8C /* TMCC12          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC13           0x8D /* TMCC13          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC14           0x8E /* TMCC14          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC15           0x8F /* TMCC15          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC16           0x90 /* TMCC16          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC17           0x91 /* TMCC17          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC18           0x92 /* TMCC18          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC19           0x93 /* TMCC19          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC20           0x94 /* TMCC20          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC21           0x95 /* TMCC21          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC22           0x96 /* TMCC22          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC23           0x97 /* TMCC23          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC24           0x98 /* TMCC24          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC25           0x99 /* TMCC25          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC26           0x9A /* TMCC26          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC27           0x9B /* TMCC27          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC28           0x9C /* TMCC28          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC29           0x9D /* TMCC29          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC30           0x9E /* TMCC30          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_TMCC31           0x9F /* TMCC31          */
#define D_DTVD_TUNER_REG_TMCC_SUBA_FEC_IN           0xA0 /* FEC_IN          */
#define D_DTVD_TUNER_REG_SBER_IRQ                   0x6F /* SBER_IRQ        */
#define D_DTVD_TUNER_REG_RESET                      0x70 /* RESET           */
#define D_DTVD_TUNER_REG_STANDBY                    0x73 /* STANDBY         */
#define D_DTVD_TUNER_REG_ANALOG_PWDN                0xD0 /* ANALOG_PWDN     */
#define D_DTVD_TUNER_REG_TUNERSEL                   0xD3 /* TUNERSEL        */
#define D_DTVD_TUNER_REG_DACCTRL                    0xD4 /* DACCTRL         */
#define D_DTVD_TUNER_REG_SEARCH_CTRL                0xE6 /* SEARCH_CTRL     */
#define D_DTVD_TUNER_REG_SEARCH_CHANNEL             0xE7 /* SEARCH_CHANNEL  */
#define D_DTVD_TUNER_REG_SEARCH_SUBA                0xE8 /* SEARCH_SUBA     */
#define D_DTVD_TUNER_REG_SEARCH_SUBD                0xE9 /* SEARCH_SUBD     */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL1       0x01 /* CHANNEL1        */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL2       0x02 /* CHANNEL2        */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL3       0x03 /* CHANNEL3        */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL4       0x04 /* CHANNEL4        */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL5       0x05 /* CHANNEL5        */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL6       0x06 /* CHANNEL6        */
#define D_DTVD_TUNER_REG_SEARCH_SUBA_CHANNEL7       0x07 /* CHANNEL7        */
#define D_DTVD_TUNER_REG_AOUT                       0xEA /* AOUT            */
#define D_DTVD_TUNER_REG_GPIO_DAT                   0xEB /* GPIO_DAT        */
#define D_DTVD_TUNER_REG_GPIO_OUTSEL                0xEC /* GPIO_OUTSEL     */
#define D_DTVD_TUNER_REG_SEARCH_IRQ                 0xED /* SEARCH_IRQ      */
#define D_DTVD_TUNER_REG_SEARCH_IRQCTL              0xEE /* SEARCH_IRQCTL   */
#define D_DTVD_TUNER_REG_SEARCH_MODE                0xF0 /* SEARCH_MODE     */
#define D_DTVD_TUNER_REG_SEARCH_MDCNT               0xF1 /* SEARCH_MDCNT    */
#define D_DTVD_TUNER_REG_SEARCH_CNT                 0xF2 /* SEARCH_CNT      */
#define D_DTVD_TUNER_REG_SEARCH_START               0xF3 /* SEARCH_START    */
#define D_DTVD_TUNER_REG_SEARCH_END                 0xF4 /* SEARCH_END      */
#define D_DTVD_TUNER_REG_STB_ENABLE                 0xF8 /* STB_ENABLE      */
#define D_DTVD_TUNER_REG_STB_MODE                   0xF9 /* STB_MODE        */
#define D_DTVD_TUNER_REG_STB_WAIT                   0xFA /* STB_WAIT        */
#define D_DTVD_TUNER_REG_STB_TUWAIT                 0xFB /* STB_TUWAIT      */
#define D_DTVD_TUNER_REG_STB_EMGWAIT                0xFC /* STB_EMGWAIT     */
#define D_DTVD_TUNER_REG_TUTHREW                    0xFE /* TUTHREW         */

/*---------------------------------------------------------------------------*/
/* define定義                                                                */
/*---------------------------------------------------------------------------*/
#define D_DTVD_TUNER_DATALEN_MAX                    256

#endif /* _DTVD_TUNER_HW_H_ */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
