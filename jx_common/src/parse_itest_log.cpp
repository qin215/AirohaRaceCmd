/*
 * 美的智能生产接口程序, written by qinjiangwei 2018/12/6
 * 配合美的项目生产测试
 */
#include "stdafx.h"
#include "mmsystem.h"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <process.h>    /* _beginthread, _endthread */
#include <vector>
#include <map>
#include "regex.h"

#include "mywin.h"
#include "media_test.h"
#include "qcloud_iot_export_log.h"

#pragma comment(lib, "regex.lib")
#pragma comment(lib, "winmm.lib")

using namespace std;

/*******************************************************************
*作用
       构造函数
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
CITestFile::CITestFile()
{

}

/*******************************************************************
*作用
       构造函数
*参数
		LPCTSTR lpszPath	-	路径名
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
CITestFile::CITestFile(LPCTSTR lpszPath)
{
	try
	{
		Open(lpszPath, CFile::modeRead);
	}
	catch (CMemoryException* e)
	{
		Log_e("no memory !%s", lpszPath);
		throw;
	}
	catch (CFileException* e)
	{
		Log_e("file path not exist!%s", lpszPath);
		throw;
	}
	catch (CException* e)
	{
		Log_e("unkown exception:%s", lpszPath);
		throw;
	}
}

/*******************************************************************
*作用
       析构函数
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
CITestFile::~CITestFile()
{

}

/*******************************************************************
*作用
       解析出测试结果
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
CMediaHardwareTestResult CITestFile::GetTestResult()
{
	CMediaHardwareTestResult res;
	CString mac;
	SYSTEMTIME dt;


	mac = GetTestMacAddrString2();
	Log_d("parse mac:%s\n", mac.GetString());
	res.SetMacAddress(mac);

	dt = GetTestDateTime();
	res.SetDateTime(&dt);


	return res;
}

/*******************************************************************
*作用
       解析测试TX 测试的一条记录
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
kv_t CITestFile::ParseTxTestOneItem()
{
	CString line;
	int index;
	char *buff;
	CString ret;
	typedef enum line_status 
	{
		STATUS_GET_INIT,
		STATUS_GET_HEADER,
		STATUS_GET_DATA,
		STATUS_GET_RET,
		STATUS_GET_DONE
	} line_status_t;
	
	line_status_t status = STATUS_GET_INIT;
	int channel;
	int freq;
	CString freq_str;
	kv_t map;


#define TX_TEST_PREFIX "WT_VERIFY_TX_ALL"
	//SeekToBegin();
	while (ReadString(line))
	{
		switch (status)
		{
			case STATUS_GET_INIT:
			{
				index = line.Find(TX_TEST_PREFIX, 0);
				if (index >= 0)
				{
					char tmp[32];
					char *p = line.GetBuffer() + index + strlen(TX_TEST_PREFIX);

					status = STATUS_GET_HEADER;
					sscanf(p, "%d(%d) %3s", &freq, &channel, tmp);
					
					freq_str = CString(tmp);
					
					continue;
				}

				break;
			}

			case STATUS_GET_HEADER:
			{
#define		TX_TEST_PREFIX	"TX_[0]"
				char key[64];
				char value[64];
				char *buff;
				int n;
				
				memset(key, 0, sizeof(key));
				memset(value, 0, sizeof(value));

				index = line.Find(TX_TEST_PREFIX);
				if (index < 0)
				{
					index = line.Find("PASS");
					if (index >= 0)
					{
						map[_T("TESTRESULT")] = "1";
						return map;
					}
					
					index = line.Find("FAIL");
					if (index >= 0)
					{
						map[_T("TESTRESULT")] = "0";
						return map;
					}

					continue;
				}

				buff = line.GetBuffer() + index + strlen(TX_TEST_PREFIX);

				typedef enum 
				{
					REG_MATCH_ALL,
					REG_MATCH_CHANNEL,
					REG_MATCH_KEY,
					REG_MATCH_UNUSED,
					REG_MATCH_VALUE,
					REG_MATCH_NUM
				} match_index_t;

				regex_t reg;
				regmatch_t match[REG_MATCH_NUM + 1];
				int ncount = 0;
				match_index_t index;
				char channel_str[32] = { 0 };
			
				
				//54M  EVM         : -30.36 dB 
				//  TX_[0]    11M  Power[ 0]   : 19.18  dBm
				if (0 == regcomp(&reg, "[ ]*\\([0-9]*M\\)[ ]*\\([a-zA-Z]*\\)\\(\\[ 0\\]\\)\\?[ ]*:[ ]*\\(-\\?[0-9]\\+\\.[0-9]\\+\\)", 0))
				{
					if (0 == regexec(&reg, buff, sizeof(match) / sizeof(regmatch_t), match, 0))
					{
						Log_d("regexp match!\n");
						char mytmp[100];
						regmatch_t *p = match;
						
						p = &match[REG_MATCH_CHANNEL];		// channel
						if (p->rm_so > 0)
						{
							memset(mytmp, 0, sizeof(mytmp));
							memcpy(mytmp, &buff[p->rm_so], p->rm_eo - p->rm_so);
							Log_d("match%d: %s\n", REG_MATCH_KEY, mytmp);
							strncpy(channel_str, mytmp, sizeof(channel_str) - 1);
							ncount++;
						}
						
						p = &match[REG_MATCH_KEY];		// key
						if (p->rm_so > 0)
						{
							memset(mytmp, 0, sizeof(mytmp));
							memcpy(mytmp, &buff[p->rm_so], p->rm_eo - p->rm_so);
							Log_d("match%d: %s\n", REG_MATCH_KEY, mytmp);
							strncpy(key, mytmp, sizeof(key) - 1);
							ncount++;
						}
						
						p = &match[REG_MATCH_VALUE];
						if (p->rm_so > 0)
						{
							memset(mytmp, 0, sizeof(mytmp));
							memcpy(mytmp, &buff[p->rm_so], p->rm_eo - p->rm_so);
							Log_d("match%d: %s\n", REG_MATCH_VALUE, mytmp);
							strncpy(value, mytmp, sizeof(value) - 1);
							ncount++;
						}
					}
					else
					{
						Log_e("regexp not match\n");
					}
				}

				if (ncount != 3)
				{
					Log_e("count error: %d\n", ncount);
				}
				else
				{
					map[CString(key)] = CString(value);
					map[_T("channel")] = CString(channel_str);
				}

				regfree(&reg);

				break;
			}
				

			default:
				break;
		}
	}

	return map;
}

/*******************************************************************
*作用
       获取所有的测试项的key/value
*参数
		测试结果数组
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
MediaHwTestVector CITestFile::GetTestResults()
{
	MediaHwTestVector v;
	kv_t map;
	kv_t::iterator it;
	CMediaHardwareTestResult res;
	CString mac;
	SYSTEMTIME dt;

	mac = GetTestMacAddrString2();
	Log_d("parse mac:%s\n", mac.GetString());
	if (mac.GetLength() != 17)
	{
		return v;
	}

	res.SetMacAddress(mac);

	dt = GetTestDateTime();
	res.SetDateTime(&dt);

	SeekToBegin();

	for (;;)
	{
		map = ParseTxTestOneItem();

		if (map.empty())
		{
			break;
		}

		res.ResetKeyValue();
		for (it = map.begin(); it != map.end(); it++)
		{
			httpd_kv_t kv;

			Log_d("key=%s, value=%s\n", it->first, it->second);

			if (strcmp(it->first, "channel") == 0)
			{
				res.SetItemCode(it->second);
			}
			else if (strcmp(it->first , "TESTRESULT") == 0)
			{
				res.SetResultValue(atoi(it->second));
			}
			else
			{
				strncpy(kv.key, it->first, sizeof(kv.key));
				strncpy(kv.value, it->second, sizeof(kv.value));
				res.AddKeyValue(&kv);
			}
		}

		v.push_back(res);
	}

	return v;
}


/*******************************************************************
*作用
       解析测试的模块地址
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
CString CITestFile::GetTestMacAddrString()
{
	CString line;
	int index;
	char *buff;
	CString ret;
	char mac_str[128];

#define MACADDR_PREFIX "Read  MAC[0]: "
	SeekToBegin();
	while (ReadString(line))
	{
		index = line.Find(MACADDR_PREFIX, 0);
		if (index >= 0)
		{
			break;
		}
	}

	if (index < 0)
	{
		Log_e("no found mac address!\n");
		return ret;
	}

	buff = line.GetBuffer();
	buff += index + strlen(MACADDR_PREFIX);
	scanf(buff, "%s", mac_str);
	int len = strlen(buff);
	if (len != 12)
	{
		Log_e("mac string len error:%d\n", len);
		return ret;
	}

	for (int i = 0; i < len; i += 2)
	{
		char tmp[16];

		tmp[0] = buff[i];
		tmp[1] = buff[i + 1];
		tmp[2] = 0;

		ret += CString(tmp);

		if (i + 2 < len)		// 不是最后一个
		{
			ret += CString(":");
		}
	}

	return ret;
}


/*******************************************************************
*作用
       解析测试的模块地址
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei

	-------------------------------------------------------------------------------
	Test Time: 0.16 sec


	DUT MAC:3873EAE3A591
********************************************************************/
CString CITestFile::GetTestMacAddrString2()
{
	CString line;
	int index;
	char *buff;
	CString ret;
	char mac_str[128];

#undef MACADDR_PREFIX
#define MACADDR_PREFIX "DUT MAC:"
	SeekToBegin();
	while (ReadString(line))
	{
		index = line.Find(MACADDR_PREFIX, 0);
		if (index >= 0)
		{
			break;
		}
	}

	if (index < 0)
	{
		Log_e("no found mac address!\n");
		return ret;
	}

	buff = line.GetBuffer();
	buff += index + strlen(MACADDR_PREFIX);
	scanf(buff, "%12s", mac_str);
	int len = strlen(buff);
	if (len != 12)
	{
		Log_e("mac string len error:%d\n", len);
		return ret;
	}

	for (int i = 0; i < len; i += 2)
	{
		char tmp[16];

		tmp[0] = buff[i];
		tmp[1] = buff[i + 1];
		tmp[2] = 0;

		ret += CString(tmp);

		if (i + 2 < len)		// 不是最后一个
		{
			ret += CString(":");
		}
	}

	return ret;
}

