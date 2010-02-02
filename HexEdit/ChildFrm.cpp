// ChildFrm.cpp : implementation of the CChildFrame class
//
// Copyright (c) 2003 by Andrew W. Phillips.
//
// No restrictions are placed on the noncommercial use of this code,
// as long as this text (from the above copyright notice to the
// disclaimer below) is preserved.
//
// This code may be redistributed as long as it remains unmodified
// and is not sold for profit without the author's written consent.
//
// This code, or any part of it, may not be used in any software that
// is sold for profit, without the author's written consent.
//
// DISCLAIMER: This file is provided "as is" with no expressed or
// implied warranty. The author accepts no liability for any damage
// or loss of business that this product may cause.
//

#include "stdafx.h"
#include "HexEdit.h"

#include "MainFrm.h"
#include "HexEditView.h"
#include "DataFormatView.h"
#include "AerialView.h"

#include "ChildFrm.h"
#include <afxpriv.h>            // for WM_HELPHITTEST

extern CHexEditApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
        //{{AFX_MSG_MAP(CChildFrame)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
        ON_WM_SYSCOMMAND()
        ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
        ON_WM_SETFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
}

CChildFrame::~CChildFrame()
{
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
        CMDIChildWndEx::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
        CMDIChildWndEx::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
    ASSERT(pContext != NULL && pContext->m_pNewViewClass != NULL);

    if (!splitter_.CreateStatic(this, 1, 3))
    {
        AfxMessageBox("Failed to create splitter.");
        return FALSE;
    }

	// We create with 3 columns then delete the last 2 so that max columns is 3.
	// This allows to dynamically add 2 splits (in static splitter)
    // The extra 2 split windows are for tree (template) view and aerial view (TBD)
	splitter_.DelColumn(2);
	splitter_.DelColumn(1);

    if (!splitter_.CreateView(0, 0, RUNTIME_CLASS(CHexTabView), CSize(0, 0), pContext))
    {
        AfxMessageBox("Failed to create splitter view.");
        return FALSE;
    }
	ptv_ = (CHexTabView *)splitter_.GetPane(0, 0);
	ASSERT_KINDOF(CHexTabView, ptv_);

	return TRUE;
}

// All child frame's will have exactly one CHexEditView (unless in print preview?).  This returns it.
CHexEditView *CChildFrame::GetHexEditView() const
{
    CView *pv = GetActiveView();
    if (pv != NULL)                         // May be NULL if print preview
    {
        if (pv->IsKindOf(RUNTIME_CLASS(CHexEditView)))
            return (CHexEditView *)pv;
        else if (pv->IsKindOf(RUNTIME_CLASS(CDataFormatView)))
            return ((CDataFormatView *)pv)->phev_;
        else if (pv->IsKindOf(RUNTIME_CLASS(CAerialView)))
            return ((CAerialView *)pv)->phev_;
        else if (pv->IsKindOf(RUNTIME_CLASS(CHexTabView)))
        {
			// Find the hex view (left-most tab)
			CHexTabView *ptv = (CHexTabView *)pv;
			ptv->SetActiveView(0);  // hex view is always left-most (index 0)
			ASSERT_KINDOF(CHexEditView, ptv->GetActiveView());
			return (CHexEditView *)ptv->GetActiveView();
        }
    }
    return NULL;
}

BOOL CChildFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
    // Bypass calling CMDIChildWndEx::LoadFrame which loads an icon
    BOOL bRtn = CMDIChildWndEx::LoadFrame( nIDResource, dwDefaultStyle, pParentWnd, pContext );

    return bRtn;
}

BOOL CChildFrame::DestroyWindow() 
{
	return CMDIChildWndEx::DestroyWindow();
}

// Handles control menu commands and system buttons (Minimize etc)
void CChildFrame::OnSysCommand(UINT nID, LONG lParam)
{
    CMDIChildWndEx::OnSysCommand(nID, lParam);

    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    nID &= 0xFFF0;
    if (nID == SC_MINIMIZE || nID == SC_RESTORE || nID == SC_MAXIMIZE ||
        nID == SC_NEXTWINDOW || nID == SC_PREVWINDOW || nID == SC_CLOSE)
    {
        if ((nID == SC_NEXTWINDOW || nID == SC_PREVWINDOW || nID == SC_CLOSE) &&
            aa->recording_ && aa->mac_.size() > 0 && (aa->mac_.back()).ktype == km_focus)
        {
            // Next win, prev. win, close win cause focus change which causes a km_focus
            // for a particular window to be stored.  On replay, we don't want to
            // change to this window before executing this command.
            aa->mac_.pop_back();
        }
        aa->SaveToMacro(km_childsys, nID);
    }
}

// Handles Shift-F1 help when clicked within the window (lParam contains
// point in high and low words).  At the moment nothing is done, so all
// help goes to the same place (HIDR_HEXEDTYPE).
LRESULT CChildFrame::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
    // Intercept call so we can check what happens in debugger xxx
    LRESULT retval = CMDIChildWndEx::OnHelpHitTest(wParam, lParam);
    return retval;
}

void CChildFrame::OnClose() 
{
    CMDIChildWndEx::OnClose();
}

void CChildFrame::OnSetFocus(CWnd* pOldWnd) 
{
    CMDIChildWndEx::OnSetFocus(pOldWnd);

	CHexEditView *pv = GetHexEditView();
	if (pv != NULL)
	{
		pv->show_prop();
		pv->show_calc();
		pv->show_pos();
	}
}
