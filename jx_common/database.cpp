#include "stdafx.h"
#include <afxdisp.h>
#import "C:\Program Files\Common Files\System\ado\msado15.dll" no_namespace rename("EOF","adoEOF")
#include "homi_common.h"
#include "comdef.h"
#include "qcloud_iot_export_log.h"
#include "mywin.h"

CString VariantToString(_variant_t var);


#define DB_CHECK_HANDLE()	do { if (!ado_handle) { Log_e("ado handle NOT exist!\n"); return FALSE;}} while (0)
#define DB_CHECK_HANDLE_NULL()	do { if (!ado_handle) { Log_e("ado handle NOT exist!\n"); return NULL;}} while (0)
#define DB_CHECK_HANDLE_ZER0()	do { if (!ado_handle) { Log_e("ado handle NOT exist!\n"); return 0;}} while (0)
static int database_error_code;

/*
 * ���ݱ�����ƣ�
 *			mac_usage				--	MAC��ַʹ�ü�¼������/ALIOS��Ŀɨ���ά�����LICENSEʱʹ��
 *			mac_readback_usage		--	MAC��ַ�ض�����¼��������Ŀ�ض����ʹ��
 *			mac_alloc				--	������Ŀ����������ַ�ļ�¼��
 *			mac_seg					--	������Ŀ������ַ�η����¼��
 */

/*
 * �������ݿ��࣬ͨ�õĲ�����װ��һ��
 */
class CADOEx 
{
public:
	CADOEx();

	// ��¼������
	_RecordsetPtr m_pRs;
	// �������
	_CommandPtr m_PCmd;

	_ConnectionPtr m_pConnection;
	// �������ݿ�
	BOOL ConnectDB(const CString& server, const CString& db, const CString& user, const CString& password);

	// ִ�и��²���
	BOOL ExecuteNotSelSQL(CString strNotSelSQL);
};

//static CADOEx s_ado_object;

static void * ado_handle;


CADOEx::CADOEx()
{
	m_pRs = NULL;
	m_PCmd = NULL;
	m_pConnection = NULL;
}

/*
 * ��������
 * server: ��������
 * db: ���ݿ���
 * user: �û���
 * password: ����
 */
BOOL CADOEx::ConnectDB(const CString& server, const CString& db,
					   const CString& user, const CString& password)
{
	CString connStr;

	connStr.Format(_T("driver={SQL Server};Server=%s;DATABASE=%s;UID=%s;PWD=%s"), server, db, user, password);

	try
	{
		BSTR bstrText = connStr.AllocSysString(); 
		HRESULT hr = m_pConnection->Open(bstrText, "", "", adModeUnknown);
		SysFreeString(bstrText);
		if (!SUCCEEDED(hr))
		{
			Log_e("connect server(%s) db(%s) as user(%s) pass(%s) failed!\n", server, db, user, password);
			//AfxMessageBox(_T("���ӵ����ݿ�ʧ�ܣ�"));
			return FALSE;
		}
		
		Log_d("connect database ok!\n");
		return TRUE;
	}
	catch (const _com_error& e)
	{
		Log_e("connect server(%s) db(%s) as user(%s) pass(%s) failed!\n", server, db, user, password);
		//AfxMessageBox(_T("���ӵ����ݿ�ʧ�ܣ�"));
		return FALSE;
	}
}

/*
 * ִ��SQL���
 */
BOOL CADOEx::ExecuteNotSelSQL(CString strNotSelSQL)
{
	try
	{
		_variant_t vResult;

		vResult.vt = VT_ERROR;
		vResult.scode = DISP_E_PARAMNOTFOUND;

		//���ù�����Connection����
		m_PCmd->ActiveConnection = m_pConnection;
		//SQL����
		m_PCmd->CommandText = (_bstr_t)strNotSelSQL;
		//ִ������
		m_pRs = m_PCmd->Execute(&vResult, &vResult, adCmdText);

	}
	catch (_com_error& e)
	{
		Log_e("execute sql(%s) failed!\n", strNotSelSQL);
		database_error_code = DATABASE_CODE_EXECUTE_ERROR;
		//��ʾ������Ϣ
		//AfxMessageBox(e.ErrorMessage());
		return FALSE;
	}
	Log_d("execute sql(%s) ok!\n", strNotSelSQL);
	return TRUE;
}

