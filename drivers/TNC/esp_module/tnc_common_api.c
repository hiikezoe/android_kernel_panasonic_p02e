#include "linux_precomp.h"
#include "tnc_common_api.h"

unsigned int g_TncLoglevel = TNC_LOGLEVEL;

void TNC_PRINTF(const char *str, ...)
{

//    char* szLoglevel;
    unsigned int iLogType = 0;

    va_list arg; 
    va_start(arg, str); 

//    szLoglevel = getenv(TNC_ENV_LOGLEVEL);

    /* 環境変数が存在しない場合は、ビルド時に設定したもの(MYLIB_DEFAULT_LOGLEVEL)に従うに従う */
//    if (szLoglevel == NULL)
//    {
        iLogType = g_TncLoglevel;
//    }
    /* 環境変数が存在する場合環境変数に従う。*/
//    else
//    {
//        int iRet = sscanf(szLoglevel, "%x", &iLogType);
        
        /* 環境変数から正しく取得できなかった場合は、ビルド時の設定値に変更 */
//        if (iRet <= 0)
//        {
//             iLogType = g_TncLoglevel;
//        }
//    }
    
    /* ログレベルが0より大きい場合はログ出力を行う */
    if(iLogType > 0)
    {
        vprintk(str, arg);
    }
}
