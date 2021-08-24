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

#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
#include "cJSON.h"

#if 0

#define HTTPCMD_MAX (512 + 128) /*256*/
#define HTTPRSP_MAX (512 + 512)

#define Boolean int

typedef struct t_HTTPMSG
{
    U8 msgtype;
    U8 *rsp;
    U16 rsplen;
}HTTPMSG;

typedef struct t_HTTP_REQ
{
#ifndef WIN32
    NETSOCKET httpsock;
#endif

    U8 httpstatus;
    void (*callbackfn)(void *);
    char hostname[32];
    uip_ip4addr_t ip_addr;
    U16 port;
    U8 httpcmd[HTTPCMD_MAX];
    U16 cmdlen;
    U8 httprsp[HTTPRSP_MAX];
    U16 rsplen;
}HTTP_REQ;

enum{
    HTTP_IDLE        = 0,
    HTTP_CONNECTING,
    HTTP_SENDDATA,
    HTTP_WAITRESPONSE,
};

enum{
    HTTPREQ_SUCC        = 0,
    HTTPREQ_STILL_RUN,
    HTTPREQ_CMD_ERROR,
    HTTPREQ_CONN_ERROR,
    HTTPREQ_RSP_DATA,
    HTTPREQ_RSP_TIMEOUT,
    HTTPREQ_RSP_ERROR,
};

void http_get_weather_from_yahoo();


#define POST_METHOD "POST "
#define GET_METHOD "GET "

#define POST_LEN 5
#define GET_LEN 4

#define NR 	5
#define OK 1
#define NOK 0
#define PS 2

#define LIST_START_CHAR '['
#define LIST_END_CHAR ']'

#define OBJ_START_CHAR '{'
#define OBJ_END_CHAR '}'

#define STR_CHAR '"'

typedef struct {
	char *key;
	char *value;
} pair_t;

static char buffer[256];
static Http_Header_Status s_status;
static HTTP_REQ httpreqdata;
static HTTPMSG httpmsg;
static const char *HTTP_STATUS = "HTTP/1.1 200 OK\r\n";
static const char *HTTP_CONTENT = "Content-Length: ";
static const char *HTTP_CHUNKED = "Transfer-Encoding: chunked\r\n";
static const char *HTTP_CHUNKED_END = "0\r\n\r\n";
static Boolean bDebug = TRUE;

static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff);

static char http_content[5 * 1024];
static int http_index;
static int http_ncount;
static int get_chunk_size(char *begin, char *end);
void print_weather_info(struct weather_info_struct *p_weather);

Boolean bEnter = FALSE;
Boolean bSuccess = FALSE;

/*
 * 建立HTTP请求头
 */
