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

    /* ���ϐ������݂��Ȃ��ꍇ�́A�r���h���ɐݒ肵������(MYLIB_DEFAULT_LOGLEVEL)�ɏ]���ɏ]�� */
//    if (szLoglevel == NULL)
//    {
        iLogType = g_TncLoglevel;
//    }
    /* ���ϐ������݂���ꍇ���ϐ��ɏ]���B*/
//    else
//    {
//        int iRet = sscanf(szLoglevel, "%x", &iLogType);
        
        /* ���ϐ����琳�����擾�ł��Ȃ������ꍇ�́A�r���h���̐ݒ�l�ɕύX */
//        if (iRet <= 0)
//        {
//             iLogType = g_TncLoglevel;
//        }
//    }
    
    /* ���O���x����0���傫���ꍇ�̓��O�o�͂��s�� */
    if(iLogType > 0)
    {
        vprintk(str, arg);
    }
}
