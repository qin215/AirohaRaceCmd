/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

/*
 * This file includes functions that are called by the web server
 * scripts. The functions takes no argument, and the return value is
 * interpreted as follows. A zero means that the function did not
 * complete and should be invoked for the next packet as well. A
 * non-zero value indicates that the function has completed and that
 * the web server should move along to the next script line.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <strings.h>

#ifndef WIN32
#include "contiki-net.h"
#endif

#include "httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"

//#include "petsciiconv.h"


#include "httpd.h"
#include "data_buff.h"

static struct httpd_cgi_call *calls = NULL;

static const char closed[] =   /*  "CLOSED",*/
{0x43, 0x4c, 0x4f, 0x53, 0x45, 0x44, 0};
static const char syn_rcvd[] = /*  "SYN-RCVD",*/
{0x53, 0x59, 0x4e, 0x2d, 0x52, 0x43, 0x56,
 0x44,  0};
static const char syn_sent[] = /*  "SYN-SENT",*/
{0x53, 0x59, 0x4e, 0x2d, 0x53, 0x45, 0x4e,
 0x54,  0};
static const char established[] = /*  "ESTABLISHED",*/
{0x45, 0x53, 0x54, 0x41, 0x42, 0x4c, 0x49,
 0x53, 0x48, 0x45, 0x44, 0};
static const char fin_wait_1[] = /*  "FIN-WAIT-1",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
 0x54, 0x2d, 0x31, 0};
static const char fin_wait_2[] = /*  "FIN-WAIT-2",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
 0x54, 0x2d, 0x32, 0};
static const char closing[] = /*  "CLOSING",*/
{0x43, 0x4c, 0x4f, 0x53, 0x49,
 0x4e, 0x47, 0};
static const char time_wait[] = /*  "TIME-WAIT,"*/
{0x54, 0x49, 0x4d, 0x45, 0x2d, 0x57, 0x41,
 0x49, 0x54, 0};
static const char last_ack[] = /*  "LAST-ACK"*/
{0x4c, 0x41, 0x53, 0x54, 0x2d, 0x41, 0x43,
 0x4b, 0};
static const char none[] = /*  "NONE"*/
{0x4e, 0x4f, 0x4e, 0x45, 0};
static const char running[] = /*  "RUNNING"*/
{0x52, 0x55, 0x4e, 0x4e, 0x49, 0x4e, 0x47,
 0};
static const char called[] = /*  "CALLED"*/
{0x43, 0x41, 0x4c, 0x4c, 0x45, 0x44, 0};
static const char file_name[] = /*  "file-stats"*/
{0x66, 0x69, 0x6c, 0x65, 0x2d, 0x73, 0x74,
 0x61, 0x74, 0x73, 0};
static const char tcp_name[] = /*  "tcp-connections"*/
{0x74, 0x63, 0x70, 0x2d, 0x63, 0x6f, 0x6e,
 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e,
 0x73, 0};
static const char proc_name[] = /*  "processes"*/
{0x70, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73,
 0x65, 0x73, 0};

static const char *states[] = {
  closed,
  syn_rcvd,
  syn_sent,
  established,
  fin_wait_1,
  fin_wait_2,
  closing,
  time_wait,
  last_ack,
  none,
  running,
  called};

char * cgi_get_option_value(char *p, int ptrlen, char *value, int len);
static int my_strncasecmp(const char *s1, const char *s2, size_t n);

static buf_t * nullfunction(sock_server_t *pserver, buf_t *b)
{
	return NULL;
}

/*---------------------------------------------------------------------------*/
httpd_cgifunction httpd_cgi(char *name)
{
	struct httpd_cgi_call *f;

	/* Find the matching name in the table, return the function. */
	for (f = calls; f != NULL; f = f->next) 
	{
		if (strncmp(f->name, name, strlen(f->name)) == 0) 
		{
			return f->function;
		}
	}
	
	return nullfunction;
}

