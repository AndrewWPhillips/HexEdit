///////////////////////////////////////////////////////////////////////////
//
// GridCellDateTime.cpp: implementation of the CGridCellDateTime class.
//
// Provides the implementation for a datetime picker cell type of the
// grid control.
//
// Written by Podsypalnikov Eugen 15 Mar 2001
// Modified:
//    31 May 2001  Fixed m_cTime bug (Chris Maunder)
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "../GridCtrl_src/GridCtrl.h"
#include "../GridCtrl_src/GridCell.h"
#include "GridCellDateTime.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CGridCellDateTime

IMPLEMENT_DYNCREATE(CGridCellDateTime, CGridCell)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGridCellDateTime::CGridCellDateTime() : CGridCell()
{
	m_odt = COleDateTime::GetCurrentTime();
}

BOOL CGridCellDateTime::Edit(int nRow, int nCol, CRect rect, CPoint /* point 
*/,
							 UINT nID, UINT nChar)
{
	m_bEditing = TRUE;
	rect.InflateRect(0,2,0,2);

	// CInPlaceDateTime auto-deletes itself
	m_pEditWnd = new CInPlaceDateTime(GetGrid(), rect,
		nID, nRow, nCol, GetTextClr(), GetBackClr(), nChar, m_format, this);
	return TRUE;
}

CWnd* CGridCellDateTime::GetEditWnd() const
{
	return m_pEditWnd;
}

void CGridCellDateTime::EndEdit()
{
	if (m_pEditWnd) ((CInPlaceDateTime*)m_pEditWnd)->EndEdit();
}

void CGridCellDateTime::Init(COleDateTime odt, LPCTSTR format)
{
	m_odt = odt;
	m_format = "%c";            // default to short date + time for locale
	CString strFormat = format;
	COleDateTime tmp(3333, 11, 22, 00, 44, 55);  // used for testing formatted dates
	CString strDate;            // temp date string

	// First check if it contains at least one format spec but not any disallowed specs
	if (strFormat.Find('%') != -1 && strFormat.FindOneOf("aAbBjuwWzZ") == -1)
	{
		// Remove any allowed specs that may create alphabetic chars (AM/PM indicator)
		strFormat.Replace("%p", "");
		strFormat.Replace("%c", "");
		strFormat.Replace("%X", "");
		strDate = tmp.Format(strFormat);

		// Now if there are any alphas left they are caused by disallowed format specs (such
		// as %#c) or there are literals characters that we don't like (would need quoting)
		if (strDate.FindOneOf("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == -1)
			m_format = format;
	}

	// This is nec. in case the user presses Esc key in which case the text saved here is compared
	// with initial edit text to see if anything has changed (and if GVN_ENDLABELEDIT mesg needed).
	SetText(m_odt.Format(m_format));

	// Now convert strftime format string to date-time control format string
	strDate = tmp.Format(m_format);
	strDate.Replace("AM", "tt");
	strDate.Replace("am", "tt");
	strDate.Replace("A", "tt");
	strDate.Replace("a", "tt");
	strDate.Replace("12", "hh");    // hour 00 may appear as 12
	strDate.Replace('3', 'y');
	strDate.Replace('1', 'M');
	strDate.Replace('2', 'd');
	strDate.Replace('0', 'h');
	strDate.Replace('4', 'm');
	strDate.Replace('5', 's');
	m_format = strDate;
}

/////////////////////////////////////////////////////////////////////////////
// CInPlaceDateTime

CInPlaceDateTime::CInPlaceDateTime(CWnd* pParent, CRect& rect, UINT nID,
								   int nRow, int nColumn,
								   COLORREF crFore, COLORREF crBack,
								   UINT nFirstChar, LPCTSTR format,
								   CGridCellDateTime *pcell)
{
	m_nRow          = nRow;
	m_nCol          = nColumn;
	m_crForeClr     = crFore;
	m_crBackClr     = crBack;
	m_nLastChar     = 0;
	m_bExitOnArrows = FALSE;
	m_pcell         = pcell;

	DWORD dwStl = WS_BORDER|WS_VISIBLE|WS_CHILD|DTS_SHORTDATEFORMAT|DTS_UPDOWN;

	if (!Create(dwStl, rect, pParent, nID)) {
		return;
	}

	DateTime_SetFormat(m_hWnd, (LPARAM)format);
	SetTime(pcell->GetTime());

	SetFont(pParent->GetFont());
	SetFocus();

	switch (nFirstChar)
	{
		case VK_LBUTTON:
		case VK_RETURN: return;
		case VK_BACK:   break;
		case VK_DOWN:
		case VK_UP:
		case VK_RIGHT:
		case VK_LEFT:
		case VK_NEXT:
		case VK_PRIOR:
		case VK_HOME:
		case VK_END:    return;
		default:        break;
	}
	SendMessage(WM_CHAR, nFirstChar);
}

void CInPlaceDateTime::EndEdit()
{
	CString str;
	if (::IsWindow(m_hWnd))
	{
		GetWindowText(str);
		COleDateTime odt;
		GetTime(odt);
		m_pcell->SetTime(odt);
	}

	// Send Notification to parent
	GV_DISPINFO dispinfo;

	dispinfo.hdr.hwndFrom = GetSafeHwnd();
	dispinfo.hdr.idFrom   = GetDlgCtrlID();
	dispinfo.hdr.code     = GVN_ENDLABELEDIT;

	dispinfo.item.mask    = LVIF_TEXT|LVIF_PARAM;
	dispinfo.item.row     = m_nRow;
	dispinfo.item.col     = m_nCol;
	dispinfo.item.strText = str;
	dispinfo.item.lParam  = (LPARAM) m_nLastChar;

	CWnd* pOwner = GetOwner();
	if (IsWindow(pOwner->GetSafeHwnd())) {
		pOwner->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
	}

	// Close this window (PostNcDestroy will delete this)
	if (::IsWindow(m_hWnd)) {
		PostMessage(WM_CLOSE, 0, 0);
	}
}

void CInPlaceDateTime::PostNcDestroy()
{
	CDateTimeCtrl::PostNcDestroy();
	delete this;
}

BEGIN_MESSAGE_MAP(CInPlaceDateTime, CDateTimeCtrl)
	//{{AFX_MSG_MAP(CInPlaceDateTime)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_GETDLGCODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CInPlaceDateTime message handlers

void CInPlaceDateTime::OnKillFocus(CWnd* pNewWnd)
{
	CDateTimeCtrl::OnKillFocus(pNewWnd);

	if (GetSafeHwnd() == pNewWnd->GetSafeHwnd()) {
		return;
	}
	EndEdit();
}

UINT CInPlaceDateTime::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

void CInPlaceDateTime::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (( nChar == VK_PRIOR || nChar == VK_NEXT ||
		nChar == VK_DOWN  || nChar == VK_UP   ||
		nChar == VK_RIGHT || nChar == VK_LEFT) &&
		(m_bExitOnArrows  || GetKeyState(VK_CONTROL) < 0))
	{
		m_nLastChar = nChar;
		GetParent()->SetFocus();
		return;
	}

	CDateTimeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CInPlaceDateTime::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE)
	{
		m_nLastChar = nChar;
		GetParent()->SetFocus();    // This will destroy this window
		return;
	}

	CDateTimeCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
}