/*******************************************************************
*作用
       获取测试时间
*参数
		无
*返回值
		SYSTEMTIME	-	日期时间对象
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
SYSTEMTIME CITestFile::GetTestDateTime()
{
	SYSTEMTIME tm;
	CString line;
	int index;
	char *buff;
	int n;

#define DATETIME_PREFIX "Test Date:"

	memset(&tm, 0, sizeof(tm));
	
	SeekToBegin();
	while (ReadString(line))
	{
		index = line.Find(DATETIME_PREFIX, 0);
		if (index >= 0)
		{
			break;
		}
	}

	if (index < 0)
	{
		Log_e("no found timestamp!\n");
		return tm;
	}

	buff = line.GetBuffer();
	buff += index + strlen(DATETIME_PREFIX);
	n = sscanf(buff, " %d/%d/%d, Test Time: %d:%d:%d", &tm.wYear, &tm.wMonth, &tm.wDay, &tm.wHour, &tm.wMinute, &tm.wSecond);

	return tm;
}


/*******************************************************************
*作用
       获取目录下的log文件列表
*参数
		const CString& strSourceDir	-	目录路径
*返回值
		StringVector	-	完整路径列表
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
StringVector GetLogFileList(const CString& strSourceDir)
{
	StringVector vect_files;
	CString pattern = strSourceDir + "\\" + "*.log";
	_finddata_t file;   
	long longf;

	if ((longf = _findfirst(pattern, &file)) == -1)   
	{   
		return vect_files;
	}   

	CString fullpath;
	do 
	{
		fullpath =  strSourceDir + "\\" + file.name;
		vect_files.push_back(fullpath);
	} while (_findnext(longf, &file) == 0);

	_findclose(longf); 

	return vect_files;
}



/*******************************************************************
*作用
       测试函数
*参数
		无
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
	2018/12/6 by qinjiangwei

********************************************************************/
FUNC_DLL_EXPORT void parse_itest_stub()
{
#if 0
	try
	{
		CITestFile file(_T("D:\\itest_log.txt"));
		MediaHwTestVector v;
		MediaHwTestVector::iterator iter;

		v = file.GetTestResults();
		
		for (iter = v.begin(); iter != v.end(); iter++)
		{
			iter->FormatPrint();
		}
	}
	catch (CMemoryException* e)
	{
		
	}
	catch (CFileException* e)
	{
	}
	catch (CException* e)
	{
	}
#endif

	StringVector v;
	StringVector::iterator iter;

	return;

	v = GetLogFileList("D:\\testlog");
	for (iter = v.begin(); iter != v.end(); iter++)
	{
		Log_d("begin process logfile:%s", *iter);
		http_send_hw_test_result("192.168.3.118", 6690, "/api/iot/testing/hardware/stduri/bulkadd", *iter);
		Log_d("end process logfile:%s", *iter);
	}
}


