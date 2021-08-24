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
 * 数据表格名称：
 *			mac_usage				--	MAC地址使用记录表，美的/ALIOS项目扫描二维码更新LICENSE时使用
 *			mac_readback_usage		--	MAC地址回读检查记录表，美的项目回读检查使用
 *			mac_alloc				--	蓝牙项目分配蓝牙地址的记录表
 *			mac_seg					--	蓝牙项目蓝牙地址段分配记录表
 */

/*
 * 连接数据库类，通用的操作封装在一起
 */
class CADOEx 
{
public:
	CADOEx();

	// 记录集对象
	_RecordsetPtr m_pRs;
	// 命令对象
	_CommandPtr m_PCmd;

	_ConnectionPtr m_pConnection;
	// 连接数据库
	BOOL ConnectDB(const CString& server, const CString& db, const CString& user, const CString& password);

	// 执行更新操作
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
 * 连接数据
 * server: 服务器名
 * db: 数据库名
 * user: 用户名
 * password: 密码
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
			//AfxMessageBox(_T("连接到数据库失败！"));
			return FALSE;
		}
		
		Log_d("connect database ok!\n");
		return TRUE;
	}
	catch (const _com_error& e)
	{
		Log_e("connect server(%s) db(%s) as user(%s) pass(%s) failed!\n", server, db, user, password);
		//AfxMessageBox(_T("连接到数据库失败！"));
		return FALSE;
	}
}

/*
 * 执行SQL语句
 */
BOOL CADOEx::ExecuteNotSelSQL(CString strNotSelSQL)
{
	try
	{
		_variant_t vResult;

		vResult.vt = VT_ERROR;
		vResult.scode = DISP_E_PARAMNOTFOUND;

		//设置关联的Connection对象
		m_PCmd->ActiveConnection = m_pConnection;
		//SQL命令
		m_PCmd->CommandText = (_bstr_t)strNotSelSQL;
		//执行命令
		m_pRs = m_PCmd->Execute(&vResult, &vResult, adCmdText);

	}
	catch (_com_error& e)
	{
		Log_e("execute sql(%s) failed!\n", strNotSelSQL);
		database_error_code = DATABASE_CODE_EXECUTE_ERROR;
		//显示错误信息
		//AfxMessageBox(e.ErrorMessage());
		return FALSE;
	}
	Log_d("execute sql(%s) ok!\n", strNotSelSQL);
	return TRUE;
}

/*
 * 初始化连接数据库
 */
FUNC_DLL_EXPORT BOOL init_database(const char *server, const char *db, const char *user, const char *pass)
{
	Log_d("jx common dll version:%s", JX_COMMON_LIB_VERSION);

	//创建连接对象实例
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
		//AfxMessageBox(_T("创建连接对象实例失败！"));
		return FALSE;
	}

	//创建命令对象实例
	hr = ado_ptr->m_PCmd.CreateInstance(__uuidof(Command));
	if (!SUCCEEDED(hr))
	{
		Log_e("create command instance failed!\n");
		database_error_code = DATABASE_CODE_EXECUTE_ERROR;
		//AfxMessageBox(_T("创建命令对象失败"));
		return FALSE;
	}

	return ado_ptr->ConnectDB(server, db, user, pass);
}

/*
 * 关闭数据库连接
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
 * 检查MAC地址是否已写
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
*作用
       检查下载表中MAC地址是否已写
*参数
     const char *mac_str - mac地址的hex字符串
*返回值
     TRUE - 已使用
     FALSE - 未使用
*其它说明
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
 * 更新数据库的记录
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
*作用
       更新下载表中数据库的记录
*参数
     const char *mac_str - mac地址的hex字符串
	 int used_count - 使用的次数
*返回值
     TRUE - 更新成功
     FALSE - 更新失败
*其它说明
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
*作用
      检查MAC地址是否已记录
*参数
     const char *mac_str - mac地址字串
	 const char *table	-	数据表名称
*返回值
	 TRUE		-	MAC地址已经记录
	 FALSE		-	MAC地址未记录
*其它说明
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
*作用
      更新MAC地址使用记录
*参数
     const char *mac_str - mac地址字串
	 const char *table	-	数据表名称
	 int used_count		-	使用次数
	 const char *customer	-	客户名
*返回值
	 TRUE		-	更新成功
	 FALSE		-	更新不成功
*其它说明
	为保证效率，目前未判断记录是否已经存在，使用时请注意
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
 * 更新数据库的记录
 * mac_str: MAC 地址
 * used_count: 使用次数
 * dname: device name
 * dkey: device key
 * lic: license数据
 * customer: 客户名
 * 返回值：
 * TRUE: 执行成功
 * FALSE: 执行失败
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
 * 返回上次db执行时的出错码
 */
int database_get_error_code()
{
	return database_error_code;
}

/*******************************************************************
*作用
       将数据库中的记录表格下载到本地内存中
*参数
     const char *customer - mac地址的客户名
*返回值
     != 0		-	列表handle
	 == 0		-	失败
*其它说明
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
*作用
       将分配地址信息插入到数据库中，并返回ID值
*参数
		kal_uint32 addr - 起始地址
		kal_uint32 len	- 长度
		kal_uint32 flag	- 标志
*返回值
     > 0		-	记录的ID值
	 == 0		-	失败
*其它说明
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
*作用
       更新分配地址信息到数据库中
*参数
		kal_uint32 addr - 起始地址
		kal_uint32 len	- 长度
		kal_uint32 flag	- 标志
		kal_uint32 id	- id值
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
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
*作用
       从数据库中删除分配地址信息
*参数
		kal_uint32 id	- id值
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
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
 * OLE 初始化
 */
FUNC_DLL_EXPORT void init_ole()
{
	AfxOleInit();
}


/*
 * 测试
 */
void test_ado_db()
{
	//CADOEx ado;

	CADOEx *ado_ptr = (CADOEx *)ado_handle;

	if (ado_ptr->ConnectDB("RD-Android-06", "product", "sa", "test,123"))
	{
		//AfxMessageBox(_T("数据库连接成功"));
		//在Person数据库loginUser表中插入一条记录，loginUser表的定义见上文
		CString strSQL = "INSERT INTO product.dbo.mac_usage(MAC, USED_COUNT, LICENSE, CUSTOMER) VALUES('001122334455', 1, 'aaaa', 'midea')";
		ado_ptr->ExecuteNotSelSQL(strSQL);
		AfxMessageBox("插入了一条记录进product表中");

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
		AfxMessageBox(_T("数据库连接失败！"));
	}
}