/*
 * @file felica_ctrl_debug.h
 * @brief Local header file of FeliCa control driver
 *
 * @date 2011/06/29
 */

#ifndef __DV_FELICA_CTRL_DEBUG_H__
#define __DV_FELICA_CTRL_DEBUG_H__

/* -----------------------------------------------------------------------------
 * デバッグ用表示マクロ
 * -------------------------------------------------------------------------- */
#if defined(FELIACA_SYS_LOG_ON) || defined(FELIACA_RAM_LOG_ON)

/* Log output setting */
#if defined(FELIACA_SYS_LOG_ON)
#define felica_printk(fmt, ...)    printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#endif /* FELIACA_SYS_LOG_ON */

#if defined(FELIACA_RAM_LOG_ON)
#define felica_printk(fmt, ...)    felicadd_debug_write_ram(fmt, ##__VA_ARGS__)
#endif /* FELIACA_RAM_LOG_ON */

/* LOG Level Setting */
#if defined(FELIACA_LOG_IF_STED)
#define DEBUG_FD_LOG_IF_FNC_ST(fmt, ...)      DEBUG_FD_LOG_OUT_FUNC("***IF ST***", fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_IF_FNC_ED(fmt, ...)      DEBUG_FD_LOG_OUT_FUNC("***IF ED***", fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_IF_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_IF_FNC_ED(fmt, ...)
#endif /* FELIACA_LOG_IF_STED */

#if defined(FELIACA_LOG_IF_PARAM)
#define DEBUG_FD_LOG_IF_FNC_PARAM(fmt, ...)   DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_IF_FNC_PARAM(fmt, ...)
#endif /* FELIACA_LOG_IF_PARAM */

#if defined(FELIACA_LOG_IF_ASSERT)
#define DEBUG_FD_LOG_IF_ASSERT(fmt, ...)      DEBUG_FD_LOG_OUT_ASSERT(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_IF_ASSERT(fmt, ...)
#endif /* FELIACA_LOG_IF_ASSERT */

#if defined(FELIACA_LOG_PRIV_STED)
#define DEBUG_FD_LOG_PRIV_FNC_ST(fmt, ...)    DEBUG_FD_LOG_OUT_FUNC("PRIV ST", fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_PRIV_FNC_ED(fmt, ...)    DEBUG_FD_LOG_OUT_FUNC("PRIV ED", fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_PRIV_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_PRIV_FNC_ED(fmt, ...)
#endif /* FELIACA_LOG_PRIV_STED */

#if defined(FELIACA_LOG_PRIV_PARAM)
#define DEBUG_FD_LOG_PRIV_FNC_PARAM(fmt, ...) DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_PRIV_FNC_PARAM(fmt, ...)
#endif /* FELIACA_LOG_PRIV_PARAM */

#if defined(FELIACA_LOG_PRIV_ASSERT)
#define DEBUG_FD_LOG_PRIV_ASSERT(fmt, ...)    DEBUG_FD_LOG_OUT_ASSERT(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_PRIV_ASSERT(fmt, ...)
#endif /* FELIACA_LOG_PRIV_ASSERT */

#if defined(FELIACA_LOG_ITR_STED)
#define DEBUG_FD_LOG_ITR_FNC_ST(fmt, ...)     DEBUG_FD_LOG_OUT_FUNC("***ITR ST***", fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_ITR_FNC_ED(fmt, ...)     DEBUG_FD_LOG_OUT_FUNC("***ITR ED***", fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_ITR_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_ITR_FNC_ED(fmt, ...)
#endif /* FELIACA_LOG_ITR_STED */

#if defined(FELIACA_LOG_ITR_PARAM)
#define DEBUG_FD_LOG_ITR_FNC_PARAM(fmt, ...)  DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_ITR_FNC_PARAM(fmt, ...)
#endif /* FELIACA_LOG_ITR_PARAM */

#if defined(FELIACA_LOG_ITR_ASSERT)
#define DEBUG_FD_LOG_ITR_ASSERT(fmt, ...)     DEBUG_FD_LOG_OUT_ASSERT(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_ITR_ASSERT(fmt, ...)
#endif /* FELIACA_LOG_ITR_ASSERT */

