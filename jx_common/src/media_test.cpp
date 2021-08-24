/*
 * �������������ӿڳ���, written by qinjiangwei 2018/12/6
 * ���������Ŀ��������
 */
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef WIN32
#include "winsock2.h"
#include <WS2tcpip.h>
#endif

#include "httpparse.h"
#include "cJSON.h"
#include "mywin.h"
#include "media_test.h"
#include "qcloud_iot_export_log.h"


/*******************************************************************
*����
       ��C���ݽṹת��Ϊjson����
*����
		md_test_hw_result_t *presult	- ���Խ��
*����ֵ
     NOT NULL		- ����json��ʽ
	 NULL		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
static cJSON * md_test_result_json(md_test_hw_result_t *presult)
{
	cJSON *root;

	if (!presult)
	{
		Log_e("presult is null!\n");
		return NULL;
	}

	root = cJSON_CreateObject();
	if (NULL == root) 
	{
		Log_e("no memory for md test!\n");
		return NULL;
	}

	cJSON_AddStringToObject(root, MD_TEST_ITEM_KEY, presult->item_code);
	cJSON *pItem;
	cJSON *pArray;
	httpd_kv_t *pkv;
	pArray = cJSON_CreateArray();
	for (pkv = &presult->kv[0]; strlen(pkv->key) != 0; pkv++)
	{
		pItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pItem, MD_TEST_SUFFIX_KEY, pkv->key);
		cJSON_AddStringToObject(pItem, MD_TEST_VALUE_KEY, pkv->value);

		cJSON_AddItemToArray(pArray, pItem);
	}

	cJSON_AddItemToObject(root, MD_TEST_VALUE_EXT_KEY, pArray);
	cJSON_AddStringToObject(root, MD_TEST_MAC_KEY, presult->mac_addr);
	cJSON_AddNumberToObject(root, MD_TEST_RESULT_KEY, presult->result);
	cJSON_AddStringToObject(root, MD_TEST_BATCH_KEY, presult->batch_code);

	CString str;
	str.Format("%d-%d-%d %d:%d:%d", presult->date_time.wYear, presult->date_time.wMonth, presult->date_time.wDay,
		presult->date_time.wHour, presult->date_time.wMinute, presult->date_time.wSecond);
	cJSON_AddStringToObject(root, MD_TEST_DATETIME_KEY, str.GetBuffer());
	cJSON_AddStringToObject(root, MD_TEST_STATUS_KEY, presult->pstatus);

	return root;
}

/*******************************************************************
*����
       ����ֵ�ص�����
*����
		const char *hostname	- ����������ip��ַ
		int port				- �˿�
		md_test_hw_result_t *presult	- ���Խ��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
static void md_test_parse_hw_test_resp(void *pmsg)
{
	HTTPMSG *http = (HTTPMSG *)pmsg;
	cJSON* pRoot;
	cJSON* pItem;
	cJSON* pMsg;

	http->rsp[http->rsplen] = '\0';

	Log_d("msg:'%s'\r\n", http->rsp);

	pRoot = cJSON_Parse((const char *)http->rsp);
	if (!pRoot)
	{
		printf("json format error!\r\n");
		return;
	}

	pItem = cJSON_GetObjectItem(pRoot, "success");
	if (!pItem)
	{
		printf("can not get success\r\n");
		goto done;
	}

	if (pItem->type == cJSON_True)
	{
		Log_d("hw test result ok!\n");
	}
	else
	{
		Log_d("hw test result NOT OK!\n");
	}

	pMsg = cJSON_GetObjectItem(pRoot, "message");
	if (!pMsg)
	{
		Log_d("no message return!\n");
		goto done;
	}
	
	pItem = cJSON_GetObjectItem(pMsg, "code");
	if (!pItem)
	{
		Log_d("no code!\n");
		goto done;
	}

	Log_d("code = %s\n", pItem->valuestring);

	pItem = cJSON_GetObjectItem(pMsg, "content");
	if (!pItem)
	{
		Log_d("no content!\n");
		goto done;
	}
	Log_d("content = %s\n", pItem->valuestring);

done:
	cJSON_Delete(pRoot);
}

/*******************************************************************
*����
       ������ϵͳ����Ӳ������ֵ
*����
		const char *hostname	- ����������ip��ַ
		int port				- �˿�
		md_test_hw_result_t *presult	- ���Խ��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_hard_test_result(const char *hostname, int port, md_test_hw_result_t *presult)
{
	//const char *url = "http://192.168.3.118:6690/api/iot/testing/hardware/stduri/bulkadd";
	char *body;
	cJSON *pItem;
	cJSON *parray;
	char url[512];

	sprintf(url, "http://%s:%d/api/iot/testing/hardware/stduri/bulkadd", hostname, port);

	printf("\n\n##http_send_hard_test_result\n");

	parray = cJSON_CreateArray();
	
	for (; strlen(presult->item_code) != 0; presult++)
	{
		pItem = md_test_result_json(presult);
		if (!pItem)
		{
			Log_e("to json failed!\n");
			return FALSE;
		}
		cJSON_AddItemToArray(parray, pItem);
	}
	
	body = cJSON_PrintUnformatted(parray);
	cJSON_Delete(parray);
	Log_d("body=%s\n", body);
	if (body)
	{
		httpparse_init(YAHOO_HTTP_RSP_MAX);
		http_send_post_cmd((U8 *)url, body, md_test_parse_hw_test_resp);
		free(body);
		httpparse_data_clear();
		return TRUE;
	}

	return FALSE;
}


//IMPLEMENT_SERIAL(CMediaTestResult, CObject, 1)

/*******************************************************************
*����
       ���캯��
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
CMediaTestResult::CMediaTestResult()
{
	//memset(mac_addr, 0, sizeof(mac_addr));
	//memset(batch_code, 0, sizeof(batch_code));
	//memset(pstatus, 0, sizeof(pstatus));
	mac_addr.Empty();
	batch_code.Empty();
	pstatus.Empty();

	result = 0;

//	SYSTEMTIME tmSys;
//	GetLocalTime(&tmSys);
//	date_time = tmSys;
	date_time = CTime::GetCurrentTime();

	pstatus = "Pending";
	batch_code = "BA20181218";

	//strncpy(pstatus, "Pending", MD_TEST_STR_LEN);
	//strcpy(batch_code, "BA20181218");				// Ĭ��ֵ
}


/*******************************************************************
*����
       ���캯��
*����
		const char *macstr	-	mac��ַ, �ִ�00:11:22:33:44:55
		int res				-	���
		const char *batcode	-	���κ�
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
CMediaTestResult::CMediaTestResult(const char *macstr, int res, const char *batcode)
{
	//memset(mac_addr, 0, sizeof(mac_addr));
	//memset(batch_code, 0, sizeof(batch_code));
	//memset(pstatus, 0, sizeof(pstatus));
	mac_addr.Empty();
	batch_code.Empty();
	pstatus.Empty();

	result = 0;

	//	SYSTEMTIME tmSys;
	//	GetLocalTime(&tmSys);
	//	date_time = tmSys;
	date_time = CTime::GetCurrentTime();

	//strncpy(pstatus, "Pending", MD_TEST_STR_LEN);
	pstatus = "Pending";
	result = res;
	SetMacAddress(macstr);

	//strncpy(batch_code, batcode, MD_TEST_STR_LEN);
	batch_code = CString(batcode);
}

/*******************************************************************
*����
       ��������
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
CMediaTestResult::~CMediaTestResult()
{
}

/*******************************************************************
*����
       ������ת��Ϊjson����
*����
		��
*����ֵ
		NOT NULL	- json����
		NULL		-  ����
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
cJSON * CMediaTestResult::ToJson()
{
	cJSON *root;

	root = cJSON_CreateObject();
	if (NULL == root) 
	{
		Log_e("no memory for md test!\n");
		return NULL;
	}

	cJSON_AddStringToObject(root, MD_TEST_MAC_KEY, mac_addr);
	cJSON_AddNumberToObject(root, MD_TEST_RESULT_KEY, result);
	cJSON_AddStringToObject(root, MD_TEST_BATCH_KEY, batch_code);

	CString str;
	str.Format("%d-%d-%d %02d:%02d:%02d", date_time.GetYear(), date_time.GetMonth(), date_time.GetDay(),
		date_time.GetHour(), date_time.GetMinute(), date_time.GetSecond());
	cJSON_AddStringToObject(root, MD_TEST_DATETIME_KEY, str);
	cJSON_AddStringToObject(root, MD_TEST_STATUS_KEY, pstatus);

	return root;
}



/*******************************************************************
*����
       �������ӡΪjson�����ִ�
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
void CMediaTestResult::FormatPrint()
{
	cJSON *root;
	char *body;

	root = ToJson();
	if (!root)
	{
		Log_e("to json failed!\n");
		return;
	}
	body = cJSON_Print(root);
	if (!body)
	{
		printf("to string failed!\n");
		cJSON_Delete(root);
		return;
	}
	cJSON_Delete(root);
	Log_d("body=%s\n", body);
	free(body);
}


/*******************************************************************
*����
       ���ò��Խ����ʱ�������
*����
		PSYSTEMTIME pdt	-	����ʱ��ʱ�������
*����ֵ
		��
*����˵��
	2018/12/10 by qinjiangwei
********************************************************************/
void CMediaTestResult::SetDateTime(PSYSTEMTIME pdt)
{
	if (!pdt)
	{
		return;
	}

	date_time = CTime(pdt->wYear, pdt->wMonth, pdt->wDay, pdt->wHour, pdt->wMinute, pdt->wSecond);
}

