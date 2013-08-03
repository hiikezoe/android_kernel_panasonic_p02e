/* 不揮発未対応時に初期値として定義するの */

#ifndef	__RADIO_MB86A35_NVTMP_H__
#define	__RADIO_MB86A35_NVTMP_H__

#define D_UNVOL_DAT2_MER_WAIT        0x0000                       /* Stable time for Mer */
#define D_UNVOL_DAT2_MER_CYCLE       0x0A                         /* Wait time for Mer */
#define D_UNVOL_DAT2_BER_CYCLE2      0x05                         /* Ber Cycle */
#define D_UNVOL_DAT2_RSSI_WAIT       0x00                         /* Stable time for RSSI */
#define D_UNVOL_DAT2_RSSI_CYCLE      0x05                         /* RSSI Cycle*/
#define D_UNVOL_DAT2_INIT_ANT_FLAG   0x00                         /* Initial Antenna */
#define D_UNVOL_DAT2_OUT_FLAG        0x00                         /* Flag for switching antenna */
#define D_UNVOL_DAT2_SYNC_CYCLE      0x0A                         /* Interval for Watching Sync State */
#define D_UNVOL_DAT2_IN_TO_OUT_THR   0x03                         /* Switching time (Inner -> out)*/
#define D_UNVOL_DAT2_OUT_TO_IN_THR   0x03                         /* Switching time (out -> Inner)*/

#define D_ANTENNA_PICT_AN_QPSK0_1    0x0022DF58                   /* QPSK、R=2/3（0⇔1）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_QPSK1_2    0x001C32C0                   /* QPSK、R=2/3（1⇔2）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_QPSK2_3    0x00117650                   /* QPSK、R=2/3（2⇔3）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_QPSK3_4    0x000A6554                   /* QPSK、R=2/3（3⇔4）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_QPSK4_5    0x0005CC60                   /* QPSK、R=2/3（4⇔5）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_16QAM0_1   0x0042BA08                   /* 16QAM、R=1/2（0⇔1）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_16QAM1_2   0x003A2AA0                   /* 16QAM、R=1/2（1⇔2）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_16QAM2_3   0x002D3A20                   /* 16QAM、R=1/2（2⇔3）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_16QAM3_4   0x001E9BF0                   /* 16QAM、R=1/2（3⇔4）　0～5の6段階 */
#define D_ANTENNA_PICT_AN_16QAM4_5   0x00130524                   /* 16QAM、R=1/2（4⇔5）　0～5の6段階 */

#endif /* __RADIO_MB86A35_NVTMP_H__ */
