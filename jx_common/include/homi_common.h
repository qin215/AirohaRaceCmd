#ifndef HOMI_COMMON_H
#define HOMI_COMMON_H
#include <windows.h>
#include "ServerCmd.h"
#include "mywin.h"

/*
 * added by qinjiangwei 2018/4/11
 * ��license file �ļ�ͷ��Ϊһ������������, ����ǰ�ĳ������
 */
#define MP_ADDRESS_OFFSET	100

#define MAC_ADDR_SIZE		6				// MAC ��ַ��С
#define MAC_ADDR_FLAG		1				// MAC��ַ�Ƿ�ռ�ñ�־ռ��һ���ֽ�
#define HOMI_ID_SIZE		5				// ID ռ�ô�С

#define MP_DATA_SIZE		sizeof(macaddr_id_t)	//(MAC_ADDR_SIZE + MAC_ADDR_FLAG + HOMI_ID_SIZE)

#define kal_uint8	unsigned char
#define kal_uint32 	unsigned int

#define DATABASE_CODE_SUCCESS	(0)
#define DATABASE_CODE_NOT_OPEN	(-1)
#define DATABASE_CODE_PARAM_ERROR (-2)
#define DATABASE_CODE_EXECUTE_ERROR (-3)

/*
 * license �ļ�ͷ���ݽṹ, Ԥ�ȷ�����100�ֽڿռ�
 */
typedef struct __license_file_header
{
	kal_uint8 chksum[4];					// У���
	kal_uint8 mac_start_addr[6];			// MAC ��ʼ��ַ
	kal_uint8 mac_end_addr[6];			// MAC ������ַ
	kal_uint8 __reserv1[4];
	kal_uint8 mac_count[4];				// ��Ҫд��MAC��ַ����
	kal_uint8 left_count[4];			// ʣ��mac��ַ
	kal_uint8 __reserv2[4];
	kal_uint8 mac_current_addr[6];		// ��ǰд��MAC��ַ
	kal_uint8 __reserv3[2];
	
	kal_uint32 id_start;
	kal_uint32 id_end;
	kal_uint32 current_id;
} lic_file_header_t;

/*
 * ������license�ļ��е�MAC��ַ��ռ�ñ�־�� ���ļ�ƫ��100����ʼ
 */
typedef struct __macaddr_id_struct
{
	kal_uint8 mac_addr[MAC_ADDR_SIZE];			// Mac Address
	kal_uint8 mac_flag;							// �Ƿ�ʹ�ñ�־, 1 ��ʾ��ʹ��, 0 ��ʾδʹ��

#if HOMI_ID_SIZE 	
	kal_uint8 id[HOMI_ID_SIZE];					// �ѷ����id
#endif
} macaddr_id_t;


#define ID_TO_UINT32(__id)		(*((kal_uint32 *)__id))


/*
 * ��¼����MAC��ַ�����ļ���ռ�õ�����
 * �ڴ�ӳ���ĵ�ַΪ: 100 + index * sizeof(macaddr_id_t)
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

#define ALIOS_DEVICE_NAME_LEN		(USER_DEVICE_NAME_LEN + 1)			// ��˾�ڲ�����ʱ������һ���ֽ�
#define ALIOS_USER_SIGNATURE_LEN	(USER_SIGNATURE_LEN + 1)			// ��˾�ڲ�����ʱ������һ���ֽ�

/* �û��Զ����ֶ� */
typedef struct user_config_struct
{
	kal_uint32 key;									/* crypt key */		// is used by jx module
	kal_uint32 crypt_value;							/* crypt value */	// is used by jx module

	kal_uint32  seq;								/*���кţ� ������*/
	char batch_num[USER_BATCH_ID_LEN];				/*���κţ� �ִ�*/
	char sw_version[USER_SW_VER_LEN];				/*����汾�� �ִ�**/
	char hw_version[USER_HW_VER_LEN];				/*Ӳ���汾�� �ִ�**/
	char product_type[USER_PRODUCT_TYPE_LEN];		/*��Ʒ�ͺ� �ִ�**/
	char signature[USER_SIGNATURE_LEN];				/*֤��, */
	char device_name[USER_DEVICE_NAME_LEN];			/* Alios device name */
	char mac_addr[MAC_ADDRESS_LEN];
	Boolean used;							// ��������iot�ļ��У�������flash��
} user_config_t;

