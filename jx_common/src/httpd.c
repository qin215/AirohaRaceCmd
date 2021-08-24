/*
 * Copyright (c) 2004, Adam Dunkels.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
 
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include "windows.h"
#else
#include "contiki-net.h"
#include "systemconf.h"
#include "socket_driver.h"
#include <gpio_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwm_api.h>
#include <serial_api.h>
#include "sys_status_api.h"
#include "contiki-net.h"

#include "Cabrio-conf.h"
#include "atcmd.h"
#include "atcmd_icomm.h"  //these are all icomm-semi only debug command!!
#include "dbg-printf.h"
#include "atcmdlib.h"
#include "send_raw_packet.h"
#endif

#include "webserver-nogui.h"
#include "httpd-fs.h"
#include "httpd-cgi.h"
//#include "petsciiconv.h"
#include "http-strings.h"

#include "httpd.h"
#include "data_buff.h"

#ifndef WEBSERVER_CONF_CGI_CONNS
#define CONNS 4 //UIP_CONNS
#else /* WEBSERVER_CONF_CGI_CONNS */
#define CONNS WEBSERVER_CONF_CGI_CONNS
#endif /* WEBSERVER_CONF_CGI_CONNS */

#define STATE_WAITING 0
#define STATE_OUTPUT  1

#define SEND_STRING(s, str) PSOCK_SEND(s, (uint8_t *)str, (unsigned int)strlen(str))
//MEMB(conns, struct httpd_state, CONNS);

#define ISO_nl      0x0a
#define ISO_space   0x20
#define ISO_bang    0x21
#define ISO_percent 0x25
#define ISO_period  0x2e
#define ISO_slash   0x2f
#define ISO_colon   0x3a

void webserver_log(char *msg);
//void webserver_log_file(uip_ipaddr_t *requester, char *file);

#if 0
PROCESS(tcp_httpd_server_process, "my_web_server");
#endif