static char lfile[2048];
/*---------------------------------------------------------------------------*/
static unsigned short generate_file_stats(buf_t *b, char *arg)
{
	char *f = arg;
	int i;

	for (i = 0; i < sizeof(lfile) - 1; i++)
	{
		lfile[i] = 'A';
	}
	
	
	//b->len += _snprintf(&b->pbuff[b->len], BUF_SIZE - b->len, "%5u", httpd_fs_count(f));
	strcpy(b->pbuff, lfile);
	b->len = strlen(lfile);

	print_buffer(b);

	return TRUE;
}
/*---------------------------------------------------------------------------*/
static buf_t * file_stats(sock_server_t *s, buf_t *b, char *ptr)
{
	char *p;
	buf_t *nb;
	int ret;

	p = strchr(ptr, ' ');
	if (!p)
	{
		printf("something error!\n");
		return b;
	}

	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	
	generate_file_stats(nb, p + 1);
	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
		return b;
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		return nb;
	}

	return b;
}
/*---------------------------------------------------------------------------*/
static unsigned short make_tcp_stats(sock_server_t *pserver, buf_t *b, void *arg)
{
#if 0
	struct uip_conn *conn;
	conn = (struct uip_conn *)arg;

	b->len += snprintf(&b->pbuff[b->len], BUF_SIZE - b->len,
						"<tr align=\"center\"><td>%d</td><td>%u.%u.%u.%u:%u</td><td>%s</td><td>%u</td><td>%u</td><td>%c %c</td></tr>\r\n",
						uip_htons(conn->lport),
						conn->ripaddr.u8[0],
						conn->ripaddr.u8[1],
						conn->ripaddr.u8[2],
						conn->ripaddr.u8[3],
						uip_htons(conn->rport),
						states[conn->tcpstateflags & UIP_TS_MASK],
						conn->nrtx,
						conn->timer,
						(uip_outstanding(conn))? '*':' ',
						(uip_stopped(conn))? '!':' ');
#endif

	print_buffer(b);

	return TRUE;
}

/*---------------------------------------------------------------------------*/
static buf_t* tcp_stats(sock_server_t *s, buf_t *b, char *ptr)
{
  	int i;
	buf_t *nb;
	int ret;

	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}

	#if 0
	for (i = 0; i < UIP_CONNS; i++) 
	{
		if ((uip_conns[i].tcpstateflags & UIP_TS_MASK) != UIP_CLOSED)
		{
			make_tcp_stats(s, nb, &uip_conns[i]);
		}
	}
	#endif

	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
		return b;
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		return nb;
	}

	return b;
}
/*---------------------------------------------------------------------------*/
static unsigned short make_processes(sock_server_t *pserver, buf_t *b, void *arg)
{
	char name[40];
#if 0	
	struct process * p = (struct process *)arg;

	strncpy(name, PROCESS_NAME_STRING(p), 40);

	b->len +=  snprintf(&b->pbuff[b->len], BUF_SIZE - b->len,
						"<tr align=\"center\"><td>%p</td><td>%s</td><td>%p</td><td>%s</td></tr>\r\n",
						p, name,
						*((char **)&(((struct process *)p)->thread)),
						states[9 + ((struct process *)p)->state]);
#endif

	print_buffer(b);

	return TRUE;
}
/*---------------------------------------------------------------------------*/
static buf_t * processes(sock_server_t *s, buf_t *b, char *p)
{

//	struct process * ptr;
	buf_t *nb;
	int ret;

#if 0
	for (ptr = PROCESS_LIST(); ptr != NULL; ptr = (ptr->next))
	{
		nb = alloc_buffer();
		if (!nb)
		{
			printf("no buffer@%d\n", __LINE__);
			return b;
		}

		make_processes(s, nb, ptr);
		ret = append_buf2(b, nb);
		if (ret == -1 || ret == 0)
		{
			free_buffer(nb);
		}
		else if (ret == 1)
		{
			add_buf(&s->tx_queue, b);
			b = nb;
		}
	}
#endif
	return b;
}

static buf_t * fn_get_ap_name(sock_server_t *s, buf_t *b, char *p)
{
	buf_t *nb;
	int ret;
#if 0
	U8 ssid[33];
	U8 ssidlen = 32;
	U8 key[65];
	U8 keylen = 64;
	U8 encryt = 0;
	TAG_AP_INFO apinfo;
	U8 mode = get_wifi_mode();

	if (mode)
	{
		if (get_apmode_config(ssid, &ssidlen, &encryt, key, &keylen) == 0)
	    {
	        ssid[ssidlen] = 0;
	        key[keylen] = 0;
	    }
		else
		{
			printf("get ap setting failed!\n");
			return b;
		}
	}
	else
	{
	    if (get_wifi_connected() == 1) 
		{
	        get_connected_ap_info(&apinfo);
	        printf("%s", apinfo.name);
	        printf(", ch: %d", apinfo.channel);
	        printf(", mac: %x:%x:%x:%x:%x:%x", apinfo.mac[0],apinfo.mac[1],apinfo.mac[2],
	                  apinfo.mac[3],apinfo.mac[4],apinfo.mac[5]);
			strncpy(ssid, apinfo.name, sizeof(ssid));
	    } 
		else
		{
	        printf("please connect ap\r\n");
			return b;
	    }
	}


	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	nb->len +=  snprintf(&nb->pbuff[nb->len], BUF_SIZE - nb->len, "%s", ssid);
	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		b = nb;
	}
