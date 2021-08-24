#include "stdafx.h"
#include <initguid.h>
#include "homi_common.h"
#include "f32x_util.h"

#pragma comment(lib,"Iphlpapi.lib")

// DLL globals
CUsbIF	sgCUsbIF;
static DWORD	sgdwNumDevices = 0;
static DWORD	sgdwWriteTimeout = 0;
static DWORD	sgdwReadTimeout = 0;

// Private functions
static BOOL ValidParam(LPDWORD lpdwPointer);
static BOOL ValidParam(LPVOID lpVoidPointer);
static BOOL ValidParam(LPVOID lpVoidPointer, LPDWORD lpdwPointer);
static BOOL ValidParam(LPDWORD lpdwPointer1, LPDWORD lpdwPointer2);
static BOOL ValidParam(HANDLE* lpHandle);


DEFINE_GUID(GUID_INTERFACE_SILABS_BULK, 
			0x37538c66, 0x9584, 0x42d3, 0x96, 0x32, 0xeb, 0xad, 0xa, 0x23, 0xd, 0x13);

F32x_STATUS 
F32x_GetNumDevices(LPDWORD lpdwNumDevices)
{
	F32x_STATUS	status = F32x_DEVICE_NOT_FOUND;

	// Validate parameter
	if (!ValidParam(lpdwNumDevices))
	{
		return F32x_INVALID_PARAMETER;
	}

	// Must set the GUID for functions that access the registry.
	sgCUsbIF.SetGUID(GUID_INTERFACE_SILABS_BULK);
	*lpdwNumDevices = sgCUsbIF.GetNumDevices();

	if (*lpdwNumDevices > 0)
	{
		status = F32x_SUCCESS;
	}

	return status;
}

//------------------------------------------------------------------------
// F32x_GetProductString()
//
// Find the product string of a device by index in the registry.
//------------------------------------------------------------------------
F32x_STATUS 
F32x_GetProductString(DWORD dwDeviceNum, LPVOID lpvDeviceString, DWORD dwFlags)
{
	F32x_STATUS			status	= F32x_DEVICE_NOT_FOUND;
	CDeviceListEntry	dev;

	// Validate parameter
	if (!ValidParam(lpvDeviceString))
	{
		return F32x_INVALID_PARAMETER;
	}

	// Must set the GUID for functions that access the registry.
	sgCUsbIF.SetGUID(GUID_INTERFACE_SILABS_BULK);
	sgCUsbIF.GetDeviceStrings(dwDeviceNum, dev);

	switch (dwFlags)
	{
	case F32x_RETURN_SERIAL_NUMBER:
		if (dev.m_serialnumber.length() > 0)
		{
			strcpy((char*)lpvDeviceString, dev.m_serialnumber.c_str());
			status = F32x_SUCCESS;
		}
		break;
	case F32x_RETURN_DESCRIPTION:
		if (dev.m_friendlyname.length() > 0)
		{
			strcpy((char*)lpvDeviceString, dev.m_friendlyname.c_str());
			status = F32x_SUCCESS;
		}
		break;
	default:
		break;
	}

	return status;
}

//------------------------------------------------------------------------
// F32x_Open()
//
// Open a file handle to access a Silabs device by index number.  The open
// routine determines the device's full name and uses it to open the handle.
//------------------------------------------------------------------------
F32x_STATUS 
F32x_Open(DWORD dwDevice, HANDLE* cyHandle)
{
	F32x_STATUS	status = F32x_DEVICE_NOT_FOUND;

	// Validate parameter
	if (!ValidParam(cyHandle))
	{
		return F32x_INVALID_PARAMETER;
	}

	// Must set the GUID for functions that access the registry.
	sgCUsbIF.SetGUID(GUID_INTERFACE_SILABS_BULK);

	if (cyHandle)
	{
		*cyHandle = sgCUsbIF.Open(dwDevice);
		if (*cyHandle != INVALID_HANDLE_VALUE)
		{
			status = F32x_SUCCESS;
		}
	}
	else
	{
		status = F32x_INVALID_HANDLE;
	}

	return status;
}

//------------------------------------------------------------------------
// F32x_Close()
//
// Close file handle used to access a Silabs device.
//------------------------------------------------------------------------
F32x_STATUS
F32x_Close(HANDLE cyHandle)
{
	F32x_STATUS	status = F32x_INVALID_HANDLE;

	if ((cyHandle != NULL) && (cyHandle != INVALID_HANDLE_VALUE))
	{
		::CloseHandle(cyHandle);
		status = F32x_SUCCESS;
	}

	return status;
}


