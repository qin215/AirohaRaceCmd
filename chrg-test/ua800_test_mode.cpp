#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
#include "cJSON.h"
#include "homi_common.h"
#include "common.h"
#include "ua800_test_mode.h"
#include "qcloud_iot_export_log.h"
#include "mywin.h"
#ifdef MEICLOUD_SUPPORT
#include "media_test.h"
#endif
#include "resource.h"
#include "mp_testDlg.h"

#define UART_RCV_TIMEOUT	(3500)		// 3.5s

#define UA800_NULL_TEST		(0)				// 初始ATCMD指令
#define UA800_ENTER_TEST	(1)				// 进入TEST MODE指令
#define UA800_LEAVE_TEST	(2)				// 离开TEST MODE指令

#define UA800_WRITE_ID		(3)				// 写 MAC/ID 指令
#define UA800_READ_MAC		(4)				// 读 MAC 指令
#define UA800_READ_ID		(5)				// 读 LICENSE 指令

#define UA800_WRITE_MAC		(6)				// 写 MAC 指令

#define UA800_WRITE_MAC2	(7)				// 写 MAC2 指令
#define UA800_READ_MAC2		(8)				// 读 MAC2 指令

#define UA800_WRITE_EFUSE		(9)				// 写 EFUSE
#define UA800_READ_EFUSE		(10)			// 读 EFUSE

#define UA800_WRITE_EFUSE2		(11)				// 写 EFUSE2
#define UA800_READ_EFUSE2		(12)				// 读 EFUSE2

#define UA800_EFUSE_ENABLE		(13)				// 写 EFUSE 使能

#define UA800_ENABLE_BURST_MODE			(14)		// 进入burst mode
#define UA800_WRITE_LICENSE0			(15)		// 写入第一段license
#define UA800_WRITE_LICENSE1			(16)		// 写入第二段license

/*
 * UA800扫描更新MAC/ID
 */
#define MAX_SEG 5
#define MAX_RESEND_COUNT 3
static Boolean at_rsp_enter_cmd(buf_t *b, int *code);
static Boolean at_rsp_generic(buf_t *b, int *code);
static void homi_line_chomp(buf_t *b);
static Boolean at_rsp_enter(buf_t *b, int *code);
/*
 * write license 0 处理
 */
static void do_with_write_lic0_rsp(buf_t *b);

/*
 * write license 1 处理
 */
static void do_with_write_lic1_rsp(buf_t *b);

/*
* 发送写License指令
*/
static Boolean ua800_send_write_lic_atcmd(midea_lic_t *pline);

/*
* 获取License指令
*/
static Boolean ua800_get_write_lic_atcmd(midea_lic_t *pline);

static char myserver[32];
static int myport;

static Boolean timeout_flag = FALSE;
static int tick_count = 0;

static HANDLE hEvent = INVALID_HANDLE_VALUE;

static int bProductTest;

#define UA800_PARAM_LEN	128

static char s_mac_addr_str[32];
static char s_id_str[64];
static int s_at_cmd;
static char s_write_mac_addr_str[32];
static int burst_step;
static int lic_crc_wr_main_value;			// 写 license main 返回的 crc值
static int lic_crc_wr_backup_value;			// 写 license backup 返回的 crc值
#define printf Log_d


extern "C" Boolean ua800_send_enter_test_atcmd();

typedef Boolean (*fn_ua800_read_mac_atcmd_t)(macaddr_id_t *pmac, char mac_str[32]);
typedef Boolean (*fn_ua800_write_mac_atcmd_t)(macaddr_id_t *pmac);

/*
 * 进入测试模式, 保证程序一致性，先重启设备
 */
static buf_t * ua800_build_enter_test_mode(buf_t *b)
{
	char *pbuf;
	kal_uint8 bin_cmd[] = {0x05, 0x5A, 0x05, 0x00 ,0x06 ,0x0E ,0x00, 0x0B, 0x20};
	kal_uint8 off_log[] = {0x05, 0x5A, 0x06, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00};

	pbuf = b->pbuff;
	b->len = sizeof(off_log);
	memcpy(pbuf, off_log, b->len);

	return b;
}

/*
 * 退出测试模式
 */
