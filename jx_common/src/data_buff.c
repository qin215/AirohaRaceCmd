
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#ifdef LINUX
#define TRUE 1
#define FALSE 0
#include <semaphore.h>
#elif defined(WIN32)
#include "windows.h"
#else
#include "ssv_lib.h"
#include "osal.h"
#include "ms_hal_os.h"
#endif
#include "data_buff.h"
#include "qcloud_iot_export_log.h"

#ifdef WIN32
#define inline
#endif

typedef struct mem_pool_struct 
{
	buf_t *free_list;			// 空闲块列表
	kal_uint16 blk_size;			// 块大小
	kal_uint16 blk_cnt;				// 总的块数量
	kal_uint16 free_cnt;			// 空闲块数量
	Boolean busy;				// 是否忙
	Boolean init_flag;			// 是否已经初始化
} mem_pool_t;


static mem_pool_t *get_mem_pool(int size, Boolean alloc);

#define NR_SIZE (NR_BLK_SIZE_16 + NR_BLK_SIZE_32 + NR_BLK_SIZE_64 + NR_BLK_SIZE_128 + \
				NR_BLK_SIZE_256 + NR_BLK_SIZE_512 + NR_BLK_SIZE_544 + NR_BLK_SIZE_1024 + \
				NR_BLK_SIZE_2048)

//#define __HX_IOT_SUPPORT__ 1

//(8 * 1024)		// 定值

static buf_t buffer[NR_SIZE];

#ifdef LINUX
static sem_t mem_sem;
#elif defined(WIN32)
static HANDLE hSema;
static HANDLE hQueSema;
#else
static ms_hal_os_semaphore_t semaphore;
static ms_hal_os_semaphore_t qsemaphore;
#endif

#if 0
static buf_t *free_list;
static Boolean init_flag = FALSE;
static unsigned short free_count;
static unsigned short buff_nr;
static Boolean list_busy = FALSE;
#endif

static mem_pool_t pool_array[BLK_NUM];
static const kal_uint16 pool_blk_count_array[BLK_NUM] = 
{
	NR_BLK_SIZE_16,
	NR_BLK_SIZE_32,
	NR_BLK_SIZE_64,
	NR_BLK_SIZE_128,
	NR_BLK_SIZE_256,
	NR_BLK_SIZE_512,
	NR_BLK_SIZE_544,
	NR_BLK_SIZE_1024,
	NR_BLK_SIZE_2048
};


static const kal_uint16 pool_blk_size_array[BLK_NUM] = 
{
	BUF_16_SIZE,
	BUF_32_SIZE,
	BUF_64_SIZE,
	BUF_128_SIZE,
	BUF_256_SIZE,
	BUF_512_SIZE,
	BUF_544_SIZE,
	BUF_1024_SIZE,
	BUF_2048_SIZE,
};


#define HX_MEM_POOL_SIZE	((BUF_16_SIZE * NR_BLK_SIZE_16 + \
							BUF_32_SIZE * NR_BLK_SIZE_32 + \
							BUF_64_SIZE * NR_BLK_SIZE_64 + \
							BUF_128_SIZE * NR_BLK_SIZE_128 +  \
							BUF_256_SIZE * NR_BLK_SIZE_256 +   \
							BUF_512_SIZE * NR_BLK_SIZE_512 +  \
							BUF_544_SIZE * NR_BLK_SIZE_544 + \
							BUF_1024_SIZE * NR_BLK_SIZE_1024 + \
							BUF_2048_SIZE * NR_BLK_SIZE_2048))

static char mem_pool[HX_MEM_POOL_SIZE];		// 10K bytes memory pool

#define MAGIC0 	0xAA55AA55
#define MAGIC1 	0x55AA55AA

#define PR_DEBUG  //printf

static Boolean init_buffer_ptr(buf_t *b, int size, int index);
Boolean init_one_mem_pool(mem_pool_t *pool, buf_t *buf_begin, buf_t *buf_end, char *mem_begin, char *mem_end);
static int get_mem_pool_index(int size) ;

void init_256_mem_pool()
{
	const kal_uint16 sizes[] = {0, 0, 0, 0, (HX_MEM_POOL_SIZE / 256), 0, 0,  0, 0 };
	init_mem_pool(sizes);
}

void init_2048_mem_pool()
{
	const kal_uint16 sizes[] = {0, 0, 0, 0, 0, 0, 0, 0, (HX_MEM_POOL_SIZE / 2048) };
	init_mem_pool(sizes);
}

