#pragma once

// CHexPaneDialog

#include "ResizeCtrl.h"

class CHexPaneDialog : public CPaneDialog
{
	DECLARE_DYNAMIC(CHexPaneDialog)

public:
	CHexPaneDialog();
	virtual ~CHexPaneDialog() { }

	void ShowAndUnroll();
	void Float();
	void Hide() { ShowPane(FALSE, FALSE, FALSE); }
	BOOL Add(int ID, int left, int top, int width, int height) { return m_resizer.Add(ID, left, top, width, height); }

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

private:
	CResizeCtrl m_resizer;               // Used to move controls around when the window is resized
	static CBrush * m_pBrush;            // brush used for background
	static COLORREF m_col;               // colour used for background
};
