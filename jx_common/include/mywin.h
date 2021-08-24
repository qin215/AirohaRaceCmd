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


// ����ͨ�ÿ�汾,����и����������汾��
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
 * ��ȡ uart.ini ��, ĳ�����е� key ��Ӧ�� string ֵ
 */
FUNC_DLL_EXPORT Boolean get_config_string_value(const TCHAR *segment, const TCHAR *key, const TCHAR *default_value, TCHAR *out_buf, int len);

/* 
 * ��ȡ uart.ini ��, ĳ�����е� key ��Ӧ�� integer ֵ
 */
FUNC_DLL_EXPORT Boolean get_config_int_value(const TCHAR *segment, const TCHAR *key, int *pvalue, int default_value);

/* 
 * ��ȡ��ǰ����·��
 */
TCHAR* get_program_path(TCHAR *out_buf, int len);


/*
* ����CRC32У��ֵ
 */
unsigned int crc_32_calculate(unsigned char* content, int numread, unsigned int crc);


/*
* �����㷨
*/
unsigned int crc_32_crypt(unsigned char* content, int numread, unsigned int crc);

/*
 * �����¸�ʽ���ַ������滻mac1/mac2��ַ
 * 
 {
 "addr1":"44:57:18:3f:89:a1",
 "addr2":"44:57:18:3f:89:a2",
 "softap_ssid":"icomm-softap"
 }

 * ���������json_str, json��ʽ��������Ϣ
 *			 pmac1: mac1 ��ַ��������, 6���ֽ�
 *			 pmac2: mac2 ��ַ�������ƣ�6���ֽ�
 * ����ֵ��
 *			TRUE, ִ�гɹ��� json_str��mac��ַ���滻
 *			FALSE, ʧ��
 */
Boolean replace_all_mac_address(char *json_str, kal_uint8 *pmac1, kal_uint8 *pmac2);

/*
 * MAC ��ַ����n��
 * pmac: mac ��ַ, �����ֽ���
 * pmac_out: �ӷ�֮���mac��ַ, �����ֽ���
 */
Boolean mac_advance_step(const kal_uint8 *pmac, int len, int n, kal_uint8 *pmac_out);

/*
 * ���Գ���
 */
void test_replace_mac_str();

/*
 * �� 00:11:22:33:44:55�����ɶ���������
 */
void macstring_parser(char *macstr, char *addr);

/*
 * ��cmd console����,printf����ӡ���˴�
 */
FUNC_DLL_EXPORT void enable_console_window();

/*
 * ���ļ���¼log
 */
FUNC_DLL_EXPORT void enable_log_file();

/*
 * �ж�Ŀ¼�Ƿ���ڣ���������ڣ������������ͬ���ļ������ڣ���ɾ���ļ�������ͬ��Ŀ¼
 */
Boolean ensure_directory_exist(const char *path);


BOOL MyString2HexData(const char *hex_str, UCHAR * outBuffer, int len);


/*
 * �Ե�ǰʱ��������ļ���׺��
 */
char* HAL_get_current_dt_filename(void);

/*
 * �������Ƶ�MACת��ΪHEX��ʽ
 */
Boolean PwMacBin2Hex(char *pmac, int len, const kal_uint8 bin_mac[6]);

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
BOOL update_download_db_record(const char *mac_str, int used_count);

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
BOOL check_download_mac_used(const char *mac_str);

/*******************************************************************
*����
      ���Ժ���
*����
	��
*����ֵ
	��
*����˵��
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT void amalloc_test();


/*******************************************************************
*����
       ��16���������ִ�ת����ֵ
*����
     const CString& mac_str - 16�����ִ�ֵ
*����ֵ
     8�ֽ�����ֵ
*����˵��
	2018/11/28 by qinjiangwei
********************************************************************/
UINT64 MacString2Binary(const char *mac_str);


/*******************************************************************
*����
       ��MAC������ֵת��Ϊ16�����ַ���
*����
     UINT64 macbin - 8�ֽ�MAC��ַ��ֵ
*����ֵ
     char *buff	-	���ص��ִ�
	 TRUE		- ת���ɹ�
	 FALSE		- ת��ʧ��
*����˵��
	2018/11/28 by qinjiangwei
********************************************************************/
Boolean MacBinary2String(UINT64 macbin, char *buff, int len);

/*******************************************************************
*����
       ��ʼ����ַ�б�
*����
     UINT64 base		-	��ʼ��ַ
	 kal_uint32 len		-	����
*����ֵ
	��
*����˵��
	2018/11/27 by qinjiangwei
********************************************************************/
void * init_address_list(UINT64 base, kal_uint32 len, kal_uint32 seg_id);



/*******************************************************************
*����
       ����ַ��Ϣ���뵽�б��У�������
*����
     kal_uint32 addr	-	��ʼ��ַ
	 kal_uint32 len		-	����
	 kal_uint32 flag	-	ʹ�ñ�־
	 kal_uint32 id		-	���ݿ��¼id
*����ֵ
	��
*����˵��
	2018/11/27 by qinjiangwei
********************************************************************/
void add_address_list(void *handle, kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 id);


/*******************************************************************
*����
      ����һ�������ĵ�ַ�ռ�
*����
	kal_uint32 len	-	�����ĵ�ַ����
*����ֵ
	0 :	�޵�ַ�ɷ���
	> 0: ��ʼ��ַ
*����˵��
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT UINT64 addr_alloc(void *handle, kal_uint32 len);

/*******************************************************************
*����
      �ͷ�һ�������ĵ�ַ�ռ�
*����
	UINT64 base_addr	-	��ַ
	int len				-	��ַ����
*����ֵ
	TRUE :	�ͷųɹ�
	FALSE:  �ͷ�ʧ��
*����˵��
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean addr_free(void *handle, UINT64 base_addr, int len);

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
FUNC_DLL_EXPORT void* db_fetch_mac_allocate_list(const char *customer);

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
kal_uint32 db_insert_alloc_record(kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 seg_id);

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
Boolean db_delete_alloc_record(kal_uint32 id);


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
Boolean db_update_alloc_record(kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 id);


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
BOOL update_download_db_record(const char *mac_str, int used_count);


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
BOOL check_mac_used_record(const char *mac_str, const char *table);

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
BOOL update_mac_used_record(const char *mac_str, const char *table, const char *customer, int used_count);

/*
 * ��ӡWindowsϵͳ������
 */
void disp_win_sys_err_msg(const TCHAR *s);

/*******************************************************************
*����
		���logĿ¼�������ļ�����,��
*����
		��
*����ֵ
     TRUE		-	�ɹ�
	 FALSE		-	ʧ��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
void MonitorItestLogDirectory(void* lpszLogDir);

/*******************************************************************
*����
		�������̼߳��Ŀ¼�ļ��仯
*����
		��
*����ֵ
		��
*����˵��
	2018/12/6 by qinjiangwei

********************************************************************/
void StartMonitorItestLogThread(LPCTSTR logDir, char *hostname);

/*
 * ��ȡ��ǰʱ�����ms��λ
 */
__int64 get_current_timestamp_ms();


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

/*
 * OLE ��ʼ��
 */
FUNC_DLL_EXPORT void init_ole();

/*
 * ��ʼ���������ݿ�
 */
FUNC_DLL_EXPORT BOOL init_database(const char *server, const char *db, const char *user, const char *pass);

/*
 * �ر����ݿ�����
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
