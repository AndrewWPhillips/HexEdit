// TipWnd.cpp : implementation file
//

#include "stdafx.h"
#include <CommCtrl.h>
#include "TipWnd.h"
#include "Misc.h"    // For window within screen/monitor checks

// CTipWnd

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002
#endif

int CTipWnd::m_2k = -1;
PFSetLayeredWindowAttributes CTipWnd::m_pSLWAfunc = 0;
#define FADE_TICK 10     // Make this bigger to fade in faster

IMPLEMENT_DYNAMIC(CTipWnd, CWnd)

CTipWnd::CTipWnd(UINT fmt /* = DT_NOCLIP | DT_NOPREFIX | DT_EXPANDTABS */)
{
    static CString strClass;
    if (strClass.IsEmpty())
    {
        // Register window class
        strClass = AfxRegisterWndClass(0);
        ASSERT(!strClass.IsEmpty());
    }

	m_visible = false;
	m_fmt = fmt;
	m_margins = CSize(2, 2);
    m_bg_colour = ::GetSysColor(COLOR_INFOBK);
    m_text_colour = ::GetSysColor(COLOR_INFOTEXT);
	m_alpha = 255;

	m_in = m_out = false;

	// Check if we can do transparent window
	if (m_2k == -1)
	{
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);

        // Work out if this is Windows 200 or better
		m_2k = (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion >= 5);

        // If W2K then get pointer to SetLayeredWindowAttributes
        HINSTANCE hh;
        if (m_2k && (hh = ::LoadLibrary("USER32.DLL")) != HINSTANCE(0))
            m_pSLWAfunc = (PFSetLayeredWindowAttributes)::GetProcAddress(hh, "SetLayeredWindowAttributes");
	}

	DWORD exStyle = WS_EX_TOOLWINDOW;
	if (m_2k)
		exStyle |= WS_EX_LAYERED;  // This allows a transparent window

	VERIFY(CreateEx(exStyle,
                    strClass, NULL,
					WS_POPUP | WS_BORDER,
					0, 0, 0, 0,
                    NULL, (HMENU)0));

    ASSERT(m_hWnd != (HWND)0);
}

BEGIN_MESSAGE_MAP(CTipWnd, CWnd)
	ON_WM_PAINT()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_WM_TIMER()
    ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

void CTipWnd::Show(int delay /* = 0 */, int fade)
{
	if (!m_visible)
	{
		if (fade > 0)
		{
			// Set up fade in values
			m_fade = 0;
			m_fade_inc = m_alpha * FADE_TICK / fade;
			if (m_fade_inc < 1) m_fade_inc = 1;
		}
		else
			m_fade = m_alpha;               // This disables fade in

		if (delay <= 0)
			OnTimer(1);                 // show now
		else
			SetTimer(1, delay, NULL);   // show later
		m_visible = true;
	}

	// Make sure the window is within the screen (or closest monitor)
	CRect rect;
	GetWindowRect(&rect);
	if (::OutsideMonitor(rect))
	{
		CRect cont_rect = ::MonitorRect(rect);
		if (rect.right > cont_rect.right)
			rect += CPoint(cont_rect.right - rect.right, 0);
		if (rect.left < cont_rect.left)
			rect += CPoint(cont_rect.left - rect.left, 0);
		if (rect.bottom > cont_rect.bottom)
			rect += CPoint(0, cont_rect.bottom - rect.bottom);
		if (rect.top < cont_rect.top)
			rect += CPoint(0, cont_rect.top - rect.top);

		MoveWindow(&rect, FALSE);
	}
}

void CTipWnd::Hide(int fade /* = 200 */)
{
    if (m_visible)
    {
		KillTimer(1);                   // Kill any delayed display
		KillTimer(2); m_in = false;     // Kill any fade in
		if (fade > 0)
		{
			// Set up fade values
			m_fade = m_alpha;
			m_fade_inc = m_alpha * FADE_TICK / fade;
			if (m_fade_inc < 1) m_fade_inc = 1;
			SetTimer(3, FADE_TICK, NULL);   // Start fade out
			m_out = true;
		}
		else
		{
			m_fade = m_fade_inc = 0;                 // Instant fade out
			OnTimer(3);
		}
    }
}

void CTipWnd::Move(CPoint pt, bool centre /* = true */)
{
	// Work out current position of the window
	CRect rct;
	GetWindowRect(rct);
	CPoint curr;
	if (centre)
		curr = CPoint((rct.left + rct.right)/2, (rct.top + rct.bottom)/2);
	else
		curr = CPoint(rct.left, rct.top);

	if (pt != curr)
	{
		rct += pt - curr;
		MoveWindow(&rct, FALSE);
	}
}

