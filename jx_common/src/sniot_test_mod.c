#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
#include "sniot_test_mod.h"
#include "qcloud_iot_export_log.h"

#define UART_RCV_TIMEOUT	(10 * 1000)

#define SNIOT_NULL_TEST		(0)				// 初始ATCMD指令
#define SNIOT_ENTER_TEST	(1)				// 进入TEST MODE指令
#define SNIOT_LEAVE_TEST	(2)				// 离开TEST MODE指令
#define SNIOT_WRITE_ID		(3)				// 写 MAC/ID 指令
#define SNIOT_READ_MAC		(4)				// 读MAC指令
#define SNIOT_READ_ID		(5)				// 读 LICENSE 指令

/*
 * 司南物联测试模式AT指令模块
 */
#define MAX_SEG 5
#define MAX_RESEND_COUNT 3
static Boolean at_rsp_enter_cmd(buf_t *b, int *code);
static Boolean at_rsp_generic(buf_t *b, int *code);
static void homi_line_chomp(buf_t *b);
static Boolean at_rsp_enter(buf_t *b, int *code);

static char myserver[32];
static int myport;

static Boolean timeout_flag = FALSE;
static int tick_count = 0;

HANDLE hEvent = INVALID_HANDLE_VALUE;

#define SNIOT_PARAM_LEN	128

static char s_mac_addr_str[32];
static char s_id_str[64];

static int s_at_cmd;

typedef struct param_struct 
{
	kal_uint32 addr;			// FLASH 对应的地址
	int len;
	char param[SNIOT_PARAM_LEN];		// 写入的参数
} sniot_param_t;


#define SNIOT_PARAM_BEGIN_ADDR	(0x30E4000)			// 所有参数开始flash地址
#define SNIOT_MAC_BEGIN_ADDR	(0x300200C)			// MAC 地址开始地址


#define printf Log_d

const static sniot_param_t param_array[SNIOT_NUM] = 
{
	{
		SNIOT_PARAM_BEGIN_ADDR,
		4,
		""
	},

	{
		0x30E4005,
		16,
		""
	},

	{
		0x30E4016,
		6,
		""
	},

	{
		0x30E401D,
		11,
		""
	},

	{
		0x30E4029,
		11,
		""
	},

	{
		0x30E4035,
		32,
		""
	},

	{
		SNIOT_MAC_BEGIN_ADDR,
		12,
		""
	}
};


/*
 * 进入测试模式
 */
static buf_t * sniot_build_enter_test_mode(buf_t *b)
{
	int len;
	int index;

	index = 0;
	len = sprintf(&b->pbuff[index], "AT#SNENTEST\r");
	b->len = len;

	return b;
}

/*
 * 退出测试模式
 */
static buf_t * sniot_build_leave_test_mode(buf_t *b)
{
	int len;
	int index;

	index = 0;
	len = sprintf(&b->pbuff[index], "AT#SNEXTEST\r");
	b->len = len;

	return b;
}


/*
 * 重启
 */ 
static buf_t * sniot_build_reboot(buf_t *b)
{
	int len;
	int index;

	index = 0;
	len = sprintf(&b->pbuff[index], "AT+REBOOT\r\n");
	b->len = len;

	return b;
}

/*
 * 写所有信息，除了mac
 */ 
static buf_t * sniot_build_write_all_info(buf_t *b, sniot_param_t *p)
{
	int index;
	char tmp[512];
	int i;

	index = 0;
	for (i = SNIOT_MANUFATURE_ID; i <= SNIOT_PRODUCT_ID; i++)
	{
		index += sprintf(&tmp[index], "%s,", p[i].param);
	}

	index += sprintf(&tmp[index], "%s", p[i].param);		// PASSWORD

	index = 0;
	index += sprintf(&b->pbuff[index], "AT#FLASH -s0x%X -W%s\r", param_array[0].addr, tmp);

	b->len = index;

	return b;
}

/*
* 写MAC
*/
static buf_t * sniot_build_write_mac(buf_t *b, sniot_param_t *param)
{
	int index;

	index = 0;
	index += sprintf(&b->pbuff[index], "AT#FLASH -s0x%X -W%s\r", param_array[SNIOT_MAC_ADDR].addr, param->param);
	b->len = index;

	return b;
}

/* 
 * 从一行中填入各参数
 */
