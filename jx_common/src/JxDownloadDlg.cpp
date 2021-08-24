#include "stdafx.h"
#include "JxDownloadDlg.h"
#include <direct.h>
#include "f32x_util.h"

#include <atlbase.h>
#include <atlconv.h>
#include <string>
#include "iphlpapi.h"
#pragma comment(lib,"Iphlpapi.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEFINE_GUID(GUID_INTERFACE_SILABS_BULK, 
			0x37538c66, 0x9584, 0x42d3, 0x96, 0x32, 0xeb, 0xad, 0xa, 0x23, 0xd, 0x13);

#define SILABS_BULK_WRITEPIPE	"PIPE01"
#define SILABS_BULK_READPIPE	"PIPE00"

#define TIMER_EVENT_RESET		1
#define TIMER_EVENT_CHIPTEST	2

int PicWidgetList[MULTI_USB_DEVICE_MAXNUM] = 
{
	IDC_PIC_RESULT_1, IDC_PIC_RESULT_2, IDC_PIC_RESULT_3, IDC_PIC_RESULT_4, IDC_PIC_RESULT_5, IDC_PIC_RESULT_6,
	IDC_PIC_RESULT_7, IDC_PIC_RESULT_8, IDC_PIC_RESULT_9, IDC_PIC_RESULT_10, IDC_PIC_RESULT_11, IDC_PIC_RESULT_12,
	IDC_PIC_RESULT_13, IDC_PIC_RESULT_14, IDC_PIC_RESULT_15, IDC_PIC_RESULT_16, IDC_PIC_RESULT_17, IDC_PIC_RESULT_18,
	IDC_PIC_RESULT_19, IDC_PIC_RESULT_20
};

int DevCheckList[MULTI_USB_DEVICE_MAXNUM] = 
{
	IDC_CHECK1, 
	IDC_CHECK2, 
	IDC_CHECK3,
	IDC_CHECK4, 
	IDC_CHECK5, 
	IDC_CHECK6
};

BT_ADDR_NAME_OFFSET addr_name_offset[20] = 
{
	{0x3e0ca,0x3e09a},
	{0x1fff7,0},
	{0x1fff7,0},
	{0x1fff7,0},
	{0x7ff7,0},
	{0x7ff7,0},
	{0x7ff7,0},
	{0x400e3,0x400b3},  //3231S
	{0X41063,0X41033},  //BK3256flash
	{0X41063,0X41033},	//BK3256
	{0x3fff0,0},        //bk2471
	{0x200c,0},
	{0x400e3,0},	//3431S
//{BK3254_ADDR_OFFSET_IN_BIN_FILE, BK3254_NAME_OFFSET_IN_BIN_FILE},
//{BK3260_ADDR_OFFSET_IN_BIN_FILE, BK3260_NAME_OFFSET_IN_BIN_FILE},
//{BK3262_ADDR_OFFSET_IN_BIN_FILE, BK3262_NAME_OFFSET_IN_BIN_FILE},
};

bool gb_hadreg_flag = FALSE;
bool gb_haddown_flag = FALSE;
bool gb_addraddone_flag = TRUE;
int chip_index = 0;

int ChipMemory[15]=
{
    256*1024,   //3231
    128*1024,   //3431
    128*1024,   //2466
    128*1024,   //5866
    32*1024,   //2533
    32*1024,   //5933
    32*1024,   //2535
    512*1024,   //3231s
    512*1024,  //3256FLASH
    512*1024,  //3256
    256*1024,  //2471
    1024*1024,  //6060
    512*1024,  //3431S
};

UCHAR DevList[MULTI_USB_DEVICE_MAXNUM];
UCHAR DevList_Down[MULTI_USB_DEVICE_MAXNUM];
CString gb_regstr;
unsigned int gb_crc;

unsigned char gb_AddrTab[MULTI_USB_DEVICE_MAXNUM][6];
UINT64 id_save_buf[MULTI_USB_DEVICE_MAXNUM];


/////////////////////////////////////////////////////////////////////////////
// CJxDownloadDlg dialog