static buf_t * ua800_build_leave_test_mode(buf_t *b)
{
	int len;
	int index;
	char *pbuf;

	pbuf = b->pbuff;

	index = 0;
	pbuf[index++] = 'A';
	pbuf[index++] = 'T';
	pbuf[index++] = '+';
	pbuf[index++] = 'J';
	pbuf[index++] = 'X';
	pbuf[index++] = '_';
	pbuf[index++] = 'E';
	pbuf[index++] = 'X';
	pbuf[index++] = 'I';
	pbuf[index++] = 'T';
	pbuf[index++] = 'T';
	pbuf[index++] = 'E';
	pbuf[index++] = 'S';
	pbuf[index++] = 'T';
	pbuf[index++] = '\r';
	pbuf[index++] = '\n';
	b->len = index;

	return b;
}


/*
 * 重启
 */ 
static buf_t * ua800_build_reboot(buf_t *b)
{
	int len;
	int index;

	index = 0;
	len = sprintf(&b->pbuff[index], "AT+REBOOT\r\n");
	b->len = len;

	return b;
}


/*
* 写MAC
*/
static buf_t * ua800_build_write_mac(buf_t *b, macaddr_id_t *param)
{
	int index;
	char *pbuf;

	pbuf = b->pbuff;

	index = 0;
	index += sprintf(&b->pbuff[index], "AT+RF_WRITE_MAC1=%02X:%02X:%02X:%02X:%02X:%02X\r\n", 
										param->mac_addr[0], param->mac_addr[1],
										param->mac_addr[2], param->mac_addr[3],
										param->mac_addr[4], param->mac_addr[5]);
	b->len = index;

	sprintf(s_write_mac_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
						param->mac_addr[0], param->mac_addr[1],
						param->mac_addr[2], param->mac_addr[3],
						param->mac_addr[4], param->mac_addr[5]);

	return b;
}

/*
* 写MAC
*/
static buf_t * ua800_build_write_mac2(buf_t *b, macaddr_id_t *param)
{
	int index;
	char *pbuf;

	pbuf = b->pbuff;

	index = 0;
	index += sprintf(&b->pbuff[index], "AT+RF_WRITE_MAC2=%02X:%02X:%02X:%02X:%02X:%02X\r\n", 
										param->mac_addr[0], param->mac_addr[1],
										param->mac_addr[2], param->mac_addr[3],
										param->mac_addr[4], param->mac_addr[5]);
	b->len = index;

	sprintf(s_write_mac_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
		param->mac_addr[0], param->mac_addr[1],
		param->mac_addr[2], param->mac_addr[3],
		param->mac_addr[4], param->mac_addr[5]);

	return b;
}

/*
* 写MAC
*/
static buf_t * ua800_build_write_efuse_mac(buf_t *b, macaddr_id_t *param)
{
	int index;
	char *pbuf;

	pbuf = b->pbuff;

	index = 0;
	index += sprintf(&b->pbuff[index], "AT+EFUSE_WRITE_MAC=%02X:%02X:%02X:%02X:%02X:%02X\r\n", 
										param->mac_addr[0], param->mac_addr[1],
										param->mac_addr[2], param->mac_addr[3],
										param->mac_addr[4], param->mac_addr[5]);
	b->len = index;

	sprintf(s_write_mac_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
		param->mac_addr[0], param->mac_addr[1],
		param->mac_addr[2], param->mac_addr[3],
		param->mac_addr[4], param->mac_addr[5]);

	return b;
}


/*
 * AT指令同步Event Handle
 */
