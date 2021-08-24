#ifndef HEKR_CMD_H
#define HEKR_CMD_H

#ifdef WIN32
#include <Windows.h>
#endif

#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HEKR_GET_PROD_INFO_STR "getProdInfo"
#define HEKR_GET_PROD_INFO_RSP_STR "getProdInfo""Resp"


#define HEKR_DEV_LOGIN_STR "devLogin"
#define HEKR_DEV_LOGIN_RSP_STR	"devLoginResp"

#define HEKR_REPORT_DEV_INFO_STR	"reportDevInfo"
#define HEKR_REPORT_DEV_INFO_RSP_STR	"reportDevInfoResp"

#define HEKR_HEART_BEAT_STR "heartbeat"
#define HEKR_HEART_BEAT_RSP_STR "heartbeatResp"

#define HEKR_GET_TIMER_LIST_STR "getTimerList"
#define HEKR_GET_TIMER_LIST_RSP_STR "getTimerListResp"

#define HEKR_TIMER_REPORT_STR	"timerReport"
#define HEKR_TIMER_REPORT_RSP_STR	"timerReportResp"

#define HEKR_DEV_SYNC_STR "devSync"
#define HEKR_DEV_SYNC_RSP_STR "devSyncResp"

#define HEKR_DEV_UPGRADE_STR	"devUpgrade"
#define HEKR_DEV_UPGRADE_RSP_STR	"devUpgradeResp"

enum 
{
	HEKR_GET_PROD_INFO = 1,
	HEKR_GET_PROD_INFO_RSP,

	HEKR_DEV_LOGIN,
	HEKR_DEV_LOGIN_RSP,

	HEKR_REPORT_DEV_INFO,
	HEKR_REPORT_DEV_INFO_RSP,

	HEKR_HEART_BEAT,
	HEKR_HEART_BEAT_RSP,

	HEKR_GET_TIMER_LIST,
	HEKR_GET_TIMER_LIST_RSP,

	HEKR_TIMER_REPORT,
	HEKR_TIMER_REPORT_RSP,

	HEKR_DEV_SYNC,
	HEKR_DEV_SYNC_RSP,

	HEKR_DEV_UPGRADE,
	HEKR_DEV_UPGRADE_RSP,

	HEKR_HEKE_CMD_NUM
};

typedef struct 
{
	int cmd;
	char *cmd_str;
} hekr_table_t;

#define HEKR_TOKEN_LEN 33
#define HEKR_CMD_TIMEOUT_SECONDS 5

// 氦氪云消息基类
typedef struct
{
	int msgid;			// 消息序号
	int cmd;			// 命令
	char ptr[HEKR_TOKEN_LEN];
} hekr_msg_t;

// 登录消息
typedef struct 
{
	int msgid;
	int cmd;
	
	char mid[HEKR_TOKEN_LEN];
	char devTid[HEKR_TOKEN_LEN];
	char token[HEKR_TOKEN_LEN];
	char ctrlKey[HEKR_TOKEN_LEN];
	char bindKey[HEKR_TOKEN_LEN];
	char prodKey[HEKR_TOKEN_LEN];
} hekr_login_msg_t;

// 通用回应消息
typedef struct
{
	int msgid;
	int cmd;

	int code;
	char desc[128];
} hekr_rsp_msg_t;

#define HEKR_RAW_TASK_LEN 128
typedef struct 
{
	int taskid;
	int delay;
	int loop_count;
	int loop_time;
	int time;			// 开始时间
	char task[HEKR_RAW_TASK_LEN];
	char rsp[HEKR_RAW_TASK_LEN];
} hekr_task_t;

extern char hekr_hostname[128];
extern int hekr_port;

#define HEKR_PROD_INFO_SERVER "info-dev.hekr.me"
#define HEKR_PROD_INFO_PORT 91


#define HEKR_MAX_TASK_COUNT 10

buf_t * hekr_build_login_pack();
void test_hekr_rsp();
void hekr_parse_recv_buff(buf_t *b);
buf_t * hekr_build_heart_beat_pack();
void hekr_send_heart_pack();
buf_t * hekr_build_get_product_info_pack();
void hekr_close_sock();
sock_status_t net_tcp_connect_server(char *hostname, int port, Boolean isLogin);
Boolean hekr_check_msg(int sock, int msgid);
buf_t * hekr_build_report_devinfo();
void hekr_send_report_info_pack();
buf_t * hekr_build_dev_sync_rsp_pack(int msgid);
void hekr_send_dev_sync_rsp_pack(int msgid);
buf_t * hekr_build_get_timer_list_pack();
void hekr_send_get_timer_task_pack();
Boolean hekr_check_with_timer_task();
buf_t * hekr_build_timer_report_pack(int taskid, char *raw);
void hekr_send_timer_task_report_pack(int taskid, char *raw);
Boolean hekr_add_task_rsp_from_uart(fm_packet_t *pack);
int net_get_hekr_sock();
sock_status_t net_udp_connect_server(char *hostname, int port, int sk_index);
void net_tcp_send_cb(int socket, char *buf, int len, Boolean resend);

#ifdef WIN32

typedef UINT (*AFX_THREADPROC)(LPVOID);

UINT win32_recv_thread(LPVOID pParam);
int win32_net_connect(char *hostname, int port,Boolean isLogin);
void win32_start_thread(AFX_THREADPROC proc, LPVOID param);
void win32_stop_thread();
#endif

#ifdef __cplusplus
}
#endif

#endif