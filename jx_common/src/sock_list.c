#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <winsock.h>
#include "ServerCmd.h"

#define atcmdprintf printf
////////////////////////////////////
#define buf_size 256
#define RCV_TASK_NAME "ATCMD_SocketTask"
/////////////////Link list/////////////////
enum task_st
{
	TASK_ST_Term = -1,
	TASK_ST_Stop,
	TASK_ST_Running,
};

#define  u16 unsigned short



struct list_q {
	struct list_q   *next;
	struct list_q   *prev;
	unsigned int    qlen;
};


struct listen_socket_q
{
	struct list_q list;
	int socket;
	u16 port;
};

struct socket_q
{
	struct list_q list;
	int socket;
	int parent_sd;
};

static Boolean hx_listhead_is_inited = 0;
static struct list_q hx_listen_socket;
static struct list_q hx_normal_socket;

static int hx_max_sd = -1; 
static fd_set hx_r_set;
static fd_set hx_clr_set;

static void hx_list_q_init(struct list_q *qhd)
{
	qhd->prev = (struct list_q *)qhd;
	qhd->next = (struct list_q *)qhd;
	qhd->qlen = 0;
}

static void hx_list_q_qtail(struct list_q *qhd, struct list_q *newq)
{
	struct list_q *next = qhd;
	struct list_q* prev = qhd->prev;

	newq->next = next;
	newq->prev = prev;
	next->prev = newq;
	prev->next = newq;
	qhd->qlen++;
}

static void hx_list_q_insert(struct list_q *qhd, struct list_q *prev, struct list_q *newq)
{
	struct list_q *next = prev->next;

	newq->next = next;
	newq->prev = prev;
	next->prev = newq;
	prev->next = newq;
	qhd->qlen++;
}

static void hx_list_q_remove(struct list_q *qhd, struct list_q *curt)
{
	struct list_q *next = curt->next;
	struct list_q *prev = curt->prev;

	prev->next = next;
	next->prev = prev;
	qhd->qlen--;
}

static int hx_mark_rm_listen_sd_by_port(u16 port)
{
	int socket = -1;
	struct listen_socket_q* item;

	//printf("%s::%d\n",__FUNCTION__, __LINE__);

	if (hx_listen_socket.qlen == 0)
	{
		return socket;
	}

	item = (struct listen_socket_q*)hx_listen_socket.next;

	while ((item != &hx_listen_socket) && (item->port != port))
	{
		item = (struct listen_socket_q*)item->list.next;
	}

	if (item->port == port)
	{
		socket = item->socket;
		FD_SET(socket, &hx_clr_set);
		//printf("%s::%d, socket = %d\n",__FUNCTION__, __LINE__, socket);
	}

	return socket;
}

static int hx_mark_rm_sd(int sd)
{    
	struct socket_q* item;

	//printf("%s::%d\n",__FUNCTION__, __LINE__);
	if (hx_normal_socket.qlen == 0)
	{
		return -1;
	}

	item = (struct socket_q*)hx_normal_socket.next;

	while (item != &hx_normal_socket)
	{
		if (item->socket == sd)
		{
			FD_SET(sd, &hx_clr_set);
			//printf("%s::%d, socket = %d, opt = %x\n",__FUNCTION__, __LINE__, sd);
			return sd;
		}

		item = (struct socket_q*)item->list.next;        
	}

	return -1;
}

static int hx_rm_sd(int sd)
{    
	struct socket_q* item;

	//printf("%s::%d\n",__FUNCTION__, __LINE__);
	if (hx_normal_socket.qlen == 0)
	{
		return -1;
	}

	item = (struct socket_q*)hx_normal_socket.next;

	while (item != &hx_normal_socket)
	{
		if (item->socket == sd)
		{
			hx_list_q_remove(&hx_normal_socket, item);
			free(item);
			//printf("%s::%d, socket = %d, opt = %x\n",__FUNCTION__, __LINE__, sd);
			return sd;
		}
		item = (struct socket_q*)item->list.next;        
	}

	return -1;
}

static int hx_rm_listen_sd(int sd)
{    
	struct listen_socket_q* item;
	//printf("%s::%d\n",__FUNCTION__, __LINE__);
	if (hx_listen_socket.qlen == 0)
	{
		return -1;
	}

	item = (struct listen_socket_q*)hx_listen_socket.next;

	while (item != &hx_listen_socket)
	{
		if (item->socket == sd)
		{
			hx_list_q_remove(&hx_listen_socket, item);
			free(item);
			//printf("%s::%d, socket = %d, opt = %x\n",__FUNCTION__, __LINE__, sd);
			return sd;
		}

		item = (struct listen_socket_q*)item->list.next;        
	}

	return -1;
}


