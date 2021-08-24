#include "stdafx.h"
#include "homi_common.h"
#include "qcloud_iot_export_log.h"
#include "mywin.h"

/*
 * added by qinjiangwei 2018/4/11
 * ��license file �ļ�ͷ��Ϊһ������������, ����ǰ�ĳ������
 */
 /*
 * ��д��ɹ���MAC��ַ��ID��ʽ����buffer��
 */
static void log_mac_id_use( const macaddr_id_t *pmac_addr);

/*
 * ��ȡLicense�ļ�
 */
lic_file_header_t * ReadLicenseFileOld(const TCHAR *lpszPath, int *filelen)
{
	unsigned char *buf;
	CFile file;
	int fsize;
	
	if (NULL == file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone))
	{
		AfxMessageBox(_T("��License�ļ�ʧ��\r\n"));
		return NULL;
	}
	
	fsize = file.GetLength();
	buf = new unsigned char[fsize];
	if (!buf)
	{
		afxDump << "no memory!\n";
		return NULL;
	}

	file.Seek(0, CFile::begin);

	if (file.Read(buf, fsize) < fsize)
	{
		AfxMessageBox(_T("��ȡLicense�ļ�ʧ��\r\n"));
		free(buf);
		file.Close();
		return NULL;
	}
	
	lic_file_header_t *pheader = (lic_file_header_t *)buf;
	int checksum = 0;

	for (int i = 4; i < fsize; i++)
	{
	 	checksum += buf[i];
	}

	int sum = (pheader->chksum[0] << 24) | (pheader->chksum[1] << 16) | (pheader->chksum[2] << 8) | (pheader->chksum[3]);
	if (checksum != sum)
	{
		AfxMessageBox(_T("LicenseУ��ʧ��\r\n"));
		free(buf);
		file.Close();
		return NULL;
	}

	file.Close();
	*filelen = fsize;
	return pheader;
}


/*
 * ����License�ļ�
 */
BOOL SaveLicenseFile(const TCHAR *lpszPath, __int64 start_addr, int write_num, kal_uint32 start_id)
{
	unsigned char *buf = new unsigned char[MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE];
	FILE* file;
	UCHAR temp_buf[6];
	BOOL ret = TRUE;
	int i;
	
	if (!buf)
	{
		afxDump << "no memory!\n";
		return FALSE;
	}

	file = fopen(lpszPath, "wb");
	if (!file)
	{
		afxDump << "open file " << lpszPath << " failed!\n";
		free(buf);

		return FALSE;
	}
	
	memset(buf, 0, MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE);

	temp_buf[0] =(unsigned char)(start_addr>>40);
    temp_buf[1] =(unsigned char)(start_addr>>32);
    temp_buf[2] =(unsigned char)(start_addr>>24);
    temp_buf[3] =(unsigned char)(start_addr>>16);
    temp_buf[4] =(unsigned char)(start_addr>>8);
    temp_buf[5] =(unsigned char)(start_addr>>0);

	if (temp_buf[0] & 0x1)			//  ���һBITΪ1, Ϊ�鲥��ַ���Ƿ�
	{
		AfxMessageBox("MAC ��ַ���Ϸ�!");
		fclose(file);
		free(buf);

		return FALSE;
	}

	lic_file_header_t *pheader = (lic_file_header_t *)buf;

	memcpy(&pheader->mac_start_addr[0], temp_buf, 6); //��ʼ��
	memcpy(&pheader->mac_current_addr[0], temp_buf, 6); //��ʼ��
	
	//ǰ��4���ֽ���У����
//	memcpy(&buf[4], temp_buf, 6); //��ʼ��
//	memcpy(&buf[32], temp_buf, 6); //�Ѿ���¼�����һ����ַ
		
	__int64 end_addr = start_addr + write_num;
	temp_buf[0] =(unsigned char)(end_addr>>40);
    temp_buf[1] =(unsigned char)(end_addr>>32);
    temp_buf[2] =(unsigned char)(end_addr>>24);
    temp_buf[3] =(unsigned char)(end_addr>>16);
    temp_buf[4] =(unsigned char)(end_addr>>8);
    temp_buf[5] =(unsigned char)(end_addr>>0);
	
//	memcpy(&buf[10], temp_buf, 6);  //������
	memcpy(&pheader->mac_end_addr[0], temp_buf, 6); //��ʼ��
	
	temp_buf[0] = (unsigned char)(write_num>>24);  //��¼����
	temp_buf[1] = (unsigned char)(write_num>>16);
	temp_buf[2] = (unsigned char)(write_num>>8);
	temp_buf[3] = (unsigned char)(write_num>>0);
	
	memcpy(&pheader->mac_count[0], temp_buf, 4); //��ʼ��

	pheader->id_start = start_id;
	pheader->id_end = start_id + write_num;
	pheader->current_id = start_id;
	
	TO_NW_INT(write_num, pheader->left_count);
	
	for (i = 0;i < write_num;i++)
	{
		temp_buf[0] =(unsigned char)(start_addr>>40);
		temp_buf[1] =(unsigned char)(start_addr>>32);
		temp_buf[2] =(unsigned char)(start_addr>>24);
		temp_buf[3] =(unsigned char)(start_addr>>16);
		temp_buf[4] =(unsigned char)(start_addr>>8);
		temp_buf[5] =(unsigned char)(start_addr>>0);
		
		int offset = MP_ADDRESS_OFFSET + i * MP_DATA_SIZE;

		for (int j = 0; j < MAC_ADDR_SIZE; j++)
		{
		    buf[offset++] = temp_buf[j];
		}

		kal_uint32 *pid = (kal_uint32 *)&buf[offset + 1];		// ����Flag��־

		*pid = start_id;
		
		start_addr++;
		start_id++;
	}

	int checksum = 0;
	
	for (i = 4; i < MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE; i++)
	{
	 	checksum += buf[i];
	}

	temp_buf[0] = (unsigned char)(checksum>>24);  //������
	temp_buf[1] = (unsigned char)(checksum>>16);
	temp_buf[2] = (unsigned char)(checksum>>8);
	temp_buf[3] = (unsigned char)(checksum>>0);
	memcpy(&pheader->chksum[0], temp_buf, 4); //��ʼ��
	
	if (fwrite(buf, MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE, 1, file) != 1)
	{
		afxDump << "write file " << lpszPath << " failed!\n";
		ret = FALSE;
	}

	fclose(file);
	free(buf);

	return ret;
}


