// mp_test.h : main header file for the MP_TEST application
//

#if !defined(AFX_MP_TEST_H__C66BF1D5_182F_4655_B869_9432700A6064__INCLUDED_)
#define AFX_MP_TEST_H__C66BF1D5_182F_4655_B869_9432700A6064__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMp_testApp:
// See mp_test.cpp for the implementation of this class
//

class CMp_testApp : public CWinApp
{
public:
	CMp_testApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMp_testApp)
	public:
	virtual BOOL InitInstance();
	virtual int DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMp_testApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MP_TEST_H__C66BF1D5_182F_4655_B869_9432700A6064__INCLUDED_)