void init_mem_pool(const kal_uint16 *size_array)
{
	int i;
	int buf_index;
	int mem_index;
	int n;

#ifdef LINUX
	if (sem_init(&mem_sem, 0, 1) < 0)
	{
		printf("semaphonre init failed!\n");
	}
#elif defined(WIN32)
	hSema = CreateSemaphore(NULL, 1, 1, "MySemaphore");
	hQueSema = CreateSemaphore(NULL, 1, 1, "MyQueneSemaphore");
#else
	if (OS_SUCCESS != OS_SemInit(&semaphore, 1, 1))
	{
		printf("sema init failed!!");
	}
	
	if (OS_SUCCESS != OS_SemInit(&qsemaphore, 1, 1))
	{
		printf("sema init2 failed!!");
	}
#endif
	if (size_array)
	{
		for (i = 0, n = 0; i < BLK_NUM; i++)
		{
			n += size_array[i];
		}

		if (n > NR_SIZE)
		{
			printf("n = %d, nr_size=%d\r\n", n, NR_SIZE);
		}
	}
	else
	{
		size_array = pool_blk_count_array;
	}
	
	for (i = 0, buf_index = 0, mem_index = 0; i < BLK_NUM; i++)
	{
		pool_array[i].blk_size = pool_blk_size_array[i];
		pool_array[i].blk_cnt = size_array[i];
		
		if (init_one_mem_pool(&pool_array[i], &buffer[buf_index], &buffer[NR_SIZE], \
								&mem_pool[mem_index], &mem_pool[HX_MEM_POOL_SIZE]))
		{
			buf_index += pool_array[i].blk_cnt;
			mem_index += pool_array[i].blk_cnt * pool_array[i].blk_size;
		}
		else
		{
			break;
		}
	}
	
	print_mem_pool_array();
}

/*
 * pool: mem_pool_t 管理节点
 * buf_begin: 起始buf
 * buf_end: 终止buf
 * mem_begin: 起始内存
 * mem_end: 终止内存
 */
Boolean init_one_mem_pool(mem_pool_t *pool, buf_t *buf_begin, buf_t *buf_end, char *mem_begin, char *mem_end)
{
	int blksize = pool->blk_size;
	int blkcnt = pool->blk_cnt;
	int i;
	
	if (buf_begin + blkcnt > buf_end)
	{
		printf("begin:%p, cnt=%d, end=%p\n", buf_begin, blkcnt, buf_end);
		return FALSE;
	}

	if (mem_begin + blksize * blkcnt > mem_end)
	{
		printf("mbegin:%p, cnt=%d, mend=%p\n", mem_begin, blksize * blkcnt, mem_end);
		return FALSE;
	}

	pool->free_cnt = 0;
	for (i = 0; i < blkcnt; i++)
	{
		init_buffer(&buf_begin[i]);
		buf_begin[i].pbuff = &mem_begin[i * blksize];
		buf_begin[i].size = blksize;
		
		buf_begin[i].next = &buf_begin[i + 1];
		pool->free_cnt++;
	}

	pool->free_list= &buf_begin[0];
	buf_begin[i - 1].next = NULL;
	pool->init_flag = TRUE;

	return TRUE;
}

void print_mem_pool(mem_pool_t *pool)
{
	printf("%8d\t%8d\t%8d\n",  pool->blk_size, pool->blk_cnt, pool->free_cnt);
}

void print_mem_pool_array()
{
	int i;
	mem_pool_t *pmem;

	printf("\n%8s\t%8s\t%8s\n","blksize", "total cnt", "free cnt");
	for (i = 0, pmem = pool_array; i < BLK_NUM; i++, pmem++)
	{
		print_mem_pool(pmem);
	}
}

buf_t * get_buff_from_mem_pool(char *p)
{
	int i;
	buf_t *b;
	
	for (i = 0, b = buffer; i < NR_SIZE; i++, b++)
	{
		if (p == b->pbuff)
		{
			return b;
		}
	}

	printf("mem(%p) is not buffer!\n", p);

	return NULL;
}

static mem_pool_t *get_mem_pool(int size, Boolean alloc)
{
	int i;
	mem_pool_t *pmem = pool_array;

	if (size <= 0)
	{
		return NULL;
	}

#if 1
	i = get_mem_pool_index(size);
	if (i < 0)
	{
		printf("no memory pool index!\n");
		return NULL;
	}
	
	if (!alloc)
	{
		return pmem + i;
	}

	pmem += i;
	for (; i <= BLK_SIZE_2048; i++, pmem++)
	{
		if (pmem->free_cnt > 0)
		{
			return pmem;
		}
	}

	return NULL;
	
#else
	if (size <= pmem->blk_size)
	{
		if (!alloc)
		{
			return pmem;
		}
		
		for (i = 0; i < BLK_NUM; i++, pmem++)		// 当前free blk 为0,检查下一个
		{
			if (pmem->free_cnt > 0)
			{
				return pmem;
			}
		}

		return NULL;
	}
	
	for (i = 0; i < BLK_NUM - 1; i++, pmem++)
	{
		pnext = pmem + 1;
		
		if ((size > pmem->blk_size) && (size <= pnext->blk_size))
		{
			if (!alloc)
			{
				return pnext;
			}
			
			if (pnext->free_cnt > 0)
			{
				return pnext;
			}
			else
			{
				for (i++; i < BLK_NUM; i++) 	// 当前free blk 为0,检查下一个
				{
					pnext = &pool_array[i];
					if (pnext->free_cnt > 0)
					{
						return pnext;
					}
				}
		
				return NULL;
			}
		}

	}

	if (pmem->blk_size == size)
	{
		if (!alloc || pmem->free_cnt > 0)		// 最后一个节点
		{
			return pmem;
		}
	}
#endif	

	return NULL;
}


