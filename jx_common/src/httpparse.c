#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#ifdef WIN32
#include "winsock2.h"
#include <WS2tcpip.h>
#endif

#include "httpparse.h"
#include "cJSON.h"


#pragma comment(lib, "ws2_32.lib")

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

#define DST_ZONE_UP_LIMIT 3
#define DST_ZONE_DOWN_LIMIT -2

#ifndef Boolean
#define Boolean unsigned char //int
#endif

#define NDAYS_FORECAST 3

#define BAIDU_FLAG		1
#define GOOGLE_FLAG		2
#define MAX_LOOP_COUNT 5

#define  POLL_WEATHER_PRIO OS_TASK_PRIO2

#define HTTP_SSL_STACK_SIZE 	(512 + 512 + 512 + 200)
#define POLL_WEATHER_STACK_SIZE (512)

/*
 * HTTP 请求这块不能重入，因此必须等待进入IDLE状态
 */
#define WAIT_HTTP_IDLE()	 	do { \
									while(phttpreqdata->httpstatus != HTTP_IDLE) {\
									OS_MsDelay(2000);}\
							} while(0)
#define isblank(c)	((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')
									

#if 0
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
struct dt_struct 
{
	kal_uint16 year;
	kal_uint16 month;
	kal_uint16 day;
	kal_uint16 hour;
	kal_uint16 minute;
	kal_uint16 second;
	kal_int16 offset;		// 时区 [-12, 12]
};

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

 	struct forecast_struct array[NDAYS_FORECAST];
};

typedef struct
{
	char id[36 + 1];
	char message[128];
	long datetime;
} ServerMsg;

#endif

enum
{
	GET_CITY,
	GET_WEATHER,
	GET_TIME,
	CHECK_OTA
};


Http_Header_Status httprsp_parse(U8 *httprsp, U16 rsplen);
static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff);
static kal_bool soc_ip_check(char *asci_addr, kal_uint8 *ip_addr, kal_bool *ip_validity);
static int get_chunk_size(char *begin, char *end);
static void http_do_action_rsp();
void make_weather_rsp_cb(int error_code, int len);

static void print_dt(struct dt_struct *utc);

Http_Header_Status httprsp_parse(U8 *httprsp, U16 rsplen);
static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff);
static int GetRspCode(char *str);
static Boolean GetMsgIds(char *buff);

kal_bool soc_ip_check(char *asci_addr,
                      kal_uint8 *ip_addr,
                      kal_bool *ip_validity);
kal_uint8* get_ip_from_cache(char *host);
void add_ip_to_table(const char *host, kal_uint8 ip[]);
static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff);
static int get_chunk_size(char *begin, char *end);
void print_weather_info(struct weather_info_struct *p_weather);
static int mark_server_flag(int index);
static int get_server_flag();
void sbkj_update_srv_flag(U8 *p_flag);
void sbkj_get_srv_flag(U8 *p_flag);

Boolean check_testmode_pos_ok();
Boolean http_get_city_name();


#ifdef EMBED_SYSTEM
/*******************************************************************
*作用
     启动天气预报任务
      1. 由于googleapis定位需要占用较大的stack, rsp buffer需要的少，特点和天气预报正好相反，因此提出单独一个线程
*参数
     OsTimer t - 暂时不用
*返回值
     无
*其它说明
	2018/11/26 by qinjiangwei
********************************************************************/
static void start_weather_task(OsTimer t);

/*******************************************************************
*作用
      地理定位任务
      1. 由于googleapis定位需要占用较大的stack, rsp buffer需要的少，特点和天气预报正好相反，因此提出单独一个线程
*参数
     void *pdata - 暂时不用
*返回值
     无
*其它说明
	2018/11/26 by qinjiangwei
********************************************************************/
static void http_get_position_task(void *pdata);
#endif

static const char cert[] = "-----BEGIN CERTIFICATE-----\n"
"MIIDOzCCAiOgAwIBAgIBADANBgkqhkiG9w0BAQsFADBgMQswCQYDVQQGEwJDTjES\n"
"MBAGA1UECAwJWmhlIEppYW5nMRIwEAYDVQQHDAlIYW5nIFpob3UxEDAOBgNVBAoM\n"
"B1RlbmNlbnQxFzAVBgNVBAMMDmlvdC5xY2xvdWQuY29tMB4XDTE3MDkxNzA5MTk0\n"
"M1oXDTE4MDkxNzA5MTk0M1owITEfMB0GA1UEAwwWUUNsb3VkIElPVCBDZXJ0aWZp\n"
"Y2F0ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK90GB0/CXgpIJFf\n"
"1w7KzGaADePYUeF0g3+VuxVfbwuVbYozM0Rb8uoteiDWoGzNT39MefJPde9hdSpZ\n"
"8d0jiAoQ796DYhbAjA3rFx1/Y9LqAuuDbslxua1moLpj5+IYMpRWsX7FwX/eXADn\n"
"qX9dHhjao+ZEldU5/mXUpXN40C/xtZAVW9pM+l4YxFBTv5epaSNiXPmDQXvXIm66\n"
"l+Ex50Cl+NY2VnvwO34QNETP/bClOlQiMEIA0iUV0xdr9npGKP9F08AcF2Eo9Sqx\n"
"NhmEgCEi3wSk9Ogncdkxa2OdY1z5gZC9TOaHmfjWv1IG5BbCQj0mwyTpb1vgltws\n"
"+oSynj8CAwEAAaM/MD0wDAYDVR0TAQH/BAIwADAOBgNVHQ8BAf8EBAMCB4AwHQYD\n"
"VR0OBBYEFA7kKJ88Ox/Xwwt0TSorlKeBIYD1MA0GCSqGSIb3DQEBCwUAA4IBAQCF\n"
"lGTSc9MuHyL9w9cOeH8Vgc0AgAQ+speIq2dO9GzSx14W7w85o/IvCtoBX5RZ4bmC\n"
"OuJJH3vMJ2YTOcVUB8rsVJlpJm08quQ/Ua7gUWc3anYQ+9oq90Uypipk+ATSRKLm\n"
"UJCliHKHN+Utal5oAWknDyZU5TXCbJi6V+M794sPBUyxi/QGiPva+Dn4UoBBpGg0\n"
"lJUgND1xzwPhyBVIll6OpopSwmEIz0e8WejSKQedbT2BF5+4BSwcRUcrvp8Yz8nQ\n"
"tvPWtSEXs/j794Gh+9240GrNQ4YE1JnRHrBIRHtniK0xGLQrRFtBiafzeIoJpK6N\n"
"qvjtCbuWt3sOwq7321wd\n"
"-----END CERTIFICATE-----\n";


static const char privatekey[] = "-----BEGIN PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCvdBgdPwl4KSCR\n"
"X9cOysxmgA3j2FHhdIN/lbsVX28LlW2KMzNEW/LqLXog1qBszU9/THnyT3XvYXUq\n"
"WfHdI4gKEO/eg2IWwIwN6xcdf2PS6gLrg27JcbmtZqC6Y+fiGDKUVrF+xcF/3lwA\n"
"56l/XR4Y2qPmRJXVOf5l1KVzeNAv8bWQFVvaTPpeGMRQU7+XqWkjYlz5g0F71yJu\n"
"upfhMedApfjWNlZ78Dt+EDREz/2wpTpUIjBCANIlFdMXa/Z6Rij/RdPAHBdhKPUq\n"
"sTYZhIAhIt8EpPToJ3HZMWtjnWNc+YGQvUzmh5n41r9SBuQWwkI9JsMk6W9b4Jbc\n"
"LPqEsp4/AgMBAAECggEACbGrZ2pFNqtnDkEyW7Hey0cF/XHFTGSoo1L9jDfCpewy\n"
"qjEcAwnliQTMO6ZAJOLPIqRyFG5JgNlspNNlZqeA67V122B8+e7XjilQFKQmOtAN\n"
"K0AOzfiHnsoN1V6RwE8/9Cw6EFwSzeLWSIKEPfJCKHNkHOPrN2XF5ZBzN8cUEaH8\n"
"zs+/gnKJ0X9oijqf0tChSsiPG++3K12y54aELNIxw2/8RzleplDZ27ZzWI2JGo0e\n"
"3E4oQisNYFLV013n3pgM5wtZtmogd2e3Xi3+yu+BbtIEFciIMoHciPxedK3G7ABU\n"
"eXZMAREzSZ8Af4FH2r7M6wTpyZFLmHtbgNlJlRDGKQKBgQDXpPapJXhIFUWFbNOd\n"
"ikUz08tPSBNOPpgumA099LP6tGkF4MVElZXqIAzFMgQ0nj2uL56ot8zwM0NRi/dL\n"
"oIK+pFqVf4MSxCLVAYXvlMjE66J5Y3YM+ew48t4fc0/HOAusnCSpj8mpylDJAFXl\n"
"Vwn91dHFS5+rDRDuZGATBlFIUwKBgQDQSazr0S15pvrVksBAxnLV1qKVwMqUErHR\n"
"msP++z3mfmYdpV1ssv16Z0DL1HK6tsNbHkqBphtPyKkjAZy8oaJdMtCbYQ9OLFE5\n"
"RCZ4S5Zyb93wJH9u5EI367SL/QMxQ3P7/u82A02SqPP2AB7n+2n083gASgv28aIz\n"
"Kgvv4cHk5QKBgDa0CUJ1V+LpSn6DUddICTARjSSqgwM5S7+eINXTqohxlabBEynH\n"
"zITc5oBrpLd4YOs790KThbB1QKGCIRSVPjWAWd5Mv4JWMQPZ5BEXrCqQH+ItQfge\n"
"ujbkHUbbKKS/6e46me5NpHmm8gFbTW7lOoqi4Bjtiy6IHQBshBavZpUzAoGAefCo\n"
"K+bqmK4Ja7/ejg2gbVCbHtEfyFCpiezxkfXE54xYfEKzz0961o5cgPh/spANDutM\n"
"81or4ym226e1+zkltSpqtoy8SSfo5X3gh8y454ZWiKDVejZoDhUQPmSB2fWDkaRO\n"
"p3CHmbUQHpUzgtzNy3o+Zuzy1D/ildGn62hf+4ECgYEAxtaxMaeoM+3HYeVivgPs\n"
"jzR48vYD3Tanf3qDUXcqQS1iL4Kk+XYYIcAd+Mq+l/EBE1rGQLLMPYMcXzdn9U29\n"
"K2u0KAunE6KYkcpU37KNrxJ9da3r64GjTJD/SbIud8JxxY38NctgPpxbOuQN3ilp\n"
"qmNt7lOnenBHNTPc8J2wuzw=\n"
"-----END PRIVATE KEY-----\n"
;

