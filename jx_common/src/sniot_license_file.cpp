#include <stdio.h>

#include "stdafx.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "qcloud_iot_export_log.h"

#include "ServerCmd.h"
#include "sniot_test_mod.h"
#include "mywin.h"

/*
 * ��д��ɹ���MAC��ַ��ID��ʽ����buffer��
 */
static void log_mac_use(const user_config_t *pline);

/*
 * added by qinjiangwei 2018/4/11
 * ��license file �ļ�ͷ��Ϊһ������������, ����ǰ�ĳ������
 */
 
static const char line_buf[] = "1000,LCO0000000000186,V1.7.6,SNIOT601AGP,1000003C574,NEmEQF5RtLLPZ5ZVTRG9DAXErj7tXP18,2435CC08A11C";

typedef Boolean (*fn_parse_line_t)(user_config_t *pline, const char *line_buf, int index);

static const int MAX_LIC_NR	= 20 * 1024 + 1;			// MAX 20K

/*
 * 1, ���� USED ��־
 * 0, ������ USED ��־
 * -1, ��ʽ����
 */
static int check_used_flag(const char *line)
{
	const char *p;
	int line_size;
	int n;

	p = strrchr(line, ',');
	if (!p)
	{
		printf("line format error! s=%s\n", line);
		return -1;
	}

	if (strlen(p + 1) == SNIOT_MAC_ADDR_LEN)			// last one is mac
	{
		return 0;
	}
	else if (strlen(p + 1) == 1)
	{
		return 1;	
	}
	else
	{
		return -1;
	}
}

/*
 * ��ȡ�Ƿ�ʹ�õı�־
 */
static Boolean get_used_flag(char *line)
{
	char *p;
	int line_size;

	p = strrchr(line, ',');
	if (!p)
	{
		assert("not found ,");
		return FALSE;
	}

	if (strlen(p + 1) != 1)
	{
		assert("not one char!");
		return FALSE;
	}

	return *(p + 1) == '1';
}

/*
 * �������license
 */
static Boolean fill_lic_line_buff(char *buff, sniot_lic_line_t *pline, int n, Boolean used_flag)
{
	char *p = buff;
	char *sep = "\r\n";
	int i;

	p = strtok(p, sep);
	if (!p)
	{
		return FALSE;
	}

	for (i = 0; p != NULL; p = strtok(NULL, sep), i++, pline++)
	{
		if (i < n)
		{
			int len;

			len = strlen(p);
			if (len >= SNIOT_LINE_SIZE)
			{
				printf("line is too long: %d", len);
				return FALSE;
			}

			strcpy(pline->line_buf, p);
			
			if (used_flag)
			{
				pline->used = get_used_flag(p);
			}
			else
			{
				pline->used = FALSE;
			}
		}
	}

	strcpy(pline->line_buf, "");			// ���Ϊ���һ��Ԫ��

	return TRUE;	
}

/*
 * ��ȡLicense�ļ�, ����lint_t����,
 * ��������һ��Ԫ�ص�line_buff����Ϊ0
 */
sniot_lic_line_t * SniotReadLicenseFile(const TCHAR *lpszPath)
{
	char *buf;
	CFile file;
	int fsize;
	int line_size;
	char *sep = "\r\n";

	if (NULL == file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone))
	{
		AfxMessageBox(_T("��License�ļ�ʧ��\r\n"));
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
		AfxMessageBox(_T("��ȡLicense�ļ�ʧ��\r\n"));
		delete[] buf;
		file.Close();
		return NULL;
	}
	
	file.Close();
	
	char *p = strstr(buf, sep);
	if (!p)
	{
		delete buf;
		return NULL;
	}

	sniot_lic_line_t line;

	int len = p - buf;
	if (len >= SNIOT_LINE_SIZE)
	{
		printf("line is too long, len=%d!\n", len);
		delete buf;
		return NULL;
	}

	strncpy(line.line_buf, buf, len);
	line.line_buf[len] = '\0';

	int flag = check_used_flag(line.line_buf);
	if (flag == -1)
	{
		printf("not correct line format! s=%s\n", line.line_buf);
		delete buf;
		return NULL;
	}

	if (flag == 0)			// last one is mac
	{
		line_size = strlen(line_buf) + strlen("\n");
	}
	else
	{
		line_size = strlen(line_buf) + strlen("\n") + 1;		
	}

	int n;
	n = fsize / line_size;
	n++;

	sniot_lic_line_t *pline;

	pline = (sniot_lic_line_t *) malloc(n * sizeof(sniot_lic_line_t));
	if (!pline)
	{
		printf("no memory!\n");
		delete buf;
		return NULL;
	}
	
	if (!fill_lic_line_buff(buf, pline, n, flag == 1))
	{
		delete buf;
		return NULL;
	}

	delete buf;

	return pline;
}


