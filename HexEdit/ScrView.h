// ScrView.h :  CScrView is like CScrollView but also 
//              supports large scrolling and text sizes
//
// Copyright (c) 1999 by Andrew W. Phillips.
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

#ifndef SCRVIEW_INCLUDED
#define SCRVIEW_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CScrView view

class CScrView : public CView
{
protected:
        CScrView();           // protected constructor used by dynamic creation
        DECLARE_DYNAMIC(CScrView)

// Attributes
public:

// Operations
public:
    virtual BOOL MovePos(UINT nChar, UINT nRepCnt, BOOL, BOOL, BOOL);

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CScrView)
        public:
        virtual void OnInitialUpdate();
        virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
        protected:
        virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
        //}}AFX_VIRTUAL

public:
    static const CSizeAp null_size;
    static const CPointAp null_point;

    void SetSize(CSizeAp total, CSizeAp page = null_size, CSizeAp line = null_size);
    void SetTSize(CSizeAp total, CSizeAp page = null_size, CSizeAp line = null_size);
    void GetSize(CSizeAp &total, CSizeAp &page, CSizeAp &line) const
    {
        total = total_;
        page = page_;
        line = line_;
    }
    void GetDisplayRect(CRect * rct) const
	{
        GetClientRect(rct);
        rct->DeflateRect(bdr_left_, bdr_top_, bdr_right_, bdr_bottom_);
        if (rct->bottom < rct->top) rct->bottom = rct->top;     // make sure deflate does not invert rect
    }

    void SetScroll(CPointAp, BOOL strict=FALSE);
    void SetTScroll(CPointAp);    // set upper left in text units
    CPointAp GetScroll() const    // get upper left pos of display
    {
        return scrollpos_;
    }
    BOOL ScrollUp() const       // Was last scroll direction upwards?
    {
        return scroll_up_;
    }
    void SetScrollPastEnds(BOOL v = FALSE) { scroll_past_ends_ = v; }
    BOOL GetScrollPastEnds() { return scroll_past_ends_; }
    void SetAutoscroll(int aa = 10) { autoscroll_accel_ = aa; }

    void CaretMode();           // Set caret mode (arrow keys move caret)
    void ScrollMode();          // Set scroll mode (arrow keys scroll)

    void LineCaret();           // Make the caret a line to the left of the current char
    void BlockCaret();          // Make a solid block caret ove the current char

    void SetCaret(CPointAp);    // set pos of caret (move display if nec.)
    void SetTCaret(CPointAp);   // set pos of caret in text units
    CPointAp GetCaret() const   // get pos of caret
    {
        return caretpos_;
    }
    CRectAp GetCaretRect()        // get rectangle bounding the caret
    {
        return CRectAp(caretpos_, caret_size());
    }
    virtual void DisplayCaret(int char_width = -1);
    BOOL CaretDisplayed(CSizeAp win_size = null_size);
    void EnableCaret();
    void DisableCaret();
    void CaretShow();


    void SetHorzBufferZone(int n = 1) { horz_buffer_zone_ = n; }
    int GetHorzBufferZone() { return horz_buffer_zone_; }
    void SetVertBufferZone(int n = 0) { vert_buffer_zone_ = n; }
    int GetVertBufferZone() { return vert_buffer_zone_; }

    virtual void SetSel(CPointAp, CPointAp, bool base1 = false);  // set selection
    void SetTSel(CPointAp, CPointAp); // set selection in text units
    // Return start and end of current selection
    BOOL GetSel(CPointAp &start, CPointAp &end) const
    {
        start = caretpos_;
        end = selpos_;
        return basepos_ == selpos_;
    }

    void DisplayPart(CPointAp pos, CSizeAp size = null_size, BOOL show_end = FALSE);

//    void ConvertToDP(CRect &rr);
//    void ConvertToDP(CPoint &pp);
//    void ConvertFromDP(CRect &rr);
//    void ConvertFromDP(CPoint &pp);
    CRect ConvertToDP(CRectAp rr);
    CSize ConvertToDP(CSizeAp ss);
    CPoint ConvertToDP(CPointAp pp);
    CRectAp ConvertFromDP(CRect rr);
    CSizeAp ConvertFromDP(CSize ss);
    CPointAp ConvertFromDP(CPoint pp);
    bool negx() { ASSERT(init_coord_); return negx_; }
    bool negy() { ASSERT(init_coord_); return negy_; }

    CPen *SetPen(CPen *pp) { CPen *rv = pen_; pen_ = pp; return rv; }
    CBrush *SetBrush(CBrush *bb) { CBrush *rv = brush_; brush_ = bb; return rv; }
    CFont *SetFont(CFont *ff) { CFont *rv = font_; font_ = ff; return rv; }
    CBitmap *SetBitmap(CBitmap *bb) { CBitmap *rv = bitmap_; bitmap_ = bb; return rv; }
    CRgn *SetRgn(CRgn *rr) { CRgn *rv = rgn_; rgn_ = rr; return rv; }
    int SetMapMode(int mm);

    // Implementation
