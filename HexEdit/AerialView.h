#ifndef DATAFORMATVIEW_INCLUDED_
#define DATAFORMATVIEW_INCLUDED_  1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HexEditDoc.h"
#include "HexEditView.h"

// CAerialView view

class CAerialView : public CView
{
	DECLARE_DYNCREATE(CAerialView)

protected:
	CAerialView();           // protected constructor used by dynamic creation
	virtual ~CAerialView();

// Attributes
public:
    CHexEditView *phev_;                     // Ptr to sister hex view

public:
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    void SetScroll(int newpos);
    void SetScroll(FILE_ADDRESS newpos) { SetScroll(int(newpos/(GetDocument()->GetBpe() * cols_))); }
    void ShowPos(FILE_ADDRESS pos);

protected:
    virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);

    // message handlers
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDestroy();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
    CHexEditDoc *GetDocument()
    {
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHexEditDoc)));
        return (CHexEditDoc*)m_pDocument;
    }
#endif

private:
    void get_disp_params(int &rows, int &cols, int &actual_dpix);
    void update_bars();         // Make sure scroll bars reflect the part of the bitmap displayed

    // scrollpos_ = file address of the top-left displayed elt.
    FILE_ADDRESS scrollpos_;
    
    // Note: There are 2 different "zooms" which may be a bit confusing.
    // 1. BPE = number of bytes that contributes to an "elt" (ie, a "pixel" of the FreeImage bitmap).
    //    This allows the user to "zoom" in and out on large files to see more or less of the file.
    //    User can choose from between 1 and 65536, (restricted to the 16 values that are powers of 2?)
    //    For large files there is a limit on the smallest BPE allowed,
    //    otherwise a 1 TByte file would require 3 TBytes of memory at a BPE of 1.
    //    xxx need user option to specify largest amount of memory to use (min 16Mb = 1 TByte file @ BPE 65536)
    //    BPE says how big the FreeImage bitmap is so (like the bitmap) it is part of the 
    //    document and when it changes the FreeImage bitmap needs to be completely recalculated.
    // 2. DPIX = number of screen pixels required to display one "elt".
    //    Values are 1,2,3...8 for a 1x1, 2x2, 3x3, ... 8x8 square.
    //    Larger values allow individual elts to be visible on a high resolution display.
    //    xxx maybe only user option is a checkbox to toggle between 1x1 and 3x3
    //    For a small file in a large aerial view the "actual" value may be increased from the
    //    user specified value in order to make use of the extra screen real estate.
    //    When this value changes the FreeImage bitmap does not need to be recalculated (only
    //    reshaped) hence this can vary between views and is stored here.

    int dpix_;          // Preferred display pixel dimensions in X and Y used to display one elt (eg 3 gives 3x3 pixel)

    int rows_, cols_;   // Current number of rows and cols of display pixels shown in the window
    int actual_dpix_;   // Actual display pixel size - may be larger than dpix_ if the whole file fits in the window

    int bdr_left_, bdr_right_, bdr_top_, bdr_bottom_;       // Width of borders around the "bitmap"
};
#endif  // DATAFORMATVIEW_INCLUDED_
