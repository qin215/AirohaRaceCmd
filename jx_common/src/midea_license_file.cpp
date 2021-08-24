#include <stdio.h>

#include "stdafx.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "qcloud_iot_export_log.h"

#include "ServerCmd.h"
#include "homi_common.h"
#include "mywin.h"

/*
 * 将写入成功的MAC地址和ID格式化到buffer中
 */
static void log_mac_use(const user_config_t *pline);

/*
 * added by qinjiangwei 2018/4/11
 * 将license file 文件头作为一个对象来操作, 和以前的程序兼容
 */


typedef Boolean (*fn_parse_line_t)(midea_lic_t *pline, const char *line_buf, int index);

static const int MAX_LIC_NR	= 20 * 1024 + 1;			// MAX 20K

// copy the string from p to dst buffer.
static Boolean copy_subitem(const char *p, char *dst, int len)
{
	int n;

	n = strlen(p);
	if (n + 1 > len)
	{
		Log_e("buff is low!\n");
		return FALSE;
	}

	strcpy(dst, p);

	return TRUE;
}

/*
 * 解析一行格式数据
 */
static Boolean parse_one_line(midea_lic_t *pline, const char *line_buf, int index)
{
	char *ptr = strdup(line_buf);
	char *p = ptr;
	char *sep = ",";
	int i;
	char *ret_ptr;

	p = strtok_s(ptr, sep, &ret_ptr);
	if (!p)
	{
		free(ptr);
		return FALSE;
	}

	for (i = 0; p != NULL; p = strtok_s(NULL, sep, &ret_ptr), i++)
	{
		if (i == 0)
		{
			copy_subitem(p, pline->mac_addr, sizeof(pline->mac_addr));
		}
		else if (i == 1)
		{
			copy_subitem(p, pline->lic_str, sizeof(pline->lic_str));
		}
	}

	free(ptr);
	return TRUE;
}

/*
 * 解析license数据
 */
static Boolean fill_lic_line_user_config(char *buff, midea_lic_t *pline, int n, Boolean used_flag, fn_parse_line_t func)
{
	char *p = buff;
	char *sep = "\r\n";
	int i;
	char *ret_ptr;

	p = strtok_s(p, sep, &ret_ptr);
	if (!p)
	{
		sep = "\n";
		p = strtok_s(p, sep, &ret_ptr);
	}

	if (!p)
	{
		return FALSE;
	}

	for (i = 0; p != NULL; p = strtok_s(NULL, sep, &ret_ptr), i++, pline++)
	{
		if (i >= n)
		{
			break;
		}
		
		func(pline, p, i);
	}

	return TRUE;	
}


/*
 * 读取license文件，txt格式，
 */
midea_lic_t * MideaReadLicenseFile(const TCHAR *lpszPath)
{
	char *buf;
	CFile file;
	int fsize;
	int line_size;
	char *sep = "\r\n";

	if (NULL == file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone))
	{
		AfxMessageBox(_T("打开License文件失败\r\n"));
		return NULL;
	}
	
	fsize = file.GetLength();
	buf = new char[fsize + 1];
	if (!buf)
	{
		afxDump << "no memory!\n";
		return NULL;
	}

	memset(buf, 0, fsize + 1);

	file.Seek(0, CFile::begin);

	if (file.Read(buf, fsize) < fsize)
	{
		AfxMessageBox(_T("读取License文件失败\r\n"));
		delete[] buf;
		file.Close();
		return NULL;
	}
	
	file.Close();
	
	char *p = strstr(buf, sep);
	if (!p)
	{
		sep = "\n";
		p = strstr(buf, sep);
	}

	if (!p)
	{
		delete buf;
		return NULL;
	}


	int len = p - buf;
	if (len >= sizeof(midea_lic_t))
	{
		printf("line is too long, len=%d!\n", len);
		delete buf;
		return NULL;
	}


	line_size = len + strlen(sep);

	int n;
	n = fsize / line_size;
	n++;

	midea_lic_t *pline;

	pline = (midea_lic_t *) malloc(n * sizeof(midea_lic_t));
	if (!pline)
	{
		printf("no memory!\n");
		delete buf;
		return NULL;
	}

	memset(pline, 0, n * sizeof(midea_lic_t));
	
	if (!fill_lic_line_user_config(buf, pline, n, 0, parse_one_line))
	{
		delete buf;
		return NULL;
	}

	delete buf;

	return pline;
}


/*
 * 找到对应美的License的MAC地址对应的记录
 */
const midea_lic_t * MideaFindLineByMac(const midea_lic_t *plines, const char *mac_str)
{
	int i;
	int j;

	if (!mac_str || !plines)
	{
		return NULL;
	}

	if (strlen(mac_str) == 0)
	{
		return NULL;
	}

	for (j = 0, i = 0; strlen(plines->mac_addr) != 0; plines++, j++)
	{
		if (_stricmp(mac_str, plines->mac_addr) == 0)
		{
			return plines;
		}
	}
	
	return NULL;
}


/*
 * 保存License文件特定的行
 * lpszPath: 文件路径
 */
