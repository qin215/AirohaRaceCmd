#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
#include "hx_flash_opt.h"
#include "mywin.h"
#include "qcloud_iot_export_log.h"

buff_queue_t uart_tx_queue;
buff_queue_t uart_rx_queue;
buf_t uart_rx_buff;
static Boolean uart_busy;
static net_line_t *pnet;

HANDLE		UARTHandle[MAX_PORT_NUM] = {0};
COMSTAT		comstat[MAX_PORT_NUM] = {0};
static BOOL breadytoread[MAX_PORT_NUM] = {0};
static BOOL breadytowrite[MAX_PORT_NUM] = {0};
static OVERLAPPED com_ov;
static HANDLE hWriteEvent;

static OVERLAPPED com_ov_ex[MAX_PORT_NUM];
static HANDLE hWriteEvent_ex[MAX_PORT_NUM];
buf_t g_uart_rx_buff[MAX_PORT_NUM];

static Boolean m_bUartThreadExit = FALSE; //add by cbk 20180829
//static int m_port = 0;//add by cbk 20180829


static uart_rsp_cb_t fn_uart_rsp_func;
static uart_rsp_ex_cb_t fn_uart_rsp_ex_func;			// 通用的，增加端口号

#if defined(__HX_IOT_SUPPORT__)
// 以下设置需要串行化到flash by qinjiangwei 2016/9/13
static int hx_esp_char = '+';		// 红心物联逃逸字符
static int hx_esp_time = 50;		// 红心物联逃逸时间 ms
static int hx_atpt_time = 50;		// 红心物联自组帧时间 ms
static int hx_frame_length = BUF_SIZE;	// 红心物联自组帧长度
static hx_uart_t hx_uart_info  = {BAUD_38400, 8, 1, 0};
static int hx_gpio1_mode = -1;
static Boolean hx_data_flag = TRUE;
static hx_sock_t hx_sok_info;
static Boolean hx_ap_mode = FALSE;
static U8 hx_gpio1_status = 0;

#ifdef __CUSTOMER_HOMI_IOT__
static hx_sock_t hx_sokb_info;
#endif

// 以下变量仅仅存于内存中 by qinjiangwei 2016/9/13
static char hx_at_cmd[128];
static int hx_socket_array[HX_SOCKET_NUM];
static buf_t hx_at_res_buf;
static Boolean hx_echo_flag = TRUE;		// default 打开
static hx_sock_t hx_sock_info;		// udp 或者 tcp skct命令对应的连接信息

#define HX_AP_SSID_NAME		"UA-IOT-AP"
#define HX_GPIO_PIN		GPIO_2

/*
 * 当前发送数据的sock和需要发送的数据大小
 */
static int hx_current_sock;
static int hx_recv_size;
static int hx_send_size;

static Boolean hx_is_blocking = FALSE;
static int hx_auto_conn_count = 0;

#define HX_REBOOT_EVENT	0xF0
#define HX_RECONN_SOCK_EVENT	0xF1
#define HX_RECONN_SOCKB_EVENT	0xF2
#define HX_START_EVENT	0xF3

static int hx_timer_event;

static struct etimer sTm;

#ifndef WIN32
PROCESS_NAME(hx_sta_connect_process);
PROCESS_NAME(hekr_tcp_process);
PROCESS_NAME(sockb_tcp_process);
PROCESS_NAME(tcp_server_wifiUartProcess);
#endif

PROCESS(hx_timer_checker, "hx_timer_process");

#ifndef WIN32
extern TAG_CABRIO_CONFIGURATION gCabrioConf;
extern IEEE80211STATUS gwifistatus;

int At_EnableSmartReboot(stParam *param);
int At_Reboot(stParam *param);
#endif

static int hx_atcmd_atlt(stParam *param);
static int hx_atcmd_atm(stParam *param);
static int hx_atcmd_atpt(stParam *param);
static int hx_atcmd_atrm(stParam *param);
static int hx_atcmd_bssid(stParam *param);
static int hx_atcmd_brdssid(stParam *param);
static int hx_atcmd_chl(stParam *param);
static int hx_atcmd_chll(stParam *param);
static int hx_atcmd_cmdm(stParam *param);
static int hx_atcmd_dns(stParam *param);
static int hx_atcmd_e(stParam *param);
static int hx_atcmd_encry(stParam *param);
static int hx_atcmd_entm(stParam *param);
static int hx_atcmd_ents(stParam *param);
static int hx_atcmd_espc(stParam *param);
static int hx_atcmd_espt(stParam *param);
static int hx_atcmd_iom(stParam *param);
static int hx_atcmd_key(stParam *param);
static int hx_atcmd_lkstt(stParam *param);
static int hx_atcmd_nip(stParam *param);
static int hx_atcmd_pass(stParam *param);
static int hx_atcmd_pmtf(stParam *param);
static int hx_atcmd_qmac(stParam *param);
static int hx_atcmd_qver(stParam *param);
static int hx_atcmd_rstf(stParam *param);
static int hx_atcmd_skcls(stParam *param);
static int hx_atcmd_skct(stParam *param);
static int hx_atcmd_skrcv(stParam *param);
static int hx_atcmd_sksdf(stParam *param);
static int hx_atcmd_sksnd(stParam *param);
static int hx_atcmd_skstt(stParam *param);
static int hx_atcmd_ssid(stParam *param);
static int hx_atcmd_uart(stParam *param);
static int hx_atcmd_warc(stParam *param);
static int hx_atcmd_warm(stParam *param);
static int hx_atcmd_watc(stParam *param);
static int hx_atcmd_wbgr(stParam *param);
static int hx_atcmd_webs(stParam *param);
static int hx_atcmd_apst(stParam *param);
static int hx_atcmd_wjoin(stParam *param);
static int hx_atcmd_wleav(stParam *param);
static int hx_atcmd_wprt(stParam *param);
static int hx_atcmd_wpsm(stParam *param);
static int hx_atcmd_wscan(stParam *param);
static int hx_atcmd_wapst(stParam *param);
static int hx_atcmd_ioc(stParam *param);
static int hx_atcmd_z(stParam *param);
static int hx_atcm_smtlnk_reboot(stParam *param);
static int hx_atcmd_null(stParam *param);
static int hx_atcm_auto_conn(stParam *param);

#ifdef __CUSTOMER_HOMI_IOT__
static int hm_atcmd_wmode(stParam *param);
static int hm_atcmd_peld(stParam *param);
static int hm_atcmd_netp(stParam *param);
static int hm_atcmd_tcplk(stParam *param);
static int hm_atcmd_tcpto(stParam *param);
static int hm_atcmd_tcpdis(stParam *param);

static int hm_atcmd_sockb(stParam *param);
static int hm_atcmd_tcplkb(stParam *param);
static int hm_atcmd_tcptob(stParam *param);
static int hm_atcmd_tcpdisb(stParam *param);

static int hm_atcmd_wskey(stParam *param);
static int hm_atcmd_wsssid(stParam *param);
static int hm_atcmd_wann(stParam *param);
static int hm_atcmd_wsmac(stParam *param);

static int hm_atcmd_wslk(stParam *param);
static int hm_atcmd_smtlk(stParam *param);
static int hm_atcmd_auto_connb(stParam *param);

static int hm_get_sock_group(hx_sock_t *p_sock);
static Boolean hm_check_sock_group_free(int group);
#endif

static int hx_do_with_one_param(stParam *param, int *pval, Boolean *psave, Boolean *pQuery);
static int hx_do_with_one_string_param(stParam *param, char *pstr, int len, Boolean *psave);
static int hx_at_parser(char *buff, int len) ATTRIBUTE_SECTION_SRAM;
void hx_set_data_flag(Boolean flag) ATTRIBUTE_SECTION_SRAM;

static void hx_shift_param(stParam *param);
static void hx_fill_ap_info(buf_t *b);
static int hx_create_sock(hx_sock_t *p_sock);
static Boolean hx_close_active_sock();

void hx_start_reboot_timer();
void hx_start_reconn_timer(int event);

/* 该值仅仅用来做占位使用,真正的sock值在client连进来之后确定 */		
#define HX_SPECIAL_TCP_SERVER_SOCK 		(33)	

static Boolean hx_uart_busy = FALSE;

static void hx_start_auto_conn_timer();
static Boolean hx_check_auto_conn_timeout();
static Boolean hx_check_auto_conn();


at_cmd_info hx_atcmdicomm_info_tbl[] =
{
	{HX_ATCMD_ATLT, hx_atcmd_atlt, 0},
	{HX_ATCMD_ATM, hx_atcmd_atm, 0},
	{HX_ATCMD_ATPT, hx_atcmd_atpt, 0},
	{HX_ATCMD_ATRM, hx_atcmd_atrm, 0},
	{HX_ATCMD_BSSID, hx_atcmd_bssid, 0},
	{HX_ATCMD_BRDSSID, hx_atcmd_brdssid, 0},
	{HX_ATCMD_CHL, hx_atcmd_chl, 0},
	{HX_ATCMD_CHLL, hx_atcmd_chll, 0},
	{HX_ATCMD_CMDM, hx_atcmd_cmdm, 0},
	{HX_ATCMD_DNS, hx_atcmd_dns, 0},
	{HX_ATCMD_E, hx_atcmd_e, 0},
	{HX_ATCMD_ENCRY, hx_atcmd_encry, 0},
	{HX_ATCMD_ENTM, hx_atcmd_entm, 0},
	{HX_ATCMD_ENTS, hx_atcmd_ents, 0},
	{HX_ATCMD_ESPC, hx_atcmd_espc, 0},
	{HX_ATCMD_ESPT, hx_atcmd_espt, 0},
	{HX_ATCMD_IOM, hx_atcmd_iom, 0},
	{HX_ATCMD_KEY, hx_atcmd_key, 0},
	{HX_ATCMD_LKSTT, hx_atcmd_lkstt, 0},
	{HX_ATCMD_NIP, hx_atcmd_nip, 0},
	{HX_ATCMD_PASS, hx_atcmd_pass, 0},
	{HX_ATCMD_PMTF, hx_atcmd_pmtf, 0},
	{HX_ATCMD_QMAC, hx_atcmd_qmac, 0},
	{HX_ATCMD_QVER, hx_atcmd_qver, 0},
	{HX_ATCMD_RSTF, hx_atcmd_rstf, 0},
	{HX_ATCMD_SKCLS, hx_atcmd_skcls, 0},
	{HX_ATCMD_SKCT, hx_atcmd_skct, 0},
	{HX_ATCMD_SKRCV, hx_atcmd_skrcv, 0},
	{HX_ATCMD_SKSDF, hx_atcmd_sksdf, 0},
	{HX_ATCMD_SKSND, hx_atcmd_sksnd, 0},
	{HX_ATCMD_SKSTT, hx_atcmd_skstt, 0},
	{HX_ATCMD_SSID, hx_atcmd_ssid, 0},
	{HX_ATCMD_UART, hx_atcmd_uart, 0},
	{HX_ATCMD_WARC, hx_atcmd_warc, 0},
	{HX_ATCMD_WARM, hx_atcmd_warm, 0},
	{HX_ATCMD_WATC, hx_atcmd_watc, 0},
	{HX_ATCMD_WBGR, hx_atcmd_wbgr, 0},
	{HX_ATCMD_WEBS, hx_atcmd_webs, 0},
	{HX_ATCMD_APST, hx_atcmd_apst, 0},
	{HX_ATCMD_WJOIN, hx_atcmd_wjoin, 0},
	{HX_ATCMD_WLEAV, hx_atcmd_wleav, 0},
	{HX_ATCMD_WPRT, hx_atcmd_wprt, 0},
	{HX_ATCMD_WPSM, hx_atcmd_wpsm, 0},
	{HX_ATCMD_WSCAN, hx_atcmd_wscan, 0},
	{HX_ATCMD_WAPST, hx_atcmd_wapst, 0},
	{HX_ATCMD_IOC, hx_atcmd_ioc, 0},
	{HX_ATCMD_Z, hx_atcmd_z, 0},
	{HX_ATCMD_SMARTLINK, hx_atcm_smtlnk_reboot, 0},
	{HX_ATCMD_AUTO_CONN, hx_atcm_auto_conn, 0},
	{HX_ATCMD_AUTO_CONN_START, hx_atcm_auto_conn, 0},
	
#ifdef __CUSTOMER_HOMI_IOT__
	{HM_ATCMD_WMODE, hm_atcmd_wmode, 0},
	{HM_ATCMD_RELD, hm_atcmd_peld, 0},
	{HM_ATCMD_NETP, hm_atcmd_netp, 0},
	{HM_ATCMD_TCPLK, hm_atcmd_tcplk, 0},
	{HM_ATCMD_TCPTO, hm_atcmd_tcpto, 0},
	{HM_ATCMD_TCPDIS, hm_atcmd_tcpdis, 0},
	
	{HM_ATCMD_SOCKB, hm_atcmd_sockb, 0},
	{HM_ATCMD_TCPLKB, hm_atcmd_tcplkb, 0},
	{HM_ATCMD_TCPTOB, hm_atcmd_tcptob, 0},
	{HM_ATCMD_TCPDISB, hm_atcmd_tcpdisb, 0},
	
	{HM_ATCMD_WSKEY, hm_atcmd_wskey, 0},
	{HM_ATCMD_WSSSID, hm_atcmd_wsssid, 0},
	{HM_ATCMD_WANN, hm_atcmd_wann, 0},
	{HM_ATCMD_WSMAC, hm_atcmd_wsmac, 0},

	{HM_ATCMD_WSLK, hm_atcmd_wslk, 0},
	{HM_ATCMD_SMTLK, hm_atcmd_smtlk, 0},
	{HM_ATCMD_AUTO_CONN_B, hm_atcmd_auto_connb, 0},
	{HM_ATCMD_AUTO_CONN_B_START, hm_atcmd_auto_connb, 0},
#endif
	{HX_ATCMD_NULL, hx_atcmd_null, 0},	// 必须放最后一个
	{0, NULL, 0}
};

extern void sampleDataUartRead(void *dara);

void hx_init_parameters()
{
	Boolean ret;

	memset(&hx_socket_array, -1, sizeof(hx_socket_array));
	
	ret = hx_init_parameter(&hx_esp_char, &hx_esp_time, &hx_atpt_time, \
						&hx_frame_length, &hx_uart_info, &hx_gpio1_mode,\
						&hx_data_flag, &hx_sok_info, &hx_ap_mode, &hx_gpio1_status);

	if (ret)
	{
		GPIO_CONF conf;

		conf.dirt = INPUT;
		conf.pull_en = FALSE;
		conf.driving_en = FALSE;

		deinit_data_uart_port();
		init_data_uart_port(hx_uart_info.uart_baud_rate, sampleDataUartRead);
		
		if (hx_gpio1_mode == INPUT + 1)
		{
			conf.dirt = INPUT;
		}
		else if (hx_gpio1_mode == OUTPUT + 1)
		{
			conf.dirt = OUTPUT;
		}
		
		pinMode(HX_GPIO_PIN, conf);
		if (hx_gpio1_mode == OUTPUT + 1)
		{
			digitalWrite(HX_GPIO_PIN, hx_gpio1_status);		
		}

		if (hx_ap_mode)
		{
			gconfig_set_softap_ssid(HX_AP_SSID_NAME);
			//softap_init_ex2(192, 168, 0, 1, 0, 0, (U8 *)"", 5);
			softap_init_ex2(192, 168, 0, 1, 0, 0, 0, 1, 50);
			softap_start();			
		}
	}
#ifdef __CUSTOMER_HOMI_IOT__
	hx_init_sockb_parameter(&hx_sokb_info);
	hx_init_echo_flag(&hx_echo_flag);
#endif
	
}

static void hx_save_to_flash(int addr, int val)
{

}

#ifdef __CUSTOMER_HOMI_IOT__

/*
 *设置查询WIFI工作模式（AP/STA）
 */
