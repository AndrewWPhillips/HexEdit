// AerialView.cpp : implementation file 
//
// Copyright (c) 2007-2010 by Andrew W. Phillips.
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

#include "stdafx.h"
#include "HexEdit.h"
#include "AerialView.h"
#include "HexFileList.h"
#include "Misc.h"
#include "Boyer.h"

// TODO for aerial view
// - autosync/scroll?
// - global options: default dpix?, max bitmap size (range 16 Mb to size of physical memory?)
// - ability to print the bitmap

static bool timer_on = true;  // allow turning off of ants for testing things like invalidation

// CAerialView
IMPLEMENT_DYNCREATE(CAerialView, CView)

CAerialView::CAerialView()
{
	phev_ = NULL;
	scrollpos_ = -1;
	actual_dpix_ = -1;
	mouse_down_ = false;
	rows_ = cols_ = -1;

	bdr_left_ = 11;
	bdr_top_ = 3;
	bdr_right_ = bdr_bottom_ = 0;
	disp_.dpix = 1;     // set later but make non-zero for safety
	sbfact_ = 1;        // default to normal scrollbar units

	last_tip_elt_ = -1;

	// We use a timer to update the "marching ants" border, which is a cycle of 12 pixels: 4 pixels
	// of one ant colour, 2 blanks, 4 pixels of another ant colour, 2 blanks, then repeat
	// (****  ====  ****  ====  ...). Updating at 3 times the caret blink rate means when there
	// is no selection the pixel for the current cursor posn will blink at the caret blink rate.
	timer_id_ = 0;
	timer_msecs_ = ::GetCaretBlinkTime()/10;   // increased ant speed after double-buffering added
	bfactor_ = 1.0;
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
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)

	// Our special commands
	ON_COMMAND(ID_VIEWTEST, OnViewTest)  // this is for testing only

	// These are for toggling display options
	ON_COMMAND(ID_IND_SEL, OnIndSelection)
	ON_UPDATE_COMMAND_UI(ID_IND_SEL, OnUpdateIndSelection)
	ON_COMMAND(ID_IND_MARK, OnIndMark)
	ON_UPDATE_COMMAND_UI(ID_IND_MARK, OnUpdateIndMark)
	ON_COMMAND(ID_IND_HL, OnIndHighlights)
	ON_UPDATE_COMMAND_UI(ID_IND_HL, OnUpdateIndHighlights)
	ON_COMMAND(ID_IND_SEARCH, OnIndSearch)
	ON_UPDATE_COMMAND_UI(ID_IND_SEARCH, OnUpdateIndSearch)
	ON_COMMAND(ID_IND_BM, OnIndBookmarks)
	ON_UPDATE_COMMAND_UI(ID_IND_BM, OnUpdateIndBookmarks)
	ON_COMMAND(ID_ANT_SEL, OnAntSelection)
	ON_UPDATE_COMMAND_UI(ID_ANT_SEL, OnUpdateAntSelection)
	ON_COMMAND(ID_ANT_MARK, OnAntMark)
	ON_UPDATE_COMMAND_UI(ID_ANT_MARK, OnUpdateAntMark)
	ON_COMMAND(ID_ANT_HL, OnAntHighlights)
	ON_UPDATE_COMMAND_UI(ID_ANT_HL, OnUpdateAntHighlights)
	ON_COMMAND(ID_ANT_SEARCH, OnAntSearch)
	ON_UPDATE_COMMAND_UI(ID_ANT_SEARCH, OnUpdateAntSearch)
	ON_COMMAND(ID_ANT_BM, OnAntBookmarks)
	ON_UPDATE_COMMAND_UI(ID_ANT_BM, OnUpdateAntBookmarks)
	ON_COMMAND(ID_AERIAL_ZOOM1, OnZoom1)
	ON_COMMAND(ID_AERIAL_ZOOM2, OnZoom2)
	ON_COMMAND(ID_AERIAL_ZOOM3, OnZoom3)
	ON_COMMAND(ID_AERIAL_ZOOM4, OnZoom4)
	ON_COMMAND(ID_AERIAL_ZOOM5, OnZoom5)
	ON_COMMAND(ID_AERIAL_ZOOM6, OnZoom6)
	ON_COMMAND(ID_AERIAL_ZOOM7, OnZoom7)
	ON_COMMAND(ID_AERIAL_ZOOM8, OnZoom8)
	ON_COMMAND(ID_AERIAL_ZOOM9, OnZoom9)
	ON_COMMAND(ID_AERIAL_ZOOM10, OnZoom10)
	ON_COMMAND(ID_AERIAL_ZOOM11, OnZoom11)
	ON_COMMAND(ID_AERIAL_ZOOM12, OnZoom12)
	ON_COMMAND(ID_AERIAL_ZOOM13, OnZoom13)
	ON_COMMAND(ID_AERIAL_ZOOM14, OnZoom14)
	ON_COMMAND(ID_AERIAL_ZOOM15, OnZoom15)
	ON_COMMAND(ID_AERIAL_ZOOM16, OnZoom16)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM1, OnUpdateZoom1)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM2, OnUpdateZoom2)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM3, OnUpdateZoom3)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM4, OnUpdateZoom4)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM5, OnUpdateZoom5)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM6, OnUpdateZoom6)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM7, OnUpdateZoom7)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM8, OnUpdateZoom8)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM9, OnUpdateZoom9)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM10, OnUpdateZoom10)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM11, OnUpdateZoom11)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM12, OnUpdateZoom12)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM13, OnUpdateZoom13)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM14, OnUpdateZoom14)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM15, OnUpdateZoom15)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_ZOOM16, OnUpdateZoom16)

	// Disable commands that we do not want passed onto the hex view as they
	// are confusing or we may want to implement them for this view one day.
		ON_UPDATE_COMMAND_UI(ID_DEL, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COMPARE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateDisable)

		ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_DIRECT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, OnUpdateDisable)


END_MESSAGE_MAP()

#define ANT_SIZE  12    // Number of pixels from the start of one marching ant to the next
#define MAX_DPIX  16    // Max zoom is each elt shown as 7x7 square (note that this is related to the size of the dpix bit-field)

///////////////////////////////////////////////////////////////////////////////
// CAerialView drawing

// Note that the FreeImage bitmap is a "summary" of the file where one pixel of
// the bitmap corresponds to one or more bytes of the file (see BPE - bytes/elt).
// The actual display of the bitmap in the view's window requires BLT from the
// FreeImage bitmap - one bitmap pixel may be BLTed into a block of pixels in the
// window (ie single pixel, 2x2, 3x3 ... 7x7) as determined by actual_dpix_.
// Further is the whole bitmap cannot be displayed we only show part of it
// as determined by scrollpos_, rows_, and cols_.
// To avoid confusion we use the following names:
// scrollpos_ = posn in the file of the top left pixel - always divisible by width (below)
// endpos     = posn in the file of the pixel below the bottom left one - always div by width
// bpe        = how many bytes of the file are in each "elt" of the FileImage bitmap
// actual_dpix_ = actual display pixel size - one elt is BLTed into a square of this dimension
// cols_      = # of elts across a row of the view - always divisible by 8
//              when multiplied by actual_dpix_ gives the number pixels used to display a row
// width      = # of file bytes across a row = BPE * cols_
// rows_      = vertical # of elts displayed in the view
//              when multiplied by actual_dpix_ gives the number pixels used to display a column

