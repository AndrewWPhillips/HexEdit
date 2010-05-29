// CompareView.h : header file of the CCompareView class
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
/////////////////////////////////////////////////////////////////////////////

#ifndef COMPAREVIEW_INCLUDED
#define COMPAREVIEW_INCLUDED  1

#include "ScrView.h"
#include <vector>

// Forward declarations
class CHexEditDoc;
class CChildFrame;

// When comparing files we use this to displayed the compared with file.
// It depends on the corresponding CHexEditView for a lot of its display
// formating but there is independent control over:
//  - file displayed and hence OnDraw and OnInitialUpdate
//  - current position (unless using auto-sync)
//  - selection (so the user can select parts of this file)
//  - searches

class CCompareView : public CScrView
{
    friend CHexEditView;
protected: // create from serialization only
    CCompareView();
    virtual ~CCompareView();
    DECLARE_DYNCREATE(CCompareView)

public:
	CHexEditView * phev_;

// Attributes
public:
	CHexEditDoc * GetDocument() { return (CHexEditDoc*)phev_->m_pDocument; }

	FILE_ADDRESS GetPos() const { return pos2addr(GetCaret()); }
    BOOL ReadOnly() const { return TRUE; }
    BOOL CharMode() const { return phev_->display_.edit_char; }
	BOOL EbcdicMode() const { return phev_->display_.char_set == CHARSET_EBCDIC; }
    BOOL OEMMode() const { return phev_->display_.char_set == CHARSET_OEM; }
    BOOL ANSIMode() const { return phev_->display_.char_set == CHARSET_ANSI; }
	BOOL DecAddresses() const { return !phev_->display_.hex_addr; }  // Now that user can show both addresses at once this is probably the best return value

// Operations
    void calc_addr_width(FILE_ADDRESS);     // Also used by recalc_display
    void draw_bg(CDC* pDC, const CRectAp &doc_rect, bool neg_x, bool neg_y,
                 int char_height, int char_width, int char_width_w,
                 COLORREF, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr,
                 int draw_height = -1);
    //virtual void SetSel(CPointAp, CPointAp, bool base1 = false);

    bool CopyToClipboard();
    COLORREF GetDefaultTextCol() { return phev_->text_col_; }
    COLORREF GetBackgroundCol() { return phev_->bg_col_; }
    COLORREF GetDecAddrCol() { return phev_->dec_addr_col_; }
    COLORREF GetHexAddrCol() { return phev_->hex_addr_col_; }
    COLORREF GetSearchCol() { return phev_->search_col_; }
    COLORREF GetHighlightCol() { return phev_->hi_col_; }
    COLORREF GetMarkCol() { return phev_->mark_col_; }
    COLORREF GetBookmarkCol() { return phev_->bm_col_; }
    CString GetSchemeName() { return phev_->scheme_name_; }

public:

// Overrides
    //virtual void DisplayCaret(int char_width = -1);

public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual void OnInitialUpdate();
//    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
protected:
    //virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    //virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    //virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    //virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
    //virtual void OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView);
    //virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
    //virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
    //virtual void DoInvalidate();

protected:
    virtual void ValidateCaret(CPointAp &pos, BOOL inside=TRUE);
    //virtual void InvalidateRange(CPointAp start, CPointAp end, bool f = false);
    //virtual void DoInvalidateRect(LPCRECT lpRect);
    //virtual void DoInvalidateRgn(CRgn* pRgn);
    //virtual void DoScrollWindow(int xx, int yy);
    //virtual void DoUpdateWindow();
    //virtual void DoHScroll(int total, int page, int pos);
    //virtual void DoVScroll(int total, int page, int pos);
    //void DoUpdate();

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    CPointAp addr2pos(FILE_ADDRESS address, int row = 0) const; // Convert byte address in doc to display position
    int hex_pos(int column, int width=0) const // get X coord of hex display column
    {
		// TBD xxx fix for this view
        if (width == 0) width = phev_->text_width_;
        return (phev_->addr_width_ + column*3 + column/phev_->group_by_)*width;
    }
    int char_pos(int column, int widthd=0, int widthw=0) const // get X coord of ASCII/EBCDIC display column
    {
		// TBD xxx fix for this view
        if (widthd == 0) widthd = phev_->text_width_;
        if (widthw == 0) widthw = phev_->text_width_w_;
        if (phev_->display_.vert_display)
            return phev_->addr_width_*widthd +
                   (column + column/phev_->group_by_)*widthw;
        else if (phev_->display_.hex_area)
            return (phev_->addr_width_ + phev_->rowsize_*3)*widthd +
                   ((phev_->rowsize_-1)/phev_->group_by_)*widthd +
                   column*widthw;
        else
            return phev_->addr_width_*widthd +
                   column*widthw;
    }
    int pos_hex(int, int inside = FALSE) const;  // Closest hex display col given X
    int pos_char(int, int inside = FALSE) const; // Closest char area col given X
    FILE_ADDRESS pos2addr(CPointAp pos, BOOL inside = TRUE) const; // Convert a display position to closest address
    int pos2row(CPointAp pos);                    // Find vert_display row (0, 1, or 2) of display position

	// Functions for selection tip (sel_tip_)
//    void show_selection_tip();
    void invalidate_addr_range(FILE_ADDRESS, FILE_ADDRESS); // Invalidate hex/aerial display for address range
	void invalidate_hex_addr_range(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr);  // Invalidate hex view only

	void recalc_display();
    int addr_width_;            // How much room in display does address area take?
    int hex_width_, dec_width_, num_width_; // Components of addr_width_
};

#endif // COMPAREVIEW_INCLUDED

/////////////////////////////////////////////////////////////////////////////