CJxDownloadDlg::CJxDownloadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJxDownloadDlg::IDD, pParent)
{

	//{{AFX_DATA_INIT(CJxDownloadDlg)
	//m_btaddr_current = _T("当前地址:\t00:00:00:00:00:00");
	m_num_current = _T("烧录数量：0");
	m_CRCResult= _T("文件检验和: 0");
	m_BinFilePathName = _T("");
	m_btaddrStr[0] = _T("00");
	m_btaddrStr[1] = _T("00");
	m_btaddrStr[2] = _T("00");
	m_btaddrStr[3] = _T("00");
	m_btaddrStr[4] = _T("00");
	m_btaddrStr[5] = _T("00");
	m_btname = _T("");
	m_info_text = _T("");
	m_id_text = _T("");
	memset(m_ResultFlag, 0, 4);
	memset(m_AddrBuf, 0, 24);
	memset(&m_btaddr[0], 0, 6);
	m_BinFileSize = 0;
	dl_num = 0;
	m_comm_open = 0;
	m_ctest_tick = 0;
	m_power_tick = 0;
	m_SupportUpdateBTAddr = 0;
	m_hDeviceIndexMutex = NULL;
	m_DeviceIndex = 0;
	memset(m_hUSBDevice, 0, sizeof(m_hUSBDevice));
	   
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hSuccess = AfxGetApp()->LoadIcon(IDI_ICON2);
	m_hFail = AfxGetApp()->LoadIcon(IDI_ICON3);

	HKEY hkey;
   	LPCTSTR data_Set=_T("Software\\JX\\Set");

    unsigned char chbuf[250];
	//CString strCompany = "";
	//CString strSeries = "";
	//HKEY	hkey;
	DWORD type;
	DWORD len = 250;
	
	memset(chbuf, 0, sizeof(chbuf));
    ::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_READ, &hkey);
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{ 	
	//int  chip_type = ((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
	    ::RegQueryValueEx(hkey, "ADDONE", NULL, &type, chbuf, &len);
  	    if (::RegQueryValueEx(hkey, "ADDONE", NULL, &type, chbuf, &len) == ERROR_SUCCESS)
		{
            CString str=chbuf;
			if(str=="FALSE")
			   gb_addraddone_flag=FALSE;
			else
			   gb_addraddone_flag=TRUE;
		}
        ::RegQueryValueEx(hkey, "CHIP", 0, &type, chbuf, &len);
	    if (::RegQueryValueEx(hkey, "CHIP", 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 chip_index=chbuf[0];
		}
		chip_index = AI6166;
		
		/*if (RegQueryValueEx(hkey, _T("FILENAME"), 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
            m_BinFilePathName=chbuf;
			CFile file_crc;
	        if( NULL == file_crc.Open(m_BinFilePathName, CFile::modeRead | CFile::shareDenyNone))
	        {
	            m_BinFilePathName="";
	        }
		    else
		    {
		        file_crc.Close();
		    }
		}*/
		
        ::RegQueryValueEx(hkey, "ID0", 0, &type, chbuf, &len);
		if (::RegQueryValueEx(hkey, "ID0", 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 m_btaddrStr[0]=chbuf ;
		}
		::RegQueryValueEx(hkey, _T("ID1"), 0, &type, chbuf, &len);
		if (::RegQueryValueEx(hkey, _T("ID1"), 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 m_btaddrStr[1]=chbuf ;
		}
		::RegQueryValueEx(hkey, _T("ID2"), 0, &type, chbuf, &len);
		if (::RegQueryValueEx(hkey, _T("ID2"), 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 m_btaddrStr[2]=chbuf ;
		}
		::RegQueryValueEx(hkey, _T("ID3"), 0, &type, chbuf, &len);
		if (::RegQueryValueEx(hkey, _T("ID3"), 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 m_btaddrStr[3]=chbuf ;
		}
		::RegQueryValueEx(hkey, _T("ID4"), 0, &type, chbuf, &len);
		if (::RegQueryValueEx(hkey, _T("ID4"), 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 m_btaddrStr[4]=chbuf ;
		}
	    ::RegQueryValueEx(hkey, _T("ID5"), 0, &type, chbuf, &len);
		if (::RegQueryValueEx(hkey, _T("ID5"), 0, &type, chbuf, &len) == ERROR_SUCCESS)
		{
			 m_btaddrStr[5]=chbuf ;
		}
	}	  
	else	
	{
	    if (::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_ALL_ACCESS, &hkey)!= ERROR_SUCCESS)
	    {
	        VERIFY(!RegCreateKey(HKEY_CURRENT_USER, data_Set, &hkey));
			VERIFY(!RegSetValueEx(hkey, _T("ID0"), 0, REG_SZ, (BYTE *)m_btaddrStr[0].GetBuffer(m_btaddrStr[0].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID1"), 0, REG_SZ, (BYTE *)m_btaddrStr[1].GetBuffer(m_btaddrStr[1].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID2"), 0, REG_SZ, (BYTE *)m_btaddrStr[2].GetBuffer(m_btaddrStr[2].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID3"), 0, REG_SZ, (BYTE *)m_btaddrStr[3].GetBuffer(m_btaddrStr[3].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID4"), 0, REG_SZ, (BYTE *)m_btaddrStr[4].GetBuffer(m_btaddrStr[4].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID5"), 0, REG_SZ, (BYTE *)m_btaddrStr[5].GetBuffer(m_btaddrStr[5].GetLength()), 2));
		}
	}

    RegCloseKey(hkey);
//	((CStatic *)GetDlgItem(IDC_STATIC_1))->SetWindowText(_T("aa"));
}

void CJxDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJxDownloadDlg)
	DDX_Control(pDX, IDC_BUTTON_DOWNLOAD, m_btn_download);
	DDX_Control(pDX, IDC_PR1, m_progress_dl[0]);
	DDX_Control(pDX, IDC_PR2, m_progress_dl[1]);
	DDX_Control(pDX, IDC_PR3, m_progress_dl[2]);
	DDX_Control(pDX, IDC_PR4, m_progress_dl[3]);
	
	DDX_Control(pDX, IDC_PR5, m_progress_dl[4]);
	DDX_Control(pDX, IDC_PR6, m_progress_dl[5]);
	DDX_Control(pDX, IDC_PR7, m_progress_dl[6]);
	DDX_Control(pDX, IDC_PR8, m_progress_dl[7]);
	DDX_Control(pDX, IDC_PR9, m_progress_dl[8]);
	DDX_Control(pDX, IDC_PR10, m_progress_dl[9]);
	DDX_Control(pDX, IDC_PR11, m_progress_dl[10]);
	DDX_Control(pDX, IDC_PR12, m_progress_dl[11]);
	DDX_Control(pDX, IDC_PR13, m_progress_dl[12]);
	DDX_Control(pDX, IDC_PR14, m_progress_dl[13]);
	DDX_Control(pDX, IDC_PR15, m_progress_dl[14]);
	DDX_Control(pDX, IDC_PR16, m_progress_dl[15]);
	DDX_Control(pDX, IDC_PR17, m_progress_dl[16]);
	DDX_Control(pDX, IDC_PR18, m_progress_dl[17]);
	DDX_Control(pDX, IDC_PR19, m_progress_dl[18]);
	DDX_Control(pDX, IDC_PR20, m_progress_dl[19]);
	
	DDX_Control(pDX, IDC_CHECK1, m_checkbox[0]);
	DDX_Control(pDX, IDC_CHECK2, m_checkbox[1]);
	DDX_Control(pDX, IDC_CHECK3, m_checkbox[2]);
	DDX_Control(pDX, IDC_CHECK4, m_checkbox[3]);
	DDX_Control(pDX, IDC_CHECK5, m_checkbox[4]);
	DDX_Control(pDX, IDC_CHECK6, m_checkbox[5]);
	
	DDX_Control(pDX, IDC_PIC_RESULT_1, m_static_result[0]);
	DDX_Control(pDX, IDC_PIC_RESULT_2, m_static_result[1]);
	DDX_Control(pDX, IDC_PIC_RESULT_3, m_static_result[2]);
	DDX_Control(pDX, IDC_PIC_RESULT_4, m_static_result[3]);
	DDX_Control(pDX, IDC_PIC_RESULT_5, m_static_result[4]);
	DDX_Control(pDX, IDC_PIC_RESULT_6, m_static_result[5]);
	DDX_Control(pDX, IDC_PIC_RESULT_7, m_static_result[6]);
	DDX_Control(pDX, IDC_PIC_RESULT_8, m_static_result[7]);
	DDX_Control(pDX, IDC_PIC_RESULT_9, m_static_result[8]);
	DDX_Control(pDX, IDC_PIC_RESULT_10, m_static_result[9]);
	DDX_Control(pDX, IDC_PIC_RESULT_11, m_static_result[10]);
	DDX_Control(pDX, IDC_PIC_RESULT_12, m_static_result[11]);
	DDX_Control(pDX, IDC_PIC_RESULT_13, m_static_result[12]);
	DDX_Control(pDX, IDC_PIC_RESULT_14, m_static_result[13]);
	DDX_Control(pDX, IDC_PIC_RESULT_15, m_static_result[14]);
	DDX_Control(pDX, IDC_PIC_RESULT_16, m_static_result[15]);
	DDX_Control(pDX, IDC_PIC_RESULT_17, m_static_result[16]);
	DDX_Control(pDX, IDC_PIC_RESULT_18, m_static_result[17]);
	DDX_Control(pDX, IDC_PIC_RESULT_19, m_static_result[18]);
	DDX_Control(pDX, IDC_PIC_RESULT_20, m_static_result[19]);
	
	DDX_Text(pDX, IDC_EDIT1, m_btaddrStr[0]);
	DDV_MaxChars(pDX, m_btaddrStr[0], 2);
	DDX_Text(pDX, IDC_EDIT2, m_btaddrStr[1]);
	DDV_MaxChars(pDX, m_btaddrStr[1], 2);
	DDX_Text(pDX, IDC_EDIT3, m_btaddrStr[2]);
	DDV_MaxChars(pDX, m_btaddrStr[2], 2);
	DDX_Text(pDX, IDC_EDIT4, m_btaddrStr[3]);
	DDV_MaxChars(pDX, m_btaddrStr[3], 2);
	DDX_Text(pDX, IDC_EDIT5, m_btaddrStr[4]);
	DDV_MaxChars(pDX, m_btaddrStr[4], 2);
	DDX_Text(pDX, IDC_EDIT6, m_btaddrStr[5]);
	DDV_MaxChars(pDX, m_btaddrStr[5], 2);
	//DDX_Text(pDX, IDC_STATIC_BTADD, m_btaddr_current);
	DDX_Text(pDX, IDC_STATIC_NUM, m_num_current);
	DDX_Text(pDX, IDC_STATIC_CRC, m_CRCResult);	
	DDX_Text(pDX, IDC_EDIT_BIN, m_BinFilePathName);
	DDX_Text(pDX, IDC_EDIT_BTNAME, m_btname);
	DDV_MaxChars(pDX, m_btname, 32);

    DDX_Text(pDX, IDC_INFO, m_info_text);
    DDX_Text(pDX, IDC_ID, m_id_text);
	DDV_MaxChars(pDX, m_id_text, 16);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJxDownloadDlg, CDialog)
	//{{AFX_MSG_MAP(CJxDownloadDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_DOWNLOAD, OnButtonDownload)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnButtonUSBReset)
	ON_BN_CLICKED(IDC_BUTTON_READFLASH, OnButtonReadFlash)
	ON_BN_CLICKED(IDC_BUTTON_ERASEALL, OnButtonEraseAll)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_ADDTSET, OnButtonAddtset)
	ON_BN_CLICKED(IDC_BUTTON_DEVNAMESET, OnButtonBtNameSet)
	ON_WM_TIMER()
	ON_WM_DEVICECHANGE()
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_TARGET, OnSelchangeCombo1)
    ON_EN_CHANGE(IDC_ID,OnEnChangeEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CJxDownloadDlg::OnEnChangeEdit()
{
    UpdateData(TRUE);
    if (m_id_text.GetLength() == 16)
    {
        CJxDownloadDlg::OnButtonDownload(); 
    }
	else
	{
	    UpdateData(FALSE);
	}
}
	
// CJxDownloadDlg message handlers
void CJxDownloadDlg::OnSelchangeCombo1() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	int chip_type = AI6166;//((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
    if ((chip_type==BK3231)||(chip_type==BK3231S)||(chip_type==BK3431)||(chip_type==BK3256Flash)  \
		||(chip_type==BK3256)||(chip_type==AI6166)||(chip_type==BK3431S))
	{
		GetDlgItem(IDC_EDIT6)->EnableWindow(TRUE);	
	}
	else
	{
		GetDlgItem(IDC_EDIT6)->EnableWindow(FALSE);	
	}

	UpdateData(FALSE);
}

void CJxDownloadDlg::OnTimer(UINT nIDEvent)
{
   int i = 0;
   UpdateData(FALSE);
   KillTimer(1);
}

void CJxDownloadDlg::OnClose()
{
    if (gb_hadreg_flag == FALSE)
    {
	   CWnd::OnClose();
	   return;
    }
	
	UpdateData(TRUE);
	char *p;
	int len,j;
	bool save_flag=TRUE;
	CString str;

	for (j = 0; j < 6; j++)
	{
	   len = m_btaddrStr[j].GetLength();
	   if (len % 2) //首字节添加0, 补足偶数
	   {
	      str = "0" + m_btaddrStr[j];
		  m_btaddrStr[j] = str;
		  len++;
	   }
	   
	   p = m_btaddrStr[j].GetBuffer(len);

	   if (((*p >= 'a') && (*p <= 'f')) || \
		   ((*p >= 'A') && (*p <= 'F')) || \
		   ((*p >= '0') && (*p <= '9')) || \
		   (*p == 8))
	   {
		  
	   }
	   else
	   {
		  save_flag = FALSE;
	   }
	   p++;
	   if (((*p >= 'a') && (*p <= 'f')) || ((*p >= 'A') && (*p <= 'F')) || ((*p >= '0') && (*p <= '9'))||(*p==8))
	   {
		  
	   }
	   else
	   {
		  save_flag = FALSE;
	   }
	}
	
	HKEY hkey;
	LPCTSTR data_Set=_T("Software\\JX\\Set");
   
	if (RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_READ, &hkey)== ERROR_SUCCESS)
	{
		VERIFY(!RegCreateKey(HKEY_CURRENT_USER, data_Set, &hkey));
		if(save_flag==TRUE)
		{
			VERIFY(!RegSetValueEx(hkey, _T("ID0"), 0, REG_SZ, (BYTE *)m_btaddrStr[0].GetBuffer(m_btaddrStr[0].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID1"), 0, REG_SZ, (BYTE *)m_btaddrStr[1].GetBuffer(m_btaddrStr[1].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID2"), 0, REG_SZ, (BYTE *)m_btaddrStr[2].GetBuffer(m_btaddrStr[2].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID3"), 0, REG_SZ, (BYTE *)m_btaddrStr[3].GetBuffer(m_btaddrStr[3].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID4"), 0, REG_SZ, (BYTE *)m_btaddrStr[4].GetBuffer(m_btaddrStr[4].GetLength()), 2));
			VERIFY(!RegSetValueEx(hkey, _T("ID5"), 0, REG_SZ, (BYTE *)m_btaddrStr[5].GetBuffer(m_btaddrStr[5].GetLength()), 2));	
		}
		BOOL check_flag=((CButton *)(GetDlgItem(IDC_CHECK7)))->GetCheck();
		CString str="FALSE";
		if(check_flag)
		   str="TURE";
		VERIFY(!RegSetValueEx(hkey, _T("ADDONE"), 0, REG_SZ, (BYTE *)str.GetBuffer(str.GetLength()),10));

        /*CFile file_crc;
	    if( NULL == file_crc.Open(m_BinFilePathName, CFile::modeRead | CFile::shareDenyNone))
	    {
	    }
		else
		{
		    file_crc.Close();
			VERIFY(!RegSetValueEx(hkey, _T("FILENAME"), 0, REG_SZ, (BYTE *)m_BinFilePathName.GetBuffer(m_BinFilePathName.GetLength()),250));
		}*/
		
		chip_index= AI6166;//((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
        VERIFY(!RegSetValueEx(hkey, _T("CHIP"), 0, REG_DWORD, (BYTE *)&chip_index, 4));
	}	
   
	RegCloseKey(hkey);
/*
	CFile				file_crc;
	int  chip_type = ((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
	if( NULL == file_crc.Open(m_BinFilePathName, CFile::modeReadWrite | CFile::shareDenyNone))
	{
		CWnd::OnClose();
		return;
	}
	if(file_crc.GetLength() > addr_name_offset[chip_type].addr_offset)
	{
		file_crc.Seek( addr_name_offset[chip_type].addr_offset, CFile::begin);
		file_crc.Write(m_btaddr, BT_ADDR_LENGTH_IN_BIN_FILE);	
	}
	file_crc.Close();
*/
	CWnd::OnClose();
	
}


BOOL CJxDownloadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	
	// TODO: Add extra initialization here
	sgCUsbIF.SetGUID(GUID_INTERFACE_SILABS_BULK);
	((CComboBox*) GetDlgItem(IDC_TARGET))->SetCurSel(0);
	DeviceUpdate();	
    ((CButton *)(GetDlgItem(IDC_CHECK7)))->SetCheck(gb_addraddone_flag);
    
	//((CComboBox*) GetDlgItem(IDC_TARGET))->SetCurSel(chip_index);

	((CButton *)(GetDlgItem(IDC_PASSWORD)))->SetCheck(FALSE);

	int  chip_type = AI6166;//((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
    if((chip_type==BK3231)||(chip_type==BK3231S)||(chip_type==BK3431)||  \
		(chip_type==BK3256Flash)||(chip_type==BK3256)||(chip_type==AI6166)||(chip_type==BK3431S))
		GetDlgItem(IDC_EDIT6)->EnableWindow(TRUE);	
	else
		GetDlgItem(IDC_EDIT6)->EnableWindow(FALSE);	
    //SetTimer(0, 5000, NULL); 

//-------------------------------------------------------------
	if(gb_hadreg_flag==FALSE)
	{
		HKEY hkey;
		HKEY hkey1;
		LPCTSTR data_Set=_T("Software\\JX\\Set1");
		LPCTSTR data_Set1=_T("Software\\JX\\Set2");
      
		unsigned char  chbuf[250];
		DWORD type;
		DWORD len=250;
		CString password_str;
		
		memset(chbuf, 0, sizeof(chbuf));

	    ::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_READ|KEY_EXECUTE, &hkey);
		if (::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_READ|KEY_EXECUTE, &hkey) == ERROR_SUCCESS)
		{ 	 
		    CString str,str1;
			CString str2;
		    int  crc_buf[6];
   
			unsigned char crc_sum[6];
			//m_btaddrStr[0]=_T("11");
            long ret0=(::RegQueryValueEx(hkey, "USER", NULL, &type, chbuf, &len));
			ret0=(::RegQueryValueEx(hkey, "USER", NULL, &type, chbuf, &len));
			if (ret0 == ERROR_SUCCESS)
			{
	            str2=chbuf;
				m_BinFilePathName = str2;
				gb_regstr=str2;
				//m_btaddrStr[1]=_T("22");
//-------------------------------------------------------------------
                ::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set1, 0, KEY_READ|KEY_EXECUTE, &hkey1);
				if (::RegOpenKeyEx(HKEY_CURRENT_USER, data_Set1, 0, KEY_READ|KEY_EXECUTE , &hkey1) == ERROR_SUCCESS)
				{
				    m_btaddrStr[2]=_T("33");
					::RegQueryValueEx(hkey1, "USER", NULL, &type, chbuf, &len);
					if (::RegQueryValueEx(hkey1, "USER", NULL, &type, chbuf, &len) == ERROR_SUCCESS)
					{
				        password_str=chbuf;
						//m_btaddrStr[3]=_T("44");
					}	
				}	
				::RegCloseKey(hkey1);	
//-------------------------------------------------------------------
				if((gb_regstr!="")&&(password_str!=""))
				{
				    //m_btaddrStr[4]=_T("55");
				    int i;
					str1=GetMac1();
					str=gb_regstr.Left(12);
                    
					if(str!=str1)
					   goto OnInitDialog_exit;
					
					char szTemp[100];
					memset(szTemp,0,sizeof(szTemp));
					sprintf(szTemp,"%s",gb_regstr);

					for(i=0;i<6;i++)
					{
						str=szTemp[0+(i*4)];
						str+=szTemp[1+(i*4)];
						str+=szTemp[2+(i*4)];
						str+=szTemp[3+(i*4)];
						crc_buf[i]=String2Int(str);
					}
					crc_buf[0]=crc_buf[0]^0x5555;
					crc_buf[1]=crc_buf[1]^0x5555;
					crc_buf[2]=crc_buf[2]^0x5555;

					gb_crc=0x8585;
					unsigned char sum_crc;
					for(i=0;i<6;i++)
					{
					   sum_crc=crc_buf[i]&0x00ff;
					   crc_sum[5-i]=crc_32_calculate(&sum_crc,1,gb_crc)&0x00ff;
					}
                    str1="";
					str.Format("%02X", crc_sum[0]);
					str1+=str;
					str.Format("%02X", crc_sum[1]);
					str1+=str;
					str.Format("%02X", crc_sum[2]);
                    str1+=str;
					str.Format("%02X", crc_sum[3]);
					str1+=str;
					str.Format("%02X", crc_sum[4]);
					str1+=str;
					str.Format("%02X", crc_sum[5]);
					str1+=str;
					//m_btname="1="+password_str+"  2="+str1;
					UpdateData(FALSE);
					//m_btname=password_str+"   "+str1;
					if(password_str==str1)
					{
					   m_BinFilePathName=_T("");
					   gb_hadreg_flag=TRUE;
					   DeviceUpdate();	
					   UpdateData(FALSE);
					   return TRUE;
					}
				}
				else
				{
OnInitDialog_exit:				
			        m_btname=_T("请输入注册码!");

					m_btaddrStr[0]=_T("");
					m_btaddrStr[1]=_T("");
					m_btaddrStr[2]=_T("");
					m_btaddrStr[3]=_T("");
					m_btaddrStr[4]=_T("");
					m_btaddrStr[5]=_T("");
					UpdateData(FALSE);
				}
			}
		}	  
		else
		{
		    int i;
			CString str,str1;
			str1 = GetMac1();

			srand((unsigned)time(NULL)); 
		    i = rand();
			
	        str.Format("%04X", i);
			str1 += str;
			i = rand();
			str.Format("%04X", i);
			str1 += str;
			i = rand();
			str.Format("%04X", i);
			str1 += str;
		    m_BinFilePathName = str1;
	        m_btname = _T("请输入注册码!");

			m_btaddrStr[0] = _T("");
			m_btaddrStr[1] = _T("");
			m_btaddrStr[2] = _T("");
			m_btaddrStr[3] = _T("");
			m_btaddrStr[4] = _T("");
			m_btaddrStr[5] = _T("");
			UpdateData(FALSE);
			gb_regstr = str1;
		    if (RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_ALL_ACCESS, &hkey)!= ERROR_SUCCESS)
		    {
		        VERIFY(!RegCreateKey(HKEY_CURRENT_USER, data_Set, &hkey));
				//VERIFY(!RegSetValueEx(hkey, _T("USER"), 0, REG_SZ, (BYTE *)m_btaddrStr[0].GetBuffer(m_btaddrStr[0].GetLength()), 2));
                VERIFY(!RegSetValueEx(hkey, _T("USER"), 0, REG_SZ, (BYTE *)str1.GetBuffer(str1.GetLength()),250));
			}
		}
        (GetDlgItem(IDC_BUTTON_LOAD))->SetWindowText("注册");
	    ::RegCloseKey(hkey);
		UpdateData(FALSE);
	}
