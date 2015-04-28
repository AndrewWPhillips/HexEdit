// PrevwView.cpp : implements CPrevwView
//

#include "stdafx.h"
#include "HexEdit.h"
#include "PrevwView.h"
#include "HexFileList.h"    // used to retrieve settings

IMPLEMENT_DYNCREATE(CPrevwView, CView)

CPrevwView::CPrevwView()
{
	phev_ = NULL;
	zoom_ = 0.0;    // flag that zoom/pos has not yet been set
}

CPrevwView::~CPrevwView()
{
}

BEGIN_MESSAGE_MAP(CPrevwView, CView)
END_MESSAGE_MAP()

// CPrevwView drawing
void CPrevwView::OnInitialUpdate()
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL && phev_ != NULL);
	pDoc->AddPreviewView(phev_);

	// Get last display option
	CHexFileList *pfl = theApp.GetFileList();
	int recent_file_index = -1;
	if (pDoc->pfile1_ != NULL)
		recent_file_index = pfl->GetIndex(pDoc->pfile1_->GetFilePath());

	// Get preview view display options from pfl xxx TBD zoom and pos
	//pfl->GetData(recent_file_index, CHexFileList::PREVIEWXXX);

	CView::OnInitialUpdate();
}

// OnUpdate is called when the document changes
// - background calc of elts (FreeImage bitmap) is finished (or is finished for display area)
// - doc changes requiring recalc of the elts
// - bpe changes requiring recalc of the elts
// - background search finished (CBGSearchHint) - perhaps display bg search occurrences
// - bookmark change (CBookmarkHint) - perhaps update display of the bookmark
void CPrevwView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (pHint == NULL)
		return;

	if (pHint->IsKindOf(RUNTIME_CLASS(CHexHint)))
	{
		Invalidate();
	}
}

void CPrevwView::OnDraw(CDC* pDC)
{
	validate_display();
	draw_bitmap(pDC);
}

// CPrevwView diagnostics

#ifdef _DEBUG
void CPrevwView::AssertValid() const
{
	CView::AssertValid();
}

#ifndef _WIN32_WCE
void CPrevwView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif
#endif //_DEBUG


// CPrevwView message handlers


// Drawing routines
void CPrevwView::draw_bitmap(CDC* pDC)
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL || pDoc->preview_dib_ == NULL)
		return;

	BITMAPINFOHEADER *bih = FreeImage_GetInfoHeader(pDoc->preview_dib_);
	ASSERT(bih->biCompression == BI_RGB && bih->biHeight > 0);

	CRect rct;
	GetClientRect(&rct);

	//pDC->GetClipBox(&rct);                       // only BLT the area that needs it

	::StretchDIBits(pDC->GetSafeHdc(),
					//rct.left, rct.top, rct.Width(), rct.Height(),
					pos_.x, pos_.y, int(bih->biWidth/zoom_), int(bih->biHeight/zoom_),
					0, 0, bih->biWidth, bih->biHeight,
					FreeImage_GetBits(pDoc->preview_dib_), FreeImage_GetInfo(pDoc->preview_dib_),
					DIB_RGB_COLORS, SRCCOPY);
}

void CPrevwView::validate_display()
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	// GEt size of the display area (client area of the window)
	CRect rct;
	GetClientRect(&rct);

	// Get dimaensions of the bitmap
	double width  = FreeImage_GetWidth(pDoc->preview_dib_);
	double height = FreeImage_GetHeight(pDoc->preview_dib_);

	// zoom of zero is probably due to first display
	if (zoom_ <= 0.0)
	{
		// Work out zoom so the whole bitmap fits in the display
		zoom_ = max(width/rct.Width(), height/rct.Height());

		// Work out position so there are equal bars at top/bottom OR left/right
		pos_.x = (rct.Width()  - int(width/zoom_))/2;    // xxx round instead of truncate???
		pos_.y = (rct.Height() - int(height/zoom_))/2;
		ASSERT(pos_.x == 0 || pos_.y == 0);              // at least one dimension should fill all the way and be at the left OR top edge
	}

	if (pos_.x < 0 && pos_.x + int(width/zoom_) < rct.Width())
	{
		// move it to the right
		if (int(width/zoom_) < rct.Width())
			pos_.x = 0;
		else
			pos_.x = rct.Width() - int(width/zoom_);
	}

	if (pos_.x > 0 && pos_.x + int(width/zoom_) > rct.Width())
	{
		// move it to the left
		if (int(width/zoom_) < rct.Width())
			pos_.x = rct.Width() - int(width/zoom_);
		else
			pos_.x = 0;
	}

	if (pos_.y < 0 && pos_.y + int(height/zoom_) < rct.Height())
	{
		// move it down
		if (int(height/zoom_) < rct.Height())
			pos_.y = 0;
		else
			pos_.y = rct.Height() - int(height/zoom_);
	}

	if (pos_.y > 0 && pos_.y + int(height/zoom_) > rct.Height())
	{
		// move it up
		if (int(height/zoom_) < rct.Height())
			pos_.y = rct.Height() - int(height/zoom_);
		else
			pos_.y = 0;
	}
}