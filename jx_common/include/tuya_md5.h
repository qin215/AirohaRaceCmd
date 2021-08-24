#ifndef MD5_H
#define MD5_H

typedef struct
{
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
} Tuya_MD5_CTX;


#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))
#define FF(a,b,c,d,x,s,ac) \
          { \
          a += F(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define GG(a,b,c,d,x,s,ac) \
          { \
          a += G(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define HH(a,b,c,d,x,s,ac) \
          { \
          a += H(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define II(a,b,c,d,x,s,ac) \
          { \
          a += I(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
void Tuya_MD5Init(Tuya_MD5_CTX *context);
void Tuya_MD5Update(Tuya_MD5_CTX *context, unsigned char *input, unsigned int inputlen);
void Tuya_MD5Final(Tuya_MD5_CTX *context, unsigned char digest[16]);
void Tuya_MD5Transform(unsigned int state[4], unsigned char block[64]);
void Tuya_MD5Encode(unsigned char *output, unsigned int *input, unsigned int len);
void Tuya_MD5Decode(unsigned int *output, unsigned char *input, unsigned int len);

#endif