#ifndef HOMI_COMMON_H
#define HOMI_COMMON_H
#include <windows.h>
#include "ServerCmd.h"
#include "mywin.h"

/*
 * added by qinjiangwei 2018/4/11
 * 将license file 文件头作为一个对象来操作, 和以前的程序兼容
 */
#define MP_ADDRESS_OFFSET	100

#define MAC_ADDR_SIZE		6				// MAC 地址大小
#define MAC_ADDR_FLAG		1				// MAC地址是否占用标志占用一个字节
#define HOMI_ID_SIZE		5				// ID 占用大小

#define MP_DATA_SIZE		sizeof(macaddr_id_t)	//(MAC_ADDR_SIZE + MAC_ADDR_FLAG + HOMI_ID_SIZE)

#define kal_uint8	unsigned char
#define kal_uint32 	unsigned int

#define DATABASE_CODE_SUCCESS	(0)
#define DATABASE_CODE_NOT_OPEN	(-1)
#define DATABASE_CODE_PARAM_ERROR (-2)
#define DATABASE_CODE_EXECUTE_ERROR (-3)

/*
 * license 文件头数据结构, 预先分配了100字节空间
 */
typedef struct __license_file_header
{
	kal_uint8 chksum[4];					// 校验和
	kal_uint8 mac_start_addr[6];			// MAC 起始地址
	kal_uint8 mac_end_addr[6];			// MAC 结束地址
	kal_uint8 __reserv1[4];
	kal_uint8 mac_count[4];				// 需要写的MAC地址数量
	kal_uint8 left_count[4];			// 剩余mac地址
	kal_uint8 __reserv2[4];
	kal_uint8 mac_current_addr[6];		// 当前写的MAC地址
	kal_uint8 __reserv3[2];
	
	kal_uint32 id_start;
	kal_uint32 id_end;
	kal_uint32 current_id;
} lic_file_header_t;

/*
 * 保存在license文件中的MAC地址和占用标志， 从文件偏移100处开始
 */
typedef struct __macaddr_id_struct
{
	kal_uint8 mac_addr[MAC_ADDR_SIZE];			// Mac Address
	kal_uint8 mac_flag;							// 是否使用标志, 1 表示已使用, 0 表示未使用

#if HOMI_ID_SIZE 	
	kal_uint8 id[HOMI_ID_SIZE];					// 已分配的id
#endif
} macaddr_id_t;


#define ID_TO_UINT32(__id)		(*((kal_uint32 *)__id))


/*
 * 记录空闲MAC地址和在文件中占用的索引
 * 内存映射后的地址为: 100 + index * sizeof(macaddr_id_t)
 */
typedef struct macaddr_mgmt_struct
{
	int index;
	macaddr_id_t addr;
} macaddr_mgmt_t;

#define USER_BATCH_ID_LEN			(20 + 1)
#define USER_SW_VER_LEN				(12 + 1)
#define USER_HW_VER_LEN				(12 + 1)
#define USER_PRODUCT_TYPE_LEN		(12 + 1)
#define USER_SIGNATURE_LEN			(64 + 1)
#define USER_DEVICE_NAME_LEN		(32 + 1)
#define MAC_ADDRESS_LEN				(12 + 1)

#define ALIOS_DEVICE_NAME_LEN		(USER_DEVICE_NAME_LEN + 1)			// 公司内部定义时增加了一个字节
#define ALIOS_USER_SIGNATURE_LEN	(USER_SIGNATURE_LEN + 1)			// 公司内部定义时增加了一个字节

/* 用户自定义字段 */
typedef struct user_config_struct
{
	kal_uint32 key;									/* crypt key */		// is used by jx module
	kal_uint32 crypt_value;							/* crypt value */	// is used by jx module

	kal_uint32  seq;								/*序列号， 二进制*/
	char batch_num[USER_BATCH_ID_LEN];				/*批次号， 字串*/
	char sw_version[USER_SW_VER_LEN];				/*软件版本号 字串**/
	char hw_version[USER_HW_VER_LEN];				/*硬件版本号 字串**/
	char product_type[USER_PRODUCT_TYPE_LEN];		/*产品型号 字串**/
	char signature[USER_SIGNATURE_LEN];				/*证书, */
	char device_name[USER_DEVICE_NAME_LEN];			/* Alios device name */
	char mac_addr[MAC_ADDRESS_LEN];
	Boolean used;							// 仅仅存于iot文件中，不存入flash中
} user_config_t;

