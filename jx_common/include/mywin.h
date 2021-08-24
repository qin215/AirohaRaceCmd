#ifndef _MYWIN_H_
#define _MYWIN_H_

#ifdef WIN32
#include <tchar.h>
#include "windows.h"
#include <BaseTsd.h>
#else
#define TCHAR char
#endif


#include "ServerCmd.h"
#include "data_buff.h"
#ifdef __cplusplus
extern "C"
{
#endif


//#define DLL_EXPORT 1

#ifdef DLL_EXPORT
#ifdef __cplusplus
#define FUNC_DLL_EXPORT extern "C" __declspec(dllexport) 
#else
#define FUNC_DLL_EXPORT __declspec(dllexport) 
#endif

#else
#define FUNC_DLL_EXPORT
#endif


// 集贤通用库版本,如果有更改请升级版本号
#define JX_COMMON_LIB_VERSION	"1.6"

typedef struct DWORD_BITS_Struct
{
	char b0:1;
	char b1:1;
	char b2:1;
	char b3:1;
	char b4:1;
	char b5:1;
	char b6:1;
	char b7:1;
	char b8:1;
	char b9:1;
	char b10:1;
	char b11:1;
	char b12:1;
	char b13:1;
	char b14:1;
	char b15:1;
	char b16:1;
	char b17:1;
	char b18:1;
	char b19:1;
	char b20:1;
	char b21:1;
	char b22:1;
	char b23:1;
	char b24:1;
	char b25:1;
	char b26:1;
	char b27:1;
	char b28:1;
	char b29:1;
	char b30:1;
	char b31:1;
} DWORD_BITS;

typedef struct BYTES_BITS_Struct
{
	char b0:1;
	char b1:1;
	char b2:1;
	char b3:1;
	char b4:1;
	char b5:1;
	char b6:1;
	char b7:1;
} BYTES_BITS;

typedef union _DWORD_UNION
{
	DWORD_BITS bits_v;
	unsigned int int_v;
} DWORD_UNION;

typedef union _BYTE_UNION
{
	BYTES_BITS bits_v;
	unsigned char byte_v;
} BYTE_UNION;


#define RAW_DATA_VALUE_IGNORE	(-1)
#define RAW_DATA_VALUE_ERROR	(-2)

#define RAW_DATA_STAGE_FAR		1
#define RAW_DATA_STATE_NEAR		2

/* 
 * 获取 uart.ini 中, 某个段中的 key 对应的 string 值
 */
FUNC_DLL_EXPORT Boolean get_config_string_value(const TCHAR *segment, const TCHAR *key, const TCHAR *default_value, TCHAR *out_buf, int len);

/* 
 * 获取 uart.ini 中, 某个段中的 key 对应的 integer 值
 */
FUNC_DLL_EXPORT Boolean get_config_int_value(const TCHAR *segment, const TCHAR *key, int *pvalue, int default_value);

/* 
 * 获取当前程序路径
 */
TCHAR* get_program_path(TCHAR *out_buf, int len);


/*
* 计算CRC32校验值
 */
unsigned int crc_32_calculate(unsigned char* content, int numread, unsigned int crc);


/*
* 加密算法
*/
unsigned int crc_32_crypt(unsigned char* content, int numread, unsigned int crc);

/*
 * 在如下格式的字符串中替换mac1/mac2地址
 * 
 {
 "addr1":"44:57:18:3f:89:a1",
 "addr2":"44:57:18:3f:89:a2",
 "softap_ssid":"icomm-softap"
 }

 * 输入参数：json_str, json格式的配置信息
 *			 pmac1: mac1 地址，二进制, 6个字节
 *			 pmac2: mac2 地址，二进制，6个字节
 * 返回值：
 *			TRUE, 执行成功， json_str的mac地址被替换
 *			FALSE, 失败
 */
Boolean replace_all_mac_address(char *json_str, kal_uint8 *pmac1, kal_uint8 *pmac2);

/*
 * MAC 地址增加n个
 * pmac: mac 地址, 网络字节序
 * pmac_out: 加法之后的mac地址, 网络字节序
 */
Boolean mac_advance_step(const kal_uint8 *pmac, int len, int n, kal_uint8 *pmac_out);

/*
 * 测试程序
 */
void test_replace_mac_str();

/*
 * 将 00:11:22:33:44:55解析成二进制数据
 */
void macstring_parser(char *macstr, char *addr);

/*
 * 打开cmd console窗口,printf语句打印到此处
 */
FUNC_DLL_EXPORT void enable_console_window();

/*
 * 打开文件记录log
 */
FUNC_DLL_EXPORT void enable_log_file();

/*
 * 判断目录是否存在，如果不存在，则建立；如果相同的文件名存在，则删除文件，建立同名目录
 */
Boolean ensure_directory_exist(const char *path);


BOOL MyString2HexData(const char *hex_str, UCHAR * outBuffer, int len);


/*
 * 以当前时间戳生成文件后缀名
 */
char* HAL_get_current_dt_filename(void);

/*
 * 将二进制的MAC转换为HEX格式
 */
Boolean PwMacBin2Hex(char *pmac, int len, const kal_uint8 bin_mac[6]);

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
BOOL update_download_db_record(const char *mac_str, int used_count);

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
BOOL check_download_mac_used(const char *mac_str);

/*******************************************************************
*作用
      测试函数
*参数
	无
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT void amalloc_test();


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
UINT64 MacString2Binary(const char *mac_str);


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
Boolean MacBinary2String(UINT64 macbin, char *buff, int len);

/*******************************************************************
*作用
       初始化地址列表
*参数
     UINT64 base		-	起始地址
	 kal_uint32 len		-	长度
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
void * init_address_list(UINT64 base, kal_uint32 len, kal_uint32 seg_id);



/*******************************************************************
*作用
       将地址信息加入到列表中，并排序
*参数
     kal_uint32 addr	-	起始地址
	 kal_uint32 len		-	长度
	 kal_uint32 flag	-	使用标志
	 kal_uint32 id		-	数据库记录id
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
void add_address_list(void *handle, kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 id);


/*******************************************************************
*作用
      分配一段连续的地址空间
*参数
	kal_uint32 len	-	连续的地址长度
*返回值
	0 :	无地址可分配
	> 0: 起始地址
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT UINT64 addr_alloc(void *handle, kal_uint32 len);

/*******************************************************************
*作用
      释放一段连续的地址空间
*参数
	UINT64 base_addr	-	地址
	int len				-	地址长度
*返回值
	TRUE :	释放成功
	FALSE:  释放失败
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean addr_free(void *handle, UINT64 base_addr, int len);

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
FUNC_DLL_EXPORT void* db_fetch_mac_allocate_list(const char *customer);

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
kal_uint32 db_insert_alloc_record(kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 seg_id);

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
Boolean db_delete_alloc_record(kal_uint32 id);


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
Boolean db_update_alloc_record(kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 id);


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
BOOL update_download_db_record(const char *mac_str, int used_count);


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
BOOL check_mac_used_record(const char *mac_str, const char *table);

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
BOOL update_mac_used_record(const char *mac_str, const char *table, const char *customer, int used_count);

/*
 * 打印Windows系统错误函数
 */
void disp_win_sys_err_msg(const TCHAR *s);

/*******************************************************************
*作用
		监控log目录下增加文件操作,当
*参数
		无
*返回值
     TRUE		-	成功
	 FALSE		-	失败
*其它说明
	2018/12/6 by qinjiangwei

********************************************************************/
void MonitorItestLogDirectory(void* lpszLogDir);

/*******************************************************************
*作用
		开启新线程监控目录文件变化
*参数
		无
*返回值
		无
*其它说明
	2018/12/6 by qinjiangwei

********************************************************************/
void StartMonitorItestLogThread(LPCTSTR logDir, char *hostname);

/*
 * 获取当前时间戳，ms单位
 */
__int64 get_current_timestamp_ms();


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

/*
 * OLE 初始化
 */
FUNC_DLL_EXPORT void init_ole();

/*
 * 初始化连接数据库
 */
FUNC_DLL_EXPORT BOOL init_database(const char *server, const char *db, const char *user, const char *pass);

/*
 * 关闭数据库连接
 */
FUNC_DLL_EXPORT BOOL deinit_database();


void ring_buff_init();


int spp_push_tx_buf(const unsigned char * ptrData, unsigned int length);

int spp_copy_buffer(uint8_t *buffer, int size);

void test_ring_buffer();

#define sz_print_pkt sz_print_data_buf

 char * convert_binary_to_hex(kal_uint8 *data, int dataLen);

#ifdef __cplusplus
}
#endif

#endif
