// mp_test.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "mp_test.h"
#include "mp_testDlg.h"
#include "common.h"
#include "homi_common.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMp_testApp

BEGIN_MESSAGE_MAP(CMp_testApp, CWinApp)
	//{{AFX_MSG_MAP(CMp_testApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMp_testApp construction

CMp_testApp::CMp_testApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMp_testApp object

CMp_testApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMp_testApp initialization

BOOL CMp_testApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	AfxOleInit();
	CoInitialize(NULL);

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif


	CMp_testDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK

	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel

	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CMp_testApp::DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWinApp::DoMessageBox(lpszPrompt, nType, nIDPrompt);
}

#if 0
extern "C"
void win32_start_thread(AFX_THREADPROC proc, LPVOID param)
{
	::AfxBeginThread(proc, param); //接下来做啥就直接调用pThead就行.
}
#endif