#define MIDEA_LIC_LEN	(1472 + 1)		// 美的lic字串长度+1
typedef struct midea_config_struct
{
	char mac_addr[MAC_ADDRESS_LEN];
	char lic_str[MIDEA_LIC_LEN];
	int count;					// 记录改mac地址烧录次数
} midea_lic_t;


/* 
* 客户配置信息
*/
typedef struct user_config_mgmt_struct
{
	int index;
	user_config_t config;
} user_config_mgmt_t;


/* 网络字节序转为主机序 */
#define TO_INT(_buf)	((_buf[0] << 24) | (_buf[1] << 16) | (_buf[2] << 8) | (_buf[3]));

/* 主机序转为网络字节序 */
#define TO_NW_INT(val, _buf)	do { 	_buf[0] = (unsigned char)(val>>24);  	\
										_buf[1] = (unsigned char)(val>>16);		\
										_buf[2] = (unsigned char)(val>>8);		\
										_buf[3] = (unsigned char)(val>>0);		\
								} while (0)

#ifdef __cplusplus
extern "C"
{
#endif
/*
* 读取License文件
*/
lic_file_header_t * ReadLicenseFileOld(const TCHAR *lpszPath, int *fsize);

/*
* 生成License文件
*/
BOOL SaveLicenseFile(const TCHAR *lpszPath, __int64 start_addr, int write_num, kal_uint32 start_id);

/*
 * 从License File 中获取空闲的Mac/Id
 */
int FindFreeMachAddrOld(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr, int n);

/*
 * 标记MAC/ID已使用
 */
BOOL UpdateMachFlag(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr);

/*
 * 重新计算checksum, 保存license
 */
BOOL SaveLicenseFile2(const TCHAR *lpszPath, kal_uint8 *pbuff, int len);

/* 
 * 检查License文件数据的一致性
 * 返回值:
 * 1 : 信息正确
 * 0 : 信息不正确，已自动更正
 * -1 : 信息不正确，没法自动更正
 */
int CheckLicenseFile(const TCHAR *lpszPath);


/*
 * 获取MAC地址对应的ID值和使用标志
 *
 * 参数：
 *		pubff: license文件在内存中的开始地址
 *		len: 内存长度
 *      paddr->addr.mac_addr： 需要查找的MAC地址
 * 返回值：
 *		TRUE: 找到，paddr 中的使用标志和id值
 *		FALSE: 未找到
 */
BOOL GetMacIdInfo(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr);

/*
 * 从字串s尾部匹配str字串
 */
char *strrstr(char *s, char *str);

/*
 * 保存License文件特定的行
 * lpszPath: 文件路径
 * pline: 指向license内存数据
 * index: 记录索引
 */
Boolean PwSaveLicenseFileIndex(const TCHAR *lpszPath, user_config_t *pline, int index);


/*
 * 找到对应的mac/name/secret记录
 * 返回索引
 */
int AliosFindLineByMacIndex(const user_config_t *plines, const char *mac_str);

/*
 * 找到对应的mac/name/secret记录
 * 二分查找，加快速度
 */
const user_config_t * AliosBinaryFindLineByMac(const user_config_t *plines, int n, const char *mac_str);

/*
 * 从License File 中获取空闲的Mac/Id
 * 输入参数：
 *	pline: license 数组
 *  paddr: 存储返回的数据
 *  n : 需要返回的个数
 * 返回值：
 *	有效的数据个数
 */
int PwFindFreeMachAddrFast(user_config_t *pline, const user_config_t *pcurrent, user_config_mgmt_t *paddr, int n);

/*
 * 获取第一个未使用的MAC地址记录, current 为当前指针
 */
const user_config_t *PwGetFirstUnusedFast(const user_config_t *pline, const user_config_t *pcurrent);

/*
 * 输出当前的时间戳，调试程序运行时间使用
 */
void print_current_timestamp(const char *prompt_str);

/*
 * 打印Windows系统错误函数
 */
void disp_win_sys_err_msg(const TCHAR *s);

/*
 * 获取当前时间戳，ms单位
 */
__int64 get_current_timestamp_ms();

/*
 * 清除串口
 */
kal_bool win32_clear_uart_data(int port);

/*
 * 找到对应美的License的MAC地址对应的记录
 */
const midea_lic_t * MideaFindLineByMac(const midea_lic_t *plines, const char *mac_str);

/*
 * 读取license文件，txt格式，
 */
midea_lic_t * MideaReadLicenseFile(const TCHAR *lpszPath);

/*
 * 保存License文件特定的行
 * lpszPath: 文件路径
 * pline: 指向license内存数据
 * index: 记录索引
 */
Boolean MideaSaveLicenseFileIndex(const TCHAR *lpszPath, midea_lic_t *pline, int index);

/*
 * c:\\path1\\path2\\abcc.csv ===> c:\\path1\\path2\\abcc_postfix.csv
 */
Boolean append_filename_postfix(LPCTSTR lpszPath, char *new_path, int pathsize, const char *post_fix);

/*
 * test stub
 */
void midea_test_lic_file();

/*打印出报文*/
void sz_print_pkt(char *str, kal_uint8 *data, int dataLen);

/*
 * c:\\path1\\path2\\abcc.csv ===> c:\\path1\\path2\\
 */
Boolean get_file_path(LPCTSTR lpszPath, char *new_path, int pathsize);

/*
 * 二进制数据转为hex数据
 */
int Binary2HexData(const UCHAR * inBuff, const int len, UCHAR *outBuff, int out_len);


kal_uint16 crc16_ccitt_cal(kal_uint16 crc_start, unsigned char *buf, int len);

/*
 * 初始化连接数据库
 */
FUNC_DLL_EXPORT BOOL init_database(const char *server, const char *db, const char *user, const char *pass);

/*
 * 关闭数据库连接
 */
FUNC_DLL_EXPORT BOOL deinit_database();

/*
 * 检查MAC地址是否已写
 */
BOOL check_mac_used(const char *mac_str);

/*
 * 更新数据库的记录
 */
BOOL update_db_record(const char *mac_str, int used_count);

//
void test_ado_db();

/*
 * 保存MAC地址记录行，打印二维码使用
 * lpszPath: 文件路径
 * new_mac_str: 新mac地址
 */
Boolean MideaUpdateMacAddress(const TCHAR *lpszPath, const char *new_mac_str);

/*
 * 从src中读取一行并返回
 * src: 原始输入行
 * next_line: 下一行开始
 * sep:	分隔符 \n 或者 \r\n , 如果传入NULL, 则系统自己判断
 * return:
 *			本行长度，不包含\r\n 或者 \n
 */
int get_one_line(const char *src, const char **next_line, char *sep);

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
BOOL update_db_record_info(const char *mac_str, int used_count, const char *dname, const char *dkey, const char *lic, const char *customer);

/*
 * 返回上次db执行时的出错码
 */
int database_get_error_code();

// 处理每一行的回调函数
/*
 * line_ptr: 每一行的数据, 不包含\r\n或者\n
 * param1, param2: 参数
 * TRUE: 本行已处理，后续行不需要关注
 * FALSE: 本行未处理，后续行继续
 */
typedef Boolean (*line_callback)(const char *line_ptr, void *param1, void *param2);

/*
 * 处理每一行数据
 * src: 整行buffer, 包含一行或者多行数据
 * fn: 获取到一个完整行, 进行处理的回调函数
 * param1, param2: 回调传递的参数
 * 返回值:
 *		TRUE, 回调已处理; FALSE: 回调未处理
 */
Boolean process_each_line(const char *src, line_callback fn, void *param1, void *param2);

/*******************************************************************
*作用
       更新下载iot中数据使用记录
*参数
	 const TCHAR *lpszPath - 文件路径
	 const kal_uint8 *ptr - 缓冲区数据指针
	 const int len - 缓冲区长度
	 int index - 数据区的索引记录
*返回值
     TRUE - 更新成功
     FALSE - 更新失败
*其它说明
	2018/11/5 by qinjiangwei
********************************************************************/
Boolean SaveLicenseFileIndex(const TCHAR *lpszPath, const kal_uint8 *ptr, const int len, int index);


/*******************************************************************
*作用
       License File 中获取空闲的Mac/Id, 从数据库中比对，如果有已使用记录，则更新本地记录
*参数
	 UCHAR *pbuff - 缓冲区地址
	 INT len - 缓冲区长度
	 macaddr_mgmt_t *paddr - 返回的mac地址
	 int n - 所需要的地址个数
	 const TCHAR *lpszPath - 文件路径
*返回值
     INT - 实际可以使用的mac地址数量
*其它说明
	2018/11/5 by qinjiangwei
********************************************************************/
int FindFreeMachAddrOld2(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr, int n, const TCHAR *lpszPath);

#ifdef __cplusplus	
}
#endif


#endif