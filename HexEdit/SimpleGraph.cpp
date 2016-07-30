// SimpleGraph.cpp : Simple window that shows a graph - used in properties dialog
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEditView.h"
#include "SimpleGraph.h"
#include "misc.h"

// CSimpleGraph

IMPLEMENT_DYNAMIC(CSimpleGraph, CWnd)

CSimpleGraph::CSimpleGraph()
{
	m_pdc = NULL;
	m_bar = -1;
	m_heightAdjust = 39;
}

void CSimpleGraph::SetData(FILE_ADDRESS maximum, std::vector<FILE_ADDRESS> val, std::vector<COLORREF> col)
{
	if (val == m_val && col == m_col)
		return;                         // no change

	ASSERT(val.size() == col.size());
	m_max = maximum;
	if (m_max < 1) m_max = 1;  // avoid divide by zero
	m_val.swap(val);
	m_col.swap(col);

	update();
}

void CSimpleGraph::update()
{
	CRect rct;
	GetClientRect(rct);

	// If the graph has been resized we need a new DC/bitmap
	if (m_rct != rct)
	{
		m_rct = rct;

		CWnd *pAnc;
		if ((pAnc = GetParent()) != NULL && (pAnc = pAnc->GetParent()) != NULL)
		{
			CRect rctAnc;
			pAnc->GetClientRect(rctAnc);

			// Somehow when the Properties windows is resized small or -ve the graph window
			// becomes bigger than its container so we work out the adjustment here.
			m_rct.bottom = rctAnc.Height() - m_heightAdjust;
		}
		// Delete old DC/bitmap
		if (m_pdc != NULL)
		{
			delete m_pdc;
			m_pdc = NULL;
			m_bm.DeleteObject();
			m_hfnt.DeleteObject();
			m_vfnt.DeleteObject();
		}

		// Recreate DC/bitmap for drawing into
		CDC* pDC = GetDC();
		m_pdc = new CDC();
		m_pdc->CreateCompatibleDC(pDC);
		m_bm.CreateCompatibleBitmap(pDC, rct.Width(), rct.Height());
		m_pdc->SelectObject(&m_bm);

		// Set up text including creating the font
		LOGFONT lf;
		memset(&lf, '\0', sizeof(lf));
		lf.lfHeight = 12;
		strcpy(lf.lfFaceName, "Tahoma");   // Simple font face for small digits
		m_hfnt.CreateFontIndirect(&lf);
		lf.lfEscapement = 900;             // text at 90 degrees
		m_vfnt.CreateFontIndirect(&lf);

		m_pdc->SetBkMode(TRANSPARENT);
		m_pdc->SetTextColor(m_axes);       // use same colour as axes for now
	}

	// Get adjusted colours
	COLORREF *pc = new COLORREF[m_col.size()];
	for (int ii = 0; ii < m_col.size(); ++ii)
		pc[ii] = add_contrast(m_col[ii], m_back);

	// Draw background
	m_pdc->FillSolidRect(0, 0, rct.Width(), rct.Height(), m_back);
	draw_axes();

	int width = m_rct.Width() - m_left - m_right;     // width of the actual graph area
	int height = m_rct.Height() - m_top - m_bottom;   // height of the graph
	int ypos = m_rct.Height() - m_bottom;             // bottom of the graph (dist. from rect top)

	// Draw bars of graph
	ASSERT(m_val.size() == m_col.size());
	int xpos = m_left;
	for (int ii = 0; ii < m_val.size(); ++ii)
	{
		int next_xpos = m_left + (width*(ii+1))/m_val.size();
		int cy = int((m_val[ii]*height)/m_max);

		m_pdc->FillSolidRect(xpos, ypos - cy, next_xpos - xpos, cy, pc[ii]);        // draw bar for this byte value
		//m_pdc->FillSolidRect(xpos, ypos - cy, next_xpos - xpos,  1, RGB(0, 0, 0));  // add black tip to all bars

		// Add ticks
		if (ii % 16 == 0)
		{
			int siz;
			if (ii %64 == 0)
				siz = 4;
			else
				siz = 2;
			m_pdc->FillSolidRect(xpos - 1, ypos + 1, 1, siz, m_axes);
		}
		xpos = next_xpos;
	}
	m_pdc->FillSolidRect(xpos - 1, ypos + 1, 1, 4, m_axes);   // Right end tick
	delete[] pc;

	Invalidate();
}