/*
 * ��ʼ���������ݿ�
 */
FUNC_DLL_EXPORT BOOL init_database(const char *server, const char *db, const char *user, const char *pass)
{
	Log_d("jx common dll version:%s", JX_COMMON_LIB_VERSION);

	//�������Ӷ���ʵ��
	if (ado_handle)
	{
		Log_e("ado handle exist!\n");
		return FALSE;
	}

	ado_handle = (void *) new CADOEx;
	if (!ado_handle)
	{
		Log_e("no memory!\n");
		return FALSE;
	}

	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	HRESULT hr = ado_ptr->m_pConnection.CreateInstance(_uuidof(Connection));
	if (!SUCCEEDED(hr))
	{
		Log_e("create connection instance failed!\n");
		//AfxMessageBox(_T("�������Ӷ���ʵ��ʧ�ܣ�"));
		return FALSE;
	}

	//�����������ʵ��
	hr = ado_ptr->m_PCmd.CreateInstance(__uuidof(Command));
	if (!SUCCEEDED(hr))
	{
		Log_e("create command instance failed!\n");
		database_error_code = DATABASE_CODE_EXECUTE_ERROR;
		//AfxMessageBox(_T("�����������ʧ��"));
		return FALSE;
	}

	return ado_ptr->ConnectDB(server, db, user, pass);
}

/*
 * �ر����ݿ�����
 */
FUNC_DLL_EXPORT BOOL deinit_database()
{
	if (!ado_handle)
	{
		Log_e("ado handle NOT exist!\n");
		return FALSE;
	}

	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	ado_ptr->m_pConnection->Close();
	ado_ptr->m_pConnection = NULL;

	delete ado_ptr;
	ado_ptr = NULL;

	return TRUE;
}


/*
 * ���MAC��ַ�Ƿ���д
 */
BOOL check_mac_used(const char *mac_str)
{
	CString strSQL;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	strSQL = "SELECT MAC, USED_COUNT, CUSTOMER FROM product.dbo.mac_usage WHERE MAC LIKE ";
	strSQL += "'";
	strSQL += CString(mac_str);
	strSQL += "'";
	
	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return FALSE;
	}

	_variant_t vitem;
	CString strMac;
	int nr = 0;
	int used = 0;
	while (!ado_ptr->m_pRs->adoEOF)
	{
		vitem = ado_ptr->m_pRs->GetCollect("MAC");
		strMac = VariantToString(vitem);
		vitem = ado_ptr->m_pRs->GetCollect("USED_COUNT");
		used = (vitem.vt == VT_INT) ? vitem.intVal : 0;
		nr++;
		ado_ptr->m_pRs->MoveNext();
	}

	ado_ptr->m_pRs->Close();

	return nr > 0 || used > 0;
}


/*******************************************************************
*����
       ������ر���MAC��ַ�Ƿ���д
*����
     const char *mac_str - mac��ַ��hex�ַ���
*����ֵ
     TRUE - ��ʹ��
     FALSE - δʹ��
*����˵��
	2018/11/5 by qinjiangwei
********************************************************************/
BOOL check_download_mac_used(const char *mac_str)
{
	CString strSQL;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	strSQL = "SELECT MAC, USED_COUNT, CUSTOMER FROM product.dbo.mac_dl_usage WHERE MAC LIKE ";
	strSQL += "'";
	strSQL += CString(mac_str);
	strSQL += "'";

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return FALSE;
	}

	_variant_t vitem;
	CString strMac;
	int nr = 0;
	int used = 0;
	while (!ado_ptr->m_pRs->adoEOF)
	{
		vitem = ado_ptr->m_pRs->GetCollect("MAC");
		strMac = VariantToString(vitem);
		vitem = ado_ptr->m_pRs->GetCollect("USED_COUNT");
		used = (vitem.vt == VT_INT) ? vitem.intVal : 0;
		nr++;
		ado_ptr->m_pRs->MoveNext();
	}

	ado_ptr->m_pRs->Close();

	return nr > 0 || used > 0;
}

