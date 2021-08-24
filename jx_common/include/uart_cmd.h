#ifndef _UART_CMD_H_
#define _UART_CMD_H_

#include "ServerCmd.h"
#include "data_buff.h"

#ifndef WIN32
#include "UA_Func.h"
#endif



#ifdef __cplusplus
extern "C"
{
#endif

#ifndef U8 
#define U8 unsigned char 
#endif

#ifdef WIN32
	////Customer AT Command
#define MAX_ARGUMENT 24
	typedef struct _stParam
	{
		char*	argv[MAX_ARGUMENT];
		int	argc;
	} stParam;

	typedef int (*PF_handle) (stParam *param);
	typedef struct _at_cmd_info
	{
		char*		atCmd;
		PF_handle	pfHandle;
		U8 maxargu;
	} at_cmd_info;
#endif

#ifdef __CUSTOMER_HOMI_IOT__
#define	HX_ATRSP_OK                    "+ok"
#else
#define	HX_ATRSP_OK                    "+OK"
#endif

#define	HX_ATRSP_ERROR                 "+ERROR:%d\r\n"
#define	HX_ATRSP_ERROR_STR           	"+ERROR:%s\r\n"

#define FM_START_CHAR 'H'
#define FM_MIN_LENGTH	6
#define FM_MAX_LENGTH	255

typedef enum 
{
	UPLINK_PACKET = 0x1,			// MCU->WIFI->CLOUD
	DOWNLINK_PACKET,				// CLOUD->WIFI->MCU

	MODULE_PACKET = 0xFE,			// MCU->WIFI
	ERROR_PACKET = 0xFF

} fm_type_t;

typedef enum 
{
	ERR_OP = 0X1,		/* 操作错误 */
	ERR_CHKSUM,			/* 校验和错误 */
	ERR_RANGE,			/* 数据范围错误*/
	ERR_NO_SUPPORT		/* 命令不支持 */
} fm_error_t;


#define PACK_HEADER_LEN 4
#define PACK_CHKSUM_LEN 1

typedef struct {
	U8 header;		// 包头
	U8 len;			// 包长
	U8 type;		// 包类型
	U8 seq;			// 包序号
	U8 data[1];		// 有效数据
	//U8 chksum;	// 最后一个为校验和
} fm_packet_t;

typedef enum 
{
	MOD_QUERY_CMD = 0x1,		// 模块查询工作状态和信号强度
	MOD_REBOOT_CMD,				// 模块重启
	MOD_RESTORE_FACTORY_CMD,	// 恢复出厂设置
	MOD_CONFIG_CMD,				// 配置
	MOD_SLEEP_CMD,				// 关闭射频进入休眠模式
	MOD_WAKEUP_CMD,				// 休眠唤醒
	MOD_GET_VERSION_CMD,		// 版本号查询
	MOD_GET_PRODKEY_CMD,		// 获取 prodkey
	MOD_FACTORY_TEST_CMD,		// 进入厂测模式
	MOD_SET_PRODKEY_CMD			// 设置 prodkey
} module_cmd_t;

typedef enum 
{
	HK_STA_MODE = 0X1,
	HK_HEKR_CONFIG_MODE,
	HK_AP_MODE,
	HK_STA_AND_AP_MODE,
	HK_SLEEP_MODE
} wifi_mode_t;

typedef enum 
{
	ST_NORMAL_ROUTE = 0X1,
	ST_FAILED_ROUTE,
	ST_CONNECTING_ROUTE,
	ST_PASSWORD_ERROR,
	ST_NO_SIGNAL,
	ST_ROUTE_TIMEOUT
} wifi_status_t;

typedef enum 
{
	ST_NORMAL_CLOUD = 0x1,
	ST_DNS_ERROR,
	ST_CLOUD_TIMEOUT
} cloud_status_t;

typedef enum 
{
	NO_SIGNAL,
	SIGNAL_LEVEL_1,
	SIGNAL_LEVEL_2,
	SIGNAL_LEVEL_3,
	SIGNAL_LEVEL_4,
	SIGNAL_LEVEL_5
} wifi_signal_t;

enum reply_slot 
{
	INDEX_MODE,
	INDEX_WIFI_STATUS,
	INDEX_CLOUD_STATUS,
	INDEX_SIGNAL,
	INDEX_RESERVED,
	INDEX_REPLY_END
};

enum version_type 
{
	VER_TYPE_NONE,
	VER_TYPE_A,
	VER_TYPE_B
};

enum get_version_slot
{
	INDEX_MAJOR_VERSION,
	INDEX_MINOR_VERSION,
	INDEX_COMPILE_SEQ,
	INDEX_VERSION_TYPE,
	INDEX_VER_END
};

enum factory_mode_type 
{
	FM_TYPE_ENABLE,
	FM_TYPE_DISABLE
};


enum factory_mode_slot
{
	INDEX_FM_FLAG,
	INDEX_FM_END
};

typedef enum
{
	BLK_NONE, 	
	BLK_UDP_NSLOOKUP,		// udp客户模式下获取ip地址
	BLK_TCP_NSLOOKUP,
	BLK_UDP_CONNECT,		// udp服务器模式下等待客户连接
	BLK_TCP_CONNECT,
	BLK_WSCAN,
	BLK_WJOIN,
	BLK_TCP_CLOSE,
	BLK_TCP_CONNECTB
} block_type_t;

enum 
{
	HX_CONNECT_OK = 0,
	HX_CONNECT_ERROR,
	HX_DISCONNECT
};


typedef void (*uart_rsp_cb_t)(buf_t *b);
typedef void (*uart_rsp_ex_cb_t)(buf_t *b, int port);			// 增加回调的端口号

#if defined(__HX_IOT_SUPPORT__)

#define HX_ATCMD_NULL	"AT+"        
#define HX_ATCMD_ATLT	"AT+ATLT"               
#define HX_ATCMD_ATM	"AT+ATM"            
#define HX_ATCMD_ATPT	"AT+ATPT"               
#define HX_ATCMD_ATRM	"AT+ATRM"           
#define HX_ATCMD_BSSID	"AT+BSSID"           

#define HX_ATCMD_BRDSSID	"AT+BRDSSID"	        
#define HX_ATCMD_CHL	"AT+CHL"            
#define HX_ATCMD_CHLL	"AT+CHLL"              
#define HX_ATCMD_CMDM	"AT+CMDM"           
#define HX_ATCMD_DNS	"AT+DNS"        

#define HX_ATCMD_E	"AT+E"

#define HX_ATCMD_ENCRY	"AT+ENCRY"             
#define HX_ATCMD_ENTM	"AT+ENTM"               
#define HX_ATCMD_ENTS	"AT+ENTS"           
#define HX_ATCMD_ESPC	"AT+ESPC"                  

#define HX_ATCMD_ESPT	"AT+ESPT"            
#define HX_ATCMD_IOM	"AT+IOM"         
#define HX_ATCMD_KEY	"AT+KEY"             
#define HX_ATCMD_LKSTT	"AT+LKSTT"             
#define HX_ATCMD_NIP	"AT+NIP"              


#define HX_ATCMD_PASS	"AT+PASS"           
#define HX_ATCMD_PMTF	"AT+PMTF"                  
#define HX_ATCMD_QMAC	"AT+QMAC"            
#define HX_ATCMD_QVER	"AT+QVER"         
#define HX_ATCMD_RSTF	"AT+RSTF"             

#define HX_ATCMD_SKCLS	"AT+SKCLS"                  
#define HX_ATCMD_SKCT	"AT+SKCT"            
#define HX_ATCMD_SKRCV	"AT+SKRCV"         
#define HX_ATCMD_SKSDF	"AT+SKSDF"            


#define HX_ATCMD_SKSND	"AT+SKSND"                  
#define HX_ATCMD_SKSTT	"AT+SKSTT"           
#define HX_ATCMD_SSID	"AT+SSID"         
#define HX_ATCMD_UART	"AT+UART"            


#define HX_ATCMD_WARC	"AT+WARC"                  
#define HX_ATCMD_WARM	"AT+WARM"           
#define HX_ATCMD_WATC	"AT+WATC"         
#define HX_ATCMD_WBGR	"AT+WBGR"            

#define HX_ATCMD_WEBS	"AT+WEBS"                  
#define HX_ATCMD_APST	"AT+APST"           
#define HX_ATCMD_WJOIN	"AT+WJOIN"         
#define HX_ATCMD_WLEAV	"AT+WLEAV"           


#define HX_ATCMD_WPRT	"AT+WPRT"                  
#define HX_ATCMD_WPSM	"AT+WPSM"           
#define HX_ATCMD_WSCAN	"AT+WSCAN" 

#define HX_ATCMD_WAPST	"AT+WAPST" 

#define HX_ATCMD_IOC	"AT+IOC"

#define HX_ATCMD_Z	"AT+Z"

#define HX_ATCMD_SMARTLINK	"AT+SMTLNKREBOOT"

#define HX_ATCMD_AUTO_CONN	"AT+AUTOCONN"
#define HM_ATCMD_AUTO_CONN_B	"AT+AUTOCONNB"

#define HX_ATCMD_AUTO_CONN_START	"AT+ACONNSTART"
#define HM_ATCMD_AUTO_CONN_B_START	"AT+ACONNBSTART"

#define HM_ATCMD_WMODE	"AT+WMODE"
#define HM_ATCMD_RELD	"AT+RELD"
#define HM_ATCMD_NETP	"AT+NETP"
#define HM_ATCMD_TCPLK	"AT+TCPLK"
#define HM_ATCMD_TCPTO	"AT+TCPTO"
#define HM_ATCMD_TCPDIS	"AT+TCPDIS"

#define HM_ATCMD_SOCKB	"AT+SOCKB"
#define HM_ATCMD_TCPLKB	"AT+TCPLKB"
#define HM_ATCMD_TCPTOB	"AT+TCPTOB"
#define HM_ATCMD_TCPDISB	"AT+TCPDISB"

#define HM_ATCMD_WSKEY	"AT+WSKEY"
#define HM_ATCMD_WSSSID	"AT+WSSSID"

#define HM_ATCMD_WANN	"AT+WANN"
#define HM_ATCMD_WSMAC	"AT+WSMAC"

#define HM_ATCMD_WSLK	"AT+WSLK"
#define HM_ATCMD_SMTLK	"AT+SMTLK"

#define HX_AT_RSP_LINE_END "\r\n\r\n"
#define HX_AT_STARTUP	"+startup\r\n\r\n"


#define HX_ERROR_SUCCESS (0)
#define HX_ERROR_INVALID_CMD (-1)
#define HX_ERROR_NOT_SUPPORT (-2)
#define HX_ERROR_INVALID_OP (-3)

#define HX_ERROR_INVALID_PARAMETER (-4)
#define HX_ERROR_OP_REFUSED (-5)
#define HX_ERROR_NO_MEMORY (-6)

#define HX_ERROR_FLASH_ERROR (-7)
#define HX_ERROR_JOIN_NETWORK_FAIL (-10)
#define HX_ERROR_NO_SOCK (-11)

#define HX_ERROR_INVALID_SOCK (-12)
#define HX_ERROR_CONNECT_SOCK (-13)
#define HX_ERROR_UNKOWN (-100)

#define HX_ERROR_BLOCK (-200)

#define HX_AT_CMD_QUERY_CHAR '?'
#define HX_AT_CMD_SAVE_CHAR '!'

#define HX_NO_DATA (-1)
#define HX_DATA_NOT_FULL (1)
#define HX_DATA_FULL (2)


#define HX_PROCESS_EVENT_AUTO_CONN	(0xA1)
#define HX_PROCESS_EVENT_AUTO_CONNB		(0xA2)

#define HX_SOCK_CONN_TIME	2
#define HX_MAX_RECONN_COUNT 1

typedef enum 
{
	HX_TCP_CLIENT_SOCKET = 0,
	HX_TCP_SERVER_SOCKET,
	HX_UDP_CLIENT_SOCKET,
	HX_UDP_SERVER_SOCKET,

#ifdef __CUSTOMER_HOMI_IOT__	
	HX_TCP_CLIENT_SOCKETB,		//
	HX_TCP_SERVER_SOCKETB,
	HX_UDP_CLIENT_SOCKETB,
	HX_UDP_SERVER_SOCKETB,
#endif

	HX_SOCKET_NUM
} hx_socket_t;

void hx_set_sock(int sock, hx_socket_t type);
int hx_get_sock(hx_socket_t type);
void hx_set_conn_success(hx_socket_t sok_index);

int hx_stop_udp_server();
int hx_get_udp_server_sock();
void hx_get_udp_server_addr(uip_ipaddr_t* ip, int *port);
int hx_start_udp_connection(hx_socket_t sk_index);
void hx_set_udp_cli_addr(uip_ipaddr_t addr, int port, hx_socket_t sk_index);
int hx_get_esp_char();
int hx_get_esp_time();
int hx_get_frame_length();
Boolean hx_get_data_flag()  ATTRIBUTE_SECTION_SRAM;
int hx_parse_at_cmd()  ATTRIBUTE_SECTION_SRAM;
Boolean hx_check_exit_data_flag() ATTRIBUTE_SECTION_SRAM;
int hx_get_atpt_time();
int hx_check_sok_send();
void hx_clear_sok_sendsize();
int hx_check_sok_recv(buf_t *b);
void hx_clear_sok_recvsize();
void hx_set_current_sock(int sock);
void net_tcp_send_cb(int socket, char *buf, int len, Boolean resend) ATTRIBUTE_SECTION_SRAM;;
int hx_do_with_uart_frame() ATTRIBUTE_SECTION_SRAM;
int hx_get_current_sock() ATTRIBUTE_SECTION_SRAM;
int hx_do_with_blk(block_type_t blk);
int hx_do_with_net_frame(buf_t *b, int sock);
int hx_get_udp_cli_sock(hx_socket_t sk_index);
sock_status_t hx_start_udp_server(int udp_port, int timeout);
int hx_get_tcp_server_sock();
sock_status_t hx_start_tcp_server(int port, int timeout);

void hx_stop_tcp_server();
void hx_stop_udp_client(hx_socket_t sk_index);

char *hx_get_current_ssid();
char *hx_get_current_bssid();
Boolean hx_set_current_ssid(char *s);
Boolean hx_set_current_key(char *k);
Boolean hx_set_encrypt_mode(int mode);
void hx_notify_uart_process(int blk);
int hx_get_current_encrypt_mode();
char *hx_get_current_key();
int hx_get_wifi_connect_status();
void hx_get_peer_address(uip_ipaddr_t *ip, int *port);
void hx_get_client_peer_address(uip_ipaddr_t *ip, int *port);
void hx_get_udp_client_peer_address(uip_ipaddr_t *ip, int *port,  hx_socket_t sk_index);
int hx_get_blk_type();
void hx_init_parameters();
void hx_init_conn_parameters();
Boolean hx_check_auto_connect();
int hx_emit_skct_at_cmd();
void hx_update_ap_mode(Boolean *p_ap_mode);
void hx_report_startup();
void hx_update_gpio1_status(U8 *p_gpio1_status);
int hm_get_tcp_timeout();
int hm_get_tcpb_timeout();
Boolean hx_init_sockb_parameter(hx_sock_t *p_sock_info);
void hx_update_sockb_parameter(hx_sock_t *p_sock_info);
void hx_start_reboot_timer();
int net_get_tcp_close_reason();
int net_get_tcpb_close_reason();
int hx_emit_skct_start_at_cmd();
int net_get_sokb();

sock_status_t net_tcp_connect_serverb(char *hostname, int port);

void net_set_tcp_close_reason(int res);
void net_set_tcpb_close_reason(int res);
void hx_do_check_auto_connect();
void hx_at_rsp_ok(char *cmd, int code);
int hm_emit_auto_connb_at_cmd_start();
void hm_start_conn_ap();

Boolean hm_init_smtlnk_flag(Boolean *p_flag);
void hm_update_smtlnk_flag(Boolean *p_flag);
int hx_get_active_udp_sock();

void net_add_rx_net_line_all() ATTRIBUTE_SECTION_SRAM;
void net_send_net_line_all() ATTRIBUTE_SECTION_SRAM;
#endif

typedef enum 
{
	SOCK_STATUS_FAIL,
	SOCK_STATUS_SUCCESS,
	SOCK_STATUS_BLOCK,
	SOCK_STATUS_NUM
} sock_status_t;

enum
{
	HX_SCAN,
	HX_CONN
};

enum
{
	HX_TCP_SOCK_INITIAL,
	HX_TCP_SOCK_TIMEOUT,
	HX_TCP_SOCKB_TIMEOUT
};

enum 
{
	HX_HEX_FORMAT,
	HX_ASCII_FORMAT
};

typedef struct 
{
	int sok_protocol;
	int sok_cs;
	int sok_port;
	char sok_hostname[64];		// 主机名或者ip地址
	int sok_timeout;
} hx_sock_t;

typedef struct 
{
	int uart_baud_rate;
	int uart_data_bit;
	int uart_stop_bit;
	int uart_parity;
} hx_uart_t;

#define HX_CLIENT_MODE 0
#define HX_SERVER_MODE 1

#define HX_TCP_PROTOCOL 0
#define HX_UDP_PROTOCOL 1

#define MAX_PORT_NUM 256

#ifdef WIN32
#define HX_WIN32_UART_PORT	2
UINT win32_uart_recv_thread(LPVOID pParam);
UINT win32_uart_recv_thread_ex(LPVOID pParam);
#endif

#define HX_SSID_LEN	33
#define HX_BSSID_LEN 13

#define HX_ESP_CHAR_NUM 	3
typedef void (*uart_cb_fn_t)(void *);

extern buff_queue_t uart_tx_queue;
extern buf_t uart_rx_buff;

extern buff_queue_t net_tx_queue;
extern buf_t net_rx_buff;
extern Boolean hx_auto_conn_startup_flag;

extern SOCKET net_hekr_sock;

void udp_send_ok_callback(int sock, tcp_send_cb fn);
void test_uart_cmd();
void test_cmd();
void send_uart();
Boolean init_uart_buff();
void init_net_buff();
void unlink_uart2sock(int s);
void link_uart2sock(int s) ATTRIBUTE_SECTION_SRAM;
net_line_t* get_net_line(int sock) ATTRIBUTE_SECTION_SRAM;
void net_send_tx_queue(int sock, int resend, tcp_send_cb fn) ATTRIBUTE_SECTION_SRAM;
net_line_t* register_net_line(int sock) ATTRIBUTE_SECTION_SRAM;
Boolean unregister_net_line(int sock);
void tcp_send_ok_callback(int sock, tcp_send_cb fn);
net_line_t * uart_get_net_line();
Boolean add_uart_tx_buffer();
fm_packet_t * get_packet(buf_t *b, int *error);
void do_with_packet(fm_packet_t *pack);
fm_packet_t * parse_data_buff();
void test_json();
void uart_send_callback(Boolean bOk);
Boolean uart_send_buf(buf_t *b, send_cb fn);
void obd_log_uart_rx_data(buf_t *b);

//add by cbk 20180829
Boolean init_uart_buff_ex();
Boolean init_serial_port(int port);
void clear_uart_rx_buff(int port);
Boolean get_uart_rx_buff(int port, unsigned char *out_buf, int len,int *actual_len);
//////////////////////

#ifndef WIN32
void init_data_uart_port(BAUD_RATE rate, uart_cb_fn_t fn);
#endif

void deinit_data_uart_port();

void net_udp_send_cb(int socket, char *buf, int len, Boolean resend);
sock_status_t net_tcp_connect_server(char *hostname, int port, Boolean isLogin);

void add_ip_to_table(const char *host, kal_uint8 ip[]);
kal_uint8* get_ip_from_cache(char *host);
U8 calc_chksum(fm_packet_t *pack);
fm_packet_t* uart_build_report_status();

void test_it();
fm_packet_t * get_packet(buf_t *b, int *error);

#ifdef WIN32

//typedef unsigned char   TBYTE;
typedef unsigned short  TWORD;
//typedef	unsigned long		TDWORD;
typedef unsigned int    TDWORD;

typedef unsigned char Tu8;
typedef unsigned short Tu16;
typedef unsigned int Tu32;
typedef signed char Ts8;
typedef signed short Ts16;
typedef signed int Ts32;
typedef long long	Ts64;
typedef unsigned long long Tu64;

typedef enum t_TAG_SECURITY
{
	NONE = 0,
	WEP,
	WPA,
	WPA2,
	WPAWPA2,
} TAG_SECURITY;

typedef struct _SVersion
{
	TDWORD	dwMask;
	TBYTE	szBuildTime[32];
	TBYTE	szPrjName[26];
	TBYTE	btHWVer;
	TBYTE	btSWVer;
	TBYTE	szRCVer[40];
} SVersion;


#define U16 unsigned short
typedef union uip_ip4addr_t {
  U8  u8[4];			/* Initializer, must come first. */
  U16 u16[2];
} uip_ip4addr_t;

#ifndef HTTP_HEADER_STATUS
#define HTTP_HEADER_STATUS
typedef enum {
	HTTP_STATUS_INIT = 0,
	HTTP_STATUS_GET_HEADER,
	HTTP_STATUS_GET_CONTENT,
	HTTP_STATUS_GET_ALL,
	HTTP_STATUS_CHUNKED,
	HTTP_STATUS_ERROR
} Http_Header_Status;
#endif

typedef uip_ip4addr_t uip_ipaddr_t;

#define STR_FILE_LEN	"filelength"
#define STR_DATA	"data"
#define STR_RSP_OK		"+OK\r\n"
#define STR_RSP_ERR		"+ERROR:"
#define STR_HEAT_BEAT	"+HEARTBEAT\r\n"
#define LINE_END	"\r\n"
#define WRITE_PAGE_SIZE (2 * 1024)

enum 
{
	NOT_CHECK_ALL = -6,
	VERSION_IS_LOW = -5,
	CUSTOMER_NOT_EQUAL = -4,
	DATA_CKSUM_ERROR = -3,
	DATA_CRC_ERROR = -2,
	DATA_FORMAT_ERROR = -1,
};

typedef Boolean (*at_rsp_fn)(buf_t *b, int *code);

typedef struct at_cmd_t {
	char *cmd_str;
	int cmd;
	at_rsp_fn func;
} at_cmd_t; 

#define U32 unsigned int
#define  D_szRCVer  "N"
U8 get_local_mac(U8 *mac, U8 len);
kal_bool win32_UART_Open(int port);
kal_bool win32_UART_Open_ex(int port);
kal_uint16 win32_UART_PutBytes(int port, kal_uint8 *Buffaddr, kal_uint16 Length);
kal_uint16 win32_UART_GetBytes(int port, kal_uint8 *Buffaddr, kal_uint16 Length, kal_uint8 *status);
void win32_UART_Close(int port);
fm_packet_t *build_echo_packet(fm_packet_t *pack);
void test_http_url(char *url);
char *read_file_memory(const char *filename, int *plen);
void ota_send_file_length(int sock);
void ota_check_customer(int sock);
void ota_check_version(int sock);
kal_uint16 crc16(unsigned char *addr, int num, kal_uint16 crc);
Boolean ota_parse_recv_buff(int sock, buf_t *b);
void obd_log_recv_buff(buf_t *b);

#endif

Boolean homi_conn_server(char *server, int port);
void homi_do_with_uart_rsp(buf_t *b);
void homi_test_send_uart();
buf_t* uart_build_file_buff();

UINT win32_udp_recv_thread(LPVOID pParam);
Boolean start_udp_server();
void hx_test_http();
Http_Header_Status httprsp_parse(U8 *httprsp, U16 rsplen);
void hx_send_http_content(int sock);

extern SVersion	S_Ver;

extern Boolean bPosOk;
void http_get_weather_from_yahoo();

void register_uart_rsp_func(uart_rsp_cb_t fn);

/*
 * 注册串口数据回调函数
 */
void register_uart_rsp_ex_func(uart_rsp_ex_cb_t fn);

/*
 * 将串口数据记录到文件中
 *	b : 缓冲区数据
 *	port: 串口端口号
 */
void obd_log_uart_rx_data_ex(buf_t *b, int port);

#ifdef __cplusplus
}
#endif
#endif
