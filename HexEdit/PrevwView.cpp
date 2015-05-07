// PrevwView.cpp : implements CPrevwView
//

#include "stdafx.h"
#include "HexEdit.h"
#include "PrevwView.h"
#include "HexFileList.h"    // used to retrieve settings

// TODO
// Code
//  > handle alpha channel
//  > double-buffer to addresses flickering
//    > scroll window when dragging to stop flicker
//  - fix border if bpp < 32 (currently black)
//  - scrollbars??
//  - properties (disk/memory): format, dimensions, bpp
// Maybe later
//  - copy to clipboard command??
//  - commands to rotate/flip the view of the bitmap
//    - ie. alpha blend (possibly large) FreeImage bitmap into window sized (smaller) hidden bitmap then rotate/flip that before/while blt to screen
//    - flip vert/horiz (maybe use FreeImage_FlipHorizontal during render process)
//    - rotate command (Note: 90/270 requires swap of bitmap heigh/width in calcs)
// Test
//  > zoom min/max with different sized bitmaps
//  - BLT boundary conditions

IMPLEMENT_DYNCREATE(CPrevwView, CView)

// TODO we should be able to calculate better values (eg, that avoid overflow in
// bitmap calcs) based on the size of the bitmap
static const double zoom_min = 1e-4;
static const double zoom_max = 1e3;

CPrevwView::CPrevwView() : phev_(NULL), zoom_(0.0), background_(WHITE), mouse_down_(false)
{
}

CPrevwView::~CPrevwView()
{
}

BEGIN_MESSAGE_MAP(CPrevwView, CView)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()

	ON_COMMAND(ID_PREVW_ZOOM_IN, OnZoomIn)
	ON_UPDATE_COMMAND_UI(ID_PREVW_ZOOM_IN, OnUpdateZoomIn)
	ON_COMMAND(ID_PREVW_ZOOM_OUT, OnZoomOut)
	ON_UPDATE_COMMAND_UI(ID_PREVW_ZOOM_OUT, OnUpdateZoomOut)
	ON_COMMAND(ID_PREVW_ZOOM_ACTUAL, OnZoomActual)
	ON_COMMAND(ID_PREVW_ZOOM_FIT, OnZoomFit)
	ON_COMMAND(ID_PREVW_BG_CHECK, OnBackgroundCheck)
	ON_UPDATE_COMMAND_UI(ID_PREVW_BG_CHECK, OnUpdateBackgroundCheck)
	ON_COMMAND(ID_PREVW_BG_WHITE, OnBackgroundWhite)
	ON_UPDATE_COMMAND_UI(ID_PREVW_BG_WHITE, OnUpdateBackgroundWhite)
	ON_COMMAND(ID_PREVW_BG_BLACK, OnBackgroundBlack)
	ON_UPDATE_COMMAND_UI(ID_PREVW_BG_BLACK, OnUpdateBackgroundBlack)
	ON_COMMAND(ID_PREVW_BG_GREY, OnBackgroundGrey)
	ON_UPDATE_COMMAND_UI(ID_PREVW_BG_GREY, OnUpdateBackgroundGrey)
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

	// Get last preview view display options for thsi file from pfl
	zoom_  = atof(pfl->GetData(recent_file_index, CHexFileList::PREVIEWZOOM)) / 1e4;
	if (zoom_ < zoom_min || zoom_ > zoom_max)
		zoom_ = 0.0;
	pos_.x = atoi(pfl->GetData(recent_file_index, CHexFileList::PREVIEWX));
	pos_.y = atoi(pfl->GetData(recent_file_index, CHexFileList::PREVIEWY));
	background_ = background_t(atoi(pfl->GetData(recent_file_index, CHexFileList::PREVIEWBG)));

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
		phev_->show_prop();
	}
}

void CPrevwView::OnDraw(CDC* pDC)
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL || pDoc->preview_dib_ == NULL || zoom_ == 0.0)
	{
		pDC->SetBkMode(TRANSPARENT);

		CRect cli;
		GetClientRect(&cli);
		pDC->DPtoLP(&cli);
		pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		pDC->DrawText("Unrecognized format or corrupt file", -1, &cli, DT_CENTER | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		return;
	}
	draw_bitmap(pDC);
}