/*
 * �������ݿ�ļ�¼
 */
BOOL update_db_record(const char *mac_str, int used_count)
{
	CString strSQL = "INSERT INTO product.dbo.mac_usage(MAC,USED_COUNT,LICENSE,CUSTOMER) VALUES('";
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	str.Format("%d", used_count);

	strSQL += CString(mac_str);
	strSQL += CString("',");
	strSQL += str;
	strSQL += CString(",'aaaa', 'midea')");
	return ado_ptr->ExecuteNotSelSQL(strSQL);
}


/*******************************************************************
*����
       �������ر������ݿ�ļ�¼
*����
     const char *mac_str - mac��ַ��hex�ַ���
	 int used_count - ʹ�õĴ���
*����ֵ
     TRUE - ���³ɹ�
     FALSE - ����ʧ��
*����˵��
	2018/11/5 by qinjiangwei
********************************************************************/
BOOL update_download_db_record(const char *mac_str, int used_count)
{
	CString strSQL = "INSERT INTO product.dbo.mac_dl_usage(MAC,USED_COUNT,CUSTOMER) VALUES('";
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	str.Format("%d", used_count);

	strSQL += CString(mac_str);
	strSQL += CString("',");
	strSQL += str;
	strSQL += CString(",'midea')");
	return ado_ptr->ExecuteNotSelSQL(strSQL);
}

/*******************************************************************
*����
      ���MAC��ַ�Ƿ��Ѽ�¼
*����
     const char *mac_str - mac��ַ�ִ�
	 const char *table	-	���ݱ�����
*����ֵ
	 TRUE		-	MAC��ַ�Ѿ���¼
	 FALSE		-	MAC��ַδ��¼
*����˵��
	2018/12/13 by qinjiangwei
********************************************************************/
BOOL check_mac_used_record(const char *mac_str, const char *table)
{
	CString strSQL;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	strSQL = "SELECT MAC, USED_COUNT, CUSTOMER FROM product.dbo."; //mac_usage "WHERE MAC LIKE ";
	strSQL += CString(table);
	strSQL += CString(" WHERE MAC LIKE ");
	strSQL += "'";
	strSQL += CString(mac_str);
	strSQL += "'";

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return FALSE;
	}

	_variant_t vitem;
	CString strMac;
	int nr = 0;
	int used = 0;
	while (!ado_ptr->m_pRs->adoEOF)
	{
		vitem = ado_ptr->m_pRs->GetCollect("MAC");
		strMac = VariantToString(vitem);
		vitem = ado_ptr->m_pRs->GetCollect("USED_COUNT");
		used = (vitem.vt == VT_INT) ? vitem.intVal : 0;
		nr++;
		ado_ptr->m_pRs->MoveNext();
	}

	ado_ptr->m_pRs->Close();

	return nr > 0 || used > 0;
}


