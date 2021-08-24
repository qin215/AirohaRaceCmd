#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include "windows.h"
#else
#include "osal.h"
#include "lwipopts.h"
#include "netstack_def.h"
#include "sockets.h"
#include "ServerCmd.h"
#include "ms_hal_socket.h"
#include "ms_hal_os.h"
#endif

#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
//#include "net.h"
#include "hx_flash_opt.h"

#define MAX_NET_LINE_NR	3
static net_line_t net_array[MAX_NET_LINE_NR];

#ifdef WIN32 
SOCKET net_hekr_sock = -1;
#endif

#ifdef LINUX
static sem_t line_semaphore;
#elif defined(WIN32)
static HANDLE line_semaphore;
#else
static ms_hal_os_semaphore_t line_semaphore;
#endif



void init_net_buff()
{
	int i;

#ifdef LINUX
	if (sem_init(&line_semaphore, 0, 1) < 0)
	{
		printf("semaphonre init failed!\n");
	}
#elif defined(WIN32)
	line_semaphore = CreateSemaphore(NULL, 1, 1, "LineSemaphore");
#else
	if (OS_SUCCESS != OS_SemInit(&line_semaphore, 1, 1))
	{
		printf("sema init failed!!");
	}
#endif

	for (i = 0; i < MAX_NET_LINE_NR; i++)
	{
		net_array[i].sock = -1;

		init_buff_queue(&(net_array[i].tx_queue), "ntxq");
		init_buff_queue(&(net_array[i].rx_queue), "nrxq");
	}
}

/*
 * 判断网络线路是否繁忙
 */
Boolean net_check_line_busy(net_line_t *pnet)
{
	if (!pnet)
	{
		return FALSE;
	}

	if (pnet->sock < 0)
	{
		return FALSE;
	}

	return pnet->tx_busy;
}


void net_close_all_lines()
{
	int i;
	net_line_t *pnet = net_array;
	buf_t *b;

	for (i = 0; i < MAX_NET_LINE_NR; i++, pnet++)
	{
		if (pnet->sock >= 0)
		{
		#ifdef WIN32
			closesocket(pnet->sock);
		#else
			closesocket(pnet->sock);
		#endif		
		}
		
		pnet->sock = -1;
	}
}


/*
 * 将发送队列的buff通过TCP发送出去
 */
void net_send_tx_queue(int sock, int resend, tcp_send_cb fn)
{
	buf_t *b;
	net_line_t *pnet;

	//printf("%s sock = %d\r\n", __FUNCTION__, sock);

	if ((pnet = get_net_line(sock)) == NULL)
	{
		printf("not register?\n");
		return;
	}

	for (;;)
	{
		b = remove_buf(&pnet->tx_queue);
		if (!b)
		{
			printf("ntx null\n");
			return;
		}
		
		pnet->tx_busy = TRUE;

		//HX_DEBUG_BUFFER(b);
		//printf("tcp=%d, sock=%d\n", pnet->tcp, sock);

		if (pnet->tcp)
		{
			net_send_buffer(sock, b->pbuff, b->len, resend);
		}
		else
		{
			//printf("addr=%x, port=%x\n", pnet->addr.sin_addr, pnet->addr.sin_port);
			if (sendto(sock, b->pbuff, b->len, 0, (struct sockaddr *)&pnet->addr, sizeof(struct sockaddr_in)) < 0)
			{
				printf("udp send failed!\n");
			}
			else
			{
				printf("udp send ok!\n");
			}
		}
		
		pnet->tx_busy = FALSE;
		free_buffer(b);
	}
}

net_line_t* get_net_line(int sock)
{
	int i;
	net_line_t *pnet = net_array;

	if (sock < 0)
	{
		return NULL;
	}

	for (i = 0; i < MAX_NET_LINE_NR; i++, pnet++)
	{
		if (pnet->sock == sock)
		{
			return pnet;
		}
	}

	return NULL;
}