void CSimpleGraph::draw_axes()
{
	int width = m_rct.Width() - m_left - m_right;
	int height = m_rct.Height() - m_top - m_bottom;

	m_pdc->FillSolidRect(m_left - 1, m_top - 1, 1, height + 1, m_axes);      // vert axis

	// Calculate tick interval, units etc for the vertical axis
	int max;                         // scaled max value
	CString ustr;                    // scaling unit
	if (m_max >= 2000000000)
	{
		max = int(m_max/1000000000);
		ustr = "Billions";
	}
	else if (m_max >= 2000000)
	{
		max = int(m_max/1000000);
		ustr = "Millions";
	}
	else if (m_max >= 2000)
	{
		max = int(m_max/1000);
		ustr = "Thousands";
	}
	else
	{
		max = int(m_max);
		ustr = "";
	}

	int max_ticks = height/20;              // leave at least 20 pixels between ticks
	int min_spacing = max_ticks == 0 ? max : max/max_ticks + 1;    // min tick distance in graph units

	char buf[10];
	sprintf(buf, "%d", min_spacing);
	double mag = pow(10.0, double(strlen(buf) - 1));   // max magnitude
	double vv = min_spacing/mag;

	int tick;                              // how far apart are ticks in scaled units
	if (vv > 5.0)
		tick = 10 * int(mag);
	else if (vv > 2.0)
		tick = 5 * int(mag);
	else if (vv > 1.0)
		tick = 2 * int(mag);
	else
		tick = int(mag);

	CRect rct;
	m_pdc->SelectObject(&m_hfnt);
	for (int tt = 0; tt < max; tt += tick)
	{
		int ypos = m_rct.Height() - m_bottom - (tt*height)/max;    // xxx is this right as max is rounded??
		m_pdc->FillSolidRect(m_left - 3, ypos, 3, 1, m_axes);

		// Draw units next to tick
		//if (tt < (max*7)/8)        // don't draw near the top so we have room for the "Show" button
		{
			sprintf(buf, "%d", tt);
			rct.left = 0;
			rct.right = 28;
			rct.bottom = ypos + 7;
			rct.top = rct.bottom - 20;
			m_pdc->DrawText(buf, strlen(buf), rct, DT_SINGLELINE | DT_BOTTOM | DT_RIGHT | DT_NOCLIP);
			//m_pdc->ExtTextOut(14, ypos-7, 0, NULL, buf, strlen(buf), NULL);
		}
	}

	// Vertical unit scaling text
	m_pdc->SelectObject(&m_vfnt);
	m_pdc->ExtTextOutA(0, m_rct.Height() - m_bottom, 0, NULL, ustr, strlen(ustr), NULL);

	// Draw horizontal axis
	m_pdc->FillSolidRect(m_left - 1, m_rct.Height() - m_bottom, width + 1, 1, m_axes);
}

void CSimpleGraph::track_mouse(unsigned long flag)
{
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.dwFlags = flag;
	tme.dwHoverTime = 0;
	tme.hwndTrack = m_hWnd;

	VERIFY(::_TrackMouseEvent(&tme));
}

int CSimpleGraph::get_bar(CPoint pt)
{
	++pt.x;  // mouse posn seems to be out by 1 pixel
	if (pt.y <= m_top || pt.y >= m_rct.Height() - m_bottom ||
		pt.x < m_left || pt.x > m_rct.Width() - m_right)
	{
		// Outside graph area so return
		return -1;
	}

	int retval = ((pt.x - m_left)*m_val.size())/(m_rct.Width() - m_left - m_right);
	if (retval >= m_val.size())
		retval = m_val.size() - 1;

	return retval;
}

BEGIN_MESSAGE_MAP(CSimpleGraph, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

// CSimpleGraph message handlers
int CSimpleGraph::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Fix the height adjustment for current DPI (Windows default is 96 dots/inch)
	m_heightAdjust = (int)(m_heightAdjust * GetDC()->GetDeviceCaps(LOGPIXELSY) / 96.0);
	return 0;
}

void CSimpleGraph::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rct;
	GetClientRect(rct);

	// Just BLT the current graph into the display
	dc.BitBlt(rct.left, rct.top, rct.Width(), rct.Height(), m_pdc, 0, 0, SRCCOPY);
}

void CSimpleGraph::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	update();
}

BOOL CSimpleGraph::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint pt;
	::GetCursorPos(&pt);            // get mouse location (screen coords)
	ScreenToClient(&pt);
	if (get_bar(pt) > -1)
		::SetCursor(theApp.LoadCursor(IDC_INFO));  // show info cursor if over the graph
	else
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

void CSimpleGraph::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd::OnMouseMove(nFlags, point);

	// Check for hover if no tip window is visible
	if (!m_tip.IsWindowVisible())
		track_mouse(TME_HOVER);
	else if (get_bar(point) != m_bar && !m_tip.FadingOut())
		m_tip.Hide(300);
}

LRESULT CSimpleGraph::OnMouseHover(WPARAM, LPARAM lp)
{
	CPoint pt(LOWORD(lp), HIWORD(lp));  // client window coords

	CHexEditView *pv = GetView();
	ASSERT(pv != NULL);

	m_bar = get_bar(pt);
	if (m_bar > -1 && !m_tip.IsWindowVisible())
	{
		// Update the tip info
		m_tip.Clear();
		CString ss;
		if (theApp.hex_ucase_)
			ss.Format("Byte: %d [%02.2Xh] %s", m_bar, m_bar, pv->DescChar(m_bar));
		else
			ss.Format("Byte: %d [%02.2xh] %s", m_bar, m_bar, pv->DescChar(m_bar));
		m_tip.AddString(ss);

		char buf[32];                   // used with sprintf (CString::Format can't handle __int64)
		sprintf(buf, "%I64d", m_val[m_bar]);
		ss = buf;
		AddCommas(ss);
		m_tip.AddString("Count: " +ss);

		// Work out the tip window display position and move the tip window there
		CPoint tip_pt = pt + CSize(16, 16);
		ClientToScreen(&tip_pt);
		m_tip.Move(tip_pt, false);

		m_tip.Show();
		track_mouse(TME_LEAVE);
		return 0;  // return 0 to say we processed it
	}

	return 1;
}

LRESULT CSimpleGraph::OnMouseLeave(WPARAM, LPARAM lp)
{
	// Make sure we hide the tip window when the mouse leave the window (no more move messages are received)
	m_tip.Hide(300);
	return 0;
}