//-------------------------------------------------------------
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CJxDownloadDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CJxDownloadDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
        
		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CJxDownloadDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


BOOL CJxDownloadDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
    if (gb_hadreg_flag == FALSE)
	{
		return FALSE;
	}
	
    if (gb_haddown_flag)
    {
        gb_haddown_flag=false;

        MessageBox("USB出现异常将重启", "错误", MB_OK);
	    PROCESS_INFORMATION info; 
        STARTUPINFO startup; 
        char szPath[128]; 
        char *szCmdLine; 
    
        GetModuleFileName(AfxGetApp()->m_hInstance, szPath, sizeof(szPath));
        szCmdLine = GetCommandLine(); 
        GetStartupInfo(&startup);     
        BOOL bSucc = CreateProcess(szPath, szCmdLine, NULL, NULL, 
										FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
										&startup, &info);     
	    if (1) 
	    { 
	        CWnd *pWnd = AfxGetMainWnd();
	        if (pWnd!=NULL) 
	        { 
	            pWnd->PostMessage(WM_CLOSE,0,0); 
	        } 
	        else 
	        {
	            ExitProcess(-1); 
	        }
	    } 
	    else 
	    {
	        ExitProcess(-1); 
	    }
    }    
  	DeviceUpdate();	
	return TRUE;
}

void CJxDownloadDlg::DeviceUpdate()
{
	int WidgetList[]=
	{
		IDC_CHECK1,
		IDC_CHECK2,
		IDC_CHECK3,
		IDC_CHECK4,
		IDC_CHECK5,
		IDC_CHECK6,
	};
	
	int WidgetList1[]= 
	{
		IDC_PR1,
		IDC_PR2,
		IDC_PR3,
		IDC_PR4, 
		IDC_PR5,
		IDC_PR6,
		IDC_PR7,
		IDC_PR8,
		IDC_PR9,
		IDC_PR10,
		IDC_PR11,
		IDC_PR12,
		IDC_PR13,
		IDC_PR14,
		IDC_PR15,
		IDC_PR16,
		IDC_PR17,
		IDC_PR18,
		IDC_PR19,
		IDC_PR20
	};

	F32x_STATUS status;
	int i;//, WidegtArrayLen, WidgetNumEachDev;
	DWORD mb_Device = m_DeviceNum;

	
	status = F32x_GetNumDevices(&m_DeviceNum); 
	if (status != F32x_SUCCESS)
	{
		m_DeviceNum = 0;
	}
	
	//WidegtArrayLen = sizeof(WidgetList)/sizeof(WidgetList[0]);
	//WidgetNumEachDev = WidegtArrayLen/MULTI_USB_DEVICE_MAXNUM;
    
	if(m_BinFileSize && m_DeviceNum)
	{
		GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(false);
	}
	    //GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(true);
	/*if(m_DeviceNum)
	{
		GetDlgItem(IDC_BUTTON_READFLASH)->EnableWindow(true);
		GetDlgItem(IDC_BUTTON_ERASEALL)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_BUTTON_READFLASH)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON_ERASEALL)->EnableWindow(false);
	}
	
	for(i=0; i< m_DeviceNum*WidgetNumEachDev; i++)
	{
		if(i&0x01)
		{
		   GetDlgItem(WidgetList[i])->ShowWindow(SW_SHOW);
           GetDlgItem(WidgetList[i])->SendMessage(PBM_SETBKCOLOR, 0,  0xc0c0c0);
		}   
	}*/
	memset(DevList, 0, MULTI_USB_DEVICE_MAXNUM);
	FillDeviceList();

	for (i = 0; i < MULTI_USB_DEVICE_MAXNUM; i++)
	{
	    if (DevList[i])
	    {
		    GetDlgItem(WidgetList1[i])->ShowWindow(SW_SHOW);
            GetDlgItem(WidgetList1[i])->SendMessage(PBM_SETBKCOLOR, 0, 0xc0c0c0);  
	    }
		else
		{
            GetDlgItem(WidgetList1[i])->ShowWindow(SW_HIDE);
		}
	}

	for (i = 0; i < 6; i++)
	{
		GetDlgItem(WidgetList[i])->ShowWindow(SW_HIDE);
	}
	
	//for(i=m_DeviceNum; i< 20; i++)
	//{
	//	GetDlgItem(WidgetList1[i])->ShowWindow(SW_HIDE);
	//}

	
	//m_progress_dl[0].SendMessage(PBM_SETBKCOLOR, 0,  0xc0c0c0);
	//m_progress_dl[1].SendMessage(PBM_SETBKCOLOR, 0,  0xfffaf0);

	for (i = 0; i < MULTI_USB_DEVICE_MAXNUM; i++)  // clear all result-related icons
	{
		GetDlgItem(PicWidgetList[i])->ShowWindow(SW_HIDE);
	}

	for (i = 0; i < MULTI_USB_DEVICE_MAXNUM; i++)
	{
		m_progress_dl[i].SetPos(0);
	}
}

void CJxDownloadDlg::FillDeviceList()
{
	F32x_DEVICE_STRING	devStr;
	int i;
    
	if (gb_hadreg_flag == FALSE)
	{
	   return;
	}

	for (i = 0; i < MULTI_USB_DEVICE_MAXNUM; i++)
	{
		m_hUSBWrite[i]=NULL;
		m_hUSBRead[i]=NULL;
	}

	for (i = 0; i < m_DeviceNum; i++)
	{
		if (F32x_SUCCESS == F32x_GetProductString(i, devStr, F32x_RETURN_SERIAL_NUMBER))
		{
			OpenUsbDevice(i);
		}
	}
}

bool CJxDownloadDlg::OpenUsbDevice(unsigned int devIndex)
{
	bool rt=true;
	HANDLE usb_write;
	HANDLE usb_read;
	
	F32x_STATUS status = F32x_Open(devIndex, &m_hUSBDevice[devIndex]);
	
	// Open selected file.
	if (status == F32x_SUCCESS)
	{
		// Write file to device in MAX_PACKET_SIZE-byte chunks.
		// Get the write handle
		usb_write = sgCUsbIF.OpenUSBfile(SILABS_BULK_WRITEPIPE);
		
		if (usb_write == INVALID_HANDLE_VALUE)
		{
			CString sMessage;

			rt=false;
			//sMessage.Format("Error opening Write device: %s\n\nApplication is aborting.\nReset hardware and try again.",SILABS_BULK_WRITEPIPE);
			//AfxMessageBox(sMessage,MB_OK|MB_ICONEXCLAMATION);
			m_num_current.Format("USB打开失败\r\n");
			UpdateData(FALSE);
		}
		
		// Get the read handle
		usb_read = sgCUsbIF.OpenUSBfile(SILABS_BULK_READPIPE);
		
		if (usb_read == INVALID_HANDLE_VALUE)
		{
			CString sMessage;
			rt = false;
			
			//sMessage.Format("Error opening Read device: %s\n\nApplication is aborting.\nReset hardware and try again.",SILABS_BULK_READPIPE);
			//AfxMessageBox(sMessage,MB_OK|MB_ICONEXCLAMATION);
			m_num_current.Format("USB打开失败\r\n");
			UpdateData(FALSE);
		}
	}
	else
	{
		rt = false;
	}
	
	b_usb_ok = rt;
	if (rt == false)
	{
	   return rt;
	}
	
	BYTE buf[32];
	DWORD dwBytesWritten,dwBytesRead;
	BOOL success;
	int retry_times;

    if (1)
	{
	    buf[0] = USB_CHECK_CMD;
	    buf[1] = 0;
		if (TRUE == DeviceWrite_Handle(usb_write, buf, 2, &dwBytesWritten))
		{
			memset(buf, 0, 2);
			retry_times=60;
			Sleep(5);
			success = DeviceRead_Handle(usb_read, buf, 2, &dwBytesRead);

			// Check for ACK packet after writing 8 pkts.
			while (!success)
			{
				Sleep(50);
				success = DeviceRead_Handle(usb_read, buf, 2, &dwBytesRead);
				Sleep(1);
				if ((retry_times--) <= 0)
				{
					break;
				}
			}
			
			if (success)
			{
                DevList[buf[0]] = 1;
				m_hUSBWrite[buf[0]] = usb_write;
				m_hUSBRead[buf[0]] = usb_read;
			}
		}
    }

	return rt;
}


BOOL CJxDownloadDlg::DeviceRead(unsigned int devIndex, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesRead, DWORD dwTimeout)
{
	F32x_STATUS	status			= F32x_SUCCESS;
	
	status = F32x_Read(m_hUSBRead[devIndex], buffer, dwSize, lpdwBytesRead);
	
	return (status == F32x_SUCCESS);
}

BOOL CJxDownloadDlg::DeviceWrite(unsigned int devIndex, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesWritten, DWORD dwTimeout)
{
	F32x_STATUS	status	= F32x_SUCCESS;
	
	status = F32x_Write(m_hUSBWrite[devIndex], buffer, dwSize, lpdwBytesWritten);
	
	return (status == F32x_SUCCESS);
}

BOOL CJxDownloadDlg::DeviceRead_Handle(HANDLE read_handle, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesRead, DWORD dwTimeout)
{
	F32x_STATUS	status			= F32x_SUCCESS;
	
	status = F32x_Read(read_handle, buffer, dwSize, lpdwBytesRead);
	
	return (status == F32x_SUCCESS);
}

