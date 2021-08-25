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

static HANDLE hRaceCmdEvent = INVALID_HANDLE_VALUE;
static kal_uint16 s_race_cmd;

#define T5506_RACECMD_SEND_COUNT	5
//#define T5506_CMD_CLOSE_LOG			0x5000

#ifdef VERSION_V1
#define T5506_CMD_GET_RAW_DATA		0X0E06
#else
#define T5506_CMD_GET_RAW_DATA		0X2000
#endif



#define T5506_CMD_NULL				0X0000
#define RACE_CMD_UART_RCV_TIMEOUT	1000

#define ONEWIRE_FRAME_START			0x5
#define ONEWIRE_FRAME_SIGNAL_BYTE	0x5B

BOOL g_old_version = FALSE;


enum _RACE_CMD_ENUM 
{
	PSENSOR_RACE_CAL_CT = 3,
	PSENSOR_RACE_CAL_G2 = 4,
	PSENSOR_RACE_SWITCH_DBG = 5,
	PSENSOR_RACE_ENTER_DISCOVERY = 6,
	PSENSOR_RACE_ENTER_TWS_PAIRING = 7,
	PSENSOR_RACE_SENSOR_INIT_OK = 8,			// ONLY for debuggging
	PSENSOR_RACE_IN_EAR = 9,
	PSENSOR_RACE_OUT_EAR = 0xA,
	PSENSOR_RACE_RESET_FACTORY = 0XB,
	PSENSOR_RACE_ANC_ON = 0XC,
	PSENSOR_RACE_ANC_OFF = 0XD,
	PSENSOR_RACE_SPP_LOG_ON = 0XE,
	PSENSOR_RACE_SPP_LOG_OFF = 0XF,

	PSENSOR_DUMP_INFO = 0X10,
	PSENSOR_TEST_FAST_PAIRING = 0X11,
	PSENSOR_CHECK_CUSTOMER_UI = 0X12,
	PSENSOR_CHECK_PRODUCT_MODE = 0X13,
	PSENSOR_CHECK_PSENSOR_SIMU = 0X14,
	PSENSOR_SET_PRODUCT_MODE = 0x15,
	PSENSOR_CLEAN_PRODUCT_MODE = 0x16,
	PSENSOR_GET_CALI_STATUS = 0x17,

	PSENSOR_SET_CUSTOMER_UI = 0X18,
	PSENSOR_CLEAN_CUSTOMER_UI = 0X19,

	PSENSOR_GET_INEAR_STATUS = 0x1A,


	PSENSOR_GET_NEAR_THRESHOLD_HIGH = 0X1B,
	PSENSOR_GET_NEAR_THRESHOLD_LOW = 0X1C,

	PSENSOR_GET_FAR_THRESHOLD_HIGH = 0X1D,
	PSENSOR_GET_FAR_THRESHOLD_LOW = 0X1E,

	PSENSOR_GET_RAW_DATA_HIGH = 0X20,
	PSENSOR_GET_RAW_DATA_LOW = 0x21,

	PSENSOR_QUERY_CALI_STATUS = 0X22,
	//wlh begin
	PSENSOR_ANC_HIGH = 0X23,
	PSENSOR_ANC_LOW = 0X24,
	PSENSOR_ANC_WIND = 0X25,
	PSENSOR_CHANNEL_LEFT = 0X26,
	PSENSOR_CHANNEL_RIGHT = 0X27,
	//wlh end
	PSENSOR_ONE_PARAM_END,

	PSENSOR_GET_CALI_DATA = 0X30,
	PSENSOR_GET_RAW_DATA = 0x31,
};

#define T5506_CMD_GET_RAW_DATA_HIGH	PSENSOR_GET_RAW_DATA_HIGH
#define T5506_CMD_GET_RAW_DATA_LOW	PSENSOR_GET_RAW_DATA_LOW


static int snd_count = 0;
static int rcv_count = 0;
static int seq_index = 0;
static int rsp_value;


/*
 * id1/id2 帧头识别关键字节
 */
kal_uint8 *get_rsp_frame(BYTE id1, BYTE id2, kal_uint8* ptr, int len, uint8_t **next_frame, int *left_len)
{
	int i;
	const CHAR *p;
	const onewire_frame_t *pframe = NULL;
	int frame_len;
	
	for (i = 0, p = (const CHAR *)ptr; i < len; i++, p++)
	{
		 if (*p == id1 && (*(p + 1) == id2))
		 {
			pframe = (const onewire_frame_t *)p;

			frame_len = pframe->len + sizeof(pframe->len) + 2; 		// the length of frame length \ header \ type 
			if (frame_len <= (len - i))
			{
				*next_frame = (uint8_t *)(p + frame_len);
				*left_len = len - i - frame_len;

				return (kal_uint8 *)pframe;
			}
			
			// 不够一帧
			*next_frame = (uint8_t *)p;
			*left_len = len - i;
			return NULL;
		 }
	}

	// 未找到帧头
	*next_frame = NULL;
	*left_len = 0;
	
	return NULL;
}

