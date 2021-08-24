#if 0
#include "contiki.h"
#include "uip.h"
#include "dbg-printf.h"
#include "socket_driver.h"
#include "ssv_lib.h"
#include "uiplib.h"
#include "ntp_client.h"
#include "sys/clock.h"
#include "sys_status_api.h"
#include "net/resolv.h"
#include "limits.h"
#include "string.h"
#include <stdio.h>
#else
#include "string.h"
#include <stdio.h>

#endif

#include "data_buff.h"

#if 1//def __SK_TIMER_SUPPORT__
#include "sk_timer.h"

#ifndef S16
#define S16 short
#endif

#define SK_NO_NEXT_EVENT 	0xFFFFFFFF

sk_timer_t gSkTimer[SK_TIMER_NUM + 1];
S16 gTimeZone;			// 当前时区

static Boolean sunday_first = FALSE;

int tuya_cons_timer_list(char *data, int size);

#define PEROID_T	10

/*
 * flag 传入的是 星期日 排最后一个, 根据sunday_first设置做调整
 */
static U8 sk_change_week_flag(U8 flag)
{
	U8 bit;
	
	if (!sunday_first)
	{
		return flag;
	}

	bit = flag & 1;

	flag >>= 1;
	flag |= (bit << 6);

	return flag;
}

static void sk_sort_timer(sk_timer_t *parray)
{
	int i;
	int j;
	U32 tmp;
	sk_timer_t t;
	
	for (i = 0; i < SK_TIMER_NUM; i++)
	{
		if (parray[i].timer_id == SK_TIMER_INVALID_ID)
		{
			continue;
		}

		tmp = parray[i].time;
		
		for (j = i + 1; j < SK_TIMER_NUM; j++)
		{
			if (parray[j].timer_id == SK_TIMER_INVALID_ID)
			{
				continue;
			}
			
			if (parray[j].time < tmp)
			{
				t = parray[j];
				parray[j] = parray[i];

				parray[i] = t;

				tmp = t.time;
			}
		}
		
	}

}

void sk_report_timer_action(sk_timer_t *p)
{

}

Boolean sk_do_timer_task(int index)
{
	sk_timer_t *p = &gSkTimer[index];

	if (index < 0 || index >= SK_TIMER_NUM)
	{
		return FALSE;
	}
	
	switch (p->timer_action)
	{
		case SK_ON:
		{
			printf("tid:%d@%d, week:%d, sk on!\n", p->timer_id,  p->time % PEROID_T, current_daytime.tm_wday);
			sk_report_timer_action(p);
			break;
		}

		case SK_OFF:
		{
			printf("tid:%d@%d, week:%d, sk off\n", p->timer_id, p->time % PEROID_T, current_daytime.tm_wday);
			sk_report_timer_action(p);
			break;
		}

		default:
			break;
	}

	return TRUE;
}

Boolean sk_remove_timer(int index)
{
	sk_timer_t t;

	if (index < 0 || index >= SK_TIMER_NUM)
	{
		return FALSE;
	}

	for (; index < SK_TIMER_NUM - 1; index++)
	{
		gSkTimer[index] = gSkTimer[index + 1];
	}

	gSkTimer[index].timer_id = SK_TIMER_INVALID_ID;

	return TRUE;
}

/*
 * 获取下一次定时器的时刻，SK_NO_NEXT_EVENT 表示无下一个循环事件
 */
U32 sk_timer_get_next_event_time(sk_timer_t *node, U32 curr_second)
{
	int week = current_daytime.tm_wday;
	int i;
	Boolean flag = FALSE;
	int days = 0;
	U8 wflag;
	//struct tm local_tm;


	//__secs_to_tm(curr_second + ntp_get_time_offset() * 3600,  &local_tm);

	week = 1; //local_tm.tm_wday;

	if (week == 0)			// 0 为星期日
	{
		week = 6;
	}
	else
	{
		week--;
	}
	
	printf("week=%d\n", week);
	if (week < 0 || week > 7)
	{
		printf("week overflow!\n");
		return FALSE;
	}

	if (!node->week_flag)
	{
		return SK_NO_NEXT_EVENT;
	}

	wflag = sk_change_week_flag(node->week_flag);
	printf("wflag=%x\n", wflag);

	for (i = week + 1, days = 1; i < 7; i++, days++)
	{
		if (wflag & ( 1 << i))
		{
			flag = TRUE;
			break;
		}
	}

	if (!flag)
	{
		for (i = 0; i <= week; i++, days++)
		{
			if (wflag & ( 1 << i))
			{
				flag = TRUE;
				break;
			}
		}
	}

	if (!flag)
	{
		return SK_NO_NEXT_EVENT;
	}

	printf("days=%d\n", days);

	return curr_second + days * PEROID_T; // * 24L * 3600L;
}