static Boolean sniot_init_param(const char *line_buf, sniot_param_t *parray, int n)
{
	int buflen;
	const char *sep = ",";
	char *p;
	char tmp_buf[1024];
	int i;

	buflen = strlen(line_buf);
	if (buflen == 0) 
	{
		return FALSE;
	}

	if (buflen >= sizeof(tmp_buf))
	{
		printf("line is too long!\n");
		return FALSE;
	}

	if (n < SNIOT_NUM)
	{
		n = SNIOT_NUM;
	}

	memcpy(parray, param_array, n * sizeof(sniot_param_t));

	strcpy(tmp_buf, line_buf);

	p = strtok(tmp_buf, sep);
	if (!p)
	{
		return FALSE;
	}

	for (i = 0; p != NULL; p = strtok(NULL, sep), i++)
	{
		if (i < n)
		{
			buflen = strlen(p);
			if (buflen >= SNIOT_PARAM_LEN)
			{
				printf("toke is too long!, buflen=%d", buflen);
				return FALSE;
			}
			if (buflen != param_array[i].len)
			{
				printf("token len(%d) is not equal(%d)\n", buflen, param_array[i].len);
				return FALSE;
			}

			strcpy(parray[i].param, p);
		}
	}

	return TRUE;
}

/*
 * AT指令同步Event Handle
 */
void sniot_init_handle()
{
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		return;
	}

	hEvent = CreateEvent(NULL, FALSE, FALSE, "UART_Event");
	if (hEvent == INVALID_HANDLE_VALUE)
	{
		printf("create event failed!\n");
	}
}


static void homi_line_chomp(buf_t *b)
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


/*
 * 进入test mode的处理逻辑：
 * 1. 先重启
 * 2. 再发送特殊的字串表明重启成功
 */
static void do_enter_test_mode_rsp(buf_t *b)
{
	static int state_machine;

#define STATE_INIT_OK		(0)
#define STATE_GET_RSP_OK	(1)
#define STATE_GET_BOOT_OK	(2)

	char *p;

	switch (state_machine)
	{
		case STATE_INIT_OK:
		{
			p = strstr(b->pbuff, "AT#SNENTEST=OK");
			if (!p)
			{
				return;
			}

			state_machine = STATE_GET_RSP_OK;
			printf("get entest rsp!\n");

			break;
		}

		case STATE_GET_RSP_OK:
		{
			p = strstr(b->pbuff, "boot_main");			// 启动成功标志
			if (!p)
			{
				return;
			}

			state_machine = STATE_GET_BOOT_OK;
			printf("get boot flag!\n");
			
			break;
		}

		case STATE_GET_BOOT_OK:
		{
			p = strstr(b->pbuff, "[sn_PulseInit]");			// 启动成功标志
			if (!p)
			{
				return;
			}
			printf("get init flag!\n");
			SetEvent(hEvent);
			state_machine = STATE_INIT_OK;
			break;
		}

		default:
			break;
	}
}

/*
 * 串口回应处理
 */
void sniot_do_with_uart_rsp(buf_t *b)
{
	char *p;

//	print_buffer(b);

	b->pbuff[b->len] = '\0';

	//Log_d("recv str:%s", b->pbuff);

	if (s_at_cmd == SNIOT_ENTER_TEST)
	{
		do_enter_test_mode_rsp(b);
	}
	else if (s_at_cmd == SNIOT_LEAVE_TEST)
	{
		p = strstr(b->pbuff, "AT#SNEXTEST=OK");
		if (!p)
		{
			return;
		}

		printf("get exit test mode rsp!\n");
		SetEvent(hEvent);
	}
	else if (s_at_cmd == SNIOT_WRITE_ID)
	{
		p = strstr(b->pbuff, "AT#FLASH=OK");
		if (!p)
		{
			return;
		}

		printf("get write id/mac mode rsp!\n");
		SetEvent(hEvent);
	}
	else if (s_at_cmd == SNIOT_READ_MAC || s_at_cmd == SNIOT_READ_ID)
	{
		char *ptr;
		int n;

		p = strstr(b->pbuff, "AT#FLASH=OK");
		if (!p)
		{
			return;
		}

		p = strrstr(b->pbuff, "ssv6060>:");
		if (!p)
		{
			printf("not found ssv6060 flag");
			return;
		}

		p += strlen("ssv6060>:");

		ptr = strrstr(b->pbuff, "\n\r\n\r");
		if (!ptr)
		{
			printf("no found end string!\n");
			return;
		}
		
		n = ptr - p;

		if (s_at_cmd == SNIOT_READ_MAC)
		{
			if (n != 12)
			{
				printf("mac string length(%d) error!", n);
				return;
			}

			strncpy(s_mac_addr_str, p, 12);
			printf("get mac str(%s)!\n", s_mac_addr_str);
		}
		else
		{
			if (n != 32)
			{
				printf("mac string length(%d) error!", n);
				return;
			}

			strncpy(s_id_str, p, 32);
			printf("get id str(%s)!\n", s_id_str);
		}
		
		SetEvent(hEvent);
	}

}

