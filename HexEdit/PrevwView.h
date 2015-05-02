// PrevwView.h - interface for CPrevwView

#ifndef PREVWVIEW_INCLUDED_ 
#define PREVWVIEW_INCLUDED_  1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HexEditDoc.h"
#include "HexEditView.h"

class CPrevwView : public CView
{
	friend CHexEditView;
	DECLARE_DYNCREATE(CPrevwView)

protected:
	CPrevwView();           // protected constructor used by dynamic creation
	virtual ~CPrevwView();

// Attributes
public:
	CHexEditView *phev_;                     // Ptr to sister hex view

public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
	{
		// If preview view can't handle a command try "owner" hex view
		if (CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
		else if (phev_ != NULL)
			return phev_->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view

#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	DECLARE_MESSAGE_MAP()

#ifndef _DEBUG
	inline
#endif
	CHexEditDoc *GetDocument()
	{
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHexEditDoc)));
		return (CHexEditDoc*)m_pDocument;
	}

private:
	double zoom_;               // Current zoom level where 1.0 means 1 bitmap pixel displays as 1 screen pixel
	CPoint pos_;                // Where is the current client rect is the top left of the bitmap (coords may be negative)

	bool mouse_down_;           // Is the left mouse button currently down?

	void draw_bitmap(CDC* pDC);
	void validate_display();
};
#endif  // PREVWVIEW_INCLUDED_


