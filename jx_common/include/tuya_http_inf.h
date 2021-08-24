#ifndef _TUYA_HTTP_INF_H_
#define _TUYA_HTTP_INF_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "c_types.h"
#include "tuya_com_def.h"
#include "data_buff.h"
#include "cJSON.h"


#define TUYA_DATA_VERSION 4					// attention. 

#define TUYA_HTTP_DEBUG_ON 1


#ifdef TUYA_HTTP_DEBUG_ON
#define TUYA_HTTP_DEBUG(fmt,...) os_printf(fmt, ##__VA_ARGS__)
#else
#define TUYA_HTTP_DEBUG(fmt,...)
#endif

// 0x79:SAVE1  0x7A:SAVE2  0x7B:flag
#define TUYA_PARAM_START_SEC         0x79

#define TUYA_OBJ_NAME_LEN	8
#define TUYA_OBJ_FLAG_RDONLY 1
#define TUYA_OBJ_FLAG_RDWR 2
#define TUYA_OBJ_FLAG_NULL	4

#define TUYA_MAX_OBJ_NUM	16

#define TUYA_BOOL_OBJ_TYPE	1
#define TUYA_INT_OBJ_TYPE 2


typedef struct
{
	char devId[32];
	char secKey[32];
	char localKey[32];
	char authKey[48];
	char productKey[32];
	char uuid[32];
	char muid[32];
	char macstr[32];
	
	char oldDevId[32];
	char oldSecKey[32];
	char prtl_ver[16];

	char apiUrl[32];
	char mqttUrl[32];

	uint8 register_flag;// 1:该设备已经注册, devId
	uint8 active_flag;// 该设备已经激活, secKey&localKey
	uint8 init_flag;	// 1:参数初始化, 非1:表示第一次载入参数
	int   data_version;		// 变化一次加1
	
	char token[32];		// APP token , 配网的时候提供

	Boolean is_link;	// 是否配网

	char ota_url[128];
	int  ota_size;
	char ota_md5[32 + 1];
	char ota_ver[16];
} tuya_cloud_info_t;

typedef struct
{
	unsigned char total;
	unsigned char cnt;
	char *pos[0];
} HTTP_PARAM_H_S;

#define DEF_URL_LEN 900
typedef struct
{
	HTTP_PARAM_H_S *param_h; // param handle
	char *param_in; // param insert pos
	unsigned short head_size; // head_size == "url?" or "url"
	unsigned short buf_len; // bufer len
	char buf[0]; // "url?key=value" + "kev=value statistics"
} HTTP_URL_H_S;

// aes
typedef struct
{
	//MUTEX_HANDLE mutex;
	unsigned char key[16];
	unsigned char iv[16]; // init vector
} AES_H_S;

typedef struct {
	char name[TUYA_OBJ_NAME_LEN];			// 对象名称
	int offset;								// 在flash中的存储位置
} tuya_idx_entry_t;

typedef struct
{
	uint8_t type;
	uint8_t flag;
} tuya_obj_base_t;				// 基类

typedef struct
{
	uint8_t type;
	uint8_t flag;
	Boolean v;
} tuya_obj_bool_t;			// Boolean 对象

typedef struct
{
	uint8_t type;
	uint8_t flag;
	int v;
	int min;
	int max;
	int step;
} tuya_obj_int_t;		// 整数对象

// 所有的整数都是用大端表示
// lan protocol app head
#ifdef WIN32

#pragma pack(1)
typedef struct lan_app_packet_struct
{
    UINT head; // 0x55aa			
    UINT fr_num;
    UINT fr_type;
    UINT len;
    BYTE data[0];
} lan_app_packet_t ;			// APP发送给网关的数据包

typedef struct lan_tail_packet_struct
{
    UINT crc;
    UINT tail; // 0xaa55
} lan_tail_packet_t;		// 包的结束

// lan protocol gateway head 
typedef struct lan_gw_packet_struct
{
    UINT head; // 0x55aa
    UINT fr_num;
    UINT fr_type;
    UINT len;
    UINT ret_code;
    BYTE data[0];
} lan_gw_packet_t;		// 网关回给APP的数据包

#pragma pack()

#else

typedef struct
{
    UINT head; // 0x55aa			
    UINT fr_num;
    UINT fr_type;
    UINT len;
    BYTE data[0];
} __attribute__ ((packed)) lan_app_packet_t ;			// APP发送给网关的数据包

typedef struct 
{
    UINT crc;
    UINT tail; // 0xaa55
} __attribute__ ((packed)) lan_tail_packet_t;		// 包的结束

// lan protocol gateway head 
typedef struct 
{
    UINT head; // 0x55aa
    UINT fr_num;
    UINT fr_type;
    UINT len;
    UINT ret_code;		
    BYTE data[0];
}  __attribute__ ((packed)) lan_gw_packet_t;		// 网关回给APP的数据包

