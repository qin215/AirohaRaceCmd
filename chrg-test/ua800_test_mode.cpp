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

#define UA800_NULL_TEST		(0)				// ��ʼATCMDָ��
#define UA800_ENTER_TEST	(1)				// ����TEST MODEָ��
#define UA800_LEAVE_TEST	(2)				// �뿪TEST MODEָ��

#define UA800_WRITE_ID		(3)				// д MAC/ID ָ��
#define UA800_READ_MAC		(4)				// �� MAC ָ��
#define UA800_READ_ID		(5)				// �� LICENSE ָ��

#define UA800_WRITE_MAC		(6)				// д MAC ָ��

#define UA800_WRITE_MAC2	(7)				// д MAC2 ָ��
#define UA800_READ_MAC2		(8)				// �� MAC2 ָ��

#define UA800_WRITE_EFUSE		(9)				// д EFUSE
#define UA800_READ_EFUSE		(10)			// �� EFUSE

#define UA800_WRITE_EFUSE2		(11)				// д EFUSE2
#define UA800_READ_EFUSE2		(12)				// �� EFUSE2

#define UA800_EFUSE_ENABLE		(13)				// д EFUSE ʹ��

#define UA800_ENABLE_BURST_MODE			(14)		// ����burst mode
#define UA800_WRITE_LICENSE0			(15)		// д���һ��license
#define UA800_WRITE_LICENSE1			(16)		// д��ڶ���license

/*
 * UA800ɨ�����MAC/ID
 */
#define MAX_SEG 5
#define MAX_RESEND_COUNT 3
static Boolean at_rsp_enter_cmd(buf_t *b, int *code);
static Boolean at_rsp_generic(buf_t *b, int *code);
static void homi_line_chomp(buf_t *b);
static Boolean at_rsp_enter(buf_t *b, int *code);
/*
 * write license 0 ����
 */
static void do_with_write_lic0_rsp(buf_t *b);

/*
 * write license 1 ����
 */
static void do_with_write_lic1_rsp(buf_t *b);

/*
* ����дLicenseָ��
*/
static Boolean ua800_send_write_lic_atcmd(midea_lic_t *pline);

/*
* ��ȡLicenseָ��
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
static int lic_crc_wr_main_value;			// д license main ���ص� crcֵ
static int lic_crc_wr_backup_value;			// д license backup ���ص� crcֵ
#define printf Log_d


extern "C" Boolean ua800_send_enter_test_atcmd();

typedef Boolean (*fn_ua800_read_mac_atcmd_t)(macaddr_id_t *pmac, char mac_str[32]);
typedef Boolean (*fn_ua800_write_mac_atcmd_t)(macaddr_id_t *pmac);

/*
 * �������ģʽ, ��֤����һ���ԣ��������豸
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
 * �˳�����ģʽ
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
 * ����
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
* дMAC
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
* дMAC
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
* дMAC
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
 * ATָ��ͬ��Event Handle
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
		disp_win_sys_err_msg(_T("���������¼�����!"));
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
 * ���дMAC��ַ,atcmd����ֵ�Ƿ�OK.
 * rsp: ATCMD�ķ����ַ���
 * mac_str: ��д���mac��ַ�ִ�
 * index: 1, MAC1; 2, MAC2
 * ����ֵ:
 *	TRUE: ����OK
 *  FALSE: ���س���
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

#define STATE_INIT_OK		(0)			// ��ʼ��״̬
#define STATE_GET_RSP_OK	(1)			// �ҵ���bootloader��־
#define STATE_GET_BOOT_OK	(2)			// �����ɹ��ˣ��ж�bProductTest״̬

/*
 * ״̬���
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
			ptr = strstr(p, "page header length");			// �����ɹ���־
			if (!ptr)
			{
				return STATE_GET_RSP_OK;
			}

			Log_d("get boot flag!\n");
			return STATE_GET_BOOT_OK;
		}

		case STATE_GET_BOOT_OK:
		{
			ptr = strstr(p, "bProductTest=");			// �����ɹ���־
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
 * ����test mode�Ĵ����߼���
 * 1. ������
 * 2. �ٷ���������ִ����������ɹ�
 */