/*
 * 从pool中分配一个空闲buffer
 */
buf_t *new_alloc_buffer(int size)
{
	mem_pool_t *pool;
	buf_t *ret;

	if (!(pool = get_mem_pool(size, TRUE)))
	{
		printf("alloc_buffer size %d failed\n", size);
		return NULL;
	}

#ifdef LINUX
	sem_wait(&mem_sem);
#elif defined(WIN32)
	WaitForSingleObject(hSema, INFINITE);
#else
	ms_hal_os_semaphore_get(&semaphore, portMAX_DELAY);
#endif

	ret = pool->free_list;

	if (ret == NULL)
	{
		printf("no free buffer!\n");
#ifdef LINUX
		sem_post(&mem_sem);
#elif defined(WIN32)
		ReleaseSemaphore(hSema, 1, NULL);
#else
		ms_hal_os_semaphore_put(&semaphore);
#endif		
		return NULL;
	}

	if (pool->busy)
	{
		printf("Warning:list busy\n");
	}

	pool->busy = TRUE;
	pool->free_list = pool->free_list->next;
	pool->busy = FALSE;

#ifdef LINUX
	sem_post(&mem_sem);
#elif defined(WIN32)
	ReleaseSemaphore(hSema, 1, NULL);
#else
	ms_hal_os_semaphore_put(&semaphore);
#endif

	//check_buffer_valid(ret, __func__);
	init_buffer(ret);
	ret->size = pool->blk_size;
	pool->free_cnt--;
	PR_DEBUG("alloc count=%d, blk_size=%d, size=%d, buf=%p, pbuf=%p!\r\n", pool->free_cnt, pool->blk_size, size, ret, ret->pbuff);

	return ret;
}

buf_t *new_alloc_index_buffer(int size_index)
{
	int size = (1 << size_index) * 32;

	return new_alloc_buffer(size);
}


static int get_mem_pool_index(int size)
{
	int index = 0;
	int i;
	
#if 1
	
	for (i = 0; i < BLK_NUM - 1; i++)
	{
		if (size == pool_blk_size_array[i])
		{
			return i;
		}

		if (size > pool_blk_size_array[i] && size < pool_blk_size_array[i + 1])
		{
			return i + 1;
		}
	}

	if (size < pool_blk_size_array[0])
	{
		return 0;
	}

	if (size == pool_blk_size_array[i])
	{
		return i;
	}

	printf("size is too large(%d)\n", size);
	return -1;
#else
	
	if ((n = (size >> 5)) == 0) // size / 32
	{
		return 0;
	}

	left = size & (32 - 1);	// size % 32

	if (size > ((1 << BLK_SIZE_2048) * 32))
	{
		printf("size>2048\n");
		return BLK_SIZE_2048;
	}
	
	while (n)
	{
		index++;

		n >>= 1;
	}

	if (left)
	{
		return index;
	}

	return index - 1;
#endif	
}

void new_free_buffer(buf_t *b)
{
	mem_pool_t *pool;

	if (!b)
	{
		printf("free_buffer null buff@%d!\r\n", __LINE__);
		return;
	}

	if (!(pool = get_mem_pool(b->size, FALSE)))
	{
		printf("free_buffer size:%d not alloc\n", b->size);
		return;
	}
	
	if (b < &buffer[0] || b >= &buffer[NR_SIZE])
	{
		printf("free_buffer buffer is not alloc from pool.\r\n");
		return;
	}

#if 1
	if (pool->busy)
	{
		printf("free_buffer Warning:list busy\n");
	}
#endif

#ifdef LINUX
	sem_wait(&mem_sem);
#elif defined(WIN32)
	WaitForSingleObject(hSema, INFINITE);
#else
	ms_hal_os_semaphore_get(&semaphore, portMAX_DELAY);
#endif
	pool->busy = TRUE;
	if (pool->free_list == NULL)
	{
		pool->free_list = b;
		b->next = NULL;
		b->prev = NULL;
		pool->busy = FALSE;
		pool->free_cnt++;
		
#ifdef LINUX
	sem_post(&mem_sem);
#elif defined(WIN32)
	ReleaseSemaphore(hSema, 1, NULL);
#else
	ms_hal_os_semaphore_put(&semaphore);
#endif		
		return;
	}

	//check_buffer_valid(b, __func__);

	b->next = pool->free_list;
	pool->free_list = b;
	pool->free_cnt++;
	pool->busy = FALSE;
#ifdef LINUX
	sem_post(&mem_sem);
#elif defined(WIN32)
	ReleaseSemaphore(hSema, 1, NULL);
#else
	ms_hal_os_semaphore_put(&semaphore);
#endif

	PR_DEBUG("free count=%d, size=%d buf=%p!\r\n", pool->free_cnt, pool->blk_size, b);	
}


void reset_init_flag()
{

}

/*
* added by qinjiangwei 2016/7/20
* 初始化队列
*/
void init_buff_queue(buff_queue_t *pb, char *name)
{
	if (!pb)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return;
	}

	memset(pb, 0, sizeof(buff_queue_t));
	strncpy(pb->name, name, sizeof(pb->name) - 1);