// So we have 5 "coordinate" systems:
// A) file address
// B) elt as sequential position in FreeImage bitmap
// C) elt as pixel in FreeImage bitmap - varies depending on how the bitmap has been re-shaped
// D) elt as pixel on the screen = C shifted vertically
// W) windows coords in pixels
// A to B: divide by BPE
// B to A: multiply by BPE
// B to C: X = elt % cols_  Y = elt / cols_
// B to D: X = elt % cols_  Y = elt / cols_ - scrollpos_/cols_
// C to B: elt = Y * cols_ + X
// C to W: x = X*actual_dpix_;
//         y = (Y - scrollpos_/BPE/cols_)*actual_dpix_
// W to C: X = x/actual_dpix_
//         Y = y/actual_dpix_ + scrollpos_/BPE/cols_
// B to W: x = (elt % cols_)*actual_dpix_
//         y = ((elt - scrollpos_/BPE)/cols_)*actual_dpix_
// W to B: elt = scrollpos_/BPE + (y/actual_dpix_)*cols_ + (x/actual_dpix_)
// A to W: x = (address % width)*actual_dpix_
//         y = ((address - scrollpos_)/width)*actual_dpix_
// W to A: address = scrollpos_ + (y/actual_dpix_)*width + (x/actual_dpix_)*BPE

void CAerialView::OnInitialUpdate() 
{
	ASSERT(phev_ != NULL);
	GetDocument()->AddAerialView(phev_);

	// Get last display option
	CHexFileList *pfl = theApp.GetFileList();
	int recent_file_index = -1;
	if (GetDocument()->pfile1_ != NULL)
		recent_file_index = pfl->GetIndex(GetDocument()->pfile1_->GetFilePath());

	// Get aerial view display options (disp_)
	disp_state_ = atoi(pfl->GetData(recent_file_index, CHexFileList::AERIALDISPLAY));
	scrollpos_ = _atoi64(pfl->GetData(recent_file_index, CHexFileList::AERIALPOS));
	if (disp_.dpix < 1 || disp_.dpix > MAX_DPIX) disp_.dpix = 1; // make sure we don't get div0 errors

	get_disp_params(rows_, cols_, actual_dpix_);

	CView::OnInitialUpdate();

	// Calculate initial scroll position and display parameters
	get_disp_params(rows_, cols_, actual_dpix_);
	//scrollpos_ = 0;
	update_bars();
	update_display();

	phev_->GetSelAddr(prev_start_addr_, prev_end_addr_);
	timer_count_ = 0;
	StartTimer();
}

// OnUpdate is called when the document changes
// - background calc of elts (FreeImage bitmap) is finished (or is finished for display area)
// - doc changes requiring recalc of the elts
// - bpe changes requiring recalc of the elts
// - background search finished (CBGSearchHint) - perhaps display bg search occurrences
// - bookmark change (CBookmarkHint) - perhaps update display of the bookmark
void CAerialView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (pHint == NULL)
		return;

	if (pHint->IsKindOf(RUNTIME_CLASS(CHexHint)) ||
		pHint->IsKindOf(RUNTIME_CLASS(CBGSearchHint)) ||
		pHint->IsKindOf(RUNTIME_CLASS(CBookmarkHint)) ||
		pHint->IsKindOf(RUNTIME_CLASS(CBGAerialHint))   )
	{
		// Document, search occurrences, or bookmarks have changed
		get_disp_params(rows_, cols_, actual_dpix_);  // file length may have changed which could change actual_dpix_
		update_display();
		Invalidate();
	}
}

// OnDraw draws the bitmap then other stuff (like the selection) on top
void CAerialView::OnDraw(CDC* pDC)
{
	if (actual_dpix_ == -1) return;          // Seems to happen at startup
	int width = GetDocument()->GetBpe() * cols_;

	if (!pDC->IsPrinting())
	{
		// Flash top-left corner rect until bg scanning is finished
		COLORREF clr;
		if (GetDocument()->AerialScanning())
		{
			switch (timer_count_ % 3)
			{
			case 0:
				clr = RGB(0,0,0);
				break;
			case 1:
				clr = RGB(128,128,128);
				break;
			case 2:
				clr = RGB(255,255,255);
				break;
			}
		}
		else
		{
			if ((clr = phev_->GetBackgroundCol()) == -1)
				clr = RGB(192,192,192);
		}
		CRect rct(0, 0, bdr_left_, bdr_top_);
		CBrush brush;
		brush.CreateSolidBrush(clr);
		brush.UnrealizeObject();
		pDC->FillRect(&rct, &brush);
	}

	// TODO When printing (pDC->IsPrinting()) use bigger value for actual_dpix_ scaled for printer DPI

	// Set up for finding what lines bookmarks, search occurrences etc are on
	COLORREF clr_sel, clr_mark, clr_hl, clr_search, clr_bm, dummy;
	FILE_ADDRESS start_addr, end_addr;
	if (disp_.draw_bdr_sel)
	{
		phev_->GetSelAddr(start_addr, end_addr);
		if (end_addr <= start_addr) end_addr = start_addr + 1;
		clr_sel = RGB(0,0,0);
	}
	if (disp_.draw_bdr_mark)
	{
		get_colours(phev_->GetMarkCol(), clr_mark, dummy);
	}
	range_set<FILE_ADDRESS>::range_t::iterator phl;
	if (disp_.draw_bdr_hl)
	{
		// Find the first highlight 
		for (phl = phev_->hl_set_.range_.begin();
			 phl != phev_->hl_set_.range_.end();
			 ++phl)
		{
			if (phl->slast > scrollpos_)
				break;
		}
		get_colours(phev_->GetHighlightCol(), dummy, clr_hl);
	}
	std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator psp; // iter into search_pair_
	if (disp_.draw_bdr_search)
	{
		psp = search_pair_.begin();
		get_colours(phev_->GetSearchCol(), clr_search, dummy);
	}
	std::vector<FILE_ADDRESS> bm;
	std::vector<FILE_ADDRESS>::const_iterator pbm;
	if (disp_.draw_bdr_bm)
	{
		bm = GetDocument()->bm_posn_;
		std::sort(bm.begin(), bm.end());
		pbm = bm.begin();
		while (pbm != bm.end() && *pbm < scrollpos_)
			++pbm;
		get_colours(phev_->GetBookmarkCol(), clr_bm, dummy);
	}
	// For each row of the bitmap draw zero or more indicators in the left border for various things
	bool ind_sel, ind_mark, ind_hl, ind_search, ind_bm;
	FILE_ADDRESS endpos = scrollpos_ + width*rows_;
	for (FILE_ADDRESS pp = scrollpos_; pp < endpos; pp += width)
	{
		// First work out which indicators we are showing for this line
		ind_sel = ind_mark = ind_hl = ind_search = ind_bm = false;

		if (disp_.draw_bdr_sel && end_addr > pp && start_addr < pp + width)
			ind_sel = true;

		if (disp_.draw_bdr_mark && phev_->GetMark() >= pp && phev_->GetMark() < pp + width)
			ind_mark = true;

		if (disp_.draw_bdr_hl && phl != phev_->hl_set_.range_.end() && phl->sfirst < pp + width)
		{
			ind_hl = true;
			// Skip this highlight (and end following) if they end before the end of this line
			while (phl != phev_->hl_set_.range_.end() && phl->slast <= pp + width)
				++phl;
		}

		if (disp_.draw_bdr_search && psp != search_pair_.end() && psp->first < pp + width)
		{
			ind_search = true;
			// Skip forward over this and any other search occurrences on this line
			while (psp != search_pair_.end() && psp->second <= pp + width)
				++psp;
		}

		if (disp_.draw_bdr_bm && pbm != bm.end() && *pbm < pp + width)
		{
			ind_bm = true;
			// Skip forward over this and any other bookmarks on this line
			while (pbm != bm.end() && *pbm < pp + width)
				++pbm;
		}

		// We draw 2 columns of pixels for each indicator, but if either adjacent indicator
		// is not drawn we can spread into it by 1 pixel, except that the left and
		// right indicators outside the 10 pixel indicator area.
		int row = int(pp - scrollpos_)/width;
		if (ind_sel)
			draw_left_border(pDC, 0,           3, row, clr_sel);            //      0, 1, (2)
		if (ind_mark)
			draw_left_border(pDC, ind_sel    ? 2 : 1, 5, row, clr_mark);    // (1), 2, 3, (4)
		if (ind_hl)
			draw_left_border(pDC, ind_mark   ? 4 : 3, 7, row, clr_hl);      // (3), 4, 5, (6)
		if (ind_search)
			draw_left_border(pDC, ind_hl     ? 6 : 5, 9, row, clr_search);  // (5), 6, 7, (8)
		if (ind_bm)
			draw_left_border(pDC, ind_search ? 8 : 7, 10, row, clr_bm);     // (7), 8, 9

	}
	// Show selection in the top border
	if (disp_.draw_bdr_sel)
	{
		if (end_addr - start_addr > width)
		{
			draw_top_border(pDC, 0, cols_, clr_sel);
		}
		else
		{
			int left = int(start_addr/GetDocument()->GetBpe()) %cols_;
			int right = int((end_addr-1)/GetDocument()->GetBpe()) % cols_ + 1;
			if (left < right)
				draw_top_border(pDC, left, right, clr_sel);
			else
			{
				draw_top_border(pDC, left, cols_, clr_sel);
				draw_top_border(pDC, 0, right, clr_sel);
			}
		}
	}

	// Work out what part of the display to draw the bitmap in
	// Adjusting the clip rect here simplifies later calculations.
	CRect rct;
	rct.left   = bdr_left_;
	rct.right  = rct.left + cols_ * actual_dpix_;
	rct.top    = bdr_top_;
	rct.bottom = rct.top + rows_ * actual_dpix_;
	pDC->IntersectClipRect(&rct);

	{
		// Use memory DC for double buffering.  (Will render to pDC when destructed.)
		CMemDC bufDC(*pDC, this);

		t0_.restart();
		draw_bitmap(&(bufDC.GetDC()));
		t0_.stop();

		if (disp_.draw_ants_sel)
		{
			// Draw marching ants for selection
			FILE_ADDRESS start_addr, end_addr;
			phev_->GetSelAddr(start_addr, end_addr);
			if (end_addr == start_addr) ++end_addr;    // Make sure we draw something at the caret position
			draw_ants(&(bufDC.GetDC()), start_addr, end_addr, RGB(0,0,0));  // black ants
		}
		if (disp_.draw_ants_mark)
		{
			// Draw ants around mark (this just regularly changes the pixel when actual_dpix_ == 1)
			draw_ants(&(bufDC.GetDC()), phev_->GetMark(), phev_->GetMark() + 1, phev_->GetMarkCol());
		}
		if (disp_.draw_ants_hl)
		{
			// Draw ants for highlights
			range_set<FILE_ADDRESS>::range_t::const_iterator pr;
			for (pr = phev_->hl_set_.range_.begin(); pr != phev_->hl_set_.range_.end(); ++pr)
				draw_ants(&(bufDC.GetDC()), pr->sfirst, pr->slast, phev_->GetHighlightCol());  // highlight colour ants (yellow?)
		}
		if (disp_.draw_ants_search && theApp.pboyer_ != NULL)
		{
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator psp; // iter into search_pair_
			for (psp = search_pair_.begin(); psp != search_pair_.end(); ++psp)
				draw_ants(&(bufDC.GetDC()), psp->first, psp->second, phev_->GetSearchCol());
		}
		if (disp_.draw_ants_bm)
		{
			for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
				 pbm != GetDocument()->bm_posn_.end(); ++pbm)
			{
				if (*pbm >= scrollpos_ && *pbm <= scrollpos_ + width*rows_)
					draw_ants(&(bufDC.GetDC()), *pbm, *pbm + 1, phev_->GetBookmarkCol());
			}
		}
	}
}