/*******************************************************************
*����
       ����ģ���mac��ַ
*����
		const char *mac	-	ģ���mac��ַ
*����ֵ
		��
*����˵��
	2018/12/10 by qinjiangwei
********************************************************************/
void CMediaTestResult::SetMacAddress(const char *mac)
{
	if (!mac)
	{
		return;
	}
	char tmp[32] = { 0 };

	int len = strlen(mac);
	
	if (len == 12)
	{
		int i;
		int j;

#define ISHEX(x)		(((x) >= '0' && (x) <= '9') || \
						((x) >= 'A' && (x) <= 'F') || \
						((x) >= 'a' && (x) <= 'f'))
		for (i = 0, j = 0; i < len; i++)
		{
			if (!ISHEX(mac[i]))
			{
				Log_e("mac (%s) format error!\n", mac);
				return;
			}

			tmp[j++] = toupper(mac[i]);

			if (i > 0 && (i + 1) % 2 == 0 && ((i + 1) != len))
			{
				tmp[j++] = ':';
			}
		}

		tmp[j] = '\0';

		mac_addr = CString(tmp);

		return;
	}

	if (len == 17)
	{
		strncpy(tmp, mac, sizeof(tmp) - 1);
		for (int i = 0; i < strlen(tmp); i++)
		{
			tmp[i] = toupper(tmp[i]);
		}

		mac_addr = CString(tmp);
	}
	else
	{
		Log_e("mac (%s) format error!\n", mac);
	}
}