void CPrevwView::StoreOptions(CHexFileList *pfl, int idx)
{
	pfl->SetData(idx, CHexFileList::PREVIEWZOOM, __int64(zoom_ * 1e4));
	pfl->SetData(idx, CHexFileList::PREVIEWX, pos_.x);
	pfl->SetData(idx, CHexFileList::PREVIEWY, pos_.y);
	pfl->SetData(idx, CHexFileList::PREVIEWBG, background_);
}

// CPrevwView message handlers
void CPrevwView::OnSize(UINT nType, int cx, int cy)
{
	if (phev_ == NULL) return;

	if (cx > 0 && cy > 0)
		validate_display();

	CView::OnSize(nType, cx, cy);    // this should invalidate -> redraw with new values
}

BOOL CPrevwView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		CHexEditDoc *pDoc = GetDocument();
		ASSERT(pDoc != NULL);

		CRect rct;
		GetClientRect(&rct);

		if (zoom_ < zoom_fit())
		{
			//if (mouse_down_)
			//	::SetCursor(theApp.LoadCursor(IDC_HANDGRAB));        // indicates user is dragging the image
			//else
				::SetCursor(theApp.LoadCursor(IDC_HANDOPEN));        // can be dragged
			return TRUE;
		}
	}

	return CView::OnSetCursor(pWnd, nHitTest, message);
}

void CPrevwView::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);

	if (mouse_down_)
	{
		CPoint saved_pos(pos_);
		pos_.x += (point.x - mouse_point_.x);
		pos_.y += (point.y - mouse_point_.y);

		validate_display();         // keep zoom/position within allowed limits
		if (pos_ != saved_pos)
		{
			ScrollWindow( pos_.x - saved_pos.x, pos_.y - saved_pos.y);
			//UpdateWindow();
		}

		mouse_point_ = point;
	}
}

void CPrevwView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (zoom_ < zoom_fit())
	{
		mouse_down_ = true;
		mouse_point_ = point;
		SetCapture();                 // Capture the mouse so user can drag past edges (and so we get a mouse up event)
		::SetCursor(theApp.LoadCursor(IDC_HANDGRAB));        // indicates user is dragging the image
	}

	CView::OnLButtonDown(nFlags, point);
}

void CPrevwView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (mouse_down_)
	{
		mouse_down_ = false;
		ReleaseCapture();
	}

	CView::OnLButtonUp(nFlags, point);
}

void CPrevwView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//mouse_down_ = true;

	CView::OnLButtonDblClk(nFlags, point);
}

void CPrevwView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CHexEditDoc *pDoc = GetDocument();

	//if (nChar <= VK_CONTROL) return;  // This helps when debugging Ctrl+key

	CPoint saved_pos(pos_);
	double saved_zoom = zoom_;

	switch (nChar)
	{
	case VK_UP:
		pos_.y += 10;
		break;
	case VK_DOWN:
		pos_.y -= 10;
		break;
	case VK_LEFT:
		pos_.x += 10;
		break;
	case VK_RIGHT:
		pos_.x -= 10;
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
	if (zoom_ != saved_zoom)
		Invalidate();
	else if (pos_ != saved_pos)
		ScrollWindow(pos_.x - saved_pos.x, pos_.y - saved_pos.y);

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CPrevwView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CPoint saved_pos(pos_);
	double saved_zoom = zoom_;

	if ((nFlags & MK_CONTROL) != 0)  // If Ctrl key down we zoom instead of scroll
	{
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
	if (zoom_ != saved_zoom)
		Invalidate();
	else if (pos_ != saved_pos)
		ScrollWindow(pos_.x - saved_pos.x, pos_.y - saved_pos.y);
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

void CPrevwView::OnUpdateZoomIn(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom_ > zoom_min);
}

void CPrevwView::OnZoomOut()
{
	zoom(1.5);
	validate_display();
	Invalidate();
}

void CPrevwView::OnUpdateZoomOut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom_ < zoom_max);
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

void CPrevwView::OnBackgroundCheck()
{
	background_ = CHECKERBOARD;
	Invalidate();
}