Boolean net_has_active_conn()
{
	int i;
	net_line_t *pnet = net_array;

	for (i = 0; i < MAX_NET_LINE_NR; i++, pnet++)
	{
		if (pnet->sock >= 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

net_line_t* register_net_line(int sock, Boolean tcp, struct sockaddr_in *paddr)
{
	int i;
	net_line_t *pnet = net_array;
	net_line_t *p;

#if defined(WIN32)
	WaitForSingleObject(line_semaphore, INFINITE);
#else
	ms_hal_os_semaphore_get(&line_semaphore, portMAX_DELAY);
#endif

	/* 先判断是否已经注册了*/
	if ((p = get_net_line(sock)) != NULL)
	{
#if defined(WIN32)
		ReleaseSemaphore(line_semaphore, 1, NULL);
#else	
		ms_hal_os_semaphore_put(&line_semaphore);
#endif		
		return p;
	}

	/* 未注册,找到一个slot注册 */
	for (i = 0; i < MAX_NET_LINE_NR; i++, pnet++)
	{
		if (pnet->sock == -1)
		{
			//memset(pnet, 0, sizeof(net_line_t));
			pnet->sock = sock;

			init_buff_queue(&pnet->rx_queue, "nrxq");
			init_buff_queue(&pnet->tx_queue, "ntxq");

			init_buffer(&pnet->rx_buff);
			init_buffer(&pnet->tx_buff);
			
			pnet->rx_buff.size = 1024 + 1024;		// 预先分配1.5k buffer, mtu 1472
#if defined(WIN32)
			pnet->rx_buff.pbuff = malloc(pnet->rx_buff.size);
#else
			pnet->rx_buff.pbuff = ms_hal_os_memory_alloc(pnet->rx_buff.size);
#endif		
			if (!(pnet->rx_buff.pbuff))
			{
				printf("malloc rx failed!\n");
			}

			pnet->tx_buff.size = 512;
#if defined(WIN32)
			pnet->tx_buff.pbuff = malloc(pnet->tx_buff.size);
#else
			pnet->tx_buff.pbuff = ms_hal_os_memory_alloc(pnet->tx_buff.size);
#endif
			if (!(pnet->tx_buff.pbuff))
			{
				printf("malloc tx failed!\n");
			}

			pnet->tx_busy = FALSE;
			pnet->tx_count = 0;
			pnet->tcp = tcp;
			pnet->addr = *paddr;
			
#if defined(WIN32)
		SetEvent(line_semaphore);
#else	
		ms_hal_os_semaphore_put(&line_semaphore);
#endif
			return pnet;
		}
	}

#if defined(WIN32)
		SetEvent(line_semaphore);
#else	
		ms_hal_os_semaphore_put(&line_semaphore);
#endif
	return NULL;
}

Boolean unregister_net_line(int sock)
{
	int i;
	net_line_t *pnet = net_array;


#if defined(WIN32)
	WaitForSingleObject(line_semaphore, INFINITE);
#else
	ms_hal_os_semaphore_get(&line_semaphore, portMAX_DELAY);
#endif
	
	for (i = 0; i < MAX_NET_LINE_NR; i++, pnet++)
	{
		if (pnet->sock == sock)
		{
			free_queue_buff(&pnet->tx_queue);
			free_queue_buff(&pnet->rx_queue);
			if (pnet->rx_buff.pbuff)
			{
#if defined(WIN32)
				free(pnet->rx_buff.pbuff);
#else			
				ms_hal_os_memory_free(pnet->rx_buff.pbuff);
#endif
				pnet->rx_buff.pbuff = NULL;
			}

			if (pnet->tx_buff.pbuff)
			{
#if defined(WIN32)
				free(pnet->tx_buff.pbuff);
#else
				ms_hal_os_memory_free(pnet->tx_buff.pbuff);
#endif
				pnet->tx_buff.pbuff = NULL;
			}
			
			pnet->sock = -1;
			
#if defined(WIN32)
			SetEvent(line_semaphore);
#else	
			ms_hal_os_semaphore_put(&line_semaphore);
#endif
			
			return TRUE;
		}
	}

#if defined(WIN32)
	SetEvent(line_semaphore);
#else	
	ms_hal_os_semaphore_put(&line_semaphore);
#endif

	return FALSE;
}

/*清空所有的buf信息*/
void unregister_net_line_all(void)
{
	int i;
	net_line_t *pnet = net_array;

#if defined(WIN32)
	WaitForSingleObject(line_semaphore, INFINITE);
#else
	ms_hal_os_semaphore_get(&line_semaphore, portMAX_DELAY);
#endif
	
	for (i = 0; i < MAX_NET_LINE_NR; i++, pnet++)
	{
		free_queue_buff(&pnet->tx_queue);
		free_queue_buff(&pnet->rx_queue);
		if (pnet->rx_buff.pbuff)
		{
#ifdef WIN32
			free(pnet->rx_buff.pbuff);
#else
			ms_hal_os_memory_free(pnet->rx_buff.pbuff);
#endif

			pnet->rx_buff.pbuff = NULL;
		}

		pnet->sock = -1;
	}
	
#if defined(WIN32)
	SetEvent(line_semaphore);
#else	
	ms_hal_os_semaphore_put(&line_semaphore);
#endif
}

#ifdef WIN32

UINT win32_recv_thread(LPVOID pParam)
{
	int len;
	net_line_t *pnet;
	buf_t *b;
#if defined(__HTTP_TEST__)
	Http_Header_Status s;
#endif
	pnet = get_net_line(net_hekr_sock);
	if (pnet == NULL)
	{
		printf("not register?\n");
		return -1;
	}

	b = &pnet->rx_buff;

	len = -1;
	b->len = 0;
	for (;;)
	{
		len = recv(net_hekr_sock, &b->pbuff[b->len], BUF_SIZE - b->len, 0);
		if (len > 0)
		{
			b->pbuff[b->len + len] = '\0';
		}
		printf("recv: len = %d, s = %s, b->len = %d\n", len, b->pbuff, b->len);
		if (len < 0)
		{
			printf("recv error = %d!\n", WSAGetLastError());
			break;
		}
		else if (len == 0)
		{
			break;
		}

        b->len += len;
#ifdef __HEKR_CLOUD_SUPPORT__
		hekr_parse_recv_buff(b);
#elif defined(__HX_IOT_SUPPORT__)
		if (hx_do_with_net_frame(b, net_hekr_sock) == HX_DATA_NOT_FULL)
		{
			continue;		// 需要接收更多的数据
		}
#elif defined(__OBD_OTA_TEST__)
		if (!ota_parse_recv_buff(net_hekr_sock, b))
		{
			continue;
		}
#elif defined(__OBD_RECV_TEST__) || defined(__UA402_RECV_TEST__)
		obd_log_recv_buff(b);
#if 1
		{
			int nbytes;
			int left;
			int total;

			for (left = b->len, total = 0; left > 0; )
			{
				nbytes = send(net_hekr_sock, &b->pbuff[total], left, 0);
				if (nbytes < 0 )
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						continue;
					}
					else
					{
						break;
					}
				}


				total += nbytes;
				left -= nbytes;	
			}
		}
#endif
#elif defined(__HTTP_TEST__)
		s = httprsp_parse(b->pbuff, b->len);
		if (s == HTTP_STATUS_GET_ALL || s == HTTP_STATUS_ERROR)
		{
			init_buffer(b);
			break;
		}
#elif defined(__TUYA_HTTP_TEST__)
		tuya_httpclient_recv_cb(b->pbuff, b->len);
#endif
		b->len = 0;
		init_buffer(b);
	}

	hekr_close_sock();

	//win32_net_connect(hekr_hostname, hekr_port, TRUE);

	return 0;
}


/*
 * 连接网络
 */
int win32_net_connect(char *hostname, int port, Boolean isLogin)
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	SOCKADDR_IN addrSrv;
	HOSTENT *host_entry;


	wVersionRequested = MAKEWORD(1,1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -1;
	}

	net_hekr_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (net_hekr_sock == INVALID_SOCKET)
	{
		printf("socket function failed with error = %d\n", WSAGetLastError() );
		return -1;
	}


	host_entry = gethostbyname(hostname);
	printf("%s\n", hostname);
	if (host_entry != 0)
	{
		printf("解析IP地址: ");
		printf("%d.%d.%d.%d",
			(host_entry->h_addr_list[0][0]&0x00ff),
			(host_entry->h_addr_list[0][1]&0x00ff),
			(host_entry->h_addr_list[0][2]&0x00ff),
			(host_entry->h_addr_list[0][3]&0x00ff));

		addrSrv.sin_addr.S_un.S_un_b.s_b1 = host_entry->h_addr_list[0][0]&0x00ff;
		addrSrv.sin_addr.S_un.S_un_b.s_b2 = host_entry->h_addr_list[0][1]&0x00ff;
		addrSrv.sin_addr.S_un.S_un_b.s_b3 = host_entry->h_addr_list[0][2]&0x00ff;
		addrSrv.sin_addr.S_un.S_un_b.s_b4 = host_entry->h_addr_list[0][3]&0x00ff;

		//服务器端口配置
		//addrSrv.sin_addr.S_un.S_addr = inet_addr("106.75.138.150");
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);

		if (connect(net_hekr_sock,(SOCKADDR *) &addrSrv, sizeof(SOCKADDR)) == 0)
		{
			printf("net_hekr_sock2 : %d\n", net_hekr_sock);
			register_net_line(net_hekr_sock, TRUE, &addrSrv);

#if defined(__HX_IOT_SUPPORT__)
			link_uart2sock(net_hekr_sock);
			hx_set_current_sock(net_hekr_sock);
#endif
			win32_start_thread(win32_recv_thread, NULL);

#ifdef __HEKR_CLOUD_SUPPORT__
			if (isLogin)
			{
				hekr_start_cmd_loop(net_hekr_sock);
			}
			else
			{
				hekr_get_product_info(net_hekr_sock);
			}
#elif defined(__OBD_OTA_TEST__)
			ota_check_version(net_hekr_sock);
#elif defined(__HTTP_TEST__)
			hx_send_http_content(net_hekr_sock);
#elif defined(__TUYA_HTTP_TEST__)
			tuya_send_http_content(net_hekr_sock);
#endif

		}
		else
		{
			printf("connect error %d!\n", WSAGetLastError());
			return -1;
		}

	}

	return 0;
}