/*
 * ����License�ļ�
 */
Boolean SniotSaveLicenseFile(const TCHAR *lpszPath, sniot_lic_line_t *pline)
{
	FILE *file;

	file = fopen(lpszPath, "wb");
	if (!file)
	{
		afxDump << "open file " << lpszPath << " failed!\n";

		return FALSE;
	}

	while (strlen(pline->line_buf) != 0)
	{
		fprintf(file, "%s,%d\r\n", pline->line_buf, pline->used);
		pline++;
	}

	fclose(file);

	return FALSE;
}


/*
 * ��License File �л�ȡ���е�Mac/Id
 */
sniot_lic_line_t * SniotFindFreeMachAddr(sniot_lic_line_t *pline)
{
	while (strlen(pline->line_buf) != 0)
	{
		if (!pline->used)
		{
			return pline;
		}

		pline++;
	}

	return NULL;
}


/*
 * ���MAC/ID��ʹ��
 */
BOOL SniotUpdateMachFlag(sniot_lic_line_t *plines, sniot_lic_line_t *pline)
{
	while (strlen(plines->line_buf) != 0)
	{
		if (plines == pline)
		{
			pline->used = TRUE;
			break;
		}

		if (strncmp(plines->line_buf, pline->line_buf, strlen(line_buf)) == 0)
		{
			plines->used = TRUE;
			break;
		}

		plines++;
	}

	return TRUE;
}

/*
 * ��license���з���mac string.
 */
const char * sniot_get_mac(sniot_lic_line_t *pline)
{
	int len;
	static char buffer[512];

	len = strlen(pline->line_buf);

	if (len >= sizeof(buffer))
	{
		printf("line is too long: %d", len);
		return NULL;
	}

	strcpy(buffer, pline->line_buf);

	char *p;

	p = strtok(buffer, ",");
	if (!p)
	{
		printf("line format error!\n");
		return NULL;
	}
	
	int i;

	for (i = 0; p != NULL; p = strtok(NULL, ","), i++)
	{
		if (i == SNIOT_MAC_ADDR)		
		{
			return p;
		}
	}

	return NULL;
}

/*
 * ��mac string�ҵ���Ӧ��license��
 * ����ֵ��
 *		NULL: ��ʾδ�ҵ�
 */
sniot_lic_line_t * sniot_find_lic_line(sniot_lic_line_t *plines, const char *mac_str)
{
	if (!plines)
	{
		return NULL;
	}

	while (strlen(plines->line_buf) != 0)
	{
		const char *p = sniot_get_mac(plines);

		if (p && strcmp(p, mac_str) == 0)
		{
			return plines;
		}

		plines++;
	}

	return NULL;
}

/*
* ��ȡlicense��ʹ�����
*/
Boolean sniot_get_license_used_info(sniot_lic_line_t *plines, int *total, int *used)
{
	int count = 0;
	int tmp = 0;

	if (!plines)
	{
		return FALSE;
	}

	while (strlen(plines->line_buf) != 0)
	{
		count++;

		if (plines->used)
		{
			tmp++;
		}

		plines++;
	}

	*total = count;
	*used = tmp;

	return TRUE;
}

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
 * 1,LCO0000000000186,V1.7.6,PCB1.1,PT61C400A,e11fbe7623a1714f02af07f8d95eea68,406A8E002300
 */