/*******************************************************************
*����
      ����MAC��ַʹ�ü�¼
*����
     const char *mac_str - mac��ַ�ִ�
	 const char *table	-	���ݱ�����
	 int used_count		-	ʹ�ô���
	 const char *customer	-	�ͻ���
*����ֵ
	 TRUE		-	���³ɹ�
	 FALSE		-	���²��ɹ�
*����˵��
	Ϊ��֤Ч�ʣ�Ŀǰδ�жϼ�¼�Ƿ��Ѿ����ڣ�ʹ��ʱ��ע��
	2018/12/13 by qinjiangwei
********************************************************************/
BOOL update_mac_used_record(const char *mac_str, const char *table, const char *customer, int used_count)
{
	CString strSQL; // = "INSERT INTO product.dbo.mac_usage(MAC,USED_COUNT,LICENSE,CUSTOMER) VALUES('";
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (!mac_str || !table)
	{
		Log_e("no macstring or table name!");
		return FALSE;
	}
	
	strSQL = "INSERT INTO product.dbo."; 
	strSQL += table;
	if (customer)
	{
		strSQL += "(MAC,USED_COUNT,CUSTOMER) VALUES('";
	}
	else
	{
		strSQL += "(MAC,USED_COUNT) VALUES('";
	}

	str.Format("%d", used_count);

	strSQL += CString(mac_str);
	strSQL += CString("',");
	strSQL += str;
	if (customer)
	{
		CString tmp;

		tmp.Format(",'%s')", customer);
		strSQL += tmp;
	}
	else
	{
		strSQL += ")";
	}
	
	return ado_ptr->ExecuteNotSelSQL(strSQL);
}


/*
 * �������ݿ�ļ�¼
 * mac_str: MAC ��ַ
 * used_count: ʹ�ô���
 * dname: device name
 * dkey: device key
 * lic: license����
 * customer: �ͻ���
 * ����ֵ��
 * TRUE: ִ�гɹ�
 * FALSE: ִ��ʧ��
 */
BOOL update_db_record_info(const char *mac_str, int used_count, const char *dname, const char *dkey, const char *lic, const char *customer)
{
	CString strSQL = "INSERT INTO product.dbo.mac_usage(MAC,USED_COUNT";
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	if (!mac_str)
	{
		Log_e("mac str is null!\n");
		database_error_code = DATABASE_CODE_PARAM_ERROR;
		return FALSE;
	}

	if (!customer)
	{
		Log_e("no customer name!\n");
		database_error_code = DATABASE_CODE_PARAM_ERROR;
		return FALSE;
	}

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (dname && dkey)
	{
		strSQL += CString(",DEVICE_NAME,DEVICE_KEY");
	}

	if (lic)
	{
		strSQL += CString(",LICENSE");
	}

	strSQL += CString(",CUSTOMER) VALUES('");

	

	strSQL += CString(mac_str);
	strSQL += CString("',");
	str.Format("%d", used_count);
	strSQL += str;

	if (dname && dkey)
	{
		str.Format(",'%s','%s'", dname, dkey);
		strSQL += str;
	}

	if (lic)
	{
		str.Format(",'%s'", lic);
		strSQL += str;
	}

	str.Format(",'%s')", customer);
	strSQL += str;
	
	return ado_ptr->ExecuteNotSelSQL(strSQL);
}

/*
 * �����ϴ�dbִ��ʱ�ĳ�����
 */
int database_get_error_code()
{
	return database_error_code;
}