BOOL CJxDownloadDlg::DeviceWrite_Handle(HANDLE write_handle, BYTE* buffer, DWORD dwSize, DWORD* lpdwBytesWritten, DWORD dwTimeout)
{
	F32x_STATUS	status	= F32x_SUCCESS;
	
	status = F32x_Write(write_handle, buffer, dwSize, lpdwBytesWritten);
	
	return (status == F32x_SUCCESS);
}

DWORD WINAPI ReadFlashContentProc(LPVOID lParam)
{
	CJxDownloadDlg * pDlg = (CJxDownloadDlg*)lParam;
	unsigned char* data_fromTarget= NULL;
	unsigned int devIndex, ret_status = BK_RET_SUCCESS;
	CWnd* cwnd;
	
	WaitForSingleObject( pDlg->m_hDeviceIndexMutex, INFINITE );
	devIndex = pDlg->m_DeviceIndex++;
	ReleaseMutex( pDlg->m_hDeviceIndexMutex );

	if (((CButton *)(pDlg->GetDlgItem(DevCheckList[devIndex])))->GetCheck() == 0) 
		goto EXIT_DISABLE_DEV;
	
	if (pDlg->m_hUSBDevice[devIndex] == INVALID_HANDLE_VALUE)
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
	}
	
	// Write file to device in MAX_PACKET_SIZE-byte chunks.			
	if (pDlg->Do_JtagStall(devIndex) == FALSE)
	{
		ret_status = BK_RET_JTAG_STALL_FAILED;
	}
	
	//pDlg->Do_JtagEraseAll(devIndex);
	if ((data_fromTarget = (unsigned char*)malloc(BK_CHECK_FILE_SIZE)) != NULL)
	{
		ret_status = pDlg->ReadFileData(devIndex, data_fromTarget, BK_CHECK_FILE_SIZE);
		pDlg->Do_JtagUnstall(devIndex);	
	}
	else
	{
		ret_status = BK_RET_MALLOC_FAILED;
	}

	if (ret_status == BK_RET_SUCCESS)
	{
		pDlg->m_static_result[devIndex].SetIcon( pDlg->m_hSuccess);
	}
	else
	{
		pDlg->m_static_result[devIndex].SetIcon( pDlg->m_hFail);
	}
	pDlg->GetDlgItem(PicWidgetList[devIndex])->ShowWindow(SW_SHOW);
	
	if (data_fromTarget != NULL)
	{
		free(data_fromTarget);
	}
	
EXIT_DISABLE_DEV:
	
	WaitForSingleObject( pDlg->m_hDeviceIndexMutex, INFINITE );
	devIndex = pDlg->m_DeviceIndex--;
	ReleaseMutex( pDlg->m_hDeviceIndexMutex );
	
	if (pDlg->m_DeviceIndex == 0)
	{
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_READFLASH);
		cwnd->EnableWindow(true);
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_ERASEALL);
		cwnd->EnableWindow(true);
		
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_DOWNLOAD);
		if (pDlg->m_BinFileSize && pDlg->m_DeviceNum)
		{
			cwnd->EnableWindow(true);		
		}
	}
	
	return 0;
}

DWORD WINAPI EraseAllProc(LPVOID lParam)
{
	CJxDownloadDlg * pDlg = (CJxDownloadDlg*)lParam;
	unsigned int devIndex, ret_status = BK_RET_SUCCESS;
	CWnd* cwnd;
	
	WaitForSingleObject(pDlg->m_hDeviceIndexMutex, INFINITE );
	devIndex = pDlg->m_DeviceIndex++;
	ReleaseMutex(pDlg->m_hDeviceIndexMutex);

	if (((CButton *)(pDlg->GetDlgItem(DevCheckList[devIndex])))->GetCheck() == 0) 
	{
		goto EXIT_DISABLE_DEV;
	}

	if (pDlg->m_hUSBDevice[devIndex] == INVALID_HANDLE_VALUE)
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
	}
	
	// Write file to device in MAX_PACKET_SIZE-byte chunks.			
	if (pDlg->Do_JtagStall(devIndex) == FALSE)
	{
		ret_status = BK_RET_JTAG_STALL_FAILED;
	}
	
	pDlg->Do_JtagEraseAll(devIndex);
	pDlg->Do_JtagUnstall(devIndex);	
EXIT_DISABLE_DEV:	
	WaitForSingleObject( pDlg->m_hDeviceIndexMutex, INFINITE );
	devIndex = pDlg->m_DeviceIndex--;
	ReleaseMutex( pDlg->m_hDeviceIndexMutex );
	
	if (pDlg->m_DeviceIndex == 0)
	{
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_READFLASH);
		cwnd->EnableWindow(true);
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_ERASEALL);
		cwnd->EnableWindow(true);
		
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_DOWNLOAD);
		if (pDlg->m_BinFileSize && pDlg->m_DeviceNum)
		{
			cwnd->EnableWindow(true);		
		}
	}
	
	return ret_status;
}

DWORD WINAPI DownloadProc(LPVOID lParam)
{
	CJxDownloadDlg * pDlg = (CJxDownloadDlg*)lParam;
	int devIndex;
	int devIndex_a;
	CWnd* cwnd;
	F32x_STATUS status = BK_RET_SUCCESS;
		
	WaitForSingleObject(pDlg->m_hDeviceIndexMutex, INFINITE );
	devIndex_a = pDlg->m_DeviceIndex;
	devIndex = DevList_Down[pDlg->m_DeviceIndex++]-1;
	ReleaseMutex( pDlg->m_hDeviceIndexMutex );

	//if(((CButton *)(pDlg->GetDlgItem(DevCheckList[devIndex])))->GetCheck() == 0) 
	//if(DevList[devIndex]==0)
	//	goto EXIT_DISABLE_DEV;

	if(pDlg->m_hUSBDevice[devIndex] == INVALID_HANDLE_VALUE)
	{
		status = BK_RET_USB_WRITE_FAILED;
		goto EXIT_DOWNLOADPROC1;
	}

    if (chip_index==BK3256)
    {
		if (pDlg->Do_JtagStall(devIndex) == FALSE)
		{
			status = BK_RET_JTAG_STALL_FAILED;
			goto EXIT_DOWNLOADPROC1;
		}
		
		if ((status = pDlg->Do_JtagEraseAll(devIndex))!= BK_RET_SUCCESS)
		{
			pDlg->Do_JtagUnstall(devIndex);
			goto EXIT_DOWNLOADPROC1;
		}

		status = pDlg->WriteFileData_Audio(devIndex);
    }
	else
    {
		// Write file to device in MAX_PACKET_SIZE-byte chunks.			
		if (pDlg->Do_SendCmd(devIndex,WRITE_RESET_CONTROL_CMD) == FALSE)
		{
			status = BK_RET_JTAG_STALL_FAILED;
			goto EXIT_DOWNLOADPROC1;
		}
		
		if (pDlg->Do_SendCmd(devIndex,FLASH_WRITE_ENABLE) == FALSE)
		{
			status = BK_RET_JTAG_STALL_FAILED;
			goto EXIT_DOWNLOADPROC1;
		}

		if (pDlg->Do_SendCmd(devIndex,FLASH_ERASE_ALL_CMD) == FALSE)
		{
			status = BK_RET_JTAG_STALL_FAILED;
			goto EXIT_DOWNLOADPROC1;
		}
		status = pDlg->WriteFileData(devIndex);
		if (pDlg->Do_SendCmd(devIndex,FLASH_WRITE_DISABLE) == FALSE)
		{
	        status = BK_RET_JTAG_STALL_FAILED;
			goto EXIT_DOWNLOADPROC1;
	    }
	}
	
	
	
	//pDlg->Do_JtagUnstall(devIndex);
EXIT_DOWNLOADPROC1:
	if (status == BK_RET_SUCCESS)
	{
		pDlg->m_static_result[devIndex].SetIcon(pDlg->m_hSuccess);
		DevList_Down[devIndex_a]=0;
	}
	else
	{
		pDlg->m_static_result[devIndex].SetIcon(pDlg->m_hFail);
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_RESET);
		cwnd->EnableWindow(true);
	}

	pDlg->GetDlgItem(PicWidgetList[devIndex])->ShowWindow(SW_SHOW);

EXIT_DISABLE_DEV:
	WaitForSingleObject( pDlg->m_hDeviceIndexMutex, INFINITE );
	devIndex = pDlg->m_DeviceIndex--;
	ReleaseMutex( pDlg->m_hDeviceIndexMutex );

	if(pDlg->m_DeviceIndex == 0)
	{
	//	Sleep(10);
		pDlg->UpdateBTAddrUI(pDlg->m_btaddr);
	    pDlg->SetTimer(1,100,0);
		//pDlg->GetDlgItem(IDC_STATIC_BTADD)->SetWindowText(pDlg->m_btaddr_current);
		pDlg->m_num_current.Format("烧录数量：%d", pDlg->dl_num);
		pDlg->GetDlgItem(IDC_STATIC_NUM)->SetWindowText(pDlg->m_num_current);
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_READFLASH);
		cwnd->EnableWindow(true);
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_ERASEALL);
		cwnd->EnableWindow(true);
		cwnd = pDlg->GetDlgItem(IDC_BUTTON_DOWNLOAD);
		cwnd->EnableWindow(true);	
		cwnd = pDlg->GetDlgItem(IDC_CHECK7);
		cwnd->EnableWindow(true);	
        cwnd = pDlg->GetDlgItem(IDC_TARGET);
		cwnd->EnableWindow(true);
        cwnd = pDlg->GetDlgItem(IDC_BUTTON_LOAD);
		cwnd->EnableWindow(true);
		cwnd = pDlg->GetDlgItem(IDC_PASSWORD);
		cwnd->EnableWindow(true);
		gb_haddown_flag = FALSE;
		//pDlg->m_id_text = _T("");
	}
	
	return 0;
}

#define JTAG_READ_FRAME_LENGTH 2048
char* BEKENDIR = "D:\\beken";
unsigned int CJxDownloadDlg::ReadFileData( unsigned int devIndex, unsigned char* InputBuf, unsigned int length)
{
	BOOL ret_status = BK_RET_SUCCESS;
	DWORD dwBytesRead = 0, dwIndex = 0;
	CFile file;
	CString m_binname;
	unsigned int total_len = length;
	m_binname.Format("D:\\beken\\flashdev%d.bin", devIndex);
	_mkdir( BEKENDIR );
	if (!file.Open(m_binname, CFile::modeReadWrite|CFile::modeCreate))
	{
		ret_status = BK_RET_OPEN_CHECK_FILE_FAILED;
		return ret_status;		
	}
	
	m_progress_dl[devIndex].SetRange32(0,total_len);
	m_progress_dl[devIndex].SetPos(0);
	
	InputBuf[0] = JTAG_FLASH_READ_START;
    InputBuf[1] = (char)(length & 0x000000FF);
    InputBuf[2] = (char)((length & 0x0000FF00) >> 8);
    InputBuf[3] = (char)((length & 0x00FF0000) >> 16);
	DeviceWrite(devIndex, &InputBuf[0], 4, &dwBytesRead);
	while(length)
	{
		if(length > JTAG_READ_FRAME_LENGTH)
		{
			m_progress_dl[devIndex].SetPos(dwIndex);
			DeviceRead(devIndex, &InputBuf[dwIndex], JTAG_READ_FRAME_LENGTH, &dwBytesRead);
			
		}
		else 
		{
			m_progress_dl[devIndex].SetPos(dwIndex);
			DeviceRead(devIndex, &InputBuf[dwIndex], length, &dwBytesRead);
		}
		dwIndex += dwBytesRead;
		length -= dwBytesRead;

	}
	file.Write(&InputBuf[0], total_len);
	file.Close();
	//m_progress_dl[devIndex].SetPos(0);
	return 	ret_status;
}

unsigned char CJxDownloadDlg::Do_JtagRegWrite(unsigned int devIndex, unsigned int addr, unsigned int val)
{
	BYTE	buf[9];
    DWORD		dwBytesWritten	= 0;
    DWORD		dwBytesRead = 0;
	
    buf[0] = JTAG_MID_WRITE_CMD;
    buf[1] = (char)(addr & 0x000000FF);
    buf[2] = (char)((addr & 0x0000FF00) >> 8);
    buf[3] = (char)((addr & 0x00FF0000) >> 16);
	buf[4] = (char)((addr & 0xFF000000) >> 24);
    buf[5] = (char)(val & 0x000000FF);
    buf[6] = (char)((val & 0x0000FF00) >> 8);
    buf[7] = (char)((val & 0x00FF0000) >> 16);
	buf[8] = (char)((val & 0xFF000000) >> 24);
	if((!(DeviceWrite(devIndex, buf, 9, &dwBytesWritten))) || (dwBytesWritten != 9))
	{
		return BK_RET_USB_WRITE_FAILED;
	}

	DeviceRead(devIndex, &buf[0], 2, &dwBytesRead);
	return BK_RET_SUCCESS;
}