static void do_enter_test_mode_rsp(buf_t *b)
{
	static int state_machine;		// ��ס��ǰ��״̬
	char *p;
	char *next_line;
	char *ptr;
	int len;
	char sep[10];				// �зָ��

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
 * ���ڻ�Ӧ����
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
 * write license 0 ����
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
 * write license 1 ����
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
 * ���ͽ������ģʽָ��
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

	for (i = 0; i < 1; i++)			// �������ģʽ��Ҫ��������������һ��
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
 * ���;����ATָ��
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
* �����뿪����ģʽָ��
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
* ����дMAC��ַָ��
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
* ���Ͷ�MAC��ַָ��
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

	memset(s_mac_addr_str, 0, sizeof(s_mac_addr_str));		// ��������

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
* ���Ͷ�SN��ַָ��
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
* ����дMAC��ַָ��
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
* ���Ͷ�MAC2��ַָ��
*/
Boolean ua800_send_read_mac2_atcmd(macaddr_id_t *pmac, char mac_str[32])
{
	buf_t *b;
	Boolean ret;
	int index;

	memset(s_mac_addr_str, 0, sizeof(s_mac_addr_str));		// ��������

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
* ����дEFUSE MAC��ַָ��
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
* ���Ͷ�EFUSE MAC��ַָ��
*/
Boolean ua800_send_read_efuse_atcmd(macaddr_id_t *pmac, char mac_str[32])
{
	buf_t *b;
	Boolean ret;
	int index;

	memset(s_mac_addr_str, 0, sizeof(s_mac_addr_str));		// ��������

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
* ����дEFUSE MACʹ��ָ��
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
 * �ı�mac/efuse��ַ
 * fn: ����UI�Ļص�����
 * param: UI���ݻ����Ĳ���
 * read_mac: ��mac/efuse����
 * write_mac: дmac/efuse����
 * pmac: mac��ַ����
 * prompt_str: ��ʾ���ַ���
 * ����ֵ��
 * ���궨��
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
		strcpy(tmp, "����д");
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
		strcpy(tmp, "����У��");
		strcat(tmp, prompt_str);
		strcat(tmp, "\n");

		//fn(param, "����У��MAC1��ַ����\n");
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
 * дһ��license key
 *  ���̣�
 * 1. ���˳�����ģʽ
 * 2. �������ģʽ��ϵͳ�Զ�����
 * 3. д��MAC
 * 4. д��ID/PASSWORD
 * 5. ���MAC���Ƿ����
 * 6. �˳�����ģʽ
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
			fn(param, "���ڶ���License����\n");
		}
	}

	if (!plines)
	{
		return UA800_ERROR_NOT_LIC_FILE;
	}

	if (fn)
	{	
		fn(param, "���ڲ���License��¼\n");
	}

	pline = (midea_lic_t *)MideaFindLineByMac(plines, mac_str);
	if (!pline)
	{
		return UA800_ERROR_NOT_FOUND_MAC;
	}


	// ��MAC��ַ���ù���QR���ӡ�����⣬ MAC��ַ�ظ��ˡ�
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

	if ((ret = ua800_do_mac_change(fn, param, ua800_send_read_mac_atcmd, ua800_send_write_mac_atcmd, &mac, "MAC1")) == UA800_SUCCESS)		// �޸�MAC1
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

#if WIFI_EFUSE_SUPPORT == 1			// wifi ģ���� efuse ����ֻ��д����
	if (fn)
	{	
		fn(param, "���ڽ���дEFUSE MACģʽ\n");
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
		fn(param, "����дLicense\n");
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

		// �ϱ���������������
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
 * ���� ����burst mode ָ��
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

	// ����burst mode
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

// ����license
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
 * ����дlicenseָ��
 * cmd: д0 or 1��
 * backup: FALSE, ������; TRUE, ��������
 * lic_data: License����, ������
 * lic_len: lic����
 */
Boolean ua800_send_wlicense_atcmd(int cmd, Boolean backup, const kal_uint8 *lic_data, int lic_len)
{
	// дmain0
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
* ����дLicenseָ��
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
* ��ȡLicenseָ��
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
*����
     ���MAC��ַ�Ƿ���Ч
*����
     const kal_uint8 *pmac - MAC��ַ
*����ֵ
     TRUE		- ��Ч
	 FALSE		- ��Ч
*����˵��
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