/*
 * ��License File �л�ȡ���е�Mac/Id
 */
int FindFreeMachAddrOld(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr, int n)
{
	lic_file_header_t *pheader = (lic_file_header_t *)pbuff;
	macaddr_id_t *ptr;
	int i;
	int j;
	kal_uint8 *end_ptr = (pbuff + len);
	
	ptr = (macaddr_id_t *)(pbuff + MP_ADDRESS_OFFSET);

	for (i = 0, j = 0; i < n; i++)
	{
		macaddr_id_t *p;
		char mac_str[13];

		for (; ; j++)
		{
			p = &ptr[j];

			if ((kal_uint8 *)p >= end_ptr)
			{
				return i;
			}

			if (!p->mac_flag)
			{
				memset(mac_str, 0, sizeof(mac_str));
				PwMacBin2Hex(mac_str, sizeof(mac_str), p->mac_addr);

				if (check_download_mac_used(mac_str) && \
					database_get_error_code() != DATABASE_CODE_NOT_OPEN)
				{
					CString str;

					str.Format(_T("����MAC(%s)ʹ�ú����ݲ�һ��!"), mac_str);
					AfxMessageBox(str);
					p->mac_flag = TRUE;
					continue;
				}

				memcpy(&paddr[i].addr, p, sizeof(macaddr_id_t));
				paddr[i].index = j;
				j++;
				break;
			}
		}
	}

	return i;
}


/*******************************************************************
*����
       License File �л�ȡ���е�Mac/Id, �����ݿ��бȶԣ��������ʹ�ü�¼������±��ؼ�¼
*����
	 UCHAR *pbuff - ��������ַ
	 INT len - ����������
	 macaddr_mgmt_t *paddr - ���ص�mac��ַ
	 int n - ����Ҫ�ĵ�ַ����
	 const TCHAR *lpszPath - �ļ�·��
*����ֵ
     INT - ʵ�ʿ���ʹ�õ�mac��ַ����
*����˵��
	2018/11/5 by qinjiangwei
********************************************************************/
int FindFreeMachAddrOld2(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr, int n, const TCHAR *lpszPath)
{
	lic_file_header_t *pheader = (lic_file_header_t *)pbuff;
	macaddr_id_t *ptr;
	int i;
	int j;
	kal_uint8 *end_ptr = (pbuff + len);
	
	ptr = (macaddr_id_t *)(pbuff + MP_ADDRESS_OFFSET);

	for (i = 0, j = 0; i < n; i++)
	{
		macaddr_id_t *p;
		char mac_str[13];

		for (; ; j++)
		{
			p = &ptr[j];

			if ((kal_uint8 *)p >= end_ptr)
			{
				return i;
			}

			if (!p->mac_flag)
			{
				memset(mac_str, 0, sizeof(mac_str));
				PwMacBin2Hex(mac_str, sizeof(mac_str), p->mac_addr);

				if (check_download_mac_used(mac_str) && \
					database_get_error_code() != DATABASE_CODE_NOT_OPEN)
				{
					CString str;

					str.Format(_T("����MAC(%s)ʹ�ú����ݲ�һ��!"), mac_str);
					AfxMessageBox(str);
					p->mac_flag = TRUE;
					SaveLicenseFile2(lpszPath, (kal_uint8 *)pbuff, len);
					continue;
				}

				memcpy(&paddr[i].addr, p, sizeof(macaddr_id_t));
				paddr[i].index = j;
				j++;
				break;
			}
		}
	}

	return i;
}


