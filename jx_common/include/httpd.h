/*
 * Copyright (c) 2001-2005, Adam Dunkels.
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

#ifndef HTTPD_H_
#define HTTPD_H_

#ifndef WIN32
#include "contiki-net.h"
#endif

#include "httpd-fs.h"
#include "data_buff.h"

#define MAX_CLIENT 3

#define MAX_COOKIE 3
typedef struct _httpd_container 
{
	httpd_kv_t cookie_array[MAX_COOKIE];		// 容器的cookies数组
	int time;			// 最近一次访问的时间
} httpd_container_t;


#define COOKIE_USER_TOKEN_STR "usertoken"


void httpd_init(void);
void httpd_appcall(void *state);

extern int httpd_token_time;

char *httpd_trim_blank(char *begin, char *end);
char *httpd_get_segment(char *ptr, int str_len, char **p_begin, char **p_end, const char *sep);
char *cgi_skip_blank(char *p);
httpd_get_req_t * httpd_parse_get_req(char *begin, char *end, httpd_get_req_t *pget);
buf_t *append_file(sock_server_t *pserver, buf_t *b);
void httpd_print_kv(httpd_kv_t* parray);

int httpd_find_container(char *cookie, httpd_kv_t** ppkv, httpd_container_t** ppcon);
httpd_container_t* httpd_update_cookie(char *key, char *value);
httpd_container_t * generate_auth_cookie();
void httpd_print_container();
Boolean httpd_check_login_auth_token(char *cook, int len);

#define isblank(c)	((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')

#if NETSTACK_CONF_WITH_IPV6
uint8_t httpd_sprint_ip6(uip_ip6addr_t addr, char * result);
#endif /* NETSTACK_CONF_WITH_IPV6 */

#endif /* HTTPD_H_ */