#define BT_ADDR_OFFSET_IN_BIN_FILE			0x41063
#define BT_ADDR_FRAME_OFFSET_IN_BIN_FILE	0x41060
#define BT_NAME_OFFSET_IN_BIN_FILE			0x41033


unsigned char CJxDownloadDlg::WriteFileData_Audio( unsigned int devIndex)
{
    unsigned char ret_status = BK_RET_SUCCESS;
    int dev_index = 0;
    int rt=0;
    CFile file;
    DWORD		dwBytesWritten	= 0;
    DWORD		dwBytesRead = 0;
    //BYTE		buf[JTAG_FRAME_LENGTH] = {0};
    DWORD numLoops = 0;
    DWORD totalWritten = 0;
	DWORD dwWriteLength	= TRANSFER_DATA_BYTE_EACH_TIME;
	DWORD frameLen = 0;
	int times = 0;
	int i=0;
	DWORD btaddr_offset = 0x41063;
	unsigned char* buf;
    unsigned char* data_buffer;
    unsigned char* data_fromTarget;		
	UINT64* pBT64addr;
	
	WaitForSingleObject( m_hDeviceIndexMutex, INFINITE );
	if(!file.Open(m_BinFilePathName, CFile::modeReadWrite | CFile::shareDenyNone))
	{
	    ReleaseMutex( m_hDeviceIndexMutex);	
		return BK_RET_OPEN_BIN_FILE_FAILED;		
	}

	buf = (unsigned char*) malloc(JTAG_FRAME_LENGTH);
	data_buffer = (unsigned char*) malloc(m_BinFileSize);
	data_fromTarget = (unsigned char*) malloc(m_BinFileSize);
    if((data_buffer == NULL)||(data_fromTarget == NULL))
    {
        ret_status = BK_RET_MALLOC_FAILED;
		ReleaseMutex( m_hDeviceIndexMutex);	
        goto Exit_writeFiledata_Audio;
    }
    memset(data_buffer, 0, m_BinFileSize);
    memset(data_fromTarget, 0, m_BinFileSize);	
    file.Read(data_buffer, m_BinFileSize);

	if(((CButton *)(GetDlgItem(IDC_CHECK7)))->GetCheck() ==1) 
	{
		//WaitForSingleObject( m_hDeviceIndexMutex, INFINITE );
		//memcpy(m_AddrBuf[devIndex], m_btaddr, 6);
		memcpy( &data_buffer[BT_ADDR_OFFSET_IN_BIN_FILE], m_btaddr, 6);
		pBT64addr= (UINT64*)m_btaddr;
		(*pBT64addr) += 1;		
		
		//memset( &data_buffer[addr_name_offset[chip_type].name_offset], 0, BT_NAME_MAX_LENGTH_IN_BIN_FILE );
		//memcpy( &data_buffer[addr_name_offset[chip_type].name_offset], m_btname.GetBuffer(m_btname.GetLength()), m_btname.GetLength()); 	
    }
    ReleaseMutex( m_hDeviceIndexMutex);			
    m_progress_dl[devIndex].SetRange32(0,m_BinFileSize);

    buf[0] = JTAG_FLASH_WRITE_START;
    buf[1] = (char)(m_BinFileSize & 0x000000FF);
    buf[2] = (char)((m_BinFileSize & 0x0000FF00) >> 8);
    buf[3] = (char)((m_BinFileSize & 0x00FF0000) >> 16);
	memcpy(&buf[4], data_buffer, 60);
	totalWritten = 60;
	if((!(DeviceWrite(devIndex, buf, 64, &dwBytesWritten))) || (dwBytesWritten != 64))
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
		goto Modify_BTADDR_FLAG_Audio;
	}
		
    memset(buf, 0, JTAG_FRAME_LENGTH);
    times = (JTAG_FRAME_LENGTH/64);

	int j;
	
	for(i=0; totalWritten < m_BinFileSize; i++ )
	{
		if((m_BinFileSize-totalWritten) <= TRANSFER_DATA_BYTE_EACH_TIME)  //LAST FRAME
		{
			dwWriteLength = (m_BinFileSize- totalWritten);			
			buf[(i%times)*64]= JTAG_FLASH_WRITE_END;
			memcpy(&(buf[(i%times)*64+1]), &data_buffer[totalWritten], dwWriteLength);
			totalWritten = m_BinFileSize;
		}
		else
		{			
			buf[(i%times)*64]= JTAG_FLASH_WRITE_DATA;
			dwWriteLength = TRANSFER_DATA_BYTE_EACH_TIME;
			memcpy(&(buf[(i%times)*64+1]), &data_buffer[totalWritten], dwWriteLength);
			totalWritten += TRANSFER_DATA_BYTE_EACH_TIME;
		}
        j=0;
		frameLen += 64;
		if((frameLen == JTAG_FRAME_LENGTH) || (totalWritten == m_BinFileSize))
		{
		    j=frameLen;
			int len=0;
			int k=0;
            while(j)
            {
				if(j>=256)
			    {
	                ret_status = DeviceWrite(devIndex, &buf[k], 256, &dwBytesWritten);
					j=j-256;
					k+=256;
			    }
				else
				{
					ret_status = DeviceWrite(devIndex, &buf[k], j, &dwBytesWritten);
                    j=0;
				}
            }
			//ret_status = DeviceWrite(devIndex, buf, frameLen, &dwBytesWritten);
			m_progress_dl[devIndex].SetPos(totalWritten);
			frameLen = 0;
			j=0;
			while(j++<2000);
		}
	}


	//JtagMidWrite(0x80101c, 0x02, 0x08);     //disable WP
	ret_status = Do_JtagRegWrite(devIndex, 0x80101c, 0x08);
	if(BK_RET_SUCCESS != ret_status)
	{
		goto Modify_BTADDR_FLAG_Audio;
	}


	//JtagMidWrite(0x801010, 0x02, 0x01);     //disable WP
	ret_status = Do_JtagRegWrite(devIndex, 0x801010, 0x01);
	if(BK_RET_SUCCESS != ret_status)
	{
		goto Modify_BTADDR_FLAG_Audio;
	}

		
	//JtagMidWrite(0x801018, 0x02, 0x02);     // support 1-byte SR
	ret_status = Do_JtagRegWrite(devIndex, 0x801018, 0x02);
	if(BK_RET_SUCCESS != ret_status)
	{
		goto Modify_BTADDR_FLAG_Audio;
	}
		
	//JtagMidWrite(0x801014, 0x02, 0x3c);     //status register = 0;
	ret_status = Do_JtagRegWrite(devIndex, 0x801014, 0x3c);
	if(BK_RET_SUCCESS != ret_status)
	{
		goto Modify_BTADDR_FLAG_Audio;
	}
	
	//JtagMidWrite(0x801004, 0x02, 0x04);     //cmd type = WRSR
	ret_status = Do_JtagRegWrite(devIndex, 0x801004, 0x04);
	if(BK_RET_SUCCESS != ret_status)
	{
		goto Modify_BTADDR_FLAG_Audio;
	}

	//JtagMidWrite(0x801008, 0x02, 0x01);     // start = 1
	ret_status = Do_JtagRegWrite(devIndex, 0x801008, 0x01);
	if(BK_RET_SUCCESS != ret_status)
	{
		goto Modify_BTADDR_FLAG_Audio;
	}

	
//JtagMidRead(0x20, 0x02)
    buf[0] = JTAG_MID_WRITE_CMD;
    buf[1] = (char)(0x20 & 0x000000FF);
    buf[2] = (char)((0x20 & 0x0000FF00) >> 8);
    buf[3] = (char)((0x20 & 0x00FF0000) >> 16);
	buf[4] = (char)((0x20 & 0xFF000000) >> 24);
	if((!(DeviceWrite(devIndex, buf, 5, &dwBytesWritten))) || (dwBytesWritten != 5))
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
		goto Modify_BTADDR_FLAG_Audio;
	}
	DeviceRead(devIndex, &buf[0], 5, &dwBytesRead);

//JtagMidRead(0x200, 0x02)
    buf[0] = JTAG_MID_WRITE_CMD;
    buf[1] = (char)(0x200 & 0x000000FF);
    buf[2] = (char)((0x200 & 0x0000FF00) >> 8);
    buf[3] = (char)((0x200 & 0x00FF0000) >> 16);
	buf[4] = (char)((0x200 & 0xFF000000) >> 24);
	if((!(DeviceWrite(devIndex, buf, 5, &dwBytesWritten))) || (dwBytesWritten != 5))
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
		goto Modify_BTADDR_FLAG_Audio;
	}
	DeviceRead(devIndex, &buf[0], 5, &dwBytesRead);

	Sleep(75);

	if((ret_status = ReadFileData(devIndex, data_fromTarget, m_BinFileSize)) != BK_RET_SUCCESS)
		goto Modify_BTADDR_FLAG_Audio;
    
	
	for(i=0; i<m_BinFileSize; i++)
	{
		if(data_fromTarget[i]!= data_buffer[i])
		{			
			ret_status = BK_RET_CONTENT_DIFF_FAILED;
			goto Modify_BTADDR_FLAG_Audio;			
		}
	}
	
	dl_num++;

Modify_BTADDR_FLAG_Audio:
	if(ret_status == BK_RET_SUCCESS)
	{
		m_ResultFlag[devIndex]= 0;
	}
	else 
	{
		m_ResultFlag[devIndex]= 1;
	}
	
Exit_writeFiledata_Audio:
    if (m_BinFilePathName.GetLength() > 0)
    	file.Close();	
    //m_progress_dl[devIndex].SetPos(0);
    free(buf);
	free(data_buffer);
	free(data_fromTarget);

    return ret_status;
}