void ua800_init_handle()
{
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		return;
	}

	hEvent = CreateEvent(NULL, FALSE, FALSE, "UART_Event");
	if (hEvent == INVALID_HANDLE_VALUE)
	{
		Log_e("create event failed!\n");
		disp_win_sys_err_msg(_T("创建串口事件错误!"));
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
 * 检查写MAC地址,atcmd返回值是否OK.
 * rsp: ATCMD的返回字符串
 * mac_str: 待写入的mac地址字串
 * index: 1, MAC1; 2, MAC2
 * 返回值:
 *	TRUE: 返回OK
 *  FALSE: 返回出错
 */
static Boolean ua800_check_write_mac_rsp(const char *rsp, const char *mac_str, int index)
{
	const char *pleft;
	const char *pright;
	int n;
	char *p;
	cJSON *proot;
	Boolean ret = FALSE;
	const char *key = index == 1 ? "addr1" : "addr2";
	cJSON *pItem;

	pleft = strchr(rsp, '{');
	if (!pleft)
	{
		Log_d("no { in rsp!\n");
		return FALSE;
	}

	pright = strchr(pleft, '}');
	if (!pright)
	{
		Log_d("no } in rsp!\n");
		return FALSE;
	}

	n = pright - pleft + 1;
	if (n < 0)
	{
		Log_d("loc } - loc { n = %d!\n", n);
		return FALSE;
	}

	p = (char *)calloc(n + 16, 1);
	if (!p)
	{
		Log_e("no memory!\n");
		return FALSE;
	}
	memcpy(p, pleft, n);

	proot = cJSON_Parse(p);
	if (!proot)
	{
		Log_e("json parse error!\n");
		goto done;
	}

	pItem = cJSON_GetObjectItem(proot, key);
	if (!pItem)
	{
		Log_e("no key(%s)\n", key);
		goto done;
	}

	if (strcmp(pItem->valuestring, mac_str) == 0)
	{
		Log_d("check mac(%d) (%s) equal\n", index, mac_str);
		ret = TRUE;
	}

done:
	if (proot)
	{
		cJSON_Delete(proot);
		proot = NULL;
	}

	free(p);
	return ret;
}

#define STATE_INIT_OK		(0)			// 初始化状态
#define STATE_GET_RSP_OK	(1)			// 找到了bootloader标志
#define STATE_GET_BOOT_OK	(2)			// 启动成功了，判断bProductTest状态

/*
 * 状态检查
 */
static int do_with_boot_state(const char *p, int state_machine)
{
	const char *ptr;

	switch (state_machine)
	{
		case STATE_INIT_OK:
		{
			ptr = strstr(p, "bootloader start");
			if (!ptr)
			{
				return STATE_INIT_OK;
			}

			Log_d("get entest rsp!\n");
			return STATE_GET_RSP_OK;
		}

		case STATE_GET_RSP_OK:
		{
			ptr = strstr(p, "page header length");			// 启动成功标志
			if (!ptr)
			{
				return STATE_GET_RSP_OK;
			}

			Log_d("get boot flag!\n");
			return STATE_GET_BOOT_OK;
		}

		case STATE_GET_BOOT_OK:
		{
			ptr = strstr(p, "bProductTest=");			// 启动成功标志
			if (!ptr)
			{
				return STATE_GET_BOOT_OK;
			}
			
			ptr += strlen("bProductTest=");
			
			if (*ptr == '1')
			{
				bProductTest = TRUE;
			}
			else if (*ptr == '0')
			{
				bProductTest = FALSE;
			}
			else
			{
				Log_e("bProductTest value error! (%s)\n", p);
			}

			Log_d("get init flag! bProductTest=%d\n", bProductTest);
			
			SetEvent(hEvent);
			return STATE_INIT_OK;
		}

		default:
			break;
	}

	return STATE_INIT_OK;
}
/*
 * 进入test mode的处理逻辑：
 * 1. 先重启
 * 2. 再发送特殊的字串表明重启成功
 */
static void do_enter_test_mode_rsp(buf_t *b)
{
	static int state_machine;		// 记住以前的状态
	char *p;
	char *next_line;
	char *ptr;
	int len;
	char sep[10];				// 行分割符

	memset(sep, 0, sizeof(sep));
	
	for (next_line = b->pbuff; next_line != NULL;)
	{
		ptr = next_line;
		len = get_one_line(ptr, (const char **)&next_line, sep);

		p = (char *)malloc(len + 1);
		if (!p)
		{
			Log_e("no memory!\n");
			return;
		}
		
		memset(p, 0, len + 1);
		strncpy(p, ptr, len);
		state_machine = do_with_boot_state(p, state_machine);
		free(p);
	}
}

/*
 * 串口回应处理
 */
void ua800_do_with_uart_rsp(buf_t *b)
{
	char *p;

//	print_buffer(b);

	b->pbuff[b->len] = '\0';

	//Log_d("recv str:%s", b->pbuff);
//	sz_print_pkt("recv bindata:", (kal_uint8 *)b->pbuff, b->len);
	b->len = process_data_buffer((kal_uint8 *)b->pbuff, b->len);
}


/*
 * write license 0 处理
 */
static void do_with_write_lic0_rsp(buf_t *b)
{
	char *ptr;

	ptr = strstr(b->pbuff, "cfg_buf=");
	if (ptr)
	{
		SetEvent(hEvent);
		return;
	}

	Log_e("illegal write lic0 rsp:'%s'\n", b->pbuff);
}

/*
 * write license 1 处理
 */
static void do_with_write_lic1_rsp(buf_t *b)
{
	char *ptr;
	int n;

#define MD_WLICENSE1_RSP_STRING			"Write Flash and check crc OK"
	ptr = strstr(b->pbuff, MD_WLICENSE1_RSP_STRING);
	if (!ptr)
	{
		Log_e("illegal write lic1 rsp:'%s'\n", b->pbuff);
		return;
	}

	ptr += strlen(MD_WLICENSE1_RSP_STRING);
	
	while (*ptr == ' ' || *ptr == '\t')
	{
		ptr++;
	}

	n = atoi(ptr);

	if (lic_crc_wr_main_value < 0)
	{
		lic_crc_wr_main_value = n;
	}
	else if (lic_crc_wr_backup_value < 0)
	{
		lic_crc_wr_backup_value = n;
	}

	SetEvent(hEvent);
}


/*
 * 发送进入测试模式指令
 */
Boolean ua800_send_enter_test_atcmd()
{
	buf_t *b;
	int i;
	DWORD ret;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	s_at_cmd = UA800_ENTER_TEST;
	bProductTest = -1;

	for (i = 0; i < 1; i++)			// 进入测试模式需要重启，仅仅发送一次
	{
		ua800_build_enter_test_mode(b);
		uart_send_buf(b, NULL);

#if 0
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
			printf("enter test mode RSP ok!\n");
			break;
		}
		else
		{
			printf("wait error!\n");
		}
#endif
	}

	free_buffer(b);

	s_at_cmd = UA800_NULL_TEST;

	if (bProductTest != 1)
	{
		Log_e("not enter test mode. bProductTest=%d\n", bProductTest);
		return FALSE;
	}

	return TRUE;
}

