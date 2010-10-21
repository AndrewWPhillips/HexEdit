// AerialView.h
//
// Copyright (c) 2008-2010 by Andrew W. Phillips.
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

#ifndef DATAFORMATVIEW_INCLUDED_ 
#define DATAFORMATVIEW_INCLUDED_  1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CAerialView view
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "timer.h"  // A timer is used to dynamically adjust ant speed

class CAerialView : public CView
{
	friend CHexEditView;
	DECLARE_DYNCREATE(CAerialView)

protected:
	CAerialView();           // protected constructor used by dynamic creation
	virtual ~CAerialView();

// Attributes
public:
	CHexEditView *phev_;                     // Ptr to sister hex view

public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
	{
		// If aerial view can't handle it try "owner" hex view
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

	void SetScroll(int newpos);
	void SetScroll(FILE_ADDRESS newpos) { SetScroll(int(newpos/(GetDocument()->GetBpe() * cols_))); }
	void ShowPos(FILE_ADDRESS pos);
	void StoreOptions(CHexFileList *pfl, int idx);
	void InvalidateRange(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr);

	void StartTimer() { if (!timer_id_) VERIFY(timer_id_ = SetTimer(1, timer_msecs_, NULL)); }
	void StopTimer() { if (timer_id_) KillTimer(timer_id_); timer_id_ = 0; }
	unsigned long DispState() { return disp_state_; }

protected:
	virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);

