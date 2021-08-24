#ifndef QCLOUD_IOT_EXPORT_LOG_H_
#define QCLOUD_IOT_EXPORT_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ServerCmd.h"


#define _IN_           
#define _OU_     

/**
 * ��־����ȼ�
 */
typedef enum 
{
    QC_DEBUG,
    QC_INFO,
    QC_WARN,
    QC_ERROR
} QC_LOG_LEVEL;

/**
 * ȫ����־���������־, ֻ���С�ڻ���ڸõȼ�����־��Ϣ
 */
extern QC_LOG_LEVEL g_log_level;

typedef Boolean (*LogMessageHandler)(const char* message);

/**
 * @brief
 *
 * @param
 */
void IOT_Log_Set_Level(QC_LOG_LEVEL level);

/**
 * @brief ��ȡ��ǰ��־�ȼ�
 *
 * @return
 */
QC_LOG_LEVEL IOT_Log_Get_Level();

/**
 * @brief ������־�ص��������û��ӹ���־��������������ļ��Ȳ���
 *
 * @param handler �ص�����ָ��
 *
 */
void IOT_Log_Set_MessageHandler(LogMessageHandler handler);

/**
 * @brief ��־��ӡ������Ĭ�ϴ�ӡ����׼��������û�������־��ӡhandlerʱ���ص�handler
 *
 * @param file Դ�ļ���
 * @param func ������
 * @param line �к�
 * @param level ��־�ȼ�
 */
void Log_writter(const char *file, const char *func, const int line, const int level, const char *fmt, ...);

#define Log_d(args, ...) Log_writter(__FILE__, __FUNCTION__, __LINE__, QC_DEBUG, args, ##__VA_ARGS__)
#define Log_i(args, ...) Log_writter(__FILE__, __FUNCTION__, __LINE__, QC_INFO, args, ##__VA_ARGS__)
#define Log_w(args, ...) Log_writter(__FILE__, __FUNCTION__, __LINE__, QC_WARN, args, ##__VA_ARGS__)
#define Log_e(args, ...) Log_writter(__FILE__, __FUNCTION__, __LINE__, QC_ERROR, args, ##__VA_ARGS__)

#ifdef IOT_DEBUG
	#define IOT_FUNC_ENTRY    \
		{\
		printf("FUNC_ENTRY:   %s L#%d \n", __FUNCTION__, __LINE__);  \
		}
	#define IOT_FUNC_EXIT    \
		{\
		printf("FUNC_EXIT:   %s L#%d \n", __FUNCTION__, __LINE__);  \
		return;\
		}
	#define IOT_FUNC_EXIT_RC(x)    \
		{\
		printf("FUNC_EXIT:   %s L#%d Return Code : %ld \n", __FUNCTION__, __LINE__, (long)(x));  \
		return x; \
		}
#else
	#define IOT_FUNC_ENTRY
	#define IOT_FUNC_EXIT 			\
		{\
			return;\
		}
	#define IOT_FUNC_EXIT_RC(x)     \
		{\
			return x; \
		}
#endif

//
void HAL_Printf(_IN_ const char *fmt, ...);

//
int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...);
 
//
char* HAL_Timer_current(void);

/*
 * �Զ����¼log����
 */
Boolean MyLogMessageHandler(const char* message);

char *strrstr(char *s, char *str);

#ifdef __cplusplus
}
#endif

#endif /* QCLOUD_IOT_EXPORT_LOG_H_ */