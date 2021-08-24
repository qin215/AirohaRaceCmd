#ifndef _SERVERCMD_H_
#define _SERVERCMD_H_ 1

#ifdef __cplusplus
extern "C"
{
#endif

#define HTTPSERVER  ("QS")

#define CLIENT  ("QC")
#define LOGIN_CMD  ("010")			// no use 
#define LOGIN_RSP_CMD  ("011")		// no use for http
#define HEART_CMD  ("014")			// no use for http
#define HEART_RSP_CMD  ("015")		// no use for http

#define NOTIFY_CMD_STR  ("016")
#define NOTIFY_RSP_STR  ("017")

#define SWITCH_CMD_STR  ("018")
#define SWITCH_RSP_STR  ("019")

#define GET_STATUS_CMD_STR  ("020")
#define GET_STATUS_RSP_STR  ("021")

#define PING_CMD  ("022")		// no use for http
#define PING_RSP  ("023")		// no use for http

#define GET_LIGHT_LIST_CMD_STR  ("024")
#define GET_LIGHT_LIST_RSP_STR  ("025")

#define SET_BRIGHT_CHROME_CMD_STR  ("026")
#define SET_BRIGHT_CHROME_RSP_STR  ("027")

#define GET_BRIGHT_CHROME_CMD_STR  ("028")
#define GET_BRIGHT_CHROME_RSP_STR  ("029")

#define PAIR_LIGHT_CMD_STR  ("030")
#define PAIR_LIGHT_RSP_STR  ("031")

#define NOTIFY_LIGHT_CHANGE_CMD_STR  ("032")
#define NOTIFY_LIGHT_CHANGE_RSP_STR  ("033")


#define HASH_CHAR '$'
#define SEP_CHAR '@'

enum 
{
	NOTIFY_CMD = 1,
	NOTIFY_RSP,

	SWITCH_CMD,
	SWITCH_RSP,

	GET_STATUS_CMD,
	GET_STATUS_RSP,

	GET_LIGHT_LIST_CMD,
	GET_LIGHT_LIST_RSP,

	SET_BRIGHT_CHROME_CMD,
	SET_BRIGHT_CHROME_RSP,

	GET_BRIGHT_CHROME_CMD,
	GET_BRIGHT_CHROME_RSP,

	PAIR_LIGHT_CMD,
	PAIR_LIGHT_RSP,

	NOTIFY_LIGHT_CHANGE_CMD,
	NOTIFY_LIGHT_CHANGE_RSP
};

#ifdef WIN32
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif

#ifndef Boolean 
#define Boolean int
#endif

#ifndef kal_uint16
#define kal_uint16 unsigned short
#endif

#ifndef kal_uint8
#define kal_uint8 unsigned char
#endif


#ifndef kal_int16
#define kal_int16 short
#endif

#ifndef kal_int32
#define kal_int32 int
#endif

#ifndef kal_uint32
#define kal_uint32 unsigned int
#endif

#define kal_bool Boolean

#define KAL_FALSE	FALSE
#define KAL_TRUE	TRUE

#ifndef U8 
#define U8 unsigned char 
#endif


#if defined (_MSC_VER)
#define	DebugPrintf(...) 		do { \
	if (bDebug) { printf(__VA_ARGS__); } \
	} while (0)

#endif

#define PROCESS_EVENT_UNREAD_MSG	(0xA1)
#define PROCESS_EVENT_GETID_MSG		(0xA2)

typedef void (*test_func_t)();


extern Boolean bImeiInit;
extern Boolean bDebug;
extern char R_szIMEI[];
extern int F_IMEI_Init(void);
extern void EnableLed(int on);
extern int GetLedStatus();

extern test_func_t mytest_func;

//PROCESS_NAME(http_poll_thread);

char * ParseCmdLine(char *buff);
void DoWithServerMsg();
void GetUnreadMessage(int pi);
typedef void (*cb_fn)(void);
char * get_unused_msg_id(void);

char * ParseCmdLine(char *buff);
void send_notify_message(int val);
//void SendMessage(char *message, char *msgId);
void send_notify_message_callback(void);

Boolean test_add_func(test_func_t f);
void invoke_test_func_queue();

#ifdef __cplusplus
}
#endif
#endif

