// InPlaceEdit.cpp : implementation file
//
// Adapted by Chris Maunder <cmaunder@mail.com>
// Copyright (c) 1998-2001. All Rights Reserved.
//
// The code contained in this file is based on the original
// CInPlaceEdit from http://www.codeguru.com/listview/edit_subitems.shtml
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name and all copyright 
// notices remains intact. 
//
// An email letting me know how you are using it would be nice as well. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
// For use with CGridCtrl v2.10+
//
// History:
//         10 May 1998  Uses GVN_ notifications instead of LVN_,
//                      Sends notification messages to the parent, 
//                      instead of the parent's parent.
//         15 May 1998  There was a problem when editing with the in-place editor, 
//                      there arises a general protection fault in user.exe, with a 
//                      few qualifications:
//                         (1) This only happens with owner-drawn buttons;
//                         (2) This only happens in Win95
//                         (3) This only happens if the handler for the button does not 
//                             create a new window (even an AfxMessageBox will avoid the 
//                             crash)
//                         (4) This will not happen if Spy++ is running.
//                      PreTranslateMessage was added to route messages correctly.
//                      (Matt Weagle found and fixed this problem)
//         26 Jul 1998  Removed the ES_MULTILINE style - that fixed a few probs!
//          6 Aug 1998  Added nID to the constructor param list
//          6 Sep 1998  Space no longer clears selection when starting edit (Franco Bez)
//         10 Apr 1999  Enter, Tab and Esc key prob fixed (Koay Kah Hoe)
//                      Workaround for bizzare "shrinking window" problem in CE
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TCHAR.h"
#include "InPlaceEdit.h"

#include "GridCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit

IMPLEMENT_DYNAMIC(CInPlaceEdit, CEdit)

CInPlaceEdit::CInPlaceEdit(CWnd* pParent, CRect& rect, DWORD dwStyle, UINT nID,
#if  _MSC_VER >= 1300
                           int nRow, int nColumn, CStringW sInitText, 
#else
                           int nRow, int nColumn, CString sInitText, 
#endif
                           UINT nFirstChar)
{
    m_sInitText     = sInitText;
    m_nRow          = nRow;
    m_nColumn       = nColumn;
    m_nLastChar     = 0; 
    m_bExitOnArrows = (nFirstChar != VK_LBUTTON);    // If mouse click brought us here,
                                                     // then no exit on arrows

    m_Rect = rect;  // For bizarre CE bug.

    DWORD dwEditStyle = WS_BORDER|WS_CHILD|WS_VISIBLE| ES_AUTOHSCROLL //|ES_MULTILINE
        | dwStyle;
    if (!Create(dwEditStyle, rect, pParent, nID)) return;

//#if  _MSC_VER >= 1300
//	// This effectively does the same as CEdit::Create but uses CreateWindowExW not CreateWindowEx.
//	HWND hw = ::CreateWindowExW(0, L"EDIT", NULL, dwEditStyle,
//	                           rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
//							   pParent->GetSafeHwnd(), (HMENU)(UINT_PTR)nID, AfxGetInstanceHandle(), NULL);
//	if (hw == (HWND)0 || !SubclassWindow(hw))
//		return;
//
//#endif

    SetFont(pParent->GetFont());

#if  _MSC_VER >= 1300
	::CallWindowProcW(*GetSuperWndProcAddr(), m_hWnd, WM_SETTEXT, 0, (LPARAM)(LPCWSTR)sInitText);
#else
    SetWindowText(sInitText);
#endif
	int len = sInitText.GetLength();
    SetFocus();

    switch (nFirstChar){
        case VK_LBUTTON: 
        case VK_RETURN:   SetSel(len, -1); return;
        case VK_BACK:     SetSel(len, -1); break;
        case VK_TAB:
        case VK_DOWN: 
        case VK_UP:
        case VK_RIGHT:
        case VK_LEFT:
        case VK_NEXT:
        case VK_PRIOR: 
        case VK_HOME:
        case VK_SPACE:
        case VK_END:      SetSel(0,-1); return;
        default:          SetSel(0,-1);
    }

    // Added by KiteFly. When entering DBCS chars into cells the first char was being lost
    // SenMessage changed to PostMessage (John Lagerquist)
    if( nFirstChar < 0x80)
        PostMessage(WM_CHAR, nFirstChar);
    else
        PostMessage(WM_IME_CHAR, nFirstChar);
}

CInPlaceEdit::~CInPlaceEdit()
{
}

BEGIN_MESSAGE_MAP(CInPlaceEdit, CEdit)
    //{{AFX_MSG_MAP(CInPlaceEdit)
    ON_WM_KILLFOCUS()
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    ON_WM_GETDLGCODE()
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit message handlers

