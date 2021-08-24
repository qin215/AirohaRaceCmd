#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include "contiki-net.h"
#endif
#include "httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"
#include "httpd.h"
#include "data_buff.h"
// http cookies management by qinjiangwei 2017/3/24
#if  1//def __WEBSERVER_SUPPORT__


static httpd_container_t con_array[MAX_CLIENT];
void httpd_print_container()
{
	httpd_container_t *pcon;
	int i;

	for (pcon = con_array, i = 0; i < MAX_CLIENT; pcon++, i++)
	{
		httpd_kv_t *pkv;

		for (pkv = pcon->cookie_array; pkv->key[0] != '\0'; pkv++)
		{
			printf("key=%s,val=%s\n", pkv->key, pkv->value);
		}
	}

}


static httpd_kv_t * find_key_in_kv(httpd_container_t* pcon, char *cookie)
{
	httpd_kv_t *pkv;

	if (pcon == NULL || cookie == NULL)
	{
		printf("null ptr\n");
		return NULL;
	}

	for (pkv = pcon->cookie_array; pkv->key[0] != '\0'; pkv++)
	{
		if (strcmp(pkv->key, cookie) == 0)
		{
			return pkv;
		}
	}

	return NULL;
}

int httpd_find_container(char *cookie, httpd_kv_t** ppkv, httpd_container_t** ppcon)
{
	httpd_container_t *pcon;
	int i;
	httpd_print_container();
	
	if (!cookie)
	{
		printf("cookie is null!\n");
		return -1;
	}

	for (pcon = con_array, i = 0; i < MAX_CLIENT; pcon++, i++)
	{
		if ((*ppkv = find_key_in_kv(pcon, cookie)))
		{
			*ppcon = pcon;
			return i;
		}
	}

	return -1;
}

static httpd_container_t * remove_oldest_contain()
{
	int i;

	for (i = MAX_CLIENT - 2; i >= 0 ; i--)
	{
		con_array[i + 1] = con_array[i];
	}

	return con_array;
}

void add_kv(httpd_container_t* pcon, char *k, char *v)
{
	httpd_kv_t *pkv = pcon->cookie_array;
	int i;
	int n;
	
	i = 0;
	while (pkv->key[0] != '\0')
	{
		if (strcmp(pkv->key, k) == 0)
		{
			break;
		}
		
		pkv++;
		i++;
		if (i == MAX_COOKIE)
		{
			printf("cookie full\n");
			return;
		}
	}

	n = strlen(k);
	if (n >= HTTPD_KV_LEN)
	{
		printf("k is long n=%d\n", n);
		return;
	}

	n = strlen(v);
	if (n >= HTTPD_KV_LEN)
	{
		printf("v is long n=%d\n", n);
		return;
	}

	strcpy(pkv->key, k);
	strcpy(pkv->value, v);
	pcon->time = 11; //clock_seconds();
}

static void move_to_first_one(int index)
{
	httpd_container_t tmp;

	tmp = con_array[index];
	for (; index > 0; index--)		// 移到第一个位置
	{
		con_array[index] = con_array[index - 1];
	}
	con_array[0] = tmp;
}

httpd_container_t* httpd_update_cookie(char *key, char *value)
{
	httpd_container_t *pcon;
	httpd_kv_t *pkv = NULL;
	int index;
	
	index = httpd_find_container(key, &pkv, &pcon);
	
	if (index >= 0)
	{
		int n;

		pcon = &con_array[index];
		
		n = strlen(value);
		if (n >= sizeof(pkv->value))
		{
			printf("cookie too long:n=%d\n", n);
			return NULL;
		}
		
		strcpy(pkv->value, value);
		pcon->time = 11; //clock_seconds();
		move_to_first_one(index);
	}
	else
	{
		int i;

		for (i = 0, pcon = con_array; i < MAX_CLIENT; i++, pcon++)
		{
			if (pcon->time == 0)
			{
				break;
			}
		}

		if (i == MAX_CLIENT)
		{
			pcon = remove_oldest_contain();
		}
		else
		{
			move_to_first_one(i);
			pcon = con_array;
		}

		add_kv(pcon, key, value);
	}

	return con_array;
}