/*
 * 发送具体的AT指令
 */
static Boolean ua800_send_atcmd(buf_t *b, int cmd)
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

	s_at_cmd = UA800_NULL_TEST;

	return ok;
}


/*
* 发送离开测试模式指令
*/
Boolean ua800_send_leave_test_atcmd()
{
	buf_t *b;
	Boolean ok = FALSE;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	ua800_build_leave_test_mode(b);
	ok = ua800_send_atcmd(b, UA800_LEAVE_TEST);
	free_buffer(b);

	return ok;
}


/*
* 发送写MAC地址指令
*/
Boolean ua800_send_write_mac_atcmd(macaddr_id_t *pmac)
{
	buf_t *b;
	Boolean ret;
	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	ua800_build_write_mac(b, pmac);
	ret = ua800_send_atcmd(b, UA800_WRITE_MAC);
	free_buffer(b);

	return ret;
}

/*
* 发送读MAC地址指令
*/
Boolean ua800_send_read_mac_atcmd(macaddr_id_t *pmac, char mac_str[32])
{
	buf_t *b;
	Boolean ret;
	int index;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	memset(s_mac_addr_str, 0, sizeof(s_mac_addr_str));		// 先清除结果

	b->len = sprintf(b->pbuff, "%s", "AT+RF_READ_MAC1\r\n");
	ret = ua800_send_atcmd(b, UA800_READ_MAC);
	free_buffer(b);

	ret = check_mac_valid(s_mac_addr_str);

	if (ret)
	{
		memcpy(mac_str, s_mac_addr_str, 6);
	}

	return ret;
}


/*
* 发送读SN地址指令
*/
Boolean ua800_send_read_id_atcmd(macaddr_id_t *pmac, char id_str[64])
{
	buf_t *b;
	Boolean ret;
	int index;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	b->len = sprintf(b->pbuff, "AT+JX_READ_ID\r");
	ret = ua800_send_atcmd(b, UA800_READ_ID);
	free_buffer(b);

	if (ret)
	{
		strcpy(id_str, s_id_str);
	}

	return ret;
}