/*
 * ���MAC/ID��ʹ��
 */
BOOL UpdateMachFlag(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr)
{
	lic_file_header_t *pheader = (lic_file_header_t *)pbuff;
	macaddr_id_t *ptr;
	int index;
	kal_uint8 *end_ptr = (pbuff + len);
	macaddr_id_t *p;
	char mac_str[13];
		
	ptr = (macaddr_id_t *)(pbuff  + MP_ADDRESS_OFFSET);

	index = paddr->index;

	if ((kal_uint8 *)&ptr[index] >= (kal_uint8 *)(pbuff + len))
	{
		return FALSE;
	}

	p = &ptr[index];
	p->mac_flag = TRUE;

	memset(mac_str, 0, sizeof(mac_str));
	PwMacBin2Hex(mac_str, sizeof(mac_str), paddr->addr.mac_addr);
	
	update_download_db_record(mac_str, 1);

	int count = TO_INT(pheader->left_count);
	count--;
	TO_NW_INT(count, pheader->left_count);

	log_mac_id_use(p);

	return TRUE;
}


/*
 * ��д��ɹ���MAC��ַ��ID��ʽ����buffer��
 */
static void log_mac_id_use( const macaddr_id_t *pmac_addr)
{
	int len;
	char buffer[256];
	const kal_uint8 *p;
#define MAC_ADDR_FORMAT		"%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_ADDR_DATA(__p)	__p[0], __p[1], __p[2], __p[3], __p[4], __p[5]
#define MYID_FORMAT	"%08X"
	
	p = pmac_addr->mac_addr;
	len = _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "use mac:"MAC_ADDR_FORMAT"\t", MAC_ADDR_DATA(p));
#if HOMI_ID_SIZE 
	len += _snprintf_s(&buffer[len], sizeof(buffer) - len, _TRUNCATE, "use id:"MYID_FORMAT"\n", ID_TO_UINT32(pmac_addr->id));
#endif

	Log_d("%s", buffer);
}


/*
 * ���¼���checksum, ����license
 */
BOOL SaveLicenseFile2(const TCHAR *lpszPath, kal_uint8 *pbuff, int len)
{
	unsigned char *buf = pbuff;
	FILE* file;
	UCHAR temp_buf[6];
	BOOL ret = TRUE;
	int i;
	int write_num;
	
	file = fopen(lpszPath, "wb");
	if (!file)
	{
		afxDump << "open file " << lpszPath << " failed!\n";

		return FALSE;
	}
	
	if (!buf)
	{
		afxDump << "ptr null!\n";
		fclose(file);
		return FALSE;
	}

	lic_file_header_t *pheader = (lic_file_header_t *)buf;

	write_num = TO_INT(pheader->mac_count);

	if (MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE != len)
	{
		afxDump << "Len = " << len << " isnot equal " << MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE;
		return FALSE;
	}

	int checksum = 0;
	
	for (i = 4; i < MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE; i++)
	{
	 	checksum += buf[i];
	}

	temp_buf[0] = (unsigned char)(checksum>>24);  	// ������
	temp_buf[1] = (unsigned char)(checksum>>16);
	temp_buf[2] = (unsigned char)(checksum>>8);
	temp_buf[3] = (unsigned char)(checksum>>0);
	memcpy(&pheader->chksum[0], temp_buf, 4); 		// ��ʼ��
	
	if (fwrite(buf, MP_ADDRESS_OFFSET + write_num * MP_DATA_SIZE, 1, file) != 1)
	{
		afxDump << "write file " << lpszPath << " failed!\n";
		ret = FALSE;
	}

	fclose(file);

	return ret;
}