/*******************************************************************
*����
       ���л�
*����
		CArchive& ar	-	�ļ�
*����ֵ
		��
*����˵��
	2018/12/10 by qinjiangwei
********************************************************************/
void CMediaTestResult::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);

	if (ar.IsStoring())  
	{  
		ar << mac_addr << result << batch_code << date_time << pstatus;  
	}  
	else  
	{  
		ar >> mac_addr >> result >> batch_code >> date_time >> pstatus;  
	}  

}

/*******************************************************************
*����
       ��ֵ
*����
		const CMediaTestResult& res	-	���Խ��
*����ֵ
		��
*����˵��
	2018/12/10 by qinjiangwei
********************************************************************/
void CMediaTestResult::operator=(const CMediaTestResult& res)
{
	mac_addr = res.mac_addr;
	result = res.result;
	batch_code = res.batch_code;
	date_time = res.date_time;
	pstatus = res.pstatus;
}

/*******************************************************************
*����
       ��ֵ
*����
		const CMediaTestResult& res	-	���Խ��
*����ֵ
		��
*����˵��
	2018/12/10 by qinjiangwei
********************************************************************/
CMediaTestResult::CMediaTestResult(const CMediaTestResult& res)
{
	mac_addr = res.mac_addr;
	result = res.result;
	batch_code = res.batch_code;
	date_time = res.date_time;
	pstatus = res.pstatus;
}