//------------------------------------------------------------------------
// F32x_Read()
//
// Read data from USB device.
// If read timeout value has been set, check RX queue until F32x_RX_COMPLETE
// flag bit is set.  If timeout the occurs before F32x_RX_COMPLETE, return
// error.  If no timeout has been set attempt read immediately. 
//------------------------------------------------------------------------
F32x_STATUS
F32x_Read(HANDLE cyHandle, LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned)
{
	F32x_STATUS	status = F32x_SUCCESS;

	// Validate parameters
	if (!ValidParam(lpBuffer, lpdwBytesReturned))
	{
		return F32x_INVALID_PARAMETER;
	}

	// Check for a valid Handle value
	if (cyHandle != INVALID_HANDLE_VALUE)
	{
		// Check that the read length is within range
		if ((dwBytesToRead > 0) && (dwBytesToRead <= F32x_MAX_READ_SIZE))
		{
			// Read transfer packet
			while(!ReadFile(cyHandle, lpBuffer, dwBytesToRead, lpdwBytesReturned, NULL))
			{	// Device IO failed.
				status = F32x_READ_ERROR;
			} 
		}
		else
			status = F32x_INVALID_REQUEST_LENGTH;
	}
	else 
		status = F32x_INVALID_HANDLE;

	return status;
}

//------------------------------------------------------------------------
// F32x_Write()
//
// Write data to USB device.
// If write timeout value has been set, continue write attempts until
// successful or timeout occurs.
//------------------------------------------------------------------------
F32x_STATUS
F32x_Write(HANDLE cyHandle, LPVOID lpBuffer, DWORD dwBytesToWrite, LPDWORD lpdwBytesWritten)
{
	F32x_STATUS	status = F32x_INVALID_HANDLE;

	// Validate parameters
	if (!ValidParam(lpBuffer, lpdwBytesWritten))
	{
		return F32x_INVALID_PARAMETER;
	}

	if (cyHandle != INVALID_HANDLE_VALUE)
	{
		if ((dwBytesToWrite > 0) && (dwBytesToWrite <= F32x_MAX_WRITE_SIZE))
		{
			if (!WriteFile(cyHandle, lpBuffer, dwBytesToWrite, lpdwBytesWritten, NULL))
			{
				status = F32x_WRITE_ERROR;

				if (sgdwWriteTimeout > 0)
				{
					DWORD	dwStart	= GetTickCount();
					DWORD	dwEnd	= GetTickCount();

					// Keep trying to write until success or timeout
					while((dwEnd - dwStart) < sgdwWriteTimeout && status != F32x_SUCCESS)
					{
						if (WriteFile(cyHandle, lpBuffer, dwBytesToWrite, lpdwBytesWritten, NULL))
						{
							status = F32x_SUCCESS;	// Write succeeded after > 1 attempts.
						}

						dwEnd = GetTickCount();
					}
				}
			}
			else
				status = F32x_SUCCESS;				// Write succeeded on first attempt.
		}
		else
			status = F32x_INVALID_REQUEST_LENGTH;
	}

	return status;
}