static const char rootCA[] = "-----BEGIN CERTIFICATE-----"
"MIIE/TCCA+WgAwIBAgIQSyxmRvQ0d1tn3zSgTdUE5jANBgkqhkiG9w0BAQsFADBI"
"MQswCQYDVQQGEwJVUzEgMB4GA1UEChMXU2VjdXJlVHJ1c3QgQ29ycG9yYXRpb24x"
"FzAVBgNVBAMTDlNlY3VyZVRydXN0IENBMB4XDTE0MTAwMTE5MjIzNloXDTI0MDky"
"ODE5MjIzNlowgbUxCzAJBgNVBAYTAlVTMREwDwYDVQQIEwhJbGxpbm9pczEQMA4G"
"A1UEBxMHQ2hpY2FnbzEhMB8GA1UEChMYVHJ1c3R3YXZlIEhvbGRpbmdzLCBJbmMu"
"MT0wOwYDVQQDEzRUcnVzdHdhdmUgT3JnYW5pemF0aW9uIFZhbGlkYXRpb24gU0hB"
"MjU2IENBLCBMZXZlbCAxMR8wHQYJKoZIhvcNAQkBFhBjYUB0cnVzdHdhdmUuY29t"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA49OohlJHBJ/juiQTmibO"
"S28pHvCCp1bPOc6elQ62DCYE/A14JXUtI8yaUc9nFjh6dNzR7KjFPEq/qDplQl7f"
"1Tvh5ifBNNP2wQ6e820TWlPO4HhR/5g4+3iFmJ1N7kFrxDp1/P8e3+9lcJsXckXC"
"tfFkQ9slogJqnaCsipgdrH4gQhicFAL5YqHw2AKfctZLSPpxhI3xISMV/lIQwZPW"
"I2221v9ALckokh5pW6dTK8x8yNUUye1zt5msWzqunfBjVYVUj5/LXu46hS99Mfxu"
"80ARJRyHsXIuHbMzwVXGqKVPZfKAdHBl5e3ElCjNwNy+Hgi09vS7PoAUhjCf/wbO"
"nQIDAQABo4IBczCCAW8wEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQUys4d"
"GAN3HhzzfFiymnCoCIAW9K4wDgYDVR0PAQH/BAQDAgGGMEgGA1UdIARBMD8wPQYP"
"KwYBBAGB7RgDAwMDBAQDMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vc3NsLnRydXN0"
"d2F2ZS5jb20vQ0EwMgYDVR0fBCswKTAnoCWgI4YhaHR0cDovL2NybC50cnVzdHdh"
"dmUuY29tL1NUQ0EuY3JsMGwGCCsGAQUFBwEBBGAwXjAlBggrBgEFBQcwAYYZaHR0"
"cDovL29jc3AudHJ1c3R3YXZlLmNvbTA1BggrBgEFBQcwAoYpaHR0cDovL3NzbC50"
"cnVzdHdhdmUuY29tL2lzc3VlcnMvU1RDQS5jcnQwHQYDVR0lBBYwFAYIKwYBBQUH"
"AwIGCCsGAQUFBwMBMB8GA1UdIwQYMBaAFEIythb6BP3+XUt6w/33TEAdWkOvMA0G"
"CSqGSIb3DQEBCwUAA4IBAQBN/Qb6x5VSQHIt/FqvXAkjfCI7jwXGzRoxThavUqv0"
"pJr1azXKv2L2ye9Sl4OCqiDHZ8ZJZ/Z19Ae1yvGTpG++0O5dWTHZ8qy/JSj17Mg1"
"vWiefVpJKtkYflzhC9B9/eB3QuKVTRKDqm1ZC9kbm3MWOwtnEOw4WURWS1X2v4Hv"
"2AJaXe11ZRqjx7/c6U50tYp3eVpRDCw7bOIXRffLG0oibhjHiEu8dgq7KkTSrx9m"
"QFS6T2x9GLiJKhj2mEfPuGr73TfBPyKsRBymOpG67LThKVnwoUfH3pghA6VkZp5Z"
"1Vm6AcDm+eelf5XPvZtNve4evLVPYueA3TmGyxXh1m2d"
"-----END CERTIFICATE-----";

static HTTP_REQ *phttpreqdata = NULL;
static HTTPMSG  httpmsg;

static const char *HTTP_STATUS = "HTTP/1.1 200 OK\r\n";
static const char *HTTP_201_STATUS = "HTTP/1.1 201 CREATED\r\n";	/*http 201回应，如机智云服务器就是这样的*/
static const char *HTTP_CONTENT = "Content-Length: ";

static Http_Header_Status s_status;
static const char *HTTP_CHUNKED = "Transfer-Encoding: chunked\r\n";
static const char *HTTP_CHUNKED_END = "0\r\n\r\n";

char *at_rsp_buff;
short *p_rsp_len;//changed by zhengchao
static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff);

#define HTTP_CONTEENT_BUFF_SIZE    (1024) //(5*1024)
static char *phttp_content = NULL;

static int http_index;
const int rsp_len = BUF_SIZE;

static Http_Header_Status s_status;

#ifdef __WIFI_WEATHER__
//static char http_content[5 * 1024];
#else
//static char http_content[3 * 1024];
#endif

static int http_index;
static int http_ncount;

static Boolean bEnter = FALSE;//正在获取天气的标志位，noted by zhengchao
Boolean bSuccess = FALSE; //获取天气成功的标志位， noted by zhengchao
struct weather_info_struct g_weather_info;
int weather_error_code;
static Boolean bTestMode = FALSE;	// 测试模式，经纬度从AT指令读取

static int http_func;

char pos_latitude[9] = "114.0550";//纬度数组 noted by zhengchao
char pos_longitude[9] = "22.5215";//经度数组 noted by zhengchao
char str_address[128];
Boolean bPosOk = FALSE;//获取位置成功的标志位，noted zhengchao

static int loop_count;
static int forecast_index;
static Boolean bOtaRunning;
static Boolean bOtaDone;

#ifdef EMBED_SYSTEM
static OsTaskHandle http_poll_weather_tast_handle;//http_poll_weather_tast句柄，zhengchao
static OsTaskHandle http_position_task_handle;
#endif

#ifdef WIN32
#define OS_MemAlloc	malloc
#define OS_MemFree	free
#endif

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


kal_int8 httpparse_init(int rsp_max_len)
{
#ifdef WIN32
	/*加载Winsock DLL*/
	WSADATA wsd;

	if (WSAStartup(MAKEWORD(2 , 2) , &wsd) != 0) 
	{
		printf("Winsock 初始化失败!\n");
		return -1;
	}
#endif
	DebugPrintf("httpparse_init");

	if (phttpreqdata == NULL)
	{
		printf("malloc phttpreqdata rsplen=%d!\n", rsp_max_len);
		phttpreqdata = (HTTP_REQ *)OS_MemAlloc(sizeof(HTTP_REQ));
		
		if(!phttpreqdata)
		{
			DebugPrintf("[error]call OS_MemAlloc!");
			return -1; 
		}
		memset(phttpreqdata, 0, sizeof(HTTP_REQ));
		phttpreqdata->httpsock = -1;

		phttpreqdata->httprsp = (U8 *)OS_MemAlloc(rsp_max_len);
		phttpreqdata->rsp_max_len = rsp_max_len;
		if (phttpreqdata->httprsp == NULL)
		{
			DebugPrintf("[error]call OS_MemAlloc(%d)!", rsp_max_len);
			return -1; 
		}

		memset(phttpreqdata->httprsp, 0, rsp_max_len);
	}
	
	if (phttp_content == NULL)
	{
		printf("malloc phttp_content!\n");
		phttp_content = (char *)OS_MemAlloc(/*HTTP_CONTEENT_BUFF_SIZE*/rsp_max_len);
		if (!phttp_content)
		{
			DebugPrintf("[error]call OS_MemAlloc(%d)!", rsp_max_len);
			return -1; 
		}
		
		memset(phttp_content, 0, sizeof(phttp_content));
	}

	return 0;
}



void httpparse_data_clear(void)
{
	HX_PRINT_DEBUG("##httpparse_data_clear!\n ");
	
	if (NULL != phttpreqdata)
	{
		printf("##clear phttpreqdata!\n ");
		OS_MemFree(phttpreqdata->httprsp);
		OS_MemFree(phttpreqdata);
		phttpreqdata = NULL;
	}
	
	if (NULL != phttp_content)
	{
		printf("##clear phttp_content!\n ");
		OS_MemFree(phttp_content);
		phttp_content = NULL;
	}

#ifdef WIN32
	WSACleanup();
#endif	
}


/*---------------------------------------------------------------------------*/
void httpparse_deinit()
{
	if (NULL != phttpreqdata)
	{
		memset(phttpreqdata, 0, sizeof(HTTP_REQ));
	}
	
	if (NULL != phttp_content)
	{
		memset(phttp_content, 0, HTTP_CONTEENT_BUFF_SIZE);
	}
}