void CPrevwView::OnUpdateBackgroundCheck(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(background_ == CHECKERBOARD);
	CHexEditDoc *pDoc = GetDocument();
	pCmdUI->Enable(pDoc!= NULL && FreeImage_GetBPP(pDoc->preview_dib_) >= 32);
}

void CPrevwView::OnBackgroundWhite()
{
	background_ = WHITE;
	Invalidate();
}

void CPrevwView::OnUpdateBackgroundWhite(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(background_ == WHITE);
	CHexEditDoc *pDoc = GetDocument();
	pCmdUI->Enable(pDoc!= NULL && FreeImage_GetBPP(pDoc->preview_dib_) >= 32);
}

void CPrevwView::OnBackgroundBlack()
{
	background_ = BLACK;
	Invalidate();
}

void CPrevwView::OnUpdateBackgroundBlack(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(background_ == BLACK);
	CHexEditDoc *pDoc = GetDocument();
	pCmdUI->Enable(pDoc!= NULL && FreeImage_GetBPP(pDoc->preview_dib_) >= 32);
}

void CPrevwView::OnBackgroundGrey()
{
	background_ = GREY;
	Invalidate();
}

void CPrevwView::OnUpdateBackgroundGrey(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(background_ == GREY);
	CHexEditDoc *pDoc = GetDocument();
	pCmdUI->Enable(pDoc!= NULL && FreeImage_GetBPP(pDoc->preview_dib_) >= 32);
}

// Drawing routines
BOOL CPrevwView::OnEraseBkgnd(CDC* pDC)
{
	CRect rct;
	pDC->GetClipBox(&rct);                       // only erase the area that needs it

	pDC->FillSolidRect(&rct, afxGlobalData.clrBtnFace);
	return TRUE;
}

