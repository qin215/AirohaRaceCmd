#pragma once
#include "afxwin.h"

class CColorStatic :
	public CStatic
{
public:
	CColorStatic(void);
	~CColorStatic(void);
	void SetForeColor(COLORREF color);
	void SetBkColor(COLORREF color);
	void SetTextFont(int FontHight, LPCTSTR FontName);

	DECLARE_MESSAGE_MAP()
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);

private:
	COLORREF m_ForeColor;  // text color
	COLORREF m_BackColor;  // background color
	CBrush	 m_BkBrush;	   // background brush
	CFont*	 p_Font;
};