static kal_bool soc_ip_check(char *asci_addr,
		kal_uint8 *ip_addr,
		kal_bool *ip_validity)
{
	kal_uint8 len;
	kal_uint8 i,octet_cnt;
	kal_int32 ip_digit;

	len = strlen(asci_addr);

	for (i = 0 ; i < len ; i++)
	{
		if (!isdigit((int)*(asci_addr+i)) && *(asci_addr+i) != '.' )
			return KAL_FALSE;
	}

	*ip_validity = KAL_TRUE;

	/* Extract the IP adress from character array */
	for (octet_cnt = 0 ; octet_cnt < 4 ; octet_cnt++)
	{
		if (*asci_addr == '\0') /* in case of "1.2.3." */
		{
			*ip_validity = KAL_FALSE;
			return KAL_FALSE;
		}

		ip_digit = atoi(asci_addr);

		if (ip_digit < 0 || ip_digit > 255)
		{
			*ip_validity = KAL_FALSE;
			return KAL_FALSE;
		}

		ip_addr[octet_cnt] = (kal_uint8)ip_digit;

		if (octet_cnt == 3)
			continue;

		asci_addr = strstr(asci_addr,".");
		if (asci_addr)
			asci_addr++;
		else
			break;
	}

	if (octet_cnt != 4)
	{
		return KAL_FALSE;
	}

	if (ip_addr[0] == 0 && ip_addr[1] == 0 && ip_addr[2] == 0 && ip_addr[3] == 0)
	{
		*ip_validity = KAL_FALSE;
	}

	return KAL_TRUE;
}


/*
 * 连接时如果没有错误发生，则进入block状态，等待网络端的connected消息再返回AT指令回应
 */