static int hm_atcmd_wmode(stParam *param)
{
	char *p;
	
    init_buffer(&hx_at_res_buf);

	if (param->argc == 0)
	{
		if (hx_ap_mode)
		{
			add_string(&hx_at_res_buf, "AP");
		}
		else
		{
			add_string(&hx_at_res_buf, "STA");
		}
		
		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	p = param->argv[0];

	if (strcmp(p, "AP") == 0)
	{
		gconfig_set_softap_ssid(HX_AP_SSID_NAME);
		softap_init_ex2(192, 168, 0, 1, 0, 0, 0, 1, 50);
		//softap_init_ex2(192, 168, 0, 1, 0, 0, (U8 *)"", 5);
		softap_start();
		hx_ap_mode = TRUE;
	}
	else if (strcmp(p, "STA") == 0)
	{
		softap_exit();
	}
	else
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	return HX_ERROR_SUCCESS;
}

/*
 *恢复出厂
 */
static int hm_atcmd_peld(stParam *param)
{
    init_buffer(&hx_at_res_buf);

	hx_atcmd_rstf(param);
	
	add_string(&hx_at_res_buf, "rebooting...");
	hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
	
	if (hx_close_active_sock())
	{
		return HX_ERROR_BLOCK;;
	}

	At_Reboot(param);
	return HX_ERROR_SUCCESS;
}

/*
 * 根据p_sock信息在 p_buf 中创建socket信息文本
 */
static int hm_cons_netp_rsp(buf_t *p_buf, hx_sock_t *p_sock)
{
	char buff[128];

	if (p_sock->sok_protocol == HX_UDP_PROTOCOL)
	{
		add_string(p_buf, "UDP,");
	}
	else if (p_sock->sok_protocol == HX_TCP_PROTOCOL)
	{
		add_string(p_buf, "tcp,");
	}
	else
	{
		printf("invalid1\r\n");
		return HX_ERROR_UNKOWN;
	}

	if (p_sock->sok_cs == HX_SERVER_MODE)
	{
		add_string(p_buf, "server,");
	}
	else if (p_sock->sok_cs == HX_CLIENT_MODE)
	{
		add_string(p_buf, "client,");
	}
	else
	{
		printf("invalid2\r\n");
		return HX_ERROR_UNKOWN;
	}
	
	sprintf(buff, "%d,", p_sock->sok_port);
	add_string(p_buf, buff);

	if (p_sock->sok_cs == HX_CLIENT_MODE)
	{
		sprintf(buff, "%s", p_sock->sok_hostname);
	}
	else
	{
		sprintf(buff, "127.0.0.1");
	}
	add_string(p_buf, buff);
	
	return HX_ERROR_SUCCESS;
}

static int hm_atcmd_fill_sockinfo(stParam *param, hx_sock_t *p_sock)
{
	char *p;
	
	p = param->argv[0];
	if (strcmp(p, "tcp") == 0 || strcmp(p, "TCP") == 0)
	{
		p_sock->sok_protocol = HX_TCP_PROTOCOL;
	}
	else if (strcmp(p, "udp") == 0 || strcmp(p, "UDP") == 0)
	{
		p_sock->sok_protocol = HX_UDP_PROTOCOL;
	}
	else
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	p = param->argv[1];
	if (strcmp(p, "server") == 0)
	{
		p_sock->sok_cs = HX_SERVER_MODE;
	}
	else if (strcmp(p, "client") == 0)
	{
		p_sock->sok_cs = HX_CLIENT_MODE;
	}
	else
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	p = param->argv[2];
	p_sock->sok_port = atoi(p);
	p_sock->sok_timeout = 300;		// 默认为300s

	p = param->argv[3];
	if (strlen(p) >= sizeof(p_sock->sok_hostname))
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	strcpy(p_sock->sok_hostname, p);

	return HX_ERROR_SUCCESS;
}

/*
 * 设置查询SOCKA的网络参数，该参数会被保存，复位生效
 * 查询
 * AT+NETP
 * 返回
 * +ok=<protocol，CS，port，IP><CR><LF><CR><LF>
 * 设置为tcpclient
 * AT+NETP=tcp,client,9999,10.10.100.254
 * 设置为tcpserver
 * AT+NETP=tcp,server,9999,10.10.100.254
 * 设置为udpclient
 * AT+NETP=UDP,client,9999,10.10.100.254
 * 设置为udpserver
 * AT+NETP=UDP,server,9999,10.10.100.254
*/
static int hm_atcmd_netp(stParam *param)
{
	int ret;
	
    init_buffer(&hx_at_res_buf);
	
	if (param->argc == 0)
	{
		return hm_cons_netp_rsp(&hx_at_res_buf, &hx_sok_info);
	}

	if (param->argc != 4)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if ((ret = hm_atcmd_fill_sockinfo(param, &hx_sok_info)) != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	hx_update_parameter(NULL, NULL, NULL, NULL, NULL, NULL, NULL, &hx_sok_info);
	
	if (hx_write_conf())
	{
		return HX_ERROR_SUCCESS;
	}
	else
	{
		return HX_ERROR_UNKOWN;
	}
}


static int hm_get_tcp_index(hx_sock_t *p_sock)
{
	int index = -1;

	if (p_sock->sok_protocol == HX_TCP_PROTOCOL)
	{
		if (p_sock->sok_cs == HX_CLIENT_MODE)
		{
			if (p_sock == &hx_sok_info)
			{
				index = HX_TCP_CLIENT_SOCKET;
			}
			else
			{
				index = HX_TCP_CLIENT_SOCKETB;
			}			
		}
		else if (p_sock->sok_cs == HX_SERVER_MODE)
		{
			index = HX_TCP_SERVER_SOCKET;
		}
	}
	else if (p_sock->sok_protocol == HX_UDP_PROTOCOL)
	{
		if (p_sock->sok_cs == HX_CLIENT_MODE)
		{
			if (p_sock == &hx_sok_info)
			{
				index = HX_UDP_CLIENT_SOCKET;
			}
			else
			{
				index = HX_UDP_CLIENT_SOCKETB;
			}			
		}
		else if (p_sock->sok_cs == HX_SERVER_MODE)
		{
			index = HX_UDP_SERVER_SOCKET;
		}
	}

	printf("index = %d\r\n", index);

	return index;
}

/* 
 * 查询做STA时tcp是否建立连接
 * 返回
 * +ok=on/off
 */
static int hm_atcmd_tcplk(stParam *param)
{
	int index = -1;
	hx_sock_t *p_sock = &hx_sok_info;
	
    init_buffer(&hx_at_res_buf);

	index = hm_get_tcp_index(p_sock);
	if (index == -1)
	{
		return HX_ERROR_UNKOWN;
	}

	if (hx_socket_array[index] == -1)
	{
		add_string(&hx_at_res_buf, "off");
	}
	else
	{
		add_string(&hx_at_res_buf, "on");
	}
	
	return HX_ERROR_SUCCESS;
}

/*
 * AT+TCPTO：查询tcp超时时间
 * 返回
 * +ok=<time>
 * AT+TCPTO=<time>：设置tcp超时时间
 * Time	
 * 默认300s
 * 0：不设超时时间
 * 返回
 * +ok
 * 模块的tcp通道未收到任何数据开始计时，接收到数据时清除计时，如果超过to时间，则断开此连接,
 * 模块做tcpclient时会自动重连server，做tcpserver的时候，tcpclient需要重新连接server。
 */
static int hm_atcmd_tcpto(stParam *param)
{
	int val;
	static int start = 1;
	
    init_buffer(&hx_at_res_buf);
	
	if (param->argc == 0)
	{
		hx_at_res_buf.len = sprintf(hx_at_res_buf.pbuff, "%d", hx_sok_info.sok_timeout);
		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	val = atoi(param->argv[0]);

	if (val < 0 || val >= 3600)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	hx_sok_info.sok_timeout = val;
	//TODO. 添加timer使得设置生效
	if (hx_socket_array[HX_TCP_CLIENT_SOCKET] >= 0)
	{
		process_post_synch(&hekr_tcp_process, PROCESS_EVENT_CONTINUE, &start);
	}
	
	return HX_ERROR_SUCCESS;
}

/*
 * AT+ TCPDIS：查询连接状态
 * 返回
 * +ok=on/off<CR><LF><CR><LF>
 * AT+TCPDIS=on/off：设置模块建立/断开tcp链接
 * 返回
 * +ok<CR><LF><CR><LF>
 */ 
static int hm_atcmd_tcpdis(stParam *param)
{
	int index;
	hx_sock_t *p_sock = &hx_sok_info;
	char *ptr;
	static int start = 0;
	int i;
	Boolean active = FALSE;
	
    init_buffer(&hx_at_res_buf);

	index = hm_get_tcp_index(p_sock);
	if (index == -1)
	{
		return HX_ERROR_UNKOWN;
	}

	for (i = HX_TCP_CLIENT_SOCKET; i < HX_UDP_SERVER_SOCKET; i++)
	{
		if (hx_socket_array[i] != -1)
		{
			active = TRUE;
			break;
		}
	}
		
	if (param->argc == 0)
	{
		printf("active = %d\r\n", active);
		if (hx_socket_array[index] == -1 && !active)
		{
			add_string(&hx_at_res_buf, "off");
		}
		else
		{
			add_string(&hx_at_res_buf, "on");
		}

		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	ptr = param->argv[0];

	if (strcmp(ptr, "on") == 0)
	{
		if (hx_socket_array[index] >= 0 || active)
		{
			return HX_ERROR_OP_REFUSED;
		}
		
		return hx_create_sock(p_sock);
	}
	else if (strcmp(ptr, "off") == 0)
	{
		int sok;
		
		sok = hx_socket_array[index];
		printf("sok = %d", sok);
	
		if (sok == -1 && !active)
		{
			return HX_ERROR_NO_SOCK; 
		}

		/*
		 *  这种情况出现的条件如下:
		 * 1. AT+NETP 设置UDP参数
		 * 2. AT+TCPDIS建立UDP连接
		 * 3. AT+NETP 设置TCP参数
		 * 4. AT+TCPDIS 返回错误
		 * 5. AT+TCPDIS=off关闭连接
		 */
		if (active && (i != index))
		{
			printf("active sock i = %d, index = %d\r\n", i, index);
			index = i;
		}

		if (index == HX_TCP_CLIENT_SOCKET)
		{
			process_post_synch(&hekr_tcp_process, PROCESS_EVENT_CONTINUE, &start);
			if (sok == net_get_hekr_sock())
			{
				tcpclose(sok);
			}
			else
			{
				return HX_ERROR_UNKOWN;
			}	
		}
		else if (index == HX_UDP_CLIENT_SOCKET)
		{
			hx_stop_udp_client(index);
			hx_socket_array[index] = -1;
		}
	}
	else 
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	return HX_ERROR_SUCCESS;
}

/*
 * 根据p_sock信息在 p_buf 中创建socket信息文本
 */
static int hm_cons_sockb_rsp(buf_t *p_buf, hx_sock_t *p_sock)
{
	char buff[128];

	if (p_sock->sok_protocol == HX_UDP_PROTOCOL)
	{
		if (p_sock->sok_cs == HX_SERVER_MODE)
		{
			add_string(p_buf, "UDPS,");
		}
		else if (p_sock->sok_cs == HX_CLIENT_MODE)
		{
			add_string(p_buf, "udp,");
		}
		else 
		{
			return HX_ERROR_UNKOWN;
		}
	}
	else if (p_sock->sok_protocol == HX_TCP_PROTOCOL)
	{
		if (p_sock->sok_cs == HX_CLIENT_MODE)
		{
			add_string(p_buf, "tcp,");
		}
		else
		{
			return HX_ERROR_UNKOWN;
		}
	}
	else
	{
		printf("invalid1\r\n");
		return HX_ERROR_UNKOWN;
	}

	sprintf(buff, "%d,", p_sock->sok_port);
	add_string(p_buf, buff);

	if (p_sock->sok_cs == HX_CLIENT_MODE)
	{
		sprintf(buff, "%s", p_sock->sok_hostname);
	}
	else
	{
		sprintf(buff, "127.0.0.1");
	}
	add_string(p_buf, buff);
	
	return HX_ERROR_SUCCESS;
}

static int hm_atcmd_fill_sockb_info(stParam *param, hx_sock_t *p_sock)
{
	char *p;
	
	p = param->argv[0];
	if (strcmp(p, "tcp") == 0 || strcmp(p, "TCP") == 0)
	{
		p_sock->sok_protocol = HX_TCP_PROTOCOL;
		p_sock->sok_cs = HX_CLIENT_MODE;
	}
	else if (strcmp(p, "udp") == 0 || strcmp(p, "UDP") == 0)
	{
		p_sock->sok_protocol = HX_UDP_PROTOCOL;
		p_sock->sok_cs = HX_CLIENT_MODE;
	}
	else if (strcmp(p, "udps") == 0 || strcmp(p, "UDPS") == 0)
	{
		p_sock->sok_protocol = HX_UDP_PROTOCOL;
		p_sock->sok_cs = HX_SERVER_MODE;
	}
	else
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	p = param->argv[1];
	p_sock->sok_port = atoi(p);
	p_sock->sok_timeout = 300;		// 默认为300s

	p = param->argv[2];
	if (strlen(p) >= sizeof(p_sock->sok_hostname))
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	strcpy(p_sock->sok_hostname, p);

	return HX_ERROR_SUCCESS;
}

static int hm_atcmd_sockb(stParam *param)
{
	int ret;
	
    init_buffer(&hx_at_res_buf);
	
	if (param->argc == 0)
	{
		return hm_cons_sockb_rsp(&hx_at_res_buf, &hx_sokb_info);
	}

	if (param->argc != 3)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if ((ret = hm_atcmd_fill_sockb_info(param, &hx_sokb_info)) != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	hx_update_sockb_parameter(&hx_sokb_info);
	if (hx_write_conf())
	{
		return HX_ERROR_SUCCESS;
	}
	else
	{
		return HX_ERROR_UNKOWN;
	}
}

static int hm_atcmd_tcplkb(stParam *param)
{
    int index = -1;
	hx_sock_t *p_sock = &hx_sokb_info;
	
    init_buffer(&hx_at_res_buf);

	index = hm_get_tcp_index(p_sock);
	if (index == -1)
	{
		return HX_ERROR_UNKOWN;
	}

	if (hx_socket_array[index] == -1)
	{
		add_string(&hx_at_res_buf, "off");
	}
	else
	{
		add_string(&hx_at_res_buf, "on");
	}
	
	return HX_ERROR_SUCCESS;
}

static int hm_atcmd_tcptob(stParam *param)
{
	int val;
	static int start = 1;
	
    init_buffer(&hx_at_res_buf);
	
	if (param->argc == 0)
	{
		hx_at_res_buf.len = sprintf(hx_at_res_buf.pbuff, "%d", hx_sokb_info.sok_timeout);
		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	val = atoi(param->argv[0]);

	if (val < 0 || val >= 3600)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	hx_sokb_info.sok_timeout = val;
	//TODO. 添加timer使得设置生效
	if (hx_socket_array[HX_TCP_CLIENT_SOCKETB] >= 0)
	{
		process_post_synch(&sockb_tcp_process, PROCESS_EVENT_CONTINUE, &start);
	}
	
	return HX_ERROR_SUCCESS;
}

static int hm_atcmd_tcpdisb(stParam *param)
{
	int index = -1;
	char *ptr;
	hx_sock_t *p_sock = &hx_sokb_info;
	static int start = 0;
	int i;
	Boolean active = FALSE;
	
    init_buffer(&hx_at_res_buf);

	index = hm_get_tcp_index(p_sock);
	if (index == -1)
	{
		return HX_ERROR_UNKOWN;
	}

	for (i = HX_TCP_CLIENT_SOCKETB; i < HX_UDP_SERVER_SOCKETB; i++)
	{
		if (hx_socket_array[i] != -1)
		{
			active = TRUE;
			break;
		}
	}
	
	if (param->argc == 0)
	{
		if (hx_socket_array[index] == -1 && !active)
		{
			add_string(&hx_at_res_buf, "off");
		}
		else
		{
			add_string(&hx_at_res_buf, "on");
		}

		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	ptr = param->argv[0];

	if (strcmp(ptr, "on") == 0)
	{
		if (hx_socket_array[index] >= 0 || active)
		{
			return HX_ERROR_OP_REFUSED;
		}
		
		return hx_create_sock(p_sock);
	}
	else if (strcmp(ptr, "off") == 0)
	{
		int sok;
		
		sok = hx_socket_array[index];
		printf("sok = %d", sok);
		if (sok == -1 && !active)
		{
			return HX_ERROR_NO_SOCK; 
		}

		/*
		 *  这种情况出现的条件如下:
		 * 1. AT+SOCKB 设置UDP参数
		 * 2. AT+TCPDISB建立UDP连接
		 * 3. AT+SOCKB 设置TCP参数
		 * 4. AT+TCPDISB 返回错误
		 * 5. AT+TCPDISB=off关闭连接
		 */
		if (active && (i != index))
		{
			printf("active sock i = %d, index = %d\r\n", i, index);
			index = i;
		}

		if (index == HX_TCP_CLIENT_SOCKETB)
		{
			if (sok == net_get_sokb())
			{
				process_post_synch(&sockb_tcp_process, PROCESS_EVENT_CONTINUE, &start);
				tcpclose(sok);
			}
			else
			{
				return HX_ERROR_UNKOWN;
			}	
		}
		else if (index == HX_UDP_CLIENT_SOCKETB)
		{
			hx_stop_udp_client(index);
			hx_socket_array[index] = -1;
		}
	}
	else 
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	return HX_ERROR_SUCCESS;
}

static void hm_append_auth_type(buf_t *p_buf)
{
	if (gwifistatus.wifi_security == NONE)
	{
		add_string(p_buf, "OPEN,");
	}
	else if (gwifistatus.wifi_security == WEP)
	{
		add_string(p_buf, "WEP,");
	}
	else if (gwifistatus.wifi_security == WPA)
	{
		add_string(p_buf, "WPA,");
	}
	else if (gwifistatus.wifi_security == WPA2)
	{
		add_string(p_buf, "WPA2,");
	}
	else if (gwifistatus.wifi_security == WPAWPA2)
	{
		add_string(p_buf, "WPAWPA2,");
	}
	else
	{
		add_string(p_buf, "UNKOWN,");
	}
}


//0: open, 1:WPA2-tkip 2:WPA2:CCMP
static void hm_append_sec_type(buf_t *p_buf)
{
	if (gwifistatus.softap_encryt_mode == 0)
	{
		add_string(p_buf, "OPEN,");
	}
	else if (gwifistatus.softap_encryt_mode == 1)
	{
		add_string(p_buf, "WPA2-TKIP,");
	}
	else if (gwifistatus.softap_encryt_mode == 2)
	{
		add_string(p_buf, "WPA2-CCMP,");
	}
	else
	{
		add_string(p_buf, "UNKOWN,");
	}
}

/*
 * AT+WSKEY：查询STA的加密参数
 * 返回
 * +ok=<auth，encry，key>
 * Exp：+ok=WPA2PSK,AES,justdoit<CR><LF><CR><LF>
 * +ok=OPEN,NONE<CR><LF><CR><LF>
 * AT+WSKEY=<auth，encry，key>：设置STA的加密参数，重启生效
 * Exp：AT+WSKEY=WPA2PSK,AES,justdoit
 * 返回
 * +ok<CR><LF><CR><LF>
*/
static int hm_atcmd_wskey(stParam *param)
{
	char *p;
	
	init_buffer(&hx_at_res_buf);

	if (param->argc == 0)
	{
		hm_append_auth_type(&hx_at_res_buf);
		hm_append_sec_type(&hx_at_res_buf);

#if 0		
		if (gwifistatus.status == IEEE80211_CONNECTED)
		{
			add_string(&hx_at_res_buf, (char *)(gwifistatus.connAP.key));

			if (strcmp((char *)(gwifistatus.connAP.key), hx_get_current_key()) != 0)
			{
				hx_set_current_key((char *)(gwifistatus.connAP.key));
				hx_write_conf();
			}
		}
		else
#endif			
		{
			add_string(&hx_at_res_buf, hx_get_current_key());
		}

		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 3)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	p = param->argv[2];

	if (strlen(p) >= HX_SSID_LEN)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	hx_set_current_key(p);
	hx_write_conf();

	return HX_ERROR_SUCCESS;
}

/*
 * AT+WSSSID：查询关联的AP ssid（最多支持32字节）
 * 返回
 * +ok=<ssid><CR><LF><CR><LF>
 * AT+WSSSID=<ssid>：设置ap关联的ssid，重启生效
 * 返回
 * +ok<CR><LF><CR><LF>
*/
static int hm_atcmd_wsssid(stParam *param)
{
	char *p;
	
	init_buffer(&hx_at_res_buf);

	if (param->argc == 0)
	{
#if 0	
		if (gwifistatus.status == IEEE80211_CONNECTED)
		{
			add_string(&hx_at_res_buf, (char *)(gwifistatus.connAP.ssid));

			if (strcmp((char *)(gwifistatus.connAP.ssid), hx_get_current_ssid()) != 0)
			{
				hx_set_current_ssid((char *)(gwifistatus.connAP.ssid));
				hx_write_conf();
			}
		}
		else
#endif			
		{
			add_string(&hx_at_res_buf, hx_get_current_ssid());
		}
	
		return HX_ERROR_SUCCESS;
	}

	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	p = param->argv[0];

	if (strlen(p) >= HX_SSID_LEN)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	hx_set_current_ssid(p);
	hx_write_conf();
	
	return HX_ERROR_SUCCESS;
}

/* 
 * AT+WANN：查询STA的网络参数
 * 返回
 * +ok=<mode，address，mask，gateway><CR><LF><CR><LF>
 * AT+WANN=<mode，address，mask，gateway>：设置STA的网络参数，重启后生效
 * 返回
 * +ok<CR><LF><CR><LF>
*/
static int hm_atcmd_wann(stParam *param)
{
 	char *p;
	int len;
	
	init_buffer(&hx_at_res_buf);
	p = hx_at_res_buf.pbuff;

	if (param->argc == 0)
	{
		if (gwifistatus.status == IEEE80211_CONNECTED)
		{
			if (gCabrioConf.dhcp_enable)
			{
				len = sprintf(p, "DHCP,");
			}
			else
			{
				len = sprintf(p, "static");
			}
			
			len += sprintf(&p[len], "%d.%d.%d.%d,", uip_ipaddr_to_quad(&(gCabrioConf.local_ip_addr)));
			len += sprintf(&p[len], "%d.%d.%d.%d,", uip_ipaddr_to_quad(&(gCabrioConf.net_mask)));
			len += sprintf(&p[len], "%d.%d.%d.%d", uip_ipaddr_to_quad(&(gCabrioConf.gateway_ip_addr)));

			hx_at_res_buf.len = len;
		}
		
		return HX_ERROR_SUCCESS;
	}

	// TODO.
	return HX_ERROR_NOT_SUPPORT;	
}

/*
 * AT+WSMAC：查询STA的mac
 * 返回：
 * +ok=ACCF23E11F33<CR><LF><CR><LF>
 */
static int hm_atcmd_wsmac(stParam *param)
{
	return hx_atcmd_qmac(param);
}

/*
 * AT+WSLK
 * 查询STA的无线状态
 * 返回
 * 没有连接：+ok=Disconnected<CR><LF><CR><LF>
 * 已经连接：+ok=AP的SSID<CR><LF><CR><LF>
 */
static int hm_atcmd_wslk(stParam *param)
{
    init_buffer(&hx_at_res_buf);

	if (gwifistatus.status == IEEE80211_CONNECTED)
	{
		add_string(&hx_at_res_buf, (char *)(gwifistatus.connAP.ssid));
	}
	else
	{
		add_string(&hx_at_res_buf, "Disconnected");
	}
	
	return HX_ERROR_SUCCESS;
}

/*
 * 返回
 * +ok<CR><LF><CR><LF>
 * 模块进入smartlink工作模式，nLink开始闪烁，模块医治等待APP推送配置信息
 */
static int hm_atcmd_smtlk(stParam *param)
{
	S32 rlt;
	Boolean smtlnk = TRUE;
	
    init_buffer(&hx_at_res_buf);

	gconfig_set_enable_smart(ICOMM, 0);

    rlt = gconfig_save_config();
    if (rlt != 0)
	{
        printf("<Error>gconfig_save_config failed!!\n");
    }
	
    rlt = remove_sysconfig();
    if (rlt != 0)
	{
        printf("<Error>systemconf remove failed!!\n");
    }

#ifdef __CUSTOMER_HOMI_IOT__	
	hm_update_smtlnk_flag(&smtlnk);
	hx_write_conf();
#endif

	if (!hx_close_active_sock())
	{
		hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
		At_Reboot(param);
		return HX_ERROR_SUCCESS;
	}
	else
	{
		hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
		return HX_ERROR_BLOCK;
	}
	
//	hx_close_active_sock();
//  api_wdog_reboot();
//  return HX_ERROR_SUCCESS;
}

static int hm_atcmd_auto_connb(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	return hx_create_sock(&hx_sokb_info);
}

int hm_get_tcp_timeout()
{
	return hx_sok_info.sok_timeout;
}

int hm_get_tcpb_timeout()
{
	return hx_sokb_info.sok_timeout;
}


#endif

static int hx_atcmd_null(stParam *param)
{
    init_buffer(&hx_at_res_buf);
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_atlt(stParam *param)
{
	int len;
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;

	len = hx_frame_length;
	ret = hx_do_with_one_param(param, &len, &bSave, &bQuery);
	if (bQuery)
	{
		return HX_ERROR_SUCCESS;
	}
	
	if (ret == HX_ERROR_SUCCESS)
	{
		if (len > BUF_SIZE || len < 32)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}

		hx_frame_length = len;

		if (bSave)
		{
			hx_save_to_flash(0, len);
			hx_update_parameter(NULL, NULL, NULL, &hx_frame_length, NULL, NULL, NULL, NULL);
		}
	}

	return ret;
}

static int hx_atcmd_atm(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int mode;
	int ret;
	
	init_buffer(&hx_at_res_buf);
	mode = hx_data_flag;
	ret = hx_do_with_one_param(param, &mode, &bSave, &bQuery);

	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (bQuery)
	{
		return HX_ERROR_SUCCESS;
	}

	if (mode != 0 && mode != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	hx_data_flag = mode;

	if (bSave)
	{
		hx_save_to_flash(0, 0);
		hx_update_parameter(NULL, NULL, NULL, NULL, NULL, NULL, &hx_data_flag, NULL);
	}

	return ret;
}

/*
 * 获取一个参数，该参数为数字
 */
static int hx_do_with_one_param(stParam *param, int *pval, Boolean *psave, Boolean *bQuery)
{
	char *p = param->argv[0];
	Boolean bSave = FALSE;

	if (param->argc < 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if (*p == HX_AT_CMD_QUERY_CHAR)
	{
		init_buffer(&hx_at_res_buf);
		hx_at_res_buf.len = sprintf(hx_at_res_buf.pbuff, "%d", *pval);
		*bQuery = TRUE;

		return HX_ERROR_SUCCESS;
	}

	if (*p == HX_AT_CMD_SAVE_CHAR)
	{
		bSave = TRUE;
		p++;

		param->argv[0] = p;		// 待测试, qinjiangwei 2016/9/13 for power saving at指令调用
	}

	*pval = atoi(p);
	*psave = bSave;

	return HX_ERROR_SUCCESS;
}

/*
 * 获取一个参数，该参数为字串
 */
static int hx_do_with_one_string_param(stParam *param, char *pstr, int len, Boolean *psave)
{
	char *p = param->argv[0];
	Boolean bSave = FALSE;
	int i;
	
	if (param->argc < 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if (*p == HX_AT_CMD_QUERY_CHAR)
	{
		init_buffer(&hx_at_res_buf);
		hx_at_res_buf.len = sprintf(hx_at_res_buf.pbuff, "%s", pstr);

		return HX_ERROR_SUCCESS;
	}

	if (*p == HX_AT_CMD_SAVE_CHAR)
	{
		bSave = TRUE;
		p++;
	}

	if (*p++ != '"')
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	for (i = 0; i < len && *p && *p != '"';)
	{
		pstr[i++] = *p++;
	}

	if (*p != '"')
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if (i == len)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	pstr[i] ='\0';
	
	*psave = bSave;

	return HX_ERROR_SUCCESS;
}


static int hx_atcmd_atpt(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int time;
	int ret;

	init_buffer(&hx_at_res_buf);
	time = hx_atpt_time;
	ret = hx_do_with_one_param(param, &time, &bSave, &bQuery);
	if (ret == HX_ERROR_SUCCESS)
	{
		if (time < 50 || time > 10000)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}

		hx_atpt_time = time;

		if (bSave)
		{
			hx_save_to_flash(0, 0);
			hx_update_parameter(NULL, NULL, &hx_atpt_time, NULL, NULL, NULL, NULL, NULL);
		}
	}

	return ret;
}

static int hx_atcmd_atrm(stParam *param)
{	
	char *p = param->argv[0];
	Boolean bSave = FALSE;
	
	init_buffer(&hx_at_res_buf);
	if (param->argc == 1)
	{
		if (*p == HX_AT_CMD_QUERY_CHAR)
		{
			char buff[128];

			sprintf(buff, "%d,", hx_sok_info.sok_protocol);
			add_string(&hx_at_res_buf, buff);

			sprintf(buff, "%d,", hx_sok_info.sok_cs);
			add_string(&hx_at_res_buf, buff);

			if (hx_sok_info.sok_cs == HX_SERVER_MODE)
			{
				sprintf(buff, "%d,", hx_sok_info.sok_timeout);
			}
			else 
			{
				sprintf(buff, "%s,", hx_sok_info.sok_hostname);
			}
			add_string(&hx_at_res_buf, buff);

			sprintf(buff, "%d", hx_sok_info.sok_port);
			add_string(&hx_at_res_buf, buff);

			return HX_ERROR_SUCCESS;
		}

		return HX_ERROR_INVALID_PARAMETER;
	}

	if (param->argc != 4)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if (*p == HX_AT_CMD_SAVE_CHAR)
	{
		bSave = TRUE;
		p++;
	}

	memset(&hx_sok_info, 0, sizeof(hx_sok_info));
	
	hx_sok_info.sok_protocol = atoi(p);
	p = param->argv[1];
	hx_sok_info.sok_cs = atoi(p);

	if (hx_sok_info.sok_cs == HX_CLIENT_MODE)
	{
		
		if (strlen(param->argv[2]) >= sizeof(hx_sok_info.sok_hostname))
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
	
		strcpy(hx_sok_info.sok_hostname, param->argv[2]);
	}
	else 
	{
		hx_sok_info.sok_timeout = atoi(param->argv[2]);
		
		if (hx_sok_info.sok_timeout < 0 || hx_sok_info.sok_timeout >= 100000)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
	}

	hx_sok_info.sok_port = atoi(param->argv[3]);

	if (hx_sok_info.sok_port < 0 || hx_sok_info.sok_port >= 10000)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if (bSave)
	{
		hx_save_to_flash(0, 0);
		hx_update_parameter(NULL, NULL, NULL, NULL, NULL, NULL, NULL, &hx_sok_info);
	}

	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_bssid(stParam *param)
{
	// 未实现
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_brdssid(stParam *param)
{
	// 未实现
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_chl(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_chll(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_cmdm(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_dns(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_e(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	if (param->argc == 0)
	{
		hx_echo_flag = !hx_echo_flag;
	}
	else
	{
		char *p = param->argv[0];

		if (strcmp(p, "on") == 0)
		{
			hx_echo_flag = TRUE;
		}
		else if (strcmp(p, "off") == 0)
		{
			hx_echo_flag = FALSE;
		}
		else
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
	}

#ifdef __CUSTOMER_HOMI_IOT__	
	hx_update_echo_flag(&hx_echo_flag);
	if (hx_write_conf())
	{
		return HX_ERROR_SUCCESS;
	}
	else
	{
		return HX_ERROR_UNKOWN;
	}
#endif	
}

static int hx_atcmd_encry(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int mode;
	int ret;
	
	init_buffer(&hx_at_res_buf);
	mode = hx_get_current_encrypt_mode();
	ret = hx_do_with_one_param(param, &mode, &bSave, &bQuery);
	if (ret == HX_ERROR_SUCCESS)
	{
		if (mode != NONE && mode != WPA2)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}

		hx_set_encrypt_mode(mode);
		if (bSave)
		{
			hx_save_to_flash(0, 0);
		}
	}

	return ret;
}

static int hx_atcmd_entm(stParam *param)
{
	init_buffer(&hx_at_res_buf);
	hx_data_flag = TRUE;

	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_ents(stParam *param)
{
	init_buffer(&hx_at_res_buf);
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_espc(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ch;
	int ret;

	init_buffer(&hx_at_res_buf);
	ch = hx_esp_char;
	ret = hx_do_with_one_param(param, &ch, &bSave, &bQuery);
	if (ret == HX_ERROR_SUCCESS)
	{
		if (ch >= 255 || ch <= 0)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}

		hx_esp_char = ch;
		if (bSave)
		{
			hx_save_to_flash(0, 0);
			hx_update_parameter(&hx_esp_char, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		}
	}

	return ret;
}

static int hx_atcmd_espt(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int time;
	int ret;

	init_buffer(&hx_at_res_buf);
	time = hx_esp_time;
	ret = hx_do_with_one_param(param, &time, &bSave, &bQuery);
	if (ret == HX_ERROR_SUCCESS)
	{
		if (time < 50 || time > 10000)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}

		hx_esp_time = time;

		if (bSave)
		{
			hx_save_to_flash(0, 0);
			hx_update_parameter(NULL, &hx_esp_time, NULL, NULL, NULL, NULL, NULL, NULL);
		}
	}

	return ret;
}

static int hx_atcmd_iom(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int mode;
	int ret;
	GPIO_CONF conf;

	conf.dirt = INPUT;
	conf.pull_en = FALSE;
	conf.driving_en = FALSE;
	
	if (hx_gpio1_mode == -1)
	{
		pinMode(HX_GPIO_PIN, conf);
		hx_gpio1_mode = INPUT + 1;		// 1  代表输入
	}
	
	init_buffer(&hx_at_res_buf);
	mode = hx_gpio1_mode;
	
	ret = hx_do_with_one_param(param, &mode, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (bQuery)
	{
		return HX_ERROR_SUCCESS;
	}

	printf("mode = %d, hx_gpio1_mode = %d\r\n", mode, hx_gpio1_mode);
	if (mode != hx_gpio1_mode)
	{
		if (mode == INPUT + 1)
		{
			conf.dirt = INPUT;
		}
		else 
		{
			conf.dirt = OUTPUT;
		}
		pinMode(HX_GPIO_PIN, conf);
		
		hx_gpio1_mode = mode;
	}

	if (bSave)
	{
		hx_update_parameter(NULL, NULL, NULL, NULL, NULL, &hx_gpio1_mode, NULL, NULL);
	}

	return HX_ERROR_SUCCESS;
}
	
static int hx_atcmd_ioc(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;
	int val;
	
	init_buffer(&hx_at_res_buf);

	val = 0;
	ret = hx_do_with_one_param(param, &val, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (bQuery)
	{
		if (hx_gpio1_mode != INPUT + 1)
		{
			return HX_ERROR_OP_REFUSED;
		}

		val = digitalRead(HX_GPIO_PIN);
		hx_at_res_buf.len = sprintf(hx_at_res_buf.pbuff, "%d", val);
		
		return HX_ERROR_SUCCESS;
	}

	if (hx_gpio1_mode != OUTPUT + 1)
	{
		return HX_ERROR_OP_REFUSED;
	}

	if (val != 0 && val != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	digitalWrite(HX_GPIO_PIN, val);
	hx_gpio1_status = val;
	
	if (bSave)
	{
		hx_update_gpio1_status((U8 *)&val);
	}
	
	return HX_ERROR_SUCCESS;
}


static int hx_atcmd_key(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int format = HX_ASCII_FORMAT;
	int index = 0;
	int ret;
	char tmp[HX_SSID_LEN];
	
	init_buffer(&hx_at_res_buf);
	ret = hx_do_with_one_param(param, &format, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (bQuery)
	{
		sprintf(tmp, ",%d,", index);
		add_string(&hx_at_res_buf, tmp);
		add_string(&hx_at_res_buf, hx_get_current_key());

		return HX_ERROR_SUCCESS;
	}
	
	hx_shift_param(param);
	ret = hx_do_with_one_param(param, &index, &bSave, &bQuery);
	printf("ret = %d\n", ret);
	if (ret == HX_ERROR_SUCCESS)
	{
		if (index != 0)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
	}
	
	hx_shift_param(param);
	ret = hx_do_with_one_string_param(param, tmp, sizeof(tmp), &bSave);
	if (ret == HX_ERROR_SUCCESS)
	{
		hx_set_current_key(tmp);
	}

	return HX_ERROR_SUCCESS;
}


static int hx_atcmd_lkstt(stParam *param)
{
	char *p;
	int len;
	
	init_buffer(&hx_at_res_buf);
	p = hx_at_res_buf.pbuff;

#ifndef WIN32
	printf("wifi status = %d\r\n", gwifistatus.status);
	if (gwifistatus.status == IEEE80211_CONNECTED)
	{
		len = sprintf(p, "%d,", 1);
	}
	else
	{
		len = sprintf(p, "%d,", 0);
	}
	
	len += sprintf(&p[len], "%d.%d.%d.%d,", uip_ipaddr_to_quad(&(gCabrioConf.local_ip_addr)));
	len += sprintf(&p[len], "%d.%d.%d.%d,", uip_ipaddr_to_quad(&(gCabrioConf.net_mask)));
	len += sprintf(&p[len], "%d.%d.%d.%d,", uip_ipaddr_to_quad(&(gCabrioConf.gateway_ip_addr)));
	len += sprintf(&p[len], "%d.%d.%d.%d", uip_ipaddr_to_quad(&(gCabrioConf.dns_server)));
#endif

	hx_at_res_buf.len = len;
	
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_nip(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_pass(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_pmtf(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	if (hx_write_conf())
	{
		return HX_ERROR_SUCCESS;
	}
	else
	{
		return HX_ERROR_UNKOWN;
	}
}

static int hx_atcmd_qmac(stParam *param)
{
	char buf[8];
	char tmp[10];
	int ret;
	int i;

	init_buffer(&hx_at_res_buf);
	ret = (int)get_local_mac((U8 *)buf, 6);
	if (ret < 0)
	{
		return HX_ERROR_UNKOWN;
	}

	for (i = 0; i < 6; i++)
	{
		sprintf(tmp, "%02X", buf[i]);
		add_string(&hx_at_res_buf, tmp);
	}
	
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_qver(stParam *param)
{
	init_buffer(&hx_at_res_buf);
	
	sprintf(hx_at_res_buf.pbuff, "Project: %s\r\nHWVer: %d.%d\r\nSWVer: %d.%d\r\nBuild Time: %s\r\nRCV: %s",
			S_Ver.szPrjName,
			(S_Ver.btHWVer >> 4) & 0xf, S_Ver.btHWVer & 0xf,
			(S_Ver.btSWVer >> 4) & 0xf, S_Ver.btSWVer & 0xf,
			S_Ver.szBuildTime,
			S_Ver.szRCVer);

	hx_at_res_buf.len = strlen(hx_at_res_buf.pbuff);

	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_rstf(stParam *param)
{
	hx_esp_char = '+';		// 红心物联逃逸字符
	hx_esp_time = 50;		// 红心物联逃逸时间 ms
	hx_atpt_time = 50;		// 红心物联自组帧时间 ms
	hx_frame_length = BUF_SIZE;	// 红心物联自组帧长度
	hx_uart_info.uart_baud_rate = BAUD_38400;
	hx_gpio1_mode = -1;
	hx_data_flag = TRUE;
	memset(&hx_sok_info, 0, sizeof(hx_sok_info));

#ifdef __CUSTOMER_HOMI_IOT__
	memset(&hx_sokb_info, 0, sizeof(hx_sokb_info));
	hx_echo_flag = TRUE;
#endif
	
	init_buffer(&hx_at_res_buf);
	hx_update_parameter(&hx_esp_char, &hx_esp_time, &hx_atpt_time, &hx_frame_length, \
		&hx_uart_info, &hx_gpio1_mode, &hx_data_flag, &hx_sok_info);
	
#ifdef __CUSTOMER_HOMI_IOT__
	hx_update_sockb_parameter(&hx_sokb_info);
	hx_update_echo_flag(&hx_echo_flag);
#endif

	if (hx_write_conf())
	{
		return HX_ERROR_SUCCESS;
	}
	else
	{
		return HX_ERROR_OP_REFUSED;
	}

}

static int hx_atcmd_skcls(stParam *param)
{
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int sok;
	int sok_index;
	int ret;

	init_buffer(&hx_at_res_buf);
	ret = hx_do_with_one_param(param, &sok_index, &bSave, &bQuery);
//	printf("sok_index = %d, ret = %d", sok_index, ret);
	if (sok_index < 0 || sok_index >= HX_SOCKET_NUM)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	sok = hx_socket_array[sok_index];

	printf("sok = %d", sok);
	
	if (ret == HX_ERROR_SUCCESS)
	{
#ifdef WIN32
		unlink_uart2sock(sok);
		unregister_net_line(sok);
		closesocket(sok);
#else
		if (sok == -1)
		{
			return HX_ERROR_INVALID_PARAMETER; 
		}

		if (sok_index == HX_UDP_CLIENT_SOCKET)
		{
			hx_stop_udp_client(sok_index);
		}
		else if (sok_index == HX_UDP_SERVER_SOCKET)
		{
			hx_stop_udp_server();
		}
		else if (sok_index == HX_TCP_CLIENT_SOCKET)
		{
			tcpclose(sok);
		}
#ifdef __CUSTOMER_HOMI_IOT__		
		else if (sok_index == HX_TCP_CLIENT_SOCKETB)
		{
			tcpclose(sok);
		}
		else if (sok_index == HX_UDP_CLIENT_SOCKETB)
		{
			hx_stop_udp_client(sok_index);
		}
#endif
		else if (sok == HX_SPECIAL_TCP_SERVER_SOCK)
		{
			hx_stop_tcp_server();
		}
		else
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
		hx_socket_array[sok_index] = -1;
		return HX_ERROR_SUCCESS;
#endif
	}
	
	return HX_ERROR_INVALID_OP;
}

/*
 * 创建socket后统一返回buff
 * result: 创建socket结果
 * sock: 对应的socket值
 */
static int hx_make_skct_rsp(Boolean result, int sock)
{
	char buff[32];
	
	if (result)
	{
		sprintf(buff, "%d", sock);
		add_string(&hx_at_res_buf, buff);
		
		return HX_ERROR_SUCCESS;
	}
	else
	{
		return HX_ERROR_CONNECT_SOCK;
	}
}

void hx_set_sock(int sock, hx_socket_t type)
{
	hx_socket_array[type] = sock;
}

int hx_get_sock(hx_socket_t type)
{
	return hx_socket_array[type];
}

#ifdef __CUSTOMER_HOMI_IOT__
static int hm_get_sock_group(hx_sock_t *p_sock)
{
	if (p_sock == &hx_sok_info)
	{
		return 0;
	}
	else if (p_sock == &hx_sokb_info)
	{
		return 1;
	}
	else
	{
		printf("invalid sok value!\r\n");
		return -1;	// 
	}
}

/*
 * 毫米科技netp中建立的 tcp/udp client 是互斥的, sockb建立的 tcp/udp client 也是互斥的
 */
static Boolean hm_check_sock_group_free(int group)
{
	int end;
	int i;
	int sk_num;
	
	if (group == 0)
	{
		sk_num = HX_TCP_CLIENT_SOCKET;
		end = HX_UDP_SERVER_SOCKET;
	}
	else if (group == 1)
	{
		sk_num = HX_TCP_CLIENT_SOCKETB;
		end = HX_UDP_SERVER_SOCKETB;
	}
	else
	{
		return FALSE;
	}
	
	for (i = sk_num; i < end; i++)
	{
		if (hx_socket_array[i] != -1)
		{
			return FALSE;
		}
	}

	return TRUE;
}

#endif

static int hx_create_sock(hx_sock_t *p_sock)
{
	sock_status_t status;
	int retsok = -1;
	int group = 0;
	
	if (p_sock->sok_port < 0 || p_sock->sok_port >= 10000)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	memcpy(&hx_sock_info, p_sock, sizeof(hx_sock_t));

#ifdef __CUSTOMER_HOMI_IOT__
	group = hm_get_sock_group(p_sock);
	if (group < 0)
	{
		return HX_ERROR_UNKOWN;
	}

	if (!hm_check_sock_group_free(group))
	{
		return HX_ERROR_OP_REFUSED;
	}
#endif

	if (p_sock->sok_protocol == HX_UDP_PROTOCOL)
	{
		if (p_sock->sok_cs == HX_CLIENT_MODE)
		{
			status = net_udp_connect_server(p_sock->sok_hostname, p_sock->sok_port, HX_UDP_CLIENT_SOCKET + group * 4);
			hx_socket_array[HX_UDP_CLIENT_SOCKET + group * 4] = hx_get_udp_cli_sock(HX_UDP_CLIENT_SOCKET + group * 4);
			if (status == SOCK_STATUS_BLOCK)
			{
				return HX_ERROR_BLOCK;			// 处于异步等待状态, 需要进入到 tcp_server_wifiUartProcess 进行处理
			}

			return hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, HX_UDP_CLIENT_SOCKET + group * 4);
		}
		else
		{
			if (hx_socket_array[HX_UDP_SERVER_SOCKET + group * 4] != -1)
			{
				return HX_ERROR_OP_REFUSED;
			}
			
			status = hx_start_udp_server(p_sock->sok_port, p_sock->sok_timeout);
			hx_socket_array[HX_UDP_SERVER_SOCKET + group * 4] = hx_get_udp_server_sock();
			if (status == SOCK_STATUS_BLOCK)
			{
				return HX_ERROR_BLOCK;			// 处于异步等待状态, 需要进入到 tcp_server_wifiUartProcess 进行处理
			}
			return hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, HX_UDP_SERVER_SOCKET + group * 4);
		}
	}
	else
	{
		if (p_sock->sok_cs == HX_CLIENT_MODE)
		{
#ifdef WIN32
			if (win32_net_connect(p_sock->sok_hostname, p_sock->sok_port, FALSE) == 0)
			{
				char buff[128];

				sprintf(buff, "%d", net_get_hekr_sock());
				add_string(&hx_at_res_buf, buff);

				return HX_ERROR_SUCCESS;
			}
			else 
			{
				return HX_ERROR_CONNECT_SOCK;
			}
#else
#ifdef __CUSTOMER_HOMI_IOT__
			if (p_sock == &hx_sokb_info)
			{
				status = net_tcp_connect_serverb(p_sock->sok_hostname, p_sock->sok_port);
				retsok = HX_TCP_CLIENT_SOCKETB;
			}
			else
#endif
			{
				status = net_tcp_connect_server(p_sock->sok_hostname, p_sock->sok_port, FALSE);
				retsok = HX_TCP_CLIENT_SOCKET;
			}

			if (status == SOCK_STATUS_BLOCK)
			{
				return HX_ERROR_BLOCK;			// 处于异步等待状态, 需要进入到 tcp_server_wifiUartProcess 进行处理
			}
			
			return hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, retsok);
#endif
		}
		else 
		{
			if (hx_socket_array[HX_TCP_SERVER_SOCKET + group * 4] != -1)
			{
				return HX_ERROR_OP_REFUSED;
			}
			
			status = hx_start_tcp_server(p_sock->sok_port, p_sock->sok_timeout);
			if (status == SOCK_STATUS_BLOCK)
			{
				return HX_ERROR_BLOCK;			// 处于异步等待状态, 需要进入到 tcp_server_wifiUartProcess 进行处理
			}
			
			hx_socket_array[HX_TCP_SERVER_SOCKET + group * 4] = HX_SPECIAL_TCP_SERVER_SOCK;
			return hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, HX_TCP_SERVER_SOCKET + group * 4);
		}
	}
}

static int hx_atcm_auto_conn(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	return hx_create_sock(&hx_sok_info);
}

/*
 * 返回值为HX_ERROR_BLOCK表示一个异步操作操作(比如获取主机地址)，需要等待事件
 * udp server 暂时不能使用
 */
static int hx_atcmd_skct(stParam *param)
{
	char *p = param->argv[0];
	
	printf("param->argc = %d", param->argc);

	init_buffer(&hx_at_res_buf);
	if (param->argc != 4)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	hx_sock_info.sok_protocol = atoi(p);

	p = param->argv[1];
	hx_sock_info.sok_cs = atoi(p);

	if (hx_sock_info.sok_cs == HX_CLIENT_MODE)
	{
		if (strlen(param->argv[2]) >= sizeof(hx_sock_info.sok_hostname))
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
		
		strcpy(hx_sock_info.sok_hostname, param->argv[2]);
	}
	else
	{
		hx_sock_info.sok_timeout = atoi(param->argv[2]);
		
		if (hx_sock_info.sok_timeout < 0 || hx_sock_info.sok_timeout >= 100000)
		{
			return HX_ERROR_INVALID_PARAMETER;
		}
	}

	hx_sock_info.sok_port = atoi(param->argv[3]);
	
	return hx_create_sock(&hx_sock_info);
}

static int hx_atcmd_skrcv(stParam *param)
{
	int sock;
	int size;
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;
	net_line_t *pnet;
	char buff[128];

	init_buffer(&hx_at_res_buf);
			
	if (param->argc != 2)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	ret = hx_do_with_one_param(param, &sock, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}

	if (sock < 0 || sock >= HX_SOCKET_NUM)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	sock = hx_socket_array[sock];

	printf("sock = %d\n", sock);

	hx_shift_param(param);
	ret = hx_do_with_one_param(param, &size, &bSave, &bQuery);
	printf("ret = %d\n", ret);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (!(pnet = get_net_line(sock)))
	{
		return HX_ERROR_INVALID_SOCK;
	}
	
//	printf("pnet = %x\n", pnet);
	hx_current_sock = sock;
	hx_recv_size = (size > BUF_SIZE) ? BUF_SIZE : size;
	
	sprintf(buff, "%d", hx_recv_size);
	add_string(&hx_at_res_buf, buff);
	
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_sksdf(stParam *param)
{
	int sock = HX_SOCKET_NUM;
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;
	net_line_t *pnet;
	int real_sock;

	init_buffer(&hx_at_res_buf);
	if (param->argc != 1)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	printf("current sok = %d\r\n", hx_get_current_sock());

	if (hx_get_current_sock() == hx_get_udp_cli_sock(HX_UDP_CLIENT_SOCKET))
	{
		sock = HX_UDP_CLIENT_SOCKET;
	}
	else if (hx_get_current_sock() == hx_get_active_udp_sock())
	{
		sock = HX_UDP_SERVER_SOCKET;
	}
	if (hx_get_current_sock() == net_get_hekr_sock())
	{
		sock = HX_TCP_CLIENT_SOCKET;
	}
	else if (hx_get_current_sock() == hx_get_tcp_server_sock())
	{
		sock = HX_TCP_SERVER_SOCKET;
	}			
#ifdef 	__CUSTOMER_HOMI_IOT__
	else if (hx_get_current_sock() == net_get_sokb())
	{
		sock = HX_TCP_CLIENT_SOCKETB;
	}
#endif	

	ret = hx_do_with_one_param(param, &sock, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (sock < 0 || sock >= HX_SOCKET_NUM)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	real_sock = hx_socket_array[sock];
	if (real_sock == -1)
	{
		return HX_ERROR_INVALID_SOCK;
	}

	if (sock == HX_UDP_SERVER_SOCKET)
	{
		if ((real_sock = hx_get_active_udp_sock()) == -1)	// 没有有效的UDP client连入进来
		{
			return HX_ERROR_OP_REFUSED;
		}
	}
	else if (sock == HX_TCP_SERVER_SOCKET)
	{
		if ((real_sock = hx_get_tcp_server_sock()) == -1)	// 没有有效的TCP client连入进来
		{
			return HX_ERROR_OP_REFUSED;
		}
	}

	if (!(pnet = get_net_line(real_sock)))
	{
		return HX_ERROR_INVALID_SOCK;
	}

	link_uart2sock(real_sock);
	hx_current_sock = real_sock;

	return HX_ERROR_SUCCESS;
}


/*
 *  移除第一个参数
 */
static void hx_shift_param(stParam *param)
{
	int i;

	if (param->argc < 1)
	{
		return;
	}

	for (i = 1; i < param->argc; i++)
	{
		param->argv[i - 1] = param->argv[i];
	}

	param->argc--;
}

static int hx_atcmd_sksnd(stParam *param)
{
	int sock;
	int size;
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;
	net_line_t *pnet;
	char buff[128];

	init_buffer(&hx_at_res_buf);
			
	if (param->argc != 2)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	ret = hx_do_with_one_param(param, &sock, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}
	
	if (sock < 0 || sock >= HX_SOCKET_NUM)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	sock = hx_socket_array[sock];
	
	hx_shift_param(param);
	ret = hx_do_with_one_param(param, &size, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}

	if (!(pnet = get_net_line(sock)))
	{
		return HX_ERROR_INVALID_SOCK;
	}
	
	hx_current_sock = sock;
	hx_send_size = (size > BUF_SIZE) ? BUF_SIZE : size;
	
	sprintf(buff, "%d", hx_send_size);
	add_string(&hx_at_res_buf, buff);

	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_skstt(stParam *param)
{
	int sock;
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;
	char *p;
	int len;
	int real_sock;
	uip_ipaddr_t addr;
	int port;

	init_buffer(&hx_at_res_buf);
	ret = hx_do_with_one_param(param, &sock, &bSave, &bQuery);
	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}

	len = hx_at_res_buf.len;
	p = hx_at_res_buf.pbuff;

	if (sock >= HX_SOCKET_NUM)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	real_sock = hx_socket_array[sock];
	if (real_sock < 0)
	{
		len += sprintf(&p[len], "%d,%d", sock, 0); 		// 断开状态
		hx_at_res_buf.len = len;

		return HX_ERROR_SUCCESS;
	}

	if (sock == HX_TCP_SERVER_SOCKET)
	{
		hx_get_peer_address(&addr, &port);		
	}
	else if (sock == HX_TCP_CLIENT_SOCKET)
	{
		hx_get_client_peer_address(&addr, &port);
	}
	else if (sock == HX_UDP_CLIENT_SOCKET)
	{
		hx_get_udp_client_peer_address(&addr, &port, sock);
	}
#ifdef __CUSTOMER_HOMI_IOT__	
	else if (sock == HX_UDP_CLIENT_SOCKETB)
	{
		hx_get_udp_client_peer_address(&addr, &port, sock);
	}
	else if (sock == HX_TCP_CLIENT_SOCKETB)
	{
		
	}
#endif	
	else
	{
		return HX_ERROR_INVALID_PARAMETER;		//暂时不支持udp server
	}
	
	len += sprintf(&p[len], "%d,%d,%d.%d.%d.%d,%d,%d", sock, 1, uip_ipaddr_to_quad(&addr), port, hx_frame_length);
	hx_at_res_buf.len = len;
	
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_ssid(stParam *param)
{
	char ssid[HX_SSID_LEN];
	Boolean bSave = FALSE;
	int ret;
	
	init_buffer(&hx_at_res_buf);
	strcpy(ssid, hx_get_current_ssid());

	ret = hx_do_with_one_string_param(param, ssid, sizeof(ssid), &bSave);
	if (ret == HX_ERROR_SUCCESS)
	{
		hx_set_current_ssid(ssid);
		
		if (bSave)
		{
			hx_save_to_flash(0, 0);
		}
	}

	return ret;
}

static const int bdrate_array[] = {1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200};

static int hx_get_bdrate(int rate)
{
	int i;

	for (i = 0; i < sizeof(bdrate_array) / sizeof(int); i++)
	{
		if (bdrate_array[i] == rate)
		{
			return i;
		}
	}

	return -1;
}

static int hx_get_uart_baudrate_value(int baud)
{
	if (baud >= BAUD_921600)
	{
		return -1;
	}

	return bdrate_array[baud];
}

static int hx_atcmd_uart(stParam *param)
{
	char *p = param->argv[0];
	Boolean bSave = FALSE;
	int rate;
	int bdrate;
	
	init_buffer(&hx_at_res_buf);
	if (param->argc == 0)		// 兼容毫米科技
	{
		char buff[128];
		int val;

		init_buffer(&hx_at_res_buf);

		val = hx_get_uart_baudrate_value(hx_uart_info.uart_baud_rate);
		if (val < 0)
		{
			return HX_ERROR_UNKOWN;
		}
		
		sprintf(buff, "%d,", val);
		add_string(&hx_at_res_buf, buff);

		sprintf(buff, "%d,", hx_uart_info.uart_data_bit);
		add_string(&hx_at_res_buf, buff);

		sprintf(buff, "%d,", hx_uart_info.uart_stop_bit);
		add_string(&hx_at_res_buf, buff);
		
		sprintf(buff, "%s,", "NONE");
		add_string(&hx_at_res_buf, buff);
		
		sprintf(buff, "%s", "NFC");
		add_string(&hx_at_res_buf, buff);
		
		return HX_ERROR_SUCCESS;
	}
	
	if (param->argc == 1)
	{
		if (*p == HX_AT_CMD_QUERY_CHAR)
		{
			char buff[128];
			int val;

			init_buffer(&hx_at_res_buf);

			val = hx_get_uart_baudrate_value(hx_uart_info.uart_baud_rate);
			if (val < 0)
			{
				return HX_ERROR_UNKOWN;
			}
			
			sprintf(buff, "%d,", val);
			add_string(&hx_at_res_buf, buff);

			sprintf(buff, "%d,", hx_uart_info.uart_data_bit);
			add_string(&hx_at_res_buf, buff);

			sprintf(buff, "%d,", hx_uart_info.uart_stop_bit);
			add_string(&hx_at_res_buf, buff);

			sprintf(buff, "%d", hx_uart_info.uart_parity);
			add_string(&hx_at_res_buf, buff);

			return HX_ERROR_SUCCESS;
		}

		return HX_ERROR_INVALID_PARAMETER;
	}

	if (param->argc != 4 && param->argc != 5)		// 兼容毫米科技
	{
		return HX_ERROR_INVALID_PARAMETER;
	}

	if (*p == HX_AT_CMD_SAVE_CHAR)
	{
		bSave = TRUE;
		p++;
	}

	rate = atoi(p);
	bdrate = hx_get_bdrate(rate);
	if (bdrate < 0)
	{
		return HX_ERROR_INVALID_PARAMETER;
	}
	
	hx_uart_info.uart_baud_rate = bdrate;
	hx_uart_info.uart_data_bit = atoi(param->argv[1]);
	hx_uart_info.uart_stop_bit = atoi(param->argv[2]);
	hx_uart_info.uart_parity = atoi(param->argv[3]);

	if (bSave)
	{
		hx_save_to_flash(0, 0);
		hx_update_parameter(NULL, NULL, NULL, NULL, &hx_uart_info, NULL, NULL, NULL);
	}

	deinit_data_uart_port();
	init_data_uart_port(bdrate, NULL);
	
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_warc(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_warm(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_watc(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_wbgr(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_webs(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

static int hx_atcmd_apst(stParam *param)
{
	return hx_atcmd_wapst(param);
}

static int hx_atcmd_wjoin(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	if (gwifistatus.status == IEEE80211_CONNECTED)
	{
		printf("wjoin connected!\n");
		hx_fill_ap_info(&hx_at_res_buf);
		return HX_ERROR_SUCCESS;
	}
	else 
	{
		process_start(&hx_sta_connect_process, NULL);
		if (hx_get_blk_type() == BLK_NONE)
		{
			return HX_ERROR_OP_REFUSED;
		}
		printf("wjoin here!\n");
		
		return HX_ERROR_BLOCK;
	}
}

static int hx_atcmd_wleav(stParam *param)
{
	init_buffer(&hx_at_res_buf);

#ifndef WIN32	
	printf("wifi status = %d\r\n", gwifistatus.status);
	if (gwifistatus.status == IEEE80211_CONNECTED)
	{
		At_Disconnect(param);
	}
	else
	{
		printf("already disconnect!\r\n");
	}
#endif	
	return HX_ERROR_SUCCESS;
}

static int hx_atcmd_wprt(stParam *param)
{
	return HX_ERROR_NOT_SUPPORT;
}

// TODO: 如何退出省电模式
static int hx_atcmd_wpsm(stParam *param)
{
	Boolean bQuery = FALSE;
	Boolean bSave = FALSE;
	int ret;
	int powersaving = 0;
	
	init_buffer(&hx_at_res_buf);
	
#ifndef WIN32

#endif

	ret = hx_do_with_one_param(param, &powersaving, &bSave, &bQuery);
	if (bQuery)
	{
		return ret;
	}

	if (bSave)
	{
		
	}

	ret = At_POWERSAVE(param);	//	

	return ret;
}

static int hx_atcmd_wapst(stParam *param)
{
	int start = 0;
	Boolean bSave = FALSE;
	Boolean bQuery = FALSE;
	int ret;
	
	init_buffer(&hx_at_res_buf);

	start = hx_ap_mode;
	
	printf("start = %d\r\n", start);
	
	ret = hx_do_with_one_param(param, &start, &bSave, &bQuery);
	if (bQuery)
	{
		return ret;
	}

	if (ret != HX_ERROR_SUCCESS)
	{
		return ret;
	}

	if (bSave)
	{
		// save to flash.
		hx_update_ap_mode(&start);
	}

	if (hx_ap_mode == start)
	{
		return ret;
	}
	
	hx_ap_mode = start;
	if (start)
	{
		gconfig_set_softap_ssid(HX_AP_SSID_NAME);

		softap_init_ex2(192, 168, 0, 1, 0, 0, 0, 1, 50);
		//softap_init_ex2(192, 168, 0, 1, 0, 0, (U8 *)"", 5);
		softap_start();
	}
	else
	{
		softap_exit();
	}

	return ret;	
}

static int hx_atcmd_wscan(stParam *param)
{
	init_buffer(&hx_at_res_buf);
	
	process_start(&hx_sta_connect_process, (const char *)&hx_at_res_buf);
	printf("wscan here!\n");
	
	return HX_ERROR_BLOCK;
}

static int hx_enable_smart_reboot(stParam *param)
{
    S32 rlt = 0;

    printf ("[%s] : +++\n", __func__);
	
    if (*(param->argv[0]) == '0') 
	{
        gconfig_set_enable_smart(NOSMART, 0);
    }
	else if (*(param->argv[0]) == '1')
	{
        gconfig_set_enable_smart(ICOMM, 0);
    }
	else 
	{
        return HX_ERROR_INVALID_PARAMETER;
    }

    rlt = gconfig_save_config();
    if (rlt != 0)
	{
        printf("<Error>gconfig_save_config failed!!\n");
    }
	
    rlt = remove_sysconfig();
    if (rlt != 0)
	{
        printf("<Error>systemconf remove failed!!\n");
    }
	
	if (!hx_close_active_sock())
	{
		hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
		At_Reboot(param);
		return HX_ERROR_SUCCESS;
	}
	else
	{
		hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
		return HX_ERROR_BLOCK;
	}
//	hx_close_active_sock();
//    api_wdog_reboot();

//    return ERROR_SUCCESS;
}

static int hx_atcm_smtlnk_reboot(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	return hx_enable_smart_reboot(param);
}

static Boolean hx_close_active_sock()
{
	int i;
	Boolean ret = FALSE;
	
	for (i = 0; i < HX_SOCKET_NUM; i++)
	{
		int sok = hx_socket_array[i];
		
		if (i == HX_TCP_CLIENT_SOCKET
#ifdef __CUSTOMER_HOMI_IOT__			
			|| i == HX_TCP_CLIENT_SOCKETB
#endif			
			)
		{
			if (sok >= 0)
			{
				printf("closing sok = %d\r\n", sok);
				tcpclose(sok);

				ret = TRUE;
			}
		}
		else
		{
			if (sok >= 0)
			{
				udpclose(sok);
			}
		}
	}

	return ret;
}

static int hx_atcmd_z(stParam *param)
{
	init_buffer(&hx_at_res_buf);

	if (!hx_close_active_sock())
	{
		hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
		At_Reboot(param);
		return HX_ERROR_SUCCESS;
	}
	else
	{
		hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
		return HX_ERROR_BLOCK;
	}
}

int hx_get_esp_char()
{
	return hx_esp_char;
}

int hx_get_esp_time()
{
	return hx_esp_time;
}

int hx_get_frame_length()
{
	return hx_frame_length;
}

int hx_get_atpt_time()
{
	return hx_atpt_time;
}

Boolean hx_get_data_flag()
{
	return hx_data_flag;
}

void hx_set_data_flag(Boolean flag)
{
	hx_data_flag = flag;
}

int hx_get_current_sock()
{
	return hx_current_sock;
}

void hx_set_current_sock(int sock)
{
	hx_current_sock = sock;
}

static void hx_line_chomp(buf_t *b)
{
	char *p = b->pbuff;
	int len = b->len;

	for (; len >= 2; len = b->len)
	{
		if (p[len - 2] == '\r' && p[len - 1] == '\n')
		{
			b->len -= 2;
		}
		else if (p[len - 1] == '\r')
		{
			b->len--;
		}
		else
		{
			break;
		}
	}
}

static int hx_chomp_2(char *p, int len)
{
	int ret = len;

	for ( ; len >= 2; len = ret)
	{
		if (p[len - 2] == '\r' && p[len - 1] == '\n')
		{
			ret -= 2;
		}
		else if (p[len - 1] == '\r')
		{
			ret--;
		}
		else
		{
			break;
		}
	}

	return ret;
}

Boolean hx_check_exit_data_flag()
{
	buf_t *b;
	int i;
	char tmp[HX_ESP_CHAR_NUM + 3];
	
	b = &uart_rx_buff;
	//print_buffer(b);

	for (i = 0; i < HX_ESP_CHAR_NUM; i++)
	{
		tmp[i] = hx_esp_char;
	}

	tmp[i++] = '\r';
	tmp[i++] = '\n';
	tmp[i] = '\0';

	if (strncmp(b->pbuff, tmp, HX_ESP_CHAR_NUM + 2) == 0)
	{
		return TRUE;
	}
	
	if (b->len == HX_ESP_CHAR_NUM)
	{
		for (i = 0; i < HX_ESP_CHAR_NUM; i++)
		{
			if (b->pbuff[i] != hx_esp_char)
			{
				return FALSE;
			}
		}
		
		return TRUE;
	}
	
	return FALSE;
}

int hx_parse_at_cmd()
{
	buf_t *b;
	int ret;

	b = &uart_rx_buff;
	print_buffer(b);
	ret = hx_at_parser(b->pbuff, b->len);
	init_buffer(b);

	return ret;
}

static int parseBuff2Param(char* bufCmd, stParam* pParam, U8 maxargu)
{
	int buflen;
	const char *sep = ",";
	char *p;

	buflen = strlen(bufCmd);
	if (buflen == 0) 
	{
		return -1;
	}

	if (maxargu == 0)
	{
		maxargu = MAX_ARGUMENT;
	}
	
	p = strtok(bufCmd, sep);
	if (!p)
	{
		pParam->argc = 1;
		pParam->argv[pParam->argc - 1] = &bufCmd[0];
		
		return 0;
	}

	for (; p != NULL; p = strtok(NULL, sep))
	{
		pParam->argc += 1;
		pParam->argv[pParam->argc - 1] = p;
	}

	return 0;
}

/* 
 * 将参数加入到hx_at_cmd的buff中，变成AT+TCPDIS'\0'on
 */
static int hx_append_param(char *buff, int len, stParam *param)
{
	int i;
	int index;

	index = strlen(buff);
	for (i = 0; i < param->argc; i++)
	{
		char *p;
		int n;

		p = param->argv[i];
		n = strlen(p);

		index++;		// skip the '\0'
		if (index + n >= len)
		{
			return -1;	// 超出buff
		}

		strcpy(&buff[index], p);
		index += n;
	}

	return 0;
}

static int hx_at_parser_help(char *buff, int len)
{
	int i = 0;
	int	nRet = -3;
	char cmd[32], operat = 0;
	stParam param;

	memset(&param, 0, sizeof(stParam));
	if ((1 == len) && (buff[0] == '\r' || buff[0] == '\n'))
	{
		nRet = HX_ERROR_SUCCESS;
		goto exit;
	}

	if (0 == len) 
	{
		printf(HX_ATRSP_ERROR, -3);
		return HX_ERROR_INVALID_PARAMETER;
	}

	len = hx_chomp_2(buff, len);

	buff[len] = 0x0;  //chomp!! replace \r\n with null string

	for (i = 0; i <= len; i++)
	{
		if (buff[i] == 0 || buff[i] == '=' || buff[i] == ' ')
		{
			memcpy(cmd, buff, i);
			operat = buff[i];
			cmd[i] = 0;
			break;
		}
	}

	for (i = 0; i < sizeof(hx_atcmdicomm_info_tbl) / sizeof(at_cmd_info); i++)
	{
		if (hx_atcmdicomm_info_tbl[i].atCmd && strcmp(hx_atcmdicomm_info_tbl[i].atCmd, cmd) == 0)
		{
			if (operat != 0)
			{
				parseBuff2Param(buff + strlen(hx_atcmdicomm_info_tbl[i].atCmd) + 1, &param, hx_atcmdicomm_info_tbl[i].maxargu);
			}

			memset(hx_at_cmd, 0, sizeof(hx_at_cmd));
			strncpy(hx_at_cmd, cmd, sizeof(hx_at_cmd) - 1);
			hx_append_param(hx_at_cmd, sizeof(hx_at_cmd), &param);
			
			nRet = hx_atcmdicomm_info_tbl[i].pfHandle(&param);
			goto exit_rsp;
		}
	}

	//Handle Customer AT Command
	i = 0;
	while (1)
	{
		if (hx_atcmdicomm_info_tbl[i].pfHandle == NULL ||\
			hx_atcmdicomm_info_tbl[i].atCmd == NULL || \
			strlen(hx_atcmdicomm_info_tbl[i].atCmd) < 0)
		{
			nRet = -3;
			break;
		}

		//local variable
		if (strcmp(hx_atcmdicomm_info_tbl[i].atCmd, cmd) == 0)
		{
			if (operat != 0)
			{
				parseBuff2Param(buff + strlen(hx_atcmdicomm_info_tbl[i].atCmd) + 1,	&param, 0);
			}

			nRet = hx_atcmdicomm_info_tbl[i].pfHandle(&param);
			goto exit_rsp;
		}

		i++;
	}

exit_rsp:
	if (nRet == HX_ERROR_BLOCK)
	{
		return nRet;
	}
	else
	{
		hx_at_rsp_ok(cmd, nRet);
	}

exit:
	return nRet;
}

static int hx_at_parser(char *buff, int len)
{
	const char *sep = "\r\n";
	char *p;
	char *ptr = buff;
	int ret = HX_ERROR_SUCCESS;
	
	buff[len] = '\0';
	
	for (p = strtok(ptr, sep); p != NULL; )
	{
		ret = hx_at_parser_help(p, strlen(p));
		
		p = strtok(NULL, sep);
	}

	return ret;
}


void hx_at_rsp_ok(char *cmd, int code)
{
	buf_t b;
	Boolean ret;

	init_buffer(&b);

	if (hx_echo_flag)
	{
		add_string(&b, cmd);
		add_string(&b, HX_AT_RSP_LINE_END);
	}

	if (code == HX_ERROR_SUCCESS)
	{
		add_string(&b, HX_ATRSP_OK);
		if (hx_at_res_buf.len > 0)		// 有结果需要返回
		{
			add_string(&b, "=");
			append_buf(&b, &hx_at_res_buf);
		}
	}
	else
	{
		b.len += sprintf(&b.pbuff[b.len], HX_ATRSP_ERROR, code);
	}

	add_string(&b, HX_AT_RSP_LINE_END);

#ifdef __CUSTOMER_HOMI_IOT__
	if (strcmp(cmd, HX_ATCMD_AUTO_CONN_START) == 0 || strcmp(cmd, HM_ATCMD_AUTO_CONN_B_START) == 0)
	{
		return;
	}
#endif

	ret = add_buf(&uart_tx_queue, &b);
	if (ret)
	{
		send_uart();
	}
}

/*
 * return value:
 * -1 : 不需要传送数据
 * 1  : 数据量还没有达到传送大小
 * 2  : 数据量达到传送大小
 */
int hx_check_sok_send()
{
	if (hx_send_size == 0)
	{
		return HX_NO_DATA;
	}

	if (uart_rx_buff.len < hx_send_size)
	{
		return HX_DATA_NOT_FULL;
	}
	else
	{
		uart_rx_buff.len = hx_send_size;		// 仅仅发送需要发送的数据大小
		return HX_DATA_FULL;
	}
}

int hx_check_sok_recv(buf_t *b)
{
	if (hx_recv_size == 0)
	{
		return HX_NO_DATA;
	}

	if (b->len < hx_recv_size)
	{
		return HX_DATA_NOT_FULL;
	}
	else
	{
		b->len = hx_recv_size;
		return HX_DATA_FULL;
	}
}


void hx_clear_sok_sendsize()
{
	hx_send_size = 0;
}

void hx_clear_sok_recvsize()
{
	hx_recv_size = 0;
}

void hx_report_startup()
{
	buf_t b;
	Boolean ret;

	init_buffer(&b);


	add_string(&b, HX_AT_STARTUP);

	ret = add_buf(&uart_tx_queue, &b);
	if (ret)
	{
		send_uart();
	}
}

void test_uart_cmd()
{
	buf_t b;

	init_buffer(&b);

	strcpy(b.pbuff, "AT+ATM=?\r");
	b.len = strlen("AT+ATM=?\r");

	hx_at_parser(b.pbuff, b.len);


	strcpy(b.pbuff, "AT+\r");
	b.len = strlen("AT+\r");
	hx_at_parser(b.pbuff, b.len);

	strcpy(b.pbuff, "AT+ATPT=?\r");
	b.len = strlen("AT+ATPT=?\r");
	hx_at_parser(b.pbuff, b.len);

}



#endif

void link_uart2sock(int s)
{
	pnet = get_net_line(s);
	if (pnet == NULL)
	{
		printf("sock is register@%d\n", __LINE__);
	}
}

net_line_t * uart_get_net_line()
{
	return pnet;
}

void unlink_uart2sock(int s)
{
	if (pnet == get_net_line(s))	// 重要,是自己对应注册的pnet，则置0
	{
		pnet = NULL;
	}
}


Boolean init_uart_buff()
{
	Boolean ret = FALSE;
	static char uart_buff[1024 * 10];

	init_mem_pool(NULL);
	init_buff_queue(&uart_tx_queue, "uart tx");
	init_buff_queue(&uart_rx_queue, "uart rx");

	init_buffer(&uart_rx_buff);

	uart_rx_buff.size = sizeof(uart_buff);
	uart_rx_buff.pbuff = uart_buff;
	
#ifdef WIN32
	if ((ret = win32_UART_Open(HX_WIN32_UART_PORT)))
	{
		win32_start_thread(win32_uart_recv_thread, NULL);

		return TRUE;
	}
	else 
	{
		printf("open uart%d failed!\n", HX_WIN32_UART_PORT);
		return FALSE;
	}
#endif

	return TRUE;
}

static unsigned char g_uart_buff[MAX_PORT_NUM][BUF_SIZE] = {0};

/************************************************************************/
/* 
功能：初始化指定串口数据
port:串口号
返回值：成功：TRUE，失败:FALSE
*/
/************************************************************************/
Boolean init_uart_buff_ex()
{
	int i;

	init_mem_pool(NULL);

	for (i = 0; i < MAX_PORT_NUM; i++)
	{
		init_buffer(&g_uart_rx_buff[i]);

		g_uart_rx_buff[i].size = sizeof(g_uart_buff[i]);
		g_uart_rx_buff[i].pbuff = g_uart_buff[i];
		clear_uart_rx_buff(i);
	}

	return TRUE;
}

/************************************************************************/
/* 
功能：清除指定串口数据，其实是把接收串口数据的队列清空
port:串口号
*/
/************************************************************************/
void clear_uart_rx_buff(int port)
{
	memset(g_uart_rx_buff[port].pbuff, 0, g_uart_rx_buff[port].size);
	g_uart_rx_buff[port].len = 0;
}

/************************************************************************/
/* 
功能：读取串口数据
port:串口号
out_buf：接收串口数据buff
buf_len: out_buf的长度
actual_len：实际读取数据的长度
返回值：成功：TRUE，失败:FALSE
*/
/************************************************************************/
Boolean get_uart_rx_buff(int port, unsigned char *out_buf, int buf_len, int *actual_len)
{
	Boolean bRet = TRUE;
	int data_len = g_uart_rx_buff[port].len;

	if (data_len > buf_len)
	{
		data_len = buf_len;
		bRet = FALSE;
	}

	memcpy(out_buf, g_uart_rx_buff[port].pbuff, data_len);
	*actual_len = data_len;

	return bRet;
}

/************************************************************************/
/* 
功能：打开指定串口
port:串口号
返回值：成功：TRUE，失败:FALSE
*/
/************************************************************************/
Boolean init_serial_port(int port)
{
	Boolean ret = FALSE;
	static int nPort = 0;

#ifdef WIN32
	if ((ret = win32_UART_Open_ex(port)))
	{
		nPort = port;
		m_bUartThreadExit = FALSE;
		//init_uart_buff_ex(port);
		win32_start_thread(win32_uart_recv_thread_ex, &nPort);

		return TRUE;
	}
	else 
	{
		printf("open uart%d failed!\n", HX_WIN32_UART_PORT);
		return FALSE;
	}
#endif
	return TRUE;
}


static Boolean add_uart_rx_to_queue()
{
	Boolean ret;
	
	ret = add_buf(&uart_rx_queue, &uart_rx_buff);
	init_buffer(&uart_rx_buff);	

	return ret;
}

/*
 * 返回值为FALSE, 表示需要进行下一步处理
 */
Boolean input_char(int input)
{
	Boolean ret;

	ret = add_char(&uart_rx_buff, input);
	
	if (!ret)
	{
		if (!add_uart_rx_to_queue())
		{
			printf("uart rx queue overflow11\n");
		}

		add_char(&uart_rx_buff, input);
	}
#if defined(__HX_IOT_SUPPORT__)
	if (uart_rx_buff.len >= hx_frame_length)
	{
		if (!add_uart_rx_to_queue())
		{
			printf("uart rx queue overflow22\n");
		}

		ret = FALSE;
	}
#endif

	return ret;
}

/*
 * 检查最后一个串口接收包，加到网络发送队列中
 */
Boolean add_rx_buffer()
{
	Boolean ret = FALSE;

	if (uart_busy)
	{
		return TRUE;
	}

	if (uart_rx_buff.len == 0)
	{
		printf("uart rx buffer is empty!");
		return FALSE;
	}

	if (pnet == NULL)
	{
		init_buffer(&uart_rx_buff);		// 未注册到网络链路上,丢掉数据
		return FALSE;
	}

	//printf("add_rx_buffer\r\n");
	uart_busy = TRUE;

	ret = add_buf(&pnet->tx_queue, &uart_rx_buff);
//	if (ret)		// 添加不成功，直接丢弃
	{
		init_buffer(&uart_rx_buff);
	}
	uart_busy = FALSE;

	return ret;
}

/*
 * 将buf添加到uart发送队列中。
 */
Boolean add_uart_tx_buffer()
{
	Boolean ret = FALSE;

	if (uart_rx_buff.len == 0)
	{
		printf("uart rx buffer is empty!");
		return FALSE;
	}

	ret = add_buf(&uart_tx_queue, &uart_rx_buff);
	if (ret)
	{
		init_buffer(&uart_rx_buff);
	}

	return ret;
}


void uart_send_callback(Boolean bOk)
{
	buf_t *b;

	printf("uart_send_callback @%d\n", __LINE__);
	if (!bOk)
	{
		return;
	}

	b = remove_buf(&uart_tx_queue);
	if (!b)
	{
		//assert(FALSE);
		printf("uart_send_callback b == NULL @%d\n", __LINE__);
	}

	// 发送队列中还有数据，继续发送
	if (get_buf(&uart_tx_queue))
	{
		send_uart();
	}
}

Boolean uart_send_buf(buf_t *b, send_cb fn)
{
	char tmp[256];
	int n;

	n = (b->len >= sizeof(tmp))? (sizeof(tmp) - 1) : b->len;

	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, b->pbuff, n);

	b->count++;

	Log_d(_T("++++++++++++\r\n"));
	print_buffer(b);
	Log_d(_T("++++++++++++\r\nsend UART len = %d, count = %d\n"), b->len, b->count);
#ifdef WIN32
	win32_UART_PutBytes(HX_WIN32_UART_PORT, b->pbuff, b->len);
#endif
	if (fn)
	{
		fn(TRUE);
	}

	return TRUE;
}


void send_uart()
{
	buf_t *b;

	b = get_buf(&uart_tx_queue);
	if (!b)
	{
		printf("null buf!\n");
		return;
	}

#ifdef WIN32
	uart_send_buf(b, uart_send_callback);
#else
	print_buffer(b);
	SerialWrite((U8 *)(b->pbuff), b->len);
	uart_send_callback(TRUE);
#endif
}

#if defined(__HX_IOT_SUPPORT__)
static Boolean hx_check_tcp_sock(int sock)
{
	return 0 <= sock && sock < UIP_CONNS;
}

/*
 * 判断是否有自动连接设置
 */
Boolean hx_check_auto_connect()
{
	Boolean ret;
	
	ret = (hx_data_flag == TRUE) && (hx_sok_info.sok_port > 0);
	if (ret)
	{
		hx_auto_conn_count++;
	}
#ifdef __CUSTOMER_HOMI_IOT__
	if (!ret)
	{
		ret = (hx_data_flag == TRUE) && (hx_sokb_info.sok_port > 0);
		
		if (ret)
		{
			hx_auto_conn_count++;
		}
	}
#endif

	printf("hx_auto_conn_count = %d\r\n", hx_auto_conn_count);

	return ret;
}

/*
 * 发出skct命令
 */
static int hx_emit_auto_conn_at_cmd_help(hx_sock_t *p_sock)
{
	int len;
	char *p = uart_rx_buff.pbuff;
	int cmd_status = 0;
	
	init_buffer(&uart_rx_buff);

	len = sprintf(p, HX_ATCMD_AUTO_CONN"\r\n");
	uart_rx_buff.len = len;

	cmd_status = hx_parse_at_cmd();

	return cmd_status;
}

static int hx_emit_auto_conn_start_at_cmd_help(hx_sock_t *p_sock)
{
	int len;
	char *p = uart_rx_buff.pbuff;
	int cmd_status = 0;
	
	init_buffer(&uart_rx_buff);

	len = sprintf(p, HX_ATCMD_AUTO_CONN_START"\r\n");
	uart_rx_buff.len = len;

	cmd_status = hx_parse_at_cmd();
	
	return cmd_status;
}

#ifdef __CUSTOMER_HOMI_IOT__
/*
 * 发出AT+TCPDISPB命令
 */
static int hm_emit_auto_connb_at_cmd()
{
	int len;
	char *p = uart_rx_buff.pbuff;
	int cmd_status = 0;
	
	init_buffer(&uart_rx_buff);

	len = sprintf(p, HM_ATCMD_AUTO_CONN_B"\r\n");
	uart_rx_buff.len = len;

	cmd_status = hx_parse_at_cmd();

	return cmd_status;
}

int hm_emit_auto_connb_at_cmd_start()
{
	int len;
	char *p = uart_rx_buff.pbuff;
	int cmd_status = 0;
	
	init_buffer(&uart_rx_buff);

	len = sprintf(p, HM_ATCMD_AUTO_CONN_B_START"\r\n");
	uart_rx_buff.len = len;

	cmd_status = hx_parse_at_cmd();

	return cmd_status;
}

#endif

/*
 * 发出skct命令
 */
int hx_emit_skct_at_cmd()
{
	int status = HX_ERROR_SUCCESS;
	
	if (hx_sok_info.sok_port > 0)
	{
		status = hx_emit_auto_conn_at_cmd_help(&hx_sok_info);
	}

	return status;
}

int hx_emit_skct_start_at_cmd()
{
	int status = HX_ERROR_SUCCESS;
	
	if (hx_sok_info.sok_port > 0)
	{
		status = hx_emit_auto_conn_start_at_cmd_help(&hx_sok_info);
	}

	return status;
}

int hx_do_with_uart_frame()
{
	int cmd_status = 0;
	Boolean flag_back;
	
	if (hx_get_data_flag())
	{
		if (!hx_check_exit_data_flag())
		{
			hx_set_data_flag(TRUE);
#ifdef __CUSTOMER_HOMI_IOT__
			net_add_rx_net_line_all();
			net_send_net_line_all();
#else
			add_rx_buffer();
			net_send_tx_queue(hx_get_current_sock(), FALSE,  hx_check_tcp_sock(hx_get_current_sock())? net_tcp_send_cb : net_udp_send_cb);
#endif			
			
		}
		else
		{
			hx_set_data_flag(FALSE);
			printf("enter cmd mode!\r\n");
			//ret = hx_parse_at_cmd();
			
			flag_back = hx_echo_flag;
			hx_echo_flag = FALSE;
			init_buffer(&hx_at_res_buf);
			hx_at_rsp_ok(NULL, HX_ERROR_SUCCESS);
			hx_echo_flag = flag_back;
			
			init_buffer(&uart_rx_buff);
		}
	}
	else
	{
		int ret = hx_check_sok_send();

		if (hx_is_blocking)
		{
			printf("at cmd is blocking..\r\n");
		//	return 0;			
		}

		if (ret == HX_NO_DATA)
		{
			cmd_status = hx_parse_at_cmd();
		}
		else if (ret == HX_DATA_NOT_FULL)
		{
			// 继续接收串口数据
		}
		else
		{
			add_rx_buffer();
			net_send_tx_queue(hx_get_current_sock(), FALSE,  hx_check_tcp_sock(hx_get_current_sock())? net_tcp_send_cb : net_udp_send_cb);
			hx_clear_sok_sendsize();
		}
	}

	if (cmd_status == HX_ERROR_BLOCK)
	{
		hx_is_blocking = TRUE;
	}
	else
	{
		hx_is_blocking = FALSE;
	}

	return cmd_status;
}

/*
 * 判断是否发出了连接指令
 */
static Boolean hx_check_conn_instruction()
{
	char *p;
	int len;
	
	p = hx_at_cmd;
	len = strlen(p);

	if (strcmp(p, HX_ATCMD_AUTO_CONN) == 0 || strcmp(p, HX_ATCMD_SKCT) == 0 || strcmp(p, HX_ATCMD_AUTO_CONN_START) == 0
#ifdef __CUSTOMER_HOMI_IOT__		
		|| strcmp(p, HM_ATCMD_AUTO_CONN_B) == 0 
		|| strcmp(p, HM_ATCMD_AUTO_CONN_B_START) == 0 
#endif		
	)
	{
		return TRUE;
	}

#ifdef __CUSTOMER_HOMI_IOT__	
	if (strcmp(p, HM_ATCMD_TCPDIS) == 0 || strcmp(p, HM_ATCMD_TCPDISB) == 0)
	{
		p += len;
		p++;

		if (strcmp(p, "on") == 0)
		{
			return TRUE;
		}
	}
#endif	
	
	return FALSE;
}


void hx_set_conn_success(hx_socket_t sok_index)
{
	if (sok_index == HX_TCP_CLIENT_SOCKET)
	{
		hx_socket_array[HX_TCP_CLIENT_SOCKET] = net_get_hekr_sock();
		hx_at_rsp_ok(hx_at_cmd, hx_make_skct_rsp(TRUE, HX_TCP_CLIENT_SOCKET));
	}
#ifdef __CUSTOMER_HOMI_IOT__	
	else if (sok_index == HX_TCP_CLIENT_SOCKETB)
	{
		hx_socket_array[HX_TCP_CLIENT_SOCKETB] = net_get_sokb();
		hx_at_rsp_ok(hx_at_cmd, hx_make_skct_rsp(TRUE, HX_TCP_CLIENT_SOCKETB));
	}
#endif	
}

int hx_do_with_blk(block_type_t blk)
{
	sock_status_t status;
	int code;
	
	//init_buffer(&hx_at_res_buf);

	printf("hx_do_with_blk blk=%d\r\n", blk);
	hx_is_blocking = FALSE;
	switch (blk & 0xFF)
	{
		case BLK_TCP_NSLOOKUP:
		{
			int group = 0;
#ifdef __CUSTOMER_HOMI_IOT__			
			if (strcmp(hx_at_cmd, HM_ATCMD_SOCKB) == 0)
			{
				group = 1;
			}
#endif
			if (group == 0)
			{
				status = net_tcp_connect_server(hx_sock_info.sok_hostname, hx_sock_info.sok_port, FALSE);
				if (status == SOCK_STATUS_BLOCK)
				{
					printf("error here!\n");
					return -1;
				}

				hx_socket_array[HX_TCP_CLIENT_SOCKET] = net_get_hekr_sock();
				code = hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, HX_TCP_CLIENT_SOCKET);
				hx_at_rsp_ok(hx_at_cmd, code);
			}
#ifdef __CUSTOMER_HOMI_IOT__			
			else
			{
					status = net_tcp_connect_serverb(hx_sock_info.sok_hostname, hx_sock_info.sok_port);
					hx_socket_array[HX_TCP_CLIENT_SOCKETB] = net_get_sokb();
					code = hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, HX_TCP_CLIENT_SOCKETB);
					hx_at_rsp_ok(hx_at_cmd, code);
			}
#endif			
			break;
		}
		
		case BLK_UDP_NSLOOKUP:
		{
			int group = 0;
			
#ifdef __CUSTOMER_HOMI_IOT__			
			if (strcmp(hx_at_cmd, HM_ATCMD_SOCKB) == 0)
			{
				group = 1;
			}
#endif

			status = net_udp_connect_server(hx_sock_info.sok_hostname, hx_sock_info.sok_port, HX_UDP_CLIENT_SOCKET + group * 4);
			if (status == SOCK_STATUS_BLOCK)
			{
				printf("error here!\n");
				return -1;
			}

			hx_socket_array[HX_UDP_CLIENT_SOCKET + group * 4] = hx_get_udp_cli_sock(HX_UDP_CLIENT_SOCKET + group * 4);			
			code = hx_make_skct_rsp(status == SOCK_STATUS_SUCCESS, HX_UDP_CLIENT_SOCKET + group * 4);
			hx_at_rsp_ok(hx_at_cmd, code);

			break;
		}

		case BLK_TCP_CONNECT:
		{
			hx_socket_array[HX_TCP_CLIENT_SOCKET] = net_get_hekr_sock();
			hx_at_rsp_ok(hx_at_cmd, hx_make_skct_rsp(TRUE, HX_TCP_CLIENT_SOCKET));
			
			break;
		}
		
#ifdef __CUSTOMER_HOMI_IOT__	
		case BLK_TCP_CONNECTB:
		{
			hx_socket_array[HX_TCP_CLIENT_SOCKETB] = net_get_sokb();
			hx_at_rsp_ok(hx_at_cmd, hx_make_skct_rsp(TRUE, HX_TCP_CLIENT_SOCKETB));
			
			break;
		}
#endif	

		case BLK_UDP_CONNECT:
		{

			break;
		}

		case BLK_WSCAN:
		{
			printf("hx scan block continue\r\n");
			
			hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
			break;
		}

		case BLK_WJOIN:
		{
			printf("hx wjoin block continue\r\n");
			if (hx_get_wifi_connect_status() == HX_CONNECT_OK)
			{
				hx_fill_ap_info(&hx_at_res_buf);
				hx_at_rsp_ok(hx_at_cmd, HX_ERROR_SUCCESS);
			}
			else 
			{
				hx_at_rsp_ok(hx_at_cmd, HX_ERROR_UNKOWN);
			}
			
			break;
		}

		case BLK_TCP_CLOSE:
		{
			printf("blk_tcp_close\r\n");

			if (strcmp(hx_at_cmd, HX_ATCMD_Z) == 0 || strcmp(hx_at_cmd, HX_ATCMD_SMARTLINK) == 0
#ifdef __CUSTOMER_HOMI_IOT__	
				|| strcmp(hx_at_cmd, HM_ATCMD_SMTLK) == 0
				|| strcmp(hx_at_cmd, HM_ATCMD_RELD) == 0
#endif
				)
			{
				if (hx_socket_array[HX_TCP_CLIENT_SOCKET] < 0 
#ifdef __CUSTOMER_HOMI_IOT__						
					&& hx_socket_array[HX_TCP_CLIENT_SOCKETB] < 0
#endif					
					)
				{
					hx_start_reboot_timer();		// 2s 后重启，解决重启时, tcp 连接 还没有完全关掉，系统就重启了
				}
			}
#ifdef __CUSTOMER_HOMI_IOT__
			else if (net_get_tcp_close_reason() == HX_TCP_SOCK_TIMEOUT)	// 注意本行必须放此地
			{
				net_set_tcp_close_reason(HX_TCP_SOCK_INITIAL);
				hx_start_reconn_timer(HX_RECONN_SOCK_EVENT);
			}
			else if (net_get_tcpb_close_reason() == HX_TCP_SOCKB_TIMEOUT)
			{
				net_set_tcpb_close_reason(HX_TCP_SOCK_INITIAL);
				hx_start_reconn_timer(HX_RECONN_SOCKB_EVENT);
			}
#endif
			else if (hx_check_conn_instruction())		// 本行必须放末尾，先判断是否timeout导致tcpclose
			{
				hx_at_rsp_ok(hx_at_cmd, HX_ERROR_CONNECT_SOCK);
			}
			
			break;
		}

		default:
			break;

	}

	return 0;
}


static void hx_fill_ap_info(buf_t *b)
{
	TAG_AP_INFO apinfo;
	char *ptr;
	int index;
	
	get_connected_ap_info(&apinfo);

	ptr = &b->pbuff[b->len];
	index = 0;

	index += sprintf(&ptr[index], "%x%x%x%x%x%x,", apinfo.mac[0],apinfo.mac[1],apinfo.mac[2],
	                      apinfo.mac[3],apinfo.mac[4],apinfo.mac[5]);
	
	index += sprintf(&ptr[index], "%d,", 0);
	index += sprintf(&ptr[index], "%d,", apinfo.channel);
	index += sprintf(&ptr[index], "%d,", apinfo.security_type);
	index += sprintf(&ptr[index], "\"%s\",", apinfo.name);
	index += sprintf(&ptr[index], "%d", apinfo.rssi);

	b->len += index;
}
#endif


#ifndef WIN32
static Boolean bDataUartInited = FALSE;
static uart_cb_fn_t fn_uart = NULL;

void init_data_uart_port(BAUD_RATE rate, uart_cb_fn_t fn)
{
	if (!bDataUartInited)
	{
		if (fn != NULL)
		{
			SerialInit(rate, fn, NULL);
			fn_uart = fn;
		}
		else 
		{
			if (fn_uart)
			{
				SerialInit(rate, fn_uart, NULL);
			}
			else
			{
				printf("default uart callback is null!\r\n");
			}
		}
		
		bDataUartInited = TRUE;
	}
}

void deinit_data_uart_port()
{
	if (bDataUartInited)
	{
		SerialDeinit();
		bDataUartInited = FALSE;
	}
}

#if defined(__HX_IOT_SUPPORT__)
void hx_start_reboot_timer()
{
//	process_start(&hx_timer_checker, NULL);
	etimer_set_icomm(&sTm, CLOCK_SECOND * 2, &hx_timer_checker);
	hx_timer_event = HX_REBOOT_EVENT;
}

void hx_start_reconn_timer(int event)
{
//	process_start(&hx_timer_checker, NULL);
	etimer_set_icomm(&sTm, CLOCK_SECOND , &hx_timer_checker);
	hx_timer_event = event;
}

void hx_do_check_auto_connect()
{
	if (hx_check_auto_connect())
	{
		hx_start_auto_conn_timer();
	}
}

static void hx_start_auto_conn_timer()
{
	printf("hx_start_auto_conn_timer\r\n");
//	process_start(&hx_timer_checker, NULL);

	etimer_set_icomm(&sTm, CLOCK_SECOND * 2, &hx_timer_checker);
	hx_timer_event = HX_START_EVENT;
}

#if 0
static Boolean hx_check_auto_conn()
{
	static int delay_count = 0;
	static int conn_sockb;
	static Boolean smtlk_mode = FALSE;
	ieee80211_state eStatus;

#ifdef __CUSTOMER_HOMI_IOT__	
	if (smtlk_mode == FALSE) 
	{
		hm_init_smtlnk_flag(&smtlk_mode);
	}
#endif

	eStatus = get_wifi_connect_status();
	printf("eStatus = %d, smtlk_mode = %d\r\n", eStatus, smtlk_mode);
	
	if (eStatus == IEEE80211_CONNECTED)
	{
#ifdef __CUSTOMER_HOMI_IOT__	
		if (smtlk_mode == TRUE)
		{
			Boolean dirty = FALSE;
			Boolean flag = FALSE;

			if (strcmp((char *)(gwifistatus.connAP.key), hx_get_current_key()) != 0)
			{
				hx_set_current_key((char *)(gwifistatus.connAP.key));
				dirty = TRUE;
			}

			if (strcmp((char *)(gwifistatus.connAP.ssid), hx_get_current_ssid()) != 0)
			{
				hx_set_current_ssid((char *)(gwifistatus.connAP.ssid));
				dirty = TRUE;
			}

			hm_update_smtlnk_flag(&flag);
			hx_write_conf();
			
			smtlk_mode = FALSE;
		}
#endif		
		
		delay_count++;
#ifdef __CUSTOMER_HOMI_IOT__		
		conn_sockb = (hx_sokb_info.sok_port > 0);
#else
		conn_sockb = 0;
#endif
		if (delay_count == HX_SOCK_CONN_TIME)		// 16s 之后自动连接
		{
			if (hx_sok_info.sok_port > 0)
			{
				//process_post(&tcp_server_wifiUartProcess, HX_PROCESS_EVENT_AUTO_CONN, &conn_sockb);
				int count;

				for (count = 0; count < MAX_RECONN_COUNT; count++)
				{
					printf("count = %d\r\n", count);
					if (hx_emit_skct_start_at_cmd() == HX_ERROR_BLOCK)
					{
						PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT || ev == PROCESS_EVENT_CONTINUE);
					
					    if (ev == PROCESS_EVENT_EXIT)
						{
					        printf("exit\r\n");
							break;
					    } 
						else if (ev == PROCESS_EVENT_CONTINUE)
						{
					    	block_type_t blk = *(int *)data;
							
							if (blk == BLK_TCP_CONNECT)
							{
								hx_set_conn_success(HX_TCP_CLIENT_SOCKET);
								break;
							}
							else if (blk == BLK_TCP_CLOSE)
							{
								etimer_set(&timeout, 1 * CLOCK_SECOND);
								PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
							}
					    }
					}
				}
#ifdef __CUSTOMER_HOMI_IOT__
				if (conn_sockb)
				{
					for (count = 0; count < MAX_RECONN_COUNT; count++)
					{
						printf("count = %d\r\n", count);

						if (hm_emit_auto_connb_at_cmd_start() == HX_ERROR_BLOCK)
						{
							PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT || ev == PROCESS_EVENT_CONTINUE);
						    if (ev == PROCESS_EVENT_EXIT)
							{
						        printf("exit\r\n");
								break;
						    } 
							else if (ev == PROCESS_EVENT_CONTINUE)
							{
						    	block_type_t blk = *(int *)data;
								
								if (blk == BLK_TCP_CONNECTB)
								{
									hx_set_conn_success(HX_TCP_CLIENT_SOCKETB);
									break;
								}
								else if (blk == BLK_TCP_CLOSE)
								{
									etimer_set(&timeout, 1 * CLOCK_SECOND);
									PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
								}
						    }
						}
					}
				}
#endif
			}
			
		}
#ifdef __CUSTOMER_HOMI_IOT__			
			else if (hx_sokb_info.sok_port > 0)
			{
				//process_post(&tcp_server_wifiUartProcess, HX_PROCESS_EVENT_AUTO_CONNB, NULL);
				int count;
				
				for (count = 0; count < MAX_RECONN_COUNT; count++)
				{
					printf("count = %d\r\n", count);

					if (hm_emit_auto_connb_at_cmd_start() == HX_ERROR_BLOCK)
					{
						PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT || ev == PROCESS_EVENT_CONTINUE);
					    if (ev == PROCESS_EVENT_EXIT)
						{
					        printf("exit\r\n");
							break;
					    } 
						else if (ev == PROCESS_EVENT_CONTINUE)
						{
					    	block_type_t blk = *(int *)data;
							
							if (blk == BLK_TCP_CONNECTB)
							{
								hx_set_conn_success(HX_TCP_CLIENT_SOCKETB);
								break;
							}
							else if (blk == BLK_TCP_CLOSE)
							{
								etimer_set(&timeout, 1 * CLOCK_SECOND);
								PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
							}
					    }
					}
				}
			}
#endif
		}
		else
		{
			hx_start_auto_conn_timer();
		}
	}
	else
	{
		hx_start_auto_conn_timer();
	}

	return TRUE;
}
#endif

Boolean hx_auto_conn_startup_flag = TRUE;

//----------------------------------------
PROCESS_THREAD(hx_timer_checker, ev, data)
{
    PROCESS_BEGIN();
	
	static int count = 0;
//	static int conn_sockb;
	static Boolean smtlk_mode = FALSE;
	static ieee80211_state eStatus;
			
	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

		if (hx_timer_event == HX_REBOOT_EVENT)
		{
			At_Reboot(NULL);
		}
		else if (hx_timer_event == HX_RECONN_SOCK_EVENT)
		{
			if (hx_sok_info.sok_port > 0)
			{
				process_post(&tcp_server_wifiUartProcess, HX_PROCESS_EVENT_AUTO_CONN, NULL);
			}
		}
#ifdef __CUSTOMER_HOMI_IOT__		
		else if (hx_timer_event == HX_RECONN_SOCKB_EVENT)
		{
			if (hx_sokb_info.sok_port > 0)
			{
				process_post(&tcp_server_wifiUartProcess, HX_PROCESS_EVENT_AUTO_CONNB, NULL);
			}
		}
#endif
		else if (hx_timer_event == HX_START_EVENT)
		{
			//hx_check_auto_conn();

#ifdef __CUSTOMER_HOMI_IOT__	
			if (smtlk_mode == FALSE) 
			{
				hm_init_smtlnk_flag(&smtlk_mode);
			}
#endif
			for (;;)
			{
				eStatus = get_wifi_connect_status();
				printf("eStatus = %d, smtlk_mode = %d\r\n", eStatus, smtlk_mode);
				if (eStatus == IEEE80211_CONNECTED)
				{
					count++;
				}

				if (count == HX_SOCK_CONN_TIME)
				{
					break;
				}
				
				etimer_set(&sTm, CLOCK_SECOND * 2);
				PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
			}
			
#ifdef __CUSTOMER_HOMI_IOT__
			if (smtlk_mode == TRUE)
			{
				Boolean dirty = FALSE;
				Boolean flag = FALSE;

				if (strcmp((char *)(gwifistatus.connAP.key), hx_get_current_key()) != 0)
				{
					hx_set_current_key((char *)(gwifistatus.connAP.key));
					dirty = TRUE;
				}

				if (strcmp((char *)(gwifistatus.connAP.ssid), hx_get_current_ssid()) != 0)
				{
					hx_set_current_ssid((char *)(gwifistatus.connAP.ssid));
					dirty = TRUE;
				}

				hm_update_smtlnk_flag(&flag);
				hx_write_conf();
				
				smtlk_mode = FALSE;
			}
#endif
			printf("auto conn start\r\n");
			hx_auto_conn_startup_flag = TRUE;
			if (hx_sok_info.sok_port > 0)
			{
				for (count = 0; count < HX_MAX_RECONN_COUNT; count++)
				{
					printf("count = %d\r\n", count);
					if (hx_emit_skct_start_at_cmd() == HX_ERROR_BLOCK)
					{
						PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT || ev == PROCESS_EVENT_CONTINUE);
					
					    if (ev == PROCESS_EVENT_EXIT)
						{
					        printf("exit\r\n");
							break;
					    } 
						else if (ev == PROCESS_EVENT_CONTINUE)
						{
					    	block_type_t blk = *(int *)data;
							
							if (blk == BLK_TCP_CONNECT)
							{
								hx_set_conn_success(HX_TCP_CLIENT_SOCKET);
								break;
							}
							else if (blk == BLK_TCP_CLOSE)
							{
								etimer_set(&sTm, 1 * CLOCK_SECOND);
								PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
							}
					    }
					}
				}
			}
#ifdef __CUSTOMER_HOMI_IOT__			
			if (hx_sokb_info.sok_port > 0)
			{
				//process_post(&tcp_server_wifiUartProcess, HX_PROCESS_EVENT_AUTO_CONNB, NULL);		
				for (count = 0; count < HX_MAX_RECONN_COUNT; count++)
				{
					printf("count = %d\r\n", count);

					if (hm_emit_auto_connb_at_cmd_start() == HX_ERROR_BLOCK)
					{
						PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT || ev == PROCESS_EVENT_CONTINUE);
					    if (ev == PROCESS_EVENT_EXIT)
						{
					        printf("exit\r\n");
							break;
					    } 
						else if (ev == PROCESS_EVENT_CONTINUE)
						{
					    	block_type_t blk = *(int *)data;
							
							if (blk == BLK_TCP_CONNECTB)
							{
								hx_set_conn_success(HX_TCP_CLIENT_SOCKETB);
								break;
							}
							else if (blk == BLK_TCP_CLOSE)
							{
								etimer_set(&sTm, 1 * CLOCK_SECOND);
								PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
							}
					    }
					}
				}
			}
#endif
			hx_auto_conn_startup_flag = FALSE;
			printf("auto conn end\r\n");
		}
	}

    PROCESS_END();
}

#endif

#else



UINT win32_uart_recv_thread(LPVOID pParam)
{
	int status;
	int len;
	fm_packet_t *pack;
	DWORD dwEvtMask = 0;
	DWORD dwErrorFlags;
	COMSTAT ComStat;
	BOOL bResult;
	unsigned char temp_buf[BUF_SIZE] = {0};

	init_buffer(&uart_rx_buff);

	SetCommMask(UARTHandle[HX_WIN32_UART_PORT], EV_RXCHAR | EV_TXEMPTY );//有哪些串口事件需要监视？

#if 0
	ZeroMemory(&com_ov, sizeof(com_ov));
	com_ov.hEvent = CreateEvent(
		NULL,   // default security attributes 
		TRUE,   // manual-reset event 
		FALSE,  // not signaled 
		NULL    // no name
		);

	assert(com_ov.hEvent);
#endif	

	for (;;)
	{
		bResult = WaitCommEvent(UARTHandle[HX_WIN32_UART_PORT], &dwEvtMask, &com_ov);// 等待串口通信事件的发生

		if (!bResult)
		{    
			// If WaitCommEvent() returns FALSE, process the last error to determin    
			// the reason..    
			switch (dwErrorFlags = GetLastError())    
			{    
				case ERROR_IO_PENDING:     
					{    
						// This is a normal return value if there are no bytes    
						// to read at the port.
						// Do nothing and continue 
						WaitForSingleObject(com_ov.hEvent, INFINITE);

						break;   
					}   
				case 87:   
					{   
						// Under Windows NT, this value is returned for some reason.    
						// I have not investigated why, but it is also a valid reply    
						// Also do nothing and continue.    
						break;   
					}   
				default:   
					{   
						// All other error codes indicate a serious error has    
						// occured.  Process this error.    
						break;   
					}   
			}   
		}   
		
		ResetEvent(com_ov.hEvent);

		if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR)
		{ // 缓冲区中有数据到达
			COMSTAT ComStat ;
			DWORD dwLength;
			DWORD dwError = 0;
			int error;
			fm_packet_t *pack;

			ClearCommError(UARTHandle[HX_WIN32_UART_PORT], &dwErrorFlags, &ComStat);

			dwLength = ComStat.cbInQue ; //输入缓冲区有多少数据？
			if (dwLength > 0)
			{
				BOOL fReadStat ;
				DWORD dwBytesRead;

				fReadStat = ReadFile(UARTHandle[HX_WIN32_UART_PORT], &uart_rx_buff.pbuff[uart_rx_buff.len], BUF_SIZE - 1 - uart_rx_buff.len, &dwBytesRead, &com_ov);

				if (!fReadStat)
				{
					DWORD dwError = GetLastError();

					if (dwError == ERROR_IO_PENDING)
					{
						while (!GetOverlappedResult(UARTHandle[HX_WIN32_UART_PORT], &com_ov, &dwBytesRead, TRUE))
						{
							dwError = GetLastError();
							if (dwError == ERROR_IO_INCOMPLETE)
							{
								continue;
							}
						}
					}
				}

				if (dwBytesRead == 0)		// 异常情况处理
				{
					Log_e("dwLength=%d, fReadStat=%d, uart read error=%d\n", dwLength, fReadStat, dwError);
					continue;
				}

				uart_rx_buff.len += dwBytesRead;
				Log_d("==========\r\nuart read len = %d\n", uart_rx_buff.len);
				uart_rx_buff.pbuff[uart_rx_buff.len] = '\0';
				//print_buffer(&uart_rx_buff);
				printf("==========\r\n");

#ifdef __HEKR_CLOUD_SUPPORT__ 
				pack = get_packet(&uart_rx_buff, &error);
				pack = build_echo_packet(pack);
				if (pack)
				{
					win32_UART_PutBytes(HX_WIN32_UART_PORT, (unsigned char *)pack, pack->len);
				}
				else
				{
					printf("not packet!\r\n");
				}
#elif defined(__OBD_TEST__)
				obd_log_uart_rx_data(&uart_rx_buff);
#elif defined(__HOMI_TEST__) || defined(__HXWL_TEST__)
				homi_do_with_uart_rsp(&uart_rx_buff);
#elif defined(__TUYA_UART_TEST__)
				tuya_parse_uart_data_bufffer(&uart_rx_buff);
#endif
				if (fn_uart_rsp_func)
				{
					fn_uart_rsp_func(&uart_rx_buff);
				}

				init_buffer(&uart_rx_buff);
			}
		}
		else if ((dwEvtMask & EV_TXEMPTY) == EV_TXEMPTY) 
		{
			printf("uart data sent!\n");
			SetEvent(hWriteEvent);
		}
	}
	
#if 0
	for (;;)
	{
		len = win32_UART_GetBytes(HX_WIN32_UART_PORT, &uart_rx_buff.pbuff[uart_rx_buff.len], BUF_SIZE - uart_rx_buff.len, &status);
		if (len > 0)
		{
			uart_rx_buff.len += len;

			printf("uart read len = %d\n", len);

#if defined(__HX_IOT_SUPPORT__)					
			hx_do_with_uart_frame();

#elif defined(__HEKR_CLOUD_SUPPORT__)
			add_rx_buffer();
			pack = parse_data_buff();
			if (pack)
			{
				hekr_add_task_rsp_from_uart(pack);
			}
#else
			//{
			//	add_rx_buffer();
			//	net_send_tx_queue(app_res.tcpServerSock, FALSE, wifi2uart_tcp_send_cb);
			//}
#endif
		}
	}
#endif

	return 0;
}

/************************************************************************/
/* 
功能：接收串口数据线程
pParam:传入指定串口号，表明接收此串口的数据
返回值：成功：KAL_TRUE，失败:KAL_FALSE
*/
/************************************************************************/
UINT win32_uart_recv_thread_ex(LPVOID pParam)
{
	int status;
	int len;
	fm_packet_t *pack;
	DWORD dwEvtMask = 0;
	DWORD dwErrorFlags;
	COMSTAT ComStat;
	BOOL bResult;
	unsigned char temp_buf[BUF_SIZE] = {0};
	int port = *(int *)pParam;

	init_buffer(&g_uart_rx_buff[port]);

	SetCommMask(UARTHandle[port], EV_RXCHAR | EV_TXEMPTY );//有哪些串口事件需要监视？

	for (;;)
	{
		bResult = WaitCommEvent(UARTHandle[port], &dwEvtMask, &com_ov_ex[port]);// 等待串口通信事件的发生

		if (!bResult)
		{    
			// If WaitCommEvent() returns FALSE, process the last error to determin    
			// the reason..    
			switch (dwErrorFlags = GetLastError())    
			{    
			case ERROR_IO_PENDING:     
				{    
					// This is a normal return value if there are no bytes    
					// to read at the port.
					// Do nothing and continue 
					WaitForSingleObject(com_ov_ex[port].hEvent, INFINITE);

					break;   
				}   
			case 87:   
				{   
					// Under Windows NT, this value is returned for some reason.    
					// I have not investigated why, but it is also a valid reply    
					// Also do nothing and continue.    
					break;   
				}   
			default:   
				{   
					// All other error codes indicate a serious error has    
					// occured.  Process this error.    
					break;   
				}   
			}   
		}   

		ResetEvent(com_ov_ex[port].hEvent);

		if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR)
		{ // 缓冲区中有数据到达
			COMSTAT ComStat ;
			DWORD dwLength;
			DWORD dwError;
			int error;
			fm_packet_t *pack;
			buf_t *ptr = &g_uart_rx_buff[port];

			ClearCommError(UARTHandle[port], &dwErrorFlags, &ComStat);

			dwLength = ComStat.cbInQue ; //输入缓冲区有多少数据？
			if (dwLength > 0)
			{
				BOOL fReadStat ;
				DWORD dwBytesRead;

				fReadStat = ReadFile(UARTHandle[port], temp_buf, sizeof(temp_buf), &dwBytesRead, &com_ov_ex[port]);
				if (!fReadStat)
				{
					DWORD dwError = GetLastError();

					if (dwError == ERROR_IO_PENDING)
					{
						while (!GetOverlappedResult(UARTHandle[port], &com_ov_ex[port], &dwBytesRead, TRUE))
						{
							dwError = GetLastError();
							if (dwError == ERROR_IO_INCOMPLETE)
							{
								continue;
							}
						}
					}
				}

				if (dwBytesRead < (BUF_SIZE - ptr->len))
				{
					memcpy(ptr->pbuff + ptr->len, temp_buf, dwBytesRead);
					ptr->len += dwBytesRead;
				}

				printf("==========\r\nuart read len = %d\n", ptr->len);
				ptr->pbuff[ptr->len] = '\0';
				print_buffer(ptr);
				printf("==========\r\n");

				obd_log_uart_rx_data_ex(ptr, port);

				if (fn_uart_rsp_ex_func)
				{
					fn_uart_rsp_ex_func(ptr, port);
				}
			}
		}
		else if ((dwEvtMask & EV_TXEMPTY) == EV_TXEMPTY) 
		{
			printf("uart data sent!\n");
			SetEvent(hWriteEvent_ex[port]);
		}
	}

	return 0;
}


kal_uint16 win32_UART_GetBytes(int port, kal_uint8 *Buffaddr, kal_uint16 Length, kal_uint8 *status)
{
	int real_read = 0;
	int i = 0;
	
	if (status) 
	{
		*status = 0;
	}
	
	if (!ReadFile(UARTHandle[port],
			(LPVOID)Buffaddr,
			(DWORD)Length,
			(LPDWORD)&real_read,
			NULL))
	{
		printf("read file error %d\n", GetLastError());
		return 0;
	}

    if (real_read < Length)
    {
        breadytoread[port] = TRUE;
    }
    else
    {
        breadytoread[port] = FALSE;
    }
	
	return real_read;
}

void win32_UART_Close(int port)
{
    CloseHandle(UARTHandle[port]);
}

/*关闭指定串口*/
void win32_UART_Close_Ex(int port)
{
    //CloseHandle(UARTHandle[port]);
	m_bUartThreadExit = TRUE;
	//win32_stop_thread();
	if (UARTHandle[port])
	{
		CloseHandle(UARTHandle[port]);
	}
	//m_port = 0;
}

void win32_CheckReadyToRead(int port)
{
	COMSTAT	comstat;
	DWORD	errors;
	
	if (ClearCommError(UARTHandle[port], &errors, &comstat))
	{
		if (comstat.cbInQue)        
		{
			breadytoread[port] = FALSE;
		}
	}        
}

void win32_CheckReadyToWrite(int port)
{
	COMSTAT	comstat;
	DWORD	errors;
	
	if (ClearCommError(UARTHandle[port], &errors, &comstat))
	{
		if(comstat.fCtsHold == 0 && comstat.fXoffHold == 0 && comstat.fXoffSent == 0)
		{            
			breadytowrite[port] = FALSE;
		}
	}            
}

void win32_QueryUARTStatus()
{
	int i = 0;
	//ready to read    
	for (i = 0; i < MAX_PORT_NUM; i++)
	{
		if (breadytoread[i])
		{
			win32_CheckReadyToRead(i);
		}

		if (breadytowrite[i])
		{
			win32_CheckReadyToWrite(i);
		}
	}    
}

int w32uart_available(int port)
{
	DWORD dummy;
	
	if (!ClearCommError(UARTHandle[port], &dummy, &comstat[port]))
	{
		return 0;
	}
	
	return comstat[port].cbInQue;
}

kal_uint16 win32_UART_PutBytes(int port, kal_uint8 *Buffaddr, kal_uint16 Length)
{
    ///
#if 0
    int wbytes;
	BOOL fWriteStat;
	DWORD dwError;
	DWORD dwBytesWritten;
    DWORD dwRet ;
#endif
	OVERLAPPED osWrite = {0};
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

   // Create this write operation's OVERLAPPED structure's hEvent.
   osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (osWrite.hEvent == NULL)
      // error creating overlapped event handle
      return FALSE;

    // Issue write.
   if (!WriteFile(UARTHandle[port], Buffaddr, Length, &dwWritten, &osWrite)) 
   {
      if (GetLastError() != ERROR_IO_PENDING) 
	  { 
         // WriteFile failed, but isn't delayed. Report error and abort.
         fRes = FALSE;
      }
      else
      {
         // Write is pending.
         dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
         switch(dwRes)
         {
            // OVERLAPPED structure's event has been signaled. 
            case WAIT_OBJECT_0:
                 if (!GetOverlappedResult(UARTHandle[port], &osWrite, &dwWritten, FALSE))
                 {
                       fRes = FALSE;
                 }
                 else
                  // Write operation completed successfully.
                  fRes = TRUE;
                 break;
            
            default:
                 // An error has occurred in WaitForSingleObject.
                 // This usually indicates a problem with the
                // OVERLAPPED structure's event handle.
                 fRes = FALSE;
                 break;
         }
      }
   }
   else
   {
      // WriteFile completed immediately.
      fRes = TRUE;
   	}

   printf("tolen = %d, write len = %d\r\n", Length, dwWritten);

   CloseHandle(osWrite.hEvent);
   return fRes;
   
#if 0
    fWriteStat = WriteFile(UARTHandle[port], Buffaddr, Length, &wbytes, &com_ov);
	if (!fWriteStat) 
	{
		dwError = GetLastError();
		printf("dwError = %d\r\n", dwError);
		if (dwError == ERROR_IO_PENDING)
		{
			while(!GetOverlappedResult(UARTHandle[port], &com_ov, &dwBytesWritten, TRUE )) 
			{
				dwError = GetLastError();

				if (dwError == ERROR_IO_INCOMPLETE)
				{
					wbytes += dwBytesWritten;
					continue; 
				}
			}
		}
	}

	dwRet = WaitForSingleObject(hWriteEvent, INFINITE);
	if (dwRet == WAIT_ABANDONED)
	{
		printf("event abandon!\r\n");
	}
	else if (dwRet == WAIT_OBJECT_0)
	{
		printf("get signal\r\n");
	}
	else
	{

	}

	ResetEvent(hWriteEvent);

    if (wbytes < Length)
    {
        breadytowrite[port] = TRUE;
    }
    else
    {
        breadytowrite[port] = FALSE;
    }

	PurgeComm(UARTHandle[port], PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);	   
        
    return wbytes;
#endif	
}

kal_bool win32_UART_Open(int port)
{
	DCB dcb;
	HANDLE hCom;
	char strbuf[256], destbuf[256];
	COMSTAT	comstat;
	DWORD	errors;
	COMMTIMEOUTS timeouts;
	int baudrate = CBR_38400;
	int flowCtrl = 0;  //0:none, 1:hw , 2:sw flow control
	char path[1024];
	char tempbuf[1024];
	char *name;	
	int len;

#if 1
	sprintf(strbuf, "UART%d", port);
	
	if (!get_config_string_value(strbuf, _TEXT("COM_PORT"), _TEXT("COM1"), tempbuf, sizeof(tempbuf)))
	{
		_tcscpy(tempbuf, "COM1");
	}

	sprintf(destbuf, ("\\\\.\\%s"), tempbuf);

	if (!get_config_int_value(strbuf, _TEXT("flowcontrol"), &flowCtrl, 0))
	{
		port = 0;
	}

	if (!get_config_int_value(strbuf, _TEXT("baud_rate"), &baudrate, 115200))
	{
		baudrate = 115200;
	}

#else

	if (!SearchPath(NULL, "stubmfc.exe", NULL, sizeof(path), path, &name))
	{
		return 0;
	}
    
	memset(tempbuf, 0, sizeof(tempbuf));
	strncpy(tempbuf, path, strlen(path) - strlen(name));
	sprintf(path, "%suart.ini", tempbuf);

	sprintf(strbuf, "UART%d", port);
	len = sprintf(destbuf, ("\\\\.\\"));
	
	GetPrivateProfileString(strbuf, "COM_PORT", "COM1", &destbuf[len], sizeof(destbuf) - len, path);
	flowCtrl = GetPrivateProfileInt(strbuf, "flowcontrol", 0, path);
	baudrate = GetPrivateProfileInt(strbuf, "baud_rate", 115200 , path);
#endif
	
	hCom = CreateFile(destbuf,
                    GENERIC_READ |  GENERIC_WRITE ,
                    0,    // must be opened with exclusive-access
                    NULL, // no security attributes
                    OPEN_EXISTING, // must use OPEN_EXISTING
                    FILE_FLAG_OVERLAPPED,    // not overlapped I/O
                    NULL  // hTemplate must be NULL for comm devices
                    );

	if (hCom == INVALID_HANDLE_VALUE)// create fail
	{
		return KAL_FALSE;
	}
	
	//Turn off fAbortOnError first!	
	ClearCommError(hCom, &errors, &comstat);

#if 0
    if (!GetCommState(hCom, &dcb))
	{
		CloseHandle(hCom);
		hCom = NULL;
		return KAL_FALSE;
	}
	
	dcb.DCBlength = sizeof(DCB);
    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.BaudRate = 921600;// baudrate;		///check!
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

	if (flowCtrl == 1)  //hardware flow control
	{
		dcb.fOutxCtsFlow = TRUE;
	}
	else
	{
		dcb.fOutxCtsFlow = FALSE;
	}

    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
	dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
	dcb.fAbortOnError = FALSE;

	if (flowCtrl == 2)  //software flow control
	{
		dcb.fOutX = TRUE;
		dcb.fInX = TRUE;
		dcb.XonChar = 0x11;
		dcb.XoffChar = 0x13;
	}
	else
	{
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.XonChar = 0;
		dcb.XoffChar = 0;
	}   
	
	if (flowCtrl == 2 || flowCtrl == 0) //sw or none flow control
	{
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	}
	else if (flowCtrl == 1)   //hw flow control
	{
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	}

    dcb.ErrorChar = 0;
    dcb.EofChar = 0;
    dcb.EvtChar = 0;	
#else
	//DCB dcb;

	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);
	sprintf(tempbuf, "%d,N,8,1", baudrate);
	if (!BuildCommDCB(tempbuf, &dcb)) 
	{   
	  // Couldn't build the DCB. Usually a problem
	  // with the communications specification string.
	  printf("build dcb failed!\r\n");
	  return FALSE;
	}

#endif

	if (!SetCommState(hCom, &dcb))
    {
        CloseHandle(hCom);		
		return KAL_FALSE;
    }	

	if (SetupComm(hCom, 8192, 8192)==FALSE )
	{
		CloseHandle(hCom);		
		return KAL_FALSE;
	}

	PurgeComm(hCom, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);	   
				
#if 1
	timeouts.ReadIntervalTimeout = 100;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.ReadTotalTimeoutMultiplier = 400; 

	timeouts.WriteTotalTimeoutMultiplier = 3;
	timeouts.WriteTotalTimeoutConstant = 2;
	
	if (SetCommTimeouts(hCom, &timeouts) == FALSE ) 
	{
		CloseHandle(hCom);		
		return KAL_FALSE;
	}
#endif

	
	UARTHandle[port]  = hCom;
	breadytoread[port] = TRUE;

	ZeroMemory(&com_ov, sizeof(com_ov));
	com_ov.hEvent = CreateEvent(
		NULL,   // default security attributes 
		TRUE,   // manual-reset event 
		FALSE,  // not signaled 
		NULL    // no name
	);

	assert(com_ov.hEvent);

	hWriteEvent = CreateEvent(
		NULL,   // default security attributes 
		TRUE,   // manual-reset event 
		FALSE,  // not signaled 
		NULL    // no name
	);
	assert(hWriteEvent);
	
	return KAL_TRUE;	
}


/* 
 * 功能：打开指定串口
 * port:串口号
 * 返回值：成功：KAL_TRUE，失败:KAL_FALSE
 * 波特率在本段中确定
 * [UART2]
 *		COM_PORT = COM3
 *		flowcontrol = 0
 *		baud_rate = 115200
 */
kal_bool win32_UART_Open_ex(int port)
{
	DCB dcb;
	HANDLE hCom;
	char strbuf[256], destbuf[256];
	COMSTAT	comstat;
	DWORD	errors;
	COMMTIMEOUTS timeouts;
	int baudrate = CBR_38400;
	int flowCtrl = 0;  //0:none, 1:hw , 2:sw flow control
	char path[1024];
	char tempbuf[1024];
	char *name;	
	int len;

	memset(destbuf, 0, sizeof(destbuf));
	sprintf(destbuf, "\\\\.\\COM%d", port);
	flowCtrl = 0;
	baudrate = CBR_9600;

	sprintf(strbuf, "UART%d", 2);
	if (!get_config_string_value(strbuf, _TEXT("COM_PORT"), _TEXT("COM1"), tempbuf, sizeof(tempbuf)))
	{
		_tcscpy(tempbuf, "COM1");
	}

	if (!get_config_int_value(strbuf, _TEXT("baud_rate"), &baudrate, CBR_9600))
	{
		baudrate = CBR_9600;
	}

	hCom = CreateFile(destbuf,
		GENERIC_READ /*| GENERIC_WRITE*/,
		0,    // must be opened with exclusive-access
		NULL, // no security attributes
		OPEN_EXISTING, // must use OPEN_EXISTING
		FILE_FLAG_OVERLAPPED,    // not overlapped I/O
		NULL  // hTemplate must be NULL for comm devices
		);

	if (hCom == INVALID_HANDLE_VALUE)// create fail
	{
		return KAL_FALSE;
	}

	//Turn off fAbortOnError first!	
	ClearCommError(hCom, &errors, &comstat);

	//DCB dcb;

	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);
	sprintf(tempbuf, "%d,N,8,1", baudrate);
	if (!BuildCommDCB(tempbuf, &dcb)) 
	{   
		// Couldn't build the DCB. Usually a problem
		// with the communications specification string.
		printf("build dcb failed!\r\n");
		return FALSE;
	}

	if (!SetCommState(hCom, &dcb))
	{
		CloseHandle(hCom);		
		return KAL_FALSE;
	}	

	if (SetupComm(hCom, 8192, 8192)==FALSE )
	{
		CloseHandle(hCom);		
		return KAL_FALSE;
	}

	PurgeComm(hCom, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);	   

#if 1
	timeouts.ReadIntervalTimeout = 100;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.ReadTotalTimeoutMultiplier = 400; 

	timeouts.WriteTotalTimeoutMultiplier = 3;
	timeouts.WriteTotalTimeoutConstant = 2;

	if (SetCommTimeouts(hCom, &timeouts) == FALSE ) 
	{
		CloseHandle(hCom);		
		return KAL_FALSE;
	}
#endif


	UARTHandle[port]  = hCom;
	breadytoread[port] = TRUE;

	ZeroMemory(&com_ov_ex[port], sizeof(com_ov_ex[port]));
	com_ov_ex[port].hEvent = CreateEvent(
		NULL,   // default security attributes 
		TRUE,   // manual-reset event 
		FALSE,  // not signaled 
		NULL    // no name
		);

	assert(com_ov_ex[port].hEvent);

	hWriteEvent_ex[port] = CreateEvent(
		NULL,   // default security attributes 
		TRUE,   // manual-reset event 
		FALSE,  // not signaled 
		NULL    // no name
		);
	assert(hWriteEvent_ex[port]);

	return KAL_TRUE;	
}

/*
 * 清除串口
 */
kal_bool win32_clear_uart_data(int port)
{
	HANDLE h;

	if (port < 0 || port >= MAX_PORT_NUM)
	{
		return FALSE;
	}

	h = UARTHandle[port];
	
	if (!PurgeComm(h, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR))
	{
		disp_win_sys_err_msg(_T("Purge comm error!"));
	}
}


static U8 local_mac[6] = {0X00, 0x12, 0x23, 0x34, 0x45, 0x56};
U8 get_local_mac(U8 *mac, U8 len)
{
	U8 status = 0;
	
    if (len > 6) 
	{
        printf("over local mac len: (%d), please check\n", len);
        status = -1;
    } else if (len < 6) 
   {
        printf("less local mac len: (%d), please check\n", len);
        status = -1;
    } else {
        memcpy(mac, local_mac, 6);
    }
	
    return status;
}

/*
 * 注册串口数据回调函数
 */
void register_uart_rsp_func(uart_rsp_cb_t fn)
{
	fn_uart_rsp_func = fn;
}

/*
 * 注册串口数据回调函数
 */
void register_uart_rsp_ex_func(uart_rsp_ex_cb_t fn)
{
	fn_uart_rsp_ex_func = fn;
}

#if 0
SVersion S_Ver = {
	0x75566572,		//	'uVer'
	__DATE__ " " __TIME__,
	"UA_iCommWifi_Demo",
	0x10,
	0x10,
	D_szRCVer
};
#endif

#endif