#endif	
	return b;
}

static buf_t * fn_get_password(sock_server_t *s, buf_t *b, char *p)
{
	buf_t *nb;
	int ret;
#if 0
	U8 ssid[33];
	U8 ssidlen = 32;
	U8 key[65];
	U8 keylen = 64;
	U8 encryt = 0;
	TAG_AP_INFO apinfo;
	U8 mode = get_wifi_mode();

	if (mode)
	{
		if (get_apmode_config(ssid, &ssidlen, &encryt, key, &keylen) == 0)
		{
			ssid[ssidlen] = 0;
			key[keylen] = 0;
		}
		else
		{
			printf("get ap setting failed!\n");
			return b;
		}
	}
	else
	{
		if (get_wifi_connected() == 1) 
		{
			strncpy(key, (char *)(gwifistatus.connAP.key), sizeof(key));
		} 
		else
		{
			printf("please connect ap\r\n");
			return b;
		}
	}
#endif

	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	nb->len += _snprintf(&nb->pbuff[nb->len], BUF_SIZE - nb->len, "%s", "WiFi-AP");
	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		b = nb;
	}
	
	return b;
}

static buf_t * fn_check_encrypt(sock_server_t *s, buf_t *b, char *p)
{
	buf_t *nb;
	int ret;
	unsigned char encryt = 0;
	char value[32];
	char *pcrypt;
	char *pselect;

#if 0
	U8 ssid[33];
	U8 ssidlen = 32;
	U8 key[65];
	U8 keylen = 64;
	TAG_AP_INFO apinfo;
	U8 mode = get_wifi_mode();

	if (mode)
	{
		if (get_apmode_config(ssid, &ssidlen, &encryt, key, &keylen) == 0)
		{
			ssid[ssidlen] = 0;
			key[keylen] = 0;
		}
		else
		{
			printf("get ap setting failed!\n");
			return b;
		}
	}
	else
	{
		if (get_wifi_connected() == 1) 
		{
			strncpy(key, (char *)(gwifistatus.connAP.key), sizeof(key));
		} 
		else
		{
			printf("please connect ap\r\n");
			return b;
		}
	}
#endif

	if (!cgi_get_option_value(s->left_bracket, s->bracket_len, value, sizeof(value)))
	{
		printf("no option value!\n");
		return b;
	}

	if (encryt == 0)
	{
		pcrypt = "open";
	}
	else 
	{
		pcrypt = "wpa2";
	}

	if (stricmp(value, pcrypt) == 0)
	{
		pselect = "selected=\"selected\"";
	}
	else
	{
		return b;
	}
	
	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	nb->len += _snprintf(&nb->pbuff[nb->len], BUF_SIZE - nb->len, "%s", pselect);
	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		b = nb;
	}
	
	return b;
}


static unsigned char encryt_mode = 0;
static char passwd[65];
static char myssid[65];

static buf_t * fn_change_sap_setting(sock_server_t *s, buf_t *b, char *p)
{
	buf_t *nb;
	char *pcrypt = NULL;
	httpd_kv_t *parray;
	Boolean ret;
	char *pkey;
	char *msg;
	
	parray = s->http_req.kv_array;

	if (strlen(myssid) > 0)
	{
		msg = "busy";
	}
	else 
	{
		while ((pkey = parray->key) && *pkey != '\0')
		{
			if (strcmp(pkey, "name") == 0)
			{
				strncpy(myssid, parray->value, sizeof(myssid) - 1);
			}
			else if (strcmp(pkey, "password") == 0)
			{
				strncpy(passwd, parray->value, sizeof(passwd) - 1);
			}
			else if (strcmp(pkey, "encrypt") == 0)
			{
				pcrypt = parray->value;
			}

			parray++;
		}

		ret = strlen(myssid) > 0;

		if (strcmp(pcrypt, "open") != 0)
		{
			encryt_mode = 1;
			ret = ret && strlen(passwd) > 0;
		}

		if (ret)
		{
			//process_start(&cgi_auto_reboot_process, NULL);
			msg = "success";
		}
		else
		{
			memset(myssid, 0, sizeof(myssid));
			msg = "failed";
		}
	}

	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	
	nb->len += _snprintf(&nb->pbuff[nb->len], BUF_SIZE - nb->len, "%s", msg);
	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		b = nb;
	}
	
	return b;
}

