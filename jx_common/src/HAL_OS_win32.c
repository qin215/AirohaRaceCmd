#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#include <errno.h>

#include "mywin.h"
#include "qcloud_iot_export_log.h"

/*
 * 分配内存
 */
void *HAL_Malloc(_IN_ size_t size)
{
    return malloc(size);
}

/*
 * 释放内存
 */
void HAL_Free(_IN_ void *ptr)
{
    free(ptr);
}

/*
 * 格式化打印
 */
void HAL_Printf(_IN_ const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

/*
 * 安全格式化打印
 */
int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

/*
 * 打印当前日期和时间的缓冲区
 */
static char now_time_str[32] = {0};

/*
 * 返回当前的日期和时间buffer, 添加毫秒支持
 */
char* HAL_Timer_current(void) 
{
	struct tm *newtime;
	char am_pm[] = "AM";
	__time64_t long_time;

	SYSTEMTIME tmSys;
	GetLocalTime(&tmSys);

	sprintf(now_time_str,"%d-%02d-%02d %02d:%02d:%02d.%03d ", tmSys.wYear, tmSys.wMonth, tmSys.wDay, 
		tmSys.wHour, tmSys.wMinute, tmSys.wSecond, tmSys.wMilliseconds);

#if 0
	_time64(&long_time );           // Get time as 64-bit integer.
	// Convert to local time.
	newtime = _localtime64(&long_time ); // C4996
	sprintf(now_time_str,"%d-%02d-%02d %02d:%02d:%02d ", newtime->tm_yday + 1900, newtime->tm_mon + 1,  newtime->tm_mday, 
					newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
#endif

	return now_time_str;
}

/*
 * 从字串s尾部匹配str字串
 */
char *strrstr(char *s, char *str)
{
	char *p; 
	int len = strlen(s);
	int n = strlen(str);

	for (p = s + len - 1; p >= s; p--) 
	{
		if ((*p == *str) && (memcmp(p, str, n) == 0)) 
		{
			return p;
		}
	}

	return NULL;
}

/*
 * 打开cmd console窗口,printf语句打印到此处
 */
FUNC_DLL_EXPORT void enable_console_window()
{
	AllocConsole();
	freopen( "CONOUT$","w",stdout);
	printf("hello");			// for testing.
}


/*
 * 判断目录是否存在，如果不存在，则建立；如果相同的文件名存在，则删除文件，建立同名目录
 */
Boolean ensure_directory_exist(const char *path)
{
	struct _stat buf;
	int result;

	result = _stat(path, &buf );

	if (result < 0)
	{
		if (errno == ENOENT)
		{
			// Directory not exist, create it.
			return CreateDirectory(path, NULL);
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		if (buf.st_mode & _S_IFDIR)
		{
			return TRUE;
		}
		else
		{
			Log_d("path (%s) is not dir!", path);
			_unlink(path);

			return CreateDirectory(path, NULL);
		}
	}
}

 void sz_print_data_buf(char *str, kal_uint8 *data, int dataLen)
{
	int i = 0;
	char buff[4096];
	int index = 0;
	int left_space = sizeof(buff);

	memset(buff, 0, sizeof(buff));
	for (; i < dataLen && left_space > 0; i++)
	{
		/*每行分两大列显示，每大列显示8个字节，每行显示16个字节*/
		left_space -= index;
		if (0 == i)
		{
			index += _snprintf(&buff[index], left_space, " %02x ", data[i] & 0xff);
			continue;
		}
		else if (0 == (i % (8 * 2)))
		{
			index += _snprintf(&buff[index], left_space, "\n ");
		}
		else if (0 == (i % 8))
		{
			index += _snprintf(&buff[index], left_space, "\t");
		}
		else
		{
			/*none*/
		}

		index += _snprintf(&buff[index], left_space, "%02x ", data[i] & 0xff);
	}

	if (left_space > 2)
	{
		index += _snprintf(&buff[index], left_space, "\n");
	}
	

	Log_d("%s:\n%s", str, buff);
}


 char * convert_binary_to_hex(kal_uint8 *data, int dataLen)
 {
	 int i = 0;
	 char *buff;
	 int index = 0;
	 int len = dataLen * 3 + 128;

	 buff = (char *)malloc(len);
	 if (!buff)
	 {
		 Log_e("no memory error!");
		 return NULL;
	 }

	 memset(buff, 0, len);
	 for (i = 0; i < dataLen; i++)
	 {
		 index += sprintf(&buff[index], "%02x ", data[i] & 0xff);
	 }

	 index += sprintf(&buff[index], "\n");

	 return buff;
 }

#if 0
/*打印出报文*/
void sz_print_pkt(char *str, kal_uint8 *data, int dataLen)
{
	Log_d("%s\n", str);
	sz_print_data_buf(data, dataLen);
}
#endif

/*
 * 以当前时间戳生成文件后缀名
 */
char* HAL_get_current_dt_filename(void) 
{
	SYSTEMTIME tmSys;
	GetLocalTime(&tmSys);

	sprintf(now_time_str,"%d-%02d-%02d_%02d_%02d_%02d ", tmSys.wYear, tmSys.wMonth, tmSys.wDay, 
		tmSys.wHour, tmSys.wMinute, tmSys.wSecond);

	return now_time_str;
}


void test_sscanf()
{
	const char *str = "xyz1235 555";
	int n1, n2;
	int ret;
	const char *line = "3873eae3a87b,2241d9976cc183c09f6f5a1e00d9de907f693b04f0c7074efaef9b9c6893410c561d63be6019c722a0e0dc42037d3a352d063cc24fcfdc27214b8158df7828a408335ee57bc9fcd9ee8d47026e370f4a596b12d779043b6510c9a52c5e1934437b69fbc1a0bcf006007936527e7b9bc7b89e8f5f3c8581e8aeee57b6b419d71ff7904c2dd89cc58c29ddf4f22ef68273995ee0539a53712ddc10b0b699d501b2d41d269d3c14876bc4ed8d61305199d782715da7a282b34b09e776055050b212e57c3ae6f1674ec209fe04c689b0b3842012eba58132bd9760e5551ba759475a8d3b8295c113b179561e1935030e713652a6d2e1bc11e97fb7f1e6e720dcf9d95ff87fdb4023a2a3971f24c04da545981d54c5c1f54d55c4b41aaf4c51a351749633c71924e54a69df4664f15902b1af7366ede0e5337b10d775334ad5917cd4baf5a369557141d78977cfb8d206a09b9c131bfadf9df8e6fe93f905bba2b829b2aebed1f2238939d78d07531ba66a789530023ed4b783c7e99de1d62f0456ced203c6fdfb8a27886992676d0e6a6308c0d559dabd34e779bfdc502941a8d0cfb72b41ea9d2758a8f5a7e03c97970402c08471869f61cd69754a129771ac981ae324127a95a2faeecb95f5d8c56681727f05d720b19b9959bb4b35ea4c00ebb15ad157e05d359e6dbedce9d46f4c5199e59ea62cf061580c520e69aa01ad75e01fc36d498ce018c432d10fdf4851c305589c3d44bcf6c1e70e74c556da38afd37249424363bec59fedcfdd4abbfb7e3abcc94e994492de593fe4e05b7034846b16021442a7109821537d9098089dea0696e1ff1cf1d83ab261ad2d3f9149960f8e6b3ae2424c9ac454fb3b6364f5e8662b23dcf84b2144e186206e03b06eb12bd3156764653999a039ffb4d54387a2114d621f39e8a8919d4edb956605c723ca5f25f03f0157bce944862d2dd49027bce2a4f043cfbddb45f5f08358fff2141416eb7fa7121f1012c10f428d9fdcf5b666cc5240f99fd2e35ccc8a3c1df9eab8";
	char mac_str[13];
	char lic[1024];

	memset(mac_str, 0, sizeof(mac_str));
	memset(lic, 0, sizeof(lic));

	ret = sscanf(line, "%[0-9a-fA-F],%[0-9A-Fa-f]", mac_str, lic);


}