static Boolean parse_one_line(user_config_t *pline, const char *line_buf, int index)
{
	char *ptr = strdup(line_buf);
	char *p = ptr;
	char *sep = ",";
	int i;
	int len;
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
			pline->seq = atoi(p);
		}
		else if (i == 1)
		{
			copy_subitem(p, pline->batch_num, sizeof(pline->batch_num));
		}
		else if (i == 2)
		{
			copy_subitem(p, pline->sw_version, sizeof(pline->sw_version));
		}
		else if (i == 3)
		{
			copy_subitem(p, pline->hw_version, sizeof(pline->hw_version));
		}
		else if (i == 4)
		{
			copy_subitem(p, pline->product_type, sizeof(pline->product_type));
		}
		else if (i == 5)
		{
			copy_subitem(p, pline->signature, sizeof(pline->signature));
		}
		else if (i == 6)
		{
			copy_subitem(p, pline->mac_addr, sizeof(pline->mac_addr));
		}
		else if (i == 7)
		{
			copy_subitem(p, pline->device_name, sizeof(pline->device_name));
		}
	}

	free(ptr);
	return TRUE;
}

/*
 * ����license����
 * 1,LCO0000000000186,V1.7.6,PCB1.1,PT61C400A,e11fbe7623a1714f02af07f8d95eea68,406A8E002300
 * ���,���κ�,����汾,Ӳ���汾,��Ʒ�ͺ�,����,MAC��ַ
 * �ִ��ԡ�\0����β
 */
static Boolean fill_lic_line_user_config(char *buff, user_config_t *pline, int n, Boolean used_flag, fn_parse_line_t func)
{
	char *p = buff;
	char *sep = "\r\n";
	int i;
	char *ret_ptr;

	p = strtok_s(p, sep, &ret_ptr);
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
		pline->used = FALSE;
	}

	return TRUE;	
}


/*
 * ��ȡlicense�ļ���txt��ʽ��
 */
user_config_t * PwReadLicenseFile(const TCHAR *lpszPath)
{
	char *buf;
	CFile file;
	int fsize;
	int line_size;
	char *sep = "\r\n";

	if (NULL == file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone))
	{
		AfxMessageBox(_T("��License�ļ�ʧ��\r\n"));
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
		AfxMessageBox(_T("��ȡLicense�ļ�ʧ��\r\n"));
		delete[] buf;
		file.Close();
		return NULL;
	}
	
	file.Close();
	
	char *p = strstr(buf, sep);
	if (!p)
	{
		delete buf;
		return NULL;
	}

	sniot_lic_line_t line;

	int len = p - buf;
	if (len >= SNIOT_LINE_SIZE)
	{
		printf("line is too long, len=%d!\n", len);
		delete buf;
		return NULL;
	}


	line_size = len + strlen("\n");

	int n;
	n = fsize / line_size;
	n++;

	user_config_t *pline;

	pline = (user_config_t *) malloc(n * sizeof(user_config_t));
	if (!pline)
	{
		printf("no memory!\n");
		delete buf;
		return NULL;
	}

	memset(pline, 0, n * sizeof(user_config_t));
	
	if (!fill_lic_line_user_config(buf, pline, n, 0, parse_one_line))
	{
		delete buf;
		return NULL;
	}

	delete buf;

	const char *pmac;
	if ((pmac = PwGetSameMac(pline)))
	{
		CString str;

		str.Format(_T("License IOT2����MAC(%s)��ַ�ظ�\r\n"), pmac);
		AfxMessageBox(str);
	}

	return pline;
}

/*
 * ��ȡlicense�ļ���iot2��ʽ��
 */
user_config_t * PwReadLicenseFileIot2(const TCHAR *lpszPath)
{
	char *buf;
	CFile file;
	int fsize;
	user_config_t *pline = NULL;
	int mem_len; 

	if (NULL == file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone))
	{
		AfxMessageBox(_T("��License�ļ�ʧ��\r\n"));
		return NULL;
	}
	
	fsize = file.GetLength();
	buf = (char *)malloc(fsize + 1 + sizeof(user_config_t));		// �����һ��user_config_t ��Ϊ��β��־ 
	if (!buf)
	{
		afxDump << "no memory!\n";
		return NULL;
	}

	memset(buf, 0, fsize + 1 + sizeof(user_config_t));
	file.Seek(0, CFile::begin);

	if (file.Read(buf, fsize) < fsize)
	{
		AfxMessageBox(_T("��ȡLicense�ļ�ʧ��\r\n"));
		free(buf);
		file.Close();
		return NULL;
	}
	
	file.Close();

	pline = (user_config_t *) buf;
	if (fsize % sizeof(user_config_t) != 0)
	{
		AfxMessageBox(_T("License IOT2�ļ���ʽ����\r\n"));
		free(buf);
		return NULL;
	}