/*
 * 在 buffer 中找到关键字所表示的帧数据
 */
onewire_frame_t * onewire_get_one_rsp_frame(BYTE id1, BYTE id2, kal_uint8 * protocol_buffer, int *plen)
{
	int left;
	uint8_t *next;
	onewire_frame_t *p;
	int buffer_index = *plen;

	//sz_print_pkt("protocol data:", protocol_buffer, buffer_index);

	p = (onewire_frame_t *)get_rsp_frame(id1, id2, protocol_buffer, buffer_index, &next, &left);
	if (!p)
	{
		if (next != NULL)
		{
			memmove(protocol_buffer, next, left);
			buffer_index = left;
		}
		
		*plen = buffer_index;

		return NULL;
	}
	else
	{
		sz_print_pkt("packet data:", (kal_uint8 *)p, p->len + 4);
	}

	buffer_index = left;
	*plen = buffer_index;

	return p;
}


// 处理数据流找到的帧数据
void do_onewire_frame_rsp(onewire_frame_t *pframe)
{
	BYTE event = pframe->event;

	// 显示接收包
	CString info;
	info.Format(_T("recv(%d):"), seq_index);
	char *precv = convert_binary_to_hex((kal_uint8 *)pframe, pframe->len + 4);
	info += CString(precv);
	dlg_update_status_ui(rcv_count, snd_count, RAW_DATA_VALUE_IGNORE, info);
	free(precv);

	int cmd_word = T5506_CMD_GET_RAW_DATA;

	if (g_old_version)
	{
		cmd_word = 0x0E06;
	}

	if (pframe->cmd != cmd_word)
	{
		Log_e("NOT T5506 cmd (%d)", pframe->cmd);
		return;
	}

	SetEvent(hRaceCmdEvent);

	BYTE value;
	if (!g_old_version)
	{
		if (pframe->event == PSENSOR_GET_RAW_DATA)
		{
			if (pframe->param[0] == FALSE)
			{
				rsp_value = RAW_DATA_VALUE_ERROR;
			}
			else
			{
				rsp_value = pframe->param[2];
				rsp_value <<= 8;
				rsp_value |= pframe->param[1];
			}
	

			Log_d(_T("raw data=%x(%d)"), rsp_value, rsp_value);
		}
		else
		{
			value = pframe->param[0];
			rsp_value = value;
			Log_d(_T("raw data=%x(%d)"), rsp_value, rsp_value);
		}

	}
	else
	{
		value = pframe->param[2];
		rsp_value = value;
		Log_d(_T("raw data=%x(%d)"), rsp_value, rsp_value);
	}
}


// 返回剩余的字节数
int process_data_buffer(kal_uint8 *pbuff, int len)
{
	onewire_frame_t *ptr;
	int nlen = len;
	kal_uint8 *tmp;
	kal_uint8 *p;
	int count = 0;

	while ((ptr = onewire_get_one_rsp_frame(ONEWIRE_FRAME_START, ONEWIRE_FRAME_SIGNAL_BYTE, pbuff, &nlen)))
	{
		int frame_len = ptr->len + 4;
		Log_d("Get frame(%d)", count);

		count++;

		rcv_count++;
		do_onewire_frame_rsp(ptr);
		
		// 消耗当前的数据包
		p = (kal_uint8 *)ptr;
		tmp = p + frame_len;

		memmove(pbuff, tmp, nlen);
	}


	return nlen;
}


/*
 * UART指令同步Event Handle
 */
void t5506_init_handle()
{
	if (hRaceCmdEvent != INVALID_HANDLE_VALUE)
	{
		return;
	}

	hRaceCmdEvent = CreateEvent(NULL, FALSE, FALSE, "RACE_CMD_UART_Event");
	if (hRaceCmdEvent == INVALID_HANDLE_VALUE)
	{
		Log_e("create event failed!\n");
		disp_win_sys_err_msg(_T("创建串口事件错误!"));
	}
}



/*
 * 构造race cmd数据包
 */
