#ifndef __HTTPPARSE_H__
#define __HTTPPARSE_H__

#include <stdio.h>
#ifdef WIN32
#include "windows.h"
#endif
#include "data_buff.h"
#include "mywin.h"
#include "ServerCmd.h"
#include	"cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

#undef DebugPrintf
#define DebugPrintf(format, ...)				printf("[%s:%d]:"format"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)


#define HX_PRINT_DEBUG DebugPrintf


#ifdef WIN32
/* 数据类型 */
//typedef UCHAR	       		U8;
//typedef unsigned short      U16;
//typedef unsigned int        U32;
#ifndef U16
#define U16 unsigned short
#endif

#ifndef U32
#define U32 unsigned int
#endif


typedef unsigned long long  U64;
typedef char             	S8;
typedef short int           S16;
typedef int             	S32;
typedef long long           S64;
#else
/* 数据类型 */
typedef uint8_t	       			U8;
typedef uint16_t      			U16;
typedef uint32_t        		U32;
typedef uint64_t      			U64;
typedef int8_t             		S8;
typedef int16_t           		S16;
typedef int32_t             	S32;
typedef int64_t       			S64;
#endif


#ifndef Boolean
#define Boolean 				uint8_t
#endif

#define kal_bool 				Boolean

#define kal_uint8				unsigned char
#define	kal_int8				char
#define kal_int32				int

#ifndef KAL_TRUE
#define KAL_TRUE 				1
#endif
#ifndef KAL_FALSE
#define KAL_FALSE 				0
#endif

#define HAVENT_REQUEST 0//未请求过
#define IS_REQUESTING     1//正在请求
#define HAD_REQUESTED  2//请求过了


#define HTTPCMD_MAX (512 + 512 + 2048 + 5 * 1024) /*256*/
//#define HTTPRSP_MAX (512 + 512) // + 2048) //+ 512 + 2048)

#define GOOGLE_GEO_HTTP_RSP_MAX 	(512 + 512 + 1024)						// GeoLocation http rsp 长度
#define YAHOO_HTTP_RSP_MAX 			(512 + 512 + 2048 + 512 + 2048)	// Yahoo 天气返回长度

enum 
{
    HTTPREQ_SUCC        = 0,
    HTTPREQ_STILL_RUN,
    HTTPREQ_CMD_ERROR,
    HTTPREQ_CONN_ERROR,
    HTTPREQ_RSP_DATA,
    HTTPREQ_RSP_TIMEOUT,
    HTTPREQ_RSP_ERROR,
};

enum
{
    HTTP_IDLE        = 0,
    HTTP_CONNECTING,
    HTTP_SENDDATA,
    HTTP_WAITRESPONSE,
};

typedef struct t_HTTPMSG
{
    U8 msgtype;
    U8 *rsp;
    U16 rsplen;
}HTTPMSG;


typedef struct t_HTTP_REQ
{
    int httpsock;
    U8 httpstatus;
    void (*callbackfn)(void *, void *);
	void *param;
    U8 hostname[16 + 128];			// qinjiangwei 2017/1/13
    U16 port;
    U8 httpcmd[HTTPCMD_MAX];
    U16 cmdlen;
    //U8 httprsp[HTTPRSP_MAX];
    U8 *httprsp;
    U16 rsplen;
	U16 rsp_max_len;
	
} HTTP_REQ;

#ifndef HTTP_HEADER_STATUS
#define HTTP_HEADER_STATUS
typedef enum {
	HTTP_STATUS_INIT = 0,
	HTTP_STATUS_GET_HEADER,
	HTTP_STATUS_GET_CONTENT,
	HTTP_STATUS_GET_ALL,
	HTTP_STATUS_CHUNKED,
	HTTP_STATUS_ERROR
} Http_Header_Status;
#endif


typedef struct
{
	char *key;
	char *value;
} pair_t;

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
struct dt_struct {
	kal_uint16 year;
	kal_uint16 month;
	kal_uint16 day;
	kal_uint16 hour;
	kal_uint16 minute;
	kal_uint16 second;
	kal_int16 offset;		// 时区 [-12, 12]
};

