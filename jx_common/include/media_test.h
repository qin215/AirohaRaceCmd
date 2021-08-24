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
#define MD_TEST_VALUE_KEY			"TEST_VALUE"			// 文档有误，需要EXT
#define MD_TEST_SUFFIX_KEY			"ITEM_CODE_SUFFIX"
#define MD_TEST_VALUE_EXT_KEY			"TEST_VALUE_EXT"			// 文档有误，需要EXT
#define MD_PACKING_DATETIME_KEY		"PACKING_DATETIME"
#define MD_TEST_STR_LEN			80
#define MD_TEST_MAX_PARAM_NR		10
#define MAX_MAC_ADDR_LEN			(13 + 5)		// 加上冒号
// 硬件测试结果
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
 * 美的测试结果类
 */
class CMediaTestResult : public CObject
{
	//DECLARE_SERIAL(CMediaTestResult)

public:
	CMediaTestResult();							// 构造函数
	CMediaTestResult(const char *macstr, int res, const char *batcode);

	CMediaTestResult(const CMediaTestResult& obj); 

	~CMediaTestResult();					// 析构函数

	virtual cJSON * ToJson();				// 转化成json格式

	virtual void FormatPrint();				// 格式化打印

	void SetDateTime(PSYSTEMTIME pdt);

	void operator=(const CMediaTestResult& res);

	/*******************************************************************
	*作用
		   设置模块的mac地址
	*参数
			const char *mac	-	模块的mac地址
	*返回值
			无
	*其它说明
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
 * 美的硬件测试结果类
 */
class CMediaHardwareTestResult: public CMediaTestResult
{
	typedef CMediaTestResult Base;
public:
	CMediaHardwareTestResult();					// 构造函数
	
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
 * 美的硬件测试结果类
 */
class CMediaPackageResult: public CMediaTestResult
{
	DECLARE_SERIAL(CMediaPackageResult)

public:
	CMediaPackageResult();					// 析构函数
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
	vector<CString> m_mac_list;					// 打包的mac地址
};

typedef map<CString, CString> kv_t;
typedef vector<CMediaHardwareTestResult> MediaHwTestVector;
typedef vector<CString> StringVector;

/*******************************************************************
*作用
       机智汇仪日志文件类
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
class CITestFile : public CStdioFile
{
public:
	typedef CStdioFile Base;

	CITestFile();					// 构造函数

	CITestFile(LPCTSTR lpszPath);		// 构造函数, lpszPath文件路径

	~CITestFile();

	CMediaHardwareTestResult GetTestResult();

	MediaHwTestVector GetTestResults();

private:
	CString GetTestMacAddrString();		// 获取测试模块的mac地址

	
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
	CString GetTestMacAddrString2();

	SYSTEMTIME GetTestDateTime();		// 获取测试的时间和日期

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
	kv_t ParseTxTestOneItem();
};



/*******************************************************************
*作用
       向美的系统发送硬件测试值
*参数
		const char *hostname	- 主机名或者ip地址
		int port				- 端口
		md_test_hw_result_t *presult	- 测试结果, 一维数组, presult->item_code内容为0结束
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
	2018/12/6 by qinjiangwei

********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_hard_test_result(const char *hostname, int port, md_test_hw_result_t *presult);


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
FUNC_DLL_EXPORT void md_test_stub();


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
FUNC_DLL_EXPORT void parse_itest_stub();



/*******************************************************************
*作用
       向美的系统发送硬件测试值
*参数
		const char *hostname	- 主机名或者ip地址
		int port				- 端口
		const char *path		- URL API 路径
		LPCTSTR lpszLogFile		- LogFile 路径
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_hw_test_result(const char *hostname,
												   int port,
												   const char *path,
												   LPCTSTR lpszLogFile);


/*******************************************************************
*作用
       向美的系统发送硬件测试值
*参数
		const char *hostname	- 主机名或者ip地址
		int port				- 端口
		md_test_hw_result_t *presult	- 测试结果
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
	2018/12/6 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean http_send_test_result(const char *hostname,
												   int port,
												   const char *path,
												   CMediaTestResult *presult, 
												   int n);

/*******************************************************************
*作用
       向美的系统发送打包mac地址
*参数
		const char *hostname	- 主机名或者ip地址
		int port				- 端口
		CMediaTestResult *presult	- 测试结果
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
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