static buf_t * t5506_build_race_cmd(buf_t *b, kal_uint16 race_cmd)
{
	kal_uint8 *pbuf = NULL;

	kal_uint8 ps_raw_data[] = {0x05, 0x5A, 0x05, 0x00 ,0x06 ,0x0E ,0x00, 0x0B, 0x20};

	onewire_frame_t *pframe = (onewire_frame_t *)ps_raw_data;

	if (!g_old_version)
	{
		pframe->cmd = T5506_CMD_GET_RAW_DATA;
	}
	
	pframe->event = race_cmd;
	pframe->param[0] = race_cmd;

	pbuf = ps_raw_data;
	b->len = sizeof(ps_raw_data);
	memcpy(b->pbuff, pbuf, b->len);

	return b;
}


/*
 * 发送 RACE CMD 给耳机
 */
Boolean t5506_send_race_cmd(kal_uint16 racecmd)
{
	buf_t *b;
	int i;
	DWORD dwWait;
	BOOL ret = FALSE;

	t5506_init_handle();

	b = alloc_buffer();
	if (!b)
	{
		return FALSE;
	}

	s_race_cmd = racecmd;
	rcv_count = 0;
	for (i = 0; i < T5506_RACECMD_SEND_COUNT; i++)
	{
		t5506_build_race_cmd(b, racecmd);
		uart_send_buf(b, NULL);

		dwWait = WaitForSingleObject(hRaceCmdEvent, RACE_CMD_UART_RCV_TIMEOUT);

		if (dwWait == WAIT_ABANDONED)
		{
			printf("wait error!\n");
			break;
		} 
		else if (dwWait == WAIT_TIMEOUT)
		{
			printf("wait rsp timeout!\n");
			continue;
		}
		else if (dwWait == WAIT_OBJECT_0)
		{
			printf("enter test mode RSP ok!\n");
			ret = TRUE;
			break;
		}
		else
		{
			printf("wait error!\n");
		}
	}

	free_buffer(b);

	snd_count = i + 1;
	
	s_race_cmd = T5506_CMD_NULL;

	return ret;
}

/*
 * 更新主线程中的UI显示
 */
void dlg_update_status_ui(int rcv_count, int snd_cnt, int value, const CString& info)
{
	CMp_testDlg * pDlg = (CMp_testDlg *)AfxGetMainWnd();
	CString *pResult = NULL;
	CString *pInfo;

	pInfo = new CString(info);

	if (value >= 0)
	{
		CString prompt;
		prompt.Format(_T("\r\n收到包:%d, 发送包:%d，光感值：0x%x(%d)\r\n"), rcv_count, snd_cnt, value, value);

		pResult = new CString(prompt);
	}
	else
	{
		if (value == RAW_DATA_VALUE_IGNORE)
		{
			pResult = NULL;
		}
		else if (value == RAW_DATA_VALUE_ERROR)
		{
			pResult = (CString *)RAW_DATA_VALUE_ERROR;
		}
		
	}

	::PostMessage(pDlg->m_hWnd, WM_UPDATE_STATIC, (WPARAM)pResult, (LPARAM)pInfo);
}


// 获取耳机的光感数据
int t5506_send_get_raw_data()
{
	int val;

	if (!t5506_send_race_cmd(T5506_CMD_GET_RAW_DATA_HIGH))
	{
		CString info(_T("读取高字节错误！"));
		dlg_update_status_ui(rcv_count, snd_count, RAW_DATA_VALUE_ERROR, info);
		return 0;
	}
	val = rsp_value;
	if (!t5506_send_race_cmd(T5506_CMD_GET_RAW_DATA_LOW))
	{
		CString info(_T("读取低字节错误！"));
		dlg_update_status_ui(rcv_count, snd_count, RAW_DATA_VALUE_ERROR, info);
		return 0;
	}

	val <<= 8;
	val |= rsp_value;

	CString info(_T("读取光感值正确！"));

	dlg_update_status_ui(rcv_count, snd_count, val, info);

	return val;
}



// 获取耳机的光感数据
int t5506_send_get_raw_data_2()
{
	int val;

	if (!t5506_send_race_cmd(PSENSOR_GET_RAW_DATA))
	{
		CString info(_T("读取光感字错误！"));
		dlg_update_status_ui(rcv_count, snd_count, RAW_DATA_VALUE_ERROR, info);
		return 0;
	}
	val = rsp_value;
	CString info;

	if (val == RAW_DATA_VALUE_ERROR)
	{
		info = _T("光感通信错误");
	}
	else
	{
		info = _T("读取光感值正确！");
	}
	 

	dlg_update_status_ui(rcv_count, snd_count, val, info);

	return val;
}