/*******************************************************************
*����
       ���캯��
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
CMediaHardwareTestResult::CMediaHardwareTestResult()
{
	Base();

	memset(kv, 0, sizeof(httpd_kv_t) * MD_TEST_MAX_PARAM_NR); 
	memset(item_code, 0, sizeof(item_code));
	kv_nr = 0;
}

/*******************************************************************
*����
       ���캯��
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
CMediaHardwareTestResult::~CMediaHardwareTestResult()
{
}


/*******************************************************************
*����
       ���캯��
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
CMediaHardwareTestResult::CMediaHardwareTestResult(const char *macstr, int res, const char *batcode, const char *item)
{
	Base(macstr, res, batcode);

	memset(kv, 0, sizeof(httpd_kv_t) * MD_TEST_MAX_PARAM_NR); 
	memset(item_code, 0, sizeof(item_code));
	kv_nr = 0;

	strncpy(item_code, item, MD_TEST_STR_LEN);
}


/*******************************************************************
*����
       ������ת��Ϊjson����
*����
		��
*����ֵ
		NOT NULL	- json����
		NULL		-  ����
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
cJSON * CMediaHardwareTestResult::ToJson()
{
	cJSON *root;
	
	root = Base::ToJson();
	if (!root)
	{
		return root;
	}

	cJSON *pItem;
	cJSON *pArray;

	httpd_kv_t *pkv;
	int i;
	pArray = cJSON_CreateArray();
	for (pkv = &kv[0], i = 0; i < kv_nr; pkv++, i++)
	{
		pItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pItem, MD_TEST_SUFFIX_KEY, pkv->key);
		cJSON_AddNumberToObject(pItem, MD_TEST_VALUE_KEY, atof(pkv->value));

		cJSON_AddItemToArray(pArray, pItem);
	}
	
	cJSON_AddItemToObject(root, MD_TEST_VALUE_EXT_KEY, pArray);
	cJSON_AddStringToObject(root, MD_TEST_ITEM_KEY, item_code);

	return root;
}


/*******************************************************************
*����
       ���������kvֵ
*����
		��
*����ֵ
		NOT NULL	- json����
		NULL		-  ����
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
Boolean CMediaHardwareTestResult::AddKeyValue(httpd_kv_t *pkv)
{
	httpd_kv_t *ptr;

	if (kv_nr >= MD_TEST_MAX_PARAM_NR)
	{
		Log_e("kv nr(%d) is too large!\n", kv_nr);
		return FALSE;
	}

	ptr = &kv[kv_nr];
	*ptr = *pkv;

	kv_nr++;

	return TRUE;
}



/*******************************************************************
*����
       ���ò�����Ŀ
*����
		��
*����ֵ
		NOT NULL	- json����
		NULL		-  ����
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
Boolean CMediaHardwareTestResult::SetItemCode(const char *item)
{
	Boolean ret = TRUE;
	if (strcmp(item, "11M") == 0)
	{
		strcpy(item_code, "WIFI_TX_CH1_11M");
	}
	else if (strcmp(item, "54M") == 0)
	{
		strcpy(item_code, "WIFI_TX_CH7_54M");
	}
	else if (strcmp(item, "65M") == 0)
	{
		strcpy(item_code, "WIFI_TX_CH13_65M");
	}
	else
	{
		Log_e("unkown item:%s", item);
		ret = FALSE;
	}

	return ret;
}


IMPLEMENT_SERIAL(CMediaPackageResult, CObject, 1)

/*******************************************************************
*����
       ���캯��
*����
		��
*����ֵ
		��
*����˵��
	2018/12/18 by qinjiangwei
********************************************************************/
CMediaPackageResult::CMediaPackageResult()
{
	//memset(package_code, 0, sizeof(package_code));
	package_code.Empty();
}


/*******************************************************************
*����
       ��������
*����
		��
*����ֵ
		��
*����˵��
	2018/12/18 by qinjiangwei
********************************************************************/
CMediaPackageResult::~CMediaPackageResult()
{
	m_mac_list.clear();
}


/*******************************************************************
*����
       ��mac��ַ���뵽��װ�б���, address��ʽΪ00:11:22:33:44:55
*����
		��
*����ֵ
		��
*����˵��
	2018/12/18 by qinjiangwei
********************************************************************/
void CMediaPackageResult::AddMacAddress(const CString& address)
{
	if (address.GetLength() != 17)		// ���ж��Ƿ�Ϊ00:11:22:33:44:55��ʽ
	{
		Log_e("mac address(%s) format error!\n", address);
		return;
	}

	m_mac_list.push_back(address);
}


/*******************************************************************
*����
       �����ð�װcode
*����
		��
*����ֵ
		��
*����˵��
	2018/12/18 by qinjiangwei
********************************************************************/
void CMediaPackageResult::AddPackageCode(const CString& pack_code)
{
	if (pack_code.GetLength() >= MD_TEST_STR_LEN + 1)
	{
		Log_e("pack code(%s) is too long!\n", pack_code);
		return;
	}

	package_code = pack_code;
}