#if 0
	if (init_flag)
	{
		printf("buffer is inited, buff_nr=%d!\r\n", buff_nr);
		return;
	}

	free_count = 0;
	for (i = 0; i < NR_SIZE; i++)
	{
		init_buffer(&buffer[i]);
		if (!init_buffer_ptr(&buffer[i], buf_size, i))
		{
			break;
		}
		
		buffer[i].next = &buffer[i + 1];
		free_count++;
	}

	buff_nr = free_count;
	free_list = &buffer[0];
	buffer[i - 1].next = NULL;
	init_flag = TRUE;
	printf("buffer inited, buff_nr=%d, buf_size=%d!\r\n", buff_nr, buf_size);
#endif	
}

/*
 * 从pool中分配一个空闲buffer
 */
buf_t *alloc_buffer()
{
#if 1
	return new_alloc_buffer(BUF_SIZE);
#else
	buf_t *ret = free_list;
	
	if (free_list == NULL)
	{
		printf("no free buffer!\n");
		return NULL;
	}

	if (list_busy)
	{
		printf("Warning:list busy\n");
	}
	list_busy = TRUE;
	free_list = free_list->next;
	list_busy = FALSE;

	//check_buffer_valid(ret, __func__);
	init_buffer(ret);
	free_count--;
	printf("alloc free_count=%d!\r\n", free_count);
	
	return ret;
#endif	
}

/*
 * 回收一个buffer到pool中
 */
void free_buffer(buf_t *b)
{
#if 1
	new_free_buffer(b);
#else
	if (b < &buffer[0] || b >= &buffer[buff_nr])
	{
		printf("buffer is not alloc from pool.\r\n");
		return;
	}

	if (!b)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return;
	}

	if (list_busy)
	{
		printf("Warning:list busy\n");
	}
	
	list_busy = TRUE;
	if (free_list == NULL)
	{
		free_list = b;
		b->next = NULL;
		b->prev = NULL;
		list_busy = FALSE;
		free_count++;
		
		return;
	}

	//check_buffer_valid(b, __func__);

	b->next = free_list;
	free_list = b;
	free_count++;
	list_busy = FALSE;
	printf("free free_count=%d!\r\n", free_count);
#endif	
}

static Boolean init_buffer_ptr(buf_t *b, int size, int index)
{
	if (index * size >= sizeof(mem_pool))
	{
		printf("init buffer ovflow!\n");
		return FALSE;
	}

	b->pbuff = &mem_pool[index * size];
	b->size = size;
	return TRUE;
}

inline void init_buffer(buf_t *b)
{
	if (!b)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return;
	}
	
//	memset(b, 0, sizeof(buf_t));
	b->len = 0;
	b->count = 0;
	b->next = b->prev = NULL;
	
	b->magic0 = MAGIC0;
	b->count = 0;			// found by wangwenke@2017/06/02
//	b->magic1 = MAGIC1;
}

Boolean init_buffer2(buf_t *b, char *ptr, int len)
{
	if (!b)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}

	if (!ptr)
	{
		printf("null ptr@%d!\r\n", __LINE__);
		return FALSE;
	}
	if (len > b->size || len < 0)
	{
		printf("warning: len(%d) beyond size(%d)\r\n", len, b->size);
		return FALSE;
	}
	
	b->next = b->prev = NULL;
	
	memcpy(b->pbuff, ptr, len);
	b->len = len;
	b->count = 0;		// found by wangwenke@2017/06/02
	b->magic0 = MAGIC0;
//	b->magic1 = MAGIC1;
	
	return TRUE;
}

/*重新初始化buf,主要是清空其下的pbuff*/
Boolean reset_buffer(buf_t *b)
{
	if (NULL == b)
	{
		printf("reset_buffer buf NULL\n");
		return FALSE;
	}

	if (NULL == b->pbuff)
	{
		printf("reset_buffer pbuff NULL\n");
		return FALSE;
	}

	memset(b->pbuff, 0, b->size);
	b->len = 0;

	return TRUE;
}

Boolean check_buffer_valid(buf_t *b, const char *func)
{
	if (!b)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}
	
	if (b->magic0 != MAGIC0)
	{
		printf("called from (%s) buff is invalid!\r\n", func);
		return FALSE;
	}

	return TRUE;
}

Boolean copy_buf(buf_t * to, buf_t * from)
{
	if (!to)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}

	if (!from)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}
	
	if (to->size < from->len)
	{
		printf("to(%d) from(%d) is low!\n", to->size, from->len);
		return FALSE;
	}

	memcpy(to->pbuff, from->pbuff, from->len);
	to->len = from->len;

	return TRUE;
}

/*
 * 填满第一个, 剩余的放入第二个
 * -1: 表示失败
 * 0 : 表示没有溢出
 * 1 : 表示溢出, 第二个buffer from会保存剩余的
 */
