#include "stdafx.h"
#include "afxdisp.h"
#include "registry_pro.h"
#include "homi_common.h"
#include "comdef.h"
#include "qcloud_iot_export_log.h"
#include "mywin.h"

/*
 * 输出当前的时间戳，调试程序运行时间使用
 */
void print_current_timestamp(const char *prompt_str)
{
	SYSTEMTIME tmSys;
	GetLocalTime(&tmSys);
	CTime tm3(tmSys);
	__int64 tmDst = __int64(tm3.GetTime())*1000 + tmSys.wMilliseconds;
	CString sMS;
	_i64toa(tmDst, sMS.GetBuffer(100), 10);
	afxDump << prompt_str <<  " ts: " << sMS << "\n";
	sMS.ReleaseBuffer();
}


/*
 * 获取当前时间戳，ms单位
 */
__int64 get_current_timestamp_ms()
{
	SYSTEMTIME tmSys;

	GetLocalTime(&tmSys);
	CTime tm3(tmSys);

	__int64 tmDst = __int64(tm3.GetTime())*1000 + tmSys.wMilliseconds;

	return tmDst;
}

/*
 * 打印Windows系统错误函数
 */
void disp_win_sys_err_msg(const TCHAR *s)
{
	LPVOID buf;
	CString msg;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &buf,
		0,
		NULL)) 
	{
		msg.Format( _T("%s: %s"), s, buf);
		LocalFree(buf);
	}
	else
	{
		msg.Format(_T("%s: unknown Windows error\n"), s);
	}

	AfxMessageBox(msg);
}

/*
 * c:\\path1\\path2\\abcc.csv ===> c:\\path1\\path2\\abcc_postfix.csv
 */
Boolean append_filename_postfix(LPCTSTR lpszPath, char *new_path, int pathsize, const char *post_fix)
{
	CString full_path(lpszPath);
	CString path;
	CString fname;
	CString newPath;
	int slash;
	int dot;

	slash = full_path.ReverseFind('\\');
	if (slash < 0)
	{
		fname = full_path;
		path = _T("");
	}
	else
	{
		fname = full_path.Right(full_path.GetLength() - slash - 1);
		path = full_path.Left(slash + 1);
	}

	dot = fname.ReverseFind('.');
	if (dot < 0)
	{
		fname += post_fix;
		newPath = path + fname;
	}
	else
	{
		CString name;
		
		name = fname.Left(dot);
		name += post_fix;
		name += fname.Right(fname.GetLength() - dot);

		newPath = path + name;
	}

	if (newPath.GetLength() >= pathsize)
	{
		return FALSE;
	}

	strcpy(new_path, newPath.GetBuffer());
	
	return TRUE;
}


/*
 * c:\\path1\\path2\\abcc.csv ===> c:\\path1\\path2\\
 */
Boolean get_file_path(LPCTSTR lpszPath, char *new_path, int pathsize)
{
	CString full_path(lpszPath);
	CString path;
	int slash;

	slash = full_path.ReverseFind('\\');
	if (slash < 0)
	{
		path = _T("C:\\");
	}
	else
	{
		path = full_path.Left(slash + 1);
	}

	strcpy(new_path, path.GetBuffer());
	
	return TRUE;
}


CString VariantToString(_variant_t var)   
{   
	CString strValue;   
	_variant_t var_t;   
	_bstr_t bstr_t;
	COleCurrency var_currency;   

	switch(var.vt)   
	{   
		case VT_EMPTY:   
		case VT_NULL:
			strValue = _T("");
			break;   

		case VT_UI1:
			strValue.Format("%d",var.bVal);
			break;   //bool   

		case VT_I2:
			strValue.Format("%d",var.iVal);
			break;    //int   

		case VT_I4:
			strValue.Format("%d",var.lVal);
			break;    //long   

		case VT_R4:
			strValue.Format("%f",var.fltVal);
			break;  //float   

		case VT_R8:
			strValue.Format("%f",var.dblVal);
			break;  //   

		case VT_CY:   
			var_currency = var;   
			strValue = var_currency.Format(0);break;   

		case VT_BSTR:   
			var_t = var;   
			bstr_t = var_t;   
			strValue.Format("%s",(const char *)bstr_t);   
			break;   

		case VT_DATE:           //时间类型   
		{   
			CTime  myTime(((COleDateTime)var).GetYear(),    
				((COleDateTime)var).GetMonth(),    
				((COleDateTime)var).GetDay(),    
				((COleDateTime)var).GetHour(),    
				((COleDateTime)var).GetMinute(),    
				((COleDateTime)var).GetSecond());    
			strValue = myTime.Format("%Y-%m-%d %H:%M:%S");   
			break;   
		}   
			
		case VT_BOOL:
			strValue.Format("%d",var.boolVal);   
			break;   

		default:
			strValue=_T("");
			break;   

	}   

	return strValue;   
}