/*******************************************************************
*����
       �����ð�װ���ݱ��json���ݶ���
*����
		��
*����ֵ
		��
*����˵��
	2018/12/18 by qinjiangwei
********************************************************************/
cJSON * CMediaPackageResult::ToJson()
{
	cJSON *root;
	cJSON *pArray;
	vector<CString>::iterator iter;

	pArray = cJSON_CreateArray();

	for (iter = m_mac_list.begin(); iter != m_mac_list.end(); iter++)
	{
		root = cJSON_CreateObject();
		if (NULL == root) 
		{
			Log_e("no memory for md package!\n");
			return NULL;
		}

		cJSON_AddStringToObject(root, MD_TEST_BATCH_KEY, batch_code);
		cJSON_AddStringToObject(root, MD_TEST_PACKCODE_KEY, package_code);

		CString str;
		str.Format("%d-%d-%d %02d:%02d:%02d", date_time.GetYear(), date_time.GetMonth(), date_time.GetDay(),
			date_time.GetHour(), date_time.GetMinute(), date_time.GetSecond());
		cJSON_AddStringToObject(root, MD_PACKING_DATETIME_KEY, str.GetBuffer());

		cJSON_AddStringToObject(root, MD_TEST_MAC_KEY, *iter);
		cJSON_AddItemToArray(pArray, root);
	}

	return pArray;
}

/*******************************************************************
*����
       �����ð�װcode
*����
		��
*����ֵ
		��
*����˵��
	2018/12/18 by qinjiangwei
********************************************************************/
void CMediaPackageResult::Serialize(CArchive &ar)
{
	Base::Serialize(ar);
	vector<CString>::iterator iter;

	if (ar.IsStoring())  
	{  
		ar << m_mac_list.size();
		
		for (iter = m_mac_list.begin(); iter != m_mac_list.end(); iter++)
		{
			ar << *iter;
		}

		ar << package_code;
	}  
	else  
	{  
		int nr;

		ar >> nr;
		for (int i = 0; i < nr; i++)
		{
			CString str;
			ar >> str;

			m_mac_list.push_back(str);
		}
		
		ar >> package_code;
	}  
}