/*
 * 发送进入测试模式指令
 */
Boolean sniot_send_enter_test_atcmd()
{
	buf_t *b;
	int i;
	DWORD ret;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	s_at_cmd = SNIOT_ENTER_TEST;

	for (i = 0; i < 1; i++)			// 进入测试模式需要重启，仅仅发送一次
	{
		sniot_build_enter_test_mode(b);
		uart_send_buf(b, NULL);

		ret = WaitForSingleObject(hEvent, UART_RCV_TIMEOUT);

		if (ret == WAIT_ABANDONED)
		{
			printf("wait error!\n");
			break;
		} 
		else if (ret == WAIT_TIMEOUT)
		{
			printf("wait rsp timeout!\n");
			continue;
		}
		else if (ret == WAIT_OBJECT_0)
		{
			printf("enter TEST mode ok!\n");
			break;
		}
		else
		{
			printf("wait error!\n");
		}
	}

	free_buffer(b);

	s_at_cmd = SNIOT_NULL_TEST;

	return TRUE;
}

/*
 * 发送具体的AT指令
 */
static Boolean sniot_send_atcmd(buf_t *b, int cmd)
{
	int i;
	DWORD ret;
	Boolean ok = FALSE;

	s_at_cmd = cmd;

	for (i = 0; i < 3; i++)
	{
		uart_send_buf(b, NULL);

		ret = WaitForSingleObject(hEvent, UART_RCV_TIMEOUT);

		if (ret == WAIT_ABANDONED)
		{
			printf("wait cmd(%s) error!\n", b->pbuff);
			break;
		} 
		else if (ret == WAIT_TIMEOUT)
		{
			printf("wait cmd(%s) rsp timeout!\n", b->pbuff);
			continue;
		}
		else if (ret == WAIT_OBJECT_0)
		{
			printf("cmd (%s) ok!\n", b->pbuff);
			ok = TRUE;
			break;
		}
		else
		{
			printf("wait error!\n");
		}
	}

	s_at_cmd = SNIOT_NULL_TEST;

	return ok;
}


/*
* 发送离开测试模式指令
*/
Boolean sniot_send_leave_test_atcmd()
{
	buf_t *b;
	Boolean ok = FALSE;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	sniot_build_leave_test_mode(b);
	ok = sniot_send_atcmd(b, SNIOT_LEAVE_TEST);
	free_buffer(b);

	return ok;
}


/*
* 发送写所有信息指令
*/
Boolean sniot_send_write_info_atcmd(sniot_lic_line_t *line)
{
	buf_t *b;
	sniot_param_t array[10];
	Boolean ret;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	sniot_init_param(line->line_buf, &array[0], 10);
	sniot_build_write_all_info(b, array);
	ret = sniot_send_atcmd(b, SNIOT_WRITE_ID);
	free_buffer(b);
	return ret;
}

/*
* 发送写MAC地址指令
*/
Boolean sniot_send_write_mac_atcmd(sniot_lic_line_t *line)
{
	buf_t *b;
	sniot_param_t array[10];
	Boolean ret;
	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	sniot_init_param(line->line_buf, &array[0], 10);
	sniot_build_write_mac(b, &array[SNIOT_MAC_ADDR]);
	ret = sniot_send_atcmd(b, SNIOT_WRITE_ID);
	free_buffer(b);

	return ret;
}


/*
* 发送读MAC地址指令
*/
Boolean sniot_send_read_mac_atcmd(sniot_lic_line_t *line, char mac_str[32])
{
	buf_t *b;
	sniot_param_t array[10];
	Boolean ret;
	int index;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	sniot_init_param(line->line_buf, &array[0], 10);
	b->len = sprintf(b->pbuff, "AT#FLASH -s0x%X -R6\r", param_array[SNIOT_MAC_ADDR].addr);
	ret = sniot_send_atcmd(b, SNIOT_READ_MAC);
	free_buffer(b);

	if (ret)
	{
		strcpy(mac_str, s_mac_addr_str);
	}

	return ret;
}