/*
 * 从src中读取一行并返回
 * src: 原始输入行
 * next_line: 下一行开始
 * sep:	分隔符 \n 或者 \r\n , 如果传入NULL, 则系统自己判断
 * return:
 *			本行长度，不包含\r\n 或者 \n
 */
int get_one_line(const char *src, const char **next_line, char *sep)
{
	const char *tmp = NULL;
	const char *p;

	tmp = sep;
	if (!tmp || strlen(sep) == 0)
	{
		tmp = "\r\n";
	}

	p = strstr(src, tmp);
	if (!p)
	{
		tmp = "\n";
		p = strstr(src, tmp);
		if (!p)
		{
			*next_line = NULL;
			return strlen(src);
		}
	}

	if (sep && sep != tmp)
	{
		strcpy(sep, tmp);
	}

	*next_line = p + strlen(tmp);

	return p - src;	
}


/*
 * 处理每一行数据
 * src: 整行buffer, 包含一行或者多行数据
 * fn: 获取到一个完整行, 进行处理的回调函数
 * param1, param2: 回调传递的参数
 * 返回值:
 *		TRUE, 回调已处理; FALSE: 回调未处理
 */
Boolean process_each_line(const char *src, line_callback fn, void *param1, void *param2)
{
	char sep[12];
	const char *next_line = NULL;
	const char *p;
	char *nline;
	int len;
	Boolean ret = FALSE;

	memset(sep, 0, sizeof(sep));
	for (p = src, next_line = src; next_line != NULL; p = next_line)
	{
		len = get_one_line(p, &next_line, sep);
		nline = (char *)malloc(len + 1);
		if (!nline)
		{
			Log_e("no memory!\n");
			break;
		}

		memset(nline, 0, len + 1);
		memcpy(nline, p, len);
		if (fn)
		{
			ret = fn(nline, param1, param2);
		}
		free(nline);

		if (ret)		// 已处理
		{
			break;
		}
	}

	return ret;
}


/*******************************************************************
*作用
       将16进制数字字串转化成值
*参数
     const CString& mac_str - 16进制字串值
*返回值
     8字节整型值
*其它说明
	2018/11/28 by qinjiangwei
********************************************************************/
UINT64 MacString2Binary(const char *mac_str)
{
	int i;
	UCHAR bin_mac[8];
	UINT64 *ptr;

	memset(bin_mac, 0, sizeof(bin_mac));

	if (strlen(mac_str) != 12)
	{
		CString str;

		str.Format(_T("Hex MAC 格式错误(%s)\r\n"), mac_str);
		AfxMessageBox(str);
		return FALSE;
	}

	MyString2HexData(mac_str, &bin_mac[2], 6);

	for (i = 0; i < 4; i++)
	{
		UCHAR tmp;

		tmp = bin_mac[i];
		bin_mac[i] = bin_mac[7 - i];
		bin_mac[7 - i] = tmp;
	}

	ptr = (UINT64 *)bin_mac;

	return *ptr;
}


/*******************************************************************
*作用
       将MAC整数数值转换为16进制字符串
*参数
     UINT64 macbin - 8字节MAC地址数值
*返回值
     char *buff	-	返回的字串
	 TRUE		- 转换成功
	 FALSE		- 转换失败
*其它说明
	2018/11/28 by qinjiangwei
********************************************************************/
Boolean MacBinary2String(UINT64 macbin, char *buff, int len)
{
	CString tmp;
	int i;
	UCHAR buf[8];

	for (i = 7; i >= 0; i--)
	{
		buf[7 - i] = (macbin >> (i * 8)) & 0xff;
	}

	if (len < 13)
	{
		return FALSE;
	}

	tmp.Format("%02X%02X%02X%02X%02X%02X", buf[2],  buf[3],  buf[4],  buf[5],  buf[6],  buf[7]);

	strcpy(buff, tmp.GetBuffer());

	return TRUE;
}