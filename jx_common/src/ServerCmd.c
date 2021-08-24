#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __HTTP_SUPPORT__

#ifndef WIN32
#include "contiki-net.h"
#include "httpparse.h"
#include "process.h"
#include "etimer.h"
#include "ssv_lib.h"
#else
#include <Windows.h>
#endif


#include "ServerCmd.h"

typedef struct {
	int cmd;
	char *cmd_str;
} table_t;

#define HELP(x)		{x, x##_STR},
#define TABLE_ELE(x) HELP(x)

enum {
	FLAG_INDEX = 0,			// 标志
	CMD_INDEX,				// 命令
	TOKEN_INDEX,			// 
	IMEI_INDEX,				// IMEI
	PARA_INDEX,				// 参数
	NR_ITEM
};

static table_t cmd_table[] = 
{
	TABLE_ELE(NOTIFY_CMD)
	TABLE_ELE(NOTIFY_RSP)

	TABLE_ELE(SWITCH_CMD)
	TABLE_ELE(SWITCH_RSP)

	TABLE_ELE(GET_STATUS_CMD)
	TABLE_ELE(GET_STATUS_RSP)

	TABLE_ELE(GET_LIGHT_LIST_CMD)
	TABLE_ELE(GET_LIGHT_LIST_RSP)

	TABLE_ELE(SET_BRIGHT_CHROME_CMD)
	TABLE_ELE(SET_BRIGHT_CHROME_RSP)

	TABLE_ELE(GET_BRIGHT_CHROME_CMD)
	TABLE_ELE(GET_BRIGHT_CHROME_RSP)

	TABLE_ELE(PAIR_LIGHT_CMD)
	TABLE_ELE(PAIR_LIGHT_RSP)

	TABLE_ELE(NOTIFY_LIGHT_CHANGE_CMD)
	TABLE_ELE(NOTIFY_LIGHT_CHANGE_RSP)

	{0, NULL}
};

static char user_token[64];
static int notify_value;

static int GetCmd(char *cmd_str)
{
	table_t *p;

	for (p = &cmd_table[0]; p->cmd != 0; p++)
	{
		if (strcmp(cmd_str, p->cmd_str) == 0)
		{
			return p->cmd;
		}
	}

	return -1;
}

static Boolean sw_status = FALSE;

static Boolean CheckImeiEqual(char *imei)
{
	if (!bImeiInit)
	{
#ifndef WIN32
		F_IMEI_Init();
#endif
		bImeiInit = TRUE;
	}

	if (strcmp(imei, R_szIMEI) == 0)
	{
		return TRUE;
	}
	
	DebugPrintf("imei not equal from s(%s) local(%s) @%d\n", imei, R_szIMEI, __LINE__);
	return FALSE;
}


/*
*
 */
static char *get_cmd_rsp_code(char *cmd_str)
{
	table_t *p;

	for (p = &cmd_table[0]; p->cmd != 0; p++)
	{
		if (strcmp(cmd_str, p->cmd_str) == 0)
		{
			p++;
			return p->cmd_str;
		}
	}

	return NULL;
}

static char *build_param_string(char *array[], int parm)
{
	static char parm_buff[128];
	int cmd;

	cmd = GetCmd(array[CMD_INDEX]);
	if (cmd == GET_STATUS_CMD) 
	{
		sprintf(parm_buff, "%d$", sw_status);
		return parm_buff;
	}
	else if (cmd == SWITCH_CMD)
	{
		sprintf(parm_buff, "%d%d$", parm, sw_status);		// modified by qinjiangwei 2016/08/15 to send back the status 
		return parm_buff;
	}
	
	return NULL;
}


static char *build_rsp_pack(char *array[], int param)
{
	static char buff[128];
	int i;
	char tmp[16];
	char *ptr;
	int len;
	char *p;

	memset(buff, 0, sizeof(buff));
	for (i = 0, p = buff; i < NR_ITEM; i++)
	{
		ptr = array[i];
		if (i == CMD_INDEX)
		{
			ptr = get_cmd_rsp_code(array[i]);
			if (!ptr)
			{
				return NULL;
			}
		}
		else if (i == PARA_INDEX)
		{
			ptr = build_param_string(array, param);
			if (!ptr)
			{
				return NULL;
			}
		}

		len = strlen(ptr);
		memcpy(p, ptr, len);
		p += len;

		if (i == PARA_INDEX)
		{
			*p++ = '\0';
		}
		else
		{
			*p++ = SEP_CHAR;
		}
	}

	return buff;
}