#define MIDEA_LIC_LEN	(1472 + 1)		// ����lic�ִ�����+1
typedef struct midea_config_struct
{
	char mac_addr[MAC_ADDRESS_LEN];
	char lic_str[MIDEA_LIC_LEN];
	int count;					// ��¼��mac��ַ��¼����
} midea_lic_t;


/* 
* �ͻ�������Ϣ
*/
typedef struct user_config_mgmt_struct
{
	int index;
	user_config_t config;
} user_config_mgmt_t;


/* �����ֽ���תΪ������ */
#define TO_INT(_buf)	((_buf[0] << 24) | (_buf[1] << 16) | (_buf[2] << 8) | (_buf[3]));

/* ������תΪ�����ֽ��� */
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
* ��ȡLicense�ļ�
*/
lic_file_header_t * ReadLicenseFileOld(const TCHAR *lpszPath, int *fsize);

/*
* ����License�ļ�
*/
BOOL SaveLicenseFile(const TCHAR *lpszPath, __int64 start_addr, int write_num, kal_uint32 start_id);

/*
 * ��License File �л�ȡ���е�Mac/Id
 */
int FindFreeMachAddrOld(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr, int n);

/*
 * ���MAC/ID��ʹ��
 */
BOOL UpdateMachFlag(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr);

/*
 * ���¼���checksum, ����license
 */
BOOL SaveLicenseFile2(const TCHAR *lpszPath, kal_uint8 *pbuff, int len);

/* 
 * ���License�ļ����ݵ�һ����
 * ����ֵ:
 * 1 : ��Ϣ��ȷ
 * 0 : ��Ϣ����ȷ�����Զ�����
 * -1 : ��Ϣ����ȷ��û���Զ�����
 */
int CheckLicenseFile(const TCHAR *lpszPath);


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
BOOL GetMacIdInfo(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr);

/*
 * ���ִ�sβ��ƥ��str�ִ�
 */
char *strrstr(char *s, char *str);

/*
 * ����License�ļ��ض�����
 * lpszPath: �ļ�·��
 * pline: ָ��license�ڴ�����
 * index: ��¼����
 */
Boolean PwSaveLicenseFileIndex(const TCHAR *lpszPath, user_config_t *pline, int index);


/*
 * �ҵ���Ӧ��mac/name/secret��¼
 * ��������
 */
int AliosFindLineByMacIndex(const user_config_t *plines, const char *mac_str);

/*
 * �ҵ���Ӧ��mac/name/secret��¼
 * ���ֲ��ң��ӿ��ٶ�
 */
const user_config_t * AliosBinaryFindLineByMac(const user_config_t *plines, int n, const char *mac_str);

/*
 * ��License File �л�ȡ���е�Mac/Id
 * ���������
 *	pline: license ����
 *  paddr: �洢���ص�����
 *  n : ��Ҫ���صĸ���
 * ����ֵ��
 *	��Ч�����ݸ���
 */
int PwFindFreeMachAddrFast(user_config_t *pline, const user_config_t *pcurrent, user_config_mgmt_t *paddr, int n);

/*
 * ��ȡ��һ��δʹ�õ�MAC��ַ��¼, current Ϊ��ǰָ��
 */
const user_config_t *PwGetFirstUnusedFast(const user_config_t *pline, const user_config_t *pcurrent);

/*
 * �����ǰ��ʱ��������Գ�������ʱ��ʹ��
 */
void print_current_timestamp(const char *prompt_str);

/*
 * ��ӡWindowsϵͳ������
 */
