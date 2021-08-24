#ifdef WIN32
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#endif

#include "ServerCmd.h"
#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"
#include "hx_flash_opt.h"
#include "mywin.h"

/*
 * 计算crc32校验值
 */
unsigned int crc_32_calculate(unsigned char* content, int numread, unsigned int crc)
{
	DWORD_UNION u_crc;
	DWORD_UNION u_new_crc;
	BYTE_UNION u_d;
	DWORD_UNION u_crc_temp;
	int i;

	u_crc_temp.int_v = 0x3456789a;

	u_crc.int_v = crc;

	for (i = 0; i < numread; i++)
	{
		u_d.byte_v = content[i];

		u_new_crc.bits_v.b0 =u_d.bits_v.b6 ^ u_d.bits_v.b0 ^u_crc.bits_v.b24 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b1 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b2 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b3 = u_d.bits_v.b7 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b4 = u_d.bits_v.b6 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b5 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b6 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b7 = u_d.bits_v.b7 ^ u_d.bits_v.b5 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b8 = u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28;

		u_new_crc.bits_v.b9 = u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b1 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b10 = u_d.bits_v.b5 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b2 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b11 = u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b3 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28;
		u_new_crc.bits_v.b12 = u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b4 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b13 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b5 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b14 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b6 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b15 = u_d.bits_v.b7 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_crc.bits_v.b7 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b16 = u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b8 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b17 = u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b9 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b18 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b10 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b19 = u_d.bits_v.b7 ^ u_d.bits_v.b3 ^ u_crc.bits_v.b11 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b20 = u_d.bits_v.b4 ^ u_crc.bits_v.b12 ^ u_crc.bits_v.b28;
		u_new_crc.bits_v.b21 = u_d.bits_v.b5 ^ u_crc.bits_v.b13 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b22 = u_d.bits_v.b0 ^ u_crc.bits_v.b14 ^ u_crc.bits_v.b24;
		u_new_crc.bits_v.b23 = u_d.bits_v.b6 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b15 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b24 = u_d.bits_v.b7 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b16 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b25 = u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b17 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27;
		u_new_crc.bits_v.b26 = u_d.bits_v.b6 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b18 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b27 = u_d.bits_v.b7 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b19 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b28 = u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b20 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b29 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b3 ^ u_crc.bits_v.b21 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b30 = u_d.bits_v.b7 ^ u_d.bits_v.b4 ^ u_crc.bits_v.b22 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b31 = u_d.bits_v.b5 ^ u_crc.bits_v.b23 ^ u_crc.bits_v.b29;

		u_crc.int_v=u_new_crc.int_v;
	}

	return u_crc.int_v;
}



/*
 * 计算crc32校验值
 */
