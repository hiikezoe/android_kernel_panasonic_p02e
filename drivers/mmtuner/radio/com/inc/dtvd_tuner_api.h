#ifndef _DTVD_TUNER_API_H
#define _DTVD_TUNER_API_H

#ifdef _DTVD_TUNER_DEBUG_OUT
#define DTVD_DEBUG_MSG_ENTER( funcname )       \
                   printf("%s %s %s %d ENTER :%s \n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (funcname) )
#define DTVD_DEBUG_MSG_EXIT( funcname )       \
                   printf("%s %s %s %d EXIT  :%s \n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (funcname) )
#define DTVD_DEBUG_MSG_CALL( funcname )       \
                   printf("%s %s %s %d CALL  :%s \n" \
                   , __DATE__, __TIME__, __FILE__, __LINE__, (funcname) )
#define DTVD_DEBUG_MSG_INFO( fmt, args... )   \
                   printf( "%s %s %s %d %s : " fmt,__DATE__,       \
                           __TIME__,__FILE__,__LINE__,__FUNCTION__,## args )
#else
#define DTVD_DEBUG_MSG_ENTER( funcname )
#define DTVD_DEBUG_MSG_EXIT( funcname )
#define DTVD_DEBUG_MSG_CALL( funcname )
#ifndef WIN32
#define DTVD_DEBUG_MSG_INFO( fmt, args... )
#else
#define DTVD_DEBUG_MSG_INFO( fmt, args)
#endif /* WIN32 */
#endif /* _DTVD_TUNER_DEBUG_OUT */

#endif /* _DTVD_TUNER_API_H */
/*****************************************************************************/
/*   Copyright(C) 2007 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