protected:
    virtual ~CScrView();
    void OnSelUpdate(CPoint);
    virtual void ValidateCaret(CPointAp &pos, BOOL inside=TRUE);
    virtual void ValidateScroll(CPointAp &pos, BOOL strict=FALSE);
    virtual void InvalidateRange(CPointAp start, CPointAp end, bool f = false);

    // We need to add these as virtual functions so that we can override them
    // in derived classes.  (For example, to allow turning off refresh in
    // HexEditView.)  This is necessary since the corresponding MFC functions
    // (Invalidate, ScrollWindow etc) are not virtual.
    virtual void DoInvalidate() { Invalidate(); }
    virtual void DoInvalidateRect(LPCRECT lpRect) { InvalidateRect(lpRect); }
    virtual void DoInvalidateRgn(CRgn* pRgn) { InvalidateRgn(pRgn); }
    virtual void DoScrollWindow(int xx, int yy)
    {
        CRect rct;
        GetDisplayRect(&rct);
        ScrollWindow(xx, yy, &rct, &rct);
    }
	virtual void AfterScroll(CPointAp newpos) { }
    virtual void DoUpdateWindow() { UpdateWindow(); }
    virtual void DoHScroll(int total, int page, int pos);
    virtual void DoVScroll(int total, int page, int pos);
    virtual void DoSetCaretPos(POINT pt) { SetCaretPos(pt); }

    // Allow derived classes to check left mouse button, shift and control key state
    BOOL mouse_down()   { return mouse_down_; }
    BOOL shift_down()   { return ::GetKeyState(VK_SHIFT) < 0; }
    BOOL control_down() { return ::GetKeyState(VK_CONTROL) < 0; }
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    int bdr_top_, bdr_left_, bdr_bottom_, bdr_right_; // Borders reserved for ruler etc

protected:
    // The following determine the current position in the doc of the top left
    // of the display, the caret and the selection.  caretpos_ is the start of
    // the selection, selpos_ is the end.  caretpos_ == selpos_ if there is
    // nothing selected.  basepos_ is used as the base position when a mouse
    // selection is in progress.
    CPointAp scrollpos_, caretpos_, selpos_, basepos_;

private:
    // The following are in logical units for the window.
    // They determine the total size of the document, the size
    // of a page and a line, for purposes of scrolling and
    // drawing the scrollbar.
    CSizeAp total_, page_, line_;

    // If page or line scroll sizes have been specified we use those
    // values, otherwise we recalculate them when the window size changes.
    BOOL page_specified_, line_specified_;

    // maxbar_ is the number of scrollbar positions.
    // Windows supports only 32768 scrollbar positions so this
    // is necessary if total_.cx/cy is greater than this.
    CSizeAp maxbar_;

    // The following are attributes of the device context
    CPen *pen_;
    CBrush *brush_;
    CFont *font_;
    CBitmap *bitmap_;
    CRgn *rgn_;
    int mapmode_;

    // The following are used to convert between device and logical units
    // (since DPtoLP and LPtoDP in Windows 95 are limited to 16 bit values!!!)
    CPoint dev_;                        // device units
    CPoint log_;                        // logical units

    // The following are just cached for improved scroll bar performance
    __int64 win_height_;
    int win_width_;

    BOOL in_update_;            // internal state for OnSize callback

    BOOL caret_mode_;           // Arrows move caret when on (else arrows just scroll)
    BOOL caret_changes_;        // Is the caret allowed to be changed (moved, deleted etc)?
//    BOOL caret_visible_;      // Has caret been shown? (ie. caretpos_ is in display)
    int caret_level_;           // 0 means caret is visible, > 0 means caret is hidden
    BOOL caret_seen_;           // Is caret actually on (when caret_level_ == 0)
    BOOL caret_block_;          // Is the caret a line or block?

    BOOL scroll_up_;            // Tracks which last scroll dirn so that screen refreshes are drawn to match
    BOOL scroll_past_ends_;     // Allow scroll past ends?
    int autoscroll_accel_;      // Rate at which autoscroll increases as you move past window edge

    int horz_buffer_zone_;      // When displaying caret how many characters from right edge to buffer
    int vert_buffer_zone_;      // Make sure caret stays this many lines from bottom/top edge

//    CPoint valid_pos(const CPoint) const;
    void update_bars(CPointAp newpos = null_point);
    CSizeAp caret_size();
    void caret_show();
    void caret_hide();

    void check_coords();        // Updates values of negx_ and negy_
    bool init_coord_;           // Have we initialised negx_ and negy_
    bool negx_, negy_;          // Are our axes opposite to logical coords?

    // The following used while selectin with mouse
    BOOL mouse_down_;           // Left mouse button is down?
    UINT timer_id_;             // Timer id for auto scroll timer (set in mouse down)

        // Generated message map functions
protected:
//      virtual void CalcWindowRect(LPRECT lpClientRect,UINT nAdjustType = adjustBorder);

        // scrolling implementation support for OLE
        virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
        virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);

        //{{AFX_MSG(CScrView)
        afx_msg void OnSize(UINT nType, int cx, int cy);
        afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
        afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
        afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
        afx_msg void OnSetFocus(CWnd* pOldWnd);
        afx_msg void OnKillFocus(CWnd* pNewWnd);
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
        afx_msg void OnMouseMove(UINT nFlags, CPoint point);
        afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // SCRVIEW_INCLUDED
