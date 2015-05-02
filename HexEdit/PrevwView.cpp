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
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()
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
// - here we just use the CBGPreviewHint which indicates the preview bitmap has finished loading
void CPrevwView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (pHint == NULL)
		return;

	if (pHint->IsKindOf(RUNTIME_CLASS(CBGPreviewHint)))
	{
		validate_display();
		Invalidate();
	}
}

void CPrevwView::OnDraw(CDC* pDC)
{
	CHexEditDoc *pDoc = GetDocument();
	if (pDoc == NULL || pDoc->preview_dib_ == NULL)
		return;

	//validate_display();
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
BOOL CPrevwView::OnEraseBkgnd(CDC* pDC)
{
	CRect rct;
	pDC->GetClipBox(&rct);                       // only erase the area that needs it

	// Fill the background with a colour that matches the current MFC theme
	pDC->FillSolidRect(rct, afxGlobalData.clrBtnFace);
	return TRUE;
}

void CPrevwView::OnSize(UINT nType, int cx, int cy)
{
	if (phev_ == NULL) return;

	if (cx > 0 && cy > 0)
		validate_display();

	CView::OnSize(nType, cx, cy);    // this should invalidate -> redraw with new values
}

void CPrevwView::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);

	if (mouse_down_)
	{
		// xxx
	}
}

void CPrevwView::OnLButtonDown(UINT nFlags, CPoint point)
{
	mouse_down_ = true;

	CView::OnLButtonDown(nFlags, point);
}

void CPrevwView::OnLButtonUp(UINT nFlags, CPoint point)
{
	mouse_down_ = false;

	CView::OnLButtonUp(nFlags, point);
}

void CPrevwView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	mouse_down_ = true;
}

void CPrevwView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CHexEditDoc *pDoc = GetDocument();

if (nChar <= VK_CONTROL) return;

	switch (nChar)
	{
	case VK_UP:
		pos_.y -= 10;
		break;
	case VK_DOWN:
		pos_.y += 10;
		break;
	case VK_LEFT:
		pos_.x -= 10;
		break;
	case VK_RIGHT:
		pos_.x += 10;
		break;

	case 'B':
		//if ((nFlags & MK_CONTROL) != 0)
		if (::GetKeyState(VK_CONTROL) < 0)    // check for control key down
		{
			zoom_ = 0.0;  // force zoom to fit
		}
		break;
	case VK_ADD:
		if ((nFlags & MK_CONTROL) != 0)  // If Ctrl key down we zoom instead of scroll
		{
			zoom_ /= 1.5;
			// xxx apply min
			// xxx zoom around centre (or mouse position?)
		}
		break;
	case VK_SUBTRACT:
		if ((nFlags & MK_CONTROL) != 0)  // If Ctrl key down we zoom instead of scroll
		{
			zoom_ *= 1.5;
			// xxx apply max
			// xxx zoom around centre (or mouse position?)
		}
		break;
	}
	validate_display();
	Invalidate();

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CPrevwView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if ((nFlags & MK_CONTROL) != 0)  // If Ctrl key down we zoom instead of scroll
	{
		// xxx zoom around mouse position
		if (theApp.reverse_zoom_) zDelta = -zDelta;
		if (zDelta < 0)
			zoom_ *= 1.5 * double(zDelta)/WHEEL_DELTA;
		else
			zoom_ /= 1.5 * fabs(double(zDelta))/WHEEL_DELTA;
	}
	else if ((nFlags & MK_SHIFT) != 0)  // If Ctrl key down we zoom instead of scroll
	{
		pos_.x += zDelta/WHEEL_DELTA*30;
	}
	else
	{
		pos_.y += zDelta/WHEEL_DELTA*30;
	}
	validate_display();
	Invalidate();
	return TRUE;
}

void CPrevwView::OnContextMenu(CWnd* pWnd, CPoint point)
{
//	CContextMenuManager *pCMM = theApp.GetContextMenuManager();
//	pCMM->ShowPopupMenu(IDR_CONTEXT_AERIAL, point.x, point.y, this);
}

// Drawing routines
void CPrevwView::draw_bitmap(CDC* pDC)
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	BITMAPINFOHEADER *bih = FreeImage_GetInfoHeader(pDoc->preview_dib_);
	ASSERT(bih->biCompression == BI_RGB && bih->biHeight > 0);

	CRect rct;
	pDC->GetClipBox(&rct);                       // only BLIT the area that needs it

	// xxx need to check this code for boundary problems with small/large bitmaps/windows
	int sl = (rct.left - pos_.x) * zoom_;
	if (sl < 0) sl = 0;
	int st = (rct.top - pos_.y) * zoom_;
	if (st < 0) st = 0;
	int sr = (rct.right - pos_.x) * zoom_ + 1;
	if (sr > bih->biWidth) sr = bih->biWidth;
	int sb = (rct.bottom - pos_.y) * zoom_ + 1;
	if (sb > bih->biHeight) sb = bih->biHeight;

	int dl = sl/zoom_ + pos_.x;
	int dt = st/zoom_ + pos_.y;
	int dr = sr/zoom_ + pos_.x;
	int db = sb/zoom_ + pos_.y;

	::StretchDIBits(pDC->GetSafeHdc(),
					dl, dt, dr - dl, db - dt,
					sl, bih->biHeight - sb, sr - sl, sb - st,
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

	// Get dimensions of the bitmap (as double so we can calculate zoom)
	double bmwidth  = FreeImage_GetWidth(pDoc->preview_dib_);
	double bmheight = FreeImage_GetHeight(pDoc->preview_dib_);

	// zoom of zero is probably due to first display
	if (zoom_ <= 0.0)
	{
		// Work out zoom so the whole bitmap fits in the display
		zoom_ = max(bmwidth/rct.Width(), bmheight/rct.Height());

		// Move to top-left - code below will centre within the display
		pos_.x = pos_.y = 0;
		//pos_.x = (rct.Width()  - int(bmwidth/zoom_))/2;
		//pos_.y = (rct.Height() - int(bmheight/zoom_))/2;
	}

	int width = int(bmwidth/zoom_);

	if (width < rct.Width())
	{
		// Centre it between the left and right side
		pos_.x = (rct.Width()  - width)/2;    // xxx round instead of truncate???
	}
	else
	{
		// Next we ensure that the bitmap is not too far offscreen.
		// (We don't want it off one side if there is spare blank space on the other side)
		if (pos_.x < 0 && pos_.x + width < rct.Width())
		{
			// move it to the right
			if (width < rct.Width())
				pos_.x = 0;
			else
				pos_.x = rct.Width() - width;
		}

		if (pos_.x > 0 && pos_.x + width > rct.Width())
		{
			// move it to the left
			if (width < rct.Width())
				pos_.x = rct.Width() - width;
			else
				pos_.x = 0;
		}
	}

	int height = int(bmheight/zoom_);

	if (height < rct.Height())
	{
		// Centre it vertically between top and bottom edges
		pos_.y = (rct.Height()  - height)/2;    // xxx round instead of truncate???
	}
	else
	{
		// Ensure not off the top/bpttom when there is blank space available opposite
		if (pos_.y < 0 && pos_.y + height < rct.Height())
		{
			// move it down
			if (height < rct.Height())
				pos_.y = 0;
			else
				pos_.y = rct.Height() - height;
		}

		if (pos_.y > 0 && pos_.y + height > rct.Height())
		{
			// move it up
			if (height < rct.Height())
				pos_.y = rct.Height() - height;
			else
				pos_.y = 0;
		}
	}
}