void sk_timer_check_callback(U32 current_second)
{
	sk_timer_t *p;
	int i;
	U32 next_time;
	static int old;

	p = gSkTimer;

	if (current_second / PEROID_T != old)
	{
		old = current_second / PEROID_T;

		current_daytime.tm_wday++;
		current_daytime.tm_wday %= 7;
	}


again:
	for (i = 0; i < SK_TIMER_NUM; i++)
	{
		if (p[i].timer_id == SK_TIMER_INVALID_ID)
		{
			break;
		}

		if (p[i].time <= current_second)
		{
			sk_do_timer_task(i);
			if ((next_time = sk_timer_get_next_event_time(&gSkTimer[i], current_second)) == SK_NO_NEXT_EVENT)
			{
				sk_remove_timer(i);
			}
			else
			{
				gSkTimer[i].time = next_time;
				sk_sort_timer(&gSkTimer[0]);
				sk_print_timer_list();
			}
			
			goto again;
		}
	}
}

static void sk_print_timer_node(sk_timer_t *node, int index)
{
	printf("idx:%d, id:%x, act:%d, flag:%x, t:%d\n", index, node->timer_id & 0xff, node->timer_action, node->week_flag, node->time);
}

void sk_print_timer_list()
{
	sk_timer_t *p;
	int i;
	
	p = gSkTimer;

	for (i = 0; i < SK_TIMER_NUM; i++, p++)
	{
		sk_print_timer_node(p, i);
	}
}