httpd_container_t *parse_cookie_parameter(char *start, char *end, Boolean added)
{
	httpd_kv_t kv;
	int index;
	char *sptr;
	char *eptr;
	char *next;
	httpd_container_t *pcon = NULL;

	for (index = 0; start != NULL; start = next)
	{
		next = httpd_get_segment(start, end + 1 - start, &sptr, &eptr, ";");
		if (!next)
		{
			sptr = start;
			eptr = end;
		}

		if (httpd_get_kv(sptr, eptr, &kv))
		{
			httpd_kv_t *pkv;
			int index;

			if (!pcon)
			{
				httpd_find_container(kv.key, &pkv, &pcon);
			}
	
			if (pcon)
			{
				if (!find_key_in_kv(pcon, kv.key))
				{
					if (added)
					{
						add_kv(pcon, kv.key, kv.value);
					}
					else
					{
						return NULL;
					}
				}
			}
			else
			{
				if (added)
				{
					pcon = httpd_update_cookie(kv.key, kv.value);
				}
				else
				{
					return NULL;
				}
			}
			
		}
	}

	return pcon;
}

int convert_cookie_to_string(httpd_container_t *pcon, char *buff, int len)
{
	httpd_kv_t *pkv;
	int index;

	if (pcon == NULL)
	{
		printf("null ptr\n");
		return -1;
	}

	buff[0] = '\0';
	strcpy(buff, "Set-Cookie:");
	index = strlen("Set-Cookie:");
	for (pkv = pcon->cookie_array; pkv->key[0] != '\0'; pkv++)
	{
		int n;

		n = strlen(pkv->key);
		if (index + n + 1 >= len)
		{
			printf("buff is low!\n");
			return -1;
		}
		strcpy(&buff[index], pkv->key);
		index += n;
		strcpy(&buff[index], "=");
		index += 1;

		n = strlen(pkv->value);
		if (index + n + 1 >= len)
		{
			printf("buff is low!\n");
			return -1;
		}
		strcpy(&buff[index], pkv->value);
		index += n;
		strcpy(&buff[index], ";");
		index += 1;
	}

	if (index + 2 >= len)
	{
		return -1;
	}
	
	strcpy(&buff[index], "\r\n\r\n");
	index += 4;

	return index;	
}

unsigned short get_last_port();
void uip_random_port();

httpd_container_t * generate_auth_cookie()
{
	char v[HTTPD_KV_LEN];
	unsigned short v1, v2;
#ifdef WIN32
	v1 = 0x33aec;
	v2 = 0x2a890;
#else
	uip_random_port();
	v1 = get_last_port();

	uip_random_port();
	v2 = get_last_port();
#endif
	sprintf(v, "%d%x", v1, v2);
	
	return httpd_update_cookie(COOKIE_USER_TOKEN_STR, v);
}


void test_cookie()
{
	int n;
	char *begin;
	char *end;
	char *next_line;
	char *ptr;
	int c;
	char *p[] = {"Cookie: theme=light; sessionToken=abc123\r\n",
				 "Cookie: theme=light2; sessionToken=abc123\r\n",
				 "Cookie: theme=light; usertoken=1567774c\r\n",
				 NULL};
	int i;
	httpd_kv_t *pkv;
	httpd_container_t *pcon;
	char buff[64];

	char cookies_str[] = "curShow=; usertoken=1567774c\r\n";
	
	for (i = 0, ptr = p[i];  i < 3; i++, ptr = p[i])
	{
		next_line = httpd_get_segment(ptr, strlen(ptr), &begin, &end, "\r\n");
		if (!next_line)
		{
			printf("not valid header!\n");
			return;
		}

		ptr = strstr(begin, "Cookie: ");
		if (!ptr)
		{
			printf("no cookie!\n");
			return;
		}

		ptr += strlen("Cookie: ");
		
		parse_cookie_parameter(ptr, end, TRUE);
	}
	
	httpd_find_container("sessionToken", &pkv, &pcon);

#if 0
	pcon = generate_auth_cookie();
	if (pcon)
	{
		int index;

		index = convert_cookie_to_string(pcon, buff, sizeof(buff));
			
		if (index < 0)
		{
			printf("buf is low!\n");
		}
	}
#endif

	httpd_check_login_auth_token(cookies_str, strlen(cookies_str));

}

#endif
