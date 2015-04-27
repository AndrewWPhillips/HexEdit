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
}

CPrevwView::~CPrevwView()
{
}

BEGIN_MESSAGE_MAP(CPrevwView, CView)
END_MESSAGE_MAP()

// CPrevwView drawing
void CPrevwView::OnInitialUpdate()
{
	ASSERT(phev_ != NULL);
	GetDocument()->AddPreviewView(phev_);

	// Get last display option
	CHexFileList *pfl = theApp.GetFileList();
	int recent_file_index = -1;
	if (GetDocument()->pfile1_ != NULL)
		recent_file_index = pfl->GetIndex(GetDocument()->pfile1_->GetFilePath());
	// Get preview view display options from pfl
	//pfl->GetData(recent_file_index, CHexFileList::AERIALXXX);

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

	CRect cliprct; pDC->GetClipBox(&cliprct);                       // only BLT the area that needs it

	::StretchDIBits(pDC->GetSafeHdc(),
					cliprct.left, cliprct.top, cliprct.Width(), cliprct.Height(),
					0, 0, bih->biWidth, bih->biHeight,
					FreeImage_GetBits(pDoc->preview_dib_), FreeImage_GetInfo(pDoc->preview_dib_),
					DIB_RGB_COLORS, SRCCOPY);
}
