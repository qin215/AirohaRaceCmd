// mp_testDlg.h : header file
//

#if !defined(AFX_MP_TESTDLG_H__0F971A54_2D78_4FD7_A88C_CDA17796A1F3__INCLUDED_)
#define AFX_MP_TESTDLG_H__0F971A54_2D78_4FD7_A88C_CDA17796A1F3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dbt.h"

#include "ColorStatic.h"


#define DEVICE_ATMEL_FLASH    2
#define DEVICE_WINBOND_FLASH  1

#define WRITE_FLASH_START_CMD    0x3
#define WRITE_FLASH_DATA_CMD     0x4
#define WRITE_RESET_CONTROL_CMD  0x7
#define EEPROM_READ_CMD          0x9
#define EEPROM_WRITE_CMD         0xA
#define FLASH_ERASE_ALL_CMD      0xA0
#define FLASH_READ_CMD           0xA1
#define FLASH_WRITE_CMD          0xA2
#define FLASH_ERASE_SECTOR_CMD   0xA3
#define FLASH_ERASE_BLOCK_CMD    0xA4
#define FLASH_READ_MASS_CMD      0xA5

#define JTAG_GET_IDCODE_CMD	    0xA0
#define JTAG_MID_READ_CMD		0xA1
#define JTAG_MID_WRITE_CMD		0xA2
#define JTAG_STALL_CMD	        0xA3
#define JTAG_UNSTALL_CMD	    0xA4
#define JTAG_ERASE_ALL_CMD		0xA5
#define JTAG_READ_ALL_CMD		0xA6
#define JTAG_SPR_READ_CMD       0xA7
#define JTAG_SPR_WRITE_CMD      0xA8
#define JTAG_FLASH_WRITE_START  0xA9
#define JTAG_FLASH_WRITE_DATA   0xAA
#define JTAG_FLASH_WRITE_END    0xAB
#define JTAG_FLASH_READ_START   0xAC
#define JTAG_FLASH_READ_DATA    0xAD
#define JTAG_FLASH_WRITE_BTADDR  0xAE
#define REG_READ_CMD 0xC1
#define REG_WRITE_CMD 0xC2

#define TRANSFER_DATA_BYTE_EACH_TIME    60
#define JTAG_FRAME_LENGTH				2048


#define REG_READ_CMD 0xC1
#define REG_WRITE_CMD 0xC2
#define FLASH_WRITE_ENABLE 0xC3
#define FLASH_WRITE_DISABLE 0xC4

#define USB_CHECK_CMD   0x55

#define MAX_PACKET_SIZE_WRITE		512
#define FT_MSG_SIZE			0x0F //0x09
#define MAX_WRITE_PKTS		0x01
#define BK3254_ADDR_OFFSET_IN_BIN_FILE      0x80064
#define BK3254_NAME_OFFSET_IN_BIN_FILE      0x80034
#define BK3260_ADDR_OFFSET_IN_BIN_FILE      0x61065
#define BK3260_NAME_OFFSET_IN_BIN_FILE      0x61035
#define BK3262_ADDR_OFFSET_IN_BIN_FILE      0x61065			//TBD
#define BK3262_NAME_OFFSET_IN_BIN_FILE      0x61035			//TBD
#define BT_NAME_MAX_LENGTH_IN_BIN_FILE  32
#define BT_ADDR_LENGTH_IN_BIN_FILE      6
#define BK_CHECK_FILE_SIZE              (1024*1024)

//mult-USB device 
#define MULTI_USB_DEVICE_MAXNUM         20



//return result definition
#define BK_RET_SUCCESS                  0x00
#define BK_RET_OPEN_BIN_FILE_FAILED     0x01               
#define BK_RET_MALLOC_FAILED            0x02               
#define BK_RET_USB_WRITE_FAILED         0x03               
#define BK_RET_CONTENT_DIFF_FAILED      0x04
#define BK_RET_OPEN_CHECK_FILE_FAILED   0x05
#define BK_RET_JTAG_STALL_FAILED        0x09


typedef enum _beken_chip_type_
{
	BK3231,
	BK3431,
	BK2466,
	BK5866,
	BK2533,
	BK5933,
	BK2535,
	BK3231S,
	BK3256Flash,
	BK3256,
	BK2471,
	AI6060,
	BK3431S,
	AI6166,
	CHIP_NUM
}BEKEN_CHIP_TYPE;

typedef struct _bt_addr_name_offset_struct_
{
   unsigned int   addr_offset; 
   unsigned int   name_offset;
}BT_ADDR_NAME_OFFSET;

extern BT_ADDR_NAME_OFFSET   addr_name_offset[20];

#define WM_UPDATE_STATIC (WM_USER + 100)  

class CMp_testDlg : public CDialog
{
// Construction
public:
	CMp_testDlg(CWnd* pParent = NULL);	// standard constructor