/*
* 发送写MAC地址指令
*/
Boolean ua800_send_write_mac2_atcmd(macaddr_id_t *pmac)
{
	buf_t *b;
	Boolean ret;
	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	ua800_build_write_mac2(b, pmac);
	ret = ua800_send_atcmd(b, UA800_WRITE_MAC2);
	free_buffer(b);

	return ret;
}

/*
* 发送读MAC2地址指令
*/
Boolean ua800_send_read_mac2_atcmd(macaddr_id_t *pmac, char mac_str[32])
{
	buf_t *b;
	Boolean ret;
	int index;

	memset(s_mac_addr_str, 0, sizeof(s_mac_addr_str));		// 先清除结果

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	b->len = sprintf(b->pbuff, "%s", "AT+RF_READ_MAC2\r\n");
	ret = ua800_send_atcmd(b, UA800_READ_MAC2);
	free_buffer(b);
	
	ret = check_mac_valid(s_mac_addr_str);

	if (ret)
	{
		memcpy(mac_str, s_mac_addr_str, 6);
	}

	return ret;
}

/*
* 发送写EFUSE MAC地址指令
*/
Boolean ua800_send_write_efuse_atcmd(macaddr_id_t *pmac)
{
	buf_t *b;
	Boolean ret;
	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	ua800_build_write_efuse_mac(b, pmac);
	ret = ua800_send_atcmd(b, UA800_WRITE_EFUSE);
	free_buffer(b);

	return ret;
}

/*
* 发送读EFUSE MAC地址指令
*/
Boolean ua800_send_read_efuse_atcmd(macaddr_id_t *pmac, char mac_str[32])
{
	buf_t *b;
	Boolean ret;
	int index;

	memset(s_mac_addr_str, 0, sizeof(s_mac_addr_str));		// 先清除结果

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	b->len = sprintf(b->pbuff, "%s", "AT+EFUSE_READ_MAC\r\n");
	ret = ua800_send_atcmd(b, UA800_READ_EFUSE);
	free_buffer(b);

	ret = check_mac_valid(s_mac_addr_str);

	if (ret)
	{
		memcpy(mac_str, s_mac_addr_str, 6);
	}

	return ret;
}

/*
* 发送写EFUSE MAC使能指令
*/
Boolean ua800_send_enable_efuse_atcmd()
{
	buf_t *b;
	Boolean ret;
	int index;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	b->len = sprintf(b->pbuff, "%s", "AT+EFUSE_ENABLE_MAC\r\n");
	ret = ua800_send_atcmd(b, UA800_EFUSE_ENABLE);
	free_buffer(b);

	return ret;
}

/*
 * test stub
 */
void test_ua800()
{
	buf_t *b;

	b = alloc_buffer();
	if (!b)
	{
		printf("no buffer!\n");
		return;
	}

}

BOOL MyString2HexData(const char *hex_str, UCHAR * outBuffer, int buf_len);


/*
 * 改变mac/efuse地址
 * fn: 更新UI的回调函数
 * param: UI传递回来的参数
 * read_mac: 读mac/efuse函数
 * write_mac: 写mac/efuse函数
 * pmac: mac地址管理
 * prompt_str: 提示的字符串
 * 返回值：
 * 见宏定义
 */