void CTipWnd::SetAlpha(int alpha)
{
	ASSERT(m_2k == 0 || m_2k == 1);
	if (m_pSLWAfunc == 0)
		return;

	m_alpha = alpha;
	if (m_alpha > 255 || m_alpha < 0)
		m_alpha = 255;

    (*m_pSLWAfunc)(m_hWnd, -1, (BYTE)m_alpha, LWA_ALPHA);
}

void CTipWnd::track_mouse(unsigned long flag)
{
	if (m_pSLWAfunc == 0)
		return;

    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = flag;
    tme.dwHoverTime = 0;
    tme.hwndTrack = m_hWnd;

    VERIFY(::_TrackMouseEvent(&tme));
}

// CTipWnd message handlers

LRESULT CTipWnd::OnSetText(WPARAM, LPARAM lp)
{
	CClientDC dc(this);
	CRect rct;

    CFont *pOldFont = (CFont*)dc.SelectStockObject(SYSTEM_FONT);
    ASSERT(pOldFont != NULL);
	dc.DrawText((LPCTSTR)lp, &rct, DT_CALCRECT | m_fmt);
	dc.SelectObject(pOldFont);

	rct.InflateRect(m_margins+CSize(1,1));
	SetWindowPos(NULL, 0, 0, rct.Width(), rct.Height(), SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	Invalidate();

	return Default();
}

void CTipWnd::OnPaint()
{
	CPaintDC dc(this);                  // Device context for painting

	// Get rectangle where text is drawn
	CRect rct;
	GetClientRect(&rct);

	// Get the text to draw
	CString ss;
	GetWindowText(ss);

	// Fill background and set text colour
    dc.FillSolidRect(&rct, m_bg_colour);
	dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(m_text_colour);

	// Set font and draw the text
    CFont *pOldFont = (CFont*)dc.SelectStockObject(SYSTEM_FONT);
    ASSERT(pOldFont != NULL);
	rct.DeflateRect(m_margins);
	dc.DrawText(ss, &rct, m_fmt);
	dc.SelectObject(pOldFont);
}

void CTipWnd::OnTimer(UINT nIDEvent)
{
	switch (nIDEvent)
	{
	case 1:
		KillTimer(1);               // This is a once off so kill it now

		if (m_fade < m_alpha)
		{
			KillTimer(3); m_out = false;  // Make sure there is no fade out happening
			SetTimer(2, FADE_TICK, NULL); // Update fade in every FADE_TICK msecs
			m_in = true;
		}

		// Show and redraw the window
		if (m_pSLWAfunc != 0)
			(*m_pSLWAfunc)(m_hWnd, -1, (BYTE)m_fade, LWA_ALPHA);
		ShowWindow(SW_SHOWNA);
		Invalidate();
		UpdateWindow();
		break;
	case 2:
		m_fade += m_fade_inc;
		if (m_fade >= m_alpha)
		{
			m_fade = m_alpha;
			KillTimer(2); m_in = false; // Finished fading in
		}
		if (m_pSLWAfunc != 0)
		{
			(*m_pSLWAfunc)(m_hWnd, -1, (BYTE)m_fade, LWA_ALPHA);
		}
		break;
	case 3:
		m_fade -= m_fade_inc;
		if (m_fade <= 0)
		{
			ShowWindow(SW_HIDE);            // Hide the window
			if (m_pSLWAfunc != 0)
				(*m_pSLWAfunc)(m_hWnd, -1, (BYTE)m_alpha, LWA_ALPHA);  // restore transparency 
			m_visible = false;
			KillTimer(3); m_out = false;   // Finished fading out
		}
		else if (m_pSLWAfunc != 0)
		{
		    (*m_pSLWAfunc)(m_hWnd, -1, (BYTE)m_fade, LWA_ALPHA);
		}
		break;
	default:
		ASSERT(0);
	}
}

void CTipWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (m_pSLWAfunc != 0)
        track_mouse(TME_HOVER);
}

LRESULT CTipWnd::OnMouseHover(WPARAM, LPARAM lp)
{
    ASSERT(m_pSLWAfunc != 0 && m_hWnd != 0);
	m_fade = m_alpha;           // Override any fade in
    (*m_pSLWAfunc)(m_hWnd, -1, (BYTE)255, LWA_ALPHA);
    track_mouse(TME_LEAVE);
    return 0;
}

LRESULT CTipWnd::OnMouseLeave(WPARAM, LPARAM lp)
{
    ASSERT(m_pSLWAfunc != 0 && m_hWnd != 0);
    (*m_pSLWAfunc)(m_hWnd, -1, (BYTE)m_alpha, LWA_ALPHA);
    return 0;
}