static int SendGetHttpCmd(U8 *url, char *body, void (*fn)(void *), Boolean bGzip)
{
	int tmpint;
	int cmdlen = strlen((char *)url);
    U8 *ptr1, *ptr2, *end = url + cmdlen;
	U8 *buff_end;
	char buffer[128];

    if (httpreqdata.httpstatus != HTTP_IDLE)
    {
        DebugPrintf("httpstatus:%d\n", httpreqdata.httpstatus);
        return HTTPREQ_STILL_RUN;
    }

    ptr1 = url;
    httpreqdata.rsplen = 0;
    httpreqdata.httprsp[0] = 0;
    if (memcmp(ptr1, "http://", 7) != 0)
    {
        DebugPrintf("1:%s\n", ptr1);
        return HTTPREQ_CMD_ERROR;
    }

    //Go to get remote ip information
    ptr1 += 7;
    ptr2 = ptr1;
    while (ptr2 < end)
    {
        if ((*ptr2 == ':') || (*ptr2 == '/'))
        {
            if (ptr2 - ptr1 > sizeof(httpreqdata.hostname) - 1)
            {
                return HTTPREQ_STILL_RUN;
            }
            else
            {
				memcpy(httpreqdata.hostname, ptr1, ptr2 - ptr1);
				httpreqdata.hostname[ptr2 - ptr1] = 0;
            }
            ptr1 = ptr2;
			
            break;
        }

        ptr2++;
    }

    //Go to check there's port information in command
    if (ptr1[0] == ':')
    {
        ptr1++;
        tmpint = 0;
        while (ptr1 < end)
        {
            if ('0' <= ptr1[0] && ptr1[0] <= '9')
            {
                tmpint = tmpint * 10 + ptr1[0] - '0';
                ptr1++;
            }
            else
            {
                break;
            }
        }

        if ((ptr1 >= end) || (ptr1[0] != '/') || (tmpint <= 0) || (tmpint >= 65536))
        {
            //DebugPrintf("2:%s, port:%d\n", ptr1 < end, tmpint);
            return HTTPREQ_CMD_ERROR;
        }
        else
        {
            httpreqdata.port = tmpint;
        }
    }
    else if (ptr1[0] == '/')
    {
        httpreqdata.port = 80;
    }
    else
    {
        DebugPrintf("3:%s\n", ptr1);
        return HTTPREQ_CMD_ERROR;
    }

	memset(httpreqdata.httpcmd, 0, sizeof(httpreqdata.httpcmd));
	httpreqdata.callbackfn = fn;
	ptr2 = httpreqdata.httpcmd;
	buff_end = ptr2 + sizeof(httpreqdata.httpcmd);
    memcpy(ptr2 , GET_METHOD, POST_LEN);
	ptr2 += GET_LEN;
    memcpy(ptr2, ptr1, (end - ptr1));
	ptr2 += (end - ptr1);

	if (body)
	{
		tmpint = strlen(body);
		if (ptr2 + tmpint > buff_end)
		{
			printf("ERROR: SEND BUFF IS SMALL@LINE = %d\r\n", __LINE__);
			return HTTPREQ_CMD_ERROR;
		}
		memcpy(ptr2 , body, tmpint);
		ptr2 += tmpint;
	}

    tmpint = strlen(" HTTP/1.1\r\nHost: ");
    memcpy(ptr2 , " HTTP/1.1\r\nHost: ", tmpint);
	ptr2 += tmpint;

    memcpy(ptr2 , httpreqdata.hostname, strlen((char *)(httpreqdata.hostname)));
	ptr2 += strlen((char *)(httpreqdata.hostname));

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nUser-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

#if 1
	ptr2 = AppendHeader(ptr2, buff_end, "\r\nConnection: keep-alive");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nCache-Control: no-cache");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}
#endif

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nContent-Type: application/x-www-form-urlencoded");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}
	
	ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept: */*");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	if (bGzip)
	{
		ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept-Encoding: gzip, deflate, sdch");
		if (!ptr2)
		{
			return HTTPREQ_CMD_ERROR;
		}

		ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept-Language: zh-CN,zh;q=0.8\r\n\r\n");
		if (!ptr2)
		{
			return HTTPREQ_CMD_ERROR;
		}
	}
	else
	{
		ptr2 = AppendHeader(ptr2, buff_end, "\r\n\r\n");
		if (!ptr2)
		{
			return HTTPREQ_CMD_ERROR;
		}
	}


    httpreqdata.cmdlen = ptr2 - httpreqdata.httpcmd;
    ptr2 = 0;

	//DebugPrintf("ip:%s, port:%d, cmd:%s\n", httpreqdata.ipstr, httpreqdata.port, ptr1);
    //DebugPrintf("len:%d, cmd:'%s'", httpreqdata.cmdlen, httpreqdata.httpcmd);
#ifndef WIN32    
    httpreqdata.httpsock = tcpconnect(&httpreqdata.ip_addr, httpreqdata.port, &http_req_process);
#endif
	//MF_ATPrintf("create socket:%d\n", httpreqdata.httpsock);
	s_status = HTTP_STATUS_INIT;

	if (win32_net_connect(httpreqdata.hostname, httpreqdata.port, FALSE) >= 0)
	{
		 return HTTPREQ_SUCC;
	}
	else
	{
		return HTTPREQ_CONN_ERROR;
	}
}

static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff)
{
	int len;

	len = strlen(buff);
	if (ptr + len > end_ptr)
	{
		printf("ERROR: SEND BUFF IS SMALL@LINE = %d\r\n", __LINE__);
		return NULL;
	}

	memcpy(ptr, buff, len);
	ptr += len;

	return ptr;
}

/*---------------------------------------------------------------------------*/
Http_Header_Status httprsp_parse(U8 *httprsp, U16 rsplen)
{
	U8 *ptr, *end = httprsp + rsplen;
	int contentlen = 0;
	int status_len = strlen(HTTP_STATUS);
	int content_len = strlen(HTTP_CONTENT);

    printf("rsp:%s\n\n\n", httprsp);

	for (ptr = httprsp; ptr < end; )
	{
		switch (s_status)
		{
			case HTTP_STATUS_INIT:
			{
			    if (strncmp((char *)ptr, HTTP_STATUS, status_len) != 0)
			    {
					httpmsg.msgtype = HTTPREQ_RSP_ERROR;
	                httpmsg.rsp = httprsp;
	                httpmsg.rsplen = ptr - httprsp;
					if (httpreqdata.callbackfn)
					{
	                	httpreqdata.callbackfn(&httpmsg);
					}

					return HTTP_STATUS_ERROR;
			    }
				else
				{
					s_status = HTTP_STATUS_GET_HEADER;
					ptr += status_len;
				}

				break;
			}

			case HTTP_STATUS_GET_HEADER:
			{
				Boolean bGetNum = FALSE;
				char *p = (char *)ptr;

				ptr = (U8 *)strstr((char *)p, HTTP_CONTENT);
				if (!ptr)
				{
					DebugPrintf("not found content, return\r\n");
					if ((ptr = (U8 *)strstr(p, HTTP_CHUNKED)))
					{
						const char *crlf = "\r\n\r\n";
						
						ptr = (U8 *) strstr((char *)ptr, crlf);
						if (!ptr)
						{
							DebugPrintf("not found chunked, return\r\n");
							return HTTP_STATUS_GET_HEADER;
						}
						
						ptr += 4;

						p = strstr((char *)ptr, "\r\n");
						if (!p)
						{
							printf("not get chunk length!\r\n");
						}

						contentlen = get_chunk_size(ptr, p);
						DebugPrintf("content len: %d\r\n", contentlen);

						ptr = p + 2;
						
						s_status = HTTP_STATUS_CHUNKED;
						http_index = 0;
						continue;
					}
					
					DebugPrintf("not found content & chunked, return\r\n");
					return HTTP_STATUS_GET_HEADER;
				}

				ptr += content_len;
			    while (ptr < end)
			    {
			        if ('0' <= ptr[0] && ptr[0] <= '9')
			        {
			            contentlen = contentlen * 10 + ptr[0] - '0';
			            ptr++;
						bGetNum = TRUE;
			        }
			        else
			        {
			            break;
			        }
			    }

				DebugPrintf("content: %d, bGetNum = %d", contentlen, bGetNum);
				if (!bGetNum)
				{
					return HTTP_STATUS_GET_HEADER;
				}

				http_index = 0;
				s_status = HTTP_STATUS_GET_CONTENT;

				break;
			}

			case HTTP_STATUS_GET_CONTENT:
			{
				const char *crlf = "\r\n\r\n";

				printf("here, return\r\n");

				ptr = (U8 *) strstr((char *)ptr, crlf);
				if (!ptr)
				{
					DebugPrintf("not found content, return\r\n");
					return HTTP_STATUS_GET_HEADER;
				}

				printf("here2, return\r\n");
				ptr += 4;
				
				memcpy(&http_content[http_index], ptr, end - ptr);		// 累积数据
				http_index += end - ptr;
				if (ptr + contentlen > end)
				{
					printf("content222: %d\r\n", contentlen);
					return HTTP_STATUS_GET_CONTENT;
				}

				printf("here3, return\r\n");
				ptr[contentlen] = 0;
				
				http_content[http_index] = '\0';
				
	            httpmsg.msgtype = HTTPREQ_RSP_DATA;
	            httpmsg.rsp = http_content;
	            httpmsg.rsplen = http_index;
            	if (httpreqdata.callbackfn)
				{
	            	httpreqdata.callbackfn(&httpmsg);
				}

				return HTTP_STATUS_GET_ALL;

			}

			case HTTP_STATUS_CHUNKED:
			{
				char *p;

				p = strstr(ptr, HTTP_CHUNKED_END);
				if (!p)
				{
					memcpy(&http_content[http_index], ptr, end - ptr);
					http_index += end - ptr;
					printf("more data chunk!\r\n");
					return HTTP_STATUS_CHUNKED;
				}

				memcpy(&http_content[http_index], ptr, p - ptr);
				http_index += p - ptr;

				http_content[http_index] = '\0';
				
	            httpmsg.msgtype = HTTPREQ_RSP_DATA;
	            httpmsg.rsp = http_content;
	            httpmsg.rsplen = http_index;
            	if (httpreqdata.callbackfn)
				{
                	httpreqdata.callbackfn(&httpmsg);
				}

				return HTTP_STATUS_GET_ALL;
			}

			default:
				break;
		}
	}

	return HTTP_STATUS_ERROR;
}

#define isblank(c)	((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')

int a2b(char c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	else if (c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}
	else if (c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
	else
	{
		printf("invalid char!\r\n");
		return 0;
	}
}

static int get_chunk_size(char *begin, char *end)
{
	int n;
	char tmp[32];
	int index = 0;
	int v = 0;
	int i;
	
	if (!begin || !end)
	{
		printf("null ptr!\r\n");
		return 0;
	}

	n = end - begin;
	if (n % 2 != 0)
	{
		tmp[index++] = '0';
	}

	if (n > sizeof(tmp))
	{
		printf("buff is small!\r\n");
		return 0;
	}

	memcpy(&tmp[index], begin, n);
	tmp[index + n] = '\0';

	for (i = 0; i < index + n; i += 2)
	{
		int t;

		t = a2b(tmp[i]) * 16 + a2b(tmp[i + 1]);

		v = v * 16 + t;
	}

	return v;
}

static char *BuildPostBody(pair_t *array)
{
	int i;
	int len;
	pair_t *ptr;
	char *p;

	memset(buffer, 0, sizeof(buffer));
	for (ptr = array, p = buffer; ptr->key != NULL; ptr++)
	{
		if (ptr != array)
		{
			*p++ = '&';
		}

		len = strlen(ptr->key);
		memcpy(p, ptr->key, len);
		p += len;

		*p++ = '=';

		len = strlen(ptr->value);
		memcpy(p, ptr->value, len);
		p += len;
	}

	return buffer;
}

char pos_latitude[32] = "114.055022";
char pos_longitude[32] = "22.5215";
char str_address[128];
Boolean bPosOk = FALSE;

static void trim_to_digits(char *str, int n)
{
	int i;
	char *p;

	for (p = str; *p && *p != '.'; p++)
	{
		;
	}

	if (*p == '\0')
	{
		return;
	}

	for (i = 0, p++; i < n; i++, p++)
	{
		if (*p == '\0')
		{
			return;
		}
	}

	*p = '\0';
}



static void http_get_long_latitude(void *pmsg)
{
	HTTPMSG *http = (HTTPMSG *)pmsg;
	cJSON *pRoot;
	cJSON* pItem;
	cJSON* pPoint;
	cJSON* pAddr;
	cJSON* pContent;

	http->rsp[http->rsplen] = '\0';

	pRoot = cJSON_Parse(http->rsp);
	if (!pRoot)
	{
		printf("json format error!\r\n");
		return;
	}

	pItem = cJSON_GetObjectItem(pRoot, "status");
	if (!pItem)
	{
		printf("can not get status item!\r\n");
		goto done;
	}

	if (pItem->valueint != 0)
	{
		printf("error code = %d\r\n", pItem->valueint);
		goto done;
	}

	pContent = cJSON_GetObjectItem(pRoot, "content");
	if (!pContent)
	{
		printf("can not get content\r\n");
		goto done;
	}

	pPoint = cJSON_GetObjectItem(pContent, "point");
	if (!pPoint)
	{
		printf("can not get point item\r\n");
		goto done;
	}

	pItem = cJSON_GetObjectItem(pPoint, "x");
	printf("x = %s,", pItem->valuestring);
	strncpy(pos_longitude, pItem->valuestring, sizeof(pos_longitude) - 1);
	trim_to_digits(pos_longitude, 4);
	
	pItem = cJSON_GetObjectItem(pPoint, "y");
	printf("y = %s\r\n", pItem->valuestring);
	strncpy(pos_latitude, pItem->valuestring, sizeof(pos_latitude) - 1);
	trim_to_digits(pos_latitude, 4);
		
	pAddr = cJSON_GetObjectItem(pContent, "address");
	strncpy(str_address, pAddr->valuestring, sizeof(str_address) - 1);
	printf("address=%s\r\n", str_address);
	
	bPosOk = TRUE;

done:
	cJSON_Delete(pRoot);
}


void http_get_city_name()
{
	const char *url = "http://api.map.baidu.com/location/ip?ak=jGBQpNiWLBjKaeEBnWzTS17HOnopA39M&coor=bd09ll";

	SendGetHttpCmd((U8 *)url, NULL, http_get_long_latitude, FALSE);
}

struct forecast_struct
{
	char dow[12];		// day of week
	int temp_high;
	int temp_low;
	char cond[32];	// 天气状况
};

/*
 *
 */
#if 1
struct dt_struct {
	kal_uint16 year;
	kal_uint16 month;
	kal_uint16 day;
	kal_uint16 hour;
	kal_uint16 minute;
	kal_uint16 second;
	kal_int16 offset;		// 时区 [-12, 12]
};
#endif

struct weather_info_struct 
{
	struct dt_struct local_dt;
	char city_name[64];
	char region_name[64];

	int humidity;	// 湿度
	int temperature;	//  温度
	int tmp_high;
	int tmp_low;
	char condition[32];	// 天气状况

 	struct forecast_struct array[2];
};

struct weather_info_struct g_weather_info;

#define isblank(c)	((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')


static char *skip_blank(char *p)
{
	int c;

	c = *p;
	if (/*c != ' '*/!isblank(c))
	{
		return p;
	}

	while (/* *p == ' ' */isblank(*p))
	{
		p++;
	}

	return p;
}

static void fill_forecast_info(struct forecast_struct *p_forecast, cJSON *pItem)
{
	cJSON *pTemp;
	
	if (!pItem)
	{
		printf("no array item\r\n");
		goto done;
	}

	if (!(pTemp = cJSON_GetObjectItem(pItem, "high")))
	{
		printf("no high!\r\n");
		goto done;
	}
	
	p_forecast->temp_high = atoi(pTemp->valuestring);
	
	if (!(pTemp = cJSON_GetObjectItem(pItem, "low")))
	{
		printf("no low!\r\n");
		goto done;
	}
	p_forecast->temp_low = atoi(pTemp->valuestring);

	if (!(pTemp = cJSON_GetObjectItem(pItem, "day")))
	{
		printf("no day!\r\n");
		goto done;
	}
	strncpy(p_forecast->dow, pTemp->valuestring, sizeof(g_weather_info.array[0].dow) - 1);

	if (!(pTemp = cJSON_GetObjectItem(pItem, "text")))
	{
		printf("no text!\r\n");
		goto done;
	}
	strncpy(p_forecast->cond, pTemp->valuestring, sizeof(g_weather_info.array[0].cond) - 1);

done:
	return;
}


const char* week_str[]= {
	"Mon",
	"Tues",
	"Wed",
	"Thur",
	"Fri",
	"Sat",
	"Sun",
	NULL
};
const char *month_str[] =
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
	NULL
};

static char *trim_blank(char *begin, char *end)
{
	int c;

	if (begin == NULL || end == NULL)
	{
		printf("null ptr!\r\n");
		return NULL;
	}

	c = *end;
	if (!isblank(c))
	{
		return end;
	}

	while (end > begin && isblank(*end))
	{
		end--;
	}

	return end;
}

static int get_dow(const char *begin, const char *end, Boolean bWeek)
{
	char **ptr_array = week_str;
	char *p;
	int n = end - begin;
	int i;

	if (!bWeek)
	{
		ptr_array = month_str;
	}
	
	for (i = 0; (p = ptr_array[i]); i++)
	{
		if (strncmp(begin, p, n) == 0)
		{
			return i;
		}
	}

	return -1;
}


static char *get_segment(char *ptr, char **p_begin, char **p_end, int c)
{
	char *begin;
	char *end;

	begin = skip_blank(ptr);
	end = strchr(begin, c);
	if (!end)
	{
		printf("no '%c'\r\n", c);
		return NULL;
	}
	ptr = end + 1;
	end = trim_blank(begin, end);

	*p_end = end;
	*p_begin = begin;

	return (ptr);
}

static Boolean fill_hour_min(char *begin, char *end, struct dt_struct *p_datetime)
{
	char *pb;
	char *pe;
	char *ptr;

	ptr = begin;
	
	ptr = get_segment(ptr, &pb, &pe, ':');
	if (!ptr)
	{
		return FALSE;
	}

	p_datetime->hour = atoi(pb);
	p_datetime->minute = atoi(ptr);

	ptr = get_segment(ptr, &pb, &pe, ':');
	if (!ptr)
	{
		return TRUE;
	}
	
	p_datetime->second= atoi(ptr);
	return TRUE;	
}

static Boolean offset_time(char *begin, char *end, struct dt_struct *p_datetime)
{
	if (strncmp(begin, "AM", 2) == 0)
	{
		return TRUE;
	}
	else if (strncmp(begin, "PM", 2) == 0)
	{
		p_datetime->hour += 12;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//"Fri, 13 Jan 2017 11:29 AM CST",
static Boolean fill_date_info(const char *ptr, struct dt_struct *p_datetime)
{
	char *p;
	char *begin;
	char *end;
	int dow;

	ptr = get_segment(ptr, &begin, &end, ',');
	if (!ptr)
	{
		return FALSE;
	}

	dow = get_dow(begin, end, TRUE);
	printf("dow = %d\r\n", dow);
	
	ptr = get_segment(ptr, &begin, &end, ' ');
	if (!ptr)
	{
		return FALSE;
	}
	p_datetime->day = atoi(begin);

	ptr = get_segment(ptr, &begin, &end, ' ');
	if (!ptr)
	{
		return FALSE;
	}
	p_datetime->month = get_dow(begin, end, FALSE);
	
	ptr = get_segment(ptr, &begin, &end, ' ');
	if (!ptr)
	{
		return FALSE;
	}
	p_datetime->year = atoi(begin);

	ptr = get_segment(ptr, &begin, &end, ' ');
	if (!ptr)
	{
		return FALSE;
	}

	if (!fill_hour_min(begin, end, p_datetime))
	{
		return FALSE;
	}
	
	ptr = get_segment(ptr, &begin, &end, ' ');
	if (!ptr)
	{
		return FALSE;
	}

	offset_time(begin, end, p_datetime);

	return TRUE;
}


//2017-01-17T00:36:35Z
void get_utc_datetime(char *pstr, struct dt_struct *p_datetime)
{
	char *begin;
	char *end;
	char *ptr;

	ptr = pstr;
	
	ptr = get_segment(ptr, &begin, &end, '-');
	if (!ptr)
	{
		return FALSE;
	}
	p_datetime->year = atoi(begin);
	
	ptr = get_segment(ptr, &begin, &end, '-');
	if (!ptr)
	{
		return FALSE;
	}
	p_datetime->month = atoi(begin) - 1;

	p_datetime->day = atoi(ptr);

	while (isdigit(*ptr))
	{
		ptr++;
	}

	if (*ptr != 'T')
	{
		printf("no T\r\n");
		return FALSE;
	}

	ptr++;
	fill_hour_min(ptr, NULL, p_datetime);

	return TRUE;	
}

Boolean is_leap_year(int year)
{
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

//							1  2   3  4   5   6   7   8   9   10  11  12
static const kal_int16 days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define START_YEAR 2017
int get_hours(struct dt_struct *dt)
{
	int i;
	int v = 0;
	
	for (i = START_YEAR; i < dt->year; i++)
	{
		v += 365 + is_leap_year(i);
	}

	for (i = 0; i <= dt->month; i++)
	{
		v += days[i];

		if (i == 1)
		{
			v += is_leap_year(dt->year);
		}
	}

	v *= 24;
	v += dt->hour;

	return v;
}

static int calc_time_region(struct dt_struct *utc, struct dt_struct *local_dt)
{
	int h1, h2;

	h1 = get_hours(utc);
	h2 = get_hours(local_dt);

	return h2 - h1;
}

static void http_get_weather(void *pmsg)
{
	HTTPMSG *http = (HTTPMSG *)pmsg;
	cJSON *pRoot;
	cJSON* pItem;
	cJSON* pQuery;
	cJSON* pChannel;
	cJSON* pLocation;
	cJSON* pDetails;
	cJSON* pCondition;
	cJSON* pArray;
	cJSON* pTemp;
	cJSON* pAtmos;
	struct dt_struct utc_dt;
	
	http->rsp[http->rsplen] = '\0';

	pRoot = cJSON_Parse(http->rsp);
	if (!pRoot)
	{
		printf("json format error!\r\n");
		return;
	}

	pQuery = cJSON_GetObjectItem(pRoot, "query");
	if (!pQuery)
	{
		printf("no query!\r\n");
		goto done;
	}

	pItem = cJSON_GetObjectItem(pQuery, "created");
	if (!pItem)
	{
		printf("no created item!\r\n");
		goto done;
	}

	get_utc_datetime(pItem->valuestring, &utc_dt);

	//strncpy(g_weather_info.utc_time, pItem->valuestring, sizeof(g_weather_info.utc_time) - 1);
	
	pItem = cJSON_GetObjectItem(pQuery, "results");
	if (!pItem)
	{
		printf("no results!\r\n");
		goto done;
	}

	if (!(pChannel = cJSON_GetObjectItem(pItem, "channel")))
	{
		printf("no channel!\r\n");
		goto done;
	}

	if (!(pAtmos = cJSON_GetObjectItem(pChannel, "atmosphere")))
	{
		printf("no atmosphere!\r\n");
		goto done;
	}

	if (!(pItem = cJSON_GetObjectItem(pAtmos, "humidity")))
	{
		printf("no humidity\r\n");
		goto done;
	}
	g_weather_info.humidity = atoi(pItem->valuestring);
	

	if (!(pLocation = cJSON_GetObjectItem(pChannel, "location")))
	{
		printf("no location!\r\n");
		goto done;
	}

	if (!(pItem = cJSON_GetObjectItem(pLocation, "city")))
	{
		printf("no city!\r\n");
		goto done;
	}
	strncpy(g_weather_info.city_name, pItem->valuestring, sizeof(g_weather_info.city_name) - 1);
	
	if (!(pItem = cJSON_GetObjectItem(pLocation, "region")))
	{
		printf("no region!\r\n");
		goto done;
	}
	strncpy(g_weather_info.region_name, pItem->valuestring, sizeof(g_weather_info.region_name) - 1);
	
	if (!(pItem = cJSON_GetObjectItem(pChannel, "lastBuildDate")))
	{
		printf("no lastBuildDate!\r\n");
		goto done;
	}
	fill_date_info(pItem->valuestring, &g_weather_info.local_dt);
	g_weather_info.local_dt.second = utc_dt.second;

	g_weather_info.local_dt.offset = calc_time_region(&utc_dt, &g_weather_info.local_dt);

	if (!(pDetails = cJSON_GetObjectItem(pChannel, "item")))
	{
		printf("no item!\r\n");
		goto done;
	}

	if (!(pCondition = cJSON_GetObjectItem(pDetails, "condition")))
	{
		printf("no condition!\r\n");
		goto done;
	}

	if (!(pItem = cJSON_GetObjectItem(pCondition, "temp")))
	{
		printf("no temp!\r\n");
		goto done;
	}
	
	g_weather_info.temperature = atoi(pItem->valuestring);
	
	if (!(pItem = cJSON_GetObjectItem(pCondition, "text")))
	{
		printf("no text!\r\n");
		goto done;
	}
	strncpy(g_weather_info.condition, pItem->valuestring, sizeof(g_weather_info.condition) - 1);

	if (!(pArray = cJSON_GetObjectItem(pDetails, "forecast")))
	{
		printf("no forecast!\r\n");
		goto done;
	}
	
	pItem = cJSON_GetArrayItem(pArray, 0);
	if (!pItem)
	{
		printf("no array 0\r\n");
		goto done;
	}

	if (!(pTemp = cJSON_GetObjectItem(pItem, "high")))
	{
		printf("no high!\r\n");
		goto done;
	}
	g_weather_info.tmp_high = atoi(pTemp->valuestring);
	
	if (!(pTemp = cJSON_GetObjectItem(pItem, "low")))
	{
		printf("no low!\r\n");
		goto done;
	}
	g_weather_info.tmp_low = atoi(pTemp->valuestring);

	// 明天天气预报
	pItem = cJSON_GetArrayItem(pArray, 1);
	fill_forecast_info(&g_weather_info.array[0], pItem);

	// 后天天气预报
	pItem = cJSON_GetArrayItem(pArray, 2);
	fill_forecast_info(&g_weather_info.array[1], pItem);

	bSuccess = TRUE;
	print_weather_info(&g_weather_info);
	
done:
	cJSON_Delete(pRoot);

	bEnter = FALSE;
}

int to_degree(int f)
{
	return 5 * (f - 32) / 9; 
}

#ifdef WIN32
void print_windows_system_time()
{
	SYSTEMTIME stTime;
	WORD wYear;
	WORD wMonth;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;

	GetLocalTime(&stTime);

	wYear = stTime.wYear;
	wMonth = stTime.wMonth;
	wDay = stTime.wDay;
	wHour = stTime.wHour;
	wMinute = stTime.wMinute;
	wSecond = stTime.wSecond;

	printf("%4d-%02d-%02d %02d:%02d:%02d\r\n", wYear, wMonth, wDay, wHour, wMinute, wSecond);
}
#endif

void print_weather_info(struct weather_info_struct *p_weather)
{
	printf("cityname = %s, %s\r\n", p_weather->city_name, p_weather->region_name);
	printf("current date: %d-%02d-%02d %02d:%02d:%02d, %d\r\n", p_weather->local_dt.year, p_weather->local_dt.month + 1, p_weather->local_dt.day + 1,
			p_weather->local_dt.hour, p_weather->local_dt.minute, p_weather->local_dt.second, p_weather->local_dt.offset);
#ifdef WIN32
	print_windows_system_time();
#endif
	printf("weather: temp = %d, high = %d, low  = %d, %s\r\n", to_degree(p_weather->temperature), to_degree(p_weather->tmp_high), to_degree(p_weather->tmp_low),
			p_weather->condition);
	
}

#define YQL_STR "http://query.yahooapis.com/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20woeid%20in%20(SELECT%20woeid%20FROM%20geo.places%20WHERE%20text=%22"
void http_get_weather_from_yahoo()
{
	static char url_str[256 + 64];
	const char post_str[] = "%22)&format=json";
	char addr[64 + 16];

	sprintf(addr, "(%s,%s)", pos_latitude, pos_longitude);

	strcpy(url_str, YQL_STR);
	strcat(url_str, addr);
	strcat(url_str, post_str);
	
	if (bSuccess)
	{
		print_weather_info(&g_weather_info);
		return;
	}

	if (bEnter)
	{
		printf("busy now!\r\n");
		return;
	}
	printf("url=%s\r\n", url_str);
	bEnter = TRUE;
	
	if (SendGetHttpCmd((U8 *)url_str, NULL, http_get_weather, TRUE) != HTTPREQ_SUCC)
	{
		bEnter = FALSE;
	}
}

#ifdef TEST

void hx_test_http()
{
	http_get_city_name();
}
#endif

void hx_send_http_content(int sock)
{
	static buf_t b;
	net_line_t *pnet;
	 
	init_buffer2(&b, httpreqdata.httpcmd, httpreqdata.cmdlen);

	 pnet = get_net_line(sock);
	 if (!pnet)
	 {
		 printf("sock not register?\n");
		 return;
	 }

	 if (!add_buf(&pnet->tx_queue, &b))
	 {
		 printf("no data in buf?\n");
		 return;
	 }

	 net_send_tx_queue(net_hekr_sock, FALSE, net_tcp_send_cb);
}

#endif