int append_buf2(buf_t *to, buf_t *from)
{
	int n;
	int ret = -1;
	
	if (!to)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return ret;
	}

	if (!from)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return ret;
	}
	
	if (from == NULL || to == NULL)
	{
		return ret;
	}

	n = 0;
	if (to->len + from->len > to->size)
	{
		n = to->size - to->len;
		ret = 1;
	}
	else
	{
		n = from->len;
		ret = 0;
	}

	memcpy(&to->pbuff[to->len], from->pbuff, n);
	to->len += n;

	if (ret == 1)
	{
		memmove(&from->pbuff[0], &from->pbuff[n], from->len - n);
		from->len -= n;
	}
	
	return ret;
}

Boolean append_buf(buf_t *to, buf_t *from)
{
	if (!to)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}

	if (!from)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}
	
	if (from == NULL || to == NULL)
	{
		return FALSE;
	}

	//check_buffer_valid(to, __func__);
	//check_buffer_valid(from, __func__);
	
	if (to->len + from->len > to->size)
	{
		return FALSE;
	}

	memcpy(&to->pbuff[to->len], from->pbuff, from->len);
	to->len += from->len;

	return TRUE;
}

void print_buffer(buf_t *b)
{
	static char tmp[1024 * 10];
	int i;
	int len;
	int index;
	Boolean overflow = FALSE;

	if (!b)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return;
	}

	//check_buffer_valid(b, __func__);
	
	len = 0;
	index = 0;
	for (i = 0; i < b->len; i++)
	{
		//if (isprint((unsigned char)(b->pbuff[i])))
		//{
		//	len = sprintf(&tmp[index], "%c", b->pbuff[i]);
		//}
		//else 
		{	
			len = sprintf(&tmp[index], "%02x ", b->pbuff[i] & 0xff);
		}
		index += len;

		if (index >= sizeof(tmp) - 5)
		{
			overflow = TRUE;
			break;
		}
	}
	
	Log_d(("len = %d, ovflow = %d, data = %s\r\n"), b->len, overflow, tmp);
}

void print_mem(char *b, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		printf("0X%02x ", b[i] & 0xff);
	}
}

/*
 * buff 必须要从buffer中分配
 */
Boolean add_buf(buff_queue_t *pb, buf_t *buff)
{
	buf_t *node;

	//check_buffer_valid(buff, __func__);
	if (buff < &buffer[0] || buff >= &buffer[NR_SIZE])
	{
		printf("add_buf buffer is not alloc from pool.\r\n");
		return FALSE;
	}

	if (!pb)
	{
		printf("add_buf null queue@%d!\r\n", __LINE__);
		return FALSE;
	}

	if (!buff)
	{
		printf("add_buf null buff@%d!\r\n", __LINE__);
		return FALSE;
	}

#if 1
	if (pb->busy)
	{
		printf("add_buf Warning:q:(%s) busy\n", pb->name);
	}
	
	pb->busy = TRUE;
#endif	

#ifdef LINUX

#elif defined(WIN32)
	WaitForSingleObject(hQueSema, INFINITE);
#else
	ms_hal_os_semaphore_get(&qsemaphore, portMAX_DELAY);
#endif
	buff->next = buff;
	buff->prev = buff;

	node = pb->tail;
	
	if (!node)
	{
		pb->tail = buff;
	}
	else
	{
		node->next->prev = buff;
		buff->next = node->next;
		
		node->next = buff;
		buff->prev = node;
		
		pb->tail = buff;
	}

	node = pb->head;
	if (!node)
	{
		pb->head = buff;
	}
	pb->busy = FALSE;

#ifdef LINUX

#elif defined(WIN32)
	ReleaseSemaphore(hQueSema, 1, NULL);
#else	
	ms_hal_os_semaphore_put(&qsemaphore);
#endif
	return TRUE;
}

/* 取第一个buf*/
buf_t *get_buf(buff_queue_t *q)
{
	if (!q)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return NULL;
	}

	if (q->head == NULL)
	{
		return NULL;
	}

	return q->head;
}

/*取尾节点buf*/
buf_t *get_buf_tail(buff_queue_t *q)
{
	if (NULL == q)
	{
		printf("get_buf_tail null queue@%d!\r\n", __LINE__);
		return NULL;
	}

	if (q->tail == NULL)
	{
		return NULL;
	}

	return q->tail;
}

int buf_get_total_len(buff_queue_t *q)
{
	int total = 0;
	buf_t *head;
	buf_t *tail;

	if (!q)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return 0;
	}

	
	head = q->head;
	tail = q->tail;
	//printf("head=%x, tail=%x\n", head, tail);
	
	if (!head)
	{
		return 0;
	}

	while (1) 
	{
		total += head->len;
		if (head == tail)
		{
			break;
		}
		
		head = head->next;
	}

	//printf("total=%d\n", total);
	return total;
}

/*获取队列节点个数*/
int queue_get_node_num(buff_queue_t *q)
{
	int i;
	int total = 0;
	buf_t *head = NULL;
	buf_t *tail = NULL;

	if (!q)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return 0;
	}

	head = q->head;
	tail = q->tail;

	if (NULL == head)
	{
		return 0;
	}
	else
	{
		total++;
	}

	for (i = 0; ; head = head->next, i++)
	{
		if (head == tail)
		{
			break;
		}

		total++;
	}

	//printf("%s total %d\n", __FUNCTION__, total);
	
	return total;
}