unsigned char CJxDownloadDlg::WriteFileData( unsigned int devIndex)
{
    unsigned char ret_status = BK_RET_SUCCESS;
    int dev_index = 0;
    int rt=0;
    CFile file;
    DWORD		dwBytesWritten	= 0;
    DWORD		dwBytesRead = 0;
    BYTE		buf[JTAG_FRAME_LENGTH] = {0};
    DWORD numLoops = 0;
    DWORD totalWritten = 0;
	DWORD dwWriteLength	= TRANSFER_DATA_BYTE_EACH_TIME;
	DWORD frameLen = 0;
	int times = 0;
	int i=0,chip_size;
    unsigned char* data_buffer;
    //unsigned char* data_fromTarget;		
	UINT64* pBT64addr;
	BYTE retry_times;
	BOOL success = TRUE;
	
	int  chip_type = AI6166;//((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();

	WaitForSingleObject( m_hDeviceIndexMutex, INFINITE );
	
	if(!file.Open(m_BinFilePathName, CFile::modeReadWrite | CFile::shareDenyNone))
	{
	    ReleaseMutex( m_hDeviceIndexMutex);		
		return BK_RET_OPEN_BIN_FILE_FAILED;		
	}

	data_buffer = (unsigned char*) malloc(ChipMemory[chip_type]);
	chip_size=ChipMemory[chip_type];
	
	//data_fromTarget = (unsigned char*) malloc(m_BinFileSize);
    if((data_buffer == NULL))//||(data_fromTarget == NULL))
    {
        ret_status = BK_RET_MALLOC_FAILED;
		ReleaseMutex( m_hDeviceIndexMutex);		
        goto Exit_writeFiledata;
    }
    memset(data_buffer, 0xff, ChipMemory[chip_type]);
    //memset(data_fromTarget, 0, m_BinFileSize);	
    file.Read(data_buffer, m_BinFileSize);
#if 0
	if(((CButton *)(GetDlgItem(IDC_CHECK7)))->GetCheck() ==1) 
	{
		//WaitForSingleObject( m_hDeviceIndexMutex, INFINITE );
		
		//memcpy(m_AddrBuf[devIndex], m_btaddr, 6);
		memcpy( &data_buffer[addr_name_offset[chip_type].addr_offset], m_btaddr, 6);
		pBT64addr= (UINT64*)m_btaddr;
		(*pBT64addr) += 1; 
		
		//ReleaseMutex( m_hDeviceIndexMutex);						
		
		//memset( &data_buffer[addr_name_offset[chip_type].name_offset], 0, BT_NAME_MAX_LENGTH_IN_BIN_FILE );
		//memcpy( &data_buffer[addr_name_offset[chip_type].name_offset], m_btname.GetBuffer(m_btname.GetLength()), m_btname.GetLength()); 	
    }
#else
    if (chip_type == AI6166)
    {
        unsigned char addr[6];
		char szTmp[32];

		addr[0] =(unsigned char)(id_save_buf[devIndex]>>40);
		addr[1] =(unsigned char)(id_save_buf[devIndex]>>32);
		addr[2] =(unsigned char)(id_save_buf[devIndex]>>24);
		addr[3] =(unsigned char)(id_save_buf[devIndex]>>16);
		addr[4] =(unsigned char)(id_save_buf[devIndex]>>8);
		addr[5] =(unsigned char)(id_save_buf[devIndex]>>0);
        sprintf(szTmp,"%02x:%02x:%02x:%02x:%02x:%02x",addr[0],addr[1], \
					addr[2],addr[3],addr[4],addr[5]);
		//memcpy( &data_buffer[0x500B],&szTmp[0], 17);//memcpy( &data_buffer[0x500F],&szTmp[0], 17); 
		
		memcpy( &data_buffer[0x500F],&szTmp[0], 17);//memcpy( &data_buffer[0x500F],&szTmp[0], 17); 

		/*addr[0] =(unsigned char)((id_save_buf[devIndex]+1)>>40);
		addr[1] =(unsigned char)((id_save_buf[devIndex]+1)>>32);
		addr[2] =(unsigned char)((id_save_buf[devIndex]+1)>>24);
		addr[3] =(unsigned char)((id_save_buf[devIndex]+1)>>16);
		addr[4] =(unsigned char)((id_save_buf[devIndex]+1)>>8);
		addr[5] =(unsigned char)((id_save_buf[devIndex]+1)>>0);
        sprintf(szTmp,"%02x:%02x:%02x:%02x:%02x:%02x",addr[0],addr[1], \
					addr[2],addr[3],addr[4],addr[5]);*/
		//memcpy( &data_buffer[0x5029],&szTmp[0], 17); //memcpy( &data_buffer[0x5030],&szTmp[0], 17); 
		memcpy( &data_buffer[0x5030],&szTmp[0], 17); //memcpy( &data_buffer[0x5030],&szTmp[0], 17); 
    }
	else
	{
        memcpy( &data_buffer[addr_name_offset[chip_type].addr_offset], &gb_AddrTab[devIndex][0], 6); 
	}
#endif
	if(chip_type==BK3231S)
	{
	   int bin_size;
	   bin_size=m_BinFileSize%256;
	   if(bin_size)
          chip_size=m_BinFileSize+(256-bin_size);
	   else
	   	  chip_size=m_BinFileSize; 
	   chip_size+=1024;

	   srand( (unsigned)time( NULL ) ); 
	   
	   for(i=0;i<32;i++)
	   	 data_buffer[0x40200+i]=rand();
	   gb_crc=0xfffff00;
	     
	   UINT32 sum_crc=0;
	   sum_crc=crc_32_calculate(&data_buffer[0x40200],32,gb_crc);
	   sum_crc+=crc_32_calculate(&data_buffer[0x40000],256,gb_crc);
	   
       data_buffer[0x40300]=(unsigned char)(sum_crc&0x000000ff);   //低位
	   data_buffer[0x40301]=(unsigned char)((sum_crc>>8)&0x000000ff);
	   data_buffer[0x40302]=(unsigned char)((sum_crc>>16)&0x000000ff);
	   data_buffer[0x40303]=(unsigned char)((sum_crc>>24)&0x000000ff);//高位
	   
	} 
	ReleaseMutex( m_hDeviceIndexMutex);		
	
    m_progress_dl[devIndex].SetRange32(0,chip_size);

    buf[0] = WRITE_FLASH_START_CMD;
    buf[1] = (char)(chip_size & 0x000000FF);
    buf[2] = (char)((chip_size & 0x0000FF00) >> 8);
    buf[3] = (char)((chip_size & 0x00FF0000) >> 16);
	buf[4] = chip_type;

	if(((CButton *)(GetDlgItem(IDC_PASSWORD)))->GetCheck()==TRUE)
	{
        buf[5] =1;
	}
	else
	{
        buf[5] =0;
	}
	memcpy(&buf[6], m_btaddr, 6);
	
	memset(&buf[12], 0, 4);
	
	totalWritten = 0;
	if((!(DeviceWrite(devIndex, buf, 16, &dwBytesWritten))) || (dwBytesWritten != 16))
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
		goto Modify_BTADDR_FLAG;
	}

	if(1)
	{
		memset(buf, 0, 2);
		retry_times=60;
	    Sleep(20);
		//success = DeviceRead(devIndex, buf, 2, &dwBytesRead);
        buf[0]=0;
		// Check for ACK packet after writing 8 pkts.
		while ( (buf[0] != 0xFF) && success)
		{
			Sleep(1);
			success = DeviceRead(devIndex, buf, 2, &dwBytesRead);
		
			if((retry_times--)<=0)
			   goto Modify_BTADDR_FLAG;
		}
		ret_status = BK_RET_SUCCESS;
	}	

	
    memset(buf, 0, JTAG_FRAME_LENGTH);
    times = (256/64);
	
	for(i=0; totalWritten < chip_size; i++ )
	{
		if((chip_size-totalWritten) <= 256)  //LAST FRAME
		{
			dwWriteLength = (chip_size- totalWritten);			
			buf[0]= WRITE_FLASH_DATA_CMD;
			memcpy(&(buf[1]), &data_buffer[totalWritten], dwWriteLength);
			totalWritten = chip_size;

			ret_status = DeviceWrite(devIndex, buf, dwWriteLength+1, &dwBytesWritten);
			m_progress_dl[devIndex].SetPos(totalWritten);
			frameLen = 0;
		}
		else
		{			
			buf[0]= WRITE_FLASH_DATA_CMD;
			dwWriteLength = 256;
			memcpy(&(buf[1]), &data_buffer[totalWritten], dwWriteLength);
			totalWritten += 256;

			ret_status = DeviceWrite(devIndex, buf, dwWriteLength+1, &dwBytesWritten);
			m_progress_dl[devIndex].SetPos(totalWritten);
			frameLen = 0;
		}

		if(ret_status==TRUE)
		{
			memset(buf, 0, 2);
			retry_times=60;
		    buf[0]=0;
			
			// Check for ACK packet after writing 8 pkts.
			while ( (buf[0] != 0xFF) && success)
			{
				Sleep(1);
				success = DeviceRead(devIndex, buf, 2, &dwBytesRead);
			
				if(((retry_times--)<=0)||((buf[0]==0XFE)&&success)) 
				   goto Modify_BTADDR_FLAG;
			}
			ret_status = BK_RET_SUCCESS;
		}
	}

/*
//JtagMidWrite(0x80101c, 0x02, 0x08);     //disable WP
ret_status = Do_JtagRegWrite(devIndex, 0x80101c, 0x08);
if(BK_RET_SUCCESS != ret_status)
{
	goto Modify_BTADDR_FLAG;
}


//JtagMidWrite(0x801010, 0x02, 0x01);     //disable WP
ret_status = Do_JtagRegWrite(devIndex, 0x801010, 0x01);
if(BK_RET_SUCCESS != ret_status)
{
	goto Modify_BTADDR_FLAG;
}

	
//JtagMidWrite(0x801018, 0x02, 0x02);     // support 1-byte SR
ret_status = Do_JtagRegWrite(devIndex, 0x801018, 0x02);
if(BK_RET_SUCCESS != ret_status)
{
	goto Modify_BTADDR_FLAG;
}
	
//JtagMidWrite(0x801014, 0x02, 0x3c);     //status register = 0;
ret_status = Do_JtagRegWrite(devIndex, 0x801014, 0x3c);
if(BK_RET_SUCCESS != ret_status)
{
	goto Modify_BTADDR_FLAG;
}
	
//JtagMidWrite(0x801004, 0x02, 0x04);     //cmd type = WRSR
ret_status = Do_JtagRegWrite(devIndex, 0x801004, 0x04);
if(BK_RET_SUCCESS != ret_status)
{
	goto Modify_BTADDR_FLAG;
}

//JtagMidWrite(0x801008, 0x02, 0x01);     // start = 1
ret_status = Do_JtagRegWrite(devIndex, 0x801008, 0x01);
if(BK_RET_SUCCESS != ret_status)
{
	goto Modify_BTADDR_FLAG;
}

	
//JtagMidRead(0x20, 0x02)
    buf[0] = JTAG_MID_WRITE_CMD;
    buf[1] = (char)(0x20 & 0x000000FF);
    buf[2] = (char)((0x20 & 0x0000FF00) >> 8);
    buf[3] = (char)((0x20 & 0x00FF0000) >> 16);
	buf[4] = (char)((0x20 & 0xFF000000) >> 24);
	if((!(DeviceWrite(devIndex, buf, 5, &dwBytesWritten))) || (dwBytesWritten != 5))
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
		goto Modify_BTADDR_FLAG;
	}
	DeviceRead(devIndex, &buf[0], 5, &dwBytesRead);

//JtagMidRead(0x200, 0x02)
    buf[0] = JTAG_MID_WRITE_CMD;
    buf[1] = (char)(0x200 & 0x000000FF);
    buf[2] = (char)((0x200 & 0x0000FF00) >> 8);
    buf[3] = (char)((0x200 & 0x00FF0000) >> 16);
	buf[4] = (char)((0x200 & 0xFF000000) >> 24);
	if((!(DeviceWrite(devIndex, buf, 5, &dwBytesWritten))) || (dwBytesWritten != 5))
	{
		ret_status = BK_RET_USB_WRITE_FAILED;
		goto Modify_BTADDR_FLAG;
	}
	DeviceRead(devIndex, &buf[0], 5, &dwBytesRead);

	Sleep(75);

	if((ret_status = ReadFileData(devIndex, data_fromTarget, m_BinFileSize)) != BK_RET_SUCCESS)
		goto Modify_BTADDR_FLAG;

	for(i=0; i<m_BinFileSize; i++)
	{
		if(data_fromTarget[i]!= data_buffer[i])
		{			
			ret_status = BK_RET_CONTENT_DIFF_FAILED;
			goto Modify_BTADDR_FLAG;			
		}
	}
*/	
	dl_num++;

Modify_BTADDR_FLAG:
	if(ret_status == BK_RET_SUCCESS)
	{
		m_ResultFlag[devIndex]= 0;
	}
	else 
	{
		m_ResultFlag[devIndex]= 1;
	}
	
Exit_writeFiledata:
    //if (m_BinFilePathName.GetLength() > 0)
    	file.Close();	
    //m_progress_dl[devIndex].SetPos(0);
	free(data_buffer);
//	free(data_fromTarget);

    return ret_status;
}