/*******************************************************************
*����
       �����ݿ��еļ�¼������ص������ڴ���
*����
     const char *customer - mac��ַ�Ŀͻ���
*����ֵ
     != 0		-	�б�handle
	 == 0		-	ʧ��
*����˵��
	2018/11/28 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT void* db_fetch_mac_allocate_list(const char *customer)
{
	CString strSQL;
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	DB_CHECK_HANDLE_NULL();

	strSQL = "SELECT ID, BASE_ADDR, TOTAL_LEN FROM product.dbo.mac_seg WHERE CUSTOMER = '";
	strSQL += CString(customer);
	strSQL += CString("'");

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return NULL;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return NULL;
	}

	_variant_t vitem;
	CString strMac;
	int nr = 0;
	int len = 0;
	int seg_id = -1;
	UINT64 baddr = 0;

	while (!ado_ptr->m_pRs->adoEOF)
	{
		vitem = ado_ptr->m_pRs->GetCollect("ID");
		seg_id = (vitem.vt == VT_I4) ? vitem.intVal : 0;

		vitem = ado_ptr->m_pRs->GetCollect("BASE_ADDR");
		strMac = VariantToString(vitem);
		baddr = MacString2Binary(strMac);

		vitem = ado_ptr->m_pRs->GetCollect("TOTAL_LEN");
		len = (vitem.vt == VT_I4) ? vitem.intVal : 0;
		nr++;

		ado_ptr->m_pRs->MoveNext();
	}
	ado_ptr->m_pRs->Close();

	if (seg_id < 0)
	{
		Log_e("no record in customer(%s)", customer);
		return NULL;
	}

	strSQL = "SELECT ID, SEG_ADDR, SEG_LEN, FLAG FROM product.dbo.mac_alloc WHERE SEG_ID = ";
	str.Format("%d", seg_id);
	strSQL += str;
	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return NULL;
	}
	
	kal_uint32 address;
	kal_uint32 addr_len;
	kal_uint32 flag;
	kal_uint32 id;
	void *handle;

	handle = init_address_list(baddr, len, seg_id);
	if (!handle)
	{
		Log_e("no memory!\n");
		return NULL;
	}

	while (!ado_ptr->m_pRs->adoEOF)
	{
		vitem = ado_ptr->m_pRs->GetCollect("SEG_ADDR");
		address = (vitem.vt == VT_I4) ? vitem.intVal : 0;

		vitem = ado_ptr->m_pRs->GetCollect("SEG_LEN");
		addr_len = (vitem.vt == VT_I4) ? vitem.intVal : 0;

		vitem = ado_ptr->m_pRs->GetCollect("FLAG");
		flag = (vitem.vt == VT_I4) ? vitem.intVal : 0;

		vitem = ado_ptr->m_pRs->GetCollect("ID");
		id = (vitem.vt == VT_I4) ? vitem.intVal : 0;

		add_address_list(handle, address, addr_len, flag, id);

		nr++;
		ado_ptr->m_pRs->MoveNext();
	}
	ado_ptr->m_pRs->Close();

	return handle;
}

/*******************************************************************
*����
       �������ַ��Ϣ���뵽���ݿ��У�������IDֵ
*����
		kal_uint32 addr - ��ʼ��ַ
		kal_uint32 len	- ����
		kal_uint32 flag	- ��־
*����ֵ
     > 0		-	��¼��IDֵ
	 == 0		-	ʧ��
*����˵��
	2018/11/28 by qinjiangwei

	CREATE TABLE dbo.mac_alloc( 
	ID INT NOT NULL IDENTITY PRIMARY KEY ,
	SEG_ID INT NOT NULL,
	SEG_ADDR INT NOT NULL , 
	SEG_LEN	INT NOT NULL,
	FLAG INT,
	CreateDate datetime not null default getdate());
********************************************************************/
kal_uint32 db_insert_alloc_record(kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 seg_id)
{
	CString strSQL;
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	strSQL = "SET NOCOUNT ON INSERT INTO product.dbo.mac_alloc(SEG_ID, SEG_ADDR, SEG_LEN, FLAG) VALUES (";
	str.Format("%d, %d, %d, %d) SELECT @@IDENTITY AS ID", seg_id, addr, len, flag);
	strSQL += str;

	DB_CHECK_HANDLE_ZER0();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return 0;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return 0;
	}

#if 0
	strSQL = "SELECT SCOPE_IDENTITY() AS ID";
	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return 0;
	}
#endif

	_variant_t vitem;
	int nr = 0;
	int id = 0;

	try
	{
		while (!ado_ptr->m_pRs->adoEOF)
		{
			vitem = ado_ptr->m_pRs->GetCollect("ID");
			id = (vitem.vt == VT_DECIMAL) ? vitem.intVal : 0;

			nr++;
			ado_ptr->m_pRs->MoveNext();
		}
		ado_ptr->m_pRs->Close();
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

	return id;
}