/*
* 发送读SN地址指令
*/
Boolean sniot_send_read_id_atcmd(sniot_lic_line_t *line, char id_str[64])
{
	buf_t *b;
	sniot_param_t array[10];
	Boolean ret;
	int index;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	sniot_init_param(line->line_buf, &array[0], 10);
	b->len = sprintf(b->pbuff, "AT#FLASH -s0x%X -R%d\r", param_array[SNIOT_PASSWORD].addr, param_array[SNIOT_PASSWORD].len);
	ret = sniot_send_atcmd(b, SNIOT_READ_ID);
	free_buffer(b);

	if (ret)
	{
		strcpy(id_str, s_id_str);
	}

	return ret;
}



/*
 * test stub
 */
void test_sniot()
{
	const char *line = "1000,LCO0000000000186,V1.7.6,SNIOT601AGP,1000003C56F,WrKYFb2XJ8UGC10LYU6MPEWP94UyL552,2435CC08A117";
	sniot_param_t array[10];
	buf_t *b;

	sniot_init_param(line, &array[0], 10);

	b = alloc_buffer();
	if (!b)
	{
		printf("no buffer!\n");
		return;
	}

	sniot_build_write_all_info(b, array);
	sniot_build_write_mac(b, &array[SNIOT_MAC_ADDR]);
}


/*
 * 写一个license key
 *  流程：
 * 1. 先退出测试模式
 * 2. 进入测试模式，系统自动重启
 * 3. 写入MAC
 * 4. 写入ID/PASSWORD
 * 5. 检查MAC，是否相等
 * 6. 退出测试模式
 */
int sniot_write_license_id(const char *lic_path, const char *mac_str, fn_send_msg fn, void *param)
{
	static sniot_lic_line_t *plines;
	sniot_lic_line_t *pline;
	TCHAR path[MAX_PATH];
	TCHAR lpszFilePath[MAX_PATH];
	int ret = FALSE;
	char tmp_mac_str[32];

	IOT_Log_Set_MessageHandler(MyLogMessageHandler);
	IOT_Log_Set_Level(QC_DEBUG);

	sniot_init_handle();
	get_program_path(path, sizeof(path));

	sprintf(lpszFilePath, "%s/a.txt", path);

	if (!plines)
	{
		plines = SniotReadLicenseFile(lic_path);
		if (fn)
		{	
			fn(param, "正在读入License数据\n");
		}
	}

	if (!plines)
	{
		return SNIOT_ERROR_NOT_LIC_FILE;
	}

	if (fn)
	{	
		fn(param, "正在查找License记录\n");
	}

	if (!(pline = sniot_find_lic_line(plines, mac_str)))
	{
		return SNIOT_ERROR_NOT_FOUND_MAC;
	}

	if (pline->used)
	{
		return SNIOT_ERROR_MAC_USED;
	}

	if (fn)
	{	
		fn(param, "正在进入测试模式\n");
	}
	sniot_send_leave_test_atcmd();

	if (!sniot_send_enter_test_atcmd())
	{
		return SNIOT_ERROR_ENTER_TST_MODE;
	}

	if (fn)
	{	
		fn(param, "正在写MAC地址\n");
	}
	if (!sniot_send_write_mac_atcmd(pline))
	{
		return SNIOT_ERROR_WRITE_MAC;
	}

	if (fn)
	{	
		fn(param, "正在写License数据\n");
	}

	if (!sniot_send_write_info_atcmd(pline))
	{
		return SNIOT_ERROR_WRITE_ID;
	}

	if (fn)
	{	
		fn(param, "正在校验MAC地址数据\n");
	}
	if (sniot_send_read_mac_atcmd(pline, tmp_mac_str))
	{
		printf("read mac(%s)\n", tmp_mac_str);
	}
	else
	{
		return SNIOT_ERROR_READ_MAC;
	}

	if (strcmp(tmp_mac_str, mac_str) != 0)
	{
		Log_e("read mac:(%s), write mac:(%s)", tmp_mac_str, mac_str);
		return SNIOT_ERROR_MAC_NOTEQUAL;
	}

	sniot_send_leave_test_atcmd();
	pline->used = TRUE;

	sprintf(lpszFilePath, "%s/Lic_record.txt", path);
	SniotSaveLicenseFile(lpszFilePath, plines);

	return SNIOT_SUCCESS;
}