/* 
 * ���License�ļ����ݵ�һ����
 * ����ֵ:
 * 1 : ��Ϣ��ȷ
 * 0 : ��Ϣ����ȷ�����Զ�����
 * -1 : ��Ϣ����ȷ��û���Զ�����
 */
int CheckLicenseFile(const TCHAR *lpszPath)
{
	lic_file_header_t *pheader;
	int fsize;
	macaddr_id_t *ptr;
	macaddr_id_t *p;
	int j;
	int free_count;
	int ret = 1;
	int total_count = 0;
	
	pheader = ReadLicenseFileOld(lpszPath, &fsize);
	if (!pheader)
	{
		afxDump << "can not open file " << lpszPath << " failed!\n";
		return -1;
	}

	kal_uint8 *end_ptr = ((kal_uint8 *)pheader + fsize);
	
	ptr = (macaddr_id_t *)((kal_uint8 *)pheader + MP_ADDRESS_OFFSET);
	for (free_count = 0, j = 0; ; j++)
	{
		p = &ptr[j];
		if (!p->mac_flag)
		{
			free_count++;
		}
		
		total_count++;
		if ((kal_uint8 *)(p + 1) >= end_ptr)
		{
			break;
		}
	}

	afxDump << "free_count = " << free_count << ", total = " << total_count << "\n";

	int c = TO_INT(pheader->mac_count);
	int f = TO_INT(pheader->left_count);

	if (c != total_count)
	{
		afxDump << "license file Failed!\n";
		
		ret = -1;
		goto done;
	}

	if (f != free_count)
	{
		ret = 0;
	}

	if (ret == 0)
	{
		afxDump << "c = " << c << ", f = " << f << "\n";

		TO_NW_INT(free_count, pheader->left_count);
		SaveLicenseFile2(lpszPath, (kal_uint8 *)pheader, fsize);
	}

	delete pheader;
	
done:
	return ret;
}


/*
 * ��ȡMAC��ַ��Ӧ��IDֵ��ʹ�ñ�־
 *
 * ������
 *		pubff: license�ļ����ڴ��еĿ�ʼ��ַ
 *		len: �ڴ泤��
 *      paddr->addr.mac_addr�� ��Ҫ���ҵ�MAC��ַ
 * ����ֵ��
 *		TRUE: �ҵ���paddr �е�ʹ�ñ�־��idֵ
 *		FALSE: δ�ҵ�
 */
BOOL GetMacIdInfo(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr)
{
	lic_file_header_t *pheader = (lic_file_header_t *)pbuff;
	macaddr_id_t *ptr;
	int j;
	kal_uint8 *end_ptr = (pbuff + len);
	macaddr_id_t *p;

	ptr = (macaddr_id_t *)(pbuff + MP_ADDRESS_OFFSET);

	// ��������mac��ַ��Ӧ����Ϣ
	for (j = 0; ; j++)
	{
		p = &ptr[j];
		if (memcmp(p->mac_addr, paddr->addr.mac_addr, 6) == 0)
		{
			memcpy(&paddr->addr, p, sizeof(macaddr_id_t));
			paddr->index = j;
			return TRUE;
		}

		if ((kal_uint8 *)p >= end_ptr)
		{
			return FALSE;
		}
	}
	
	return FALSE;
}


/*******************************************************************
*����
       ��������iot������ʹ�ü�¼
*����
	 const TCHAR *lpszPath - �ļ�·��
	 const kal_uint8 *ptr - ����������ָ��
	 const int len - ����������
	 int index - ��������������¼
*����ֵ
     TRUE - ���³ɹ�
     FALSE - ����ʧ��
*����˵��
	2018/11/5 by qinjiangwei
********************************************************************/
Boolean SaveLicenseFileIndex(const TCHAR *lpszPath, const kal_uint8 *ptr, const int len, int index)
{
	try
	{
		CFile file(lpszPath, CFile::modeReadWrite);
		long offset;
		Boolean ret = FALSE;
	
		offset = MP_ADDRESS_OFFSET + index * MP_DATA_SIZE;
		ptr += MP_ADDRESS_OFFSET + index * MP_DATA_SIZE;
		if (offset < len)
		{
			file.Seek(offset, CFile::begin);
			file.Write((const void *)ptr, MP_DATA_SIZE);
			ret = TRUE;
		}

		file.Close();
		
		return ret;
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