BOOL CAerialView::OnEraseBkgnd(CDC* pDC)
{
	if (phev_ == NULL || scrollpos_ < 0) return FALSE;          // Seems to happen at startup

	// Fill with the background colour
	COLORREF bg_col;
	if ((bg_col = phev_->GetBackgroundCol()) == -1)
		bg_col = RGB(192,192,192);

	// Create a solid brush of background colour
	CBrush backBrush;
	backBrush.CreateSolidBrush(bg_col);
	backBrush.UnrealizeObject();

	CRect rct;
	GetClientRect(rct);

	// Work out the max row that the BitBlt is going to fill
	BITMAPINFOHEADER *bih = FreeImage_GetInfoHeader(GetDocument()->dib_);
	int max_row = int((GetDocument()->NumElts()-1)/cols_) + 1 - 
				  int(scrollpos_/(GetDocument()->GetBpe() * cols_));

	// Fill the borders
	rct.left = bdr_left_ + cols_*actual_dpix_;
	pDC->FillRect(rct, &backBrush);             // right border
	rct.right = rct.left; rct.left = 0;
	if (rows_ > max_row)
		rct.top = bdr_top_ + max_row*actual_dpix_;   // fill bottom border and the bottom area of the bitmap not blted to
	else
		rct.top = bdr_top_ + rows_*actual_dpix_;    // just fill the bottom border
	pDC->FillRect(rct, &backBrush);             // bottom border
	rct.bottom = rct.top; rct.top = 0; rct.right = bdr_left_;
	pDC->FillRect(rct, &backBrush);             // left border
	rct.bottom = bdr_top_; rct.left = bdr_left_;
	rct.right = bdr_left_ + cols_*actual_dpix_;
	pDC->FillRect(rct, &backBrush);             // top border

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CAerialView message handlers

void CAerialView::OnDestroy() 
{
	StopTimer();
	CView::OnDestroy();
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
		newpos -= MAX_DPIX/actual_dpix_;
		break;
	case SB_LINEDOWN:
		newpos += MAX_DPIX/actual_dpix_;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newpos = nPos*sbfact_;
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
		newpos -= MAX_DPIX/actual_dpix_;
		break;
	case VK_DOWN:
		newpos += MAX_DPIX/actual_dpix_;
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
	if ((nFlags & MK_CONTROL) != 0)  // If Ctrl key down we zoom instead of scroll
	{
		// Get current mouse position in window coords
		CPoint pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		int old_elt = elt_at(pt);       // Elt which the mouse is over

		// Adjust the preferred zoom amount (disp_.dpix) and recalculate display based on the new value
		int prev_dpix = actual_dpix_;
		if (old_elt != -1)              // Only zoom if over a valid elt
		{
			bool zoomIn = zDelta > 0;
			if (theApp.reverse_zoom_) zoomIn = !zoomIn;

			// Zoom by binary multiple (ie double/half size)
			if (zoomIn)
			{
				if (disp_.dpix*2 <= MAX_DPIX)
					set_zoom(disp_.dpix*2);
				else
					set_zoom(MAX_DPIX);
			}
			else if (!zoomIn && disp_.dpix/2 > 0)    // zoom in
				set_zoom(disp_.dpix/2, false);
		}

		if (actual_dpix_ != prev_dpix)
		{
			// Work out row of old_elt in new layout then and subtract no of rows
			// to top of display to get the new scrollpos_.
			int newpos = old_elt/cols_ - (pt.y - bdr_top_)/actual_dpix_;
			// Note: scrollpos_ is now invalid (recalculated in SetScroll).  However, we need
			// to set it to this value to force SetScroll to redraw and not scroll the window.
			// We can't just invalidate the display here as scrollpos_ is inconsistent with cols_.
			scrollpos_ = INT_MIN;
			SetScroll(newpos);

			// Move the mouse pointer so it stays over the same address
			// Note that we add actual_dpix_/2 to both X and Y to put it in the centre of the elt.
			ASSERT(old_elt >= scrollpos_/GetDocument()->GetBpe());
			pt.x = bdr_left_ + old_elt%cols_ * actual_dpix_ + actual_dpix_/2;
			pt.y = bdr_top_ + (old_elt - int(scrollpos_/GetDocument()->GetBpe()))/cols_ * actual_dpix_ + actual_dpix_/2;
			ClientToScreen(&pt);
			ShowCursor(FALSE);          // we seem to get 2 cursors unless we hide before moving
			SetCursorPos(pt.x, pt.y);
			ShowCursor(TRUE);
		}
	}
	else
	{
		ASSERT(scrollpos_/(GetDocument()->GetBpe() * cols_) < INT_MAX);
		SetScroll(int(scrollpos_/(GetDocument()->GetBpe() * cols_)) - zDelta/actual_dpix_);
	}
	return TRUE;
}

void CAerialView::OnSize(UINT nType, int cx, int cy) 
{
	if (phev_ == NULL || scrollpos_ < 0) return;
	if (cx > 0 && cy > 0 && GetDocument()->GetBpe() > -1)
	{
		int width = GetDocument()->GetBpe() * cols_;        // old row width in bytes
		ASSERT(scrollpos_/width < INT_MAX);
		int pos = int(scrollpos_/width);                    // old pos as row no
		get_disp_params(rows_, cols_, actual_dpix_);
		width = GetDocument()->GetBpe() * cols_;            // new row width in bytes
		scrollpos_ = pos * width;                           // new scroll pos
		update_bars();
		update_display();
	}
	CView::OnSize(nType, cx, cy);    // this should invalidate -> redraw with new values
}

void CAerialView::OnTimer(UINT nIDEvent) 
{
	if (timer_on && nIDEvent == timer_id_ && IsWindowVisible())
	{
		t00_.reset();
		ASSERT(phev_ != NULL);
		timer_count_ = (timer_count_ + 1)%ANT_SIZE;
		if (GetDocument()->AerialScanning())
		{
			CRect rct(0, 0, bdr_left_, bdr_top_);
			InvalidateRect(&rct, FALSE);
		}

		int width = GetDocument()->GetBpe() * cols_;
		if (disp_.draw_ants_sel)
		{
			// Invalidate selection range
			FILE_ADDRESS start_addr, end_addr;
			phev_->GetSelAddr(start_addr, end_addr);
			if (end_addr == start_addr) ++end_addr;    // invalidate the single address

			// First remove the selection bounds of the old selection if it has changed
			if (start_addr != prev_start_addr_ || end_addr != prev_end_addr_)
			{
				invalidate_addr_range_boundary(prev_start_addr_, prev_end_addr_);
				prev_start_addr_ = start_addr;
				prev_end_addr_ = end_addr;
			}
			invalidate_addr_range_boundary(start_addr, end_addr);
		}
		if (disp_.draw_ants_mark)
		{
			// Draw ants around mark (this just flashes the pixel when actual_dpix_ == 1)
			invalidate_addr_range_boundary(phev_->GetMark(), phev_->GetMark() + 1);
		}
		if (disp_.draw_ants_hl)
		{
			// Find all highlights and invalidate their boundaries
			range_set<FILE_ADDRESS>::range_t::const_iterator pr;
			for (pr = phev_->hl_set_.range_.begin(); pr != phev_->hl_set_.range_.end(); ++pr)
				invalidate_addr_range_boundary(pr->sfirst, pr->slast);
		}
		if (disp_.draw_ants_search && theApp.pboyer_ != NULL && search_pair_.size() < 4096)  // if we have 1000's of ants marching everthing stops
		{
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator psp;
			for (psp = search_pair_.begin(); psp != search_pair_.end(); ++psp)
				invalidate_addr_range_boundary(psp->first, psp->second);
		}
		if (disp_.draw_ants_bm)
		{
			for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
				pbm != GetDocument()->bm_posn_.end(); ++pbm)
			{
				if (*pbm >= scrollpos_ && *pbm <= scrollpos_ + width*rows_)
					invalidate_addr_range_boundary(*pbm, *pbm + 1);
			}
		}
		t00_.stop();

		// Check if we are using more than half our allocated time
		if (t00_.elapsed() * 1000 > timer_msecs_/2)
		{
			UINT t = min((UINT)(t00_.elapsed()*1000*2), 5000);
			TRACE("timer increased from %d to %d (occ: %d)\n", timer_msecs_, t, int(search_pair_.size()));
			// Increase the timer period so we don't slow everything down
			StopTimer();
			timer_msecs_ = t;
			StartTimer();
		}
		else
		{
			// Look to see if we can speed up again
			UINT t = max((UINT)(t00_.elapsed()*1000*3), ::GetCaretBlinkTime()/10);
			if (t < timer_msecs_)
			{
				TRACE("timer decreased from %d to %d\n", timer_msecs_, t);
				// Reduce the timer period so the ants keep marching
				StopTimer();
				timer_msecs_ = t;
				StartTimer();
			}
		}
	}
	else
		CView::OnTimer(nIDEvent);
}

