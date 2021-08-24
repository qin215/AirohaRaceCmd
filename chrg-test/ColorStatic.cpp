#include "stdafx.h"
#include "ColorStatic.h"

BEGIN_MESSAGE_MAP(CColorStatic, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

CColorStatic::CColorStatic(void)
{
	m_ForeColor = RGB(0, 0, 0);
	m_BackColor = RGB(240, 240, 240);
	m_BkBrush.CreateSolidBrush(m_BackColor);
	p_Font = NULL;
}


CColorStatic::~CColorStatic(void)
{
	if (p_Font)
	{
		delete p_Font;
		p_Font = NULL;
	}
}


HBRUSH CColorStatic::CtlColor(CDC* pDC, UINT nCtlColor)
{
	// TODO:  Change any attributes of the DC here
	pDC->SetTextColor(m_ForeColor);
	pDC->SetBkColor(m_BackColor);
	m_BkBrush.DeleteObject();
	m_BkBrush.CreateSolidBrush(m_BackColor);

	// TODO:  Return a non-NULL brush if the parent's handler should not be called
	return (HBRUSH)m_BkBrush.GetSafeHandle();
}

void CColorStatic::SetForeColor(COLORREF color)
{
	m_ForeColor = color;
}

void CColorStatic::SetBkColor(COLORREF color)
{
	m_BackColor = color;

	SetRedraw(TRUE);
	Invalidate();
	UpdateWindow();
}

void CColorStatic::SetTextFont(int FontHight, LPCTSTR FontName)
{
	if (p_Font)
	{
		delete p_Font;
	}
	p_Font = new CFont;
	p_Font->CreatePointFont(FontHight, FontName);
	SetFont(p_Font);
}