void disp_win_sys_err_msg(const TCHAR *s);

/*
 * ��ȡ��ǰʱ�����ms��λ
 */
__int64 get_current_timestamp_ms();

/*
 * �������
 */
kal_bool win32_clear_uart_data(int port);

/*
 * �ҵ���Ӧ����License��MAC��ַ��Ӧ�ļ�¼
 */
const midea_lic_t * MideaFindLineByMac(const midea_lic_t *plines, const char *mac_str);

/*
 * ��ȡlicense�ļ���txt��ʽ��
 */
midea_lic_t * MideaReadLicenseFile(const TCHAR *lpszPath);

/*
 * ����License�ļ��ض�����
 * lpszPath: �ļ�·��
 * pline: ָ��license�ڴ�����
 * index: ��¼����
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

/*��ӡ������*/
void sz_print_pkt(char *str, kal_uint8 *data, int dataLen);

/*
 * c:\\path1\\path2\\abcc.csv ===> c:\\path1\\path2\\
 */
Boolean get_file_path(LPCTSTR lpszPath, char *new_path, int pathsize);

/*
 * ����������תΪhex����
 */
int Binary2HexData(const UCHAR * inBuff, const int len, UCHAR *outBuff, int out_len);


kal_uint16 crc16_ccitt_cal(kal_uint16 crc_start, unsigned char *buf, int len);

/*
 * ��ʼ���������ݿ�
 */
FUNC_DLL_EXPORT BOOL init_database(const char *server, const char *db, const char *user, const char *pass);

/*
 * �ر����ݿ�����
 */
FUNC_DLL_EXPORT BOOL deinit_database();

/*
 * ���MAC��ַ�Ƿ���д
 */
BOOL check_mac_used(const char *mac_str);

/*
 * �������ݿ�ļ�¼
 */
BOOL update_db_record(const char *mac_str, int used_count);

//
void test_ado_db();

/*
 * ����MAC��ַ��¼�У���ӡ��ά��ʹ��
 * lpszPath: �ļ�·��
 * new_mac_str: ��mac��ַ
 */
Boolean MideaUpdateMacAddress(const TCHAR *lpszPath, const char *new_mac_str);

/*
 * ��src�ж�ȡһ�в�����
 * src: ԭʼ������
 * next_line: ��һ�п�ʼ
 * sep:	�ָ��� \n ���� \r\n , �������NULL, ��ϵͳ�Լ��ж�
 * return:
 *			���г��ȣ�������\r\n ���� \n
 */
int get_one_line(const char *src, const char **next_line, char *sep);

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
BOOL update_db_record_info(const char *mac_str, int used_count, const char *dname, const char *dkey, const char *lic, const char *customer);

/*
 * �����ϴ�dbִ��ʱ�ĳ�����
 */
int database_get_error_code();

// ����ÿһ�еĻص�����
/*
 * line_ptr: ÿһ�е�����, ������\r\n����\n
 * param1, param2: ����
 * TRUE: �����Ѵ��������в���Ҫ��ע
 * FALSE: ����δ���������м���
 */
typedef Boolean (*line_callback)(const char *line_ptr, void *param1, void *param2);

/*
 * ����ÿһ������
 * src: ����buffer, ����һ�л��߶�������
 * fn: ��ȡ��һ��������, ���д���Ļص�����
 * param1, param2: �ص����ݵĲ���
 * ����ֵ:
 *		TRUE, �ص��Ѵ���; FALSE: �ص�δ����
 */
Boolean process_each_line(const char *src, line_callback fn, void *param1, void *param2);

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
Boolean SaveLicenseFileIndex(const TCHAR *lpszPath, const kal_uint8 *ptr, const int len, int index);


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
int FindFreeMachAddrOld2(UCHAR *pbuff, INT len, macaddr_mgmt_t *paddr, int n, const TCHAR *lpszPath);

#ifdef __cplusplus	
}
#endif


#endif