static Boolean http_tcp_connect_server()
{
	kal_uint8 ip[4];
	kal_bool is_valid = KAL_FALSE;
	char *hostname = (char *)phttpreqdata->hostname;
	int port = phttpreqdata->port;
	struct sockaddr_in server_addr;
	struct hostent *host;

	DebugPrintf("host = %s, port = %d", hostname, port);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	if (soc_ip_check(hostname, ip, &is_valid))
	{
		INT len;

		if (!is_valid)
		{
			DebugPrintf("ip is not valid");
			return FALSE;
		}	
		//inet_pton(AF_INET, hostname, &server_addr.sin_addr);
		len = sizeof(server_addr);
		WSAStringToAddress(hostname, AF_INET, NULL, &server_addr, &len);
	}
	else 
	{
		//char buf[32];
		char *p;

		if ((host = gethostbyname(hostname)) == NULL)
		{
			DebugPrintf("Get host name error");
			return FALSE;
		}

		server_addr.sin_addr = *((struct in_addr *)host->h_addr);	
		if ((p = inet_ntoa(server_addr.sin_addr)))
		{
			DebugPrintf("ipaddr:%s\n", p);
		}
		
	}
	if ((phttpreqdata->httpsock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		DebugPrintf("Socket Error");
		return FALSE;
	}

	server_addr.sin_port = htons(port);

	DebugPrintf("[debug]connect to web,ip:%04x,port:%d", server_addr.sin_addr.s_addr, server_addr.sin_port);
	if (connect(phttpreqdata->httpsock, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
	{
		DebugPrintf("Connect error");
#ifdef WIN32
		closesocket(phttpreqdata->httpsock);
#else
		close(phttpreqdata->httpsock);
#endif
		phttpreqdata->httpsock = -1;
		return FALSE;
	}

	DebugPrintf("connect ok!");

	return phttpreqdata->httpsock >= 0;
}

/*
 * 通过socket发http请求给服务器，fn为接收响应的回调函数
 */
int http_send_cmd(const char *url, int port, const char *req_str, void (*fn)(void *, void *))
{
	int len;
	int cmdlen = strlen((char *)url);
	U8 *ptr1, *ptr2, *end = (U8 *)url + cmdlen;
	int tmpint;

	if (phttpreqdata->httpstatus != HTTP_IDLE)
	{
		DebugPrintf("httpstatus:%d\n", phttpreqdata->httpstatus);
		return HTTPREQ_STILL_RUN;
	}

	phttpreqdata->httpstatus = HTTP_CONNECTING;

	phttpreqdata->rsplen = 0;
	phttpreqdata->httprsp[0] = 0;

	DebugPrintf("url=%s\n", url);
	ptr1 = (U8 *)url;
	if (memcmp(ptr1, "http://", 7) != 0)
	{
		DebugPrintf("1:%s\n", ptr1);

		len = strlen(url);
		if (len >= sizeof(phttpreqdata->hostname))
		{
			DebugPrintf("hostname too long:%d\r\n", len);
			return HTTPREQ_STILL_RUN;
		}

		strcpy((char *)phttpreqdata->hostname, (const char *)url);
		phttpreqdata->port = port;
	}
	else
	{
		//Go to get remote ip information
		ptr1 += 7;
		ptr2 = ptr1;
		while (ptr2 < end)
		{
			if ((*ptr2 == ':') || (*ptr2 == '/'))
			{
				if (ptr2 - ptr1 > sizeof(phttpreqdata->hostname) - 1)
				{
					return HTTPREQ_STILL_RUN;
				}
				else
				{
					memcpy(phttpreqdata->hostname, ptr1, ptr2 - ptr1);
					phttpreqdata->hostname[ptr2 - ptr1] = 0;
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
				DebugPrintf("2:%s, port:%d\n", ptr1, tmpint);
				return HTTPREQ_CMD_ERROR;
			}
			else
			{
				phttpreqdata->port = tmpint;
			}
		}
		else if (ptr1[0] == '/')
		{
			phttpreqdata->port = 80;
		}
		else
		{
			DebugPrintf("3:%s\n", ptr1);
			return HTTPREQ_CMD_ERROR;
		}
	}

	len = strlen(req_str);
	if (len >= sizeof(phttpreqdata->httpcmd))
	{
		DebugPrintf("httpcmd too long:%d\r\n", len);
		return HTTPREQ_STILL_RUN;
	}

	strcpy((char *)phttpreqdata->httpcmd, (const char *)req_str);
	phttpreqdata->cmdlen = len;

	if (fn)
	{
		phttpreqdata->callbackfn = fn;
	}

	s_status = HTTP_STATUS_INIT;

	if (http_tcp_connect_server())
	{
		http_do_action_rsp();
		return HTTPREQ_SUCC;
	}
	else
	{
		return HTTPREQ_CONN_ERROR;
	}
}

/* 
 * HTTP 返回字串处理
 */
static void http_do_action_rsp()
{
	if (phttpreqdata->cmdlen)
	{
		// qinjiangwei initialize the buffer and pointer. 2016/7/12
		phttpreqdata->rsplen = 0;
		memset(phttpreqdata->httprsp, 0, /*sizeof(phttpreqdata->httprsp)*/phttpreqdata->rsp_max_len);

		printf("cmd = %s, len = %d, @%d!\r\n", phttpreqdata->httpcmd, phttpreqdata->cmdlen, __LINE__);
		if (send(phttpreqdata->httpsock, phttpreqdata->httpcmd, phttpreqdata->cmdlen, 0) < 0)
		{
			DebugPrintf("send error!\r\n");
			close(phttpreqdata->httpsock);
			phttpreqdata->httpsock = -1;

			return;
		}


		for (;;)
		{
			int len;
			Http_Header_Status status;

			HX_PRINT_DEBUG("rsplen = %d, line=%d\r\n", phttpreqdata->rsplen, __LINE__);
			len = recv(phttpreqdata->httpsock, &(phttpreqdata->httprsp[phttpreqdata->rsplen]), 
						/*HTTPRSP_MAX*/phttpreqdata->rsp_max_len - 1 - phttpreqdata->rsplen, 0);
			phttpreqdata->rsplen += len;
			HX_PRINT_DEBUG("rsplen = %d, len = %d\r\n", phttpreqdata->rsplen, len);
			phttpreqdata->httprsp[phttpreqdata->rsplen] = '\0';
			if (phttpreqdata->rsplen > /*HTTPRSP_MAX*/phttpreqdata->rsp_max_len - 1)	/*直接等于判断会出问题*/
			{
				HX_PRINT_DEBUG("ERROR: rsplen = %d, BUFFER IS SMALL@LINE = %d\r\n", phttpreqdata->rsplen, __LINE__);
				status = HTTP_STATUS_ERROR;
				return;
			}
			else
			{
				status = httprsp_parse(phttpreqdata->httprsp, phttpreqdata->rsplen);
				HX_PRINT_DEBUG("[debug]call httprsp_parse return:%d", status);
			}

			if (len == 0 || status == HTTP_STATUS_ERROR || status == HTTP_STATUS_GET_ALL)
			{
#ifdef WIN32
				closesocket(phttpreqdata->httpsock);
#else
				close(phttpreqdata->httpsock);
#endif

				phttpreqdata->httpsock = -1;
				phttpreqdata->httpstatus = HTTP_IDLE;
				return;
			}
		}
	}
}


#if 0
/* 
 * HTTP 返回字串处理
 */
void https_do_action_rsp()
{
	if (phttpreqdata->cmdlen)
	{
		// qinjiangwei initialize the buffer and pointer. 2016/7/12
		phttpreqdata->rsplen = 0;
		memset(phttpreqdata->httprsp, 0, /*sizeof(phttpreqdata->httprsp)*/ phttpreqdata->rsp_max_len);

		//printf("error, cmd = %s, len = %d, @%d!\r\n", phttpreqdata->httpcmd, phttpreqdata->cmdlen, __LINE__);
		//if (hx_https_send_request(phttpreqdata->httpcmd, phttpreqdata->cmdlen) < 0)
		//{
		//	DebugPrintf("send error!\r\n");
		//	hx_https_disconnect();
		//	phttpreqdata->httpsock = -1;

		//	return;
		//}


		for (;;)
		{
			int len;
			Http_Header_Status status;

			HX_PRINT_DEBUG("rsplen = %d, line=%d\r\n", phttpreqdata->rsplen, __LINE__);
			len = hx_https_recv_data(&(phttpreqdata->httprsp[phttpreqdata->rsplen]), /*HTTPRSP_MAX*/ phttpreqdata->rsp_max_len - 1 - phttpreqdata->rsplen);
			
			HX_PRINT_DEBUG("rsplen = %d, len = %d\r\n", phttpreqdata->rsplen, len);
			if (len < 0)
			{
				status == HTTP_STATUS_ERROR;
				goto done;
			}
				
			phttpreqdata->rsplen += len;
	
			phttpreqdata->httprsp[phttpreqdata->rsplen] = '\0';
			if (phttpreqdata->rsplen > /*HTTPRSP_MAX*/phttpreqdata->rsp_max_len - 1)	/*直接等于判断会出问题*/
			{
				HX_PRINT_DEBUG("ERROR: rsplen = %d, BUFFER IS SMALL@LINE = %d\r\n", phttpreqdata->rsplen, __LINE__);
				status = HTTP_STATUS_ERROR;
				return;
			}
			else
			{
				status = httprsp_parse(phttpreqdata->httprsp, phttpreqdata->rsplen);
				HX_PRINT_DEBUG("[debug]call httprsp_parse return:%d", status);
			}
done:
			if (len == 0 || status == HTTP_STATUS_ERROR || status == HTTP_STATUS_GET_ALL)
			{
				hx_https_disconnect();
				phttpreqdata->httpsock = -1;
				phttpreqdata->httpstatus = HTTP_IDLE;
				return;
			}
		}
	}
}
#endif

/*******************************************************************
*作用
		通过SSL连接HTTP服务器
*参数
		无
*返回值
		TRUE 	- 连接成功
		FALSE  	- 连接失败
*其它说明
		2018/11/20 by qinjiangwei
********************************************************************/
static Boolean https_tcp_connect_server()
{
    kal_uint8 *pip;
	kal_uint8 ip[4];
	kal_bool is_valid = KAL_FALSE;
	char ip_adrress[32];
	char *hostname = (char *)phttpreqdata->hostname;
	int port = phttpreqdata->port;
	struct sockaddr_in server_addr;
	struct hostent *host;
	
	printf("host = %s, port = %d\r\n", hostname, port);

	if (soc_ip_check(hostname, ip, &is_valid))
	{
		INT len;

		if (!is_valid)
		{
			DebugPrintf("ip is not valid");
			return FALSE;
		}	

		//inet_pton(AF_INET, hostname, &server_addr.sin_addr);
		len = sizeof(server_addr);
		WSAStringToAddress(hostname, AF_INET, NULL, &server_addr, &len);
	}
	else 
	{
		char *p;

		if ((host = gethostbyname(hostname)) == NULL)
		{
			DebugPrintf("Get host name error");
			return FALSE;
		}

		server_addr.sin_addr = *((struct in_addr *)host->h_addr);	
		
		if ((p = inet_ntoa(server_addr.sin_addr)))
		{
			DebugPrintf("ipaddr:%s\n", p);
		}
		
	}

	return hx_https_connect_server(cert, privatekey, ip_adrress, port);
}

/*
 * 建立HTTPS请求头
 * https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyAEVo4VSyNOYB-4B4pYJulPb4k186ek4gw
 *  Content-Type: 
 */
int https_send_cmd(U8 *url, char *body, void (*fn)(void *))
{
#if 1
	int tmpint;
	int cmdlen = strlen((char *)url);
	U8 *ptr1, *ptr2, *end = url + cmdlen;
	U8 *buff_end;
	char buffer[128];

	if (phttpreqdata->httpstatus != HTTP_IDLE)
	{
		DebugPrintf("httpstatus:%d\n", phttpreqdata->httpstatus);
		return HTTPREQ_STILL_RUN;
	}

	//httpreqdata.httpstatus = HTTP_CONNECTING;
	ptr1 = url;
	phttpreqdata->rsplen = 0;
	phttpreqdata->httprsp[0] = 0;
	printf("11\n");
	if (memcmp(ptr1, "https://", 8) != 0)
	{
		DebugPrintf("1:%s\n", ptr1);
		return HTTPREQ_CMD_ERROR;
	}

	//Go to get remote ip information
	ptr1 += 8;
	ptr2 = ptr1;
	while (ptr2 < end)
	{
		if ((*ptr2 == ':') || (*ptr2 == '/'))
		{
			if (ptr2 - ptr1 > sizeof(phttpreqdata->hostname) - 1)
			{
				return HTTPREQ_STILL_RUN;
			}
			else
			{
				memcpy(phttpreqdata->hostname, ptr1, ptr2 - ptr1);
				phttpreqdata->hostname[ptr2 - ptr1] = 0;
			}
			ptr1 = ptr2;

			break;
		}

		ptr2++;
	}
	printf("22\n");
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
			phttpreqdata->port = tmpint;
		}
	}
	else if (ptr1[0] == '/')
	{
		phttpreqdata->port = 443;
	}
	else
	{
		DebugPrintf("3:%s\n", ptr1);
		return HTTPREQ_CMD_ERROR;
	}

	printf("33\n");
	phttpreqdata->callbackfn = fn;
	ptr2 = phttpreqdata->httpcmd;
	buff_end = ptr2 + sizeof(phttpreqdata->httpcmd);
	memcpy(ptr2 , POST_METHOD, POST_LEN);
	ptr2 += POST_LEN;
	memcpy(ptr2, ptr1, (end - ptr1));
	ptr2 += (end - ptr1);

	tmpint = strlen(" HTTP/1.1\r\nHost: ");
	memcpy(ptr2 , " HTTP/1.1\r\nHost: ", tmpint);
	ptr2 += tmpint;

	memcpy(ptr2 , phttpreqdata->hostname, strlen((char *)(phttpreqdata->hostname)));
	ptr2 += strlen((char *)(phttpreqdata->hostname));

	sprintf(buffer, "\r\nContent-Length: %d", strlen(body));
	tmpint = strlen(buffer);
	memcpy(ptr2 , buffer, tmpint);
	ptr2 += tmpint;

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nUser-Agent: ICOMMHTTP/1.0");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept: application/json");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nContent-Type: application/json\r\n\r\n");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	tmpint = strlen(body);
	if (ptr2 + tmpint > buff_end)
	{
		printf("ERROR: SEND BUFF IS SMALL@LINE = %d\r\n", __LINE__);
		return HTTPREQ_CMD_ERROR;
	}
	memcpy(ptr2 , body, tmpint);
	ptr2 += tmpint;

	phttpreqdata->cmdlen = ptr2 - phttpreqdata->httpcmd;
	ptr2 = 0;

	DebugPrintf("len:%d, cmd:'%s'", phttpreqdata->cmdlen, phttpreqdata->httpcmd);
	s_status = HTTP_STATUS_INIT;
//	httpreqdata.httpstatus = HTTP_CONNECTING;
	http_ssl_send_cmd(phttpreqdata);

#if 0
	if (https_tcp_connect_server())
	{
		https_do_action_rsp();

		return HTTPREQ_SUCC;
	}
	else
	{
		return HTTPREQ_CONN_ERROR;
	}
#endif

	#endif
	return 0;
}

/*
 * 对 URL host字段进行解析
 */
static int http_parse_url(char *url, char **path)
{
	int cmdlen = strlen(url);
	char *ptr1;
	char *end = url + cmdlen;
	char *ptr2;
	int tmpint;
	
	if (phttpreqdata->httpstatus != HTTP_IDLE)
	{
		DebugPrintf("httpstatus:%d\n", phttpreqdata->httpstatus);
		return HTTPREQ_STILL_RUN;
	}

	phttpreqdata->httpstatus = HTTP_CONNECTING;
	ptr1 = url;
	phttpreqdata->rsplen = 0;
	phttpreqdata->httprsp[0] = 0;
	if (memcmp(ptr1, "http://", 7) != 0)
	{
		DebugPrintf("1:%s\n", ptr1);
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}

	//Go to get remote ip information
	ptr1 += 7;
	ptr2 = ptr1;
	while (ptr2 < end)
	{
		if ((*ptr2 == ':') || (*ptr2 == '/'))
		{
			if (ptr2 - ptr1 > sizeof(phttpreqdata->hostname) - 1)
			{
				phttpreqdata->httpstatus = HTTP_IDLE;
				return HTTPREQ_STILL_RUN;
			}
			else
			{
				memcpy(phttpreqdata->hostname, ptr1, ptr2 - ptr1);
				phttpreqdata->hostname[ptr2 - ptr1] = 0;
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
			phttpreqdata->httpstatus = HTTP_IDLE;
			return HTTPREQ_CMD_ERROR;
		}
		else
		{
			phttpreqdata->port = tmpint;
		}
	}
	else if (ptr1[0] == '/')
	{
		phttpreqdata->port = 80;
	}
	else
	{
		DebugPrintf("3:%s\n", ptr1);
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}

	*path = ptr1;
	return HTTPREQ_SUCC;
}

/*
 * 建立HTTP请求头
 */
int SendGetHttpCmd(U8 *url, char *body, void (*fn)(void *, void *), Boolean bGzip)
{
#if 1//zc
	int tmpint;
	int cmdlen = strlen((char *)url);
	U8 *ptr1, *ptr2, *end = url + cmdlen;
	U8 *buff_end;
	int code;
	
#if 1
	if ((code = http_parse_url((S8 *)url, (char **)&ptr1)) != HTTPREQ_SUCC)
	{
		//printf("SendGetHttpCmdZC\n");
		return code;
	}
#else
	if (phttpreqdata->httpstatus != HTTP_IDLE)
	{
		DebugPrintf("httpstatus:%d\n", phttpreqdata->httpstatus);
		return HTTPREQ_STILL_RUN;
	}

	phttpreqdata->httpstatus = HTTP_CONNECTING;
	ptr1 = url;
	phttpreqdata->rsplen = 0;
	phttpreqdata->httprsp[0] = 0;
	if (memcmp(ptr1, "http://", 7) != 0)
	{
		DebugPrintf("1:%s\n", ptr1);
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}

	//Go to get remote ip information
	ptr1 += 7;
	ptr2 = ptr1;
	while (ptr2 < end)
	{
		if ((*ptr2 == ':') || (*ptr2 == '/'))
		{
			if (ptr2 - ptr1 > sizeof(phttpreqdata->hostname) - 1)
			{
				phttpreqdata->httpstatus = HTTP_IDLE;
				return HTTPREQ_STILL_RUN;
			}
			else
			{
				memcpy(phttpreqdata->hostname, ptr1, ptr2 - ptr1);
				phttpreqdata->hostname[ptr2 - ptr1] = 0;
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
			phttpreqdata->httpstatus = HTTP_IDLE;
			return HTTPREQ_CMD_ERROR;
		}
		else
		{
			phttpreqdata->port = tmpint;
		}
	}
	else if (ptr1[0] == '/')
	{
		phttpreqdata->port = 80;
	}
	else
	{
		DebugPrintf("3:%s\n", ptr1);
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}
#endif

	memset(phttpreqdata->httpcmd, 0, sizeof(phttpreqdata->httpcmd));
	phttpreqdata->callbackfn = fn;
	ptr2 = phttpreqdata->httpcmd;
	buff_end = ptr2 + sizeof(phttpreqdata->httpcmd);
	memcpy(ptr2 , GET_METHOD, GET_LEN);
	ptr2 += GET_LEN;
	memcpy(ptr2, ptr1, (end - ptr1));
	ptr2 += (end - ptr1);

	if (body)
	{
		tmpint = strlen(body);

		if (ptr2 + tmpint > buff_end)
		{
			DebugPrintf("ERROR: SEND BUFF IS SMALL@LINE = %d\r\n", __LINE__);
			return HTTPREQ_CMD_ERROR;
		}
		memcpy(ptr2 , body, tmpint);
		ptr2 += tmpint;
	}


	tmpint = strlen(" HTTP/1.1\r\nHost: ");
	memcpy(ptr2 , " HTTP/1.1\r\nHost: ", tmpint);
	ptr2 += tmpint;

	memcpy(ptr2 , phttpreqdata->hostname, strlen((char *)(phttpreqdata->hostname)));
	ptr2 += strlen((char *)(phttpreqdata->hostname));

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nUser-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36");
	if (!ptr2)
	{
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}

#if 1
	ptr2 = AppendHeader(ptr2, buff_end, "\r\nConnection: keep-alive");
	if (!ptr2)
	{
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nCache-Control: no-cache");
	if (!ptr2)
	{
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}
#endif

#if 0
	ptr2 = AppendHeader(ptr2, buff_end, "\r\nContent-Type: application/x-www-form-urlencoded");
	if (!ptr2)
	{
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}
#endif

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept: */*");
	if (!ptr2)
	{
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CMD_ERROR;
	}

	if (bGzip)
	{
		ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept-Encoding: gzip, deflate, sdch");
		if (!ptr2)
		{
			phttpreqdata->httpstatus = HTTP_IDLE;
			return HTTPREQ_CMD_ERROR;
		}

		ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept-Language: zh-CN,zh;q=0.8\r\n\r\n");
		if (!ptr2)
		{
			phttpreqdata->httpstatus = HTTP_IDLE;
			return HTTPREQ_CMD_ERROR;
		}
	}
	else
	{
		ptr2 = AppendHeader(ptr2, buff_end, "\r\n\r\n");
		if (!ptr2)
		{
			phttpreqdata->httpstatus = HTTP_IDLE;
			return HTTPREQ_CMD_ERROR;
		}
	}
	//printf("strlen:%d\n",strlen(phttpreqdata->httpcmd));
	//printf("##phttpreqdata->httpcmd ---------start-----------:\n%s\n--------end--------\n",phttpreqdata->httpcmd);//zc
	phttpreqdata->cmdlen = ptr2 - phttpreqdata->httpcmd;
	ptr2 = 0;

	s_status = HTTP_STATUS_INIT;

	if (http_tcp_connect_server())
	{
		http_do_action_rsp();
		return HTTPREQ_SUCC;
	}
	else
	{
		//weather_error_code = HX_ERROR_NO_SOCK;
		phttpreqdata->httpstatus = HTTP_IDLE;
		return HTTPREQ_CONN_ERROR;
	}
	#endif
}

static U8 *AppendHeader(U8 *ptr, U8 *end_ptr, const char *buff)
{
	int len;

	len = strlen(buff);
	if (ptr + len > end_ptr)
	{
		DebugPrintf("ERROR: SEND BUFF IS SMALL@LINE = %d\r\n", __LINE__);
		return NULL;
	}

	memcpy(ptr, buff, len);
	ptr += len;

	return ptr;
}

/*---------------------------------------------------------------------------*/
/*提取http的回应报文，状态从HTTP_STATUS_INIT开始，一直转下去*/
Http_Header_Status httprsp_parse(U8 *httprsp, U16 rsplen)
{
	static int contentlen = 0;
	static U8 * content_ptr;
	U8 *ptr, *end = httprsp + rsplen;
	int status_len = 0;
	int content_len = strlen(HTTP_CONTENT);
	//HX_PRINT_DEBUG("len:%d\n",strlen(httprsp));//zhengchao
	HX_PRINT_DEBUG("rsp:%s\n",httprsp);

	for (ptr = httprsp; ptr < end; )
	{
		switch (s_status)
		{
			case HTTP_STATUS_INIT:
				{
					if (0 == strncmp((char *)ptr, HTTP_STATUS, strlen(HTTP_STATUS)))
					{
						status_len = strlen(HTTP_STATUS);
					}
					else if (0 == strncmp((char *)ptr, HTTP_201_STATUS, strlen(HTTP_201_STATUS)))
					{
						status_len = strlen(HTTP_201_STATUS);
					}
					else
					{
						httpmsg.msgtype = HTTPREQ_RSP_ERROR;
						httpmsg.rsp = httprsp;
						httpmsg.rsplen = ptr - httprsp;

						if (phttpreqdata->callbackfn)
						{
							phttpreqdata->callbackfn(&httpmsg, phttpreqdata->param);
						}

						return HTTP_STATUS_ERROR;
					}

					s_status = HTTP_STATUS_GET_HEADER;
					ptr += status_len;

					break;
				}

			case HTTP_STATUS_GET_HEADER:
				{
					Boolean bGetNum = FALSE;
					char *p = (char *)ptr;
					const char *crlf = "\r\n\r\n";

					ptr = (U8 *)strstr((char *)p, HTTP_CONTENT);
					if (!ptr)
					{
						HX_PRINT_DEBUG("not found content, return\r\n");
						if ((ptr = (U8 *)strstr(p, HTTP_CHUNKED)))
						{
							ptr = (U8 *) strstr((char *)ptr, crlf);
							if (!ptr)
							{
								HX_PRINT_DEBUG("not found chunked, return\r\n");
								return HTTP_STATUS_GET_HEADER;
							}

							ptr += 4;

							p = strstr((char *)ptr, "\r\n");
							if (!p)
							{
								HX_PRINT_DEBUG("not get chunk length!\r\n");
							}
							//HX_PRINT_DEBUG("\n###ptr:%s\n",ptr);//zhengchao
							contentlen = get_chunk_size((char *)ptr, p);
							HX_PRINT_DEBUG("content len: %d\r\n", contentlen);
							//HX_PRINT_DEBUG("ptr:%.*s", contentlen, ptr);
							ptr = (U8 *)p + 2;
							content_ptr = ptr;	/*指向chunk内容*/
							s_status = HTTP_STATUS_CHUNKED;
							http_index = 0;

							continue;
						}

						HX_PRINT_DEBUG("not found content & chunked, return\r\n");
						return HTTP_STATUS_GET_HEADER;
					}

					ptr += content_len;
					contentlen = 0;
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

					HX_PRINT_DEBUG("content: %d, bGetNum = %d", contentlen, bGetNum);
					if (!bGetNum)
					{
						return HTTP_STATUS_GET_HEADER;
					}

					ptr = (U8 *) strstr((char *)p, crlf);		// http 头是否收完
					if (!ptr)
					{
						HX_PRINT_DEBUG("in header, return\r\n");		// 没有收完
						return HTTP_STATUS_GET_HEADER;
					}

					ptr += 4;
					content_ptr = ptr;
					http_index = 0;
					s_status = HTTP_STATUS_GET_CONTENT;

					break;
				}

			case HTTP_STATUS_GET_CONTENT:
				{
					int len = end - content_ptr;

					HX_PRINT_DEBUG("here len=%d, conlen=%d return\r\n", len, contentlen);
					HX_PRINT_DEBUG("rsplen = %d\r\n", phttpreqdata->rsplen);

					if (len < contentlen)
					{
						return HTTP_STATUS_GET_CONTENT;
					}

					HX_PRINT_DEBUG("here2\r\n");
					memcpy(&phttp_content[http_index], content_ptr, contentlen);		// 累积数据
					http_index += contentlen;

					phttp_content[http_index] = '\0';

					httpmsg.msgtype = HTTPREQ_RSP_DATA;
					httpmsg.rsp = (U8 *)phttp_content;
					httpmsg.rsplen = http_index;
					if (phttpreqdata->callbackfn)//调用回调
					{
						phttpreqdata->callbackfn(&httpmsg, phttpreqdata->param);
					}

					return HTTP_STATUS_GET_ALL;

				}

			case HTTP_STATUS_CHUNKED:
				{
					char *p;

					p = strstr((char *)content_ptr, HTTP_CHUNKED_END);
					if (!p)
					{
						HX_PRINT_DEBUG("more data chunk!\r\n");
						return HTTP_STATUS_CHUNKED;
					}

					memcpy(&phttp_content[http_index], content_ptr, p - (char *)content_ptr);
					http_index += (p - (char *)content_ptr);

					phttp_content[http_index] = '\0';

					httpmsg.msgtype = HTTPREQ_RSP_DATA;
					httpmsg.rsp = (U8 *)phttp_content;
					httpmsg.rsplen = http_index;
					HX_PRINT_DEBUG("##http_index:%d\n\n",http_index);//zc
					if (phttpreqdata->callbackfn)
					{
						phttpreqdata->callbackfn(&httpmsg, phttpreqdata->param);
					}

					return HTTP_STATUS_GET_ALL;
				}

			default:
				break;
		}
	}

	return HTTP_STATUS_ERROR;
}


static int a2b(char c)
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
		DebugPrintf("invalid char!\r\n");
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
		DebugPrintf("null ptr!\r\n");
		return 0;
	}

	n = end - begin;
	if (n % 2 != 0)
	{
		tmp[index++] = '0';
	}

	if (n > sizeof(tmp))
	{
		DebugPrintf("buff is small!\r\n");
		return 0;
	}

	memcpy(&tmp[index], begin, n);
	tmp[index + n] = '\0';

	DebugPrintf("tmp=%s\r\n", tmp);

	for (i = 0; i < index + n; i++)
	{
		v = v * 16 + a2b(tmp[i]);
	}

	return v;
}

#ifdef EMBED_SYSTEM
///////////
static int get_server_flag()
{
	U8 flag=0;

	//sbkj_get_srv_flag(&flag);//估计有用暂时屏蔽，因为用谷歌要用https，https没弄好

	if (flag & (1 << GOOGLE_FLAG))
	{
		return GOOGLE_FLAG;
	}
	else
	{
		return BAIDU_FLAG;
	}
}



							
Boolean http_check_ota_version()
{

	const char *url = "http://112.74.23.39:4000/getWifiInfo";
	int ret;
	#if 0//zhengchao 屏蔽
	uart_ota_status = HX_OTA_BEGIN;
	hx_atcmd_report_ota_status(HX_OTA_BEGIN);
	ret = SendGetHttpCmd((U8 *)url, NULL, http_get_ota_info, FALSE);
	if (ret != 0)
	{
		uart_ota_status = HX_OTA_ERROR;
		hx_atcmd_report_ota_status(HX_OTA_ERROR);
	}
#endif
	return ret;
}
#endif


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
	
	printf("begin:%s\n", begin);
	
	for (i = 0; (p = ptr_array[i]); i++)
	{
		if (strncmp(begin, p, strlen(p)) == 0)
		{
			return i;
		}
	}

	return -1;
}

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

static char *get_segment(char *ptr, char **p_begin, char **p_end, int c)
{
#if 1
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
	#endif
	return NULL;
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
		if (p_datetime->hour < 12)
		{
			p_datetime->hour += 12;
		}

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
	printf("m=%d\n", p_datetime->month);
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

	print_dt(p_datetime);
	
	return TRUE;
}


//2017-01-17T00:36:35Z
Boolean get_utc_datetime(char *pstr, struct dt_struct *p_datetime)
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

#ifdef EMBED_SYSTEM

void set_rsp_buff(char *buf, short *p_len)
{
	at_rsp_buff = buf;
	p_rsp_len = p_len;
}

static Boolean fill_at_rsp_string(Boolean bRsp)
{
	if (!bSuccess)
	{

		weather_error_code = HX_ERROR_JOIN_NETWORK_FAIL;

		make_weather_rsp_cb(weather_error_code, 0);
		return FALSE;
	}

	if (!at_rsp_buff)
	{
		printf("null ptr!\r\n");
		weather_error_code = HX_ERROR_NO_MEMORY;
		make_weather_rsp_cb(weather_error_code, 0);
		return TRUE;
	}

	if (http_func == GET_CITY)
	{
		int len = strlen(g_weather_info.city_name);
		int n;

		printf("rsp_buf1=%s\n", at_rsp_buff);

		*p_rsp_len = snprintf(at_rsp_buff, rsp_len, "%s,%s", g_weather_info.city_name, g_weather_info.region_name);
		weather_error_code = HX_ERROR_SUCCESS;
#if 0		
		if (len >= rsp_len)
		{
			len = rsp_len;
		}

		strncpy(at_rsp_buff, g_weather_info.city_name, len);
		n = strlen(g_weather_info.region_name) + 2;		//包含一个逗号和空格
		if (n + len >= rsp_len)
		{
			n = rsp_len - len;
		}

		strcat(at_rsp_buff, ",");
		strncat(at_rsp_buff, g_weather_info.region_name, n);
		weather_error_code = HX_ERROR_SUCCESS;
		*p_rsp_len = strlen(at_rsp_buff);
#endif

	//	printf("rsp_buf2=%s\n", at_rsp_buff);
		
		if (bRsp)
		{
			make_weather_rsp_cb(weather_error_code, *p_rsp_len);
		}
	}

	if (http_func == GET_WEATHER)
	{
		int n;
		int len;

		if (forecast_index == 0)
		{
			//printf("zhengchao test1\n");
			n = sprintf(at_rsp_buff, "%d,%d,", to_degree(g_weather_info.temperature), g_weather_info.humidity);
			//printf("at_rsp_buff1:%s\n",at_rsp_buff);
			//printf("hx_at_res_buf.pbuff:%s\n",hx_at_res_buf.pbuff);
			len = strlen(g_weather_info.condition);
			//printf("cond:%s\n", g_weather_info.condition);

			if (n + len >= rsp_len)
			{
				len = rsp_len - n;
			}

			strncat(at_rsp_buff, g_weather_info.condition, len);
			//printf("at_rsp_buff2:%s\n",at_rsp_buff);
			//printf("hx_at_res_buf.pbuff:%s\n",hx_at_res_buf.pbuff);
		}
		else
		{
			//printf("zhengchao test2\n");
			n = sprintf(at_rsp_buff, "%d,%d,", to_degree(g_weather_info.array[forecast_index - 1].temp_low), to_degree(g_weather_info.array[forecast_index - 1].temp_high));
			len = strlen(g_weather_info.array[forecast_index - 1].cond);

			if (n + len >= rsp_len)
			{
				len = rsp_len - n;
			}
			
			strncat(at_rsp_buff, g_weather_info.array[forecast_index - 1].cond, len);
		}
		
		weather_error_code = HX_ERROR_SUCCESS;
		*p_rsp_len = strlen(at_rsp_buff);
		//printf("p_rsp_len:%d\n",*p_rsp_len);
		//printf("rsp:%s, len=%d\n", at_rsp_buff, *p_rsp_len);
		
		if (bRsp)
		{
			make_weather_rsp_cb(weather_error_code, *p_rsp_len);
		}
	}
	return TRUE;
}

void set_forecast_index(int index)
{
	if (index < 0 || index > NDAYS_FORECAST)
	{
		printf("index over range, index=%d\r\n", index);
		index = 0;
	}
	
	forecast_index = index;
}

int At_get_weather(int index)
{
	printf("http_poll_weather_tast_handle:%d\n",http_poll_weather_tast_handle);
	
	if (bTestMode)
	{
		if (!(bPosOk = check_testmode_pos_ok()))
		{
			printf("lat/long isnot ok!\n");
			return IS_REQUESTING;//相当于请求失败
		}
	}

	if (bSuccess)//已经获取成功
	{
		http_func = GET_WEATHER;
		loop_count = 0;
		printf("had requested!\n");
		set_forecast_index(index);
		fill_at_rsp_string(FALSE);
		return HAD_REQUESTED;
	}

	if (!bPosOk)
	{
		if (http_position_task_handle == NULL)
		{
			http_func = GET_WEATHER;
			set_forecast_index(index);
			OS_TaskCreate(http_get_position_task, "http_position", HTTP_SSL_STACK_SIZE, NULL, POLL_WEATHER_PRIO, &http_position_task_handle);
			return HAVENT_REQUEST;
		}
		else
		{
			return IS_REQUESTING;
		}
	}
	
	if (http_poll_weather_tast_handle == NULL)//没有任务在运行
	{
		http_func = GET_WEATHER;
		set_forecast_index(index);
		OS_TaskCreate(http_poll_weather_tast, "http_poll_weather", POLL_WEATHER_STACK_SIZE, NULL, POLL_WEATHER_PRIO, &http_poll_weather_tast_handle);
		return HAVENT_REQUEST;
	}
	else//任务运行中
	{
		return IS_REQUESTING;
	}
}

int At_get_city()
{
	if (bTestMode)
	{
		if (!(bPosOk = check_testmode_pos_ok()))
		{
			printf("lat/long isnot ok!\n");
			return IS_REQUESTING;//相当于请求失败
		}
	}

	if (bSuccess)//已经获取成功
	{
		http_func = GET_CITY;
		printf("had requested!\n");
		fill_at_rsp_string(FALSE);		
		return HAD_REQUESTED;
	}
	
	if (!bPosOk)
	{
		if (http_position_task_handle == NULL)
		{
			http_func = GET_CITY;
			OS_TaskCreate(http_get_position_task, "http_position", HTTP_SSL_STACK_SIZE, NULL, POLL_WEATHER_PRIO, &http_position_task_handle);
			return HAVENT_REQUEST;
		}
		else
		{
			return IS_REQUESTING;
		}
	}
	
	if (http_poll_weather_tast_handle == NULL)//没有任务在运行
	{
		http_func = GET_CITY;
		OS_TaskCreate(http_poll_weather_tast, "http_poll_weather", POLL_WEATHER_STACK_SIZE, NULL, POLL_WEATHER_PRIO, &http_poll_weather_tast_handle);
		return HAVENT_REQUEST;
	}
	else//任务运行中
	{
		return IS_REQUESTING;
	}
}

#endif

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

	v += dt->day;
	
	v *= 24;
	v += dt->hour;

	return v;
}

static void print_dt(struct dt_struct *utc)
{
	printf("%d-%d-%d %d:%d:%d\n", utc->year, utc->month, utc->day, utc->hour, utc->minute, utc->second);
}


static int calc_time_region(struct dt_struct *utc, struct dt_struct *local_dt)
{
	int h1, h2;
	
	print_dt(utc);
	print_dt(local_dt);
	h1 = get_hours(utc);
	h2 = get_hours(local_dt);

	return h2 - h1;
}


/*
 * 100.23 return 10023, 两个有效数字
 */
static int get_lng_integer(char *lng)
{
	int n;
	int frac;
	char *p;
	int i;
	
	n = atoi(lng);

	for (p = lng; *p && *p != '.'; p++)
	{
		
	}

	if (!*p)
	{
		return n * 100;
	}

	p++;
	
	for (i = 0, frac = 0; i < 2; i++)
	{
		if (!*p)
		{
			break;
		}

		if (*p < '0' || *p > '9')
		{
			break;
		}

		frac = frac * 10 + *p - '0';
	}

	if (n < 0)
	{
		frac = -frac;
	}

	return n * 100 + frac;
}


static int calc_time_zone(char *lng)
{
	int v;
	int n;
	int zone;
	Boolean neg = FALSE;
	
	v = get_lng_integer(lng);

	if (v < 0)
	{
		neg = TRUE;
		v = -v;
	}

	n = v / 15;
	zone = n / 100;
	if (n % 100 >= 50)
	{
		zone++;
	}

	if (neg)
	{
		return -zone;
	}
	else
	{
		return zone;
	}
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


int to_degree(int f)
{
	return 5 * (f - 32) / 9; 
}


void print_weather_info(struct weather_info_struct *p_weather)
{
	printf("cityname = %s, %s\r\n", p_weather->city_name, p_weather->region_name);
	printf("current date: %d-%02d-%02d %02d:%02d:%02d, %d\r\n", p_weather->local_dt.year, p_weather->local_dt.month + 1, p_weather->local_dt.day,
			p_weather->local_dt.hour, p_weather->local_dt.minute, p_weather->local_dt.second, p_weather->local_dt.offset);
#ifdef WIN32
	print_windows_system_time();
#endif
	printf("weather: temp = %d, high = %d, low  = %d, %s\r\n", to_degree(p_weather->temperature), to_degree(p_weather->tmp_high), to_degree(p_weather->tmp_low),
			p_weather->condition);
	
}

#ifdef EMBED_SYSTEM
static void check_time_zone_changed(int tzone)
{
	int zone = 0;

	sbkj_get_time_zone((U8 *)&zone);		// 获取旧的时区
	printf("savedzone=%d,tzone=%d\n", zone, tzone);
	if (zone != 0xff)
	{
		if (zone != (tzone + 12))			// 时区改变了
		{
			U8 v = 0xff;
			sbkj_update_user_time_zone(&v);		// 清楚用户设定的时区
		}
	}
}
#endif

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
	//printf("http->rsplen:%d!\n",http->rsplen);//zhengchao
	#if 0//查看收到什么东西
		int cnt=0;
		printf("http_get_weather recieve :\n");
		for(;cnt<http->rsplen;cnt++)
		{
			printf("%c",*(http->rsp+cnt));
		}
	#endif
	pRoot = cJSON_Parse(http->rsp);
	if (!pRoot)
	{
		printf("json format error!\r\n");
#ifdef EMBED_SYSTEM
		weather_error_code = HX_ERROR_JSON_FORMAT;
#endif
		return;
	}
	printf("get json ok\n");//zc

	pQuery = cJSON_GetObjectItem(pRoot, "query");
	if (!pQuery)
	{
#ifdef EMBED_SYSTEM
		weather_error_code = HX_ERROR_NO_INFO;
#endif
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
#ifdef EMBED_SYSTEM
		weather_error_code = HX_ERROR_NO_INFO;
#endif
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
	
	g_weather_info.local_dt.offset = calc_time_region(&utc_dt, &g_weather_info.local_dt);
#ifdef EMBED_SYSTEM
	int v = calc_time_zone(pos_longitude); 
	check_time_zone_changed(/*g_weather_info.local_dt.offset*/v);
	
	U8 v1 = (v + 12) & 0xff;
	sbkj_update_time_zone(&v1);
#endif

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

		// 后天天气预报
	pItem = cJSON_GetArrayItem(pArray, 3);
	fill_forecast_info(&g_weather_info.array[2], pItem);

	bSuccess = TRUE;
	print_weather_info(&g_weather_info);
done:
	cJSON_Delete(pRoot);

	bEnter = FALSE;

}


#define YQL_STR "http://query.yahooapis.com/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20woeid%20in%20(SELECT%20woeid%20FROM%20geo.places%20WHERE%20text=%22"
void http_get_weather_from_yahoo()
{
	static char url_str[256 + 64];
	const char post_str[] = "%22)&format=json";
	char addr[64 + 16];
#if 0
	/*below place changed,just for test*/
	/*pos Basel,Sweden*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"47.554798");
	strcpy(pos_longitude,"7.581746");

	/*pos berlin,German*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"52.512220");
	strcpy(pos_longitude,"13.326727");

	/*pos hamburg,German*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"53.538096");
	strcpy(pos_longitude,"9.875498");

	/*pos Lower saxony hannover,German*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"52.407694");
	strcpy(pos_longitude,"9.657280");

	/*pos Urümqi XinJiang,China*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"43.742540");
	strcpy(pos_longitude,"87.209784");

	/*pos DaLian,China*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"38.927579");
	strcpy(pos_longitude,"121.491119");

	/*pos BeiJing,China*/
	memset(pos_latitude,0,sizeof(pos_latitude));
	memset(pos_longitude,0,sizeof(pos_longitude));
	strcpy(pos_latitude,"39.884963");
	strcpy(pos_longitude,"116.406240");
#endif	

	
	sprintf(addr, "(%s,%s)", pos_latitude, pos_longitude);

	strcpy(url_str, YQL_STR);
	strcat(url_str, addr);
	strcat(url_str, post_str);
	
	if (bSuccess)
	{
		print_weather_info(&g_weather_info);
		return;
	}

	if (!bPosOk)
	{
		printf("not pos!\r\n");
		return;
	}

	if (bEnter)
	{
		printf("busy now!\r\n");
		return;
	}
	HX_PRINT_DEBUG("url=%s\r\n", url_str);
	bEnter = TRUE;

	httpparse_init(YAHOO_HTTP_RSP_MAX);
	if (SendGetHttpCmd((U8 *)url_str, NULL, http_get_weather, TRUE) != HTTPREQ_SUCC)
	{
		bEnter = FALSE;
	}
}

static void http_get_long_latitude(void *pmsg, void *param)
{
	HTTPMSG *http = (HTTPMSG *)pmsg;
	cJSON *pRoot;
	cJSON* pItem;
	cJSON* pPoint;
	cJSON* pAddr;
	cJSON* pContent;

	http->rsp[http->rsplen] = '\0';
	//HX_PRINT_DEBUG("http->rsplen:%d!\n",http->rsplen);//zhengchao
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
	//http_get_weather_from_yahoo();
	bPosOk = TRUE;

#ifdef EMBED_SYSTEM
	mark_server_flag(BAIDU_FLAG);
#endif

done:
	cJSON_Delete(pRoot);
}

static void https_get_long_latitude(void *pmsg)
{
	HTTPMSG *http = (HTTPMSG *)pmsg;
	cJSON *pRoot;
	cJSON* pItem;
	cJSON* pLoc;

	http->rsp[http->rsplen] = '\0';

	printf("msg:'%s'\r\n", http->rsp);

	if (bTestMode)
	{
		printf("testmode!\n");
		return;
	}

	pRoot = cJSON_Parse(http->rsp);
	if (!pRoot)
	{
		printf("json format error!\r\n");
		return;
	}

	pLoc = cJSON_GetObjectItem(pRoot, "location");
	if (!pLoc)
	{
		printf("can not get content\r\n");
		goto done;
	}

	pItem = cJSON_GetObjectItem(pLoc, "lng");
	
	if (pItem->valuedouble != 0)
	{
		sprintf(pos_longitude, "%.4f", pItem->valuedouble);
	}
	else
	{
		strncpy(pos_longitude, pItem->valuestring, sizeof(pos_longitude) - 1);
	}
	trim_to_digits(pos_longitude, 4);

	printf("x = %s,", pos_longitude);
	
	pItem = cJSON_GetObjectItem(pLoc, "lat");
	

	if (pItem->valuedouble != 0)
	{
		sprintf(pos_latitude, "%.4f", pItem->valuedouble);
	}
	else
	{
		strncpy(pos_latitude, pItem->valuestring, sizeof(pos_latitude) - 1);
	}
	trim_to_digits(pos_latitude, 4);
	printf("y = %s\r\n", pos_latitude);
	//http_get_weather_from_yahoo();
	bPosOk = TRUE;
#ifdef EMBED_SYSTEM
	mark_server_flag(GOOGLE_FLAG);
#endif	
done:
	cJSON_Delete(pRoot);
}


FUNC_DLL_EXPORT Boolean http_get_city_name()
{
	const char *url = "http://api.map.baidu.com/location/ip?ak=jGBQpNiWLBjKaeEBnWzTS17HOnopA39M&coor=bd09ll";
	//const char *url = "http://api.map.baidu.com/location/ip?ak=jGBQpNiWLBjKaeEBnWzTS17HOnopA39Mxxxxx&coor=bd09ll"; // for testing googleapis.
	//const char *url = "http://180.97.93.23:80/location/ip?ak=jGBQpNiWLBjKaeEBnWzTS17HOnopA39M&coor=bd09ll";
	Boolean ret;
	
	HX_PRINT_DEBUG("\n\n##http_get_city_name\n");//zhengchao
	
	if (bPosOk)
	{
		printf("pos ok!\r\n");
		return FALSE;
	}
	
	httpparse_init(GOOGLE_GEO_HTTP_RSP_MAX);//added by zc
	ret = SendGetHttpCmd((U8 *)url, NULL, http_get_long_latitude, FALSE) == 0;
	httpparse_data_clear();

	return ret;
}

FUNC_DLL_EXPORT Boolean https_get_city_name()
{
	const char *url = "https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyAEVo4VSyNOYB-4B4pYJulPb4k186ek4gw";
	//const char *url = "https://blackboxapi.azurewebsites.net/api/device/9D1E9747-2D84-4E92-A1E7-C613FA1D3096/shop";
	//const char *url = "https://192.168.1.213/geolocation/";
	char *body;
	cJSON *root;
	cJSON *parray;
	printf("\n\n##https_get_city_name\n");//zc
	if (bPosOk)
	{
		printf("pos ok!\r\n");
		return FALSE;
	}

	root = cJSON_CreateObject();
	if (NULL == root) 
	{
		return FALSE;
	}
	
	cJSON_AddStringToObject(root, "considerIp", "true");
	
	parray = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "cellTowers", parray);

	parray = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "wifiAccessPoints", parray);
	
	body = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	printf("body=%s\n", body);
	if (body)
	{
		httpparse_init(GOOGLE_GEO_HTTP_RSP_MAX);
		https_send_cmd((U8 *)url, body, https_get_long_latitude);
		free(body);
		httpparse_data_clear();
		return TRUE;
	}

	return FALSE;
}

