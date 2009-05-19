// AerialView.cpp : implementation file
//

#include "stdafx.h"
#include "HexEdit.h"
#include "AerialView.h"


// CAerialView

IMPLEMENT_DYNCREATE(CAerialView, CView)

CAerialView::CAerialView()
{
    phev_ = NULL;
    rows_ = cols_ = -1;

    bdr_left_ = 4;
    bdr_right_ = bdr_top_ = bdr_bottom_ = 0;
    dpix_ = 1;  // xxx add user option for default dpix_
}

CAerialView::~CAerialView()
{
}

BEGIN_MESSAGE_MAP(CAerialView, CView)
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_DESTROY()
    ON_WM_KEYDOWN()
    ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

// CAerialView drawing

void CAerialView::OnDraw(CDC* pDC)
{
	// xxx pDC->GetClipBox() to find area to display/print
	// xxx When printing (pDC->IsPrinting()) use quad pixel size

    ASSERT(phev_ != NULL && GetDocument() != NULL && GetDocument()->dib_ != NULL);
    if (phev_ == NULL || GetDocument() == NULL || GetDocument()->dib_ == NULL)
        return;

    CHexEditDoc *pDoc = GetDocument();

    CRect rct;
    GetClientRect(rct);
    
    // Work out where to draw the bitmap    
    rct.DeflateRect(bdr_left_, bdr_top_, bdr_right_, bdr_bottom_);

    BITMAPINFOHEADER *bih = FreeImage_GetInfoHeader(pDoc->dib_);
    ASSERT(bih->biCompression == BI_RGB && bih->biHeight > 0);

    // Work out how to "reshape" the bitmap    
    // Note: this should only be done just before Blting the bitmap as more than one
    // view can use the same bitmap and they may require different "shapes".
    bih->biWidth = cols_;
    ASSERT(cols_ % 8 == 0 && cols_ > 0 && cols_ <= CHexEditDoc::MAX_WIDTH);
    ASSERT((pDoc->length_-1)/bih->biWidth < LONG_MAX);
    bih->biHeight = LONG((pDoc->NumElts()-1)/bih->biWidth) + 1;

    // Work out which part of the bitmap to show
    int width = pDoc->GetBpe() * cols_;
    int srcY = int(scrollpos_/width);
    int srcHeight = rows_;
    if (srcHeight > bih->biHeight - srcY) srcHeight = bih->biHeight - srcY;
    ASSERT(scrollpos_/width < INT_MAX);
    int dstWidth = cols_*actual_dpix_;
    int endpos = int((pDoc->length()-1)/width + 1);     // row just past eof
    int dstHeight = (rows_ > endpos ? endpos : rows_)*actual_dpix_;

    // Note that in order to get the bitmap drawn from the top (upside down)
    // we negate param 9 (srcHeight) and add srcHeight to param 7 (srcY).
    ::StretchDIBits(pDC->GetSafeHdc(),
                    rct.left, rct.top, dstWidth, dstHeight,
                    0, srcY + srcHeight + 1, bih->biWidth, -srcHeight,
                    FreeImage_GetBits(pDoc->dib_), FreeImage_GetInfo(pDoc->dib_),
                    DIB_RGB_COLORS, SRCCOPY);
}

// CAerialView diagnostics

#ifdef _DEBUG
void CAerialView::AssertValid() const
{
	CView::AssertValid();
}

void CAerialView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

// CAerialView message handlers
void CAerialView::OnInitialUpdate() 
{
    GetDocument()->AddAerialView(phev_);
    CView::OnInitialUpdate();

    // Calculate initial scroll position and display parameters
    get_disp_params(rows_, cols_, actual_dpix_);
    scrollpos_ = 0;     // xxx default to last posn or perhaps current carte in phev_??
    update_bars();
}