static char * do_with_cmd(char *array[])
{
	int cmd;
	char *rsp;

	if (!CheckImeiEqual(array[IMEI_INDEX]))
	{
		return NULL;
	}

	cmd = GetCmd(array[CMD_INDEX]);
	if (cmd < 0)
	{
		DebugPrintf("Not valid cmd @%d\n", __LINE__);
		return NULL;
	}

	strcpy(user_token, array[TOKEN_INDEX]);

	switch (cmd)
	{
		case NOTIFY_CMD:
		{
			break;
		}

		case SWITCH_CMD:
		{
			char *parm = array[PARA_INDEX];
			int status = atoi(parm);

			if (status != 0 && status != 1)
			{
				rsp = build_rsp_pack(array, 0);
				DebugPrintf("rsp =%s @%d\n", rsp, __LINE__);
				return rsp;
			}

			sw_status = status;
			EnableLed(sw_status);
			
			rsp = build_rsp_pack(array, 1);
			DebugPrintf("rsp =%s @%d\n", rsp, __LINE__);
			return rsp;
		}

		case GET_STATUS_CMD:
		{
			sw_status = GetLedStatus();			// 更新实际状态
			rsp = build_rsp_pack(array, 0);
			DebugPrintf("rsp =%s @%d\n", rsp, __LINE__);
			return rsp;
		}

		case GET_LIGHT_LIST_CMD:
			break;

		case SET_BRIGHT_CHROME_CMD:
			break;


		case GET_BRIGHT_CHROME_CMD:
			break;


		case PAIR_LIGHT_CMD:
			break;

		case NOTIFY_LIGHT_CHANGE_CMD:
			break;

		default:
			break;
	}

	return NULL;
}


//"QC@021@KO23hrP74P1468403082757@0023456787a7000@0$"
char* ParseCmdLine(char *buff)
{
	const char *sep = "@";
	char *p;
	char *ptr;
	char *array[NR_ITEM];
	int index = FLAG_INDEX;

	p = strtok(buff, sep);
	if (!p)
	{
		DebugPrintf("cmd format error @%d\n", __LINE__);
		return FALSE;
	}

	for (; p != NULL; p = strtok(NULL, sep))
	{
		array[index++] = p;

		if (index == NR_ITEM)
		{
			break;
		}
	}

	if (!p)
	{
		DebugPrintf("cmd format error @%d\n", __LINE__);
		return FALSE;
	}

	p = strchr(p, HASH_CHAR);
	if (!p) 
	{
		DebugPrintf("cmd format error @%d\n", __LINE__);
		return FALSE;
	}

	*p = '\0';

	ptr = do_with_cmd(array);

	return ptr;
}


//
static char * build_notify_message(int val)
{
	static char buff[128];
	char tmp[16];
	char sep[] = {SEP_CHAR, '\0'};
	char end[] = {HASH_CHAR, '\0'};

	memset(buff, 0, sizeof(buff));
	strcpy(buff, CLIENT);
	strcat(buff, sep);
	strcat(buff, NOTIFY_CMD_STR);
	strcat(buff, sep);
	if (strlen(user_token) == 0)
	{
		strcpy(user_token, "000000000000000");
	}
	strcat(buff, user_token);
	strcat(buff, sep);
	strcat(buff, R_szIMEI);
	strcat(buff, sep);
	sprintf(tmp, "%d", val);
	strcat(buff, tmp);
	strcat(buff, end);

	return buff;	
}


void send_notify_message_callback(void)
{
	send_notify_message(notify_value);
}

void send_notify_message(int val)
{
	char *msgid;
	char *message;

	printf("send_notify_message\n");
	notify_value = val;
	msgid = get_unused_msg_id();
	if (msgid == NULL)
	{
		return;
	}
	
	printf("send_notify_message22\n");
	message = build_notify_message(val);
#ifndef WIN32
	SendMessage(message, msgid);
#endif
}

void test_send_notify_msg()
{
	send_notify_message(1);
}

#ifdef WIN32
Boolean bDebug = TRUE;

int GetLedStatus()
{
	return 1;
}

void EnableLed(int on)
{

}

char R_szIMEI[15 + 1] = "123456789512345";
Boolean bImeiInit = FALSE;
char * get_unused_msg_id(cb_fn func) 
{
	return "789afeb4-2a82-4609-93ca-2e2564e93ae8";
}
#endif

#endif