#define NDAYS_FORECAST 3

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


kal_int8 httpparse_init(int rsp_max_len);

void httpparse_data_clear(void);

void httpparse_deinit();
int httprequest_send(U8 *httpcmd, U16 cmdlen, void (*fn)(void *));
int http_send_cmd(const char *hostname, int port, const char *req_str, void (*fn)(void *, void *));

char *BuildPostBody(pair_t *array);
int SendPostHttpCmd(U8 *url, char *body, void (*fn)(void *, void *));
int SendGetHttpCmd(U8 *url, char *body, void (*fn)(void *, void *), Boolean bGzip);
int SendPostHttpCmd_2(U8 *url, char *body, pair_t *header, void (*fn)(void *, void *), void *parma);
int https_send_cmd(U8 *url, char *body, void (*fn)(void *));

FUNC_DLL_EXPORT Boolean http_get_city_name();

FUNC_DLL_EXPORT Boolean https_get_city_name();

void http_poll_weather_tast(void *pdata);//合并httpparse_app

/*******************************************************************
*作用
      通过SSL发送HTTP请求, 并处理返回数据
*参数
     HTTP_REQ * http_req - HTTP请求数据
*返回值
     0 - 请求成功
     1 - 请求失败
*其它说明
	2018/11/26 by qinjiangwei
********************************************************************/
int http_ssl_send_cmd(HTTP_REQ * http_req);

Http_Header_Status httprsp_parse(U8 *httprsp, U16 rsplen);

/*
 * 建立HTTP请求头
 * https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyAEVo4VSyNOYB-4B4pYJulPb4k186ek4gw
 *  Content-Type: 
 */
int http_send_post_cmd(U8 *url, char *body, void (*fn)(void *));


FUNC_DLL_EXPORT Boolean http_get_city_name();
typedef void (*fun_http_parse_header_t)(const char *pheader);
extern fun_http_parse_header_t fn_parse_header;

extern Boolean bIpOk;
extern char pos_latitude[9];
extern char pos_longitude[9];

extern Boolean bEnter;
extern Boolean bSuccess;
extern struct weather_info_struct g_weather_info;
extern int weather_error_code;

/*******************************************************************
*作用
		获取外网的ip地址，accuweather会把客户端外网地址放入http header中的 X-Forwarded-For 字段中
*参数
		无
*返回值

*其它说明
		2018/11/20 by qinjiangwei
********************************************************************/
Boolean accu_http_get_public_ip_address();


/*
 * 建立HTTP请求头
 */
int SendGetHttpCmd(U8 *url, char *body, void (*fn)(void *, void *), Boolean bGzip);


/*******************************************************************
*作用
		获取location key
*参数
		无
*返回值

*其它说明
		2019/2/18 by qinjiangwei
********************************************************************/
Boolean accu_http_get_location_key();


void trim_to_digits(char *str, int n);

void print_weather_info(struct weather_info_struct *p_weather);

void fill_forecast_info(struct forecast_struct *p_forecast, cJSON *pItem);

int calc_time_zone(char *lng);

void check_time_zone_changed(int tzone);

//"Fri, 13 Jan 2017 11:29 AM CST",
Boolean fill_date_info(const char *ptr, struct dt_struct *p_datetime);

//2017-01-17T00:36:35Z
Boolean get_utc_datetime(char *pstr, struct dt_struct *p_datetime);

/*
 * 更新本地时间算出来的时区
 */
void sbkj_update_time_zone(U8 *tzone);

int calc_time_region(struct dt_struct *utc, struct dt_struct *local_dt);

/*******************************************************************
*作用
		获取5日天气预报
*参数
		无
*返回值

*其它说明
		2019/2/18 by qinjiangwei
********************************************************************/
void accu_http_get_weather();

/*******************************************************************
*作用
		获取5日天气预报
*参数
		无
*返回值

*其它说明
		2019/2/18 by qinjiangwei
********************************************************************/
void accu_http_get_weather_forecasts();



#ifdef __cplusplus
}
#endif

#endif /* __HTTPPARSE_H__ */