#ifdef EMBED_SYSTEM
void http_poll_weather_tast(void *pdata)
{
	int count = 0;
	
	if (sbkj_get_ota_flag())
	{
		int status = sbkj_get_ota_status();
		printf("otast=%d\n", status);
		if (status == HX_OTA_RUNNING)
		{
			status = HX_OTA_END;
		}
		
		hx_atcmd_report_ota_status(status);
		uart_ota_status = status;
		
		U8 flag = 0;
		sbkj_update_ota_flag(&flag);
	}
	
	while (1)
	{		
		if (get_wifi_status() == 0) //rtos
		//没连上直接返回错误
		{
			weather_error_code = HX_ERROR_JOIN_NETWORK_FAIL;
			make_weather_rsp_cb(weather_error_code, 0);
			break;
		}
		
		if (count++ >= MAX_LOOP_COUNT)
		{
			bEnter = FALSE;
			weather_error_code = HX_ERROR_JOIN_NETWORK_FAIL;
			break;
		}

		if (http_func == CHECK_OTA)
		{
			if (!bOtaDone)
			{
				//printf("zc 3!\n");
				if (http_check_ota_version())			// 先检查OTA版本
				{
					WAIT_HTTP_IDLE();
				}
			}
			else
			{
				break;
			}
		}
		else if (http_func == GET_WEATHER || \
				http_func == GET_CITY)
		{
			http_get_weather_from_yahoo();

			if (bSuccess)
			{
				break;
			}
		}

		OS_MsDelay(5000);
	}

	fill_at_rsp_string(TRUE);

	http_poll_weather_tast_handle = NULL;
	httpparse_data_clear(); 
	
	OS_TaskDelete(NULL);
}