#if defined(FELIACA_LOG_EXIF_STED)
#define DEBUG_FD_LOG_EXIF_FNC_ST(fmt, ...)    DEBUG_FD_LOG_OUT_FUNC("EXIF ST", fmt, ##__VA_ARGS__)
#define DEBUG_FD_LOG_EXIF_FNC_ED(fmt, ...)    DEBUG_FD_LOG_OUT_FUNC("EXIF ED", fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_EXIF_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_EXIF_FNC_ED(fmt, ...)
#endif /* FELIACA_LOG_EXIF_STED */

#if defined(FELIACA_LOG_EXIF_PARAM)
#define DEBUG_FD_LOG_EXIF_FNC_PARAM(fmt, ...) DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_EXIF_FNC_PARAM(fmt, ...)
#endif /* FELIACA_LOG_EXIF_PARAM */

#if defined(FELIACA_LOG_TERMINAL_IF)
#define DEBUG_FD_LOG_TERMINAL_IF(fmt, ...)    DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_TERMINAL_IF(fmt, ...)
#endif /* FELIACA_LOG_TERMINAL_IF */

#if defined(FELIACA_LOG_TERMINAL_DEV)
#define DEBUG_FD_LOG_TERMINAL_DEV(fmt, ...)   DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_TERMINAL_DEV(fmt, ...)
#endif /* FELIACA_LOG_TERMINAL_DEV */

#if defined(FELIACA_LOG_TERMINAL_ITR)
#define DEBUG_FD_LOG_TERMINAL_ITR(fmt, ...)   DEBUG_FD_LOG_OUT_PARAM(fmt, ##__VA_ARGS__)
#else
#define DEBUG_FD_LOG_TERMINAL_ITR(fmt, ...)
#endif /* FELIACA_LOG_TERMINAL_ITR */

/* OUTPUT format */
#define DEBUG_FD_LOG_OUT_FUNC(str, fmt, ...) ({                                \
    do{                                                                        \
        char dmsg[30];                                                         \
        struct timeval tv;                                                     \
        do_gettimeofday(&tv);                                                  \
        sprintf(dmsg, fmt, ##__VA_ARGS__);                                     \
        felica_printk("[DVFD][%03ld.%03ld.%03ld][%s][%s]%s\n"                  \
                          , tv.tv_sec%1000, tv.tv_usec/1000, tv.tv_usec%1000   \
                          , str, __func__, dmsg);                              \
    }while(0); })                                                              \

#define DEBUG_FD_LOG_OUT_PARAM(fmt, ...) ({                                    \
    do{                                                                        \
        char dmsg[30];                                                         \
        sprintf(dmsg, fmt, ##__VA_ARGS__);                                     \
        felica_printk("[DVFD][%s]%s\n", __func__, dmsg);                       \
    }while(0); })                                                              \

#define DEBUG_FD_LOG_OUT_ASSERT(fmt, ...) ({                                   \
    do{                                                                        \
        char dmsg[50];                                                         \
        sprintf(dmsg, fmt, ##__VA_ARGS__);                                     \
        felica_printk("[DVFD][---ASSERT---][%s][%d]%s\n"                       \
                                     , __func__, __LINE__, dmsg);              \
    }while(0); })                                                              \

