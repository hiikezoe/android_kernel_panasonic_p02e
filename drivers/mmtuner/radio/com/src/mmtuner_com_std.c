#include "dtvd_tuner_com.h"

/*****************************************************************************/
/* MODULE   : _dtvd_tuner_memset                                             */
/*****************************************************************************/
void _dtvd_tuner_memset
(
    void* dest,                         /* コピー先のバッファ */
    unsigned int val,                   /* 設定するデータ */
    unsigned int count                  /* 設定するバイト数 */
)
{
    if( dest == D_DTVD_TUNER_NULL )
    {
        return;
    }

    memset( dest, (signed int)val, (size_t)count );
    return;
}

/*****************************************************************************/
/* MODULE   : _dtvd_tuner_memcpy                                             */
/*****************************************************************************/
void _dtvd_tuner_memcpy
(
    void* dest,                         /* コピー先のバッファ */
    const void* src,                    /* コピー元のバッファ */
    unsigned int count                  /* コピーするバイト数 */
)
{
    if( dest == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    if( src == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    memcpy( dest, src, (size_t)count );
    return;
}

/*****************************************************************************/
/* MODULE   : _dtvd_tuner_memmove                                            */
/*****************************************************************************/
void _dtvd_tuner_memmove
(
    void* dest,                         /* 移動先のバッファ */
    const void* src,                    /* 移動元のバッファ */
    unsigned int count                  /* 移動先のバイト数 */
)
{
    if( dest == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    if( src == D_DTVD_TUNER_NULL )
    {
        /* リセット */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    memmove( dest, src, (size_t)count );
    return;
}

#ifndef _D_DTVD_TUNER_NO_BUFFOVER_CHECK
/*****************************************************************************/
/* MODULE   : dtvd_tuner_memset                                              */
/*****************************************************************************/
void dtvd_tuner_memset
(
    void* dest,                         /* コピー先のバッファ */
    unsigned int val,                   /* 設定するデータ */
    unsigned int count,                 /* 設定するバイト数 */
    unsigned int destsize               /* 対象バッファのサイズ */
)
{
    if( dest == D_DTVD_TUNER_NULL )
    {
        return;
    }

    /* メモリフィル領域サイズのチェック */
    if( count > destsize )
    {
    }
    else
    {
        _dtvd_tuner_memset( dest, val, count );
    }
    return;
}
#endif /* _D_DTVD_TUNER_NO_BUFFOVER_CHECK */

#ifndef _D_DTVD_TUNER_NO_BUFFOVER_CHECK
/*****************************************************************************/
/* MODULE   : dtvd_tuner_memcpy                                              */
/*****************************************************************************/
void dtvd_tuner_memcpy
(
    void* dest,                         /* コピー先のバッファ */
    const void* src,                    /* コピー元のバッファ */
    unsigned int count,                 /* コピーするバイト数 */
    unsigned int destsize               /* コピー先のバッファサイズ */
)
{
    /* NULLポインタ逆参照のチェック */
    if( (dest == D_DTVD_TUNER_NULL) || (src == D_DTVD_TUNER_NULL) )
    {
        /* リセットする */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    /* コピー領域サイズのチェック */
    if( count > destsize )
    {
        /* リセットする */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  count, destsize, 0, 0, 0, 0 );
    }
    else
    {
        _dtvd_tuner_memcpy( dest, src, count );
    }
    return;
}
#endif /* _D_DTVD_TUNER_NO_BUFFOVER_CHECK */

#ifndef _D_DTVD_TUNER_NO_BUFFOVER_CHECK
/*****************************************************************************/
/* MODULE   : dtvd_tuner_memmove                                             */
/*****************************************************************************/
void dtvd_tuner_memmove
(
    void* dest,                         /* コピー先のバッファ */
    const void* src,                    /* コピー元のバッファ */
    unsigned int count,                 /* コピーするバイト数 */
    unsigned int destsize               /* コピー先のバッファサイズ */
)
{
    /* NULLポインタ逆参照のチェック */
    if( (dest == D_DTVD_TUNER_NULL) || (src == D_DTVD_TUNER_NULL) )
    {
        /* リセットする */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  0, 0, 0, 0, 0, 0 );
        return;
    }

    /* コピー領域サイズのチェック */
    if( count > destsize )
    {
        /* リセットする */
        DTVD_TUNER_SYSERR_RANK_A( D_DTVD_TUNER_SYSERR_DRV_PARAM,
                                  DTVD_TUNER_COM_STD,
                                  count, destsize, 0, 0, 0, 0 );
    }
    else
    {
        /* 0バイト時は何もしない */
        if( count > 0 )
        {
            _dtvd_tuner_memmove( dest, src, count );
        }
    }
    return;
}
#endif /* _D_DTVD_TUNER_NO_BUFFOVER_CHECK */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