// Calculates the new "shape" of the bitmap.  This needs to be done when:
// - window size is changed
// - dpix_ is changed causing a different actual_dpix_ value
void CAerialView::get_disp_params(int &rows, int &cols, int &actual_dpix)
{
    // Get the actual display area
    CRect rct;
    GetClientRect(&rct);
    rct.DeflateRect(bdr_left_, bdr_top_, bdr_right_, bdr_bottom_);

    // Increase the actual display pixel size if the whole file fits in the 
    // display areas up to as big as 8 x 8.
    for (actual_dpix = dpix_; actual_dpix < 9; ++actual_dpix)
    {
        cols = (rct.Width()/(8*actual_dpix)) * 8;
        if (cols < 8)
            cols = 8;
        else if (cols > CHexEditDoc::MAX_WIDTH)
            cols = CHexEditDoc::MAX_WIDTH;
        rows = rct.Height()/actual_dpix;
        if (rows < 1) rows = 1;
        
        if ((GetDocument()->NumElts()-1) / cols + 1 > rows)
            break;
    }
    if (actual_dpix > dpix_)
    {
        // Reduce back to the size that fits
        --actual_dpix;
        cols = (rct.Width()/(8*actual_dpix)) * 8;
        if (cols < 8)
            cols = 8;
        else if (cols > CHexEditDoc::MAX_WIDTH)
            cols = CHexEditDoc::MAX_WIDTH;
        rows = rct.Height()/actual_dpix;
        if (rows < 1) rows = 1;
    }
    ASSERT(actual_dpix >= dpix_ && actual_dpix <= 8);
}

void CAerialView::update_bars()
{
    SCROLLINFO si;
    memset(&si, 0, sizeof(si)); // default to hiding the scrollbar
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    
    CHexEditDoc *pDoc = GetDocument();
    int width = pDoc->GetBpe() * cols_;
    int pos = int(scrollpos_/width);                    // current pos as row no
    int endpos = int((pDoc->length()-1)/width + 1);     // row just past eof
    
    if (endpos > rows_)                           // EOF past end of display
    {
        si.nMax = endpos;
        si.nPage = rows_;
        si.nPos = pos;
    }

    SetScrollInfo(SB_VERT, &si);
}

void CAerialView::SetScroll(int newpos)
{
    CHexEditDoc *pDoc = GetDocument();
    int width = pDoc->GetBpe() * cols_;
    ASSERT(scrollpos_/width < INT_MAX);
    int pos = int(scrollpos_/width);                    // current pos as row no
    ASSERT((pDoc->length()-1)/width + 1 < INT_MAX);
    int endpos = int((pDoc->length()-1)/width + 1);     // row just past eof

    // Make sure new scroll pos is valid
    if (newpos > endpos - rows_)
        newpos = endpos - rows_;
    if (newpos < 0)
        newpos = 0;
    if (newpos == pos)
        return;                                         // no change

    scrollpos_ = FILE_ADDRESS(newpos) * width;
    if (abs(newpos - pos) > rows_/2)
        Invalidate();           // Just redraw it all if scrolling more than half a page
    else    
    {
        CRect rct;
        rct.top    = bdr_top_;
        rct.left   = bdr_left_;
        rct.bottom = rct.top + rows_ * actual_dpix_;
        rct.right  = rct.left + cols_ * actual_dpix_;
        ScrollWindow(0, (pos - newpos)*actual_dpix_, rct, rct);
    }
    update_bars();
}

void CAerialView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    OnScroll(MAKEWORD(-1, nSBCode), nPos);
}