void CJxDownloadDlg::OnButtonDownload() 
{
	// TODO: Add your control notification handler code here
	DWORD dwThreadId;
	
    UpdateData(TRUE);
	char *p;
    int len,j;
    CString str;
	CString mac_addr_str=_T("");
	int i;

	if(m_id_text.GetLength()!=16)
	{
        MessageBox("请输入正确的地址!","错误",MB_OK);
		((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
		return;
	}
	else
	{
        TCHAR p;
		int i;

		p = m_id_text.GetAt(0);
		if(p!='M')
		{
	        MessageBox("请输入正确的地址!","错误",MB_OK);
			((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
			return;
		}
		p = m_id_text.GetAt(1);
		if(p!='A')
		{
			MessageBox("请输入正确的地址!","错误",MB_OK);
			((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
			return;
		}
		p = m_id_text.GetAt(2);
		if(p!='C')
		{
			MessageBox("请输入正确的地址!","错误",MB_OK);
			((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
			return;
		}
		p = m_id_text.GetAt(3);
		if(p!=':')
		{
			MessageBox("请输入正确的地址!","错误",MB_OK);
			((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
			return;
		}
		
		for(i = 4;i < 16;i++)
		{
           p = m_id_text.GetAt(i);
		   if(((p>='a')&&(p<='f'))||((p>='A')&&(p<='F'))||((p>='0')&&(p<='9')))
		   {
               mac_addr_str+=p;
		   }
		   else
		   {
               MessageBox("请输入正确的地址!","错误",MB_OK);
	  	       ((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
		       return;
		   }
		}
        m_info_text = _T("");
		
		char strTmp[2];
		sprintf(strTmp,"%d",m_DeviceNum);
		m_info_text += _T("总共发现: ");
		m_info_text += strTmp;
		m_info_text += _T("个设备\r\n");	
		
	    m_info_text += _T("扫描地址: ")+mac_addr_str+"\r\n";
	    m_info_text += _T("\r\n");

		CString addr_str,tempstr;
		UINT64* pBT64addr;
		
		for(j = 0; j<MULTI_USB_DEVICE_MAXNUM; j++)
		{
		    char szTmp[128];
			
		    sprintf(szTmp,"%02d",j+1);
			m_info_text += _T("设备");
			m_info_text += szTmp;
			m_info_text += _T("地址: ");
			
            if(j == 0)
            {
				for(i = 0;i < 6;i++) 
				{
				    addr_str = _T("");
			  	    addr_str += m_id_text.GetAt(i*2+4);
				    addr_str += m_id_text.GetAt(i*2+1+4);

				    m_info_text+=addr_str + _T(" ");
		            String2HexData(addr_str, &gb_AddrTab[j][i]); 

					UINT64 Iddata=0;
	                Iddata =((UINT64)gb_AddrTab[j][0])<<40;
	                Iddata|=((UINT64)gb_AddrTab[j][1])<<32;  
	                Iddata|=((UINT64)gb_AddrTab[j][2])<<24;   
	                Iddata|=((UINT64)gb_AddrTab[j][3])<<16;    
	                Iddata|=((UINT64)gb_AddrTab[j][4])<<8;
	                Iddata|=((UINT64)gb_AddrTab[j][5])<<0;

					id_save_buf[j] = Iddata;
			    }
				
            }
			else
			{   				
		        for(i = 0;i < 6;i++) 
				{
				   gb_AddrTab[j][i] = gb_AddrTab[0][i];
		        }
				UINT64 Iddata=0;
                Iddata =((UINT64)gb_AddrTab[j][0])<<40;
                Iddata|=((UINT64)gb_AddrTab[j][1])<<32;  
                Iddata|=((UINT64)gb_AddrTab[j][2])<<24;   
                Iddata|=((UINT64)gb_AddrTab[j][3])<<16;    
                Iddata|=((UINT64)gb_AddrTab[j][4])<<8;
                Iddata|=((UINT64)gb_AddrTab[j][5])<<0;
				
                Iddata+=j;
				
				gb_AddrTab[j][0] =(unsigned char)(Iddata>>40);
                gb_AddrTab[j][1] =(unsigned char)(Iddata>>32);
                gb_AddrTab[j][2] =(unsigned char)(Iddata>>24);
                gb_AddrTab[j][3] =(unsigned char)(Iddata>>16);
                gb_AddrTab[j][4] =(unsigned char)(Iddata>>8);
                gb_AddrTab[j][5] =(unsigned char)(Iddata>>0);
				sprintf(szTmp,"%02x %02x %02x %02x %02x %02x",gb_AddrTab[j][0],gb_AddrTab[j][1], \
					gb_AddrTab[j][2],gb_AddrTab[j][3],gb_AddrTab[j][4],gb_AddrTab[j][5]);

				//sprintf(szTmp,"%02x:%02x:%02x:%02x:%02x:%02x",gb_AddrTab[j][0],gb_AddrTab[j][1], \
				//	gb_AddrTab[j][2],gb_AddrTab[j][3],gb_AddrTab[j][4],gb_AddrTab[j][5]);
				m_info_text += szTmp;

				id_save_buf[j] = Iddata;
			}
			m_info_text+="\r\n";
			if(j == 0)
			{
                m_info_text+="\r\n";
			}
		}
		m_info_text += _T("\r\n");
	}
	//m_info_text += _T("\r\n****************开始烧录****************\r\n");
	m_info_text += _T("\r\n");
    
    UpdateData(FALSE);

	#if 0
	for(j=0;j<6;j++)
	{
		len = m_btaddrStr[j].GetLength();
        if(len%2) //首字节添加0, 补足偶数
        {
           str="0"+m_btaddrStr[j];
		   m_btaddrStr[j]=str;
           len++;
        }
		
		p = m_btaddrStr[j].GetBuffer(len);

		if(((*p>='a')&&(*p<='f'))||((*p>='A')&&(*p<='F'))||((*p>='0')&&(*p<='9'))||(*p==8))
	    {
	       
	    }
	    else
	    {
	        MessageBox("请输入正确的地址!","错误",MB_OK);
		    m_btaddrStr[j]=_T("00");
		    UpdateData(FALSE);
			return;
	    }
        p++;
		if(((*p>='a')&&(*p<='f'))||((*p>='A')&&(*p<='F'))||((*p>='0')&&(*p<='9'))||(*p==8))
	    {
	       
	    }
	    else
	    {
	        MessageBox("请输入正确的地址!","错误",MB_OK);
		    m_btaddrStr[j]=_T("00");
		    UpdateData(FALSE);
			return;
	    }
	}

	for(j=0;j< 6;j++)
	   String2HexData(m_btaddrStr[j], &m_btaddr[j]);
	UpdateData(FALSE);
	#endif
	
	chip_index = AI6166;//((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
	
	(GetDlgItem(IDC_BUTTON_DOWNLOAD))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_READFLASH))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_ERASEALL))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_LOAD))->EnableWindow(false);
	(GetDlgItem(IDC_PASSWORD))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_RESET))->EnableWindow(false);
	
	
	((CButton *)(GetDlgItem(IDC_CHECK7)))->EnableWindow(false);
	(GetDlgItem(IDC_TARGET))->EnableWindow(false);

	//FillDeviceList();
    
	if(m_hDeviceIndexMutex != NULL)
	{
		CloseHandle( m_hDeviceIndexMutex );
		m_hDeviceIndexMutex = NULL;
	}
	m_hDeviceIndexMutex = CreateMutex( NULL, FALSE, NULL );

	for(int i=0; i<MULTI_USB_DEVICE_MAXNUM; i++)
    {
       m_progress_dl[i].SetPos(0);
	   DevList_Down[i]=0;
	}

	j=0;
    for(i=0; i<MULTI_USB_DEVICE_MAXNUM; i++)
    {
       if(DevList[i]!=NULL)
       {
          DevList_Down[j++]=i+1;
       }
	   GetDlgItem(PicWidgetList[i])->ShowWindow(SW_HIDE);
    }
	
	for(i=0; i<m_DeviceNum; i++)
	{
		m_hThread[i] = CreateThread(NULL, 0, DownloadProc, this, 0, &dwThreadId);
		gb_haddown_flag=TRUE;
	}

	((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
	m_id_text = _T("");
	return;
}

void CJxDownloadDlg::OnButtonUSBReset() 
{
	// TODO: Add your control notification handler code here
    DWORD dwThreadId;
	
    UpdateData(TRUE);
	char *p;
    int len,j;
    CString str;
	int i;

	if(m_DeviceIndex)
		return;
	/*for(j=0;j<6;j++)
	{
		len = m_btaddrStr[j].GetLength();
        if(len%2) //首字节添加0, 补足偶数
        {
           str="0"+m_btaddrStr[j];
		   m_btaddrStr[j]=str;
           len++;
        }
		
		p = m_btaddrStr[j].GetBuffer(len);

		if(((*p>='a')&&(*p<='f'))||((*p>='A')&&(*p<='F'))||((*p>='0')&&(*p<='9'))||(*p==8))
	    {
	       
	    }
	    else
	    {
	        MessageBox("请输入正确的地址!","错误",MB_OK);
		    m_btaddrStr[j]=_T("00");
		    UpdateData(FALSE);
			return;
	    }
        p++;
		if(((*p>='a')&&(*p<='f'))||((*p>='A')&&(*p<='F'))||((*p>='0')&&(*p<='9'))||(*p==8))
	    {
	       
	    }
	    else
	    {
	        MessageBox("请输入正确的地址!","错误",MB_OK);
		    m_btaddrStr[j]=_T("00");
		    UpdateData(FALSE);
			return;
	    }
	}

	for(j=0;j< 6;j++)
	   String2HexData(m_btaddrStr[j], &m_btaddr[j]);
	UpdateData(FALSE);
	*/
	(GetDlgItem(IDC_BUTTON_DOWNLOAD))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_READFLASH))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_ERASEALL))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_LOAD))->EnableWindow(false);
	(GetDlgItem(IDC_PASSWORD))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_RESET))->EnableWindow(false);
	
	
	((CButton *)(GetDlgItem(IDC_CHECK7)))->EnableWindow(false);
	(GetDlgItem(IDC_TARGET))->EnableWindow(false);

	//FillDeviceList();
    
	if(m_hDeviceIndexMutex != NULL)
	{
		CloseHandle( m_hDeviceIndexMutex );
		m_hDeviceIndexMutex = NULL;
	}
	m_hDeviceIndexMutex = CreateMutex( NULL, FALSE, NULL );

	for(i=0; i<MULTI_USB_DEVICE_MAXNUM; i++)
    {
       if(DevList_Down[i]!=0)
       {
           m_progress_dl[DevList_Down[i]-1].SetPos(0);
		   GetDlgItem(PicWidgetList[DevList_Down[i]-1])->ShowWindow(SW_HIDE);
       }
	}
	
    UCHAR down_buf[MULTI_USB_DEVICE_MAXNUM]={0};
	j=0;
	for(i=0; i<MULTI_USB_DEVICE_MAXNUM; i++)
    {
        if(DevList_Down[i])
        {
            down_buf[j]=DevList_Down[i];
			j++;	
        }
	}
	
	for(i=0; i<MULTI_USB_DEVICE_MAXNUM; i++)
	   DevList_Down[i]=0;
	
	for(i=0; i<j; i++)
    {
       DevList_Down[i]=down_buf[i];
	}

	
	for(i=0; i<j; i++)
	{
		m_hThread[i] = CreateThread(NULL, 0, DownloadProc, this, 0, &dwThreadId);
		gb_haddown_flag=TRUE;
	}
	((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
	return;
}

void CJxDownloadDlg::OnButtonEraseAll() 
{
	// TODO: Add your control notification handler code here
	DWORD dwThreadId;

	(GetDlgItem(IDC_BUTTON_DOWNLOAD))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_READFLASH))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_ERASEALL))->EnableWindow(false);
	FillDeviceList();

	if(m_hDeviceIndexMutex != NULL)
	{
		CloseHandle( m_hDeviceIndexMutex );
		m_hDeviceIndexMutex = NULL;
	}
	m_hDeviceIndexMutex = CreateMutex( NULL, FALSE, NULL );
	for(int i=0; i<m_DeviceNum; i++)
	{
		GetDlgItem(PicWidgetList[i])->ShowWindow(SW_HIDE);
		m_hThread[i] = CreateThread(NULL, 0, EraseAllProc, this, 0, &dwThreadId);
	}
}

void CJxDownloadDlg::OnButtonReadFlash() 
{
	// TODO: Add your control notification handler code here
	DWORD dwThreadId;

	(GetDlgItem(IDC_BUTTON_DOWNLOAD))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_READFLASH))->EnableWindow(false);
	(GetDlgItem(IDC_BUTTON_ERASEALL))->EnableWindow(false);
	FillDeviceList();

	if(m_hDeviceIndexMutex != NULL)
	{
		CloseHandle( m_hDeviceIndexMutex );
		m_hDeviceIndexMutex = NULL;
	}
	m_hDeviceIndexMutex = CreateMutex( NULL, FALSE, NULL );
	for(int i=0; i<m_DeviceNum; i++)
	{
		GetDlgItem(PicWidgetList[i])->ShowWindow(SW_HIDE);
		m_hThread[i] = CreateThread(NULL, 0, ReadFlashContentProc, this, 0, &dwThreadId);
	}
}
void CJxDownloadDlg::UpdateBTAddrUI(unsigned char* tempBTaddr)
{
	memcpy(m_btaddr, tempBTaddr, BT_ADDR_LENGTH_IN_BIN_FILE);
	for(int i=0; i< BT_ADDR_LENGTH_IN_BIN_FILE; i++)
		m_btaddrStr[i].Format("%02x", m_btaddr[i]);
    

	//((CEdit*)GetDlgItem(IDC_EDIT1))->SetSel(0,-1);
	//m_btaddr_current.Format("当前地址:\t%02x:%02x:%02x:%02x:%02x:%02x",
	//	m_btaddr[5], m_btaddr[4], m_btaddr[3], m_btaddr[2], m_btaddr[1], m_btaddr[0]);
    //UpdateData(FALSE);
}

void CJxDownloadDlg::UpdateDownloadResult(DWORD devIndex, DWORD result)
{
	
	UpdateData(FALSE);

}