static OsTimer g_delay_timer;			// 让 geolocation 和 yahoo weather 错开运行, 节约内存

/*******************************************************************
*作用
      地理定位任务
      1. 由于googleapis定位需要占用较大的stack, rsp buffer需要的少，特点和天气预报正好相反，因此提出单独一个线程
*参数
     void *pdata - 暂时不用
*返回值
     无
*其它说明
	2018/11/26 by qinjiangwei
********************************************************************/
static void http_get_position_task(void *pdata)
{
	int count = 0;
	
	while (!bPosOk)
	{
		if (get_wifi_status() == 0) //rtos
		//没连上直接返回错误
		{
			weather_error_code = HX_ERROR_JOIN_NETWORK_FAIL;
			make_weather_rsp_cb(weather_error_code, 0);
			httpparse_data_clear();
			OS_TaskDelete(NULL);
			break;
		}
		
		if (count++ >= MAX_LOOP_COUNT)
		{
			bEnter = FALSE;
			weather_error_code = HX_ERROR_JOIN_NETWORK_FAIL;
			fill_at_rsp_string(TRUE);
			httpparse_data_clear();
			OS_TaskDelete(NULL);
			break;
		}

		if (get_server_flag() == GOOGLE_FLAG)
		{
			if (https_get_city_name())			// 先获取经纬度
			{
				WAIT_HTTP_IDLE();
				
				if (!bPosOk)
				{
					if (http_get_city_name())
					{
						WAIT_HTTP_IDLE();
					}
				}
			}
		}
		else
		{
			if (http_get_city_name())			// 先获取经纬度
			{
				WAIT_HTTP_IDLE();
				if (!bPosOk)
				{
					if (https_get_city_name())
					{
						WAIT_HTTP_IDLE();
					}
				}
			}
		}
	
		//printf("bPosOk flase\n");
		OS_MsDelay(500);
	}
	
	httpparse_data_clear();
	if (g_delay_timer == NULL)
	{
		OS_TimerCreate(&g_delay_timer, 300, 0, (void *)0, start_weather_task);		// 异步等待ssl线程释放完堆栈内存
	}
	
	OS_TimerStart(g_delay_timer);
	OS_TaskDelete(NULL);
}