/*******************************************************************
*����
       ������ϵͳ����Ӳ������ֵ
*����
		const char *hostname	- ����������ip��ַ
		int port				- �˿�
		md_test_hw_result_t *presult	- ���Խ��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_test_result(const char *hostname,
												   int port,
												   const char *path,
												   CMediaTestResult *presult, 
												   int n)
{
	char *body;
	cJSON *pItem;
	cJSON *parray;
	char url[512];

	//sprintf(url, "http://%s:%d/api/iot/testing/hardware/stduri/bulkadd", hostname, port, path);
	_snprintf(url, sizeof(url), "http://%s:%d%s", hostname, port, path);

	Log_d("\n\n##http_send_test_result url=%s\n", url);

	parray = cJSON_CreateArray();
	
	for (int i = 0; i < n; presult++, i++)
	{
		pItem = presult->ToJson();
		if (!pItem)
		{
			Log_e("to json failed!\n");
			return FALSE;
		}
		cJSON_AddItemToArray(parray, pItem);
	}
	
	body = cJSON_PrintUnformatted(parray);
	cJSON_Delete(parray);
	Log_d("body=%s\n", body);
	if (body)
	{
		httpparse_init(YAHOO_HTTP_RSP_MAX);
		http_send_post_cmd((U8 *)url, body, md_test_parse_hw_test_resp);
		free(body);
		httpparse_data_clear();
		return TRUE;
	}

	return FALSE;
}

/*******************************************************************
*����
       ������ϵͳ���ʹ��mac��ַ
*����
		const char *hostname	- ����������ip��ַ
		int port				- �˿�
		CMediaTestResult *presult	- ���Խ��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_package_result(const char *hostname,
												   int port,
												   const char *path,
												   CMediaTestResult *presult, 
												   int n)
{
	char *body;
	cJSON *pItem;
	char url[512];

	//sprintf(url, "http://%s:%d/api/iot/testing/hardware/stduri/bulkadd", hostname, port, path);
	_snprintf(url, sizeof(url), "http://%s:%d%s", hostname, port, path);

	Log_d("\n\n##http_send_test_result url=%s\n", url);

	pItem = presult->ToJson();
	if (!pItem)
	{
		Log_e("to json failed!\n");
		return FALSE;
	}

	body = cJSON_PrintUnformatted(pItem);
	cJSON_Delete(pItem);
	Log_d("body=%s\n", body);
	if (body)
	{
		httpparse_init(YAHOO_HTTP_RSP_MAX);
		http_send_post_cmd((U8 *)url, body, md_test_parse_hw_test_resp);
		free(body);
		httpparse_data_clear();
		return TRUE;
	}

	return FALSE;
}


/*******************************************************************
*����
       ������ϵͳ����Ӳ������ֵ
*����
		const char *hostname	- ����������ip��ַ
		int port				- �˿�
		const char *path		- URL API ·��
		LPCTSTR lpszLogFile		- LogFile ·��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_hw_test_result(const char *hostname,
												   int port,
												   const char *path,
												   LPCTSTR lpszLogFile)
{
	try
	{
		CITestFile file(lpszLogFile);
		MediaHwTestVector v;
		MediaHwTestVector::iterator iter;
		char *body;
		cJSON *pItem;
		cJSON *parray;
		char url[512];

		v = file.GetTestResults();
		
		parray = cJSON_CreateArray();
		for (iter = v.begin(); iter != v.end(); iter++)
		{
			pItem = iter->ToJson();
			if (!pItem)
			{
				Log_e("to json failed!\n");
				return FALSE;
			}
			cJSON_AddItemToArray(parray, pItem);
		}

		_snprintf(url, sizeof(url), "http://%s:%d%s", hostname, port, path);

		Log_d("\n\n##http_send_test_result url=%s\n", url);

		body = cJSON_PrintUnformatted(parray);
		cJSON_Delete(parray);
		Log_d("body=%s\n", body);
		if (body)
		{
			httpparse_init(YAHOO_HTTP_RSP_MAX);
			http_send_post_cmd((U8 *)url, body, md_test_parse_hw_test_resp);
			free(body);
			httpparse_data_clear();
			return TRUE;
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

	return FALSE;
}


/*******************************************************************
*����
       ���Ժ���
*����
		��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
FUNC_DLL_EXPORT void md_test_stub()
{
	md_test_hw_result_t res[2];
	md_test_hw_result_t *ptr;

	return;

	memset(res, 0, sizeof(md_test_hw_result_t) * 2);

	ptr = &res[0];

	strcpy(ptr->item_code, "WIFI_TX_CH1_11M");

	strcpy(ptr->kv[0].key, "evm");
	sprintf(ptr->kv[0].value, "20");

	strcpy(ptr->kv[1].key, "pwr");
	sprintf(ptr->kv[1].value, "2");

	strcpy(ptr->kv[2].key, "freq");
	sprintf(ptr->kv[2].value, "2");

	strcpy(ptr->mac_addr, "5A:F3:10:7A:40:47");
	ptr->result = 1;
	strcpy(ptr->batch_code, "B20181022");

	SYSTEMTIME tmSys;
	GetLocalTime(&tmSys);
	ptr->date_time = tmSys;
	
	strcpy(ptr->pstatus, "Pending");

	http_send_hard_test_result("192.168.3.118", 6690, res);

	CMediaTestResult tr("5aF3107A404b", 1, "B20181022");
	tr.SetMacAddress("0011223344ab");
	http_send_test_result("192.168.3.118", 6690, "/api/iot/testing/burning/stduri/bulkadd", &tr, 1);

	CMediaHardwareTestResult htr("5A:F3:10:7A:40:47", 1, "B20181022", "WIFI_TX_CH1_11M");
	httpd_kv_t kv;

	strcpy(kv.key, "evm");
	strcpy(kv.value, "20");
	htr.AddKeyValue(&kv);

	strcpy(kv.key, "pwr");
	strcpy(kv.value, "2");
	htr.AddKeyValue(&kv);

	strcpy(kv.key, "freq");
	strcpy(kv.value, "3");
	htr.AddKeyValue(&kv);
	
	htr.FormatPrint();

	CMediaPackageResult pr;
	pr.AddPackageCode("PACK1234560001");
	pr.AddMacAddress("C8:47:8C:00:9D:81");
	pr.SetBatchCode("20181011111");
	
	pr.AddMacAddress("C8:47:8C:00:9D:82");
	pr.AddMacAddress("C8:47:8C:00:9D:83");
	pr.AddMacAddress("C8:47:8C:00:9D:84");

	pr.FormatPrint();

	http_send_test_result("192.168.3.118", 6690, "/api/iot/testing/hardware/stduri/bulkadd", &htr, 1);

	http_send_hw_test_result("192.168.3.118", 6690, "/api/iot/testing/hardware/stduri/bulkadd", _T("D:\\itest_log.txt"));
}