BOOL CAerialView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll /*= TRUE*/)
{
    CHexEditDoc *pDoc = GetDocument();
    int width = pDoc->GetBpe() * cols_;
    ASSERT(scrollpos_/width < INT_MAX);
    int pos = int(scrollpos_/width);                    // current pos as row no
    ASSERT((pDoc->length()-1)/width + 1 < INT_MAX);
    int endpos = int((pDoc->length()-1)/width + 1);     // row just past eof
    int newpos = pos;                                   // new pos as row no (adjusted below)

// xxx for large files we need to add a scaling factor as the scrollbar resn is on +/- 2Gig
    switch (HIBYTE(nScrollCode))
    {
    case SB_TOP:
        newpos = 0;
        break;
    case SB_BOTTOM:
        newpos = endpos;        // set top-left to end - this will be adjusted in SetScroll
        break;
    case SB_PAGEUP:
        newpos -= rows_ - 1;
        break;
    case SB_PAGEDOWN:
        newpos += rows_ - 1;
        break;
    case SB_LINEUP:
        newpos -= 8/actual_dpix_;
        break;
    case SB_LINEDOWN:
        newpos += 8/actual_dpix_;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        newpos = nPos;
        break;
    }

    if (bDoScroll && newpos != pos)
    {
        SetScroll(newpos);
    }
    return TRUE;
}

void CAerialView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    CHexEditDoc *pDoc = GetDocument();
    int width = GetDocument()->GetBpe() * cols_;
    ASSERT(scrollpos_/width < INT_MAX);
    int pos = int(scrollpos_/width);                    // current pos as row no
    ASSERT((pDoc->length()-1)/width + 1 < INT_MAX);
    int endpos = int((pDoc->length()-1)/width + 1);     // row just past eof
    int newpos = pos;                                   // new pos as row no (adjusted below)

    switch (nChar)
    {
    case VK_HOME:
        newpos = 0;
        break;
    case VK_END:
        newpos = endpos;        // set top-left to end - this will be adjusted in SetScroll
        break;
    case VK_PRIOR:
        newpos -= rows_ - 1;
        break;
    case VK_NEXT:
        newpos += rows_ - 1;
        break;
    case VK_UP:
        newpos -= 8/actual_dpix_;
        break;
    case VK_DOWN:
        newpos += 8/actual_dpix_;
        break;
    }
        
    if (newpos != pos)
    {
        SetScroll(newpos);
        return;
    }
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CAerialView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
    if ((nFlags & MK_CONTROL) != 0)
    {
        int prev_dpix = actual_dpix_;
        if (zDelta < 0 && dpix_ < 8)  // zoom out
            ++dpix_;
        else if (zDelta > 0 && dpix_ > 1) // zoom in
            dpix_--;
        get_disp_params(rows_, cols_, actual_dpix_);
        if (actual_dpix_ != prev_dpix)
        {
            Invalidate();
            update_bars();
        }
    }
    else
    {
        ASSERT(scrollpos_/(GetDocument()->GetBpe() * cols_) < INT_MAX);
        SetScroll(int(scrollpos_/(GetDocument()->GetBpe() * cols_)) - zDelta);
    }
    return TRUE;
}

void CAerialView::OnSize(UINT nType, int cx, int cy) 
{
    if (cx > 0 && cy > 0 && GetDocument()->GetBpe() > -1)
    {
        get_disp_params(rows_, cols_, actual_dpix_);
        update_bars();
    }
    CView::OnSize(nType, cx, cy);
}

// OnUpdate is called when the document changes
// - background calc of elts (FreeImage bitmap) is finished (or is finished for display area)
// - doc changes requiring recalc of the elts
// - bpe changes requiring recalc of the elts
// - background search finished (CBGSearchHint) - perhaps display bg search occurrences
// - bookmark change (CBookmarkHint) - perhaps update display of the bookmark
void CAerialView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
}

void CAerialView::OnDestroy() 
{
    GetDocument()->RemoveAerialView();
    CView::OnDestroy();
}

BOOL CAerialView::OnEraseBkgnd(CDC* pDC) 
{
    ASSERT(phev_ != NULL);
    if (phev_ == NULL)
        return FALSE;

    // Fill with the background colour                    
    COLORREF bg_col;
    if ((bg_col = phev_->GetBackgroundCol()) == -1)
        bg_col = RGB(192,192,192);

    CRect rct;
    GetClientRect(rct);

    // Create a solid brush and fill the whole window with it
    CBrush backBrush;
    backBrush.CreateSolidBrush(bg_col);
    backBrush.UnrealizeObject();
    pDC->FillRect(rct, &backBrush);
    return TRUE;
}
