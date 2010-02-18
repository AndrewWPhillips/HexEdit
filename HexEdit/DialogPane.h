// Copyright (c) 2008 by Andrew W. Phillips.
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

#pragma once

// CDialogPane

#include "ResizeCtrl.h"

class CDialogPane : public CDockablePane
{
	DECLARE_DYNAMIC(CDialogPane)

public:
	CDialogPane(CDialogBar * pdlg = NULL) : m_pdlg(pdlg) { }
	virtual ~CDialogPane() { }

	void SetDialogBar(CDialogBar * pdlg) { m_pdlg = pdlg; }
	BOOL Add(int ID, int left, int top, int width, int height) { return resizer_.Add(ID, left, top, width, height); }
	void AdjustLayout();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()

private:
	CDialogBar * m_pdlg;                // Holds the dialog that is inserted into the pane
	CResizeCtrl resizer_;               // Used to move controls around when the window is resized
};
