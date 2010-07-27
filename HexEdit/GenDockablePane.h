// GenDockablePane.h header file for CGenDockablePane

#pragma once

#include "Resource.h"

class CGenDockablePane : public CDockablePane
{
	DECLARE_DYNAMIC(CGenDockablePane)

public:
	CGenDockablePane();
	virtual ~CGenDockablePane();

	CWnd *m_pNestedWnd;    // the window nested within (typically a dialog)

	// Overrides
	// This stops buttons being disabled because they have no command handler
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) { }

	void InitialUpdate(CWnd *pWnd);

	void ShowAndUnroll();
	void Float();
	void Hide() { ShowPane(FALSE, FALSE, FALSE); }
	void Toggle() { if (IsWindowVisible()) Hide(); else ShowAndUnroll(); }

	// Pane is floating in its own window (buy may not be visible)
	bool IsFloating() { return !IsDocked() && !IsTabbed(); }

	CSize GetDefaultSize();
	CSize GetFrameSize();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()

private:
	void AdjustLayout();
};