#endif

extern tuya_cloud_info_t g_tuya_cloud_param;

void tuya_param_save(void);
void tuya_param_load(void);
void tuya_param_restore(void);
bool tuya_httpclient_create(void);
bool tuya_httpclient_delete(void);
int tuya_httpclient_send(uint8_t *buf, int len);
void tuya_httpclient_device_create(void);
void tuya_httpclient_device_active(void);
void tuya_httpclient_device_reset(void);
void tuya_httpclient_device_exist(void);

void tuya_emit_active_process();

void tuya_send_http_content(int sock);
void tuya_httpclient_recv_cb(char *pdata, unsigned short len);

void tuya_parse_uart_data_bufffer(buf_t *b);
void user_uart_write_data(unsigned char  *pbuff, int len);

int ICACHE_FLASH_ATTR Add_Pkcs(char *p, int len) ATTRIBUTE_SECTION_SRAM;
void ICACHE_FLASH_ATTR del_http_url_h(HTTP_URL_H_S *hu_h);
OPERATE_RET ICACHE_FLASH_ATTR httpc_aes_encrypt(const unsigned char *plain, unsigned int len, unsigned char *cipher);
int ICACHE_FLASH_ATTR StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int nLen);
OPERATE_RET ICACHE_FLASH_ATTR httpc_aes_decrypt(unsigned char *plain, const unsigned char *cipher, unsigned int len) ATTRIBUTE_SECTION_SRAM;

// tuya http schema interface
Boolean tuya_push_bool_obj(tuya_obj_bool_t *p_obj, char *obj_name);
Boolean tuya_push_integer_obj(tuya_obj_int_t *p_obj, char *obj_name);
Boolean tuya_get_obj(void *ptr, int len, char *obj_name);
void tuya_test_obj_persistent();
Boolean tuya_obj_bool_compare(tuya_obj_bool_t *b1, tuya_obj_bool_t* b2);
Boolean tuya_obj_int_compare(tuya_obj_int_t *t1, tuya_obj_int_t* t2);
Boolean tuya_parse_schema_obj(cJSON *pArray);
Boolean tuya_schema_save(void);
Boolean tuya_schema_load(void);
tuya_obj_base_t * tuya_read_obj(char *obj_name);
Boolean tuya_update_obj_value(char *obj_name, int v);
void tuya_update_obj_value_from_json(cJSON *pItem);
void tuya_read_obj_to_json(cJSON *pItem);

// tuya mqtt interface
OPERATE_RET ICACHE_FLASH_ATTR tuya_mqtt_make_reply_str(char *http_data, cJSON *pRoot);
void tuya_mqtt_do_with_cmd(char *pstr);
Boolean tuya_mqtt_check_active();
void tuya_mqtt_gen_password(OUT char *pbuff, int len, IN char *key);
void tuya_mqtt_parse_cmd(char *topic, char *cmd_buf) ATTRIBUTE_SECTION_SRAM;
OPERATE_RET tuya_mqtt_check_sign(const char *pstr, char **p_data) ATTRIBUTE_SECTION_SRAM;
OPERATE_RET tuya_mqtt_decode_data(char *p_data, int p_len, const char *key, Boolean base64) ATTRIBUTE_SECTION_SRAM;
ICACHE_FLASH_ATTR  int base64_decode( const char *base64, unsigned char *bindata ) ATTRIBUTE_SECTION_SRAM;
char *tuya_mqtt_get_buff();

// lan operation interface
void init_net_buff();
net_line_t* get_net_line(int sock) ATTRIBUTE_SECTION_SRAM;
void net_send_tx_queue(int sock, int resend, tcp_send_cb fn) ATTRIBUTE_SECTION_SRAM;
net_line_t* register_net_line(int sock) ATTRIBUTE_SECTION_SRAM;
Boolean unregister_net_line(int sock);
void tcp_send_ok_callback(int sock, tcp_send_cb fn);
void net_tcp_send_cb(int socket, char *buf, int len, Boolean resend);

Boolean tuya_lan_send_tcp_packet(int sock, buf_t *b);
Boolean tuya_lan_do_with_packet(lan_app_packet_t *head, int sock);
Boolean tuya_lan_cons_broadcast_packet(buf_t *p_buf);
Boolean tuya_lan_parse_packet(buf_t *b, int sock);
void tuya_start_tcp_server();
void tuya_test_lan_agent();
OPERATE_RET tuya_lan_check_sign(const char *pstr, char **p_data);
OPERATE_RET ICACHE_FLASH_ATTR tuya_lan_make_reply_str(char *http_data, cJSON *pRoot);

#ifdef __cplusplus
}
#endif

#endif