	BOOL DeviceRead(unsigned int devIndex, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesRead, DWORD dwTimeout = 0);
	BOOL DeviceWrite(unsigned int devIndex, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesWritten, DWORD dwTimeout = 0);
    BOOL DeviceRead_Handle(HANDLE read_handle, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesRead, DWORD dwTimeout =0);
	BOOL DeviceWrite_Handle(HANDLE write_handle, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesWritten, DWORD dwTimeout=0);
	

	void DeviceUpdate();
	void FillDeviceList();
	bool OpenUsbDevice(unsigned int devIndex);
	unsigned char  WriteFileData(unsigned int devIndex);
	unsigned char  WriteFileData_Audio(unsigned int devIndex);
	unsigned int ReadFileData(unsigned int devIndex, unsigned char* InputBuf, unsigned int length);
	void UpdateBTAddrUI(unsigned char* tempBTaddr);
	void UpdateDownloadResult(DWORD devIndex, DWORD result);
	BOOL Do_JtagStall(unsigned int devIndex); 
	BOOL Do_JtagUnstall(unsigned int devIndex); 
	BOOL Do_SendCmd(unsigned int devIndex,BYTE cmd); 
	unsigned int  Do_JtagEraseAll(unsigned int devIndex);
	unsigned char Do_JtagRegWrite(unsigned int devIndex, unsigned int addr, unsigned int val);

	void SendRawCmd();

	VOID TimerProc(HWND hwnd,UINT message,UINT iTimerID,DWORD dwTime);
   // unsigned char Do_JtagRegRead(unsigned int devIndex, unsigned int addr,);
    
// Dialog Data
	//{{AFX_DATA(CMp_testDlg)
	enum { IDD = IDD_MP_TEST_DIALOG };
	CProgressCtrl	m_progress_dl[MULTI_USB_DEVICE_MAXNUM];
    CButton	m_checkbox[MULTI_USB_DEVICE_MAXNUM];
	CButton	m_btn_download;
	CString	m_btaddr_current;
	CString	m_num_current;
	CString	m_BinFilePathName;
	int		m_BinFileSize;
    bool    m_SupportUpdateBTAddr;
	int		dl_num;
	HANDLE m_hUSBDevice[MULTI_USB_DEVICE_MAXNUM];
    DWORD  m_DeviceNum;
    volatile DWORD  m_DeviceIndex;
	HANDLE m_hUSBWrite[MULTI_USB_DEVICE_MAXNUM];
	HANDLE m_hUSBRead[MULTI_USB_DEVICE_MAXNUM];
	HANDLE m_hThread[MULTI_USB_DEVICE_MAXNUM];
	int    m_ResultFlag[MULTI_USB_DEVICE_MAXNUM];
	UCHAR  m_AddrBuf[MULTI_USB_DEVICE_MAXNUM][6];
    HANDLE m_hDeviceIndexMutex;
	HICON m_hSuccess;
	HICON m_hFail;
	CString m_comm_name;
	bool b_usb_ok;
	CString	m_btaddrStr[6];
	CString m_btname;
	CString m_CRCResult;
	UCHAR	m_btaddr[6];
	bool	m_comm_open;
	int		m_power_tick;
	int		m_ctest_tick;
    CStatic	m_static_result[MULTI_USB_DEVICE_MAXNUM];

	CButton	m_rawdata_button;
	int		m_rawdata_stage;	

	CEdit m_edit;

    CString m_text1;

    CString m_info_text;
    CString m_id_text;

	CString m_result_text;

	CColorStatic m_far_result;
	CColorStatic m_near_result;

	char	m_mac_str[32];			// mac_string.
	BOOL	m_running;

	int		m_total_count;		// 总共写入数量
	int		m_ok_count;			// 写入成功的数量

	BOOL	m_device_is_open;	// U盾是否打开

	char	m_http_server_name[128];
	BOOL	m_db_support;		// 是否支持database.

	BOOL	m_gpib_power;
	int		m_far_high_value;
	int		m_near_low_value;
	BOOL	m_racecmd_running;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMp_testDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	BOOL	m_test_mode;			// 测试模式
	static const UINT TIMER_ID_TEST_MODE;

	// Generated message map functions
	//{{AFX_MSG(CMp_testDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonDownload();
	afx_msg void OnButtonUSBReset();	
	afx_msg void OnButtonEraseAll();
	afx_msg void OnButtonReadFlash();    
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonAddtset();
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD dwData);
	afx_msg void OnButtonBtNameSet();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSelchangeCombo1();
	afx_msg void OnEnChangeEdit();
	afx_msg LRESULT OnUpdateStatic(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MP_TESTDLG_H__0F971A54_2D78_4FD7_A88C_CDA17796A1F3__INCLUDED_)