static map<CString, __int64> logfile_map;			// change
static char http_server_name[128];					// http server hostname
/*
 * 定时器周期检查函数
 */
static void CALLBACK timer_callback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	map<CString, __int64>::iterator iter;
	typedef map<CString, __int64>::iterator MapIterator;
	vector<MapIterator> v;


#define ITTEST_TIMEOUT_VALUE	(6 * 1000)
	for (iter = logfile_map.begin(); iter != logfile_map.end(); iter++)
	{
		__int64 ts = get_current_timestamp_ms();

		DWORD diff = ts - iter->second;
		if (diff >= ITTEST_TIMEOUT_VALUE)
		{
			Log_d("begin process logfile:%s", iter->first);
			http_send_hw_test_result(http_server_name, 6690, "/api/iot/testing/hardware/stduri/bulkadd", iter->first);
			Log_d("end process logfile:%s", iter->first);

			v.push_back(iter);
		}
	}

	vector<MapIterator>::iterator it;
	for (it = v.begin(); it != v.end(); it++)
	{
		logfile_map.erase(*it);
	}
}

/*******************************************************************
*作用
		开启新线程监控目录文件变化
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
void StartMonitorItestLogThread(LPCTSTR logDir, char *hostname)
{
	strncpy(http_server_name, hostname, sizeof(http_server_name) - 1);
	_beginthread(MonitorItestLogDirectory, 0, (void *)logDir);
}



/*******************************************************************
*作用
		监控log目录下增加文件操作,当
*参数
		无
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
	2018/12/6 by qinjiangwei

********************************************************************/
void MonitorItestLogDirectory(void *arg)
{
	HANDLE hDir;
	LPCTSTR lpszLogDir = (LPCTSTR)arg;

	hDir = CreateFile(
				lpszLogDir,                          // pointer to the file name
				FILE_LIST_DIRECTORY,                // access (read-write) mode
				FILE_SHARE_READ|FILE_SHARE_DELETE,  // share mode
				NULL,                               // security descriptor
				OPEN_EXISTING,                      // how to create
				FILE_FLAG_BACKUP_SEMANTICS,         // file attributes
				NULL                                // file with attributes to copy
				);

	if (hDir == INVALID_HANDLE_VALUE)
	{
		Log_e("open directory(%s) failed!", lpszLogDir);
		return;
	}

	if (timeSetEvent(1000, 1, timer_callback, NULL, TIME_PERIODIC) == NULL)
	{
		Log_e("timer set event error!\n");
	}

	Log_d("begin monitor directory:%s", lpszLogDir);

	UCHAR buffer[2 * 1024];
	DWORD buf_len = 0;
	while (1)
	{
		memset(buffer, 0, sizeof(buffer));

		if (ReadDirectoryChangesW(hDir, 
									buffer, 
									sizeof(buffer),
									TRUE, 
									FILE_NOTIFY_CHANGE_FILE_NAME,
									&buf_len, 
									NULL, 
									NULL))
		{
			FILE_NOTIFY_INFORMATION *ptr;

			ptr = (FILE_NOTIFY_INFORMATION *)buffer;
			for (;;)
			{
				WCHAR *lpszPath;

				if (ptr->Action == FILE_ACTION_ADDED)
				{
					int pathlen;

					// 获取整个变化的文件名路径
					pathlen = lstrlen(lpszLogDir);
					pathlen += 1;			// '\'
					pathlen += ptr->FileNameLength + 1;

					lpszPath = (WCHAR *)malloc(pathlen * sizeof(WCHAR));
					if (!lpszPath)
					{
						Log_e("no memory for filename(%d)", ptr->FileNameLength + 1);
						goto next_file;
					}

					memset(lpszPath, 0, pathlen * sizeof(WCHAR));
					MultiByteToWideChar(CP_ACP, 0, lpszLogDir, -1, lpszPath, pathlen);
					
					int n = lstrlenW(lpszPath);
					lpszPath[n++] = '\\';
				
					int i;
					for (i = 0; i < ptr->FileNameLength; i++)
					{
						lpszPath[n + i] = ptr->FileName[i];
					}

					lpszPath[n + i] = '\0';

					int num = WideCharToMultiByte(CP_ACP, 0, lpszPath, -1, NULL, 0, NULL, 0);
					if (num == 0)
					{
						disp_win_sys_err_msg("convert to gb2312 failed!\n");
						free(lpszPath);
						goto next_file;
					}
					char *buff;
					buff = (char *)malloc(num + 1);
					if (!buff)
					{
						free(lpszPath);
						Log_e("no memory for filename(%d)", ptr->FileNameLength + 1);
						goto next_file;
					}
					
					memset(buff, 0, num + 1);
					WideCharToMultiByte(CP_ACP, 0, lpszPath, -1, buff, num, NULL, 0);
					
					Log_d("change filepath=%s", buff);
					CString path = CString(buff);
					if (path.Right(4) == CString(".log"))			// .log结尾
					{
						logfile_map[CString(buff)] = get_current_timestamp_ms();
					}

					free(lpszPath);
					free(buff);
				}
				else if (ptr->Action == FILE_ACTION_REMOVED)
				{

				}
				else if (ptr->Action == FILE_ACTION_MODIFIED)
				{

				}

next_file:
				if (ptr->NextEntryOffset == 0)
				{
					break;
				}

				char *p = (char *)ptr;
				p += ptr->NextEntryOffset;
				ptr = (FILE_NOTIFY_INFORMATION *)p;
			}
		}
		else
		{
			disp_win_sys_err_msg("watch log directory failed!");
		}
	}

	CloseHandle(hDir);
}