void CAerialView::OnKillFocus(CWnd* pNewWnd) 
{
	mouse_down_ = false;
	StopTimer();
	CView::OnKillFocus(pNewWnd);
}

void CAerialView::OnSetFocus(CWnd* pOldWnd) 
{
	StartTimer();
	CView::OnSetFocus(pOldWnd);
}

LRESULT CAerialView::OnMouseHover(WPARAM, LPARAM lp)
{
	// Time to show a tip window if we are in the right place
	CPoint pt(LOWORD(lp), HIWORD(lp));  // client window coords
	int elt = elt_at(pt);
	if (elt != -1 && elt < GetDocument()->NumElts() && update_tip(elt))
	{
		CPoint tip_pt;
		tip_pt = pt + CSize(::GetSystemMetrics(SM_CXCURSOR)/2, ::GetSystemMetrics(SM_CXCURSOR)/2); // Move tip window away from under mouse
		last_tip_elt_ = elt;
		ClientToScreen(&tip_pt);
		tip_.Move(tip_pt, false);
		tip_.Show();
		track_mouse(TME_LEAVE);
	}
	return 0;
}

LRESULT CAerialView::OnMouseLeave(WPARAM, LPARAM lp)
{
	tip_.Hide(300);
	return 0;
}

void CAerialView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CView::OnMouseMove(nFlags, point);

	if (mouse_down_)
	{
		tip_.Hide(0);                    // Don't display tip while the left button is down
	}
	else if (!tip_.IsWindowVisible())
	{
		// Start timer so that we can check if we want to display a tip window
		track_mouse(TME_HOVER);
	}
	else if (elt_at(point) != last_tip_elt_ && tip_.IsWindowVisible())
	{
		// Hide the tip window since the mouse has moved away
		if (!tip_.FadingOut())   // Don't keep it alive if we have already told it to fade away
			tip_.Hide(300);
	}
}

void CAerialView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	tip_.Hide(300);
	mouse_down_ = true;
	CView::OnLButtonDown(nFlags, point);
}

void CAerialView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	mouse_down_ = false;
	CView::OnLButtonUp(nFlags, point);
}

void CAerialView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	mouse_down_ = true;

	// Get element clicked on
	int elt = elt_at(point);
	if (elt != -1)
	{
		FILE_ADDRESS bpe = GetDocument()->GetBpe();     // use 64 bit int so next expression does not overflow
		FILE_ADDRESS addr = elt * bpe;
		FILE_ADDRESS end;
		if (bpe == 1)
			end = -1;                                   // move the caret to the single byte
		else
		{
			end = addr + bpe;                           // select the whole elt
			if (end > GetDocument()->length_) end = GetDocument()->length_;           // but don't try to select past EOF
		}

		ASSERT(phev_ != NULL);
		phev_->MoveToAddress(addr, end, -1, -1, FALSE, FALSE, 0, "Select element");
	}
}

