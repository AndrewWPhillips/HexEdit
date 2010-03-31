// HexPaneDialog.cpp : implementation file
//

#include "stdafx.h"
#include "HexPaneDialog.h"

// CHexPaneFrameWnd
// This is a kludge to get acces to protected members of CPaneFrameWnd

class CHexPaneFrameWnd : public CPaneFrameWnd
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

// CHexPaneDialog

CBrush * CHexPaneDialog::m_pBrush = NULL;
COLORREF CHexPaneDialog::m_col = -1;

IMPLEMENT_DYNAMIC(CHexPaneDialog, CPaneDialog)

CHexPaneDialog::CHexPaneDialog()
{
}

BEGIN_MESSAGE_MAP(CHexPaneDialog, CPaneDialog)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// CHexPaneDialog message handlers

int CHexPaneDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPaneDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Get min size (bug in CPaneDialog base class)
	CRect rct;
	GetWindowRect(&rct);
	//m_sizeDefault = rct.Size();

	// Just make the minimimum size the size in the dialog editor
	SetMinSize(rct.Size());

	// Set up resizer control
	m_resizer.Create(GetSafeHwnd(), TRUE, 100, TRUE);
	m_resizer.SetInitialSize(rct.Size());

	return 0;
}

BOOL CHexPaneDialog::OnEraseBkgnd(CDC *pDC)
{
	CRect rct;
	GetClientRect(&rct);

	// Fill the background with a colour that matches the current BCG theme (and hence sometimes with the Windows Theme)
	pDC->FillSolidRect(rct, afxGlobalData.clrBarFace);
	return TRUE;
}

HBRUSH CHexPaneDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);                            // Make sure text has no background
//		return(HBRUSH) afxGlobalData.brWindow.GetSafeHandle();  // window colour
		if (m_pBrush == NULL || m_col != afxGlobalData.clrBarFace)
		{
			m_col = afxGlobalData.clrBarFace;
			if (m_pBrush != NULL)
				delete m_pBrush;
			m_pBrush = new CBrush(afxGlobalData.clrBarFace);
		}
        return (HBRUSH)*m_pBrush;
	}
	else
	{
		return CPaneDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}

void CHexPaneDialog::ShowAndUnroll()
{
	if (!IsWindowVisible())
		ShowPane(TRUE, FALSE, TRUE);

	CPaneFrameWnd * pFrm;
	if ((pFrm = DYNAMIC_DOWNCAST(CPaneFrameWnd, GetParentMiniFrame())) != NULL)
	{
		((CHexPaneFrameWnd *)pFrm)->Unroll();
	}

	SetFocus();
}

void CHexPaneDialog::Float()
{
	CRect rct;
	GetWindowRect(&rct);
	FloatPane(rct);
}
