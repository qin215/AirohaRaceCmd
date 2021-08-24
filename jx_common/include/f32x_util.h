#ifndef __F32X_UTIL_H__
#define __F32X_UTIL_H__

#include "iphlpapi.h"
#include "UsbIF.h"

#ifdef __cplusplus
extern "C"
{
#endif

// GetProductString() function flags
#define		F32x_RETURN_SERIAL_NUMBER	0x00
#define		F32x_RETURN_DESCRIPTION		0x01

// Return codes
#define		F32x_SUCCESS				0x00
#define		F32x_DEVICE_NOT_FOUND		0xFF
#define		F32x_INVALID_HANDLE			0x01
#define		F32x_READ_ERROR				0x02
#define		F32x_RX_QUEUE_NOT_READY		0x03
#define		F32x_WRITE_ERROR			0x04
#define		F32x_RESET_ERROR			0x05
#define		F32x_INVALID_PARAMETER		0x06
#define		F32x_INVALID_REQUEST_LENGTH	0x07
#define		F32x_DEVICE_IO_FAILED		0x08

// RX Queue status flags
#define		F32x_RX_NO_OVERRUN			0x00
#define		F32x_RX_OVERRUN				0x01
#define		F32x_RX_READY				0x02

// Buffer size limits
#define		F32x_MAX_DEVICE_STRLEN		256
#define		F32x_MAX_READ_SIZE			4096
#define		F32x_MAX_WRITE_SIZE			4096

// Type definitions
typedef		int		F32x_STATUS;
typedef		char	F32x_DEVICE_STRING[F32x_MAX_DEVICE_STRLEN];


extern CUsbIF sgCUsbIF;
extern const GUID GUID_INTERFACE_SILABS_BULK;


F32x_STATUS F32x_GetNumDevices(
							   LPDWORD lpdwNumDevices
							   );

F32x_STATUS F32x_GetProductString(
								  DWORD dwDeviceNum,
								  LPVOID lpvDeviceString,
								  DWORD dwFlags
								  );

F32x_STATUS F32x_Open(
					  DWORD dwDevice,
					  HANDLE* cyHandle
					  ); 

F32x_STATUS F32x_Close(
					   HANDLE cyHandle
					   );

F32x_STATUS F32x_Read(
					  HANDLE cyHandle,
					  LPVOID lpBuffer,
					  DWORD dwBytesToRead,
					  LPDWORD lpdwBytesReturned
					  );

F32x_STATUS F32x_Write(
					   HANDLE cyHandle,
					   LPVOID lpBuffer,
					   DWORD dwBytesToWrite,
					   LPDWORD lpdwBytesWritten
					   );


int  Char2Int(char c);
BYTE Hex2Char(BYTE data);
BYTE Char2Hex(BYTE hex);

UINT64 String2Int(CString &str);
BOOL String2HexData(CString &str, UCHAR * outBuffer);


CString  GetMac1(void);

unsigned int crc_32_calculate(unsigned char* content, int numread, unsigned int crc);

#ifdef __cplusplus
}
#endif

#endif
