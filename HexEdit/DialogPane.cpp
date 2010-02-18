// DialogPane.cpp : MFC9 dockable pane (CDockablePane) that can embed a CDialog
//
// Copyright (c) 2010 by Andrew W. Phillips.
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
#include "DialogPane.h"

// CDialogPane

IMPLEMENT_DYNAMIC(CDialogPane, CDockablePane)

BEGIN_MESSAGE_MAP(CDialogPane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

// CDialogPane message handlers

int CDialogPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_pdlg == NULL || !m_pdlg->Create(this, m_nID, CBRS_ALIGN_ANY, m_nID))
	{
		TRACE0("Failed to create dialog bar\n");
		return -1;      // fail to create
	}

	resizer_.Create(m_pdlg->GetSafeHwnd(), TRUE, 100);
	resizer_.SetInitialSize(m_pdlg->m_sizeDefault);

	AdjustLayout();
	return 0;
}

void CDialogPane::OnSize(UINT nType, int cx, int cy)
{
	TRACE("xxx AAA dialog pane WM_SIZE event\r\n");
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CDialogPane::OnSetFocus(CWnd* pOldWnd)
{
	TRACE("xxx AAA dialog pane WM_SETFOCUS event\r\n");
	CDockablePane::OnSetFocus(pOldWnd);

	if (m_pdlg == NULL) return;
	m_pdlg->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// public methods

void CDialogPane::AdjustLayout()
{
	if (m_pdlg == NULL) return;
	ASSERT(GetSafeHwnd() != NULL && m_pdlg->GetSafeHwnd() != NULL);

	CRect rct;
	GetClientRect(&rct);
	m_pdlg->SetWindowPos(NULL, rct.left, rct.top, rct.Width(), rct.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	TRACE("xxx AAA dialog pane set window pos\r\n");
}
