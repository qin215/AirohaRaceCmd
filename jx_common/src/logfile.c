#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "qcloud_iot_export_log.h"
#include "mywin.h"


static FILE *fp;
#define MAX_LOG_FILESIZE	(8 * 1024 * 1024) 			// 300k
#define MAX_CHECK_TIMEOUT	(60 * 1000)					// 1 Minute

static char log_filename[128];
static int filename_index;

static int get_log_filesize();
static void check_logfile();

#define LOG_DIR	"log\\"

/*
 * log �ص�����
 */
Boolean MyLogMessageHandler(const char* message)
{
	TCHAR path[MAX_PATH];
	int len;
	char error_buf[128];
	static __int64 timestamp = 0;
	__int64 tmp;
	static int count;

	while (!fp)
	{
		get_program_path(path, sizeof(path));
		strcat(path, LOG_DIR);
		ensure_directory_exist(path);

		len = strlen(path);
		_snprintf_s(&path[len], sizeof(log_filename) - len, _TRUNCATE, "log_%s.txt", HAL_get_current_dt_filename());

		fp = fopen(path, "a+");
		if (!fp)
		{
			strerror_s(error_buf, sizeof(error_buf), errno);
			printf("open failed:%s!\n", error_buf);
			return FALSE;
		}

		check_logfile();
	}

	fprintf(fp, "%s", message);
	fflush(fp);

	if (count++ < 5)
	{
		return FALSE;
	}

	count = 0;

	tmp = get_current_timestamp_ms();

	if (timestamp == 0)
	{
		timestamp = tmp;
	}
	else
	{
		if (tmp - timestamp >= MAX_CHECK_TIMEOUT)	// ��ʱ���
		{
			check_logfile();
			timestamp = tmp;
		}
	}

	return FALSE;
}


/*
 * ��ȡ��ǰlog file�ļ���С
 */
static int get_log_filesize()
{
	int filesize = 0;
	int pos;
	
	if (fp)
	{
		pos = ftell(fp);
		
	 	fseek(fp, 0L, SEEK_END);  
	    filesize = ftell(fp);

		fseek(fp, pos, SEEK_SET);
	}
	
    return filesize;  
}


/*
 * �ر�logfile
 */
static void close_logfile()
{
	if (fp)
	{
		fflush(fp);
		fclose(fp);

		fp = NULL;
	}
}

/*
 * ���logfile ��С, �������Ԥ����log��С����ر�
 */
static void check_logfile()
{
	int filesize;

	filesize = get_log_filesize();
	printf("filesize=%d\n", filesize);
	
	if (filesize >= MAX_LOG_FILESIZE)
	{
		close_logfile();
		filename_index++;
	}
}

/*
 * ���ļ���¼log
 */
FUNC_DLL_EXPORT void enable_log_file()
{
	IOT_Log_Set_MessageHandler(MyLogMessageHandler);
	IOT_Log_Set_Level(QC_DEBUG);
}