unsigned int crc_32_crypt(unsigned char* content, int numread, unsigned int crc)
{
	DWORD_UNION u_crc;
	DWORD_UNION u_new_crc;
	BYTE_UNION u_d;
	int i;

	u_crc.int_v = crc;

	for (i = 0; i < numread; i++)
	{
		u_d.byte_v = content[i];

		u_new_crc.bits_v.b0 = u_d.bits_v.b6 ^ u_d.bits_v.b0 ^u_crc.bits_v.b24 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b1 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b2 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b3 = u_d.bits_v.b7 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b4 = u_d.bits_v.b6 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b5 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b6 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b7 = u_d.bits_v.b7 ^ u_d.bits_v.b5 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b31;
		
		u_new_crc.bits_v.b8 = u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b0 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28;
		u_new_crc.bits_v.b9 = u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b1 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b10 = u_d.bits_v.b5 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b2 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b11 = u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b3 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28;
		u_new_crc.bits_v.b12 = u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b4 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b13 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b5 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b14 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b6 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b15 = u_d.bits_v.b7 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_crc.bits_v.b7 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b31;
		
		u_new_crc.bits_v.b16 = u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b8 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b17 = u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b9 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b18 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b10 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b19 = u_d.bits_v.b7 ^ u_d.bits_v.b3 ^ u_crc.bits_v.b11 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b20 = u_d.bits_v.b4 ^ u_crc.bits_v.b12 ^ u_crc.bits_v.b28;
		u_new_crc.bits_v.b21 = u_d.bits_v.b5 ^ u_crc.bits_v.b13 ^ u_crc.bits_v.b29;
		u_new_crc.bits_v.b22 = u_d.bits_v.b0 ^ u_crc.bits_v.b14 ^ u_crc.bits_v.b24;
		u_new_crc.bits_v.b23 = u_d.bits_v.b6 ^ u_d.bits_v.b1 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b15 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b30;
		
		u_new_crc.bits_v.b24 = u_d.bits_v.b7 ^ u_d.bits_v.b2 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b16 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b25 = u_d.bits_v.b3 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b17 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b27;
		u_new_crc.bits_v.b26 = u_d.bits_v.b6 ^ u_d.bits_v.b4 ^ u_d.bits_v.b3 ^ u_d.bits_v.b0 ^ u_crc.bits_v.b18 ^ u_crc.bits_v.b24 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b27 = u_d.bits_v.b7 ^ u_d.bits_v.b5 ^ u_d.bits_v.b4 ^ u_d.bits_v.b1 ^ u_crc.bits_v.b19 ^ u_crc.bits_v.b25 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b28 = u_d.bits_v.b6 ^ u_d.bits_v.b5 ^ u_d.bits_v.b2 ^ u_crc.bits_v.b20 ^ u_crc.bits_v.b26 ^ u_crc.bits_v.b29 ^ u_crc.bits_v.b30;
		u_new_crc.bits_v.b29 = u_d.bits_v.b7 ^ u_d.bits_v.b6 ^ u_d.bits_v.b3 ^ u_crc.bits_v.b21 ^ u_crc.bits_v.b27 ^ u_crc.bits_v.b30 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b30 = u_d.bits_v.b7 ^ u_d.bits_v.b4 ^ u_crc.bits_v.b22 ^ u_crc.bits_v.b28 ^ u_crc.bits_v.b31;
		u_new_crc.bits_v.b31 = u_d.bits_v.b5 ^ u_crc.bits_v.b23 ^ u_crc.bits_v.b29;

		u_crc.int_v += u_crc.int_v * crc + u_new_crc.int_v;
	}

	return u_crc.int_v;
}

/* Table of CRC constants - implements x^16+x^12+x^5+1 */
static const kal_uint16 crc16_tab[] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

kal_uint16 crc16_ccitt_cal(kal_uint16 crc_start, unsigned char *buf, int len)
{
	int i;
	kal_uint16 cksum;

	cksum = crc_start;

	for (i = 0;  i < len;  i++)
		cksum = crc16_tab[((cksum>>8) ^ *buf++) & 0xff] ^ (cksum << 8);

	return cksum;
}


#define SPP_MAX_PACKET_SIZE    10
#define SPP_SEND_BUFFER_SIZE   (SPP_MAX_PACKET_SIZE * 1)

static uint8_t spp_tx_buffer[SPP_SEND_BUFFER_SIZE] = {0};
static uint16_t spp_tx_buf_data_total = 0;

static uint16_t read_ptr;			// added by qin
static uint16_t write_ptr;

static Boolean check_buffer_full()
{
	if (spp_tx_buf_data_total >= SPP_SEND_BUFFER_SIZE)
	{
		DBG_LOG_APP_AIRAPP( "[APP_CustSPP] buffer full total=%d, size=%d", spp_tx_buf_data_total, SPP_SEND_BUFFER_SIZE);
		return TRUE;
	}

	return FALSE;
}

static Boolean check_buffer_empty()
{
	return spp_tx_buf_data_total == 0;
}


/*
  * 从ring buffer中拷贝数据,  返回已拷贝的数据
  */
