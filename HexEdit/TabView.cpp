// TabView.cpp : allows 2 nested (tabbed) views: CHexEditView & CDataFormatView
//
// Copyright (c) 1999-2000 by Andrew W. Phillips.
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
#include "ChildFrm.h"
#include "TabView.h"
#include "HexEditView.h"
#include "DataFormatView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabView

IMPLEMENT_DYNCREATE(CTabView, CBCGTabView)

CTabView::CTabView()
{
    init_ = false;
}

CTabView::~CTabView()
{
}


BEGIN_MESSAGE_MAP(CTabView, CBCGTabView)
	//{{AFX_MSG_MAP(CTabView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CTabView::OnSize(UINT nType, int cx, int cy)
{
    // This is necessary since we can get resize events before the tab view
    // is ready (before tab window has been created).
    // This caused crash in 2.6 if Window/New Window is used when window maximized.
    if (init_)
        CBCGTabView::OnSize(nType, cx, cy);
}

void CTabView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
    if (bActivate)
    {
        // Make sure hex view is active
        SetActiveView(0);

        // Get child frame
        CWnd *pp = GetParent();
        while (pp != NULL && !pp->IsKindOf(RUNTIME_CLASS(CChildFrame)))
            pp = pp->GetParent();
        ASSERT_KINDOF(CChildFrame, pp);

        //((CChildFrame *)pp)->MDIActivate();
        ((CChildFrame *)pp)->SetActiveView(GetActiveView());
        //GetActiveView()->SetFocus();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTabView drawing

void CTabView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CTabView diagnostics

#ifdef _DEBUG
void CTabView::AssertValid() const
{
	CBCGTabView::AssertValid();
}

void CTabView::Dump(CDumpContext& dc) const
{
	CBCGTabView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTabView message handlers

int CTabView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CBCGTabView::OnCreate(lpCreateStruct) == -1)
        return -1;

    init_ = true;

	// If there is only one tab (ie, hex view only) hide the tabs
	GetTabControl().HideSingleTab();

    //AddView(RUNTIME_CLASS (CHexEditView), _T("Raw Data (Hex) View"));
    AddView(RUNTIME_CLASS(CHexEditView), _T("Raw Data (Hex) View"), -1, (CCreateContext *)lpCreateStruct->lpCreateParams);

    // This will later be moved to depend on per file saved setting xxx
	if (theApp.tree_view_ == 2)
	{
		AddView(RUNTIME_CLASS (CDataFormatView), _T("Data Format (Tree) View"));
	    
		SetActiveView(1);
		CDataFormatView *pdfv = (CDataFormatView *)GetActiveView();
		ASSERT_KINDOF(CDataFormatView, pdfv);

		SetActiveView(0);
		CHexEditView *phev = (CHexEditView *)GetActiveView();
		ASSERT_KINDOF(CHexEditView, phev);

		// Make sure dataformat view knows which hex view it is assoc. with
		pdfv->psis_ = phev;
		phev->psis_ = pdfv;
	}

    return 0;
}

BOOL CTabView::OnEraseBkgnd(CDC* pDC) 
{
    return TRUE;
}