#if 0			// �ӿ��ٶȣ������mac��ַ�Ƿ��ظ�
	const char *pmac;
	if ((pmac = PwGetSameMac(pline)))
	{
		CString str;

		str.Format(_T("License IOT2����MAC(%s)��ַ�ظ�\r\n"), pmac);
		AfxMessageBox(str);
	}
#endif

	return pline;
}

/*
 * ��ȡlicense�ļ�����ͬ��MAC��ַ
 */
const char * PwGetSameMac(const user_config_t *pline)
{
	const user_config_t *p;

	for (; pline->seq != 0; pline++)
	{
		int len;

		len = strlen(pline->mac_addr);		// �����mac��ַ���µ�����

		for (p = pline + 1; p->seq != 0; p++)
		{
			if (len > 0 && strcmp(p->mac_addr, pline->mac_addr) == 0)
			{
				return p->mac_addr;
			}
		}
	}

	return NULL;
}


/*
 * ���license�ļ����Ƿ������ͬ��MAC��ַ
 */
Boolean PwCheckSameMac(const user_config_t *pline)
{
	const user_config_t *p;

	for (; pline->seq != 0; pline++)
	{
		for (p = pline + 1; p->seq != 0; p++)
		{
			if (strcmp(p->mac_addr, pline->mac_addr) == 0)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*
 * ����License�ļ�
 */
Boolean PwSaveLicenseFile(const TCHAR *lpszPath, user_config_t *pline)
{
	FILE *file;

	file = fopen(lpszPath, "wb");
	if (!file)
	{
		afxDump << "open file " << lpszPath << " failed!\n";

		return FALSE;
	}

	while (pline->seq != 0)
	{
		if (fwrite(pline, sizeof(user_config_t), 1, file) != 1)
		{
			Log_e("write file failed!\n");
		}

		pline++;
	}

	fclose(file);

	return TRUE;
}

/*
 * ����License�ļ��ض�����
 * lpszPath: �ļ�·��
 * pline: ָ��license�ڴ�����
 * index: ��¼����
 */
Boolean PwSaveLicenseFileIndex(const TCHAR *lpszPath, user_config_t *pline, int index)
{
#if 1
	try
	{
		CFile file(lpszPath, CFile::modeReadWrite);
		long offset;
	
		offset = index * sizeof(user_config_t);
		pline += index;
		file.Seek(offset, CFile::begin);
		file.Write((const void *)pline, sizeof(user_config_t));
		file.Close();
		
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
#else
	FILE *file;
	off_t offset;
	char *tmp = "1234567890";
	
	file = fopen(lpszPath, "wb+");
	if (!file)
	{
		afxDump << "open file " << lpszPath << " failed!\n";
		Log_e("open file(%s) failed!\n", lpszPath);
		return FALSE;
	}

	if (fseek(file, index * sizeof(user_config_t), SEEK_SET) < 0)
	{
		Log_e("fseek error!\n");
	}
	pline += index;

	offset = ftell(file);
	afxDump << "offset = " << offset << "\n";

	if (fwrite(/*pline*/tmp, sizeof(user_config_t), 1, file) != 1)
	{
		Log_e("write file failed!\n");
	}

	fclose(file);

	return TRUE;
#endif
}

/*
 * ��ȡ��һ��δʹ�õ�MAC��ַ
 */
const char *PwGetFirstUnusedMac(const user_config_t *pline)
{
	while (pline->seq != 0)
	{
		if (!pline->used)
		{
			return pline->mac_addr;
		}

		pline++;
	}

	return NULL;
}

/*
 * ��ȡ��һ��MAC��ַ
 */
const char *PwGetFirstMac(const user_config_t *pline)
{
	if (pline->seq != 0)
	{
		return pline->mac_addr;
	}

	return NULL;
}


/*
 * ��ȡ���һ��δʹ�õ�MAC��ַ
 */
const char *PwGetLastUnusedMac(const user_config_t *pline)
{
	const char *pmac = NULL;

	while (pline->seq != 0)
	{
		if (!pline->used)
		{
			pmac = pline->mac_addr;
		}

		pline++;
	}

	return pmac;
}

/*
 * ��ȡ���һ��MAC��ַ
 */
const char *PwGetLastMac(const user_config_t *pline)
{
	const char *pmac = NULL;

	while (pline->seq != 0)
	{
		pmac = pline->mac_addr;
		pline++;
	}

	return pmac;
}

/*
* ����δʹ�õ�mac��ַ����
*/
int PwGetUnusedNumber(const user_config_t *pline)
{
	int nr = 0;

	for (; pline->seq != 0; pline++)
	{
		if (!pline->used)
		{
			nr++;
		}
	}

	return nr;
}

/*
* �������е�mac��ַ����
*/
int PwGetTotalNumber(const user_config_t *pline)
{
	int nr = 0;

	for (; pline->seq != 0; pline++)
	{
		nr++;

		if (nr >= MAX_LIC_NR)			// max 20k 
		{
			return -1;
		}
	}

	return nr;
}



/*
 * ��License File �л�ȡ���е�Mac/Id
 * ���������
 *	pline: license ����
 *  paddr: �洢���ص�����
 *  n : ��Ҫ���صĸ���
 * ����ֵ��
 *	��Ч�����ݸ���
 */
int PwFindFreeMachAddr(user_config_t *pline, user_config_mgmt_t *paddr, int n)
{
	int i;
	int j;

	for (j = 0, i = 0; pline->seq != 0; pline++, j++)
	{
		if (i >= n)
		{
			break;
		}

		if (!pline->used)
		{
			memcpy(&paddr[i].config, pline, sizeof(user_config_t));
			paddr[i].index = j;
			i++;
		}
	}

	return i;
}

/*
 * ��ȡ��һ��δʹ�õ�MAC��ַ��¼, current Ϊ��ǰָ��
 */
const user_config_t *PwGetFirstUnusedFast(const user_config_t *pline, const user_config_t *pcurrent)
{
	const user_config_t *ptr = pcurrent;

	if (!pcurrent)		// ��һ��
	{
		// ��ͷ��ʼ����
		for (ptr = pline; ptr->seq != 0; ptr++)
		{
			if (!ptr->used)
			{
				return ptr;
			}
		}

		return NULL;
	}

	while (ptr->seq != 0)
	{
		if (!ptr->used)
		{
			return ptr;
		}

		ptr++;
	}

	// ��ͷ��ʼ����
	for (ptr = pline; ptr != pcurrent; ptr++)
	{
		if (!ptr->used)
		{
			return ptr;
		}
	}

	return NULL;
}


/*
 * ��License File �л�ȡ���е�Mac/Id
 * ���������
 *	pline: license ����
 *  paddr: �洢���ص�����
 *  n : ��Ҫ���صĸ���
 * ����ֵ��
 *	��Ч�����ݸ���
 */
int PwFindFreeMachAddrFast(user_config_t *pline, const user_config_t *pcurrent, user_config_mgmt_t *paddr, int n)
{
	int i;
	const user_config_t *ptr = pcurrent;

	for (i = 0; ptr->seq != 0; ptr++)
	{
		if (i >= n)
		{
			break;
		}

		if (!ptr->used)
		{
			memcpy(&paddr[i].config, ptr, sizeof(user_config_t));
			paddr[i].index = ptr - pline;
			i++;
		}
	}

	// ��ͷ��ʼ����
	if (i < n)
	{
		ptr = pline;
		for (; ptr != pcurrent; ptr++)
		{
			if (i >= n)
			{
				break;
			}

			if (!ptr->used)
			{
				memcpy(&paddr[i].config, ptr, sizeof(user_config_t));
				paddr[i].index = ptr - pline;
				i++;
			}
		}
	}

	return i;
}


/*
 * ��HEX��ʽ��MAC��ַתΪ������
 */
Boolean PwHexMac2Binary(const char *pmac, kal_uint8 bin_mac[6])
{
	int i;

	if (strlen(pmac) != 12)
	{
		CString str;

		str.Format(_T("Hex MAC ��ʽ����(%s)\r\n"), pmac);
		AfxMessageBox(str);
		return FALSE;
	}

	CString mac_str = pmac;
	return MyString2HexData(pmac, bin_mac, 6);
}

/*
 * �������Ƶ�MACת��ΪHEX��ʽ
 */
Boolean PwMacBin2Hex(char *pmac, int len, const kal_uint8 bin_mac[6])
{
	if (len <= 12)
	{
		CString str;

		str.Format(_T("Hex(%d) ������ƫС\r\n"), len);
		AfxMessageBox(str);
		return FALSE;
	}

	sprintf(pmac, "%02X%02X%02X%02X%02X%02X", bin_mac[0], bin_mac[1], bin_mac[2], bin_mac[3], bin_mac[4], bin_mac[5]);
	return TRUE;
}


/*
 * ���MAC/ID��ʹ��
 */
BOOL PwUpdateMachFlag(user_config_t *pline, user_config_mgmt_t *paddr)
{
	int index;
	user_config_t *p;
	int n;

	index = paddr->index;

	n = PwGetTotalNumber(pline);			// ע��Ч�ʣ������Ż�
	if (index >= n)
	{
		Log_e("update used flag index(%d) is greater than n(%d)", index, n);
		return FALSE;
	}

	p = &pline[index];
	p->used = TRUE;

	log_mac_use(p);

	return TRUE;
}

/*
 * ��д��ɹ���MAC��ַ��ID��ʽ����buffer��
 */
static void log_mac_use(const user_config_t *pline)
{
	char buffer[256];
	
	sprintf(buffer, "use mac:%\t", pline->mac_addr);

	Log_d("%s", buffer);
}

// д�밢����Ȩ�ļ�
/*
 * ��ȡlicense�ļ���txt��ʽ��
 */

/*
 * product type,device name,device secret
 */
static Boolean alios_parse_one_line(user_config_t *pline, const char *line_buf, int index)
{
	char *ptr = strdup(line_buf);
	char *p = ptr;
	char *sep = ",";
	int i;
	int len;
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
			copy_subitem(p, pline->product_type, sizeof(pline->product_type));
		}
		else if (i == 1)
		{
			copy_subitem(p, pline->device_name, sizeof(pline->device_name));
		}
		else if (i == 2)
		{
			copy_subitem(p, pline->signature, sizeof(pline->signature));
		}
	}

	pline->seq = index + 1;

	free(ptr);
	return TRUE;
}

/*
 * ���밢��OS��Ȩ���ñ�
 */
user_config_t * ReadAliosLicenseFile(const TCHAR *lpszPath)
{
	char *buf;
	CFile file;
	int fsize;
	int line_size;
	char *sep = "\r\n";

	if (NULL == file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone))
	{
		AfxMessageBox(_T("��Alios License�ļ�ʧ��\r\n"));
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
		AfxMessageBox(_T("��ȡLicense�ļ�ʧ��\r\n"));
		delete[] buf;
		file.Close();
		return NULL;
	}
	
	file.Close();
	
	char *p = strstr(buf, sep);
	if (!p)
	{
		delete buf;
		return NULL;
	}

	sniot_lic_line_t line;

	int len = p - buf;
	if (len >= SNIOT_LINE_SIZE)
	{
		printf("line is too long, len=%d!\n", len);
		delete buf;
		return NULL;
	}

	line_size = len + strlen("\n");

	int n;
	n = fsize / line_size;
	n++;

	user_config_t *pline;

	pline = (user_config_t *) malloc(n * sizeof(user_config_t));
	if (!pline)
	{
		printf("no memory!\n");
		delete buf;
		return NULL;
	}

	memset(pline, 0, n * sizeof(user_config_t));
	
	if (!fill_lic_line_user_config(buf, pline, n, 0, alios_parse_one_line))
	{
		delete buf;
		return NULL;
	}

	delete buf;

	const char *pmac;
	if ((pmac = PwGetSameMac(pline)))
	{
		CString str;

		str.Format(_T("License IOT2����MAC(%s)��ַ�ظ�\r\n"), pmac);
		AfxMessageBox(str);
	}

	return pline;
}

/*
 * ��ʼ��ַΪpmac����n����ַ���µ�user_config_t��
 */
Boolean AliosFillMacAddress(user_config_t *pline, kal_uint8 pmac[6], int n)
{
	int count;
	CString str;
	__int64 addr;

	count = PwGetTotalNumber(pline);
	if (n > count)
	{
		Log_e("n (%d) is more than count(%d)\n", n, count);
		str.Format(_T("�����MAC��ַ(%d)�������������ļ�����(%d)\r\n"), n, count);
		AfxMessageBox(str);

		n = count;
	}

	memset(&addr, 0, sizeof(addr));
	kal_uint8 *ptr = (kal_uint8 *)&addr;
	
	int i;
	for (i = 0; i < 6; i++)
	{
		ptr[i] = pmac[5 - i];
	}

	for (i = 0; i < n; i++, pline++, addr++)
	{
		kal_uint8 tmp[6];
		int j;

		for (j = 0; j < 6; j++)
		{
			tmp[j] = ptr[5 - j];
		}

		PwMacBin2Hex(pline->mac_addr, sizeof(pline->mac_addr), tmp);
	}

	return TRUE;
}

/*
 * �ҵ���Ӧ��mac/name/secret��¼
 */
const user_config_t * AliosFindLineByMac(const user_config_t *plines, const char *mac_str)
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

	for (j = 0, i = 0; plines->seq != 0; plines++, j++)
	{
		if (strcmp(mac_str, plines->mac_addr) == 0)
		{
			return plines;
		}
	}
	
	return NULL;
}