int spp_copy_buffer(uint8_t *buffer, int size)
{
	int n = 0;
	int left = 0;
	
	if (check_buffer_empty())
	{
		return 0;
	}

	if (read_ptr < write_ptr)
	{
		n = write_ptr - read_ptr;
		if (n > size)
		{
			n = size;
		}

		memcpy(buffer, &spp_tx_buffer[read_ptr], n);
		read_ptr += n;
		spp_tx_buf_data_total -= n;

		return n;
	}

	n = SPP_SEND_BUFFER_SIZE - read_ptr;
	if (n >= size)
	{
		n = size;
	}

	left = size - n;

	memcpy(buffer, &spp_tx_buffer[read_ptr], n);
	read_ptr += n;
	read_ptr %= SPP_SEND_BUFFER_SIZE;
	spp_tx_buf_data_total -= n;

	if (left > 0)
	{
		if (left > write_ptr)
		{
			left = write_ptr;
		}

		memcpy(&buffer[n], spp_tx_buffer, left);
		n += left;

		read_ptr += left;
		spp_tx_buf_data_total -= left;
	}

	return n;
}


int spp_push_tx_buf(const unsigned char * ptrData, unsigned int length)
{
	uint16_t left = 0;
	uint16_t n;

	if (check_buffer_full())
	{
		return 0;
	}

	if (read_ptr > write_ptr)
	{
		left = read_ptr  - write_ptr;

		if (left <= length)
		{
			DBG_LOG_APP_AIRAPP( "[APP_CustSPP] buffer too small read_ptr=%d, write_ptr=%d, left=%d, length=%d", read_ptr, write_ptr, left, length);
			length = left;
		}

		memcpy(&spp_tx_buffer[write_ptr], ptrData, length);
		write_ptr += length;
		spp_tx_buf_data_total += length;
		DBG_LOG_APP_AIRAPP( "[APP_CustSPP] update buffer var read_ptr=%d, write_ptr=%d, left=%d, length=%d", read_ptr, write_ptr, left, length);
		return length;
	}

	if (write_ptr + length >=  SPP_SEND_BUFFER_SIZE)
	{
		n = SPP_SEND_BUFFER_SIZE - write_ptr;
		left = length - n;
	}
	else
	{
		n = length;
		left = 0;
	}

	memcpy(&spp_tx_buffer[write_ptr], ptrData, n);
	write_ptr += n;
	write_ptr %= SPP_SEND_BUFFER_SIZE;
	spp_tx_buf_data_total += n;

	if (left)
	{
		// write_ptr 为 0
		if (left > read_ptr)
		{
			DBG_LOG_APP_AIRAPP( "[APP_CustSPP] buffer too small read_ptr=%d, write_ptr=%d, left=%d, length=%d, n = %d", read_ptr, write_ptr, left, length, n);
			left = read_ptr;
		}
		
		memcpy(&spp_tx_buffer[write_ptr], ptrData, left);
		write_ptr += left;
		spp_tx_buf_data_total += left;

		n += left;
	}

	return n;
}


void ring_buff_init()
{
	read_ptr = 0;				// added by qin
	write_ptr = 0;
	spp_tx_buf_data_total = 0;
}



void test_ring_buffer()
{
	kal_uint8 *data = "abcdefghij";
	char out[32];
	int len;

	memset(out, 0, sizeof(out));
	ring_buff_init();
	
	len = spp_push_tx_buf(data, strlen(data));
	printf("put len=%d\n", len);

	len = spp_copy_buffer(out, sizeof(out));
	out[len] = '\0';
	printf("get len=%d, s= '%s'\n", len, out);

	len = spp_push_tx_buf(data, strlen(data));
	printf("put len=%d\n", len);

	len = spp_copy_buffer(out, 3);
	out[len] = '\0';
	printf("get len=%d, s= '%s'\n", len, out);

	len = spp_copy_buffer(out, 3);
	out[len] = '\0';
	printf("get len=%d, s= '%s'\n", len, out);

	len = spp_copy_buffer(out, 3);
	out[len] = '\0';
	printf("get len=%d, s= '%s'\n", len, out);

	len = spp_push_tx_buf(data, strlen(data));
	printf("put len=%d\n", len);

	len = spp_push_tx_buf(data, strlen(data));
	printf("put len=%d\n", len);

	len = spp_copy_buffer(out, sizeof(out));
	out[len] = '\0';
	printf("get len=%d, s= '%s'\n", len, out);

	len = spp_copy_buffer(out, 3);
	out[len] = '\0';
	printf("get len=%d, s= '%s'\n", len, out);
}