#define DEBUG_FD_LOG_MINOR_TO_STR(minor) \
    DEBUG_FD_LOG_IF_FNC_PARAM(                                                 \
        "minor=%s(%d)",                                                        \
        (minor==FELICA_CTRL_MINOR_NO_PON ? "FELICA_CTRL_MINOR_NO_PON" :        \
        (minor==FELICA_CTRL_MINOR_NO_CEN ? "FELICA_CTRL_MINOR_NO_CEN" :        \
        (minor==FELICA_CTRL_MINOR_NO_RFS ? "FELICA_CTRL_MINOR_NO_RFS" :        \
        (minor==FELICA_CTRL_MINOR_NO_ITR ? "FELICA_CTRL_MINOR_NO_ITR" :        \
        (minor==SNFC_CTRL_MINOR_NO_RFS   ? "SNFC_CTRL_MINOR_NO_RFS"   :        \
        (minor==SNFC_CTRL_MINOR_NO_INTU  ? "SNFC_CTRL_MINOR_NO_INTU"  :        \
        (minor==SNFC_CTRL_MINOR_NO_CEN   ? "SNFC_CTRL_MINOR_NO_CEN"   :        \
        (minor==SNFC_CTRL_MINOR_NO_AVAILABLE ? "SNFC_CTRL_MINOR_NO_AVAILABLE" :\
        (minor==SNFC_CTRL_MINOR_NO_HSEL  ? "SNFC_CTRL_MINOR_NO_HSEL"  :        \
        (minor==SNFC_CTRL_MINOR_NO_AUTOPOLL ? "SNFC_CTRL_MINOR_NO_AUTOPOLL"  : \
        (minor==SNFC_CTRL_MINOR_NO_PON   ? "SNFC_CTRL_MINOR_NO_PON"   :        \
        "FELICA_CTRL_MINOR_NO_?????")))))))))))                                 \
        ,minor )                                                               \

#else

#define DEBUG_FD_LOG_IF_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_IF_FNC_ED(fmt, ...)
#define DEBUG_FD_LOG_IF_FNC_PARAM(fmt, ...)
#define DEBUG_FD_LOG_IF_ASSERT(fmt, ...)

#define DEBUG_FD_LOG_PRIV_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_PRIV_FNC_ED(fmt, ...)
#define DEBUG_FD_LOG_PRIV_FNC_PARAM(fmt, ...)
#define DEBUG_FD_LOG_PRIV_ASSERT(fmt, ...)

#define DEBUG_FD_LOG_ITR_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_ITR_FNC_ED(fmt, ...)
#define DEBUG_FD_LOG_ITR_FNC_PARAM(fmt, ...)
#define DEBUG_FD_LOG_ITR_ASSERT(fmt, ...)

#define DEBUG_FD_LOG_EXIF_FNC_ST(fmt, ...)
#define DEBUG_FD_LOG_EXIF_FNC_ED(fmt, ...)
#define DEBUG_FD_LOG_EXIF_FNC_PARAM(fmt, ...)

#define DEBUG_FD_LOG_TERMINAL_IF(fmt, ...)
#define DEBUG_FD_LOG_TERMINAL_DEV(fmt, ...)
#define DEBUG_FD_LOG_TERMINAL_ITR(fmt, ...)

#define DEBUG_FD_LOG_MINOR_TO_STR(minor)

#endif /* FELIACA_SYS_LOG_ON || FELIACA_RAM_LOG_ON */

/* -------------------------------------------------------------------------- */
/* デバイスグローバル情報                                                     */
/* -------------------------------------------------------------------------- */
#if defined(FELIACA_RAM_LOG_ON)
extern struct cdev g_cdev_fcommdebug;       /* キャラクタドライバ(DEBUG用)    */
#endif /* FELIACA_RAM_LOG_ON */

/*
 * for Module test
 */
#ifdef FELIACA_MODULE_TEST
#define static
#define FELICA_MT_WEAK __attribute__((weak))
#else
#define FELICA_MT_WEAK
#endif  /* FELIACA_MODULE_TEST */

#ifdef FELIACA_MODULE_TEST
#include <stdio.h>
#include <sys/time.h>
#define static
#define printk printf
#define do_gettimeofday(tv) (gettimeofday((tv), NULL))
#define KERN_DEBUG
#endif  /* FELIACA_MODULE_TEST */

#ifdef FELICA_DEBUG_PROC_ON
#define FELICA_STATIC
#else
#define FELICA_STATIC static
#endif /* FELICA_DEBUG_PROC_ON */

/* -------------------------------------------------------------------------- */
/* プロトタイプ宣言                                                           */
/* -------------------------------------------------------------------------- */

#ifdef FELIACA_RAM_LOG_ON
void felicadd_debug_write_ram(unsigned char*, ...);
ssize_t felicadd_debug_read_ram(struct file*, char*, size_t, loff_t*);
int felicadd_debug_ioctl(struct inode*, struct file*, unsigned int, unsigned long);
#endif /* FELIACA_RAM_LOG_ON */

#endif /* __DV_FELICA_CTRL_DEBUG_H__ */