/*
 * �ҵ���Ӧ��mac/name/secret��¼
 * ��������
 */
int AliosFindLineByMacIndex(const user_config_t *plines, const char *mac_str)
{
	int i;
	int j;

	if (!mac_str || !plines)
	{
		return -1;
	}

	if (strlen(mac_str) == 0)
	{
		return -1;
	}

	for (j = 0, i = 0; plines->seq != 0; plines++, j++)
	{
		if (strcmp(mac_str, plines->mac_addr) == 0)
		{
			return j;
		}
	}
	
	return -1;
}

/*
 * HEX������תΪ������
 * ���磺
 * 3873EAE42DD9 ==> D92DE4EA7338
 */
static void HexNetwork2Host(const kal_uint8 *psrc, kal_uint8 *pdst, int n)
{
	int i;

	for (i = 0; i < n; i += 2)
	{
		pdst[n - i - 2] = psrc[i];
		pdst[n - i - 1] = psrc[i + 1];
	}
}

/*
 * ���ֲ��ұȽϺ���
 * a : item 1
 * b : item 2
 * ����ֵ:
 * > 0 : a->key > b->key
 * == 0 : a->key == b->key
 * < 0  : a->key < b->key
 */
static int compare_func(const void *a, const void *b)
{
	const user_config_t *item1 = (const user_config_t *)a;
	const user_config_t *item2 = (const user_config_t *)b;
	__int64 x = 0;
	__int64 y = 0;
	kal_uint8 mac1[MAC_ADDRESS_LEN] = { 0 };
	kal_uint8 mac2[MAC_ADDRESS_LEN] = { 0 };

	HexNetwork2Host((const kal_uint8 *)item1->mac_addr, mac1, MAC_ADDRESS_LEN - 1);
	HexNetwork2Host((const kal_uint8 *)item2->mac_addr, mac2, MAC_ADDRESS_LEN - 1);

	PwHexMac2Binary((const char *)mac1, (kal_uint8 *)&x);
	PwHexMac2Binary((const char *)mac2, (kal_uint8 *)&y);

	return x - y;
}