void CPrevwView::draw_bitmap(CDC* pDC)
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	CRect rct;
	pDC->GetClipBox(&rct);                       // only BLIT the area that needs it

	if (pDoc == NULL || rct.IsRectEmpty())
		return;

	ASSERT(pDoc->preview_width_ > 0 && pDoc->preview_height_ > 0);

	int sl = (rct.left - pos_.x) * zoom_;
	if (sl < 0) sl = 0;
	int st = (rct.top - pos_.y) * zoom_;
	if (st < 0) st = 0;
	int sr = (rct.right - pos_.x) * zoom_ + 1;
	if (sr > pDoc->preview_width_) sr = pDoc->preview_width_;
	int sb = (rct.bottom - pos_.y) * zoom_ + 1;
	if (sb > pDoc->preview_height_) sb = pDoc->preview_height_;

	int dl = sl/zoom_ + pos_.x;
	int dt = st/zoom_ + pos_.y;
	int dr = sr/zoom_ + pos_.x;
	int db = sb/zoom_ + pos_.y;

	// Create an off-screen bitmap for double-buffering (remove annoying flickering)
	// This is slightly bigger than the clip area (extra at top and left) so the checkerboard  is
	// always drawn at the same relative offset to pos_ (by using %32 since 32 is size of repeating pattern)
	// Note we add 65535 (=65536-1) to make sure modulus (%) is performed on a +ve integer (xxx is that big enough?)
	int xoff = (rct.left - pos_.x + 0xFFFFFF)%32, yoff = (rct.top - pos_.y + 0xFFFFFF)%32;
	CDC BufDC;
	CBitmap BufBM;
	BufDC.CreateCompatibleDC(pDC);
	BufBM.CreateCompatibleBitmap(pDC, rct.Width() + xoff, rct.Height() + yoff);
	CBitmap* pOldBM = BufDC.SelectObject(&BufBM);
	//BufDC.SetWindowOrg(0, 0);

	// For 32-bit bitmaps we use AlphaBlend to preserve the alpha channel (transparency)
	// I worked out how to do this using example code at http://sourceforge.net/p/freeimage/discussion/36111/thread/3ed64628/
	if (::GetDeviceCaps(pDC->GetSafeHdc(), BITSPIXEL) >= 32 && FreeImage_GetBPP(pDoc->preview_dib_) >= 32)
	{
		CRect fillRect(0, 0, rct.Width() + xoff, rct.Height() + yoff);

		// Fill the background
		switch (background_)
		{
		default:
		case CHECKERBOARD:
			{
				static unsigned char HatchBits[32*4] = {0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 
														0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, };
				CBitmap bm;
				bm.CreateBitmap(32, 32, 1, 1, HatchBits);
				CBrush brush;
				brush.CreatePatternBrush(&bm);
				BufDC.SetTextColor(RGB(128,128,128));
				BufDC.FillRect(&fillRect, &brush);
			}
			break;
		case WHITE:
			BufDC.FillSolidRect(&fillRect, RGB(255,255,255));
			break;;
		case BLACK:
			BufDC.FillSolidRect(&fillRect, RGB(0,0,0));
			break;
		case GREY:
			BufDC.FillSolidRect(&fillRect, afxGlobalData.clrBtnFace);
			break;
		}

		// Create temp bitmap - size of clip area
		CDC MemDC;
		CBitmap bm;
		MemDC.CreateCompatibleDC(pDC);
		bm.CreateCompatibleBitmap(pDC,  rct.Width(), rct.Height());
		CBitmap* pOldBitmap = MemDC.SelectObject(&bm);
		//MemDC.SetWindowOrg(0, 0);

		// Get the bitmap required by AlphBlend
		FIBITMAP * dib = FreeImage_Copy(pDoc->preview_dib_, 0, 0, pDoc->preview_width_, pDoc->preview_height_);
		FreeImage_PreMultiplyWithAlpha(dib);
		::StretchDIBits(MemDC.GetSafeHdc(),
						dl - rct.left, dt - rct.top, dr - dl, db - dt,
						sl, pDoc->preview_height_ - sb, sr - sl, sb - st,
						FreeImage_GetBits(dib), FreeImage_GetInfo(dib),
						DIB_RGB_COLORS, SRCCOPY);

		BLENDFUNCTION blend = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
		BufDC.AlphaBlend(xoff, yoff, rct.Width(), rct.Height(), &MemDC, 0, 0, rct.Width(), rct.Height(), blend);

		// Cleanup
		FreeImage_Unload(dib);
		MemDC.SelectObject(pOldBitmap);
		bm.DeleteObject();
		MemDC.DeleteDC();
	}
	else
	{
		// Copy all or part of the FreeImage bitmap to the display
		::StretchDIBits(BufDC.GetSafeHdc(),
						dl - rct.left + xoff, dt - rct.top + yoff, dr - dl, db - dt,
						sl, pDoc->preview_height_ - sb, sr - sl, sb - st,
						FreeImage_GetBits(pDoc->preview_dib_), FreeImage_GetInfo(pDoc->preview_dib_),
						DIB_RGB_COLORS, SRCCOPY);
	}

	pDC->BitBlt(rct.left, rct.top, rct.Width(), rct.Height(), &BufDC, xoff, yoff, SRCCOPY);
	BufDC.SelectObject(pOldBM);
	BufBM.DeleteObject();
	BufDC.DeleteDC();
}

void CPrevwView::validate_display()
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL || pDoc->preview_dib_ == NULL)
		return;

	// Get size of the display area (client area of the window)
	CRect rct;
	GetClientRect(&rct);

	// zoom of zero is probably due to first display
	if (zoom_ <= 0.0)
	{
		// Work out zoom so the whole bitmap fits in the display
		zoom_ = zoom_fit();

		// Move to top-left - code below will centre within the display
		pos_.x = pos_.y = 0;
	}

	int width = int(pDoc->preview_width_/zoom_);

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

	int height = int(pDoc->preview_height_/zoom_);

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
		// Get size of the display area (client area of the window)
		CRect rct;
		GetClientRect(&rct);

		pos_.x = rct.Width()/2  - int((rct.Width()/2  - pos_.x) * prev_zoom / zoom_);
		pos_.y = rct.Height()/2 - int((rct.Height()/2 - pos_.y) * prev_zoom / zoom_);
	}
}

// Calculate zoom level where the bitmap just fits inside the display
double CPrevwView::zoom_fit()
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	// Get size of the display area (client area of the window)
	CRect rct;
	GetClientRect(&rct);

	// Get dimensions of the bitmap (as double so we can calculate zoom)
	double bmwidth  = pDoc->preview_height_;
	double bmheight = pDoc->preview_height_;

	return max(bmwidth/rct.Width(), bmheight/rct.Height());
}

