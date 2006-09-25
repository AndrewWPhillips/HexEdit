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

IMPLEMENT_DYNCREATE(CChildFrame, CBCGMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CBCGMDIChildWnd)
        //{{AFX_MSG_MAP(CChildFrame)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
        ON_WM_SYSCOMMAND()
        ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
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
        CBCGMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
        CBCGMDIChildWnd::Dump(dc);
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

    if (!splitter_.CreateView(0, 0, RUNTIME_CLASS(CTabView), CSize(0, 0), pContext))
    {
        AfxMessageBox("Failed to create splitter view.");
        return FALSE;
    }
	ptv_ = (CTabView *)splitter_.GetPane(0, 0);
	ASSERT_KINDOF(CTabView, ptv_);

#if 0  // This is now done on a per file basis - so we have to do it in CHexEditView::OnInitialUpdate
	if (theApp.tree_view_ == 1)
	{
		// Add left column for DFFD (template) view
		VERIFY(splitter_.InsColumn(0, theApp.tree_width_, RUNTIME_CLASS(CDataFormatView), pContext));

        splitter_.SetActivePane(0, 1);   // make hex view active

        CDataFormatView *pdfv = (CDataFormatView *)splitter_.GetPane(0, 0);
        ASSERT_KINDOF(CDataFormatView, pdfv);
		ptv_->SetActiveView(0);   // view 0 is always hex view - make it active
        CHexEditView *phev = (CHexEditView *)ptv_->GetActiveView();
        ASSERT_KINDOF(CHexEditView, phev);

		ASSERT(splitter_.FindViewColumn(ptv_->GetSafeHwnd()) == 1);

        // Make sure dataformat view knows which hex view it is assoc. with
        pdfv->phev_ = phev;
        phev->pdfv_ = pdfv;
	}
#endif

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
        else if (pv->IsKindOf(RUNTIME_CLASS(CTabView)))
        {
			// Find the hex view (left-most tab)
			CTabView *ptv = (CTabView *)pv;
			ptv->SetActiveView(0);  // hex view is always left-most (index 0)
			ASSERT_KINDOF(CHexEditView, ptv->GetActiveView());
			return (CHexEditView *)ptv->GetActiveView();
        }
    }
    return NULL;
}

BOOL CChildFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
    // Bypass calling CBCGMDIChildWnd::LoadFrame which loads an icon
    BOOL bRtn = CBCGMDIChildWnd::LoadFrame( nIDResource, dwDefaultStyle, pParentWnd, pContext );

    return bRtn;
}

BOOL CChildFrame::DestroyWindow() 
{
	return CBCGMDIChildWnd::DestroyWindow();
}

// Handles control menu commands and system buttons (Minimize etc)
void CChildFrame::OnSysCommand(UINT nID, LONG lParam)
{
    CBCGMDIChildWnd::OnSysCommand(nID, lParam);

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
    LRESULT retval = CBCGMDIChildWnd::OnHelpHitTest(wParam, lParam);
    return retval;
}

void CChildFrame::OnClose() 
{
    // xxx not called if whole app shutdown!!!
	if (splitter_.m_hWnd != 0 && 
		splitter_.GetColumnCount() > 1 &&
		splitter_.GetPane(0, 0)->IsKindOf(RUNTIME_CLASS(CDataFormatView)))
    {
        // Save width of splitter column 1 (tree view)
        int dummy;                      // min width which we don't need
        splitter_.GetColumnInfo(0, theApp.tree_width_, dummy);
    }
	
    CBCGMDIChildWnd::OnClose();
}