buf_t * remove_node(buff_queue_t *pb, buf_t *node)
{
	if (!pb)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return NULL;
	}

	if (pb->head == NULL)
	{
		//printf("ERROR: q is empty@%d\n", __LINE__);
		return NULL;
	}

	if (pb->head == pb->tail)		// 只有一个节点
	{
		if (node != pb->head)
		{
			printf("node isn't in q!\n");
		}
		
		pb->head = NULL;
		pb->tail = NULL;
		return node;
	}

	if (node == pb->head)
	{
		pb->head = node->next;
	}

	if (node == pb->tail)
	{
		pb->tail = node->prev;
	}

	node->next->prev = node->prev;
	node->prev->next = node->next;

	node->next = node->prev = NULL;
	
	//check_buffer_valid(node, __func__);

	return node;
}


/*
 * 将队列大小减小到final_len
 */
Boolean buf_decrease_to_len(buff_queue_t *q, int final_len)
{
	int total;
	int v;
//	buf_t *b;
	buf_t *head;
	buf_t *tail;
	buf_t *node;

	if (!q)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return FALSE;
	}
	//print_queue(q);
	
	total = buf_get_total_len(q);
	if (total <= final_len)
	{
		printf("q total len = %d, final len = %d\r\n", total, final_len);
		return FALSE;
	}

	if (final_len == 0)
	{
		free_queue_buff(q);
		return FALSE;
	}

	tail = q->tail;
	head = q->head;

	node = head;

	for (v = 0; node != tail; node = node->next)
	{
		if (final_len > v && final_len <= v + node->len)
		{
			break;
		}

		v += node->len;
	}

	if (node == tail)		// 检测到尾端
	{
		v += node->len;
		if (v < final_len)
		{
			printf("q len is small: v=%d, fl=%d\n", v, final_len);
			return FALSE;
		}

		node->len -= v - final_len;
		return TRUE;
	}

	node->len -= v + node->len - final_len;

	node = node->next;
	while (1)
	{
		buf_t *tmp = node;

		node = node->next;
		
		remove_node(q, tmp);
		free_buffer(tmp);

		if (tmp == tail)
		{
			break;
		}
	}

	//print_queue(q);

#if 0
	v = 0;
	b = &q->buffer[0];
	for (index = q->index - 1; index >= 0; index--)
	{
		b = &q->buffer[index];
		if (final_len > v && final_len <= v + b->len)
		{
			break;
		}

		v += b->len;
	}

	b->len -= v + b->len - final_len;

	for (i = 0; i < index; i++)
	{
		remove_buf(q);
	}
#endif	

	return TRUE;	
}



/*
 * 将buf队列中的buf合并到一个buf中
 */
Boolean buf_acc(buff_queue_t *q, buf_t *dst_buf)
{
	buf_t *b;
	Boolean ret = FALSE;
	int count = 0;

	if (!q)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return FALSE;
	}

	while ((b = get_buf(q)))
	{
		if (append_buf(dst_buf, b))
		{
			remove_buf(q);
			free_buffer(b);
			ret = TRUE;
			count++;
		}
		else
		{
			break;
		}
	}

	//printf("acc %d packet!\n", count);
	
	return ret;	
}



#if 0
int get_buf_by_func(buff_queue_t *q, int v, fn_compare f)
{
	int i;
	buf_t *head;
	buf_t *tail;
	
	if (f == NULL)
	{
		return 0;
	}

	for (i = 0; i < q->index; i++)
	{
		buf_t *b;

		b = &q->buffer[i];
		if (f(v, b))
		{
			return i;
		}
	}

	return -1;
}
#endif

Boolean add_char(buf_t *buff, char ch)
{
	if (!buff)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}

	//check_buffer_valid(buff, __func__);
	
	if (buff->len < buff->size)
	{
		buff->pbuff[buff->len++] = ch;
		return TRUE;
	}

	return FALSE;	
}

Boolean add_string(buf_t *buff, char *str)
{
	int len;

	if (!buff)
	{
		printf("null buff@%d!\r\n", __LINE__);
		return FALSE;
	}

	if (!str)
	{
		printf("null str@%d!\r\n", __LINE__);
		return FALSE;
	}

	//check_buffer_valid(buff, __func__);
	
	len = strlen(str);
	if (buff->len + len < buff->size)
	{
		strcpy(&buff->pbuff[buff->len], str);
		buff->len += len;
		return TRUE;
	}

	return FALSE;	
}

buf_t * remove_buf(buff_queue_t *pb)
{
	buf_t *node;

	if (!pb)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return NULL;
	}

	if (pb->head == NULL)
	{
		//printf("ERROR: q is empty@%d\n", __LINE__);
		return NULL;
	}

	if (pb->busy)
	{
		printf("Warning:q2:(%s) busy\n", pb->name);
	}
	pb->busy = TRUE;

#ifdef LINUX

#elif defined(WIN32)
	WaitForSingleObject(hQueSema, INFINITE);
#else	
	ms_hal_os_semaphore_get(&qsemaphore, portMAX_DELAY);