/*******************************************************************
*作用
     启动天气预报任务
      1. 由于googleapis定位需要占用较大的stack, rsp buffer需要的少，特点和天气预报正好相反，因此提出单独一个线程
*参数
     OsTimer t - 暂时不用
*返回值
     无
*其它说明
	2018/11/26 by qinjiangwei
********************************************************************/
static void start_weather_task(OsTimer t)
{
	printf("bPosOk=%d, bSuccess=%d, func=%d\n", bPosOk, bSuccess, http_func);
	
	if (bPosOk && !bSuccess)
	{	
		if (http_func == GET_CITY || http_func == GET_WEATHER)
		{
			if (http_poll_weather_tast_handle == NULL)
			{
				OS_TaskCreate(http_poll_weather_tast, "http_poll_weather", POLL_WEATHER_STACK_SIZE, NULL, POLL_WEATHER_PRIO, &http_poll_weather_tast_handle);
			}
			else
			{
				printf("weather is busying..!\n");
			}
		}
	}
}

static int mark_server_flag(int index)
{
	U8 flag;

	flag = 0;
	flag |= 1 << index;

	sbkj_update_srv_flag(&flag);//可能有用

	return 0;
}
#endif

//__hx_at_rsp_ok_help(msg.MsgCmd, NULL, HX_ERROR_JOIN_NETWORK_FAIL, &hx_at_res_buf);