#endif


#ifndef WIN32
int hx_do_with_net_frame(buff_queue_t *q, int sock)
{
	buf_t *b = NULL;
	
	if (hx_get_data_flag())
	{
		Boolean bOut = TRUE;

		while ((b = remove_buf(q)))
		{
			if (!bOut)
			{
				free_buffer(b);
				continue;
			}

			add_buf(&uart_tx_queue, b);
		}

		send_uart();
	}
	else
	{
		//if (hx_get_current_sock() == net_hekr_sock)		// NO Need Here
		{
			int ret = hx_check_sok_recv(q);

			if (ret == HX_NO_DATA)
			{
				// 空
				while ((b = remove_buf(q)))
				{
					free_buffer(b);
				}
			}
			else if (ret == HX_DATA_FULL)
			{
				// write uart
				while ((b = remove_buf(q)))
				{
					add_buf(&uart_tx_queue, b);
				}
				send_uart();
				
				hx_clear_sok_recvsize();
			}

			return ret;
		}
	}

	return HX_DATA_FULL;
}

net_line_t * net_tcp_recv_data_to_queue(int sock, int *plen)
{
	int len;
	net_line_t *pnet;
	buf_t *b;
	int cmd;
	int left;
	struct sockaddr_in saddr;
	
	pnet = get_net_line(sock);
	if (pnet == NULL)
	{
		char buf[HX_MAX_FRAME_LEN + 1];

consume_data:
		for (;;)
		{
			len = recv(sock, buf, sizeof(buf), 0);
			printf("len=%d, error=%d\n", len, errno);
			if (len < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					break;
				}
				else
				{
					printf("read error!\n");
					break;
				}
			}
			
			if (len < sizeof(buf))
			{
				break;
			}
		}

		printf("not register?\n");
		
		return NULL;
	}

	if (!(b = alloc_buffer()))
	{
		goto consume_data;
	}

	*plen = 0;
	len = -1;
	for (;;)
	{
		if (pnet->tcp)
		{
			len = recv(sock, &b->pbuff[b->len], b->size - b->len, 0);
		}
		else
		{
			len = recvfrom(sock, &b->pbuff[b->len], b->size - b->len, 0, (struct sockaddr *)&saddr, sizeof(saddr));
			if (len > 0)
			{
				pnet->addr = saddr;
			}
		}
	
		if (hx_get_debug_flag())
		{
			printf("len = %d, b->len = %d, error=%d\n", len, b->len, errno);
		}
		
		if (len < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)		// 数据已经读完,返回,因为是异步的
			{
				break;
			}
			else
			{
				printf("read error!\n");
			}

			break;
		}
		else if (len == 0)
		{
			printf("peer sock close!\n");
			break;
		}

		

        b->len += len;
		*plen += len;
		
		if (b->len == b->size)
		{
			add_buf(&pnet->rx_queue, b);
			if (!(b = alloc_buffer()))
			{
				return pnet;
			}
		}
	}

	if (b->len == 0)		// 未使用，释放
	{
		free_buffer(b);
		b = NULL;
	}
	else
	{
		add_buf(&pnet->rx_queue, b);
	}

	HX_DEBUG_QUEUE(&pnet->rx_queue);
	
	return pnet;
}