#endif	
	node = pb->head;

	if (node == pb->tail)
	{
		pb->head = pb->tail = NULL;
		pb->busy = FALSE;
#ifdef LINUX

#elif defined(WIN32)
		ReleaseSemaphore(hQueSema, 1, NULL);
#else		
		ms_hal_os_semaphore_put(&qsemaphore);
#endif
		return node;
	}

	pb->head = node->next;

	node->next->prev = node->prev;
	node->prev->next = node->next;

	node->next = node->prev = NULL;
	pb->busy = FALSE;
	
	//check_buffer_valid(node, __func__);
#ifdef LINUX

#elif defined(WIN32)
	ReleaseSemaphore(hQueSema, 1, NULL);
#else	
	ms_hal_os_semaphore_put(&qsemaphore);
#endif

	return node;
}

void free_queue_buff(buff_queue_t *pb)
{
	buf_t *b;
	while ((b = remove_buf(pb)))
	{
		free_buffer(b);
	}
}

void *ua_malloc(size_t sz)
{
	buf_t *b = new_alloc_buffer(sz);
	
	if (b)
	{
		return b->pbuff;
	}
	else
	{
		printf("no memory!\n");
		return NULL;
	}
}

void ua_free(void *p)
{
	buf_t *b;
	b = get_buff_from_mem_pool(p);

	if (b)
	{
		new_free_buffer(b);
	}
	else
	{
		printf("ptr (%p) is not alloc from buf_t!\n", p);
	}
}

void print_queue(buff_queue_t *q)
{
	int i;
	buf_t *head;
	buf_t *tail;
	
	if (!q)
	{
		printf("null queue@%d!\r\n", __LINE__);
		return;
	}

	printf("name:%s\n", q->name);

	head = q->head;
	tail = q->tail;

	if (!head)
	{
		return;
	}

	for (i = 0; ; head = head->next, i++)
	{
		printf("i = %d, buf='%s'\r\n", i, head->pbuff);

		if (head == tail)
		{
			break;
		}
	}
}

#ifdef TEST
static buff_queue_t tx_queue;

static char *get_random_string()
{
	static char rand_str[32];
	int num;
	static char ch_array[] = "abcdefghijklmnopqrstuvwxyzABCDEFHIJKLMNOPQRSTUVWXYZ0123456789";
	int str_len = strlen(ch_array);
	int i;


#if defined(LINUX) || defined(WIN32)
	srand((unsigned int)time(NULL));
	num = rand() % sizeof(rand_str);
#else	
	num = OS_GetSysTick() % sizeof(rand_str);
#endif
	if (num <= 2)
	{
		num = 2;
	}
	
	for (i = 0; i < num - 2; i++)
	{
		int index;

#if defined(LINUX) || defined(WIN32)
		index = rand() % str_len;
#else
		index = OS_GetSysTick()  % str_len;
#endif		
		rand_str[i] = ch_array[index];
	}

	rand_str[i] = '\0';

	return rand_str;
}


#define POLY  0x1021  
static kal_uint16 test_crc16(unsigned char *addr, int num, kal_uint16 crc)
{  
	int i;
	
	for (; num > 0; num--)              /* Step through bytes in memory */  
	{  
	    crc = crc ^ (*addr++ << 8);     /* Fetch byte from memory, XOR into CRC top byte*/  
	    for (i = 0; i < 8; i++)             /* Prepare to rotate 8 bits */  
	    {  
	        if (crc & 0x8000)            /* b15 is set... */  
	            crc = (crc << 1) ^ POLY;    /* rotate and XOR with polynomic */  
	        else                          /* b15 is clear... */  
	            crc <<= 1;                  /* just rotate */  
	    }
		/* Loop for 8 bits */  
	    crc &= 0xFFFF;                  /* Ensure CRC remains 16-bit value */  
	}                               /* Loop until num=0 */  
	
	return crc;                    /* Return updated CRC */  
}  


static void fill_random_string(buf_t *b)
{
	char *rand_str;
	char tmp[128];
	static int count = 0;
	int len;
	int n;
	kal_uint16 val;

	if (!b)
	{
		return;
	}
	
	rand_str = get_random_string();
	sprintf(tmp, "buf%d", count++);
	strcat(b->pbuff, tmp);
	strcat(b->pbuff, rand_str);
	b->len = strlen(b->pbuff);

	for (len = b->len; b->len < b->size - 8; )
	{
		if (len + b->len >= b->size - 8)
		{
			n = b->size - 8 - b->len;
		}
		else
		{
			n = len;
		}

		memmove(&b->pbuff[b->len], b->pbuff, n);
		b->len += n;
	}

	val = test_crc16((kal_uint8 *)(b->pbuff), b->len, 0xFFFF);
	b->pbuff[b->len++] = val & 0xff;
	b->pbuff[b->len++] = (val >> 8) & 0xff;
}

static Boolean check_crc16(buf_t *b)
{
	kal_uint16 val;
	kal_uint16 v;

	if (!b)
	{
		return FALSE;
	}

	val = test_crc16((kal_uint8 *)(b->pbuff), b->len - 2, 0xFFFF);

	v = (kal_uint8)b->pbuff[b->len - 1];
	v <<= 8;
	v |= (kal_uint8)b->pbuff[b->len - 2];

	return v == val;
}