static int hx_update_new_listen_sd(int socket, u16 port)
{
	struct listen_socket_q* item = (struct listen_socket_q*)hx_listen_socket.next;
	struct listen_socket_q* newitem = (struct listen_socket_q*)malloc(sizeof(struct listen_socket_q));

	if(newitem == NULL)
		return -1;
	//printf("%s::%d, newitem = %x, socket = %d, hx_listen_socket = %x\n",__FUNCTION__, __LINE__, newitem, socket, &hx_listen_socket);
	newitem->socket = socket;
	newitem->port = port;
	// Insert by ordering of socket descriptor, good for finding max socket descriptor
	if(hx_listen_socket.qlen == 0)
		hx_list_q_qtail(&hx_listen_socket, newitem);
	else
	{
		while ((item != &hx_listen_socket) && (item->socket < socket))
			item = (struct listen_socket_q*)item->list.next;

		if(item->socket > socket)
			item = (struct listen_socket_q*)item->list.prev;        

		hx_list_q_insert(&hx_listen_socket, (struct list_q *)item, (struct list_q *)newitem);
	}
	return 0;
}

static int hx_update_new_sd(int socket, int parent_sd)
{
	struct socket_q* item = (struct socket_q*)hx_normal_socket.next;
	struct socket_q* newitem = (struct socket_q*)malloc(sizeof(struct socket_q));

	if (newitem == NULL)
	{
		return -1;
	}
	printf("%s::%d, newitem = %x, socket = %d, hx_normal_socket = %x\n",__FUNCTION__, __LINE__, newitem, socket, &hx_normal_socket);

	newitem->socket = socket;
	newitem->parent_sd = parent_sd;
	// Insert by ordering of socket descriptor, good for finding max socket descriptor
	if (hx_normal_socket.qlen == 0)
	{
		hx_list_q_qtail(&hx_normal_socket, newitem);
	}
	else
	{
		while ((item != &hx_normal_socket) && (item->socket < socket))
		{
			item = (struct socket_q*)item->list.next;
		}

		if (item->socket > socket)
		{
			item = (struct socket_q*)item->list.prev; 
		}

		hx_list_q_insert(&hx_normal_socket, (struct list_q *)item, (struct list_q *)newitem);
	}    

	return 0;
}

static void hx_update_max_sd()
{
	//printf("%s::%d\n",__FUNCTION__, __LINE__);
	struct list_q* item;

	if (hx_normal_socket.qlen != 0)
	{    
		item = hx_normal_socket.prev;
		//printf("addr:%x nsocket:%d max:%d\n", item, ((struct socket_q*)item)->socket, hx_max_sd);
		if(hx_max_sd < ((struct socket_q*)item)->socket)
		{
			hx_max_sd = ((struct socket_q*)item)->socket;        
		}
	}

	if (hx_listen_socket.qlen != 0)
	{    
		item = hx_listen_socket.prev;
		//printf("addr:%x lsocket:%d max:%d\n", item, ((struct listen_socket_q*)item)->socket, hx_max_sd);
		if (hx_max_sd < ((struct listen_socket_q*)item)->socket)
		{
			hx_max_sd = ((struct listen_socket_q*)item)->socket;
		}
	}
}


void hx_print_sock_list(struct list_q *head, Boolean listen)
{
	int i;
	struct list_q *node;

	for (i = 0, node = head->next; node != head; node = node->next)
	{
		if (listen)
		{
			struct listen_socket_q *item = (struct listen_socket_q *)node;

			printf("index:%d, sock=%d, port=%d\n", i, item->socket, item->port);
			i++;
		}
		else
		{
			struct socket_q *item = (struct socket_q *)node;

			printf("index:%d, sock=%d, psock=%d\n", i, item->socket, item->parent_sd);
			i++;
		}
	}
}


void mytest_sock_list()
{
	if (hx_listhead_is_inited == 0)
	{
		hx_list_q_init(&hx_listen_socket);
		hx_list_q_init(&hx_normal_socket);
		hx_listhead_is_inited = 1;
	}

	if (hx_update_new_sd(0, -1) == 0)
	{
		FD_SET(0, &hx_r_set);

		hx_print_sock_list(&hx_normal_socket, FALSE);
	}
}