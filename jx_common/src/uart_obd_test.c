#include <stdio.h>
#ifndef WIN32
#include "systemconf.h"
#include "serial_api.h"
#include "drv_uart.h"
#include "socket_driver.h"
#include "comm_apps_res.h"
#include "ssv_lib.h"
#else
#include "windows.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
#include "uart_obd_test.h"
#include "cJSON.h"
#include "mywin.h"
#include "qcloud_iot_export_log.h"

//#ifdef __OBD_TEST__

static int total_tx;
static int total_rx;
static int uart_total_rx;

#define MAX_UART_LOG_DATA_SIZE	(5 * 1024 * 1024)

static int uart_total_rx_array[MAX_PORT_NUM];

#define UART_LOG_FILE	"uart_tx"
#define NET_LOG_FILE	"net_rx"
#define UART_RX_FILE	"uart_rx"

#define LOG_DIR			"log\\"

static void log_data(buf_t *b, const char *filename, FILE **p_fp, int *p_index);

void obd_test_send_uart()
{
	static Boolean cs_inited = FALSE;
	static CRITICAL_SECTION cs; 
	
	buf_t *b;
	
	if (!cs_inited)
	{
		InitializeCriticalSection(&cs);
		cs_inited = TRUE;
	}

	EnterCriticalSection(&cs);

	b = get_buf(&uart_tx_queue);
	if (!b)
	{
		printf("null buff\n");
		LeaveCriticalSection(&cs);
		return;
	}

	uart_send_buf(b, NULL);
	remove_buf(&uart_tx_queue);
	free_buffer(b);
	
	LeaveCriticalSection(&cs);

	printf("total send = %d\r\n", total_tx);
}

static buf_t * obd_build_random_pack(buf_t *b)
{
	int i;
	int num;
	char *p = b->pbuff;
	static char ch_array[] = "abcdefghijklmnopqrstuvwxyzABCDEFHIJKLMNOPQRSTUVWXYZ0123456789";
	int str_len = strlen(ch_array);

	srand((unsigned int)time(NULL));
	num = rand() % OBD_RANDOM_MAX_PACK_SIZE;

	if (num <= 2)
	{
		num = 2;
	}
	
	for (i = 0; i < num - 2; i++)
	{
		int index;

		index = rand() % str_len;
		*p++ = ch_array[index];
	}

	*p++ = '\r';
	*p++= '\n';

	b->len = num;
	
	return b;
}

void obd_send_random_pack()
{
	static FILE *fp = NULL;
	static int filename_index = 0;
	buf_t *b;

	b = alloc_buffer();

	if (!b)
	{
		Log_e("no buffer!\n");
		return;
	}

	if (obd_build_random_pack(b))
	{
		log_data(b, UART_LOG_FILE, &fp, &filename_index);

		add_buf(&uart_tx_queue, b);
		total_tx += b->len;
		obd_test_send_uart();
	}
}

void obd_log_recv_buff(buf_t *b)
{
	static FILE *fp = NULL;
	static int filename_index = 0;

	log_data(b, NET_LOG_FILE, &fp, &filename_index);

	total_rx += b->len;
	Log_d("obd total rx = %d\r\n", total_rx);
}

void obd_log_uart_rx_data(buf_t *b)
{
	static FILE *fp = NULL;
	static int filename_index = 0;

	log_data(b, UART_RX_FILE, &fp, &filename_index);

	uart_total_rx += b->len;
	Log_d("obd uart total rx = %d\r\n", uart_total_rx);
}

/*
 * 将对应的端口串口数据记录到对应的串口文件中
 *	b : 缓冲区数据
 *	port: 串口端口号
 */
void obd_log_uart_rx_data_ex(buf_t *b, int port)
{
	static FILE *fp_array[MAX_PORT_NUM];
	static int index_array[MAX_PORT_NUM];

	FILE *fp = fp_array[port];
	int filename_index = index_array[port];
	char log_fname[MAX_PATH];

	_snprintf_s(log_fname, sizeof(log_fname), _TRUNCATE, "%s_%d", UART_RX_FILE, port);

	log_data(b, log_fname, &fp, &filename_index);
	fp_array[port] = fp;
	index_array[port] = filename_index;

	uart_total_rx_array[port] += b->len;
	Log_d("obd uart(%d) total rx = %d\r\n", port, uart_total_rx_array[port]);
}

/*
 * 将数据记录到文件中，如果文件的大小大于900K, 则重新开始记录
 * 参数：
 *  b : 保存数据的缓冲区
 *  filename: 文件名
 *  p_fp: 文件指针, 如果新文件则更新
 *  p_index: 文件名索引, 如果创建了新文件，则更新
 */
static void log_data(buf_t *b, const char *filename, FILE **p_fp, int *p_index)
{
	int pos;
	long filesize;
	char path[MAX_PATH];
	char *ptr;
	int len;
	FILE *file_fp = *p_fp;
	int filename_index = *p_index;
	const char *cur_dt;

	for (;;)
	{
		if (!file_fp)
		{
			ptr = get_program_path(path, MAX_PATH);
			strcat(path, LOG_DIR);
			ensure_directory_exist(path);
			len = strlen(path);
			_snprintf_s(&path[len], MAX_PATH - len, _TRUNCATE, "%s_%s.txt", filename, HAL_get_current_dt_filename());

			file_fp = fopen(path, "wb");
			if (!file_fp)
			{
				printf("open file failed: %s\r\n", path);
				return;
			}

			break;
		}

		// 获取文件大小
		pos = ftell(file_fp);
		fseek(file_fp, 0L, SEEK_END);  
		filesize = ftell(file_fp);
		fseek(file_fp, pos, SEEK_SET);

		// 收到数据大于900K, 重新写入新文件
		if (filesize >= MAX_UART_LOG_DATA_SIZE)
		{
			fclose(file_fp);
			file_fp = NULL;
			filename_index++;
		}
		else
		{
			break;
		}
	}

	cur_dt = HAL_Timer_current();
	fwrite(cur_dt, strlen(cur_dt), 1, file_fp);
	fwrite(b->pbuff, b->len, 1, file_fp);
	fflush(file_fp);

	*p_fp = file_fp;
	*p_index = filename_index;
}

//#endif