void CAerialView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CContextMenuManager *pCMM = theApp.GetContextMenuManager();
	pCMM->ShowPopupMenu(IDR_CONTEXT_AERIAL, point.x, point.y, this);
}

// For testing
void CAerialView::OnViewTest()
{
	// Show performance info
	TRACE("=== t0 %g\n", t0_.elapsed());
	TRACE("=== t1 %g\n", t1_.elapsed());
	t0_.reset(false);
	t1_.reset(false);
}

///////////////////////////////////////////////////////////////////////////////
// CAerialView public methods

// Make sure an address is visible within the display
void CAerialView::ShowPos(FILE_ADDRESS addr)
{
	int width = GetDocument()->GetBpe() * cols_;                // row width in bytes
	int zone = 0;                                               // no of rows from top/bottom before scrolling
	if (phev_ != NULL) zone = phev_->GetVertBufferZone();
	if (zone > rows_/2) zone = rows_/2;                         // Can't be more than half window height

	if (addr < scrollpos_ + width*zone)
	{
		SetScroll(int(addr/width) - zone);
	}
	else if (addr >= scrollpos_ + width*(rows_ - zone))
	{
		SetScroll(int(addr/width) - rows_ + zone + 1);
	}
}

// Set current display position and update the scrollbar to show it
// newpos = new top row number (as rows of elts from the start of file)
void CAerialView::SetScroll(int newpos)
{
	CHexEditDoc *pDoc = GetDocument();
	int width = pDoc->GetBpe() * cols_;                 // row width in bytes
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
		rct.left   = bdr_left_;
		rct.right  = bdr_left_ + cols_ * actual_dpix_;
		rct.top    = bdr_top_;
		rct.bottom = bdr_top_ + rows_ * actual_dpix_;
		ScrollWindow(0, (pos - newpos)*actual_dpix_, rct, rct);
		rct.left   = 0; rct.right = bdr_left_;
		ScrollWindow(0, (pos - newpos)*actual_dpix_, rct, rct);
	}
	update_bars();
	update_display();

	// Make sure we are not displaying a tip for the elt under the mouse as it has just changed
	if (!tip_.FadingOut())   // Don't keep it alive if we have already told it to fade away
		tip_.Hide(300);
}

void CAerialView::StoreOptions(CHexFileList *pfl, int idx)
{
	pfl->SetData(idx, CHexFileList::AERIALDISPLAY, disp_state_);
	pfl->SetData(idx, CHexFileList::AERIALPOS, scrollpos_);
}

void CAerialView::InvalidateRange(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr)
{
	CRect rct;
	// We always invalidate the top border as it shows the current horizontal position/extents
	// of the caret/selection which should be updated even if off screen.
	rct.left = bdr_left_;
	rct.right = bdr_left_ + cols_*actual_dpix_;
	rct.top = 0;
	rct.bottom = bdr_top_;
	InvalidateRect(&rct);

	invalidate_addr_range(start_addr, end_addr);
}

///////////////////////////////////////////////////////////////////////////////
// CAerialView worker (private) methods

// Update the scroll bar based on the current display position (scrollpos_).
// Compare with CScrView::update_bars - but note there is only a vertical
// scroll bar in an aerial view as we "reshape" the bitmap before BLTing
// so that it always fits across the window.
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
		si.nMax = endpos/sbfact_;
		si.nPage = rows_/sbfact_;
		si.nPos = pos/sbfact_;
	}

	SetScrollInfo(SB_VERT, &si);
}

// Calculates the number of elements that can be displayed in the window (returned
// in rows and cols) which is subsequently use in draw_bitmap to "reshape" the bitmap.
// This needs to be called when:
// - window size/shape is changed
// - disp_.dpix is changed causing a different actual_dpix_ value
void CAerialView::get_disp_params(int &rows, int &cols, int &actual_dpix)
{
	// Get the actual display area
	CRect rct;
	GetClientRect(&rct);
	rct.DeflateRect(bdr_left_, bdr_top_, bdr_right_, bdr_bottom_);

	// Increase the actual display pixel size if the whole file fits in the 
	// display areas up to as big as 7 x 7.
	for (actual_dpix = disp_.dpix; actual_dpix <= MAX_DPIX; ++actual_dpix)
	{
		int tmp = cols = (rct.Width()/(8*actual_dpix)) * 8;
		if (cols < 8)
			cols = 8;
		else if (cols > CHexEditDoc::MAX_WIDTH)
			cols = CHexEditDoc::MAX_WIDTH;
		rows = rct.Height()/actual_dpix;
		if (rows < 1) rows = 1;

		// Check if we have expanded the width too far since we always have at least 8 columns
		if (tmp == 0)
			break;

		// Check if we have expanded the size so it would no longer fit in the window
		if ((GetDocument()->NumElts()-1) / cols + 1 > rows)
			break;
	}
	if (actual_dpix > disp_.dpix)
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
	ASSERT(actual_dpix >= disp_.dpix && actual_dpix <= MAX_DPIX);

	// Adjust scrollbar scaling factor as scroll bars seem to be limited to signed 16 bit numbers
	int endy = int((GetDocument()->length()-1)/(GetDocument()->GetBpe() * cols_) + 1);
	// Make sure vertical dimensions do not overflow a signed 32 bit int as
	// the bitmap cannot handle it.
	ASSERT(endy < INT_MAX);
	sbfact_ = endy/30000 + 1;
}

void CAerialView::update_display()
{
	// Find all search occurrences within the display
	search_pair_.clear();
	if (GetDocument()->CanDoSearch() && theApp.pboyer_ != NULL)
	{
		// Cache search occurrences found in the display area
		FILE_ADDRESS end = scrollpos_ + rows_*cols_*GetDocument()->GetBpe();
		size_t len = theApp.pboyer_->length();
		std::vector<FILE_ADDRESS> sf = GetDocument()->SearchAddresses(scrollpos_ - len + 1, end + len);

		std::vector<FILE_ADDRESS>::const_iterator pp = sf.begin();
		std::vector<FILE_ADDRESS>::const_iterator pend = sf.end();
		pair<FILE_ADDRESS, FILE_ADDRESS> good_pair;
		if (pp != pend)
		{
			good_pair.first = *pp;
			good_pair.second = *pp + len;
			while (++pp != pend)
			{
				if (*pp >= good_pair.second)
				{
					search_pair_.push_back(good_pair);
					good_pair.first = *pp;
				}
				good_pair.second = *pp + len;
			}
			search_pair_.push_back(good_pair);
		}
	}
}