#if 0
/*---------------------------------------------------------------------------*/
static unsigned short
generate(void *state)
{
  struct httpd_state *s = (struct httpd_state *)state;

  if (s->file.len > uip_mss())
  {
    s->len = uip_mss();
  }
  else 
  {
    s->len = s->file.len;
  }
  memcpy(uip_appdata, s->file.data, s->len);
  
  return s->len;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_file(struct httpd_state *s))
{
	PSOCK_BEGIN(&s->sout);

	do 
	{
		PSOCK_GENERATOR_SEND(&s->sout, generate, s);
		s->file.len -= s->len;
		s->file.data += s->len;
	} while (s->file.len > 0);

	PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_part_of_file(struct httpd_state *s))
{
	PSOCK_BEGIN(&s->sout);

	PSOCK_SEND(&s->sout, (uint8_t *)s->file.data, s->len);

	PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
#endif

static void
next_scriptstate(sock_server_t *s)
{
	char *p;
	char *ptr;

	ptr = s->scriptptr;
	if (*(ptr - 1) == ISO_colon)
	{
		p = strchr(s->scriptptr, ISO_nl);
	}
	else
	{
		for (p = s->scriptptr; *p; p++)
		{
			if (*p == ISO_space || *p == ISO_nl)
			{
				break;
			}
		}

		if (*p == '\0')
		{
			p = NULL;
		}

/*		
		p = strchr(s->scriptptr, ISO_space);
		if (!p)
		{
			p = strchr(s->scriptptr, ISO_nl);
		}
*/		
	}
	
	if (p)
	{
		p += 1;
		s->scriptlen -= (unsigned short)(p - s->scriptptr);
		s->scriptptr = p;
	} 
	else 
	{
		s->scriptlen = 0;
	}
	/*  char *p;
	p = strchr(s->scriptptr, ISO_nl) + 1;
	s->scriptlen -= (unsigned short)(p - s->scriptptr);
	s->scriptptr = p;*/
}

#if 0
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_script(struct httpd_state *s))
{
	char *ptr;

	printf("line=%d, lc=%d, s=%s\n", __LINE__, s->scriptpt.lc, s->scriptptr);
	PT_BEGIN(&s->scriptpt);

	while (s->file.len > 0) 
	{
		/* Check if we should start executing a script. */
		if (*s->file.data == ISO_percent &&
				*(s->file.data + 1) == ISO_bang) 
		{
			s->scriptptr = s->file.data + 3;
			s->scriptlen = s->file.len - 3;
			
			if (*(s->scriptptr - 1) == ISO_colon) 
			{
				httpd_fs_open(s->scriptptr + 1, &s->file);
				printf("line=%d, lc=%d\n", __LINE__, s->scriptpt.lc);
				PT_WAIT_THREAD(&s->scriptpt, send_file(s));
			} 
			else 
			{
				PT_WAIT_THREAD(&s->scriptpt, httpd_cgi(s->scriptptr)(s, s->scriptptr));
			}
			next_scriptstate(s);
			printf("line=%d, lc=%d\n", __LINE__, s->scriptpt.lc);
			/* The script is over, so we reset the pointers and continue
			sending the rest of the file. */
			s->file.data = s->scriptptr;
			s->file.len = s->scriptlen;
		} 
		else 
		{
			/* See if we find the start of script marker in the block of HTML
			to be sent. */
			if (s->file.len > uip_mss())
			{
				s->len = uip_mss();
			} 
			else
			{
				s->len = s->file.len;
			}

			if (*s->file.data == ISO_percent) 
			{
				ptr = strchr(s->file.data + 1, ISO_percent);
			}
			else 
			{
				ptr = strchr(s->file.data, ISO_percent);
			}
			
			if (ptr != NULL && ptr != s->file.data)
			{
				s->len = (int)(ptr - s->file.data);
				if (s->len >= uip_mss())
				{
					s->len = uip_mss();
				}
			}
			
			PT_WAIT_THREAD(&s->scriptpt, send_part_of_file(s));
			
			s->file.data += s->len;
			s->file.len -= s->len;
		}
	}

	PT_END(&s->scriptpt);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_headers(struct httpd_state *s, const char *statushdr))
{
	/* gcc warning if not initialized. 
	* If initialized, minimal-net platform segmentation fault if not static...
	*/
	static const char *ptr = NULL;

	PSOCK_BEGIN(&s->sout);

	SEND_STRING(&s->sout, statushdr);

	ptr = strrchr(s->filename, ISO_period);
	if (ptr == NULL)
	{
		ptr = http_content_type_binary;
	} 
	else if (strncmp(http_html, ptr, 5) == 0 || strncmp(http_shtml, ptr, 6) == 0)
	{
		ptr = http_content_type_html;
	}
	else if (strncmp(http_css, ptr, 4) == 0) 
	{
		ptr = http_content_type_css;
	} 
	else if (strncmp(http_png, ptr, 4) == 0)
	{
		ptr = http_content_type_png;
	} 
	else if (strncmp(http_gif, ptr, 4) == 0)
	{
		ptr = http_content_type_gif;
	}
	else if (strncmp(http_jpg, ptr, 4) == 0)
	{
		ptr = http_content_type_jpg;
	} 
	else
	{
		ptr = http_content_type_plain;
	}
	SEND_STRING(&s->sout, ptr);
	PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_output(struct httpd_state *s))
{
	char *ptr;
	printf("line=%d, lc=%d\n", __LINE__, s->outputpt.lc);
	PT_BEGIN(&s->outputpt);
	printf("line=%d\n", __LINE__);
	if (!httpd_fs_open(s->filename, &s->file)) 
	{
		strcpy(s->filename, http_404_html);
		httpd_fs_open(s->filename, &s->file);
		printf("line=%d\n", __LINE__);
		PT_WAIT_THREAD(&s->outputpt,send_headers(s, http_header_404));
		printf("line=%d\n", __LINE__);
		PT_WAIT_THREAD(&s->outputpt, send_file(s));
		printf("line=%d\n", __LINE__);
	}
	else
	{
		printf("line=%d\n", __LINE__);
		PT_WAIT_THREAD(&s->outputpt, send_headers(s, http_header_200));
		printf("line=%d\n", __LINE__);
		ptr = strrchr(s->filename, ISO_period);
		if (ptr != NULL && strncmp(ptr, http_shtml, 6) == 0) 
		{
			PT_INIT(&s->scriptpt);
			printf("line=%d\n", __LINE__);
			PT_WAIT_THREAD(&s->outputpt, handle_script(s));
			printf("line=%d\n", __LINE__);
		} 
		else
		{
			printf("line=%d\n", __LINE__);
			PT_WAIT_THREAD(&s->outputpt, send_file(s));
			printf("line=%d\n", __LINE__);
		}
		printf("line=%d\n", __LINE__);
	}
	printf("line=%d\n", __LINE__);
	PSOCK_CLOSE(&s->sout);
	printf("line=%d\n", __LINE__);
	PT_END(&s->outputpt);
	printf("line=%d\n", __LINE__);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_input(struct httpd_state *s))
{
	PSOCK_BEGIN(&s->sin);
	printf("line=%d\n", __LINE__);
	PSOCK_READTO(&s->sin, ISO_space);
	printf("line=%d\n", __LINE__);
	if (strncmp(s->inputbuf, http_get, 4) != 0) 
	{
		PSOCK_CLOSE_EXIT(&s->sin);
	}
	PSOCK_READTO(&s->sin, ISO_space);

	if (s->inputbuf[0] != ISO_slash)
	{
		PSOCK_CLOSE_EXIT(&s->sin);
	}

	if (s->inputbuf[1] == ISO_space) 
	{
		strncpy(s->filename, http_index_html, sizeof(s->filename));
	}
	else 
	{
		s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = 0;
		strncpy(s->filename, s->inputbuf, sizeof(s->filename));
	}

	petsciiconv_topetscii(s->filename, sizeof(s->filename));
	webserver_log_file(&uip_conn->ripaddr, s->filename);
	petsciiconv_toascii(s->filename, sizeof(s->filename));
	s->state = STATE_OUTPUT;

	while (1) 
	{
		PSOCK_READTO(&s->sin, ISO_nl);
		printf("line=%d\n", __LINE__);
		if (strncmp(s->inputbuf, http_referer, 8) == 0)
		{
			s->inputbuf[PSOCK_DATALEN(&s->sin) - 2] = 0;
			petsciiconv_topetscii(s->inputbuf, PSOCK_DATALEN(&s->sin) - 2);
			webserver_log(s->inputbuf);
		}
	}
	PSOCK_END(&s->sin);
}
/*---------------------------------------------------------------------------*/
static void
handle_connection(struct httpd_state *s)
{
	printf("line=%d\n", __LINE__);
	handle_input(s);
	printf("line=%d\n", __LINE__);
	if (s->state == STATE_OUTPUT)
	{
		handle_output(s);
	}
	printf("line=%d\n", __LINE__);
}