void test1()
{
	char *s = "aaaa bbbb\r\n";
	int i;
	buf_t *b1;
	buf_t *barray[NR_SIZE];
	
	static Boolean inited;
	static int count = 0;
	int size;

	print_mem_pool_array();

	if (!(b1 = alloc_buffer()))
	{
		printf("no buffer!\n");
		return;
	}



	for (i = 0; i < NR_SIZE / 3; i++)
	{
#if defined(LINUX) || defined(WIN32)
		srand((unsigned int)time(NULL));
		size = rand() % 1024;
#else
		size = OS_GetSysTick() % 4;
#endif
		if (size == 0)
		{
			size = 3;
		}
	
		if ((barray[i] = new_alloc_buffer(size)) != NULL)
		{
			fill_random_string(barray[i]);	
		}
	}

	printf("after alloc!\n");
	print_mem_pool_array();
	
	free_buffer(b1);

	for (i = 0; i < NR_SIZE / 3; i++)
	{
		if (barray[i] && !check_crc16(barray[i]))
		{
			printf("val changed@%d!\n", i);
		}

		if (barray[i])
		{
			new_free_buffer(barray[i]);
		}
	}
	
	print_mem_pool_array();
}

void test_data_buff()
{
	char *s = "aaaa bbbb\r\n";
	int i;
	buf_t *b1;
	buf_t *barray[NR_SIZE];
	
	static Boolean inited;
	static int count = 0;
	char tmp[32];
	char *rand_str;
	int size;

	init_mem_pool(NULL);
	print_mem_pool_array();

	if (!(b1 = alloc_buffer()))
	{
		printf("no buffer!\n");
		return;
	}
	
	for (i = 0; i < NR_SIZE / 3; i++)
	{
#if defined(LINUX) || defined(WIN32)
		srand((unsigned int)time(NULL));
		size = rand() % 1024;
#else
		size = OS_GetSysTick() % 4;
#endif
		if (size == 0)
		{
			size = 1;
		}
	
		if ((barray[i] = new_alloc_buffer(size)) != NULL)
		{
			fill_random_string(barray[i]);	
		}
	}

	printf("after alloc!\n");
	print_mem_pool_array();

#if 1
	if (!inited)
	{
		init_buff_queue(&tx_queue, "test queue");
		inited = TRUE;
	}
	
	init_buffer(b1);
	for (i = 0; i < strlen(s); i++)
	{
		add_char(b1, s[i]);
	}

	add_buf(&tx_queue, b1);
	if (count++ < 3)
	{
		init_buffer(b1);
		sprintf(tmp, "buf%d", count);
		strcat(b1->pbuff, tmp);
		rand_str = get_random_string();
		strcat(b1->pbuff, rand_str);
		b1->len = strlen(b1->pbuff);
		add_buf(&tx_queue, b1);
	}
	else 
	{
		remove_buf(&tx_queue);
	}

	if (count++ < 3)
	{
		init_buffer(b1);
		rand_str = get_random_string();
		sprintf(tmp, "buf%d", count);
		strcat(b1->pbuff, tmp);
		strcat(b1->pbuff, rand_str);
		b1->len = strlen(b1->pbuff);

		add_buf(&tx_queue, b1);
	}
	else 
	{
		remove_buf(&tx_queue);
	}
	print_queue(&tx_queue);
	printf("total = %d\r\n", buf_get_total_len(&tx_queue));

	remove_buf(&tx_queue);
	print_queue(&tx_queue);

	if (count++ < 3)
	{
		init_buffer(b1);
		rand_str = get_random_string();
		sprintf(tmp, "buf%d", count);
		strcat(b1->pbuff, tmp);
		strcat(b1->pbuff, rand_str);
		b1->len = strlen(b1->pbuff);

		add_buf(&tx_queue, b1);
	}
	print_queue(&tx_queue);
#endif

	free_buffer(b1);

	for (i = 0; i < NR_SIZE / 3; i++)
	{
		if (barray[i] && !check_crc16(barray[i]))
		{
			printf("val changed@%d!\n", i);
		}

		if (barray[i])
		{
			new_free_buffer(barray[i]);
		}
	}
	print_mem_pool_array();
}

#ifdef LINUX
int main(void)
{
	pthread_t t1;
	pthread_t t2;
	
	test_data_buff();

	pthread_create(&t1, NULL, &test1, NULL);
	pthread_create(&t2, NULL, &test1, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	return 0;
}
#endif
#endif

/*显示buf信息*/
void data_buf_show_info(void)
{
	mem_pool_t *pool = pool_array;
	int i = 0;

	printf("BUF POOL-------\n");
	printf("%-6s %-6s %-6s\n", "SIZE", "SUM", "FREE");
	printf("%-6s %-6s %-6s\n", "---", "---", "---");
	for (; i < BLK_NUM; i++)
	{
		printf("%-6d %-6d %-6d\n", pool[i].blk_size, pool[i].blk_cnt, pool[i].free_cnt);
	}
}