	// message handlers
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseHover(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnMouseLeave(WPARAM wp, LPARAM lp);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewTest();

	afx_msg void OnIndSelection() { disp_.draw_bdr_sel = !disp_.draw_bdr_sel; Invalidate(); }
	afx_msg void OnIndMark() { disp_.draw_bdr_mark = !disp_.draw_bdr_mark; Invalidate(); }
	afx_msg void OnIndHighlights() { disp_.draw_bdr_hl = !disp_.draw_bdr_hl; Invalidate(); }
	afx_msg void OnIndSearch() { disp_.draw_bdr_search = !disp_.draw_bdr_search; Invalidate(); }
	afx_msg void OnIndBookmarks() { disp_.draw_bdr_bm = !disp_.draw_bdr_bm; Invalidate(); }
	afx_msg void OnAntSelection() { disp_.draw_ants_sel = !disp_.draw_ants_sel; Invalidate(); }
	afx_msg void OnAntMark() { disp_.draw_ants_mark = !disp_.draw_ants_mark; Invalidate(); }
	afx_msg void OnAntHighlights() { disp_.draw_ants_hl = !disp_.draw_ants_hl; Invalidate(); }
	afx_msg void OnAntSearch() { disp_.draw_ants_search = !disp_.draw_ants_search; Invalidate(); }
	afx_msg void OnAntBookmarks() { disp_.draw_ants_bm = !disp_.draw_ants_bm; Invalidate(); }
	afx_msg void OnUpdateIndSelection(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_sel); }
	afx_msg void OnUpdateIndMark(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_mark); }
	afx_msg void OnUpdateIndHighlights(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_hl); }
	afx_msg void OnUpdateIndSearch(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_search); }
	afx_msg void OnUpdateIndBookmarks(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_bm); }
	afx_msg void OnUpdateAntSelection(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_sel); }
	afx_msg void OnUpdateAntMark(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_mark); }
	afx_msg void OnUpdateAntHighlights(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_hl); }
	afx_msg void OnUpdateAntSearch(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_search); }
	afx_msg void OnUpdateAntBookmarks(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_bm); }
	afx_msg void OnZoom1() { set_zoom(1); }
	afx_msg void OnZoom2() { set_zoom(2); }
	afx_msg void OnZoom3() { set_zoom(3); }
	afx_msg void OnZoom4() { set_zoom(4); }
	afx_msg void OnZoom5() { set_zoom(5); }
	afx_msg void OnZoom6() { set_zoom(6); }
	afx_msg void OnZoom7() { set_zoom(7); }
	afx_msg void OnZoom8() { set_zoom(8); }
	afx_msg void OnZoom9() { set_zoom(9); }
	afx_msg void OnZoom10() { set_zoom(10); }
	afx_msg void OnZoom11() { set_zoom(11); }
	afx_msg void OnZoom12() { set_zoom(12); }
	afx_msg void OnZoom13() { set_zoom(13); }
	afx_msg void OnZoom14() { set_zoom(14); }
	afx_msg void OnZoom15() { set_zoom(15); }
	afx_msg void OnZoom16() { set_zoom(16); }
	afx_msg void OnUpdateZoom1(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 1); }
	afx_msg void OnUpdateZoom2(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 2); }
	afx_msg void OnUpdateZoom3(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 3); }
	afx_msg void OnUpdateZoom4(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 4); }
	afx_msg void OnUpdateZoom5(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 5); }
	afx_msg void OnUpdateZoom6(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 6); }
	afx_msg void OnUpdateZoom7(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 7); }
	afx_msg void OnUpdateZoom8(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 8); }
	afx_msg void OnUpdateZoom9(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 9); }
	afx_msg void OnUpdateZoom10(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 10); }
	afx_msg void OnUpdateZoom11(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 11); }
	afx_msg void OnUpdateZoom12(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 12); }
	afx_msg void OnUpdateZoom13(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 13); }
	afx_msg void OnUpdateZoom14(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 14); }
	afx_msg void OnUpdateZoom15(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 15); }
	afx_msg void OnUpdateZoom16(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 16); }

	afx_msg void OnUpdateDisable(CCmdUI* pCmdUI) { pCmdUI->Enable(FALSE); }
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
	void invalidate_addr_range(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, bool no_border = false);
	void invalidate_addr_range_boundary(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr);
	void draw_bitmap(CDC* pDC);
	void draw_ants(CDC* pDC, FILE_ADDRESS start, FILE_ADDRESS end, COLORREF);
	void draw_bounds(CDC* pDC, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, COLORREF clr1, COLORREF clr2);
	void draw_lines(CDC* pDC, FILE_ADDRESS start, FILE_ADDRESS end, COLORREF clr1, COLORREF clr2);
	void draw_pixel(CDC* pDC, int pnum, int x, int y, COLORREF clr1, COLORREF clr2, int horz = 0, int vert = 0);
	void get_colours(COLORREF clr, COLORREF & clr1, COLORREF & clr2);
	void draw_left_border(CDC* pDC, int left, int right, int row, COLORREF clr);
	void draw_top_border(CDC* pDC, int x, int ncols, COLORREF clr);
	void set_zoom(int z, bool scroll = true);

	union
	{
		// Note as this state is written to RecentFileList you must preserve the position of bits
		// in the union below
		unsigned long disp_state_;
		struct
		{
			unsigned int draw_ants_sel: 1;
			unsigned int draw_ants_mark: 1;
			unsigned int draw_ants_hl: 1;
			unsigned int draw_ants_search: 1;
			unsigned int draw_ants_bm: 1;
			unsigned int res1: 3;               // reserve 3 more bits for future ant options

			unsigned int draw_bdr_sel: 1;
			unsigned int draw_bdr_mark: 1;
			unsigned int draw_bdr_hl: 1;
			unsigned int draw_bdr_search: 1;
			unsigned int draw_bdr_bm: 1;
			unsigned int res2: 3;               // reserve 3 more bits for future border options

			unsigned int dpix: 6;               // Currently only the bottom 4 bits are used
		} disp_;
	};

	bool mouse_down_;                           // Is the left mouse button currently down?

	// The following are used to display a popup tip window
	CTipWnd tip_;       // Tip window shown when mouse hovers
	int tip_elt_;       // The elt we display info about
	int last_tip_elt_;  // The elt we last showed a tip about
	int elt_at(CPoint pt);                      // Given a mouse posn returns the elt under it
	void track_mouse(unsigned long);            // Turns on receipt of mouse hover/leave messages
	bool update_tip(int elt);                   // Refresh tip text
	void hex_addr_tip(FILE_ADDRESS addr);       // Adds a hex address to the tip window
	void dec_addr_tip(FILE_ADDRESS addr);       // Adds decimal address to the tip window

	// We use a timer to draw marching ants
	UINT timer_id_;     // Timer for selection flashing/marching ants
	UINT timer_msecs_;  // Number of millisecs between timer calls for timer_id_
	timer t00_;         // This is a "timer" that times how long it takes to process one "timer" event
	int timer_count_;   // Cycles between 0 and 7 for each timer call
	double bfactor_;    // Stretches the cycle when we are drawing a line that is not a multiple of 8 pixels
	FILE_ADDRESS prev_start_addr_, prev_end_addr_; // Previous selection in case we need to erase it

	// The following are used in drawing the window
	int bdr_left_, bdr_right_, bdr_top_, bdr_bottom_;       // Allow for borders around the "bitmap"
	FILE_ADDRESS scrollpos_;            // file address of the top-left displayed elt.
	int rows_, cols_;                   // Current number of rows and cols of elts shown in the window

	// Note: There are 2 different "zooms" which may be a bit confusing.
	// 1. BPE = number of bytes that contributes to an "elt" (ie, a "pixel" of the FreeImage bitmap).
	//    This allows the user to "zoom" in and out on large files to see more or less of the file.
	//    User can choose from between 1 and 65536, (restricted to the 16 values that are powers of 2?)
	//    For large files there is a limit on the smallest BPE allowed,
	//    otherwise a 1 TByte file would require 3 TBytes of memory at a BPE of 1.
	//    BPE says how big the FreeImage bitmap is so (like the bitmap) it is part of the 
	//    document and when it changes the FreeImage bitmap needs to be completely recalculated.
	// 2. DPIX = number of screen pixels required to display one "elt".
	//    Values are 1,2,3...8 for a 1x1, 2x2, 3x3, ... 8x8 square.
	//    Larger values allow individual elts to be visible on a high resolution display.
	//    For a small file in a large aerial view the "actual" value may be increased from the
	//    user specified value in order to make use of the extra screen real estate.
	//    When this value changes the FreeImage bitmap does not need to be recalculated (only
	//    reshaped) hence this can vary between views and is stored here.
	int actual_dpix_;   // Actual display pixel size - may be larger than dpix_ if the whole file fits in the window
	void get_disp_params(int &rows, int &cols, int &actual_dpix);

	// We cache the search occurrences that are currently drawn in the window.
	// This speeds up redraws espe the marching ants when there are millions of
	// occurrences since we don't have to rescan CHexEditDoc::found_.
	std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > search_pair_;  // Areas that need to be drawn to indicate found occurrences

	// Scrollbars
	int sbfact_;        // This is a scaling factor for the scrollbar as for large files 32-bit ints are not enough
	void update_bars();         // Make sure scroll bars reflect the part of the bitmap displayed
	void update_display();      // Recalc display params when it changes to speed OnDraw

	timer t0_, t1_;     // Used in performance tests
};
#endif  // DATAFORMATVIEW_INCLUDED_