/*---------------------------------------------------------------------------*/
void
httpd_appcall(void *state)
{
	struct httpd_state *s = (struct httpd_state *)state;

	printf("state=%d\n", s->state);

	if (uip_closed() || uip_aborted() || uip_timedout()) 
	{
		if (s != NULL)
		{
			memb_free(&conns, s);
		}
	} 
	else if (uip_connected()) 
	{
		s = (struct httpd_state *)memb_alloc(&conns);
		if (s == NULL) 
		{
			uip_abort();
			return;
		}

		printf("line=%d\n", __LINE__);
		tcp_markconn(uip_conn, s);
		PSOCK_INIT(&s->sin, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);
		printf("line=%d\n", __LINE__);
		PSOCK_INIT(&s->sout, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);
		PT_INIT(&s->outputpt);
		s->state = STATE_WAITING;
		/*    timer_set(&s->timer, CLOCK_SECOND * 100);*/
		s->timer = 0;
		printf("line=%d\n", __LINE__);
		handle_connection(s);
	} 
	else if (s != NULL)
	{
		if (uip_poll()) 
		{
			++s->timer;
			if(s->timer >= 20) 
			{
				uip_abort();
				memb_free(&conns, s);
			}
		} 
		else 
		{
			s->timer = 0;
		}

		printf("line=%d\n", __LINE__);
		handle_connection(s);
	}
	else
	{
		uip_abort();
	}
}
#endif

/*---------------------------------------------------------------------------*/
#define MAX_CLIENT 3

static sock_server_t server_array[MAX_CLIENT];
static void do_with_recv_queue(sock_server_t *pserver);
static void web_send_ok_callback(sock_server_t *pserver, tcp_send_cb fn);
static  void net_tcp_send_cb(int socket, char *buff, int len, Boolean resend);

int httpd_token_time = -1;

static void init_sock_array(void)
{
	int i;
	
	for (i = 0; i < MAX_CLIENT; i++)
	{
		memset(&server_array[i], 0, sizeof(sock_server_t));
		server_array[i].sock = -1;
		init_buff_queue(&(server_array[i].rx_queue), "rxq");
		init_buff_queue(&(server_array[i].tx_queue), "txq");
	}
}

static int get_free_sock_slot()
{
	int i;
	sock_server_t *pserver;

	pserver = &server_array[0];
	for (i = 0; i < MAX_CLIENT; i++, pserver++)
	{
		if (pserver->sock == -1)
		{
			return i;
		}
	}

	return -1;
}

static sock_server_t* get_sock_slot(int sock)
{
	int i;
	sock_server_t *pserver;

	pserver = &server_array[0];
	for (i = 0; i < MAX_CLIENT; i++, pserver++)
	{
		if (pserver->sock == sock)
		{
			return pserver;
		}
	}

	return NULL;
}

static Boolean put_sock_slot(int sock)
{
	int i;
	sock_server_t *pserver;

	pserver = &server_array[0];
	for (i = 0; i < MAX_CLIENT; i++, pserver++)
	{
		if (pserver->sock == sock)
		{
			pserver->sock = -1;
			free_queue_buff(&pserver->rx_queue);
			free_queue_buff(&pserver->tx_queue);
			printf("free sock:%d\n", sock);
			return TRUE;
		}
	}

	return FALSE;
}

static char mybuff[128];

