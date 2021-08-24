#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "ServerCmd.h"
#include "qcloud_iot_export_log.h"

 #define snprintf _snprintf

/*
 * added by qinjiangwei 2018/5/14 解析mac1/mac2 地址
 */
static Boolean replace_mac_addr(char *json_str, kal_uint8 *pmac, const char *key);

/*
 * 在如下格式的字符串中替换mac1/mac2地址
 * 
 {
 "addr1":"44:57:18:3f:89:a1",
 "addr2":"44:57:18:3f:89:a2",
 "softap_ssid":"icomm-softap"
 }

 * 输入参数：json_str, json格式的配置信息
 *			 pmac1: mac1 地址，二进制, 6个字节
 *			 pmac2: mac2 地址，二进制，6个字节
 * 返回值：
 *			TRUE, 执行成功， json_str的mac地址被替换
 *			FALSE, 失败
 * 
 */
Boolean replace_all_mac_address(char *json_str, kal_uint8 *pmac1, kal_uint8 *pmac2)
{
	if (!replace_mac_addr(json_str, pmac1, "addr1"))
	{
		return FALSE;
	}

	if (!replace_mac_addr(json_str, pmac2, "addr2"))
	{
		return FALSE;
	}

	return TRUE;
}

/*
 * 替换一个MAC地址
 */
static Boolean replace_mac_addr(char *json_str, kal_uint8 *pmac, const char *key)
{
	char mac_str[128];
	char key_str[128];
	char *ptr;
	size_t i;

	snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", pmac[0], pmac[1], pmac[2], pmac[3], pmac[4], pmac[5]);
	if (!json_str)
	{
		return FALSE;
	}

	snprintf(key_str, sizeof(key_str), "\"%s\":\"", key);

	ptr = strstr(json_str, key_str);
	if (!ptr)
	{
		Log_e("not found (%s)\n", key_str);
		return FALSE;
	}

	ptr += strlen(key_str);

	for (i = 0; i < strlen(mac_str); i++)
	{
		*ptr++ = mac_str[i];
	}

	return TRUE;
}

/*
 * MAC 地址增加n个
 * pmac: mac 地址, 网络字节序
 * pmac_out: 加法之后的mac地址, 网络字节序
 */
Boolean mac_advance_step(const kal_uint8 *pmac, int len, int n, kal_uint8 *pmac_out)
{
	kal_uint8 buf[8];
	int i;
	UINT64 mac_value;

	memset(buf, 0, sizeof(buf));

	if (len != 6)
	{
		return FALSE;
	}

	for (i = 0; i < 6; i++)
	{
		buf[i] = pmac[5 - i];
	}

	mac_value = (*(UINT64 *)&buf[0]);
	mac_value += n;

	for (i = 0; i < 6; i++)
	{
		pmac_out[i] = (mac_value >> ((5 - i) * 8)) & 0xff;
	}

	return TRUE;	
}

/*
 * 测试程序
 */
void test_replace_mac_str()
{
	char json_str[] = "{\"addr1\":\"44:57:18:3f:89:a1\",\"addr2\":\"44:57:18:3f:89:a2\",\"softap_ssid\":\"icomm-softap\"}";
	kal_uint8 pmac1[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
	kal_uint8 pmac2[] = {0x00, 0xaa, 0xbb, 0x33, 0xc, 0x55 };
	
	IOT_Log_Set_Level(QC_DEBUG);
	if (replace_all_mac_address(json_str, pmac1, pmac2))
	{
		Log_d("replace str=%s", json_str);
	}

	mac_advance_step(pmac1, 6, 1, pmac2);
}


void macstring_parser(char *macstr, char *addr)
{
	kal_uint8 len = strlen(macstr);
	int i, index = 0;

	if (len > 17)
	{
		goto err;
	}

	addr[index] = 0;
	for(i = 0; i < len; i++)
	{
		if(index > 5)
			goto  err;

		if('0' <= macstr[i] && macstr[i] <= '9')
		{
			addr[index] = (addr[index] << 4) + macstr[i] - '0';
		}
		else if('a' <= macstr[i] && macstr[i] <= 'f')
		{
			addr[index] = (addr[index] << 4) + macstr[i] - 'a' + 10;
		}
		else if('A' <= macstr[i] && macstr[i] <= 'F')
		{
			addr[index] = (addr[index] << 4) + macstr[i] - 'A' + 10;
		}
		else if(macstr[i] == ':')
		{
			index++;
			addr[index] = 0;
		}
		else
		{
			goto err;
		}


	}

	return;

err:
	printf("Illegal mac string : %s\n", macstr);
}