static int ua800_do_mac_change(fn_send_msg fn, void *param, 
							   fn_ua800_read_mac_atcmd_t read_mac,
							   fn_ua800_write_mac_atcmd_t write_mac,
							   const macaddr_mgmt_t *pmac, 
							   const char *prompt_str)
{
	kal_uint8 tmp_mac_str[32];
	char tmp[256];

	if (fn)
	{	
		strcpy(tmp, "正在写");
		strcat(tmp, prompt_str);
		strcat(tmp, "\n");
		fn(param, tmp);
	}

	if (!write_mac((macaddr_id_t *)&pmac->addr))
	{
		return UA800_ERROR_WRITE_MAC;
	}

	if (fn)
	{		
		strcpy(tmp, "正在校验");
		strcat(tmp, prompt_str);
		strcat(tmp, "\n");

		//fn(param, "正在校验MAC1地址数据\n");
		fn(param, tmp);
	}

	if (read_mac((macaddr_id_t *)&pmac->addr, (char *)tmp_mac_str))
	{
		printf("read mac:(%02X:%02X:%02X:%02X:%02X:%02X)\n", tmp_mac_str[0], tmp_mac_str[1], tmp_mac_str[2], tmp_mac_str[3], tmp_mac_str[4], tmp_mac_str[5]);
	}
	else
	{
		return UA800_ERROR_READ_MAC;
	}

	if (memcmp(tmp_mac_str, pmac->addr.mac_addr, 6) != 0)
	{
		Log_e("read mac:(%02X:%02X:%02X:%02X:%02X:%02X), write mac:(%02X:%02X:%02X:%02X:%02X:%02X)", 
			tmp_mac_str[0], tmp_mac_str[1], tmp_mac_str[2], tmp_mac_str[3], tmp_mac_str[4], tmp_mac_str[5],
			pmac->addr.mac_addr[0], pmac->addr.mac_addr[1], pmac->addr.mac_addr[2], 
			pmac->addr.mac_addr[3], pmac->addr.mac_addr[4], pmac->addr.mac_addr[5]);
		return UA800_ERROR_MAC_NOTEQUAL;
	}

	return UA800_SUCCESS;
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
int ua800_write_license_id(const char *lic_path, const char *mac_str, fn_send_msg fn, void *param, 
						   const char *http_server,
						   Boolean db_support)
{
	static midea_lic_t *plines;
	midea_lic_t *pline;
	TCHAR path[MAX_PATH];
	int ret = UA800_SUCCESS;
	Boolean flag = FALSE;
	kal_uint8 pmac[6];
	macaddr_mgmt_t mac;

	ua800_init_handle();
	get_program_path(path, sizeof(path));

	if (!plines)
	{
		plines = MideaReadLicenseFile(lic_path);

		if (fn)
		{	
			fn(param, "正在读入License数据\n");
		}
	}

	if (!plines)
	{
		return UA800_ERROR_NOT_LIC_FILE;
	}

	if (fn)
	{	
		fn(param, "正在查找License记录\n");
	}

	pline = (midea_lic_t *)MideaFindLineByMac(plines, mac_str);
	if (!pline)
	{
		return UA800_ERROR_NOT_FOUND_MAC;
	}


	// 此MAC地址被用过，QR码打印有问题， MAC地址重复了。
	if (pline->count >= 1)
	{
		return UA800_ERROR_MAC_USED;
	}

	if (check_mac_used(pline->mac_addr))
	{
		if (database_get_error_code() == DATABASE_CODE_NOT_OPEN)
		{
			return UA800_ERROR_DATABASE;
		}

		return UA800_ERROR_MAC_USED;
	}

	memset(&mac, 0, sizeof(macaddr_mgmt_t));

	if (!MyString2HexData(mac_str, pmac, sizeof(pmac)))
	{
		return UA800_ERROR_MAC_FORMAT;
	}
	memcpy(mac.addr.mac_addr, pmac, 6);


	if (!ua800_send_enter_test_atcmd())
	{
		return UA800_ERROR_PRODUCT_MODE;
	}

	if ((ret = ua800_do_mac_change(fn, param, ua800_send_read_mac_atcmd, ua800_send_write_mac_atcmd, &mac, "MAC1")) == UA800_SUCCESS)		// 修改MAC1
	{
		flag = TRUE;
	}
	else
	{
		return ret;
	}

	if ((ret = ua800_do_mac_change(fn, param, ua800_send_read_mac2_atcmd, ua800_send_write_mac2_atcmd, &mac, "MAC2")) == UA800_SUCCESS)
	{
		flag = TRUE;
	}
	else
	{
		return ret;
	}

#if WIFI_EFUSE_SUPPORT == 1			// wifi 模块中 efuse 仅仅只能写三次
	if (fn)
	{	
		fn(param, "正在进入写EFUSE MAC模式\n");
	}

	if (ua800_send_enable_efuse_atcmd())
	{
		if ((ret = ua800_do_mac_change(fn, param, ua800_send_read_efuse_atcmd, ua800_send_write_efuse_atcmd, &mac, "EFUSE")) != UA800_ERROR_WRITE_MAC)
		{
			Log_e("write efuse failed!\n");
		}
	}
	else
	{
		Log_e("enable write efuse failed!\n");
		ret = UA800_ERROR_WRITE_MAC;
	}
#endif

#if USB_KEY_SUPPORT == 1
	if (fn)
	{	
		fn(param, "正在写License\n");
	}

	if (!ua800_send_write_lic_atcmd(pline))
	{
		ret = UA800_ERROR_WRITE_ID;
	}
#endif

	if (ret == UA800_SUCCESS)
	{
#if 1
		static char new_lic_path[MAX_PATH];

		int index = pline - plines;

		pline->count++;

		if (strlen(new_lic_path) == 0)
		{
			append_filename_postfix(lic_path, new_lic_path, MAX_PATH, "_record");
		}

		if (!MideaSaveLicenseFileIndex(new_lic_path, plines, index))
		{
			Log_e("save lic file(%s) failed!\n", lic_path);
		}
#endif
		//update_db_record(pline->mac_addr, 1);
		if (db_support && !update_db_record_info(pline->mac_addr, 1, NULL, NULL, pline->lic_str, "midea"))
		{
			Log_e("write db error!\n");
			return UA800_ERROR_DATABASE;
		}

		// 上报美的生产服务器
#ifdef MEICLOUD_SUPPORT
		if (strlen(http_server) > 0)
		{
			CMediaTestResult tr(pline->mac_addr, 1, "B20181022");
			http_send_test_result(/*"192.168.3.118"*/http_server, 6690, "/api/iot/testing/burning/stduri/bulkadd", &tr, 1);
		}
		else
		{
			Log_e("No http server!\n");
		}

#endif
	}

	return ret;
}


/*
 * 发送 进入burst mode 指令
 */ 
static Boolean ua800_enter_burst_mode()
{
	buf_t *b;
	int i;

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	// 进入burst mode
	burst_step = -1;
	b->len = sprintf(b->pbuff, "~");

	for (i = 0; i < 11; i++)
	{
		ua800_send_atcmd(b, UA800_ENABLE_BURST_MODE);
		if (burst_step == 10)
		{
			break;
		}
	}

	if (burst_step != 10)
	{
		Log_d("enter burst mode NOK!\n");
		free_buffer(b);
		return FALSE;
	}

	free_buffer(b);

	return TRUE;
}

// 解密license
kal_uint8 * decrypt_license_data(midea_lic_t *pline, int *plen)
{
	char *ptr;
	int msize = 2 * 1024 + 128;
	int len;

	ptr = (char *)malloc(msize);
	if (!ptr)
	{
		Log_e("no memory!\n");
		return NULL;
	}

	len = midea_decrypt_license(pline->mac_addr, pline->lic_str, (kal_uint8 *)ptr, msize);
	if (len < 0)
	{
		Log_e("decrypt lic failed!\n");
		free(ptr);
		return NULL;
	}

	*plen = len;
	return (kal_uint8 *)ptr;
}


/*
 * 发送写license指令
 * cmd: 写0 or 1段
 * backup: FALSE, 主区域; TRUE, 备份区域
 * lic_data: License数据, 二进制
 * lic_len: lic长度
 */
Boolean ua800_send_wlicense_atcmd(int cmd, Boolean backup, const kal_uint8 *lic_data, int lic_len)
{
	// 写main0
	char *atcmd;
	int size = 4 * 1024;
	int index;
	int tmp;
	buf_t nb;

	init_buffer(&nb);

	atcmd = (char *)malloc(size);
	if (!atcmd)
	{
		Log_e("no memory!\n");
		return FALSE;
	}

	if (backup)
	{
		lic_crc_wr_backup_value = -1;
	}
	else
	{
		lic_crc_wr_main_value = -1;
	}

	index = sprintf(atcmd, "\r\nAT+USER_FLASH=w,%s,%d,", backup ? "backup" : "main" , cmd == UA800_WRITE_LICENSE0 ? 0 : 1);
	if (cmd == UA800_WRITE_LICENSE0)
	{
		tmp = Binary2HexData((const UCHAR *)(lic_data), lic_len / 2, (UCHAR *)&atcmd[index], size - index);
	}
	else
	{
		tmp = Binary2HexData((const UCHAR *)(lic_data + lic_len / 2), lic_len / 2, (UCHAR *)&atcmd[index], size - index);
	}

	if (tmp < 0)
	{
		free(atcmd);
		return FALSE;
	}

	index += tmp;
	atcmd[index++] = '\r';
	atcmd[index++] = '\n';
	atcmd[index++] = '\0';

	
	nb.pbuff = atcmd;
	nb.len = index;
	nb.size = size;
	if (!ua800_send_atcmd(&nb, cmd))
	{
		free(atcmd);
		return FALSE;
	}

	free(atcmd);
	return TRUE;
}

/*
* 发送写License指令
*/
static Boolean ua800_send_write_lic_atcmd(midea_lic_t *pline)
{
	int lic_len;
	kal_uint8 *lic_data;
	Boolean ret;
	kal_uint16 crc;

	if (!ua800_enter_burst_mode())
	{
		Log_e("enter burst mode failed!\n");
		return FALSE;
	}

	Log_d("enter burst mode OK!\n");
	
	lic_data = decrypt_license_data(pline, &lic_len);
	if (!lic_data)
	{
		Log_e("decrypt license failed!\n");
		return FALSE;
	}

	crc = crc16_ccitt_cal(0, lic_data, lic_len);

	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE0, FALSE, lic_data, lic_len);
	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE1, FALSE, lic_data, lic_len) && ret;
	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE0, TRUE, lic_data, lic_len) && ret;
	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE1, TRUE, lic_data, lic_len) && ret;

	if (lic_crc_wr_backup_value != lic_crc_wr_main_value)
	{
		Log_e("main crc(%d) IS not equal backup crc(%d)\n", lic_crc_wr_main_value, lic_crc_wr_backup_value);
		free(lic_data);
		return FALSE;
	}

	if (crc != lic_crc_wr_backup_value)
	{
		Log_e("main crc(%d) IS not equal calculate crc(%d)\n", lic_crc_wr_main_value, crc);
		free(lic_data);
		return FALSE;
	}

	Log_d("write license ok, calc crc=%d, read crc=%d!\n", crc, lic_crc_wr_main_value);
	free(lic_data);
	return ret;
}