static buf_t * fn_change_sec_setting(sock_server_t *s, buf_t *b, char *p)
{
	buf_t *nb;
	char *pcrypt = NULL;
	httpd_kv_t *parray;
	Boolean ret;
	char *pkey;
	char *msg;
	char *pname = NULL;
	char *ppasswd = NULL;
	char *new_name = NULL;
	char *new_pass1 = NULL;
	char *new_pass2 = NULL;
	char *oldname;
	char *oldpass;
	Boolean bok = FALSE;
	
	parray = s->http_req.kv_array;

	while ((pkey = parray->key) && *pkey != '\0')
	{
		if (strcmp(pkey, "name") == 0)
		{
			pname = parray->value;
		}
		else if (strcmp(pkey, "password") == 0)
		{
			ppasswd = parray->value;
		}
		else if (strcmp(pkey, "newname") == 0)
		{
			new_name = parray->value;
		}
		else if (strcmp(pkey, "newname") == 0)
		{
			new_name = parray->value;
		}
		else if (strcmp(pkey, "newpassword1") == 0)
		{
			new_pass1 = parray->value;
		}
		else if (strcmp(pkey, "newpassword2") == 0)
		{
			new_pass2 = parray->value;
		}
		
		parray++;
	}

	oldname = "admin";// hx_get_web_username();
	oldpass = "test";//hx_get_web_password();

	if (strlen(oldpass) == 0)
	{
		bok = TRUE;
	}
	else 
	{
		if (ppasswd && pname)
		{
			bok = ((strcmp(oldname, pname) == 0) && (strcmp(oldpass, ppasswd) == 0));
		}
	}

	if (bok)
	{
		if (new_pass1 && new_pass2)
		{
			bok = (strcmp(new_pass1, new_pass2) == 0);
		}
		else
		{
			bok = ((new_pass1 == NULL) && (new_pass2 == NULL));
		}
		
		//bok = hx_update_web_username(pname, new_pass1 ? new_pass1 : "");
	}

	printf("bok=%d\n", bok);
	if (bok)
	{
		//process_start(&cgi_auto_reboot_process, NULL);
		msg = "success";
	}
	else
	{
		msg = "failed";
	}
	
	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	
	nb->len += _snprintf(&nb->pbuff[nb->len], BUF_SIZE - nb->len, "%s", msg);
	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		b = nb;
	}
	
	return b;
}

static buf_t * fn_check_login_valid(sock_server_t *s, buf_t *b, char *p)
{
	buf_t *nb;
	char *pcrypt = NULL;
	httpd_kv_t *parray;
	Boolean ret;
	char *pkey;
	char *msg;
	char *username = NULL;
	char *passwd = NULL;
	char *oldname;
	char *oldpass;
	Boolean bok = FALSE;
	struct httpd_fs_file file;
	char *filename;

	parray = s->http_req.kv_array;

	while ((pkey = parray->key) && *pkey != '\0')
	{
		if (strcmp(pkey, "username") == 0)
		{
			username = parray->value;
		}
		else if (strcmp(pkey, "password") == 0)
		{
			passwd = parray->value;
		}
	
		parray++;
	}

	oldname = "admin";//hx_get_web_username();
	oldpass = "";//hx_get_web_password();

	if (strlen(oldpass) == 0)
	{
		bok = TRUE;
	}
	else 
	{
		if (passwd && username)
		{
			bok = ((strcmp(oldname, username) == 0) && (strcmp(oldpass, passwd) == 0));
		}
	}

	nb = alloc_buffer();
	if (!nb)
	{
		printf("no buffer@%d\n", __LINE__);
		return b;
	}
	
	printf("bok=%d\n", bok);


	
	if (bok)
	{
		httpd_token_time = 111;//clock_seconds();
		filename = "/index.html";
	}
	else
	{
		httpd_token_time = 0;
		filename = "/login.html";
	}
	
	httpd_fs_open(filename, &file);
	if (file.len > BUF_SIZE)
	{
		printf("len too long=%d\n", file.len);
		file.len = BUF_SIZE;
	}
	memcpy(&nb->pbuff[0], file.data, file.len);
	nb->len = file.len;

	ret = append_buf2(b, nb);
	if (ret == -1 || ret == 0)
	{
		free_buffer(nb);
	}
	else if (ret == 1)
	{
		add_buf(&s->tx_queue, b);
		b = nb;
	}
	
	return b;
}