#if 0
PROCESS_THREAD(tcp_httpd_server_process, ev, data)
{
    SOCKETMSG msg;
    
    PROCESS_BEGIN();
    icomprintf(GROUP_USER1, LOG_INFO, "tcp_httpd_server_process begin\n");
    
    while (1)
	{
        PROCESS_WAIT_EVENT();
		
        if (ev == PROCESS_EVENT_EXIT)
		{
            break;
        }
		else if (ev == PROCESS_EVENT_TIMER)
		{
			
		}
		else if (ev == PROCESS_EVENT_MSG) 
		{
            msg = *(SOCKETMSG *)data;
            if (msg.status == SOCKET_CLOSED) 
			{
               	put_sock_slot(msg.socket);
                icomprintf(GROUP_USER1, LOG_INFO, "socked:%d port:%d closed\n", (int)msg.socket, msg.lport);
            }
			else if (msg.status == SOCKET_SENDACK) 
			{
				sock_server_t *pserver;
				buf_t *b;
				
				printf("socked:%d port:%d send data ack\n", (int)msg.socket, msg.lport);
				pserver = get_sock_slot(msg.socket);
				if (pserver)
				{
                	web_send_ok_callback(pserver, net_tcp_send_cb);
				}
				else
				{
					printf("sock:%d not register!\n", msg.socket);
				}
            }
			else if (msg.status == SOCKET_NEWDATA)
			{
                if (0 <= msg.socket && msg.socket < UIP_CONNS)
				{
					int len;
					sock_server_t *pserver;
					buf_t *b;
					
					pserver = get_sock_slot(msg.socket);
					if (!pserver)
					{
						printf("no register!\n");
						for (;;)
						{
							len = tcprecv(msg.socket, &mybuff[0], sizeof(mybuff));
							printf("len = %d\n", len);
							if (len < 0)
							{
								printf("recv erro!\n");
								break;
							}
							else if (len == 0)
							{
								break;
							}
						}

						continue;
					}

					for (;;)
					{
						b = alloc_buffer();
						len = tcprecv(msg.socket, &b->pbuff[0], BUF_SIZE - 1);
						if (len <= 0)
						{
							free_buffer(b);
							printf("recv done!\n");
							break;
						}
						
						b->len = len;
//						print_buffer(b);
						add_buf(&(pserver->rx_queue), b);
					}

					do_with_recv_queue(pserver);
                }
				else
				{
                    icomprintf(GROUP_USER1,LOG_ERROR, "Illegal socket:%d\n", (int)msg.socket);
                }
            } 
			else if (msg.status == SOCKET_NEWCONNECTION)
			{
                int i;
				printf("new connected to listen port(%d), socket:%d\n", msg.lport, (int)msg.socket);
				i = get_free_sock_slot();
				if (i < 0)
				{
					printf("client full!\n");
					tcpclose(msg.socket);
					continue;
				}

				server_array[i].sock = msg.socket;
            } 
			else
			{
                tcpclose(msg.socket);
                icomprintf(GROUP_USER1, LOG_ERROR, "unknow message type: %d\n", msg.status);
            }
        }	
    }  
    
    icomprintf(GROUP_USER1, LOG_INFO, "tcp_ota_server_Process end\n");
    PROCESS_END();
}

static  void net_tcp_send_cb(int socket, char *buff, int len, Boolean resend)
 {
	U8 status = ERROR_SUCCESS;

	//printf("sok = %d, STR = %s, len = %d\n", socket, buff, len);
    if (tcpsend(socket, buff, len) == -1) 
	{
        printf("send data fail\n");
    } 
	else 
	{
        printf("send data ok\n");
    }
 }


/*
 * 将发送队列的buff通过TCP发送出去
 */
static void web_send_tx_queue(sock_server_t *pserver, int resend, tcp_send_cb fn)
{
	buf_t *b;

	if (pserver->sock < 0)
	{
		printf("no valid sock!\n");
		return;
	}
	
	b = get_buf(&pserver->tx_queue);
	if (!b)
	{
		printf("tx index = %d\n", pserver->tx_queue.index);
		
		return;
	}

	if (fn)
	{
		fn(pserver->sock, b->pbuff, b->len, resend);
	}
}

/*
 * TCP 发送成功
 */
static void web_send_ok_callback(sock_server_t *pserver, tcp_send_cb fn)
{
	buf_t *b;
	
	b = remove_buf(&pserver->tx_queue);
	if (!b)
	{
		printf("tcp_send_ok_callback b == NULL @%d\n", __LINE__);
	}
	else
	{
		free_buffer(b);
	}

	// 发送队列中还有数据，继续发送
	if (get_buf(&pserver->tx_queue))
	{
		web_send_tx_queue(pserver, FALSE, fn);
	}
	else
	{
		tcpclose(pserver->sock);
		put_sock_slot(pserver->sock);
	}
}
#endif