int hx_start_tcp_server(int port, int timeout, int *fd)
{
	*fd = hx_tcp_server_listen(port);
	return *fd;
}

kal_bool soc_ip_check(char *asci_addr,
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

        asci_addr = strstr(asci_addr, ".");
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

static int net_connect_server(char *hostname, int port, Boolean tcp)
{
    kal_uint8 *pip;
	kal_uint8 ip[4];
	kal_bool is_valid = KAL_FALSE;
	kal_uint32 net_ip_addr;
	int ret;
	struct sockaddr_in s_sockaddr;

	printf("hostname='%s'\r\n", hostname);

	memset(&s_sockaddr, 0, sizeof(s_sockaddr));
	s_sockaddr.sin_family = AF_INET;
	s_sockaddr.sin_port = htons(port);
	s_sockaddr.sin_len = sizeof(s_sockaddr);
	
	if (soc_ip_check(hostname, ip, &is_valid))
	{
		if (!is_valid)
		{
			printf("ip is not valid\n");
			return HX_ERROR_INVALID_PARAMETER;;
		}

		inet_aton(hostname, &s_sockaddr.sin_addr);
	}
	else
	{
		struct hostent *hp = NULL;
		
		hp = gethostbyname(hostname);
		if (NULL == hp)
		{
			printf("get host %s fail", hostname);
			return HX_ERROR_UNKOWN;
		}

		if (hp != NULL && hp->h_addrtype == AF_INET) 
		{
			s_sockaddr.sin_addr.s_addr = *(u32_t *)hp->h_addr;
		}
		else
		{
			printf("get2 host %s fail", hostname);
			return HX_ERROR_UNKOWN;
		}
	}
	
	if (tcp)
	{
		ret = hx_connect_tcp_server(&s_sockaddr);
	}
	else
	{
		ret = hx_connect_udp_server(&s_sockaddr);
	}

	return ret;
}


int net_tcp_connect_server(char *hostname, int port, int *status)
{
	*status = net_connect_server(hostname, port, TRUE);

	return *status;
}

int net_udp_connect_server(char *hostname, int port, int *status)
{
	*status = net_connect_server(hostname, port, FALSE);
	return *status;
}


#endif

Boolean net_send_buffer(int fd, char *buf, int len, Boolean resend)
{
	int n;
	int left;
	int index;
	
	if (fd < 0)
	{
		return FALSE;
	}

	for (left = len, index = 0; left > 0; index += n, left -= n)
	{
		n = send(fd, &buf[index], left, 0);
		if (n <= 0)
		{
#if 0
			if (errno == EAGAIN)
			{
				continue;
			}
			else
#endif
			{
				printf("send failed! err=%d\n", errno);
				return FALSE;
			}
		}
	}

	return TRUE;
}

void hekr_close_sock()
{
	if (net_hekr_sock >= 0)
	{
#ifdef WIN32
		printf("close sock: %d\r\n", net_hekr_sock);

		unlink_uart2sock(net_hekr_sock);
		unregister_net_line(net_hekr_sock);
		closesocket(net_hekr_sock);
		net_hekr_sock = -1;
#else
		tcpclose(net_hekr_sock);
#endif
	}
}

void tcp_send_ok_callback(int sock, tcp_send_cb fn)
{
	buf_t *b;
	net_line_t *pnet;
	
	//printf("tcp_send_callback @%d\n", __LINE__);

	if ((pnet = get_net_line(sock)) == NULL)
	{
		printf("not register?\n");
		return;
	}
	
	b = remove_buf(&pnet->tx_queue);
	if (!b)
	{
		//assert(FALSE);
		printf("tcp_send_ok_callback b == NULL @%d\n", __LINE__);
	}

	// 发送队列中还有数据，继续发送
	if (get_buf(&pnet->tx_queue))
	{
		net_send_tx_queue(sock, FALSE, fn);
	}
}


void net_tcp_send_cb(int socket, char *buf, int len, Boolean resend)
 {
 #ifdef WIN32
	 //U8 status = ERROR_SUCCESS;
	 int n;
	 int left;
	 int total = 0;
#endif

	if (!resend)
	{

	}

	printf("sok = %d, s = %s, len = %d\n", socket, buf, len);
#ifndef WIN32
    if (tcpsend(socket, buf, len) == -1)
	{
        printf("send data fail\n");
        //status = ERROR_TCP_CONNECTION;
    }
	else
	{
        net_hekr_get_ack = 1;
    }
#else
	left = len;

	while (left > 0)
	{
		n = send(socket, &buf[total], left, 0);
		if (n > 0)
		{
			left -= n;
			total += n;
		}
		else
		{
			break;
		}
	}

	if (left == 0)
	{
		tcp_send_ok_callback(socket, net_tcp_send_cb);
	}
#endif
 }
