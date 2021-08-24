#ifndef __DATA_BUFF_H__
#define __DATA_BUFF_H__

#ifdef WIN32
#include "windows.h"
#include "winsock.h"
#endif


#include "ServerCmd.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TEST 1
//#define __HEKR_CLOUD_SUPPORT__ 1
//#define __OBD_OTA_TEST__ 1
//#define __HOMI_TEST__ 1
#define __OBD_TEST__ 1
//#define __OBD_RECV_TEST__ 1

//#define __HXWL_UDP_RECV_TEST__ 1

//#define __UDP_SDK_TEST__ 1
//#define __UDP_SDK_RECV_TEST__ 1

//#define __UA402_RECV_TEST__ 1

//#define __HTTP_TEST__ 1

//#define __TUYA_HTTP_TEST__ 1
//#ifdef __TUYA_HTTP_TEST__
//#define __TUYA_CLOUD_SUPPORT__ 1
//#endif
//#define __TUYA_UART_TEST__ 1

//#define __HXWL_TEST__ 1

#ifndef Boolean 
#define Boolean int
#endif

#ifndef kal_uint16
#define kal_uint16 unsigned short
#endif

#ifndef kal_uint8
#define kal_uint8 unsigned char
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define DBG_LOG_APP_AIRAPP printf


#define ATTRIBUTE_SECTION_SRAM 

enum 
{
	BLK_SIZE_16 = 0,
	BLK_SIZE_32 = 1,
	BLK_SIZE_64 = 2,
	BLK_SIZE_128 = 3,
	BLK_SIZE_256 = 4,
	BLK_SIZE_512 = 5,
	BLK_SIZE_544 = 6,		// for mediea project
	BLK_SIZE_1024 = 7,
	BLK_SIZE_2048 = 8,
	BLK_NUM
};

#define NR_BLK_SIZE_16		(160)	/*与机智云中对接时，有很多小数据*/
#define NR_BLK_SIZE_32		(128)		/*与机智云中对接时，有很多小数据*/
#define NR_BLK_SIZE_64		(128)
#define NR_BLK_SIZE_128		(16)
#define NR_BLK_SIZE_256		(16)
#define NR_BLK_SIZE_512		(4)
#define NR_BLK_SIZE_544		(4)
#define NR_BLK_SIZE_1024	(1)
#define NR_BLK_SIZE_2048	(100)		/*主要用于与服务器的http交互*/

#define BUF_16_SIZE			(16)
#define BUF_32_SIZE			(32)
#define BUF_64_SIZE			(64)
#define BUF_128_SIZE 		(128)
#define BUF_256_SIZE		(256)
#define BUF_512_SIZE 		(512)
#define BUF_544_SIZE 		(544)
#define BUF_1024_SIZE 		(1024)
#define BUF_2048_SIZE 		(2048)

#define BUF_SIZE 			(/*256*/BUF_2048_SIZE)


#define ATTRIBUTE_SECTION_SRAM			/* blank */

typedef struct buf_t_struct 
{
	struct buf_t_struct *next;	// 队列指针
	struct buf_t_struct *prev;
	char *pbuff;			// pbuff 和 size 仅仅初始化一次
	short size;				// pbuff 总长度
	short len;				// 有效长度
	short count;			// 重发次数
	int magic0;					// magic 
} buf_t;


typedef struct
{
//	int index;
	buf_t *head;
	buf_t *tail;
	char name[15];
	Boolean busy;
} buff_queue_t;

typedef struct {
	int sock;
	buf_t rx_buff;
	buf_t tx_buff;		// added by qinjiangwei 2017/7/22
	short tx_size;		// 当前发送的大小
	Boolean tx_busy;	// 已经发送,但是没有收到ACK,则认为是BUSY.
	short tx_count;
	buff_queue_t rx_queue;
	buff_queue_t tx_queue;
	Boolean tcp;		// tcp or udp.
	struct sockaddr_in addr;		// peer address
	
#ifdef __HEKR_CLOUD_SUPPORT__	
	buff_queue_t tx_sent_queue;			// 氦氪云支持
#endif
} net_line_t;

