#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include "osal.h"
#endif
#include "ringbuff.h"

/*
 * size Ϊ 2^n 
 */
struct ringbuffer *ringbuffer_create(unsigned int size)
{
	struct ringbuffer *ring_buf;

#ifndef WIN32
	ring_buf = (struct ringbuffer *)OS_MemAlloc(sizeof(struct ringbuffer));
#else
	ring_buf = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
#endif
	if (!ring_buf)
	{
		printf("no mem!\n");
		return NULL;
	}

#ifndef WIN32
	ring_buf->data = (kal_uint8 *)OS_MemAlloc(size);
#else	
	ring_buf->data = (kal_uint8 *)malloc(size);
#endif
	if (!ring_buf->data) 
	{
#ifndef WIN32
		OS_MemFree(ring_buf);
#else
		free(ring_buf);
#endif
		return NULL;
	}

	ring_buf->size = size;
	ring_buf->count = 0;
	ring_buf->read_pos = 0;
	ring_buf->write_pos = 0;

#ifndef WIN32
	if (OS_MutexInit(&ring_buf->mutex) != OS_SUCCESS)
	{
		printf("mutex init failed!\n");
	}
#endif

	return ring_buf;
}

void ringbuffer_destroy(struct ringbuffer *ring_buf)
{
#ifndef WIN32
	OS_MutexDelete(ring_buf->mutex);
#endif

#ifndef WIN32
	OS_MemFree(ring_buf->data);
#else
	free(ring_buf->data);
#endif

	ring_buf->data = NULL;
#ifndef WIN32
	OS_MemFree(ring_buf);
#else
	free(ring_buf);
#endif
	ring_buf = NULL;
}

/**
 * ringbuffer_put - puts some data into the ringbuffer, no locking version
 * @ring_buf: the ringbuffer to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the ringbuffer depending on the free space, and returns the number of
 * bytes copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */

unsigned int ringbuffer_put(struct ringbuffer *ring_buf,   
		const kal_uint8 *buffer, int len)   
{   
	int n;
	int left;
	
	if (ringbuffer_is_full(ring_buf))
	{
		return 0;
	}

	left = ring_buf->size - ring_buf->count;

	if (left < len)
	{
		printf("buf is small(len=%d, left=%d)\n", len, left);
		len = left;
	}

	if (ring_buf->write_pos >= ring_buf->read_pos)
	{
		if (len <= (ring_buf->size - ring_buf->write_pos))
		{
			memcpy(ring_buf->data + ring_buf->write_pos, buffer, len);
			ring_buf->write_pos += len;  

			if (ring_buf->write_pos == ring_buf->size)
			{
				ring_buf->write_pos = 0;
			}
			
			goto done;
		}

		n = ring_buf->size - ring_buf->write_pos;
		memcpy(ring_buf->data + ring_buf->write_pos, buffer, n);
		ring_buf->write_pos = 0;

		left = len - n;
		memcpy(ring_buf->data, &buffer[n], left);
		ring_buf->write_pos = left;
		
		goto done;
	}

	memcpy(ring_buf->data + ring_buf->write_pos, &buffer[0], len);
	ring_buf->write_pos += len;
	
done:
#ifndef WIN32
	OS_MutexLock(ring_buf->mutex);
#endif
	ring_buf->count += len;
#ifndef WIN32
	OS_MutexUnLock(ring_buf->mutex);
#endif
	return len;   
}  

/**
 *  ringbuffer_print - print ringbuf data
 *  @ring_buf: the ringbuffer to be used.
 *  @cnt : the number byte to be print.
 */
void ringbuffer_print(struct ringbuffer *ring_buf, int cnt)
{

}


/**
 *  ringbuffer_get - gets some data from the ringbuffer, no locking version
 *  @ring_buf: the ringbuffer to be used.
 *  @buffer: where the data must be copied.
 *  @len: the size of the destination buffer.
 * 
 *  This function copies at most @len bytes from the ringbuffer into the
 *  @buffer and returns the number of copied bytes.
 * 
 *  Note that with only one concurrent reader and one concurrent
 *  writer, you don't need extra locking to use these functions.
 */
unsigned int ringbuffer_get(struct ringbuffer *ring_buf,
		kal_uint8 *buffer, int len)
{
	int n;
	int left;
	kal_uint8 *src;
	
	if (ringbuffer_is_empty(ring_buf))
	{
		return 0;
	}
	
	len = min(len, ringbuffer_len(ring_buf));
	
	if (ring_buf->read_pos >= ring_buf->write_pos)
	{
		n = ring_buf->size - ring_buf->read_pos;

		if (n > len)
		{
			memcpy(buffer, ring_buf->data + ring_buf->read_pos, len);
			ring_buf->read_pos += len;

			goto done;
		}

		memcpy(buffer, ring_buf->data + ring_buf->read_pos, n);

		left = len - n;
		memcpy(&buffer[n], ring_buf->data, left);

		ring_buf->read_pos = left;

		goto done;
	}

	memcpy(&buffer[0], ring_buf->data + ring_buf->read_pos, len);
	ring_buf->read_pos += len;
	
done:	
#ifndef WIN32
	OS_MutexLock(ring_buf->mutex);
#endif
	ring_buf->count -= len;
#ifndef WIN32
	OS_MutexUnLock(ring_buf->mutex);
#endif	
	return len;
}


/**
 * ringbuffer_reset - removes the entire ringbuffer content
 * @ring_buf: address of the ringbuffer to be used
 *
 * Note: usage of ringbuffer_reset() is dangerous. It should be only called when the
 * ringbuffer is exclusived locked or when it is secured that no other thread is
 * accessing the fifo.
 */
void ringbuffer_reset(struct ringbuffer *ring_buf)
{
#ifndef WIN32
	OS_MutexLock(ring_buf->mutex);
#endif
	ring_buf->write_pos = ring_buf->read_pos = 0;
#ifndef WIN32
	OS_MutexUnLock(ring_buf->mutex);
#endif	
}

Boolean ringbuffer_is_empty(struct ringbuffer *ring_buf)
{
	return ring_buf->count == 0;
}

Boolean ringbuffer_is_full(struct ringbuffer *ring_buf)
{
	return ring_buf->size == ring_buf->count;
}

int ringbuffer_len(struct ringbuffer *ring_buf)
{
	return ring_buf->count;
}


#if 1 //def TEST
void test_ring_buff()
{
	ring_buf_t *ring;
	kal_uint8 *data = "aaaaaaaaabbbb";
	char out[32];
	int len;
	
	ring = ringbuffer_create(8);

	if (!ring)
	{
		return;
	}

	len = ringbuffer_put(ring, data, strlen(data));
	printf("put len=%d\n", len);

	len = ringbuffer_get(ring, out, sizeof(out));
	printf("get len=%d\n", len);

	len = ringbuffer_put(ring, data, strlen(data));
	printf("put len=%d\n", len);

	len = ringbuffer_put(ring, data, strlen(data));
	printf("put len=%d\n", len);

	len = ringbuffer_put(ring, data, strlen(data));
	printf("put len=%d\n", len);

	len = ringbuffer_get(ring, out, sizeof(out));
	printf("get len=%d\n", len);

	ringbuffer_destroy(ring);
}
#endif