static void append_html_header(sock_server_t *pserver, buf_t *b, const char *statushdr)
{
	/* gcc warning if not initialized. 
	* If initialized, minimal-net platform segmentation fault if not static...
	*/
	const char *ptr = NULL;
	add_string(b, statushdr);

	ptr = strrchr(pserver->filename, ISO_period);
	if (ptr == NULL)
	{
		ptr = http_content_type_binary;
	} 
	else if (strncmp(http_html, ptr, 5) == 0 || strncmp(http_shtml, ptr, 6) == 0)
	{
		ptr = http_content_type_html;
	}
	else if (strncmp(http_css, ptr, 4) == 0) 
	{
		ptr = http_content_type_css;
	} 
	else if (strncmp(http_png, ptr, 4) == 0)
	{
		ptr = http_content_type_png;
	} 
	else if (strncmp(http_gif, ptr, 4) == 0)
	{
		ptr = http_content_type_gif;
	}
	else if (strncmp(http_jpg, ptr, 4) == 0)
	{
		ptr = http_content_type_jpg;
	} 
	else
	{
		ptr = http_content_type_plain;
	}
	add_string(b, ptr);

	//print_buffer(b);
}

char * my_strnchr(char *p, int c, int n)
{
	int i;

	for (i = 0; i < n; i++, p++)
	{
		if (*p == c)
		{
			return p;
		}
	}

	return NULL;
}

static buf_t * append_script_file(sock_server_t *pserver, buf_t *b)
{
	sock_server_t *s = pserver;
	char *ptr;
	buf_t *ret;
	
	if (!s)
	{
		printf("server null!\n");
		return NULL;
	}

	while (s->file.len > 0) 
	{
		if (*s->file.data == '<')
		{
			s->left_bracket = s->file.data;
		}
			
		/* Check if we should start executing a script. */
		if (*s->file.data == ISO_percent && \
			*(s->file.data + 1) == ISO_bang) 
		{
			char buff[32];
			int n;
			
			s->scriptptr = s->file.data + 3;
			s->scriptlen = s->file.len - 3;

			strncpy(buff, s->scriptptr, 32);
			printf("scriptname='%s'\n", buff);

			if (*(s->scriptptr - 1) == ISO_colon) 
			{
				struct httpd_fs_file file;

				file = s->file;		// backup it.
				
				httpd_fs_open(s->scriptptr + 1, &s->file);
				if ((ret = append_file(s, b)))
				{
					b = ret;
				}
				else
				{
					printf("something error@%d\n", __LINE__);
				}

				s->file = file;
			} 
			else 
			{
				httpd_cgifunction f;

				s->bracket_len = s->scriptptr - s->left_bracket;
				f = httpd_cgi(s->scriptptr);
				if (!(ret = f(s, b, s->scriptptr)))
				{
					printf("something error@%d\n", __LINE__);
				}
				else
				{
					b = ret;
				}
			}
			
			next_scriptstate(s);
			/* The script is over, so we reset the pointers and continue
			sending the rest of the file. */
			s->file.data = s->scriptptr;
			s->file.len = s->scriptlen;
		} 
		else 
		{
			struct httpd_fs_file file;

			if (*s->file.data == ISO_percent) 
			{
				ptr = my_strnchr(s->file.data + 1, ISO_percent, s->file.len - 1);
			}
			else
			{
				ptr = my_strnchr(s->file.data, ISO_percent, s->file.len);
			}

			file = s->file;

			if (ptr > s->file.data + s->file.len)		// 溢出了
			{
				ptr = NULL;
			}

			if (ptr != NULL && s->file.data != ptr)
			{
				s->file.len = ptr - s->file.data;
			}

			if (!(ret = append_file(s, b)))
			{
				printf("something erro!\n");
			}
			else
			{
				b = ret;
			}

			if (ptr)
			{
				s->file.len = file.len - (ptr - file.data);
				s->file.data = file.data + (ptr - file.data);
			}
		}
	}

//	print_buffer(b);
//	add_buf(&pserver->tx_queue, b);

	return b;
}

static char *find_last_left_bracket(char *p, int len)
{
	char *ptr = NULL;
	if (len <= 0)
	{
		return NULL;
	}

	while (len--)
	{
		if (*p == '<')
		{
			ptr = p;
		}
		
		p++;
	}

	return ptr;
}

buf_t *append_file(sock_server_t *pserver, buf_t *b)
{
	const char *ptr = NULL;
	int len;
	buf_t *nb;

	if (b->len + pserver->file.len >= BUF_SIZE)
	{
		int left;
		
		nb = alloc_buffer();
		if (!nb)
		{
			printf("no buff:%d\n", __LINE__);
			return NULL;
		}

		left = BUF_SIZE - b->len;
		memcpy(&b->pbuff[b->len], pserver->file.data, left);
		b->len += left;
		add_buf(&pserver->tx_queue, b);
		pserver->left_bracket = find_last_left_bracket(pserver->file.data, left);
//		print_buffer(b);
		pserver->file.data += left;
		pserver->file.len -= left;

		b = nb;
	}

	if (b->len + pserver->file.len > BUF_SIZE)
	{
		printf("buff is low=@d\n", __LINE__);
		return NULL;
	}
	
	memcpy(&b->pbuff[b->len], pserver->file.data, pserver->file.len);
	b->len += pserver->file.len;
	pserver->left_bracket = find_last_left_bracket(pserver->file.data, pserver->file.len);
//	add_buf(&pserver->tx_queue, b);
//	print_buffer(b);
	pserver->file.len = 0;
	pserver->file.data += pserver->file.len;
	return b;
}