void CJxDownloadDlg::OnButtonLoad() 
{
	// TODO: Add your control notification handler code here
	CFileDialog			dlgFileLoad(TRUE,"bin", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Bin File (*.bin)|*.bin|All Files (*.*)|*.*||");
	CFile				file_crc;
	unsigned char       buffer[BT_NAME_MAX_LENGTH_IN_BIN_FILE];
	unsigned int   crc_len = 0;
	unsigned short crc_result= 0;
	int  chip_type = AI6166;// ((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
    int  crc_buf[6];
	unsigned char crc_sum[6];
	BOOL flag = TRUE;
	
	if(gb_hadreg_flag==FALSE)
	{
        int i;
		CString str,str1;
		str1=GetMac1();
		str=gb_regstr.Left(12);

		if(str!=str1)
		   return;

		char szTemp[100];
        memset(szTemp,0,sizeof(szTemp));
        sprintf(szTemp,"%s",gb_regstr);
	   
        for(i=0;i<6;i++)
        {
			str=szTemp[0+(i*4)];
			str+=szTemp[1+(i*4)];
			str+=szTemp[2+(i*4)];
			str+=szTemp[3+(i*4)];
			crc_buf[i]=String2Int(str);
        }
		crc_buf[0]=crc_buf[0]^0x5555;
		crc_buf[1]=crc_buf[1]^0x5555;
		crc_buf[2]=crc_buf[2]^0x5555;

		gb_crc=0x8585;
		unsigned char sum_crc;
        for(i=0;i<6;i++)
        {
           sum_crc=crc_buf[i]&0x00ff;
		   crc_sum[i]=crc_32_calculate(&sum_crc,1,gb_crc)&0x00ff;
        }
		
        		
		UpdateData(TRUE);
		for(i=0; i< 6; i++)
		   flag &= String2HexData(m_btaddrStr[i], &m_btaddr[5-i]);

		if(memcmp(&m_btaddr[0],&crc_sum[0],6)!=0)
		{
            MessageBox("注册码输入错误","错误",MB_OK);
		    return;
		}
		else
		{
            for(i=0; i< 6; i++)
	        {
		       str.Format("%02X", m_btaddr[5-i]);
	           m_btaddrStr[i]=str;
			}
		  
//------------------------------------------------------------------
	        HKEY hkey;
			LPCTSTR data_Set=_T("Software\\JX\\Set2");
         
			CString str1;
			str1=m_btaddrStr[0]+m_btaddrStr[1]+m_btaddrStr[2]+m_btaddrStr[3]+  \
				 m_btaddrStr[4]+m_btaddrStr[5];

		    if (RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
		    { 
			   VERIFY(!RegCreateKey(HKEY_CURRENT_USER, data_Set, &hkey));
			   //VERIFY(!RegSetValueEx(hkey, _T("USER"), 0, REG_SZ, (BYTE *)m_btaddrStr[0].GetBuffer(m_btaddrStr[0].GetLength()), 2));
			   VERIFY(!RegSetValueEx(hkey, _T("USER"), 0, REG_SZ, (BYTE *)str1.GetBuffer(str1.GetLength()),250));

		    }
			else
		    if(RegOpenKeyEx(HKEY_CURRENT_USER, data_Set, 0, KEY_ALL_ACCESS, &hkey)!= ERROR_SUCCESS)
		    {
		        VERIFY(!RegCreateKey(HKEY_CURRENT_USER, data_Set, &hkey));
				//VERIFY(!RegSetValueEx(hkey, _T("USER"), 0, REG_SZ, (BYTE *)m_btaddrStr[0].GetBuffer(m_btaddrStr[0].GetLength()), 2));
	            VERIFY(!RegSetValueEx(hkey, _T("USER"), 0, REG_SZ, (BYTE *)str1.GetBuffer(str1.GetLength()),250));
			}
			RegCloseKey(hkey);
			MessageBox("注册码输入正确，请重启!","正确",MB_OK);
//------------------------------------------------------------------
		}
	    return;    
	}
	
	if( dlgFileLoad.DoModal() == IDCANCEL)
		return;
	
	m_BinFilePathName = dlgFileLoad.GetPathName();	
	

	if( NULL == file_crc.Open(m_BinFilePathName, CFile::modeRead | CFile::shareDenyNone))
	{
		m_btaddr_current.Format("烧录bin文件打开失败\r\n");
		UpdateData(FALSE);
		return;
	}
	
	m_BinFileSize = file_crc.GetLength();

	if(m_BinFileSize>ChipMemory[chip_type])
	{
        file_crc.Close();
		MessageBox("文件大小超出芯片内存!","错误",MB_OK);
		return;
	}

	if(m_BinFileSize)
	{
		//begin CRC caculation 
		crc_len =  m_BinFileSize;
		
		file_crc.Seek(0, CFile::begin);
		
		while(crc_len--)
		{
			file_crc.Read(buffer, 1);
			crc_result += buffer[0];
		}
		
		/*crc_len = 6;
		file_crc.Seek(addr_name_offset[chip_type].addr_offset, CFile::begin);
		while(crc_len--)
		{
			file_crc.Read(buffer, 1);
			crc_result-= buffer[0];
		}*/

		m_CRCResult.Format("文件检验和:0x%x", crc_result);
		//end CRC caculation 
	}
	
	if(m_BinFileSize && m_DeviceNum)
		GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(true);
	else
		GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(false);
		//GetDlgItem(IDC_BUTTON_DOWNLOAD)->EnableWindow(true);
	
	dl_num = 0;
	m_num_current.Format("烧录数量：%d", dl_num);

	//memset(m_btaddr, 0, BT_ADDR_LENGTH_IN_BIN_FILE);
	m_btname = _T("");
	if(1)//m_BinFileSize > (addr_name_offset[chip_type].addr_offset + 60))
	{
		m_SupportUpdateBTAddr = 1;
		if((chip_type==BK3231)||(chip_type==BK3231S)||(chip_type==BK3256Flash)||(chip_type==BK3256))
		{
		   file_crc.Seek(addr_name_offset[chip_type].name_offset, CFile::begin);
		   file_crc.Read(buffer, BT_NAME_MAX_LENGTH_IN_BIN_FILE);
		}
		else
		   memset(buffer,0,BT_NAME_MAX_LENGTH_IN_BIN_FILE);
		
		m_btname.Format("%s", buffer);
		
	    //    file_crc.Seek(addr_name_offset[chip_type].addr_offset, CFile::begin);
		//file_crc.Read(buffer, BT_ADDR_LENGTH_IN_BIN_FILE);
		//UpdateBTAddrUI(buffer);
	}
	else
	{
		memset(buffer, 0, BT_NAME_MAX_LENGTH_IN_BIN_FILE);
		m_btname.Format("%s", buffer);
		UpdateBTAddrUI(buffer);
		m_SupportUpdateBTAddr = 0;
		
	}

	file_crc.Close();

	((CEdit*)GetDlgItem(IDC_ID))->SetFocus();
	
	UpdateData(FALSE);

	return;
}

BOOL CJxDownloadDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	return CDialog::DestroyWindow();
}

void CJxDownloadDlg::OnButtonAddtset() 
{
	// TODO: Add your control notification handler code here
	BOOL flag = TRUE;

	UpdateData(TRUE);
	for(int i=0; i< 6; i++)
		flag &= String2HexData(m_btaddrStr[i], &m_btaddr[i]);

	if(flag == FALSE)
	{
		m_btaddr_current.Format("地址格式错误!\r\n");
		(GetDlgItem(IDC_BUTTON_DOWNLOAD))->EnableWindow(false);
		UpdateData(FALSE);
		return;
	}
	else
	{
		if(m_BinFileSize && m_DeviceNum)
			(GetDlgItem(IDC_BUTTON_DOWNLOAD))->EnableWindow(true);
	}

	m_btaddr_current.Format("当前地址:\t%02x:%02x:%02x:%02x:%02x:%02x",
		m_btaddr[5], m_btaddr[4], m_btaddr[3], m_btaddr[2], m_btaddr[1], m_btaddr[0]);

	UpdateData(FALSE);
		
}

void CJxDownloadDlg::OnButtonBtNameSet()
{
	UpdateData(TRUE);
}


BOOL CJxDownloadDlg::Do_JtagStall(unsigned int devIndex) 
{
	BYTE buf[32];
	DWORD dwBytesWritten,dwBytesRead;
	BOOL success;
	int retry_times;

	buf[0]=JTAG_STALL_CMD;
	buf[1]= ((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();
	if(TRUE == DeviceWrite(devIndex, buf, 2, &dwBytesWritten))
	{
		memset(buf, 0, 2);
		retry_times=30;
		Sleep(50);
		success = DeviceRead(devIndex, buf, 11, &dwBytesRead);

		// Check for ACK packet after writing 8 pkts.
		while ( (buf[0] != 0xee) && success)
		{
			Sleep(50);
			success = DeviceRead(devIndex, buf, 11, &dwBytesRead);
			Sleep(1);
			if((retry_times--)<=0)
				break;
		}//while
		
		if(buf[1]== 0x03)
		{
			return TRUE;
		}
	}

	return FALSE;

}

BOOL CJxDownloadDlg::Do_JtagUnstall(unsigned int devIndex) 
{
	BYTE buf[32];
	DWORD dwBytesWritten,dwBytesRead;
	BOOL success;
	int retry_times;


	buf[0]=JTAG_UNSTALL_CMD;

	if(TRUE == DeviceWrite(devIndex, buf, 1, &dwBytesWritten))
	{
		memset(buf, 0, 2);
		retry_times=30;
		Sleep(50);
		success = DeviceRead(devIndex, buf, 11, &dwBytesRead);

		// Check for ACK packet after writing 8 pkts.
		while ( (buf[0] != 0xee) && success)
		{
			Sleep(50);
			success = DeviceRead(devIndex, buf, 11, &dwBytesRead);
			Sleep(1);
			if((retry_times--)<=0)
				break;
		}//while
		return TRUE;
	}
	return FALSE;	

}

unsigned int  CJxDownloadDlg::Do_JtagEraseAll(unsigned int devIndex) 
{
    BYTE buf[64];
	DWORD dwBytesWritten,dwBytesRead;
	unsigned int  status;
	int retry_times;

	buf[0]=JTAG_ERASE_ALL_CMD;
	
	if(TRUE == DeviceWrite(devIndex, buf, 1, &dwBytesWritten))
	{
		memset(buf, 0, 4);
		retry_times=30;
		status = DeviceRead(devIndex, buf, 2, &dwBytesRead);
		// Check for ACK packet after writing 8 pkts.
		while((buf[0] != 0xee) && status)
		{
			status = DeviceRead(devIndex, buf, 2, &dwBytesRead);
			Sleep(5);
			if((retry_times--)<=0)
				break;
		}
		return BK_RET_SUCCESS;
		
	}

	return BK_RET_USB_WRITE_FAILED;
}

BOOL CJxDownloadDlg::Do_SendCmd(unsigned int devIndex,BYTE cmd) 
{
	BYTE buf[32];
	DWORD dwBytesWritten,dwBytesRead;
	BOOL success;
	int retry_times;
  
	buf[0]=cmd;
	buf[1]= AI6166;//((CComboBox*) GetDlgItem(IDC_TARGET))->GetCurSel();;

    if(FLASH_ERASE_ALL_CMD == cmd)
    {
        if(((CButton *)(GetDlgItem(IDC_CHECK8)))->GetCheck() ==1) 
        {
            buf[1] |= 0x10; 
        }
		
		if(((CButton *)(GetDlgItem(IDC_CHECK9)))->GetCheck() ==1) 
        {
			buf[1] |= 0x20; 
        }

		if(((CButton *)(GetDlgItem(IDC_CHECK10)))->GetCheck() ==1) 
        {
			buf[1] |= 0x40; 
        }

		if(((CButton *)(GetDlgItem(IDC_CHECK11)))->GetCheck() ==1) 
        {
			buf[1] |= 0x80; 
        }
    }
	
	if(TRUE == DeviceWrite(devIndex, buf, 2, &dwBytesWritten))
	{
		memset(buf, 0, 2);
		retry_times=60;
		Sleep(100);
		success = DeviceRead(devIndex, buf, 2, &dwBytesRead);

		// Check for ACK packet after writing 8 pkts.
		while ( (buf[0] != 0xee) && success)
		{
			Sleep(50);
			success = DeviceRead(devIndex, buf, 2, &dwBytesRead);
			Sleep(1);
			if((retry_times--)<=0)
				break;
		}
		return TRUE;
	}

	return FALSE;
}