static Boolean midea_save_to_licfile(const TCHAR *lpszPath, midea_lic_t *pline)
{
	char *p;
	int len;
	
	p = (char *)malloc(sizeof(midea_lic_t));
	if (!p)
	{
		Log_e("no memory!\n");
		return FALSE;
	}

	try
	{
		CFile file(lpszPath, CFile::modeWrite | CFile::modeCreate);

		for (; strlen(pline->mac_addr) != 0; pline++)
		{
			len = sprintf(p, "%s,%d,%s\n", pline->mac_addr, pline->count, pline->lic_str);
			file.Write(p, len);
		}

		file.Close();
	}
	catch (CMemoryException* e)
	{
		Log_e("write file no memory failed!\n");
	}
	catch (CFileException* e)
	{
		Log_e("file exception!\n");
	}
	catch (CException* e)
	{
		Log_e("file common exception!\n");
	}

	free(p);
	return TRUE;
}

/*
 * 保存License文件特定的行
 * lpszPath: 文件路径
 * pline: 指向license内存数据
 * index: 记录索引
 */
Boolean MideaSaveLicenseFileIndex(const TCHAR *lpszPath, midea_lic_t *pline, int index)
{
	try 
	{
		CFile myfile(lpszPath, CFile::modeRead);
	}
	catch (CFileException* e)
	{
		Log_d("file %s not exist!\n", lpszPath);
		midea_save_to_licfile(lpszPath, pline);
	}
	
	try
	{
		CFile file(lpszPath, CFile::modeReadWrite);
		long offset;
		char *p;
		int len;

		p = (char *)malloc(sizeof(midea_lic_t));
		if (!p)
		{
			Log_e("no memory!\n");
			return FALSE;
		}

		pline += index;
		len = sprintf(p, "%s,%d,%s\n", pline->mac_addr, pline->count, pline->lic_str);
		offset = index * len;
		
		file.Seek(offset, CFile::begin);
		file.Write((const void *)p, len);
		file.Close();

		free(p);
		
		return TRUE;
	}
	catch (CMemoryException* e)
	{
		Log_e("write file no memory failed!\n");
	}
	catch (CFileException* e)
	{
		Log_e("file exception!\n");
	}
	catch (CException* e)
	{
		Log_e("file common exception!\n");
	}
	
	return FALSE;
}

/*
 * 保存MAC地址记录行，打印二维码使用
 * lpszPath: 文件路径
 * new_mac_str: 新mac地址
 */
Boolean MideaUpdateMacAddress(const TCHAR *lpszPath, const char *new_mac_str)
{
	const char mac_bar_str[] = "3873EAE41A68 2018DP5130,MAC:3873EAE41A68,07SAITU0001810220000010000,051201031839,5.0V,500mA";
	int len = strlen(mac_bar_str);;

	try 
	{
		CFile myfile(lpszPath, CFile::modeRead);
	}
	catch (CFileException* e)
	{
		Log_d("file %s not exist!\n", lpszPath);
		CFile file(lpszPath, CFile::modeWrite | CFile::modeCreate);

		file.Write(mac_bar_str, len);
		file.Close();
	}
	
	try
	{
		CFile file(lpszPath, CFile::modeReadWrite);
		long offset;
		char *p;
		UINT ret;
		char tmp[13];

		p = (char *)malloc(len + 1);
		if (!p)
		{
			Log_e("no memory!\n");
			return FALSE;
		}

		memset(p, 0, len + 1);
		ret = file.Read(p, len);
		if (ret != len)
		{
			CString str;
			Log_e("format error, error=%s, ok=%s!\n",  p, mac_bar_str);
			str.Format(_T("打码格式不对:%s, 正确的为:%s"), p, mac_bar_str);
			AfxMessageBox(str);
			file.Close();
			free(p);
			return FALSE;
		}

		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, p, 12);
		char *ptr = p + 12;

		ptr = strstr(ptr, tmp);
		if (!ptr)
		{
			CString str;
			Log_e("format error, error=%s, ok=%s!\n",  p, mac_bar_str);
			str.Format(_T("打码格式不对:%s, 正确的为:%s"), p, mac_bar_str);
			AfxMessageBox(str);
			file.Close();
			free(p);
			return FALSE;
		}

		memcpy(p, new_mac_str, 12);
		memcpy(ptr, new_mac_str, 12);

		file.Seek(0, CFile::begin);
		file.Write((const void *)p, len);
		file.Close();

		free(p);
		
		return TRUE;
	}
	catch (CMemoryException* e)
	{
		Log_e("write file no memory failed!\n");
	}
	catch (CFileException* e)
	{
		Log_e("file exception!\n");
	}
	catch (CException* e)
	{
		Log_e("file common exception!\n");
	}
	
	return FALSE;
}



/*
 * test stub
 */
void midea_test_lic_file()
{
	midea_lic_t *pline;
	TCHAR path[MAX_PATH];
	TCHAR lpszFilePath[MAX_PATH];

	get_program_path(path, sizeof(path));

	sprintf(lpszFilePath, "%s/a.csv", path);
	if ((pline = MideaReadLicenseFile(lpszFilePath)) != NULL)
	{
		const midea_lic_t *ptr;

		ptr = MideaFindLineByMac(pline, "3873eae3a8c4");
		if (ptr)
		{
			printf("found!\n");
		}
		else
		{
			printf("not found!\n");
		}
		
		free(pline);
	}

}