//------------------------------------------------------------------------
// ValidParam(LPDWORD)
//
// Checks validity of an LPDWORD pointer value.
//------------------------------------------------------------------------
static BOOL ValidParam(LPDWORD lpdwPointer)
{
	DWORD temp = 0;

	try 
	{
		temp = *lpdwPointer;
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------
// ValidParam(LPVOID)
//
// Checks validity of an LPVOID pointer value.
//------------------------------------------------------------------------
static BOOL ValidParam(LPVOID lpVoidPointer)
{
	BYTE temp = 0;

	try 
	{
		temp = *((BYTE*)lpVoidPointer);
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}


//------------------------------------------------------------------------
// ValidParam(HANDLE*)
//
// Checks validity of an HANDLE* pointer value.
//------------------------------------------------------------------------
static BOOL ValidParam(HANDLE* lpHandle)
{
	HANDLE temp = 0;

	try 
	{
		temp = *lpHandle;
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------
// ValidParam(LPVOID, LPDWORD)
//
// Checks validity of LPVOID, LPDWORD pair of pointer values.
//------------------------------------------------------------------------
static BOOL ValidParam(LPVOID lpVoidPointer, LPDWORD lpdwPointer)
{
	if (ValidParam(lpVoidPointer))
		if (ValidParam(lpdwPointer))
			return TRUE;

	return FALSE;
}

//------------------------------------------------------------------------
// ValidParam(LPDWORD, LPDWORD)
//
// Checks validity of LPDWORD, LPDWORD pair of pointer values.
//------------------------------------------------------------------------
static BOOL ValidParam(LPDWORD lpdwPointer1, LPDWORD lpdwPointer2)
{
	if (ValidParam(lpdwPointer1))
		if (ValidParam(lpdwPointer2))
			return TRUE;

	return FALSE;
}

int Char2Int(char c)
{
	switch(c)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return c-'0';
		break;

	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		return c-'a'+10;
		break;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		return c-'A'+10;
		break;
	default:
		return 0xFFFFFFFF;

	}

}

BYTE Hex2Char(BYTE data)
{
	BYTE hex= data&0x0f;

	if(hex>=0x00 && hex<=0x09)
		hex += '0';
	else if(hex>=0x0a && hex<=0x0f)
		hex += ('A'-0x0a);

	return hex;
}

BYTE Char2Hex(BYTE hex) 
{
	if (hex>='a' && hex<='f')
	{
		hex -= 0x20;
	}

	if ((hex>='0' && hex<='9') || (hex>='A' && hex<='F'))
	{
		if(hex>='0' && hex<='9')
			hex -= '0';
		if(hex>='A' && hex<='F')
			hex -= ('A' - 0x0A);
		return hex;
	}

	return 0;
}

BOOL String2HexData(CString &str, UCHAR * outBuffer)
{
	int iLen, i;
	char * p;

	str.Remove('h');

	iLen = str.GetLength();
	if (iLen%2) //首字节添加0, 补足偶数
	{
		str.Insert(iLen, "0");
		iLen++;
	}

	p = str.GetBuffer(iLen);
	for(i=0; i<iLen/2; i++)
	{
		int temp;

		*outBuffer = 0;
		temp = Char2Int(*p);
		if( temp == 0xFFFFFFFF )
			return FALSE;
		*outBuffer = temp;
		p++;
		*outBuffer <<= 4;
		temp = Char2Int(*p);
		if( temp == 0xFFFFFFFF )
			return FALSE;
		*outBuffer |= temp;
		p++;
		*outBuffer++;
	}

	return TRUE;
}

/*
 * 二进制数据转为hex数据
 */
int Binary2HexData(const UCHAR * inBuff, const int len, UCHAR *outBuff, int out_len)
{
	int i;
	int index;

	if (out_len < len * 2)
	{
		return -1;
	}

	for (i = 0, index = 0; i < len; i++)
	{
		index += sprintf((char *)&outBuff[index], "%02X", inBuff[i]);
	}

	return index;
}


UINT64 String2Int(CString &str)
{
	str.Remove('h');

	int iLen = str.GetLength();
	LPTSTR lpszHexNum = str.GetBuffer(iLen);
	char * p = lpszHexNum;
	UINT64 dwVal = 0;
	int r;

	for(int i = 0; i < iLen; i++)
	{
		dwVal = dwVal << 4;
		r = Char2Int(*p);
		dwVal |= r ;
		p++;
	}

	return dwVal;

}


BOOL String2HexData(CString &str, UCHAR * outBuffer, int buff_len)
{
	int iLen, i;
	char * p;

	str.Remove('h');

	iLen = str.GetLength();
	if (iLen%2) //首字节添加0, 补足偶数
	{
		str.Insert(iLen, "0");
		iLen++;
	}

	if (iLen / 2 > buff_len)
	{
		return FALSE;
	}

	p = str.GetBuffer(iLen);
	for(i=0; i<iLen/2; i++)
	{
		int temp;

		*outBuffer = 0;
		temp = Char2Int(*p);
		if( temp == 0xFFFFFFFF )
			return FALSE;
		*outBuffer = temp;
		p++;
		*outBuffer <<= 4;
		temp = Char2Int(*p);
		if( temp == 0xFFFFFFFF )
			return FALSE;
		*outBuffer |= temp;
		p++;
		*outBuffer++;
	}

	return TRUE;
}

extern "C"
BOOL MyString2HexData(const char *hex_str, UCHAR * outBuffer, int len)
{
	int iLen, i;
	char * p;
	CString str(hex_str);

	return String2HexData(str, outBuffer, len);
}



/////////////////////////////////////////////////////////////////////////////
CString  GetMac1(void)
{
	CString strMac;

	ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfoBkp = NULL, *pAdapterInfo = (IP_ADAPTER_INFO*)new char[ulAdapterInfoSize];
	if( GetAdaptersInfo(pAdapterInfo, &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW ) // 缓冲区不够大
	{
		delete pAdapterInfo;
		pAdapterInfo = (IP_ADAPTER_INFO*)new char[ulAdapterInfoSize];
		pAdapterInfoBkp = pAdapterInfo;
	}

	if( GetAdaptersInfo(pAdapterInfo, &ulAdapterInfoSize) == ERROR_SUCCESS )
	{
		do{ // 遍历所有适配器
			if(pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET)    // 判断是否为以太网接口
			{
				// pAdapterInfo->Description 是适配器描述
				// pAdapterInfo->AdapterName 是适配器名称

				for(UINT i = 0; i < pAdapterInfo->AddressLength; i++)
				{
					char szTmp[8];
					unsigned char addr=pAdapterInfo->Address[i];
					addr=addr^0x55;
					sprintf(szTmp, "%02X",addr);//, (i == pAdapterInfo->AddressLength-1) ? '\0':'-');
					strMac+=szTmp; 
					// strMac.append(szTmp);
				}
				break;
			}
			pAdapterInfo = pAdapterInfo->Next;
		}while(pAdapterInfo);
	}

	if(pAdapterInfoBkp)
		delete pAdapterInfoBkp;

	return strMac;
}

extern "C"
void win32_start_thread(AFX_THREADPROC proc, LPVOID param)
{
	/*m_pUartRecvThread = */::AfxBeginThread(proc, param); //接下来做啥就直接调用pThead就行.
}