#include <stdio.h>

#include "stdafx.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ServerCmd.h"
#include "sniot_test_mod.h"
#include "mywin.h"

/*
 * added by qinjiangwei 2018/4/11
 * ��license file �ļ�ͷ��Ϊһ������������, ����ǰ�ĳ������
 */
 
static const char line_buf[] = "1000,LCO0000000000186,V1.7.6,SNIOT601AGP,1000003C574,NEmEQF5RtLLPZ5ZVTRG9DAXErj7tXP18,2435CC08A11C";

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

/*
 * test stub
 */
void sniot_test_lic_file()
{
	sniot_lic_line_t *pline;
	TCHAR path[MAX_PATH];
	TCHAR lpszFilePath[MAX_PATH];

	get_program_path(path, sizeof(path));

	sprintf(lpszFilePath, "%s/a.txt", path);

	pline = SniotReadLicenseFile(lpszFilePath);

	sniot_find_lic_line(pline, "2435CC08A115");

	sprintf(lpszFilePath, "%s/b.txt", path);
	SniotSaveLicenseFile(lpszFilePath, pline);

	free(pline);
}