// Given a range of addresses in the file, then rectangles that cover all displayed
// addresses are invalidated which (sooner or later) causes those areas to be redrawn.
// Note that this invalidates the inside which is slow for a large area and 
// not necessary for marching ants (see invalidate_addr_range_boundary below).
// Note: We have now uncovered another flaw in Windows invalidation.  When we BLT
//       part of the bitmap we don't want to erase the background as there is no point
//       (since none shows through) but mainly because it causes annoying flashing.
//       But we do want to erase the background for the left border as we just draw
//       indicators as required and the bacground will show through.  However,
//       mixing calls to InvalidateRect with the bErase parameter set to TRUE and
//       FALSE in different calls does not work - all areas have their background
//       erased.  What I think Windows is doing is combining invalidation areas
//       into one bounding rect and if any of the areas is to have the background
//       erased then the whole rect is erased.  This has now been solved by not
//       erasing the bitmap area - just the borders in OnEraseBkgnd.
void CAerialView::invalidate_addr_range(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, bool no_border /*=false*/)
{
	CHexEditDoc *pDoc = GetDocument();
	int width = GetDocument()->GetBpe() * cols_;

	// Restrict to just what is in the display then convert to window device coords
	if (start_addr < scrollpos_) start_addr = scrollpos_;
	if (end_addr > scrollpos_ + width*rows_) end_addr = scrollpos_ + width*rows_;

	// Work out the elts we are invalidating offset from the elt at the top-left
	int start_elt = int((start_addr - scrollpos_)/GetDocument()->GetBpe());
	int end_elt   = int((end_addr - 1 - scrollpos_)/GetDocument()->GetBpe() + 1);
	if (start_elt >= end_elt)
		return;                         // Nothing to invalidate or all outside display

	// Work out line range that needs invalidating
	int start_line = int(start_elt/cols_);
	int end_line   = int(end_elt/cols_);

	CRect rct;

	// If start and end on the same line just invalidate between them
	if (start_line == end_line)
	{
		rct.left   = bdr_left_ + (start_elt%cols_)*actual_dpix_;
		rct.right  = bdr_left_ + (end_elt%cols_)*actual_dpix_;
		rct.top    = bdr_top_ + start_line*actual_dpix_;
		rct.bottom = rct.top + actual_dpix_;
		InvalidateRect(&rct);
		if (!no_border)
		{
			rct.left = 0; rct.right = bdr_left_;    // Do left border too
			InvalidateRect(&rct);
		}
		return;
	}
	// Invalidate partial lines at each end
	rct.left   = bdr_left_ + (start_elt%cols_)*actual_dpix_;
	rct.right  = bdr_left_ + cols_*actual_dpix_;
	rct.top    = bdr_top_ + start_line*actual_dpix_;
	rct.bottom = rct.top + actual_dpix_;
	InvalidateRect(&rct);
	if (!no_border)
	{
		rct.left = 0; rct.right = bdr_left_;        // Do left border too
		InvalidateRect(&rct);
	}

	rct.left   = bdr_left_;
	rct.right  = bdr_left_ + (end_elt%cols_)*actual_dpix_;
	rct.top    = bdr_top_ + end_line*actual_dpix_;
	rct.bottom = rct.top + actual_dpix_;
	InvalidateRect(&rct);
	if (!no_border)
	{
		rct.left = 0; rct.right = bdr_left_;  // Do left border too
		InvalidateRect(&rct);
	}

	// If more than one line between start and end then invalidate that block too
	if (start_line + 1 < end_line)
	{
		rct.left   = bdr_left_;
		rct.right  = bdr_left_ + cols_*actual_dpix_;
		rct.top    = bdr_top_ + start_line*actual_dpix_;
		rct.bottom = bdr_top_ + end_line*actual_dpix_;
		InvalidateRect(&rct);
		if (!no_border)
		{
			rct.left = 0; rct.right = bdr_left_;
			InvalidateRect(&rct);
		}
	}
}

// Similar to invalidate_addr_range but only used for marching ants.
// Note that this is a real mess due to the way Windows invalidation works.
// There are 2 situations that need different handling:
// 1. If we have a large area to draw ants around then we can't just
//    invalidate the bounding rect since this will cause a large (ie slow)
//    BitBlt in OnDraw.  We don't need or want to redraw the inside of
//    the area.  Even if we only invalidate the changed areas
//    (ie we invalidate just the borders) Windows helpfully combines
//    them into one rectangle for the next WM_PAINT event.  The
//    solution in this case is to invalidate the border areas
//    then call UpdateWindow for each one to force an immediate redraw.
// 2. BUT if we have lots of small areas to draw the ants in then calling
//    UpdateWindow 4 times for each area causes many repaints (ie calls
//    to OnDraw) which has not insignificant overhead per call.  This
//    slows the whole system (not just the ants) to a crawl.  In this
//    case it's probably faster to just invalidate the bounding box and
//    redraw it all at once.
void CAerialView::invalidate_addr_range_boundary(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr)
{
	CHexEditDoc *pDoc = GetDocument();
	int width = GetDocument()->GetBpe() * cols_;

	// Restrict to just what is in the display then convert to window device coords
	if (start_addr < scrollpos_) start_addr = scrollpos_;
	if (end_addr > scrollpos_ + width*rows_) end_addr = scrollpos_ + width*rows_;

	// Work out the elts we are invalidating offset from the elt at the top-left
	int start_elt = int((start_addr - scrollpos_)/GetDocument()->GetBpe());
	int end_elt   = int((end_addr - 1 - scrollpos_)/GetDocument()->GetBpe() + 1);
	if (start_elt >= end_elt)
		return;                         // Nothing to invalidate or all outside display

	// Work out line range that needs invalidating
	int start_line = int(start_elt/cols_);
	int end_line   = int(end_elt/cols_);

	CRect rct;

	// If we have a small number of lines to invalidate we just do
	// normal invalidation (see 2 in note above).
	if (start_line + 3 >= end_line)
	{
		invalidate_addr_range(start_addr, end_addr, true);  // Don't invalidate the border as it hasn't changed
		return;
	}

	// Invalidate and draw the four sides.
	// Note that we call UpdateWindow to force an immediate redraw otherwise Windows
	// combines them into one big rect which is slow (like invalidate_addr_range).
	// Also note that the marching ants can now be up to 4 pixels wide which
	// explains the offsets of 4 below.

	// Top
	rct.left   = bdr_left_;
	rct.right  = bdr_left_ + cols_*actual_dpix_;
	rct.top    = bdr_top_ + start_line*actual_dpix_;
	rct.bottom = rct.top + actual_dpix_ + 4;
	InvalidateRect(&rct, FALSE);
	UpdateWindow();
	// Bottom
	rct.left   = bdr_left_;
	rct.right  = bdr_left_ + cols_*actual_dpix_;
	rct.top    = bdr_top_ + end_line*actual_dpix_ - 4;
	rct.bottom = bdr_top_ + (end_line+1)*actual_dpix_;
	InvalidateRect(&rct, FALSE);
	UpdateWindow();
	// Left
	rct.left   = bdr_left_;
	rct.right  = rct.left + 4;
	rct.top    = bdr_top_ + start_line*actual_dpix_;
	rct.bottom = bdr_top_ + (end_line+1)*actual_dpix_;
	InvalidateRect(&rct, FALSE);
	UpdateWindow();
	// Right
	rct.right  = bdr_left_ + cols_*actual_dpix_;
	rct.left   = rct.right - 4;                         // 4 pixels less than right
	rct.top    = bdr_top_ + start_line*actual_dpix_;
	rct.bottom = bdr_top_ + (end_line+1)*actual_dpix_;
	InvalidateRect(&rct, FALSE);
	UpdateWindow();
}

// Redraws the part of the bitmap that need to be shown in the window.
// The source (FreeImage) bitmap is reshaped to fit across the display:
// 1. The width (bih->biWidth) is modified so that the correct number of
//    elements for the client window width is used.
// Note that this relies on the bitmap width always being a multiple
// of 4 (actually we use 8 for possible future compatibility) so that there
// are no pad bytes on the end of the scan lines.
// 2. The height (bih->biHeight) is calculated so that the bitmap size
//    includes all elements in the bitmap.
// Note that this means that the actual bitmap size varies depending on
// the width, but that has been allowed for by allocating a bit of extra
// space at the end (see CHexEditDoc::AddAerialView).