/*---------------------------------------------------------------------------*/
void httpd_cgi_add(struct httpd_cgi_call *c)
{
	struct httpd_cgi_call *l;

	c->next = NULL;
	if (calls == NULL) 
	{
		calls = c;
	} 
	else 
	{
		for (l = calls; l->next != NULL; l = l->next)
		{
			;
		}
		
		l->next = c;
	}
}
/*---------------------------------------------------------------------------*/
HTTPD_CGI_CALL(file, file_name, file_stats);
HTTPD_CGI_CALL(tcp, tcp_name, tcp_stats);
HTTPD_CGI_CALL(proc, proc_name, processes);

static struct httpd_cgi_call get_ap = {NULL, "get_ap_name", fn_get_ap_name};
static struct httpd_cgi_call get_password = {NULL, "get_password", fn_get_password};
static struct httpd_cgi_call check_encrypt = {NULL, "check_encrypt", fn_check_encrypt};
static struct httpd_cgi_call change_sap_setting = {NULL, "cgi_change_sap_setting", fn_change_sap_setting};
static struct httpd_cgi_call change_sec_setting = {NULL, "cgi_change_sec_setting", fn_change_sec_setting};
static struct httpd_cgi_call check_login_valid = {NULL, "cgi_check_login_valid", fn_check_login_valid};

void httpd_cgi_init(void)
{
	httpd_cgi_add(&file);
	httpd_cgi_add(&tcp);
	httpd_cgi_add(&proc);
	httpd_cgi_add(&get_ap);
	httpd_cgi_add(&get_password);
	httpd_cgi_add(&check_encrypt);
	httpd_cgi_add(&change_sap_setting);
	httpd_cgi_add(&change_sec_setting);
	httpd_cgi_add(&check_login_valid);
}
/*---------------------------------------------------------------------------*/
#define isblank(c)	((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')


#define DIFF_VALUE ('a' - 'A')
//#define FALSE      ('z' - 'A')

static int my_strncasecmp(const char *s1, const char *s2, size_t n)
{
	int ch1 = 0;
	int ch2 = 0;

	if (NULL == s1 || NULL == s2 || 0 > n)
	{
		return -1;
	}

	if (0 == n)
	{
		return 0;
	}

	do
	{
		if ((ch1 = *(unsigned char *)s1++) >= 'A' && (ch1 <= 'Z'))
		{
			ch1 += DIFF_VALUE;
		}
		if ((ch2 = *(unsigned char *)s2++) >= 'A' && (ch2 <= 'Z'))
		{
			ch2 += DIFF_VALUE;
		}
		n--;
	} while (n != 0 && ch1 && (ch1 == ch2));

	return ch1 - ch2;
}

char *cgi_skip_blank(char *p)
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

char * cgi_get_option_value(char *p, int ptrlen, char *value, int len)
{
	int i;
	char *ptr = p;
	char *tmp;

	if (!p)
	{
		return NULL;
	}

	if (*ptr++ != '<')
	{
		return NULL;
	}

	ptr = cgi_skip_blank(ptr);

	if (my_strncasecmp(ptr, "option", 6) != 0)
	{
		return NULL;
	}

	ptr += 6;

	tmp = strchr(ptr, '=');
	if (!tmp)
	{
		return NULL;
	}

	if (tmp > p + ptrlen)
	{
		return NULL;
	}

	ptr = cgi_skip_blank(ptr);
	if (my_strncasecmp(ptr, "value", 5) != 0)
	{
		return NULL;
	}

	ptr += 5;

	ptr = cgi_skip_blank(ptr);

	if (ptr != tmp)
	{
		return NULL;
	}

	ptr++; 		// skip =

	ptr = cgi_skip_blank(ptr);
	if (*ptr != '"')
	{
		return NULL;
	}
	ptr++;
	i = 0;
	while (*ptr != '"')
	{
		value[i++] = *ptr++;

		if (i >= len - 1)
		{
			break;
		}
	}

	value[i] = '\0';

	return value;
}
