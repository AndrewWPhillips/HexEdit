// GenDockablePane.cpp : implementation file
//

#include "stdafx.h"
#include "GenDockablePane.h"

// CGenPaneFrameWnd
// This is a kludge to get access to protected members of CPaneFrameWnd

class CGenPaneFrameWnd : public CPaneFrameWnd
{
public:
	//BOOL IsRolled() { return m_bRolledUp; }
	void Unroll()
	{
		if (!m_bRolledUp) return;

		//// Save current mouse location
		//CPoint ptSaved;
		//GetCursorPos(&ptSaved);

		// Get frames window location
		CRect rct;
		GetWindowRect(rct);

		// Try to move the mouse over the "pin" button (right end of title bar)
		// (We need to move the mouse over the window so it stays unrolled and we may as well move
		// it over the "pin" button to draw the users attention to it and allow them to click it.)
		int distRight = 32;
		if (m_lstCaptionButtons.GetCount() > 0)
		{
			// The first (right most) button is normally the close button so skip its width
			POSITION pos = m_lstCaptionButtons.GetHeadPosition();
			CMFCCaptionButton* pBtn = (CMFCCaptionButton*) m_lstCaptionButtons.GetNext(pos);
			distRight = pBtn->GetRect().Width() + 16;
		}
		// Set mouse position over the window so OnCheckRollState() unrolls it
		SetCursorPos(rct.left + rct.Width() - distRight, rct.top + rct.Height()/2);
		OnCheckRollState();

		//// Restore mouse position
		//SetCursorPos(ptSaved.x, ptSaved.y);
	}
};

// CGenDockablePane

IMPLEMENT_DYNAMIC(CGenDockablePane, CDockablePane)

CGenDockablePane::CGenDockablePane() : m_pNestedWnd(NULL)
{
}

CGenDockablePane::~CGenDockablePane()
{
}

BEGIN_MESSAGE_MAP(CGenDockablePane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

// Window manipulation

void CGenDockablePane::ShowAndUnroll()
{
	if (!IsWindowVisible())
		ShowPane(TRUE, FALSE, TRUE);

	CPaneFrameWnd * pFrm;
	if ((pFrm = DYNAMIC_DOWNCAST(CPaneFrameWnd, GetParentMiniFrame())) != NULL)
	{
		((CGenPaneFrameWnd *)pFrm)->Unroll();
	}

	SetFocus();
}

void CGenDockablePane::Float()
{
	CRect rct;
	GetWindowRect(&rct);
	FloatPane(rct);
}

// Message handlers

int CGenDockablePane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

void CGenDockablePane::InitialUpdate(CWnd *pWnd)
{
	m_pNestedWnd = pWnd;

	CRect rct;
	m_pNestedWnd->GetWindowRect(&rct);
	SetMinSize(rct.Size());
	//m_recentDockInfo.m_recentSliderInfo.m_rectDockedRect = rct;

	AdjustLayout();
}


void CGenDockablePane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
    if (m_pNestedWnd != NULL)
		AdjustLayout();
}

void CGenDockablePane::OnSetFocus(CWnd* pOldWnd)
{
	ASSERT(m_pNestedWnd != NULL);
	CDockablePane::OnSetFocus(pOldWnd);
	m_pNestedWnd->SetFocus();
}

// Private methods

void CGenDockablePane::AdjustLayout()
{
	ASSERT(m_pNestedWnd != NULL);
	if (GetSafeHwnd() == NULL)
		return;

	CRect rct;
	GetClientRect(rct);
	m_pNestedWnd->SetWindowPos(NULL,
		                 rct.left, rct.top,
						 rct.Width(), rct.Height(),
		                 SWP_NOACTIVATE | SWP_NOZORDER);
}