typedef void (*send_cb)(Boolean);
typedef void (*tcp_send_cb)(int socket, char *buf, int len, Boolean resend);
typedef Boolean (*fn_compare)(int, buf_t *b);
typedef void (*map_fn_t)(buf_t *b);

void tcp_send_callback(Boolean bOk);

struct httpd_fs_file 
{
  char *data;
  int len;
};

// key=value
#define HTTPD_KV_LEN 32
#define HTTPD_MAX_PARAM 8
#define HTTPD_PATH_LEN 64
typedef struct httpd_kv_struct
{
	char key[HTTPD_KV_LEN];
	char value[HTTPD_KV_LEN];
} httpd_kv_t;


typedef struct httpd_get_request_struct
{
	char filepath[HTTPD_PATH_LEN];
	httpd_kv_t kv_array[HTTPD_MAX_PARAM];
} httpd_get_req_t;

typedef struct _sock_server_ 
{
	int sock;
	buff_queue_t rx_queue;
	buff_queue_t tx_queue;
	char filename[HTTPD_PATH_LEN];
	struct httpd_fs_file file;
	char *scriptptr;
	int scriptlen;

	char *left_bracket;
	int  bracket_len;
	
	httpd_get_req_t http_req;

} sock_server_t;

void init_buff();
void init_buffer(buf_t *b);
void mysend(int resend);
Boolean input_char(int input);
void tx_timeout_callback();
Boolean recv_check();
Boolean add_rx_buffer() ATTRIBUTE_SECTION_SRAM;
buf_t *get_buf(buff_queue_t *q) ATTRIBUTE_SECTION_SRAM;;
Boolean add_buf(buff_queue_t *pb, buf_t *b);
Boolean add_char(buf_t *buff, char ch) ATTRIBUTE_SECTION_SRAM;;
void init_buffer(buf_t *b);
Boolean tcp_send_buf(buf_t *b, send_cb fn);
buf_t * remove_buf(buff_queue_t *pb);
void init_buff_queue(buff_queue_t *pb, char *name);
void print_queue(buff_queue_t *q);
Boolean check_timeout(buff_queue_t *q, int seconds);
void remove_timeout_buf(buff_queue_t *q, int seconds);
int get_buf_by_func(buff_queue_t *q, int v, fn_compare f);
buf_t * remove_buf_by_index(buff_queue_t *q, int index);
Boolean add_string(buf_t *buff, char *str);
Boolean append_buf(buf_t *to, buf_t *from);
void print_buffer(buf_t *b);
void test_data_buff();
void map_buf(buff_queue_t *q, map_fn_t f);
int buf_get_total_len(buff_queue_t *q);
Boolean buf_decrease_to_len(buff_queue_t *q, int final_len);
Boolean init_buffer2(buf_t *b, char *ptr, int len);

buf_t *alloc_buffer();
void free_buffer(buf_t *b);
void free_queue_buff(buff_queue_t *pb);
Boolean add_buf2(buff_queue_t *pb, buf_t *buff);
int append_buf2(buf_t *to, buf_t *from);
Boolean copy_buf(buf_t * to, buf_t * from);
void init_mem_pool(const kal_uint16 *size_array);
void init_256_mem_pool();
void init_2048_mem_pool();
buf_t * get_buff_from_mem_pool(char *p);
void print_mem_pool_array();
Boolean check_buffer_valid(buf_t *b, const char *func) ATTRIBUTE_SECTION_SRAM;

buf_t *new_alloc_index_buffer(int size_index);
/*重新初始化buf,主要是清空其下的pbuff*/
extern Boolean reset_buffer(buf_t *b);

/*取尾节点buf*/
extern buf_t *get_buf_tail(buff_queue_t *q);

/*显示buf信息*/
extern void data_buf_show_info(void);

Boolean buf_acc(buff_queue_t *q, buf_t *dst_buf);

void print_mem(char *b, int len);

void ua_free(void *p);

void *ua_malloc(size_t sz);

void test_data_buff();
#ifdef __cplusplus
}
#endif
#endif