// If an arrow key (or associated) is pressed, then exit if
//  a) The Ctrl key was down, or
//  b) m_bExitOnArrows == TRUE
void CInPlaceEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    if ((nChar == VK_PRIOR || nChar == VK_NEXT ||
        nChar == VK_DOWN  || nChar == VK_UP   ||
        nChar == VK_RIGHT || nChar == VK_LEFT) &&
        (m_bExitOnArrows || GetKeyState(VK_CONTROL) < 0))
    {
        m_nLastChar = nChar;
        GetParent()->SetFocus();
        return;
    }

    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

// As soon as this edit loses focus, kill it.
void CInPlaceEdit::OnKillFocus(CWnd* pNewWnd)
{
    CEdit::OnKillFocus(pNewWnd);
    EndEdit();
}

void CInPlaceEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_TAB || nChar == VK_RETURN)
    {
        m_nLastChar = nChar;
        GetParent()->SetFocus();    // This will destroy this window
        return;
    }
    if (nChar == VK_ESCAPE) 
    {
#if  _MSC_VER >= 1300
		::CallWindowProcW(*GetSuperWndProcAddr(), m_hWnd, WM_SETTEXT, 0, (LPARAM)(LPCWSTR)m_sInitText);
#else
        SetWindowText(m_sInitText);    // restore previous text
#endif
        m_nLastChar = nChar;
        GetParent()->SetFocus();
        return;
    }

    CEdit::OnChar(nChar, nRepCnt, nFlags);

    // Resize edit control if needed

    // Get text extent
    CString str;
    GetWindowText( str );

    // add some extra buffer
    str += _T("  ");

    CWindowDC dc(this);
    CFont *pFontDC = dc.SelectObject(GetFont());
    CSize size = dc.GetTextExtent( str );
    dc.SelectObject( pFontDC );

    // Get client rect
    CRect ParentRect;
    GetParent()->GetClientRect( &ParentRect );

    // Check whether control needs to be resized
    // and whether there is space to grow
    if (size.cx > m_Rect.Width())
    {
        if( size.cx + m_Rect.left < ParentRect.right )
            m_Rect.right = m_Rect.left + size.cx;
        else
            m_Rect.right = ParentRect.right;
        MoveWindow( &m_Rect );
    }
}

UINT CInPlaceEdit::OnGetDlgCode() 
{
    return DLGC_WANTALLKEYS;
}

////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit overrides

// Stoopid win95 accelerator key problem workaround - Matt Weagle.
BOOL CInPlaceEdit::PreTranslateMessage(MSG* pMsg) 
{
    // Catch the Alt key so we don't choke if focus is going to an owner drawn button
    if (pMsg->message == WM_SYSCHAR)
        return TRUE;

    return CWnd::PreTranslateMessage(pMsg);
}

// Auto delete
void CInPlaceEdit::PostNcDestroy() 
{
    CEdit::PostNcDestroy();

    delete this;
}

////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit implementation

void CInPlaceEdit::EndEdit()
{
    // EFW - BUG FIX - Clicking on a grid scroll bar in a derived class
    // that validates input can cause this to get called multiple times
    // causing assertions because the edit control goes away the first time.
    static BOOL bAlreadyEnding = FALSE;

    if(bAlreadyEnding)
        return;

    bAlreadyEnding = TRUE;

    // Send Notification to parent
    GV_DISPINFO dispinfo;

    dispinfo.hdr.hwndFrom = GetSafeHwnd();
    dispinfo.hdr.idFrom   = GetDlgCtrlID();
    dispinfo.hdr.code     = GVN_ENDLABELEDIT;

    dispinfo.item.mask    = LVIF_TEXT|LVIF_PARAM;
    dispinfo.item.row     = m_nRow;
    dispinfo.item.col     = m_nColumn;
#if _MSC_VER >= 1300
	wchar_t buf[2048];
	::CallWindowProcW(*GetSuperWndProcAddr(), m_hWnd, WM_GETTEXT, 2048, (LPARAM)buf);
	dispinfo.item.strText = CStringW(buf);
#else
    GetWindowText(dispinfo.item.strText);
#endif
    dispinfo.item.lParam  = (LPARAM) m_nLastChar;

    CWnd* pOwner = GetOwner();
    if (pOwner)
        pOwner->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo );

    // Close this window (PostNcDestroy will delete this)
    if (IsWindow(GetSafeHwnd()))
        SendMessage(WM_CLOSE, 0, 0);
    bAlreadyEnding = FALSE;
}