static unsigned short days[4][12] =
{
	{   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
	{ 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},
	{ 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
	{1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
};

unsigned int date_time_to_epoch(struct tm* date_time)
{
	unsigned int second = date_time->tm_sec;  // 0-59
	unsigned int minute = date_time->tm_min;  // 0-59
	unsigned int hour   = date_time->tm_hour;    // 0-23
	unsigned int day    = date_time->tm_mday - 1;   // 0-30
	unsigned int month  = date_time->tm_mon; // 0-11
	unsigned int year   = date_time->tm_year;    // 0-99
	return (((year/4*(365*4+1)+days[year%4][month]+day)*24+hour)*60+minute)*60+second + 946684800ul;
}

unsigned int datetime_to_second(int year, int month, int day, int hour, int minute, int second)
{
	struct tm t;

	t.tm_year = year - 2000;
	t.tm_mon = month - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = second;

	return date_time_to_epoch(&t);
}

#if 1 //def TEST
static void sk_timer_init()
{
	int i;
	struct tm dt;
	int v;
	char buf[256];

	dt.tm_hour = 7;
	dt.tm_min = 29;
	dt.tm_sec = 6;
	dt.tm_mday = 24;
	dt.tm_mon = 6 - 1;
	dt.tm_year = 17;

	printf("sec=%lu\n", datetime_to_second(2017, 6, 24, 16, 18, 0));



	memset(gSkTimer, 0, sizeof(gSkTimer));
	for (i = 0; i < SK_TIMER_NUM; i++)
	{
		gSkTimer[i].timer_id = SK_TIMER_INVALID_ID;
	}
	
	gSkTimer[0].timer_id = 0;
	gSkTimer[0].timer_action = SK_ON;
	gSkTimer[0].week_flag = 0; //1 | (1 << 1) | (1 << 2);		// 星期一, 二, 三 置位
	gSkTimer[0].timeZone = 8;
	gSkTimer[0].time = 360;

	gSkTimer[1].timer_id = 2;
	gSkTimer[1].timer_action = SK_ON;
	gSkTimer[1].week_flag = 0; //1 | (1 << 1) | (1 << 2);		// 星期一, 二, 三 置位
	gSkTimer[1].timeZone = 8;
	gSkTimer[1].time = 3;


	gSkTimer[2].timer_id = 4;
	gSkTimer[2].timer_action = SK_ON;
	gSkTimer[2].week_flag = 0;// 1 | (1 << 1) | (1 << 2);		// 星期一, 二, 三 置位
	gSkTimer[2].timeZone = 8;
	gSkTimer[2].time = 5;

	gSkTimer[3].timer_id = 9;
	gSkTimer[3].timer_action = SK_ON;
	gSkTimer[3].week_flag = 1 | (1 << 1) | (1 << 2);		// 星期一, 二, 三 置位
	gSkTimer[3].timeZone = 8;
	gSkTimer[3].time = 7;

	gSkTimer[4].timer_id = 3;
	gSkTimer[4].timer_action = SK_ON;
	gSkTimer[4].week_flag = 1 | (1 << 1) | (1 << 2);		// 星期一, 二, 三 置位
	gSkTimer[4].timeZone = 8;
	gSkTimer[4].time = 8;

	gSkTimer[5].timer_id = 4;
	gSkTimer[5].timer_action = SK_ON;
	gSkTimer[5].week_flag =  (1 << 6);		// 星期一, 二, 三 置位
	gSkTimer[5].timeZone = 8;
	gSkTimer[5].time = 9;

	v =	sk_timer_get_next_event_time(&gSkTimer[5], 0);


	tuya_cons_timer_list(buf, sizeof(buf));
	
}

void sk_timer_test()
{
	sk_timer_init();
	sk_print_timer_list();
	printf("================\n");
	sk_sort_timer(&gSkTimer[0]);
	sk_print_timer_list();
}

#pragma pack(1)
typedef struct __tuya_data_t
{
	U8 cmd;
	U8 version;
	U8 data_cmd;
	U8 status;
	union 
	{
		U8 _action;
		U8 _count;
	} _u;
	sk_timer_t sk_timer;
} tuya_data_t;
#pragma pack()

#define DWORD_SWAP(X)(((X)&0xff)<<24) + \
	(((X)&0xff00)<<8) + \
	(((X)&0xff0000)>>8) + \
	(((X)&0xff000000)>>24)
#define HTONL(X) DWORD_SWAP(X)

/*填充一个timer节点放到param所对应的buf中*/
void sk_timer_fill_one_timer(sk_timer_t *ptimer, char *buf, int size)
{
	U32 delta_time = 0;
	sk_timer_t *ptr;
	
	if (size < sizeof(sk_timer_t))
	{
		printf("size is too low!\n");
		return;
	}

	memcpy(buf, ptimer, sizeof(sk_timer_t));

	ptr = (sk_timer_t *)buf;
	
	/*计算时间差值，即最近生效时刻与*/
	delta_time = ptimer->time;
	delta_time = HTONL(delta_time);
	memcpy(&ptr->time, &delta_time, sizeof(U32));

	return;
}

/*遍历定时器组时，进行填充处理，timeCount表示是第几个，以便计算buf中的偏移*/
static void sk_timer_each_fill_handle(sk_timer_t *ptimer, void *param, int size, int timeCount)
{
	char *buf = param;
	int offset = 0;

	offset += (timeCount - 1) * sizeof(sk_timer_t);	/*每个定时器节点占7Bit*/
	sk_timer_fill_one_timer(ptimer, (buf + offset), size - offset);	

	return;
}

/*遍历所有的定时器节点，进行fun处理，fun由应用模块实现及传入,返回值为处理的有效节点个数*/
int sk_for_each_handle(sk_handle_timer_fun fun, void *param, int size)
{
	int i;
	int timerNum = 0;

	for (i = 0; i < SK_TIMER_NUM; i++)
	{
		if (SK_TIMER_INVALID_ID != gSkTimer[i].timer_id)
		{
			timerNum++;
			fun(&gSkTimer[i], param, size, timerNum);
		}
	}
	
	return timerNum;
}

/*
 * 将定时器数据填入到outdata中，size 为outdata大小, 返回值为所占用大小, -1 表示失败
 */
int sk_timer_get_list(U8 *outdata, int size, Boolean report)
{
	int timeNum = 0;

	if ((NULL == outdata))
	{
		printf("ERROR: sk_timer_get_list invalid param\n");
		return -1;
	}

	timeNum = sk_for_each_handle(sk_timer_each_fill_handle, &outdata[1], size - 1);
	outdata[0] = timeNum;

	if (!report)
	{
		outdata[0] = 0;
	}
	
	return timeNum * sizeof(sk_timer_t) + 1;
}

#define _action _u._action
#define _count _u._count
Boolean tuya_test_lan_do_with_cmd(char *pstr);

static Boolean check_float_digit(const char *pstr);
static void test_get_hours();

/*
 * 组装定时器列表中所有的定时器信息
 */
int tuya_cons_timer_list(char *data, int size)
{
	tuya_data_t *ptr;
	int len;
	char tmp[128];
	int i;
	int j;
	int index;
	char mydata[] = "BQEAAAYAAQAA///7qgIBAAD///usBAEAAP//+64JAQcA///7sAMBBwD///uxBAFAAP//+7I=";
	//char mytestdata[] = "{\"devId\":\"6ccd76d4d5ccb08430y5mn\",\"dps\":{\"1\":true,\"101\":\"BQEAAAYAAQAA///7qgIBAAD///usBAEAAP//+64JAQcA///7sAMBBwD///uxBAFAAP//+7I=\"},\"t\":1498806681,\"uid\":\"ay14987304859493qhbn\"}";
	char mytestdata[] = "{\"devId\":\"6ccd76d4d5ccb08430y5mn\",\"dps\":{\"1\":true},\"t\":1498806681,\"uid\":\"ay14987304859493qhbn\"}";

	ptr = (tuya_data_t *)tmp;

	ptr->cmd = 5;
	ptr->version = 1;
	ptr->data_cmd = 0;
	ptr->status = 0;

	len = sk_timer_get_list((U8 *)&ptr->_count, sizeof(tmp) - 4, /*FALSE*/TRUE);
	if (len == -1 || len > (sizeof(tmp) - 4))
	{
		return -1;
	}

	if (size < len * 2)
	{
		printf("size is low!\n");
		return -1;
	}

	for (i = 0, j = 0, index = 0; i < 4 + len; i++)
	{
		index += sprintf(&data[index], "%02x", tmp[i] & 0xFF);
	}

	printf("%s", data);
	//memset(tmp, 0, sizeof(tmp));
	//len = base64_decode(mydata, tmp);
	
	tuya_test_lan_do_with_cmd(mytestdata);

	check_float_digit("2.33aa5");

	test_get_hours();

	return index;
}


Boolean tuya_test_lan_do_with_cmd(char *pstr)
{
	Boolean ret = FALSE;
	char *p;
	const char *ptmp = "\"dps\":{";
	char *pend;
	char name[16] = { 0 };
	int type = 0 ;//cJSON_NULL;
	int val = 0;
	char *json_end;
	char *pbegin;

	//PR_DEBUG("json:%s\n", pstr);

	p = strstr(pstr, ptmp);
	if (!p)
	{
		return FALSE;
	}

	json_end = strstr(pstr, "},");
	if (!json_end)
	{
		return FALSE;
	}

	p += strlen(ptmp);

	for ( ; p < json_end; p++)
	{
		pbegin = strchr(p, '\"');
		if (!pbegin)
		{
			break;
		}

		pbegin++;
		pend = strchr(pbegin, '\"');
		if (!pend)
		{
			break;
		}

		strncpy(name, pbegin, pend - pbegin);

		pend++; //skip "
		pend++; // :

		if (strcmp(name, "1") == 0)
		{
			if (strncmp(pend, "true", 4) == 0)
			{
				//type = cJSON_True;
				pend += 4;
			}
			else if (strncmp(pend, "false", 5) == 0)
			{
				//type = cJSON_False;
				pend += 5;
			}
			else if (strncmp(pend, "null", 4) == 0)
			{
				//type = cJSON_NULL;
				pend += 4;
			}
			else 
			{
				//type = cJSON_Number;
				val = atoi(pend);

				while (isdigit(*pend))
				{
					pend++;
				}
			}

			p = pend;
			//ret = __tuya_update_obj_value_from_json(name, type, val);
		}
#if 0
		if (strcmp(name, "101") == 0)
		{
			char data[256];
			buf_t b;
			char *ptr;
			int len;

			init_buffer(&b);
			b.pbuff = data;
			b.size = sizeof(data);

			if (*pend != '"')
			{
				break;
			}

			pend++;

			if (!(ptr = strchr(pend, '"')))
			{
				break;
			}

			*ptr = '\0';
			b.len = base64_decode(pend, data);
			*ptr = '"';	

			print_buffer(&b);

			p = ++ptr;
		}
#endif
	}

	return ret;
}

static Boolean check_float_digit(const char *pstr)
{
	if (!(*pstr == '-' || isdigit(*pstr)))
	{
		return FALSE;
	}

	pstr++;

	while (*pstr && isdigit(*pstr))
	{
		pstr++;
	}

	if (!*pstr)
	{
		return TRUE;
	}

	if (*pstr != '.')
	{
		return FALSE;	
	}

	pstr++;
	while (*pstr && isdigit(*pstr))
	{
		pstr++;
	}

	if (!*pstr)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static const kal_int16 mydays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define START_YEAR 2017
static Boolean is_leap_year(int year)
{
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

struct datetime_struct
{
	kal_uint16 year;
	kal_uint16 month;
	kal_uint16 day;
	kal_uint16 hour;
	kal_uint16 minute;
	kal_uint16 second;
	kal_int16 offset;		// 时区 [-12, 12]
};

static int get_hours(struct datetime_struct *mydt)
{
	int i;
	int v = 0;

	for (i = START_YEAR; i < mydt->year; i++)
	{
		v += 365 + is_leap_year(i);
	}

	for (i = 0; i <= mydt->month; i++)
	{
		v += mydays[i];

		if (i == 1)
		{
			v += is_leap_year(mydt->year);
		}
	}

	v += mydt->day;

	v *= 24;
	v += mydt->hour;

	return v;
}

static void test_get_hours()
{
	struct datetime_struct d1;
	struct datetime_struct d2;
	int h1;
	int h2;

	d1.year = 2017;
	d1.month = 6;
	d1.day = 10;
	d1.hour = 3;
	d1.minute = 44;
	d1.second = 0;

	d2.year = 2050;
	d2.month = 11;
	d2.day = 28;
	d2.hour = 10;
	d2.minute = 44;
	d2.second = 0;

	h1 = get_hours(&d1);
	h2 = get_hours(&d2);


	printf("h2 - h1=%d\n", h2 - h1);

}
#endif


#endif

unsigned char data_pack[10];

void Check_CalaCRC(unsigned char *pdat,unsigned len)
{
	unsigned int Crc_data = 0,i;
	for(i = 0;i < len; i++)
	{
		Crc_data += *pdat++;
	}
	*pdat = (unsigned char)Crc_data;

}

void data_pack_pro(unsigned char addr,unsigned char number,unsigned char function,float *data1,float *data2,unsigned char i)
{
	data_pack[0] = 0x00;
	data_pack[1] = addr;
	if((*data1 == 0)&&(*data2 == 0)&&(i >=2))
	{
		number = 0xff;
	}
	data_pack[2] = number;
	data_pack[3] = function;
	data_pack[4] = ((int)((*data1)*10))& 0xff;
	data_pack[5] = ((int)((*data1)*10)>>8);
	data_pack[6] = ((int)((*data2)*10))& 0xff;
	data_pack[7] = ((int)((*data2)*10)>>8);
	Check_CalaCRC(&data_pack[2], 6);
	data_pack[9] = 0xEE;
}

void test_monitor()
{
	float a[2] = {0.0, 1.0};

	data_pack_pro(0x20, 0x01, 0x02, a, &a[1], 0);

}