/*******************************************************************
*����
       ���·����ַ��Ϣ�����ݿ���
*����
		kal_uint32 addr - ��ʼ��ַ
		kal_uint32 len	- ����
		kal_uint32 flag	- ��־
		kal_uint32 id	- idֵ
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/11/28 by qinjiangwei

	CREATE TABLE dbo.mac_alloc( 
	ID INT NOT NULL IDENTITY PRIMARY KEY ,
	SEG_ID INT NOT NULL,
	SEG_ADDR INT NOT NULL , 
	SEG_LEN	INT NOT NULL,
	FLAG INT,
	CreateDate datetime not null default getdate());
********************************************************************/
Boolean db_update_alloc_record(kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 id)
{
	CString strSQL;
	CString str;
	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	strSQL = "UPDATE product.dbo.mac_alloc SET SEG_ADDR = "; //, SEG_ADDR, SEG_LEN, FLAG)
	str.Format("%d, ", addr);
	strSQL += str;
	str.Format(" SEG_LEN = %d, ", len);
	strSQL += str;
	str.Format(" FLAG = %d WHERE ID = %d", flag, id);
	strSQL += str;

	DB_CHECK_HANDLE();

	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return FALSE;
	}

	return TRUE;
}


/*******************************************************************
*����
       �����ݿ���ɾ�������ַ��Ϣ
*����
		kal_uint32 id	- idֵ
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/11/28 by qinjiangwei

	CREATE TABLE dbo.mac_alloc( 
	ID INT NOT NULL IDENTITY PRIMARY KEY ,
	SEG_ID INT NOT NULL,
	SEG_ADDR INT NOT NULL , 
	SEG_LEN	INT NOT NULL,
	FLAG INT,
	CreateDate datetime not null default getdate());
********************************************************************/
Boolean db_delete_alloc_record(kal_uint32 id)
{
	CString strSQL;
	CString str;

	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	strSQL = "DELETE FROM product.dbo.mac_alloc WHERE ID = "; //, SEG_ADDR, SEG_LEN, FLAG)
	str.Format("%d", id);
	strSQL += str;

	DB_CHECK_HANDLE();
	
	if (!ado_ptr->m_pConnection)
	{
		database_error_code = DATABASE_CODE_NOT_OPEN;
		return FALSE;
	}

	if (!ado_ptr->ExecuteNotSelSQL(strSQL))
	{
		Log_e("execute sql(%s) failed!\n", strSQL);
		return FALSE;
	}

	return TRUE;
}


/*
 * OLE ��ʼ��
 */
FUNC_DLL_EXPORT void init_ole()
{
	AfxOleInit();
}


/*
 * ����
 */
void test_ado_db()
{
	//CADOEx ado;

	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	if (ado_ptr->ConnectDB("RD-Android-06", "product", "sa", "test,123"))
	{
		//AfxMessageBox(_T("���ݿ����ӳɹ�"));
		//��Person���ݿ�loginUser���в���һ����¼��loginUser��Ķ��������
		CString strSQL = "INSERT INTO product.dbo.mac_usage(MAC, USED_COUNT, LICENSE, CUSTOMER) VALUES('001122334455', 1, 'aaaa', 'midea')";
		ado_ptr->ExecuteNotSelSQL(strSQL);
		AfxMessageBox("������һ����¼��product����");

		strSQL = "SELECT MAC, USED_COUNT, CUSTOMER FROM product.dbo.mac_usage";
		if (!ado_ptr->ExecuteNotSelSQL(strSQL))
		{
			return;
		}
		
		_variant_t vUsername, vmac, vname;
		TCHAR *pmac;
		CString mac_str;
		while (!ado_ptr->m_pRs->adoEOF)
		{
			vmac = ado_ptr->m_pRs->GetCollect("MAC");
			mac_str = VariantToString(vmac);
			pmac = mac_str.GetBuffer();

			ado_ptr->m_pRs->MoveNext();
		}
	}
	else
	{
		AfxMessageBox(_T("���ݿ�����ʧ�ܣ�"));
	}
}