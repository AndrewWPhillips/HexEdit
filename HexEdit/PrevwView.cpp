// PrevwView.cpp : implements CPrevwView
//

#include "stdafx.h"
#include "HexEdit.h"
#include "PrevwView.h"
#include "HexFileList.h"    // used to retrieve settings


// TODO
// Code
//  > zoom/scroll using mouse wheel
//  > zoom centred
//  > zoom min.max
//  - flip commands
//  - rotate command + reverse X/Y pos?
//  - handle alpha channel: AlphaBlend() into window-size bitmap then BLT into display
//    - also addresses flicker seen with big bitmaps
//  - copy to clipboard command??
//  - scrollbars??
// Test
//  - zoom min/max with different sized bitmaps
//  - BLT boundary contions


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

	ON_COMMAND(ID_PREVW_ZOOM_IN, OnZoomIn)
	ON_COMMAND(ID_PREVW_ZOOM_OUT, OnZoomOut)
	ON_COMMAND(ID_PREVW_ZOOM_ACTUAL, OnZoomActual)
	ON_COMMAND(ID_PREVW_ZOOM_FIT, OnZoomFit)
	ON_UPDATE_COMMAND_UI(ID_PREVW_ZOOM_IN, OnUpdateZoomIn)
	ON_UPDATE_COMMAND_UI(ID_PREVW_ZOOM_OUT, OnUpdateZoomOut)
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
		if ((nFlags & MK_CONTROL) != 0)  // If Ctrl-Plus is zoom in
		{
			zoom(1/1.5);
		}
		break;
	case VK_SUBTRACT:
		if ((nFlags & MK_CONTROL) != 0)  // If Ctrl-Minus is zoom out
		{
			zoom(1.5);
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
			zoom((1.5 * abs(zDelta)) / WHEEL_DELTA);
		else
			zoom(WHEEL_DELTA / (1.5 * zDelta));
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
	CContextMenuManager *pCMM = theApp.GetContextMenuManager();
	pCMM->ShowPopupMenu(IDR_CONTEXT_PREVW, point.x, point.y, this);
}

void CPrevwView::OnZoomIn()
{
	zoom(1/1.5);
	validate_display();
	Invalidate();
}

void CPrevwView::OnZoomOut()
{
	zoom(1.5);
	validate_display();
	Invalidate();
}

void CPrevwView::OnZoomActual()
{
	zoom_ = 1.0;                // 1 bitmap pixel = 1 display pixel
	validate_display();
	Invalidate();
}

void CPrevwView::OnZoomFit()
{
	zoom_ = 0.0;                // force zoom recalc to fit window
	validate_display();
	Invalidate();
}

// TODO we should be able to calulate better values (eg, that avoid overflow in
// bitmap calcs) based on the size of the bitmap
static const double zoom_min = 1e-4;
static const double zoom_max = 1e3;

void CPrevwView::OnUpdateZoomIn(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom_ > zoom_min);
}

void CPrevwView::OnUpdateZoomOut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom_ < zoom_max);
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

#if 0
	::StretchDIBits(pDC->GetSafeHdc(),
					dl, dt, dr - dl, db - dt,
					sl, bih->biHeight - sb, sr - sl, sb - st,
					FreeImage_GetBits(pDoc->preview_dib_), FreeImage_GetInfo(pDoc->preview_dib_),
					DIB_RGB_COLORS, SRCCOPY);
#else
	// Create temp bitmap of same size as window

	// Fill with background and BLT the bitmap into it with alpha channel
	//FillRect();
	AlphaBlend();

	// BLT the temp bitmap to the display (including flipping vert/horiz)
#endif
}

void CPrevwView::validate_display()
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	// eEt size of the display area (client area of the window)
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
	}

	int width = int(bmwidth/zoom_);

	// Check if the displayed bitmap width is less than the window width
	if (width < rct.Width())
	{
		// Centre it between the left and right side
		pos_.x = (rct.Width()  - width)/2;
	}
	else if (pos_.x + width < rct.Width())
	{
		// Right edge should not be left of window's right edge
		pos_.x = rct.Width() - width;
	}
	else if (pos_.x > 0)
	{
		// Bitmap's left edge should not be any further right than the left edge of the window
		pos_.x = 0;
	}

	int height = int(bmheight/zoom_);

	// Check if the displayed height is less than the window height
	if (height < rct.Height())
	{
		// Centre it vertically between top and bottom edges
		pos_.y = (rct.Height()  - height)/2;
	}
	else if (pos_.y + height < rct.Height())
	{
		// Displayed bitmap's bottom edge should be at or below window bottom
		pos_.y = rct.Height() - height;
	}
	else if (pos_.y > 0)
	{
		// Displayed top edge should be at or above the window top
		pos_.y = 0;
	}
}

void CPrevwView::zoom(double amount)
{
	double prev_zoom = zoom_;
	zoom_ *= amount;

	// Keep within a sensible range
	if (zoom_ < zoom_min)
		zoom_ = zoom_min;
	else if (zoom_ > zoom_max)
		zoom_ = zoom_max;

	// Adjust position so that zoom occurs around the centre of the window
	if (zoom_ != prev_zoom)
	{
		// Gett size of the display area (client area of the window)
		CRect rct;
		GetClientRect(&rct);

		pos_.x = rct.Width()/2  - int((rct.Width()/2  - pos_.x) * prev_zoom / zoom_);
		pos_.y = rct.Height()/2 - int((rct.Height()/2 - pos_.y) * prev_zoom / zoom_);
	}
}