void CAerialView::draw_bitmap(CDC* pDC)
{
	CHexEditDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL && pDoc->dib_ != NULL);
	if (pDoc == NULL || pDoc->dib_ == NULL)
		return;

	BITMAPINFOHEADER *bih = FreeImage_GetInfoHeader(pDoc->dib_);
	ASSERT(bih->biCompression == BI_RGB && bih->biHeight > 0);

	// Work out how to "reshape" the bitmap based cols_ (already calced from width of the window and dpix)
	// Note: changing the bitmap header (ie width/height) should only be done just before BLTing
	// the bitmap as more than one view can use the same bitmap and they may require different "shapes".
	bih->biWidth = cols_;
	ASSERT(cols_ % 8 == 0 && cols_ > 0 && cols_ <= CHexEditDoc::MAX_WIDTH);
	ASSERT((pDoc->length_-1)/bih->biWidth < LONG_MAX);
	bih->biHeight = LONG((pDoc->NumElts()-1)/bih->biWidth) + 1;

	int pos = int(scrollpos_/(GetDocument()->GetBpe() * cols_));    // no of rows above top of window
	CRect cliprct; pDC->GetClipBox(&cliprct);                       // only BLT the area that needs it

	// Work out src rect
	int sl = (cliprct.left - bdr_left_)/actual_dpix_;
	if (sl < 0) sl = 0;
	int st = pos + (cliprct.top - bdr_top_)/actual_dpix_;
	if (st < 0) st = 0;
	int sr = (cliprct.right - bdr_left_ - 1)/actual_dpix_ + 1;
	if (sr > bih->biWidth) sr = bih->biWidth;
	int sb = pos + (cliprct.bottom - bdr_top_ - 1)/actual_dpix_ + 1;
	if (sb > bih->biHeight) sb = bih->biHeight;

	// Work out dest rect
	int dl = bdr_left_ + sl*actual_dpix_;
	int dt = bdr_top_ + (st - pos)*actual_dpix_;
	int dr = bdr_left_ + sr*actual_dpix_;
	int db = bdr_top_ + (sb - pos)*actual_dpix_;

	::StretchDIBits(pDC->GetSafeHdc(),
					dl, dt, dr - dl, db - dt,
					sl, sb + 1, sr - sl, st - sb,
					FreeImage_GetBits(pDoc->dib_), FreeImage_GetInfo(pDoc->dib_),
					DIB_RGB_COLORS, SRCCOPY);
}

// Draws "marching ants" to highlight a range if addresses in the display.
// If this is a tiny it just shows ants marching along a line of the display,
// otherwise it displays ants marching around the boundary.
// The "marching" affect is achieved using a timer that updates the display
// and cycles the timer_count_ value which determines where the ants are.
void CAerialView::draw_ants(CDC* pDC, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, COLORREF clr)
{
	int width = GetDocument()->GetBpe() * cols_;
	ASSERT(scrollpos_ % width == 0);

	if (end_addr <= scrollpos_ || start_addr >= scrollpos_ + width*rows_)
		return;  // all outside the display

	// We draw ants in 2 colours so they are visible on any background colour
	COLORREF clr1, clr2;
	get_colours(clr, clr1, clr2);

	// Constrain  the addresses to those that might affect the display.
	// (We need to go a row above and below the actual displayed rows.)
	if (start_addr < scrollpos_ - width) start_addr = scrollpos_ - width;
	if (end_addr > scrollpos_ + width*(rows_+1)) end_addr = scrollpos_ + width*(rows_+1);

	t1_.restart();
	if (actual_dpix_ == 1 && end_addr - start_addr < 2*width)
		draw_lines(pDC, start_addr, end_addr, clr1, clr2);       // Just draw a row of ants as there's not even 2 rows of pixels
	else
		draw_bounds(pDC, start_addr, end_addr, clr1, clr2);
	t1_.stop();
}

void CAerialView::draw_lines(CDC* pDC, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, COLORREF clr1, COLORREF clr2)
{
	ASSERT(actual_dpix_ == 1);  // Only really to be used in this case (+ it makes calcs slightly easier)
	int ss = int((start_addr - scrollpos_)/GetDocument()->GetBpe());       // pixel starting from top left, moving acros by rows then down
	int ee = int((end_addr - scrollpos_)/GetDocument()->GetBpe());

	for (int cc = ss; cc < ee; ++cc)
		draw_pixel(pDC, cc, cc%cols_, cc/cols_, clr1, clr2);
}

void CAerialView::draw_bounds(CDC* pDC, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, COLORREF clr1, COLORREF clr2)
{
	int width = GetDocument()->GetBpe() * cols_;
	// Make sure we have something to show and it is all within the display area
	ASSERT(start_addr < end_addr);

	// Work out the elts involved offset from the elt at the top-left of the display
	int start_elt = int((start_addr - scrollpos_)/GetDocument()->GetBpe());
	int end_elt   = int((end_addr - 1 - scrollpos_)/GetDocument()->GetBpe() + 1);
	ASSERT(start_elt < end_elt);

	// Work out the bounding box in elts (for pixels we still have to multiply by actual_dpix_)
	CRect rct;
	rct.left   = 0;                     // Default to left side
	rct.right  = cols_;                 // Default to right side
	rct.top    = start_elt/cols_;       // Line at top of this row of elts
	rct.bottom = (end_elt-1)/cols_;     // Line at bottom of this row of elts
	if (start_elt < 0) rct.top--;  // Compensate if start row is just above top of window

	// Check for a split over 2 lines that results in 2 separate areas
	if (rct.top + 1 == rct.bottom && (end_elt - start_elt) <= cols_)
	{
		// Draw the 2 areas separately
		draw_ants(pDC, start_addr, ((end_addr-1)/width)*width, clr1);
		draw_ants(pDC, ((end_addr-1)/width)*width, end_addr, clr1);
		return;
	}

	// Work out left indent of first line and right indent of last line
	int left_indent = (start_elt+cols_)%cols_;     // Add cols_ before modulus to avoid modulus of -ve
	int right_indent = (end_elt-1)%cols_ + 1;
	// Check if we just have one row of elts
	if (rct.top == rct.bottom)
	{
		// It is just a simple rectangle with width possibly narrower than cols_
		rct.left = left_indent;
		rct.right = right_indent;
	}

	// Work out the boundary length in pixels
	// Note we subtract 4 for the 4 corners (because we are drawing inside the bounds on bottom and right)
	int blength = rct.Width()*actual_dpix_*2 + (rct.Height()+1)*actual_dpix_*2 - 4;

	// We also need a factor to adjust the moving ants position as blength may not be a multiple of 8
	bfactor_ = (((blength-ANT_SIZE/2)/ANT_SIZE + 1)*ANT_SIZE)/double(blength);
	int pnum = 0;       // Current pixel we are drawing

	// Draw top line
	for (int x = left_indent*actual_dpix_; x < rct.right*actual_dpix_; ++x)
		draw_pixel(pDC, pnum++, x, rct.top*actual_dpix_, clr1, clr2, 0, 1);

	// Draw right side
	if (rct.right == right_indent)
	{
		// Draw simple right side
		for (int y = rct.top*actual_dpix_ + 1; y < (rct.bottom+1)*actual_dpix_; ++y)
			draw_pixel(pDC, pnum++, rct.right*actual_dpix_ - 1, y, clr1, clr2, -1, 0);
	}
	else
	{
		// Draw right side down to indent part
		int x, y;
		for (y = rct.top*actual_dpix_ + 1; y < rct.bottom*actual_dpix_; ++y)
			draw_pixel(pDC, pnum++, rct.right*actual_dpix_ - 1, y, clr1, clr2, -1, 0);

		// Draw horizontal line for indent part
		for (x = rct.right*actual_dpix_ - 2; x >= right_indent*actual_dpix_ - 1; x--)
			draw_pixel(pDC, pnum++, x, y - 1, clr1, clr2, 0, -1);

		// Draw vertical line for indent part
		for ( ; y < (rct.bottom+1)*actual_dpix_; ++y)
			draw_pixel(pDC, pnum++, right_indent*actual_dpix_ - 1, y, clr1, clr2, -1, 0);
	}

	// Draw bottom line
	for (int x = right_indent*actual_dpix_ - 2; x >= rct.left*actual_dpix_; x--)
		draw_pixel(pDC, pnum++, x, (rct.bottom+1)*actual_dpix_ - 1, clr1, clr2, 0, -1);

	// Draw left side
	if (rct.left == left_indent)
	{
		// Draw simple left side
		for (int y = (rct.bottom+1)*actual_dpix_ - 2; y > rct.top*actual_dpix_; y--)
			draw_pixel(pDC, pnum++, rct.left*actual_dpix_, y, clr1, clr2, 1, 0);
	}
	else
	{
		// Draw left side up to indent part
		int x, y;
		for (y = (rct.bottom+1)*actual_dpix_ - 2; y >= (rct.top+1)*actual_dpix_; y--)
			draw_pixel(pDC, pnum++, rct.left*actual_dpix_, y, clr1, clr2, 1, 0);

		// Draw horizontal line for indent part
		for (x = rct.left*actual_dpix_ + 1; x <= left_indent*actual_dpix_; ++x)
			draw_pixel(pDC, pnum++, x, y + 1, clr1, clr2, 0, 1);

		// Draw vertical line for indent part
		for ( ; y > rct.top*actual_dpix_; y--)
			draw_pixel(pDC, pnum++, left_indent*actual_dpix_, y, clr1, clr2, 1, 0);
	}
	ASSERT(pnum == blength);
}