static void send_http_reply_packet(sock_server_t *pserver)
{
	buf_t *b;
	char *ptr;
	buf_t *ret;
	
	if (!httpd_fs_open(pserver->filename, &pserver->file)) 
	{
		strcpy(pserver->filename, http_404_html);
		httpd_fs_open(pserver->filename, &pserver->file);
		b = alloc_buffer();
		if (!b)
		{
			printf("no buffer!\n");
			return;
		}
		append_html_header(pserver, b,  http_header_404);
		if (add_buf(&pserver->tx_queue, b))
		{
//			web_send_tx_queue(pserver, FALSE, net_tcp_send_cb);
		}
		else
		{
			printf("error:add buff failed!\r\n");
		}
	}
	else
	{
		b = alloc_buffer();
		if (!b)
		{
			printf("no buffer!\n");
			return;
		}
		append_html_header(pserver, b,  http_header_200);
		
		ptr = strrchr(pserver->filename, ISO_period);
		if (ptr != NULL && strncmp(ptr, http_shtml, 6) == 0) 
		{
			ret = append_script_file(pserver, b);
		} 
		else
		{
			ret = append_file(pserver, b);
		}

		if (ret)
		{
			add_buf(&pserver->tx_queue, ret);
		}
		
	//	web_send_tx_queue(pserver, FALSE, net_tcp_send_cb);
	}
}