/*
 * 建立HTTP请求头
 * https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyAEVo4VSyNOYB-4B4pYJulPb4k186ek4gw
 *  Content-Type: 
 */
int http_send_post_cmd(U8 *url, char *body, void (*fn)(void *))
{
	int tmpint;
	int cmdlen = strlen((char *)url);
	U8 *ptr1, *ptr2, *end = url + cmdlen;
	U8 *buff_end;
	char buffer[128];

	if (phttpreqdata->httpstatus != HTTP_IDLE)
	{
		DebugPrintf("httpstatus:%d\n", phttpreqdata->httpstatus);
		return HTTPREQ_STILL_RUN;
	}

#define HTTP_SCHEMA	"http://"
#define HTTP_SCHEMA_LEN	(strlen(HTTP_SCHEMA))
	ptr1 = url;
	phttpreqdata->rsplen = 0;
	phttpreqdata->httprsp[0] = 0;
	printf("11\n");
	if (memcmp(ptr1, HTTP_SCHEMA, HTTP_SCHEMA_LEN) != 0)
	{
		DebugPrintf("1:%s\n", ptr1);
		return HTTPREQ_CMD_ERROR;
	}

	//Go to get remote ip information
	ptr1 += HTTP_SCHEMA_LEN;
	ptr2 = ptr1;
	while (ptr2 < end)
	{
		if ((*ptr2 == ':') || (*ptr2 == '/'))
		{
			if (ptr2 - ptr1 > sizeof(phttpreqdata->hostname) - 1)
			{
				return HTTPREQ_STILL_RUN;
			}
			else
			{
				memcpy(phttpreqdata->hostname, ptr1, ptr2 - ptr1);
				phttpreqdata->hostname[ptr2 - ptr1] = 0;
			}
			ptr1 = ptr2;

			break;
		}

		ptr2++;
	}
	printf("22\n");
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
			phttpreqdata->port = tmpint;
		}
	}
	else if (ptr1[0] == '/')
	{
		phttpreqdata->port = 80;
	}
	else
	{
		DebugPrintf("3:%s\n", ptr1);
		return HTTPREQ_CMD_ERROR;
	}

	printf("33\n");
	phttpreqdata->callbackfn = fn;
	ptr2 = phttpreqdata->httpcmd;
	buff_end = ptr2 + sizeof(phttpreqdata->httpcmd);
	memcpy(ptr2 , POST_METHOD, POST_LEN);
	ptr2 += POST_LEN;
	memcpy(ptr2, ptr1, (end - ptr1));
	ptr2 += (end - ptr1);

	tmpint = strlen(" HTTP/1.1\r\nHost: ");
	memcpy(ptr2 , " HTTP/1.1\r\nHost: ", tmpint);
	ptr2 += tmpint;

	memcpy(ptr2 , phttpreqdata->hostname, strlen((char *)(phttpreqdata->hostname)));
	ptr2 += strlen((char *)(phttpreqdata->hostname));

	sprintf(buffer, "\r\nContent-Length: %d", strlen(body));
	tmpint = strlen(buffer);
	memcpy(ptr2 , buffer, tmpint);
	ptr2 += tmpint;

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nUser-Agent: ICOMMHTTP/1.0");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nAccept: application/json");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	ptr2 = AppendHeader(ptr2, buff_end, "\r\nContent-Type: application/json\r\n\r\n");
	if (!ptr2)
	{
		return HTTPREQ_CMD_ERROR;
	}

	tmpint = strlen(body);
	if (ptr2 + tmpint > buff_end)
	{
		printf("ERROR: SEND BUFF IS SMALL@LINE = %d\r\n", __LINE__);
		return HTTPREQ_CMD_ERROR;
	}
	memcpy(ptr2 , body, tmpint);
	ptr2 += tmpint;

	phttpreqdata->cmdlen = ptr2 - phttpreqdata->httpcmd;
	ptr2 = 0;

	DebugPrintf("len:%d, cmd:'%s'", phttpreqdata->cmdlen, phttpreqdata->httpcmd);
	s_status = HTTP_STATUS_INIT;

	if (http_tcp_connect_server())
	{
		http_do_action_rsp();

		return HTTPREQ_SUCC;
	}
	else
	{
		return HTTPREQ_CONN_ERROR;
	}
}