// Draw a pixel of a marching ants display on top of the bitmap.
// This uses pnum (param 2) and the current value of timer_count_ (see OnTimer)
// to decide the colour of the pixels.
// x and y are the coordinates of the pixel (device coords) offset from top/left border.
void CAerialView::draw_pixel(CDC* pDC, int pnum, int x, int y, COLORREF clr1, COLORREF clr2, int horz /*=0*/, int vert)
{
	// Work out which pixel of the ant we are drawing.
	// Note we add ANT_SIZE below as modulus (%) does not behave as expected for -ve numbers.
	int anum = (int(pnum*bfactor_) + ANT_SIZE - timer_count_)%ANT_SIZE;
	if (actual_dpix_ < 7)
	{
		// When zoomed out quickly draw a single pixel
		if (anum < 4)
			pDC->SetPixel(bdr_left_ + x, bdr_top_ + y, clr1);
		else if (anum >= 6 && anum < 10)
			pDC->SetPixel(bdr_left_ + x, bdr_top_ + y, clr2);
		// else we do not drawn anything (transparent)
	}
	else
		for (int ii = 0; ii < actual_dpix_/4; ++ii)
		{
			if (anum < 4)
				pDC->SetPixel(bdr_left_ + x + ii*horz, bdr_top_ + y + ii*vert, clr1);
			else if (anum >= 6 && anum < 10)
				pDC->SetPixel(bdr_left_ + x + ii*horz, bdr_top_ + y + ii*vert, clr2);
			// else we do not drawn anything (transparent)
		}
}

// Draw a rectangular block of indicator pixels in the left border
// Horizontally its drawn from the pixel at left to the one before right
// Vertically it is drawn at the row (in elts not pixels), the height
// of which is determined by actual_dpix_.
void CAerialView::draw_left_border(CDC* pDC, int left, int right, int row, COLORREF clr)
{
	ASSERT(left < right && right <= bdr_left_);
	for (int rr = 0; rr < actual_dpix_; ++rr)
		for (int cc = left; cc < right; ++cc)
			pDC->SetPixel(cc, bdr_top_ + row*actual_dpix_ + rr, clr);
}

// Draws a rectangular block at the top of the window
void CAerialView::draw_top_border(CDC* pDC, int col1, int col2, COLORREF clr)
{
	int nrows = bdr_top_;
	if ((col2 - col1)*actual_dpix_ > 2)
		nrows--;             // don't make it too big
	ASSERT(nrows <= bdr_top_);
	for (int rr = 0; rr < nrows; ++rr)
		for (int cc = col1*actual_dpix_; cc < col2*actual_dpix_; ++cc)
			pDC->SetPixel(bdr_left_ + cc, rr, clr);
}

// Get a dark and light colours of the same hue.
void CAerialView::get_colours(COLORREF clr, COLORREF & clr_dark, COLORREF & clr_light)
{
	clr_dark = clr_light = clr;         // use input as defaults
	int hh, ll, ss; get_hls(clr, hh, ll, ss);
	if (ll > 65)
		clr_dark = get_rgb(hh, 35, ss);
	else if (ll < 35)
		clr_light = get_rgb(hh, 65, ss);
	else
	{
		clr_dark = get_rgb(hh, 35, ss);
		clr_light = get_rgb(hh, 65, ss);
	}
}

// Get elt at point pt (client coords)
int CAerialView::elt_at(CPoint pt)
{
	// Convert to elt coords
	pt.x = (pt.x - bdr_left_)/actual_dpix_;
	pt.y = (pt.y - bdr_top_)/actual_dpix_;

	// First make sure it is within the display area
	if (pt.x < 0 || pt.x >= cols_ || pt.y < 0 || pt.y >= rows_)
		return -1;

	// Work out absolute elt in the file
	int elt = int(scrollpos_/GetDocument()->GetBpe()) + pt.y * cols_ + pt.x;
	if (elt < 0 || elt >= GetDocument()->NumElts())
		return -1;
	else
		return elt;
}

void CAerialView::set_zoom(int z, bool scroll /*=true*/)
{
	ASSERT(z > 0 && z <= MAX_DPIX);
	disp_.dpix = z;

	int prev_dpix = actual_dpix_;
	get_disp_params(rows_, cols_, actual_dpix_);

	// Check if the display has changed at all
	if (scroll && actual_dpix_ != prev_dpix)
	{
		int newpos = int(scrollpos_/(GetDocument()->GetBpe() * cols_));
		// scrollpos_ is now invalid (recalculated in SetScroll below).  However, we need
		// to set it to this value to force SetScroll to redraw and not scroll the window.
		// We can't just invalidate the display here as scrollpos_ is inconsistent with cols_.
		scrollpos_ = INT_MIN;
		SetScroll(newpos);
	}
}

// Returns true if tip text was updated or false if there is nothing to show.
// The parameter (elt) if the elt about which we show information
bool CAerialView::update_tip(int elt)
{
	tip_elt_ = elt;
	tip_.Clear();
	FILE_ADDRESS addr = FILE_ADDRESS(elt)*GetDocument()->GetBpe();

	ASSERT(phev_ != NULL);
	if (phev_->DecAddresses())
	{
		// Show dec then hex address in the tip
		dec_addr_tip(addr);
		hex_addr_tip(addr);
	}
	else
	{
		// Show hex then decimal address in the tip
		hex_addr_tip(addr);
		dec_addr_tip(addr);
	}

	tip_.SetAlpha(theApp.tip_transparency_);
	return true;
}

void CAerialView::hex_addr_tip(FILE_ADDRESS addr)
{
	CString ss;
	char buf[32];                   // used with sprintf (CString::Format can't handle __int64)
	if (theApp.hex_ucase_)
		sprintf(buf, "%I64X", addr);
	else
		sprintf(buf, "%I64x", addr);
	ss = buf;
	AddSpaces(ss);
	tip_.AddString(ss, phev_->GetHexAddrCol());
}

void CAerialView::dec_addr_tip(FILE_ADDRESS addr)
{
	CString ss;
	char buf[32];                   // used with sprintf (CString::Format can't handle __int64)
	sprintf(buf, "%I64d", addr);
	ss = buf;
	AddCommas(ss);
	tip_.AddString(ss, phev_->GetDecAddrCol());
}

void CAerialView::track_mouse(unsigned long flag)
{
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.dwFlags = flag;
	tme.dwHoverTime = 0;
	tme.hwndTrack = m_hWnd;

	VERIFY(::_TrackMouseEvent(&tme));
}