static Boolean httpd_check_login_timeout()
{
	unsigned long s = 11;//clock_seconds();

	if (httpd_token_time < 0)
	{
		return TRUE;
	}

	if ((s - httpd_token_time) < 10 * 60)		// 10 Minutes 超时
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
static Boolean httpd_check_login_auth_token_help(char *cookie, int len)
{
	httpd_container_t* pcon;
	httpd_kv_t kv;
	httpd_kv_t *pkv;

	if (!httpd_get_kv(cookie, &cookie[len], &kv))
	{
		return FALSE;
	}

	if (httpd_find_container(COOKIE_USER_TOKEN_STR, &pkv, &pcon) < 0)
	{
		return FALSE;
	}

	printf("pkv->value=%s, kv.value=%s\n", pkv->value, kv.value);

	if (strcmp(pkv->value, kv.value) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

Boolean httpd_check_login_auth_token(char *cook, int len)
{
	char *p;
	char *ptr = cook;
	Boolean ret = FALSE;
	
	int bk1;	// for backup
	char *bk1_ptr = NULL;
	
	int bk2 = cook[len];
	char *bk2_ptr = &cook[len];

	if (!cook)
	{
		return FALSE;
	}

	// backup the char.
	cook[len] = '\0';
	bk1_ptr = strstr(cook, "\r\n");
	if (bk1_ptr)
	{
		bk1 = *bk1_ptr;
		*bk1_ptr = '\0';
	}

	for (;;)
	{
		p = strchr(ptr, ';');
		if (p)
		{
			int c;
			
			c = *p;
			*p = '\0';
			ret = httpd_check_login_auth_token_help(ptr, p - ptr - 1);
			*p = c;

			if (ret)
			{
				break;
			}

			ptr = p + 1;		// skip the blank.
			while (*ptr && *ptr == ' ') 
			{
				ptr++;
			}
		}
		else
		{
			if (bk1_ptr)
			{
				p = bk1_ptr;
			}
			else
			{
				p = bk2_ptr;
			}
			
			ret = httpd_check_login_auth_token_help(ptr, p - ptr - 1);
			break;
		}
	}

	// restore the char
	*bk2_ptr = bk2;
	if (bk1_ptr)
	{
		*bk1_ptr = bk1;
	}

	return ret;
}

Boolean httpd_check_css_file(const char *filename)
{
	const char *ptr = NULL;

	ptr = strrchr(filename, ISO_period);
	if (ptr == NULL)
	{
		return FALSE;
	} 
	
	if (strncmp(http_css, ptr, 4) == 0) 
	{
		return TRUE;
	}

	return FALSE;
}

static void do_with_recv_queue(sock_server_t *pserver)
{
	char *p;
	char *ptr;
	int len;
	
	//print_queue(&pserver->rx_queue);
	p = pserver->rx_queue.head->pbuff;
	len = pserver->rx_queue.head->len;

	p[len] = '\0';

	if (!strstr(p, "\r\n\r\n"))
	{
		return TRUE;
	}

	while (*p == ISO_space)
	{
		p++;
	}
	
	if (strncmp(p, http_get, 4) != 0)
	{
		printf("not http header1!\n");
	//	tcpclose(pserver->sock);
		goto done;
	}

	p += 4;		// skip GET
	if (*p != ISO_slash)
	{
		printf("not http header3!\n");
//		tcpclose(pserver->sock);
		goto done;
	}
	
	if (*(p + 1) == ISO_space) 
	{
		if (httpd_check_login_timeout())
		{
			strncpy(pserver->filename, "/login.html", sizeof(pserver->filename));
		}
		else
		{
			strncpy(pserver->filename, http_index_html, sizeof(pserver->filename));
		}
	}
	else
	{
		int n;
		char *begin;
		char *end;
		char *next_line;
		int c;

		next_line = httpd_get_segment(p, len, &begin, &end, "\r\n");
		if (!next_line)
		{
			printf("not valid header!\n");
			goto done;
		}

		if (!httpd_parse_get_req(begin, end, &pserver->http_req))
		{
			printf("not http header4!\n");
//			tcpclose(pserver->sock);
		}

		strncpy(pserver->filename, pserver->http_req.filepath, /*HTTPD_PATH_LEN*/64);
#if 0		
		ptr = strchr(p, ISO_space);
		if (!ptr)
		{
			printf("not http header4!\n");
//			tcpclose(pserver->sock);
			goto done;
		}

		n = ptr - p;
		if (n >= sizeof(pserver->filename))
		{
			n = sizeof(pserver->filename) - 1;
			printf("filename too long!\n");
		}
		strncpy(pserver->filename, p, n);
		pserver->filename[n] = '\0';
#endif
	}
	printf("filename:%s\n", pserver->filename);

	ptr = strstr(p, "\r\n");
	if (!ptr)
	{
		printf("not http header5!\n");
		//tcpclose(pserver->sock);
		goto done;
	}
	if (httpd_check_login_timeout() && \
		strcmp(pserver->filename, "/login.shtml") != 0 && \
		!httpd_check_css_file(pserver->filename))
	{
		strncpy(pserver->filename, "/login.html", sizeof(pserver->filename));
	}

	// output
	send_http_reply_packet(pserver);
done:	
	free_queue_buff(&pserver->rx_queue);
}

void
httpd_init(void)
{
	//memb_init(&conns);
	httpd_cgi_init();
	init_sock_array();
#if 0
	process_start(&tcp_httpd_server_process, NULL);
	if (tcplisten(80, &tcp_httpd_server_process) == -1)
	{
		printf("listen 80 failed!\n");
	}
#endif	
}

static const unsigned char ascii2petscii[128] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x14,0x09,0x0d,0x11,0x93,0x0a,0x0e,0x0f,
	0x10,0x0b,0x12,0x13,0x08,0x15,0x16,0x17,
	0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
	0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
	0xd8,0xd9,0xda,0x5b,0x5c,0x5d,0x5e,0x5f,
	0xc0,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
	0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
	0x58,0x59,0x5a,0xdb,0xdd,0xdd,0x5e,0xdf,
};

void petsciiconv_toascii(char *buf, unsigned int len)
{
	unsigned int i;
	char *ptr;
	char c;

	ptr = buf;
	for (i = len; i > 0; --i) 
	{
		c = *ptr;
		if (c == 0x0a) 
		{
			c = 0x0d;
		}
		else if (c == 0x0d)
		{
			c = 0x0a;
		}

		if (c != 0x40) 
		{
			switch (c & 0xe0)
			{
				case 0x40:                
				case 0x60:
					c ^= 0x20;
					break;
					
				case 0xc0:               
					c ^= 0x80;
					break;
			}
		}

		*ptr = c & 0x7f;
		++ptr;
	}
}

void petsciiconv_topetscii(char *buf, unsigned int len)
{
	unsigned int i;
	char *ptr;

	ptr = buf;
	for (i = len; i > 0; --i) 
	{
		*ptr = ascii2petscii[*ptr & 0x7f];
		++ptr;
	}
}

char * cgi_get_option_value(char *p, int ptrlen, char *value, int len);
void test_cookie();
char * my_strstr(char *ptr, int len, char *str);

void test_httpd()
{
	buf_t *b;
	sock_server_t *pserver;
	char value[128];
	char *input = "/do.cgi?name=WiFi-AP&password=12345678&encrypt=w\r\n";
	httpd_get_req_t req;
	char *begin;
	char *end;
	char *next_line;
	int c;
	//char *input = "<option value=\"open\" %! check_encrypt >open</option>";
	char http_str[] = "GET /main.css HTTP/1.1\r\nHost: 192.168.0.10..Connection: keep-alive\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8..User-Agent: Mozilla/5.0 "
		"(Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/601.7.7 (KHTML, like Gecko) Version/9.1.2 Safari/601.7.7\r\n"
		"Accept-Language: zh-cn\r\nReferer: http://192.168.0.10/\r\nAccept-Encoding: gzip, deflate\r\n\r\n";

	httpd_init();
	
	b = alloc_buffer();

	pserver = server_array;
	strcpy(pserver->filename, "/status.shtml");

	strcpy(b->pbuff, http_str);
	b->len = strlen(http_str);

	add_buf(&pserver->rx_queue, b);
	do_with_recv_queue(pserver);

	//cgi_get_option_value(input, strlen(input), value, sizeof(value));
	//send_http_reply_packet(pserver);

	test_cookie();

	end = my_strstr(http_str, strlen(http_str), "\r\n");

#if 0
	next_line = httpd_get_segment(input, &begin, &end, "\r\n");
	if (!next_line)
	{
		printf("not valid header!\n");
	}
	else
	{
		httpd_parse_get_req(begin, end, &req);
	}
#endif
}

char *httpd_trim_blank(char *begin, char *end)
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

char * my_strstr(char *ptr, int len, char *str)
{
	int c;
	int i;
	int nlen = strlen(str);

	for (i = 0; i < len; i++)
	{
		if (ptr[i] == *str)
		{
			if (strncmp(&ptr[i], str, nlen) == 0)
			{
				return &ptr[i];
			}
		}
	}
	
	return NULL;
}
char *httpd_get_segment(char *ptr, int string_len, char **p_begin, char **p_end, const char *sep)
{
	char *begin;
	char *end;

	begin = cgi_skip_blank(ptr);
	end = my_strstr(begin, string_len, sep);		// should sub 
	if (!end)
	{
		printf("no '%s'\r\n", sep);
		return NULL;
	}

	ptr = end + strlen(sep);

	end--;		// point to the last byte.
	end = httpd_trim_blank(begin, end);

	*p_end = end;
	*p_begin = begin;

	return (ptr);
}

Boolean httpd_get_kv(char *start, char *end, httpd_kv_t* pkv)
{
	char *next;
	char *sptr;
	char *eptr;
	int n;
	
	next = httpd_get_segment(start, end + 1 - start, &sptr, &eptr, "=");
	if (!next)
	{
		return FALSE;
	}

	memset(pkv, 0, sizeof(*pkv));
	n = sizeof(pkv->key);
	n = min(n - 1, eptr + 1 - sptr);
	strncpy(pkv->key, sptr, n);

	n = sizeof(pkv->value);
	n = min(n - 1, end + 1 - next);
	strncpy(pkv->value, next, n);

	return TRUE;
}

httpd_get_req_t * httpd_parse_parameter(char *start, char *end, httpd_get_req_t *pget)
{
	httpd_kv_t *parray;
	int index;
	char *sptr;
	char *eptr;
	char *next;

	for (index = 0; start != NULL; start = next)
	{
		parray = &pget->kv_array[index];

		next = httpd_get_segment(start, end + 1 - start, &sptr, &eptr, "&");
		if (!next)
		{
			sptr = start;
			eptr = end;
		}

		if (httpd_get_kv(sptr, eptr, parray))
		{
			index++;
		}

		if (index >= HTTPD_MAX_PARAM - 1)
		{
			printf("parameter too long!\n");
			break;
		}
	}

	parray = &pget->kv_array[index];
	parray->key[0] ='\0';

	return pget;
}

char *httpd_reverse_get_blank(char *begin, char *end)
{
	int c;

	if (begin == NULL || end == NULL)
	{
		printf("null ptr!\r\n");
		return NULL;
	}
	
	while (end > begin && *end != ISO_space)
	{
		end--;
	}

	if (end == begin)
	{
		return NULL;
	}

	return end;
}

void httpd_print_kv(httpd_kv_t* parray)
{
	while (parray->key[0] != '\0')
	{
		printf("k=%s,v=%s\n", parray->key, parray->value);
		parray++;
	}
}

///do.cgi?name=WiFi-AP&password=12345678&encrypt=w
httpd_get_req_t * httpd_parse_get_req(char *begin, char *end, httpd_get_req_t *pget)
{
	char *p;
	int n;
	memset(pget, 0, sizeof(httpd_get_req_t));
	
	if (!begin || !end || !pget)
	{
		return NULL;
	}

	if (!(end = httpd_reverse_get_blank(begin, end)))
	{
		return NULL;
	}

	end--;	// remove last blank
	
	for (p = begin; p < end; p++)
	{
		if (*p == '?')
		{
			break;
		}
	}
	
	memset(&pget->filepath[0], 0, sizeof(pget->filepath));
	// no action
	if (p == end)
	{
		n = end + 1 - begin;
		n = min(n, sizeof(pget->filepath) - 1);
		strncpy(pget->filepath, begin, n);
		pget->kv_array[0].key[0] = '\0';		// no parameters

		return pget;
	}
	// 
	n = p - begin;
	n = min(n, sizeof(pget->filepath) - 1);
	strncpy(pget->filepath, begin, n);

	httpd_parse_parameter(p + 1, end, pget);
	httpd_print_kv(pget->kv_array);
	return pget;
}