/*
 * �ҵ���Ӧ��mac/name/secret��¼
 * ���ֲ��ң��ӿ��ٶ�
 */
const user_config_t * AliosBinaryFindLineByMac(const user_config_t *plines, int n, const char *mac_str)
{
	user_config_t key;
	user_config_t *ret;

	if (!mac_str || !plines)
	{
		return NULL;
	}

	if (strlen(mac_str) == 0)
	{
		return NULL;
	}

	if (n <= 0)
	{
		return NULL;
	}

	memset(&key, 0, sizeof(key));
	strncpy(key.mac_addr, mac_str, MAC_ADDRESS_LEN - 1);

	ret = (user_config_t *)bsearch(&key, plines, n, sizeof(user_config_t), compare_func);

	return ret;
}



/*
 * test stub
 */
void sniot_test_lic_file()
{
	user_config_t *pline;
	TCHAR path[MAX_PATH];
	TCHAR lpszFilePath[MAX_PATH];

	get_program_path(path, sizeof(path));

	sprintf(lpszFilePath, "%s/a.txt", path);

#if 0
	pline = SniotReadLicenseFile(lpszFilePath);
	sniot_find_lic_line(pline, "2435CC08A115");
	sprintf(lpszFilePath, "%s/b.txt", path);
	SniotSaveLicenseFile(lpszFilePath, pline);
	free(pline);
#endif

	pline = PwReadLicenseFile(lpszFilePath);
	sprintf(lpszFilePath, "%s/b.txt", path);
	PwSaveLicenseFile(lpszFilePath, pline);
	free(pline);
}