/*
* 获取License指令
*/
static Boolean ua800_get_write_lic_atcmd(midea_lic_t *pline)
{
	int lic_len;
	kal_uint8 *lic_data;
	Boolean ret;
	kal_uint16 crc;

	lic_data = decrypt_license_data(pline, &lic_len);
	if (!lic_data)
	{
		Log_e("decrypt license failed!\n");
		return FALSE;
	}

	crc = crc16_ccitt_cal(0, lic_data, lic_len);

	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE0, FALSE, lic_data, lic_len);
	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE1, FALSE, lic_data, lic_len) && ret;
	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE0, TRUE, lic_data, lic_len) && ret;
	ret = ua800_send_wlicense_atcmd(UA800_WRITE_LICENSE1, TRUE, lic_data, lic_len) && ret;

	if (lic_crc_wr_backup_value != lic_crc_wr_main_value)
	{
		Log_e("main crc(%d) IS not equal backup crc(%d)\n", lic_crc_wr_main_value, lic_crc_wr_backup_value);
		free(lic_data);
		return FALSE;
	}

	if (crc != lic_crc_wr_backup_value)
	{
		Log_e("main crc(%d) IS not equal calculate crc(%d)\n", lic_crc_wr_main_value, crc);
		free(lic_data);
		return FALSE;
	}

	Log_d("write license ok, calc crc=%d, read crc=%d!\n", crc, lic_crc_wr_main_value);
	free(lic_data);
	return ret;
}


/*******************************************************************
*作用
     检查MAC地址是否有效
*参数
     const kal_uint8 *pmac - MAC地址
*返回值
     TRUE		- 有效
	 FALSE		- 无效
*其它说明
	2018/11/26 by qinjiangwei
********************************************************************/
Boolean check_mac_valid(const char *pmac)
{
	int i;
	int v = 0;

	for (i = 0; i < 6; i++)
	{
		v += pmac[i];
	}

	if (v == 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

