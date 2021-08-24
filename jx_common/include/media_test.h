#ifndef MEDIA_TEST_H
#define MEDIA_TEST_H
#include "windows.h"
#include "afx.h"
#include <vector>
#include <map>
#include "ServerCmd.h"
#include "data_buff.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

using namespace std;


// JSON key
#define MD_TEST_MAC_KEY				"MAC_ADDRESS"
#define MD_TEST_RESULT_KEY			"TEST_RESULT"
#define MD_TEST_BATCH_KEY			"BATCH_NO"
#define MD_TEST_DATETIME_KEY		"TEST_DATETIME"
#define MD_TEST_STATUS_KEY			"PROCESS_STATUS"
#define MD_TEST_PACKCODE_KEY		"PACKING_CODE"
#define MD_TEST_PACKDT_KEY			"PACKING_DATETIME"
#define MD_TEST_ITEM_KEY			"ITEM_CODE"
#define MD_TEST_VALUE_KEY			"TEST_VALUE"			// �ĵ�������ҪEXT
#define MD_TEST_SUFFIX_KEY			"ITEM_CODE_SUFFIX"
#define MD_TEST_VALUE_EXT_KEY			"TEST_VALUE_EXT"			// �ĵ�������ҪEXT
#define MD_PACKING_DATETIME_KEY		"PACKING_DATETIME"
#define MD_TEST_STR_LEN			80
#define MD_TEST_MAX_PARAM_NR		10
#define MAX_MAC_ADDR_LEN			(13 + 5)		// ����ð��
// Ӳ�����Խ��
typedef struct hw_test_struct
{
	char mac_addr[MAX_MAC_ADDR_LEN];
	float result;
	char batch_code[MD_TEST_STR_LEN];
	SYSTEMTIME date_time;
	char pstatus[MD_TEST_STR_LEN];			// process status

	char item_code[MD_TEST_STR_LEN];			// item code
	httpd_kv_t kv[MD_TEST_MAX_PARAM_NR];		// key-value
} md_test_hw_result_t;

/*
 * ���Ĳ��Խ����
 */
class CMediaTestResult : public CObject
{
	//DECLARE_SERIAL(CMediaTestResult)

public:
	CMediaTestResult();							// ���캯��
	CMediaTestResult(const char *macstr, int res, const char *batcode);

	CMediaTestResult(const CMediaTestResult& obj); 

	~CMediaTestResult();					// ��������

	virtual cJSON * ToJson();				// ת����json��ʽ

	virtual void FormatPrint();				// ��ʽ����ӡ

	void SetDateTime(PSYSTEMTIME pdt);

	void operator=(const CMediaTestResult& res);

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
	void SetMacAddress(const char *mac);
	
	void SetResultValue(int v) { result = v; }

	void SetBatchCode(const char *code) { batch_code = CString(code); }

	virtual void Serialize(CArchive &ar); 

protected:
	//char mac_addr[MAX_MAC_ADDR_LEN];
	CString mac_addr;
	int result;
	//char batch_code[MD_TEST_STR_LEN];
	CString batch_code;

	//SYSTEMTIME date_time;
	CTime date_time;

	//char pstatus[MD_TEST_STR_LEN];			// process status
	CString pstatus;
};

/*
 * ����Ӳ�����Խ����
 */
class CMediaHardwareTestResult: public CMediaTestResult
{
	typedef CMediaTestResult Base;
public:
	CMediaHardwareTestResult();					// ���캯��
	
	// 
	CMediaHardwareTestResult(const char *macstr, int res, const char *batcode, const char *item);

	//
	virtual ~CMediaHardwareTestResult();

	virtual cJSON * ToJson();

	Boolean AddKeyValue(httpd_kv_t *pkv);

	Boolean SetItemCode(const char *item);

	void ResetKeyValue() { kv_nr = 0;}

private:
	char item_code[MD_TEST_STR_LEN];			// item code
	httpd_kv_t kv[MD_TEST_MAX_PARAM_NR];		// key-value
	int kv_nr;
};



/*
 * ����Ӳ�����Խ����
 */
class CMediaPackageResult: public CMediaTestResult
{
	DECLARE_SERIAL(CMediaPackageResult)

public:
	CMediaPackageResult();					// ��������
	typedef CMediaTestResult Base;
	
	virtual ~CMediaPackageResult();

	void AddMacAddress(const CString& address);
	void AddPackageCode(const CString& pack_code);
	virtual cJSON * ToJson();
	int  GetMacAddrNumber()	{ return m_mac_list.size(); }
	void ClearMacList()		{ m_mac_list.clear();}
	virtual void Serialize(CArchive &ar); 

private:
	CString package_code;			// item code
	vector<CString> m_mac_list;					// �����mac��ַ
};

typedef map<CString, CString> kv_t;
typedef vector<CMediaHardwareTestResult> MediaHwTestVector;
typedef vector<CString> StringVector;

/*******************************************************************
*����
       ���ǻ�����־�ļ���
*����˵��
	2018/12/6 by qinjiangwei
********************************************************************/
class CITestFile : public CStdioFile
{
public:
	typedef CStdioFile Base;

	CITestFile();					// ���캯��

	CITestFile(LPCTSTR lpszPath);		// ���캯��, lpszPath�ļ�·��

	~CITestFile();

	CMediaHardwareTestResult GetTestResult();

	MediaHwTestVector GetTestResults();

private:
	CString GetTestMacAddrString();		// ��ȡ����ģ���mac��ַ

	
	/*******************************************************************
	*����
		   �������Ե�ģ���ַ
	*����
			��
	*����ֵ
			��
	*����˵��
		2018/12/6 by qinjiangwei

		-------------------------------------------------------------------------------
		Test Time: 0.16 sec


		DUT MAC:3873EAE3A591
	********************************************************************/
	CString GetTestMacAddrString2();

	SYSTEMTIME GetTestDateTime();		// ��ȡ���Ե�ʱ�������

	/*******************************************************************
	*����
	   ��������TX ���Ե�һ����¼
	*����
		��
	*����ֵ
		��
	*����˵��
	2018/12/6 by qinjiangwei
	********************************************************************/
	kv_t ParseTxTestOneItem();
};



/*******************************************************************
*����
       ������ϵͳ����Ӳ������ֵ
*����
		const char *hostname	- ����������ip��ַ
		int port				- �˿�
		md_test_hw_result_t *presult	- ���Խ��, һά����, presult->item_code����Ϊ0����
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_hard_test_result(const char *hostname, int port, md_test_hw_result_t *presult);


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
FUNC_DLL_EXPORT void md_test_stub();


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
FUNC_DLL_EXPORT void parse_itest_stub();



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
												   LPCTSTR lpszLogFile);


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
												   int n);

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
												   int n);
#ifdef __cplusplus
}
#endif
#endif