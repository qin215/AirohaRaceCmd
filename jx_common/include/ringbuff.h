#ifndef __RINGBUFFER_H
#define	__RINGBUFFER_H
#include "ServerCmd.h"

typedef struct ringbuffer 
{
	kal_uint8 *data;
	int size;
	int count;			// ืÜด๓ะก
	int read_pos;
	int write_pos;
#ifndef WIN32
	OsMutex mutex;
#endif
} ring_buf_t;

struct ringbuffer *ringbuffer_create(unsigned int byte);
void ringbuffer_destroy(struct ringbuffer *ring_buf);
void ringbuffer_reset(struct ringbuffer *ring_buf);

unsigned int ringbuffer_put(struct ringbuffer *ring_buf, 
 	const unsigned char *buf, int len);

unsigned int ringbuffer_get(struct ringbuffer *ring_buf,
 		unsigned char *buf, int len);

void ringbuffer_print(struct ringbuffer *ring_buf, int cnt);

Boolean ringbuffer_is_empty(struct ringbuffer *ring_buf);
Boolean ringbuffer_is_full(struct ringbuffer *ring_buf);
int ringbuffer_len(struct ringbuffer *ring_buf);

#endif
